/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mirror.c
 * Creation Date:   May 15, 2013 (from fm4000_api_mirror.c)
 * Description:     Structures and functions for dealing with mirrors.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Simple helper macro that counts and returns the number of bits set in
 * value. */
#define FM_COUNT_SET_BITS(value, bits)                                         \
    {                                                                          \
        fm_int modifValue = value;                                             \
        for ((bits) = 0; modifValue; (bits)++)                                 \
        {                                                                      \
            modifValue &= modifValue - 1;                                      \
        }                                                                      \
    }

#define MIRROR_RULE(group, index) \
        ( ( (group) * FM10000_MIRROR_TRIG_GROUP_SIZE ) + (index) )

#define NUM_FFU_MASK_VALUES \
        (FM_FIELD_UNSIGNED_MAX(FM10000_TRIGGER_CONDITION_FFU, FFU_Mask) + 1)

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** HandleMirrorFfuRes
 * \ingroup intMirror
 *
 * \desc            This function properly handle the Mirror Trigger resource
 *                  between the different mirror group available for ACL
 *                  filtering.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleMirrorFfuRes(fm_int sw)
{
    fm_switch *                switchPtr;
    fm10000_switch *           switchExt;
    fm_portMirrorGroup *       grp;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_int                     group;
    fm_int                     ffuResId;
    fm_uint32                  bitValue;
    fm_int                     i;
    fm_status                  err = FM_OK;
    fm_int                     newGrp = -1;
    fm_int                     activeGrp = 0;
    fm_int                     maxGrp = 0;
    fm_int                     oldFfuResMask;
    fm_int                     newFfuResMask = 0;
    fm_bool                    ffuResIdUsed[NUM_FFU_MASK_VALUES] = {FALSE};

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Go over all the Mirror Group and compute currently used FFU Resource Id.
     * This loops also compute the number of active groups that needs FFU
     * Resource Id and execute some validation. */
    for (group = 0 ; group < FM10000_MAX_MIRRORS_GRP ; group++)
    {
        grp = &switchPtr->mirrorGroups[group];

        if (grp->used)
        {
            grpExt = &switchExt->mirrorGroups[group];

            if (grp->ffuFilter)
            {
                /* High level configuration inform that FFU resource should
                 * be used but was still not being allocated. */
                if (grpExt->ffuResIdMask == FM10000_MIRROR_NO_FFU_RES)
                {
                    newGrp = group;
                }
                else
                {
                    /* Every FFU Resource Mask must be the same */
                    if (switchExt->mirrorFfuResMask != grpExt->ffuResIdMask)
                    {
                        err = FM_FAIL;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
                    }

                    /* Burn this Resource Id */
                    ffuResIdUsed[grpExt->ffuResId] = TRUE;
                    activeGrp++;

                    /* Used to remove unused bit */
                    newFfuResMask |= grpExt->ffuResId;
                }
            }
            else
            {
                grpExt->ffuResId     = FM10000_MIRROR_NO_FFU_RES;
                grpExt->ffuResIdMask = FM10000_MIRROR_NO_FFU_RES;
            }
        }
    }

    oldFfuResMask = switchExt->mirrorFfuResMask;

    /* Is it possible to remove one of the FFU trigger bit? */
    if (switchExt->mirrorFfuResMask != newFfuResMask)
    {
        if ( (switchExt->mirrorFfuResMask | newFfuResMask) != switchExt->mirrorFfuResMask )
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }

        /* Remove all the unused bit */
        for (i = 0 ; i < FM10000_TRIGGER_FFU_TRIG_ID_BITS ; i++)
        {
            if ( (switchExt->mirrorFfuResMask & (1 << i)) &&
                 ((newFfuResMask & (1 << i)) == 0) )
            {
                err = fm10000FreeTriggerResource(sw,
                                                 FM_TRIGGER_RES_FFU,
                                                 i,
                                                 TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

                switchExt->mirrorFfuResMask &= ~(1 << i);
            }
        }

        if (newFfuResMask != switchExt->mirrorFfuResMask)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }

        err = fm10000UpdateAclTriggerMask(sw,
                                          oldFfuResMask,
                                          switchExt->mirrorFfuResMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        /* Trigger Condition must also be updated to now includes the
         * updated FFU Resource Mask. */
        for (group = 0 ; group < FM10000_MAX_MIRRORS_GRP ; group++)
        {
            grp = &switchPtr->mirrorGroups[group];

            if (grp->used)
            {
                grpExt = &switchExt->mirrorGroups[group];

                if ( grp->ffuFilter &&
                    (grpExt->ffuResIdMask != FM10000_MIRROR_NO_FFU_RES) )
                {
                    grpExt->ffuResIdMask = switchExt->mirrorFfuResMask;

                    err = fm10000WritePortMirrorGroup(sw, grp);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
                }
            }
        }

        oldFfuResMask = switchExt->mirrorFfuResMask;
    }

    /* No new FFU Resource to be allocated */
    if (newGrp == -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
    }

    /* First Mirror Group that needs FFU Resource Id */
    if (switchExt->mirrorFfuResMask == 0)
    {
        err = fm10000AllocateTriggerResource(sw,
                                             FM_TRIGGER_RES_FFU,
                                             &bitValue,
                                             TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        switchExt->mirrorFfuResMask = 1 << bitValue;
    }
    /* Active Mirror Group already uses FFU Resource */
    else
    {
        FM_COUNT_SET_BITS(oldFfuResMask, bitValue);

        maxGrp = (1 << bitValue) - 1;

        /* Can't find free FFU Resource Id under the original mask, new bit
         * are needed... */
        if (maxGrp <= activeGrp)
        {
            err = fm10000AllocateTriggerResource(sw,
                                                 FM_TRIGGER_RES_FFU,
                                                 &bitValue,
                                                 TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

            switchExt->mirrorFfuResMask |= (1 << bitValue);
        }
    }

    /* FFU Resource Mask was updated to cover more active mirror groups
     * that extend filtering with ACLs. All ACL/rule that set FFU Trigger
     * ID must be updated with the new mask. */
    if ( (oldFfuResMask != 0) &&
         (switchExt->mirrorFfuResMask != oldFfuResMask) )
    {
        err = fm10000UpdateAclTriggerMask(sw,
                                          oldFfuResMask,
                                          switchExt->mirrorFfuResMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        /* Trigger Condition must also be updated to now includes the
         * updated FFU Resource Mask. */
        for (group = 0 ; group < FM10000_MAX_MIRRORS_GRP ; group++)
        {
            grp = &switchPtr->mirrorGroups[group];

            if (grp->used)
            {
                grpExt = &switchExt->mirrorGroups[group];

                if ( grp->ffuFilter &&
                    (grpExt->ffuResIdMask != FM10000_MIRROR_NO_FFU_RES) )
                {
                    grpExt->ffuResIdMask = switchExt->mirrorFfuResMask;

                    err = fm10000WritePortMirrorGroup(sw, grp);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
                }
            }
        }
    }

    grpExt = &switchExt->mirrorGroups[newGrp];

    /* Find a Free FFU Resource Id position for the new group. */
    for (ffuResId = 1 ; ffuResId < NUM_FFU_MASK_VALUES ; ffuResId++)
    {
        if ( (ffuResIdUsed[ffuResId] == FALSE) &&
             ((ffuResId | switchExt->mirrorFfuResMask) == switchExt->mirrorFfuResMask) )
        {
            grpExt->ffuResId = ffuResId;
            grpExt->ffuResIdMask = switchExt->mirrorFfuResMask;
            break;
        }
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end HandleMirrorFfuRes */




/*****************************************************************************/
/** GetMirrorCounters 
 * \ingroup intMirror
 *
 * \desc            Retrive ingress and egress mirror counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp is the Group ID for which counter to be retrieved.
 *
 * \param[out]      ingressCnt  is the user-allocated storage in which ingress
 *                  mirror counter value is stored.
 *
 * \param[out]      egressCnt  is the user-allocated storage in which egress
 *                  mirror counter value is stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetMirrorCounters(fm_int              sw,
                                   fm_portMirrorGroup *grp,
                                   fm_uint64          *ingressCnt,
                                   fm_uint64          *egressCnt)
{
    fm_status       status;
    fm_int          group;

    group = grp->groupId;

    switch (grp->mirrorType)
    {
        case FM_MIRROR_TYPE_INGRESS:
        case FM_MIRROR_TYPE_REDIRECT:
            status = fm10000GetTriggerAttribute(sw,
                                                FM10000_TRIGGER_GROUP_MIRROR,
                                                MIRROR_RULE(group, 0),
                                                FM_TRIGGER_ATTR_COUNTER,
                                                (void *)ingressCnt);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);
            *egressCnt = 0;
            break;

        case FM_MIRROR_TYPE_EGRESS:
            status = fm10000GetTriggerAttribute(sw,
                                                FM10000_TRIGGER_GROUP_MIRROR,
                                                MIRROR_RULE(group, 0),
                                                FM_TRIGGER_ATTR_COUNTER,
                                                (void *)egressCnt);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);
            *ingressCnt = 0;
            break;

        case FM_MIRROR_TYPE_BIDIRECTIONAL:
            status = fm10000GetTriggerAttribute(sw,
                                                FM10000_TRIGGER_GROUP_MIRROR,
                                                MIRROR_RULE(group, 0),
                                                FM_TRIGGER_ATTR_COUNTER,
                                                (void *)ingressCnt);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);

            status = fm10000GetTriggerAttribute(sw,
                                                FM10000_TRIGGER_GROUP_MIRROR,
                                                MIRROR_RULE(group, 1),
                                                FM_TRIGGER_ATTR_COUNTER,
                                                (void *)egressCnt);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);
            break;

        case FM_MIRROR_TYPE_TX_EGRESS:
        case FM_MIRROR_TYPE_RX_INGRESS_TX_EGRESS:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, status);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, status);

}  /* end GetMirrorCounters */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000MirrorInit
 * \ingroup intMirror
 *
 * \desc            This function is called during switch initialization to
 *                  prepare for mirror support.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MirrorInit(fm_int sw)
{
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    /* Initialize the mirror group table */
    FM_CLEAR(switchExt->mirrorGroups);

    switchExt->mirrorFfuResMask = 0;

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_OK);

}   /* end fm10000MirrorInit */




/*****************************************************************************/
/** fm10000GetMirrorId
 * \ingroup intMirror
 *
 * \desc            Retrieve the FFU Resource information from a Group
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the Group ID to retrieve.
 * 
 * \param[out]      ffuResId is the FFU Trigger Resource allocated.
 * 
 * \param[out]      ffuResIdMask is the FFU Trigger Mask Resource allocated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMirrorId(fm_int  sw,
                             fm_int  group,
                             fm_int *ffuResId,
                             fm_int *ffuResIdMask)
{
    fm_switch *                 switchPtr;
    fm10000_switch *            switchExt;
    fm_portMirrorGroup *        grp;
    fm_fm10000PortMirrorGroup * grpExt;
    fm_status                   err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, group = %d\n",
                  sw,
                  group);

    if ( (group >= FM10000_MAX_MIRRORS_GRP) ||
         (group < 0) )
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    grp       = &switchPtr->mirrorGroups[group];
    grpExt    = &switchExt->mirrorGroups[group];

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }
    else
    {
        *ffuResId = grpExt->ffuResId;
        *ffuResIdMask = grpExt->ffuResIdMask;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000GetMirrorId */




/*****************************************************************************/
/** fm10000CreateMirror
 * \ingroup intMirror
 *
 * \desc            Creates a port mirror group.
 *                  Called through the CreateMirror function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp points to the mirror group structure
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CreateMirror(fm_int              sw,
                              fm_portMirrorGroup *grp)
{
    fm_int                     group;
    fm10000_switch *           switchExt;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_int                     portSet;
    fm_uint32                  mirrorProfile;
    fm_char                    trigName[FM10000_MIRROR_TRIG_NAME_SIZE];
    fm_status                  err = FM_OK;
    fm_bool                    mirGrpRuleInit[FM10000_MIRROR_TRIG_GROUP_SIZE] = {FALSE};
    fm_bool                    mirRxPortSetInit = FALSE;
    fm_bool                    mirTxPortSetInit = FALSE;
    fm_bool                    mirProfileInit = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d)\n",
                  sw,
                  (void *) grp,
                  grp->groupId );

    group     = grp->groupId;
    switchExt = GET_SWITCH_EXT(sw);
    grpExt    = &switchExt->mirrorGroups[group];

    switch (grp->mirrorType)
    {
        case FM_MIRROR_TYPE_INGRESS:
        case FM_MIRROR_TYPE_EGRESS:
        case FM_MIRROR_TYPE_REDIRECT:
        case FM_MIRROR_TYPE_BIDIRECTIONAL:
            break;

        case FM_MIRROR_TYPE_TX_EGRESS:
        case FM_MIRROR_TYPE_RX_INGRESS_TX_EGRESS:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_UNSUPPORTED);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);

    }   /* end switch (grp->mirrorType) */

    /* Each Mirror Groups uses 1 Trigger for each direction so all types uses
     * only one trigger except for the mirror type bidirectional that make uses 
     * of 2 to cover ingress and egress direction. */
    if (grp->mirrorType == FM_MIRROR_TYPE_EGRESS)
    {
        FM_SPRINTF_S(trigName,
                     sizeof(trigName),
                     "MirrorEgressTrig[%d]", group);
    }
    else
    {
        FM_SPRINTF_S(trigName,
                     sizeof(trigName),
                     "MirrorIngressTrig[%d]", group);
    }

    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_MIRROR,
                               MIRROR_RULE(group, 0),
                               TRUE,
                               trigName);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    mirGrpRuleInit[0] = TRUE;

    if (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL)
    {
        FM_SPRINTF_S(trigName,
                     sizeof(trigName),
                     "MirrorEgressTrig[%d]",
                     group);
        err = fm10000CreateTrigger(sw,
                                   FM10000_TRIGGER_GROUP_MIRROR,
                                   MIRROR_RULE(group, 1),
                                   TRUE,
                                   trigName);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        mirGrpRuleInit[1] = TRUE;
    }

    /* Each group consumes one mirror profile. The Mapping between Trigger
     * Id and Mirror Profile is arbitrary. */
    err = fm10000AllocateTriggerResource(sw,
                                         FM_TRIGGER_RES_MIRROR_PROFILE,
                                         &mirrorProfile,
                                         TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    mirProfileInit = TRUE;

    grpExt->rxPortSet = FM_PORT_SET_ALL;
    grpExt->txPortSet = FM_PORT_SET_ALL;

    /* PortSet are allocated at creation and those are updated every time
     * Ingress or Egress ports are added/removed. */
    if ( (grp->mirrorType == FM_MIRROR_TYPE_INGRESS) ||
         (grp->mirrorType == FM_MIRROR_TYPE_REDIRECT) ||
         (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL) )
    {
        err = fmCreatePortSetInt(sw, &portSet, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        grpExt->rxPortSet = portSet;
        mirRxPortSetInit = TRUE;
    }

    if ( (grp->mirrorType == FM_MIRROR_TYPE_EGRESS) ||
         (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL) )
    {
        err = fmCreatePortSetInt(sw, &portSet, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        grpExt->txPortSet = portSet;
        mirTxPortSetInit = TRUE;
    }

    grpExt->mirrorProfile = mirrorProfile;
    grpExt->ffuResId = FM10000_MIRROR_NO_FFU_RES;
    grpExt->ffuResIdMask = FM10000_MIRROR_NO_FFU_RES;
    grpExt->vlanResId = FM10000_MIRROR_NO_VLAN_RES;

ABORT:

    /* Free resource allocation on failure */
    if (err != FM_OK)
    {
        if (mirGrpRuleInit[0])
        {
            fm10000DeleteTrigger(sw,
                                 FM10000_TRIGGER_GROUP_MIRROR,
                                 MIRROR_RULE(group, 0),
                                 TRUE);
        }

        if (mirGrpRuleInit[1])
        {
            fm10000DeleteTrigger(sw,
                                 FM10000_TRIGGER_GROUP_MIRROR,
                                 MIRROR_RULE(group, 1),
                                 TRUE);
        }

        if (mirProfileInit)
        {
            fm10000FreeTriggerResource(sw,
                                       FM_TRIGGER_RES_MIRROR_PROFILE,
                                       mirrorProfile,
                                       TRUE);
        }

        if (mirRxPortSetInit)
        {
            fmDeletePortSetInt(sw, grpExt->rxPortSet);
        }

        if (mirTxPortSetInit)
        {
            fmDeletePortSetInt(sw, grpExt->txPortSet);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000CreateMirror */




/*****************************************************************************/
/** fm10000DeleteMirror
 * \ingroup intMirror
 *
 * \desc            Deletes a port mirror group.
 *                  Called through the DeleteMirror function pointer.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp points to the mirror group structure
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMirror(fm_int              sw,
                              fm_portMirrorGroup *grp)
{
    fm10000_switch *           switchExt;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_int                     group;
    fm_status                  err = FM_OK;
    fm_uint64                  vlanID;
    fm10000_vlanEntry *        ventryExt;
    fm_treeIterator            itVlan;
    void *                     nextValue;
    fm_uint32                  regVals[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_bool                    regLockTaken = FALSE;
    fm_bool                    referenced = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d)\n",
                  sw,
                  (void *) grp,
                  grp->groupId );

    group     = grp->groupId;
    switchExt = GET_SWITCH_EXT(sw);
    grpExt    = &switchExt->mirrorGroups[group];

    /* Validate that FFU resource is no longer referenced */
    if (grpExt->ffuResId != FM10000_MIRROR_NO_FFU_RES)
    {
        err = fm10000ValidateAclTriggerId(sw, grpExt->ffuResId, &referenced);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        if (referenced)
        {
            err = FM_ERR_INVALID_ACL_PARAM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }
    }

    /* Resource allocation are returned back on mirror deletion */
    err = fm10000DeleteTrigger(sw,
                               FM10000_TRIGGER_GROUP_MIRROR,
                               MIRROR_RULE(group, 0),
                               TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    if (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL)
    {
        err = fm10000DeleteTrigger(sw,
                                   FM10000_TRIGGER_GROUP_MIRROR,
                                   MIRROR_RULE(group, 1),
                                   TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    if (grpExt->rxPortSet != FM_PORT_SET_ALL)
    {
        err = fmDeletePortSetInt(sw, grpExt->rxPortSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    if (grpExt->txPortSet != FM_PORT_SET_ALL)
    {
        err = fmDeletePortSetInt(sw, grpExt->txPortSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    err = fm10000FreeTriggerResource(sw,
                                     FM_TRIGGER_RES_MIRROR_PROFILE,
                                     grpExt->mirrorProfile,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    if (grpExt->vlanResId != FM10000_MIRROR_NO_VLAN_RES)
    {
        /* Clear all the VLAN previously configured for this mirror group. */
        for (fmTreeIterInit(&itVlan, &grp->vlan1s) ;
             (err = fmTreeIterNext(&itVlan, &vlanID, &nextValue)) == FM_OK ; )
        {
            ventryExt = GET_VLAN_EXT(sw, vlanID);

            if (ventryExt->trigger == grpExt->vlanResId)
            {
                TAKE_REG_LOCK(sw);
                regLockTaken = TRUE;

                ventryExt->trigger = 0;

                /* write to this one register (single indexed) */
                err = fmRegCacheReadSingle1D(sw,
                                             &fm10000CacheEgressVidTable,
                                             regVals,
                                             vlanID,
                                             FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

                FM_ARRAY_SET_FIELD(regVals,
                                   FM10000_EGRESS_VID_TABLE,
                                   TrigID,
                                   ventryExt->trigger);

                /* write to this one register (single indexed) */
                err = fmRegCacheWriteSingle1D(sw,
                                              &fm10000CacheEgressVidTable,
                                              regVals,
                                              vlanID,
                                              FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

                DROP_REG_LOCK(sw);
                regLockTaken = FALSE;
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            goto ABORT;
        }

        err = fm10000FreeTriggerResource(sw,
                                         FM_TRIGGER_RES_VLAN,
                                         grpExt->vlanResId,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* Free FFU resource if possible */
    if (grpExt->ffuResId != FM10000_MIRROR_NO_FFU_RES)
    {
        err = HandleMirrorFfuRes(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }


ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000DeleteMirror */




/*****************************************************************************/
/** fm10000WritePortMirrorGroup
 * \ingroup intMirror
 *
 * \desc            Sets up a port mirror group in the switch hardware.
 *                  Called through the WritePortMirrorGroup function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp points to the mirror group structure
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WritePortMirrorGroup(fm_int              sw,
                                      fm_portMirrorGroup *grp)
{
    fm_switch *                 switchPtr;
    fm10000_switch *            switchExt;
    fm_fm10000PortMirrorGroup * grpExt;
    fm10000_mirrorCfg           config;
    fm_int                      group;
    fm_triggerAction            trigAction;
    fm_triggerCondition         trigCondition;
    fm_int                      cpi;
    fm_status                   err;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d)\n",
                  sw,
                  (void *) grp,
                  grp->groupId );

    group     = grp->groupId;
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    grpExt    = &switchExt->mirrorGroups[group];

    /* FM10000 does not support mirroring on multiple ports */
    if (grp->mirrorPortType != FM_PORT_IDENTIFIER_PORT_NUMBER)
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    /* Configure the mirror profile. */
    FM_CLEAR(config);

    err = fmMapLogicalPortToPhysical(switchPtr,
                                     grp->mirrorLogicalPort,
                                     &config.physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    if (switchPtr->cpuPort == grp->mirrorLogicalPort)
    {
        config.glort = (switchPtr->glortInfo.cpuBase & 0xFF00) |
                       (FM10000_MIRROR_CPU_CODE_BASE + grp->trapCodeId);
    }
    else
    {
        err = fmGetLogicalPortGlort(sw, grp->mirrorLogicalPort, &config.glort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    config.vlan = (grp->encapVlan == FM_MIRROR_NO_VLAN_ENCAP) ? 0 : grp->encapVlan;
    config.vlanPri = grp->encapVlanPri;
    config.truncate = grp->truncateFrames;

    err = fm10000SetMirrorProfileConfig(sw, grpExt->mirrorProfile, &config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    /* Configure the first Trigger action */
    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    trigAction.cfg.mirrorAction = FM_TRIGGER_MIRROR_ACTION_MIRROR;
    trigAction.param.mirrorSelect = 1;
    trigAction.param.mirrorProfile = grpExt->mirrorProfile;

    /* Redirect Mirror Group uses Drop + Mirror Action. This has the benefit of
     *  being able to truncate or encapsulate the frame. */
    if (grp->mirrorType == FM_MIRROR_TYPE_REDIRECT)
    {
        trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_DROP;
        trigAction.param.dropPortset = FM_PORT_SET_ALL_BUT_CPU;
    }

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_MIRROR,
                                  MIRROR_RULE(group, 0),
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    /* Both Triggers have the same action if mode is bidirectional. */
    if (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL)
    {
        err = fm10000SetTriggerAction(sw,
                                      FM10000_TRIGGER_GROUP_MIRROR,
                                      MIRROR_RULE(group, 1),
                                      &trigAction,
                                      TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* Configure the Trigger Conditions */
    err = fmInitTriggerCondition(sw, &trigCondition);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    /* VLAN Condition */
    if ( (grpExt->vlanResId != FM10000_MIRROR_NO_VLAN_RES) &&
         (grp->mirrorType == FM_MIRROR_TYPE_EGRESS) )
    {
        trigCondition.cfg.matchVlan = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
        trigCondition.param.vidId = grpExt->vlanResId;
    }

    /* ACL Condition */
    if (grpExt->ffuResId != FM10000_MIRROR_NO_FFU_RES)
    {
        trigCondition.cfg.matchFFU = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
        trigCondition.param.ffuId = grpExt->ffuResId;
        trigCondition.param.ffuIdMask = grpExt->ffuResIdMask;
    }

    /* Sampling Condition */
    if ( (grp->sample != FM_MIRROR_SAMPLE_RATE_DISABLED) &&
         (grp->sample > 1) )
    {
        trigCondition.cfg.matchRandomNumber = TRUE;
        trigCondition.param.randGenerator = FM_TRIGGER_RAND_GEN_A;
        trigCondition.param.randMatchThreshold =
            ((FM10000_MIRROR_TRIG_MAX_SAMPLE + 1) / grp->sample) - 1;
    }

    /* Rebuild the Rx PortSet based on the Ingress Ports selected */
    if (grpExt->rxPortSet != FM_PORT_SET_ALL)
    {
        err = fmClearPortSetInt(sw, grpExt->rxPortSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        for (cpi = 0 ; ; cpi++)
        {
            err = fmFindBitInBitArray(&grp->ingressPortUsed, cpi, TRUE, &cpi);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

            if (cpi < 0 || cpi >= switchPtr->numCardinalPorts)
            {
                break;
            }

            err = fmAddPortSetPortInt(sw, grpExt->rxPortSet, GET_LOGICAL_PORT(sw, cpi));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }
    }

    /* Rebuild the Tx PortSet based on the Egress Ports selected */
    if (grpExt->txPortSet != FM_PORT_SET_ALL)
    {
        err = fmClearPortSetInt(sw, grpExt->txPortSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        for (cpi = 0 ; ; cpi++)
        {
            err = fmFindBitInBitArray(&grp->egressPortUsed, cpi, TRUE, &cpi);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

            if (cpi < 0 || cpi >= switchPtr->numCardinalPorts)
            {
                break;
            }

            err = fmAddPortSetPortInt(sw, grpExt->txPortSet, GET_LOGICAL_PORT(sw, cpi));
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }
    }

    trigCondition.cfg.rxPortset = grpExt->rxPortSet;

    if (grp->mirrorType == FM_MIRROR_TYPE_EGRESS)
    {
        trigCondition.cfg.matchTx = FM_TRIGGER_TX_MASK_CONTAINS;
        trigCondition.cfg.txPortset = grpExt->txPortSet;
    }
    else
    {
        trigCondition.cfg.matchTx = FM_TRIGGER_TX_MASK_DOESNT_CONTAIN;
        trigCondition.cfg.txPortset = FM_PORT_SET_NONE;
    }

    /* Mirror all frames */
    trigCondition.cfg.matchFtypeMask = (FM_TRIGGER_FTYPE_NORMAL |
                                        FM_TRIGGER_FTYPE_SPECIAL);
    trigCondition.cfg.HAMask = 0xffffffffffffffffLL;

    /* First Trigger */
    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_MIRROR,
                                     MIRROR_RULE(group, 0),
                                     &trigCondition,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    /* Bidirectional mirror type uses the second trigger to cover the
       Egress direction. */
    if (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL)
    {
        /* VLAN Condition */
        if (grpExt->vlanResId != FM10000_MIRROR_NO_VLAN_RES)
        {
            trigCondition.cfg.matchVlan = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
            trigCondition.param.vidId = grpExt->vlanResId;
        }

        trigCondition.cfg.rxPortset = FM_PORT_SET_ALL;
        trigCondition.cfg.matchTx = FM_TRIGGER_TX_MASK_CONTAINS;
        trigCondition.cfg.txPortset = grpExt->txPortSet;

        /* Second Trigger */
        err = fm10000SetTriggerCondition(sw,
                                         FM10000_TRIGGER_GROUP_MIRROR,
                                         MIRROR_RULE(group, 1),
                                         &trigCondition,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000WritePortMirrorGroup */




/*****************************************************************************/
/** fm10000SetMirrorAttribute
 * \ingroup intMirror
 *
 * \desc            Sets a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is invalid.
 * \return          FM_ERR_UNSUPPORTED if unrecognized attribute.
 * \return          FM_ERR_INVALID_VALUE if value points to an invalid value
 *                  for the specified attribute.
 *
 *****************************************************************************/
fm_status fm10000SetMirrorAttribute(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_int              attr,
                                    void *              value)
{
    fm_int                     sample;
    fm_int                     vlan;
    fm_int                     trapCodeId;
    fm_byte                    vlanPri;
    fm_bool                    ffuFilterOldVal;
    fm_bool                    referenced;
    fm10000_switch *           switchExt;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_status                  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d), attr = %d, value = %p\n",
                  sw,
                  (void *) grp,
                  grp->groupId,
                  attr,
                  (void *) value );

    switch (attr)
    {
        case FM_MIRROR_TRUNCATE:
            grp->truncateFrames = *( (fm_bool *) value );

            err = fm10000WritePortMirrorGroup(sw, grp);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            break;

        case FM_MIRROR_SAMPLE_RATE:
            sample = *( (fm_int *) value );
            
            if ( ((sample < 1) || (sample > (FM10000_MIRROR_TRIG_MAX_SAMPLE + 1))) &&
                 (sample != FM_MIRROR_SAMPLE_RATE_DISABLED) )
            {
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_VALUE);
            }

            grp->sample = sample;

            err = fm10000WritePortMirrorGroup(sw, grp);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            break;

        case FM_MIRROR_ACL:
            ffuFilterOldVal = grp->ffuFilter;

            /* Can only deactivate ACL Filtering if none of the current ACL
             * rule refer to this mirror Group. */
            if ((*( (fm_bool *) value ) == FALSE) &&
                (ffuFilterOldVal == TRUE))
            {
                switchExt = GET_SWITCH_EXT(sw);
                grpExt = &switchExt->mirrorGroups[grp->groupId];
                err = fm10000ValidateAclTriggerId(sw,
                                                  grpExt->ffuResId,
                                                  &referenced);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);

                if (referenced)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ACL_PARAM);
                }
            }

            grp->ffuFilter = *( (fm_bool *) value );

            err = HandleMirrorFfuRes(sw);
            if (err == FM_OK)
            {
                err = fm10000WritePortMirrorGroup(sw, grp);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            }
            else
            {
                /* Failure to allocate FFU Resource Id */
                grp->ffuFilter = ffuFilterOldVal;
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
            }
            break;

        case FM_MIRROR_VLAN:
            vlan = *( (fm_int *) value );
            if ( ((vlan < 0) || (vlan >= FM_MAX_VLAN)) &&
                 (vlan != FM_MIRROR_NO_VLAN_ENCAP) )
            {
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_VALUE);
            }

            grp->encapVlan = vlan;

            err = fm10000WritePortMirrorGroup(sw, grp);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            break;

        case FM_MIRROR_VLAN_PRI:
            vlanPri = *( (fm_byte *) value );
            if (vlanPri >= FM_MAX_VLAN_PRIORITIES)
            {
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_VALUE);
            }

            grp->encapVlanPri = vlanPri;

            err = fm10000WritePortMirrorGroup(sw, grp);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            break;

        case FM_MIRROR_TRAPCODE_ID:
            trapCodeId = *( (fm_int *) value );
            /* For APP Mirror Usage Type:
             *   Trap Code ID: 0 to FM10000_SFLOW_TRAPCODE_ID_START -1
             *  For SFLOW Mirror Usage Type:
             *   Trap Code ID: FM10000_SFLOW_TRAPCODE_ID_START to 
             *                 FM10000_MIRROR_NUM_TRAPCODE_ID -1 */ 
            if ( (grp->mirrorUsageType == FM_MIRROR_USAGE_TYPE_APP) &&
                 ( (trapCodeId < 0) ||
                   (trapCodeId >= FM10000_SFLOW_TRAPCODE_ID_START) ) )
            {
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_VALUE);
            }
            else if ( (grp->mirrorUsageType == FM_MIRROR_USAGE_TYPE_SFLOW) &&
                      ( (trapCodeId < FM10000_SFLOW_TRAPCODE_ID_START) ||
                        (trapCodeId >= FM10000_MIRROR_NUM_TRAPCODE_ID ) ) )
            {
                FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_VALUE);
            }

            grp->trapCodeId = trapCodeId;

            err = fm10000WritePortMirrorGroup(sw, grp);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_UNSUPPORTED);
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000SetMirrorAttribute */




/*****************************************************************************/
/** fm10000GetMirrorAttribute
 * \ingroup intMirror
 *
 * \desc            Gets a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to get.
 *
 * \param[in]       value points to caller-provided memory into which the value
 *                  of the requested attribute will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 *
 *****************************************************************************/
fm_status fm10000GetMirrorAttribute(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_int              attr,
                                    void *              value)
{
    fm_status err = FM_OK;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_float            sample;
    fm_uint64           otherCounter;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d), attr = %d, value = %p\n",
                  sw,
                  (void *) grp,
                  grp->groupId,
                  attr,
                  (void *) value );

    switch (attr)
    {
        case FM_MIRROR_TRUNCATE:
            *( (fm_bool *) value ) = grp->truncateFrames;
            break;

        case FM_MIRROR_SAMPLE_RATE:
            *( (fm_int *) value ) = grp->sample;

            /* Sampling rate is done using mantissa/exponent and we must read
             * back the actual hardware for real value. */
            if (grp->sample > 1)
            {
                err = fm10000GetTrigger(sw,
                                        FM10000_TRIGGER_GROUP_MIRROR,
                                        MIRROR_RULE(grp->groupId, 0),
                                        &trigCond,
                                        &trigAction);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MIRROR, err);

                sample = (FM10000_MIRROR_TRIG_MAX_SAMPLE + 1.0) /
                         (trigCond.param.randMatchThreshold + 1.0);
                *( (fm_int *) value ) = (fm_int) round(sample);
            }
            else
            {
                *( (fm_int *) value ) = grp->sample;
            }
            break;

        case FM_MIRROR_ACL:
            *( (fm_bool *) value ) = grp->ffuFilter;
            break;

        case FM_MIRROR_VLAN:
            *( (fm_int *) value ) = grp->encapVlan;
            break;

        case FM_MIRROR_VLAN_PRI:
            *( (fm_byte *) value ) = grp->encapVlanPri;
            break;

        case FM_MIRROR_TRAPCODE_ID:
            *( (fm_int *) value ) = grp->trapCodeId;
            break;

        case FM_MIRROR_INGRESS_COUNTER:
            err = GetMirrorCounters(sw, grp, value, (void *)&otherCounter); 
            break;

        case FM_MIRROR_EGRESS_COUNTER:
            err = GetMirrorCounters(sw, grp, (void *)&otherCounter, value); 
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_UNSUPPORTED);
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000GetMirrorAttribute */




/*****************************************************************************/
/** fm10000AddMirrorVlan
 * \ingroup intMirror
 *
 * \desc            Adds a vlan to a mirror group and specifies whether its
 *                  ingress vlan, egress vlan, or both should be mirrored.
 *                                                                      \lb\lb
 *                  If none of the vlans are added using this function, the
 *                  mirror group will be applied to all vlans.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp is the mirror group number to which the vlan should
 *                  be added.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the vlan to be added to the mirror group. This
 *                  vlan's traffic will be mirrored to the group's mirror port
 *                  if the ingress/egress ports condition match.
 *
 * \param[in]       direction indicates whether vlan's ingress traffic,
 *                  egress traffic or both are mirrored
 *                  (see ''fm_mirrorVlanType'').
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
fm_status fm10000AddMirrorVlan(fm_int              sw,
                               fm_portMirrorGroup *grp,
                               fm_vlanSelect       vlanSel,
                               fm_uint16           vlanID,
                               fm_mirrorVlanType   direction)
{
    fm10000_switch *           switchExt;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_int                     group;
    fm_status                  err = FM_OK;
    fm10000_vlanEntry *        ventryExt;
    fm_uint32                  regVals[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_bool                    regLockTaken = FALSE;
    fm_uint32                  vlanResId;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d), vlanSel = %d, "
                  "vlanID = %d, direction = %d\n",
                  sw,
                  (void *) grp,
                  grp->groupId,
                  vlanSel,
                  vlanID,
                  direction );

    group     = grp->groupId;
    switchExt = GET_SWITCH_EXT(sw);
    grpExt    = &switchExt->mirrorGroups[group];

    /* FM10000 can only match on Egress Vlan1 */
    if (vlanSel != FM_VLAN_SELECT_VLAN1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    if (direction != FM_MIRROR_VLAN_EGRESS)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    ventryExt = GET_VLAN_EXT(sw, vlanID);

    if (ventryExt->trigger == 0)
    {
        /* First Vlan entered, needs to grab a vlanId */
        if (grpExt->vlanResId == FM10000_MIRROR_NO_VLAN_RES)
        {
            err = fm10000AllocateTriggerResource(sw,
                                                 FM_TRIGGER_RES_VLAN,
                                                 &vlanResId,
                                                 TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

            grpExt->vlanResId = vlanResId;
        }

        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        ventryExt->trigger = grpExt->vlanResId;

        /* write to this one register (single indexed) */
        err = fmRegCacheReadSingle1D(sw,
                                     &fm10000CacheEgressVidTable,
                                     regVals,
                                     vlanID,
                                     FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        FM_ARRAY_SET_FIELD(regVals,
                           FM10000_EGRESS_VID_TABLE,
                           TrigID,
                           ventryExt->trigger);

        /* write to this one register (single indexed) */
        err = fmRegCacheWriteSingle1D(sw,
                                      &fm10000CacheEgressVidTable,
                                      regVals,
                                      vlanID,
                                      FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        DROP_REG_LOCK(sw);
        regLockTaken = FALSE;
    }
    else
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* No Vlan Mirroring --> Vlan Mirroring */
    if (fmTreeSize(&grp->vlan1s) == 1)
    {
        /* Update the mirror group condition */
        err = fm10000WritePortMirrorGroup(sw, grp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
    
}   /* end fm10000AddMirrorVlan */




/*****************************************************************************/
/** fm10000DeleteMirrorVlan
 * \ingroup intMirror
 *
 * \desc            Deletes a vlan from a mirror group so that its traffic
 *                  is no longer mirrored.
 * 
 *                  If all of the vlans are removed using this function, the
 *                  mirror group will be applied to all vlans.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       grp is the mirror group number from which the vlan should
 *                  be removed.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the vlan to be removed from the mirror group.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMirrorVlan(fm_int              sw,
                                  fm_portMirrorGroup *grp,
                                  fm_vlanSelect       vlanSel,
                                  fm_uint16           vlanID)
{
    fm10000_switch *           switchExt;
    fm_fm10000PortMirrorGroup *grpExt;
    fm_int                     group;
    fm_status                  err = FM_OK;
    fm10000_vlanEntry *        ventryExt;
    fm_uint32                  regVals[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_bool                    regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, grp = %p (%d), vlanSel = %d, "
                  "vlanID = %d\n",
                  sw,
                  (void *) grp,
                  grp->groupId,
                  vlanSel,
                  vlanID );

    group     = grp->groupId;
    switchExt = GET_SWITCH_EXT(sw);
    grpExt    = &switchExt->mirrorGroups[group];

    if (vlanSel != FM_VLAN_SELECT_VLAN1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    ventryExt = GET_VLAN_EXT(sw, vlanID);

    if (ventryExt->trigger != 0)
    {
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        ventryExt->trigger = 0;

        /* write to this one register (single indexed) */
        err = fmRegCacheReadSingle1D(sw,
                                     &fm10000CacheEgressVidTable,
                                     regVals,
                                     vlanID,
                                     FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        FM_ARRAY_SET_FIELD(regVals,
                           FM10000_EGRESS_VID_TABLE,
                           TrigID,
                           ventryExt->trigger);

        /* write to this one register (single indexed) */
        err = fmRegCacheWriteSingle1D(sw,
                                      &fm10000CacheEgressVidTable,
                                      regVals,
                                      vlanID,
                                      FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        DROP_REG_LOCK(sw);
        regLockTaken = FALSE;
    }
    else
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* Vlan Mirroring --> No Vlan Mirroring */
    if (fmTreeSize(&grp->vlan1s) == 0)
    {
        /* Update the mirror group condition */
        err = fm10000WritePortMirrorGroup(sw, grp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        if (grpExt->vlanResId != FM10000_MIRROR_NO_VLAN_RES)
        {
            err = fm10000FreeTriggerResource(sw,
                                             FM_TRIGGER_RES_VLAN,
                                             grpExt->vlanResId,
                                             TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

            grpExt->vlanResId = FM10000_MIRROR_NO_VLAN_RES;
        }
    }

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fm10000DeleteMirrorVlan */




/*****************************************************************************/
/** fm10000DbgDumpMirror
 * \ingroup intDiagMirror 
 *
 * \desc            Dumps the Mirror configuration.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMirror(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_int              group;
    fm_portMirrorGroup *grp;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_treeIterator     itVlan;
    fm_uint64           vlanId;
    fm_mirrorVlanType   vlanType;
    fm_bool             vlanFound;
    fm10000_mirrorCfg   config;
    fm_fm10000PortMirrorGroup *grpExt;
    void *              nextValue;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Dump each group configuration */
    for (group = 0 ; group < FM10000_MAX_MIRRORS_GRP ; group++)
    {
        grp = &switchPtr->mirrorGroups[group];

        FM_LOG_PRINT("\n************************************************************\n");
        if (!grp->used)
        {
            FM_LOG_PRINT("Group %d Inactive\n", group);
            continue;
        }

        FM_LOG_PRINT("Group %d Mirror configuration:\n", group);
        FM_LOG_PRINT("Mirror Type: ");
        switch (grp->mirrorType)
        {
            case FM_MIRROR_TYPE_INGRESS:
                FM_LOG_PRINT("FM_MIRROR_TYPE_INGRESS\n");
                break;

            case FM_MIRROR_TYPE_EGRESS:
                FM_LOG_PRINT("FM_MIRROR_TYPE_EGRESS\n");
                break;

            case FM_MIRROR_TYPE_BIDIRECTIONAL:
                FM_LOG_PRINT("FM_MIRROR_TYPE_BIDIRECTIONAL\n");
                break;

            case FM_MIRROR_TYPE_REDIRECT:
                FM_LOG_PRINT("FM_MIRROR_TYPE_REDIRECT\n");
                break;

            default:
                FM_LOG_PRINT("Unsupported Type %d\n", grp->mirrorType);
                break;
        }
        FM_LOG_PRINT("Ingress port Used: ");
        fmDbgDumpBitArray(&grp->ingressPortUsed, FM10000_MAX_PORT + 1);
        FM_LOG_PRINT("\nEgress port Used: ");
        fmDbgDumpBitArray(&grp->egressPortUsed, FM10000_MAX_PORT + 1);

        FM_LOG_PRINT("\nMirrorLogicalPort: %d, ", grp->mirrorLogicalPort);
        FM_LOG_PRINT("TruncateFrames: %d, Sample: %d, FfuFilter: %d, trapCodeId: %d\n",
                     grp->truncateFrames, grp->sample, grp->ffuFilter, grp->trapCodeId);

        if (grp->encapVlan == FM_MIRROR_NO_VLAN_ENCAP)
        {
            FM_LOG_PRINT("Vlan Encapsulation: Disabled\n");
        }
        else
        {
            FM_LOG_PRINT("Vlan Encapsulation: Enabled With VLAN: %d And VPRI: %d\n",
                         grp->encapVlan, grp->encapVlanPri);
        }

        FM_LOG_PRINT("\nEgress VLAN1: ");

        vlanFound = FALSE;
        for (fmTreeIterInit(&itVlan, &grp->vlan1s) ;
             (err = fmTreeIterNext(&itVlan, &vlanId, &nextValue)) ==
                    FM_OK ; )
        {
            vlanType = (fm_mirrorVlanType) nextValue;

            if ( (vlanType == FM_MIRROR_VLAN_EGRESS) )
            {
                FM_LOG_PRINT("%lld ", vlanId);
                vlanFound = TRUE;
            }
            else
            {
                FM_LOG_PRINT("ERROR Unsupported Direction %lld ", vlanId);
            }
        }
        if (err == FM_ERR_NO_MORE)
        {
            if (!vlanFound)
            {
                FM_LOG_PRINT("ALL");
            }
        }
        else
        {
            return err;
        }

        FM_LOG_PRINT("\n\nInternal Group %d Mirror configuration:\n", group);

        grpExt = &switchExt->mirrorGroups[group];

        FM_LOG_PRINT("\nMirrorProfile: %d, ", grpExt->mirrorProfile);
        FM_LOG_PRINT("RxPortSet: %d, TxPortSet: %d, VlanResId: %d\n",
                     grpExt->rxPortSet, grpExt->txPortSet, grpExt->vlanResId);

        if ( (grpExt->ffuResId == FM10000_MIRROR_NO_FFU_RES) &&
             (grpExt->ffuResIdMask == FM10000_MIRROR_NO_FFU_RES) )
        {
            FM_LOG_PRINT("FFU Resource Allocated: None\n");
        }
        else
        {
            FM_LOG_PRINT("FFU Resource Allocated: 0x%x/0x%x\n",
                         grpExt->ffuResId, grpExt->ffuResIdMask);
        }

        if (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL)
        {
            FM_LOG_PRINT("Trigger Group: %d Rule: %d(Ingress)\n",
                         FM10000_TRIGGER_GROUP_MIRROR,
                         MIRROR_RULE(group, 0));
            FM_LOG_PRINT("                         %d(Egress)\n",
                         MIRROR_RULE(group, 1));
        }
        else
        {
            FM_LOG_PRINT("Trigger Group: %d Rule: %d\n",
                         FM10000_TRIGGER_GROUP_MIRROR,
                         MIRROR_RULE(group, 0));
        }

        FM_LOG_PRINT("\n\nRegister Definition:\n");

        err = fm10000GetMirrorProfileConfig(sw, grpExt->mirrorProfile, &config);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        FM_LOG_PRINT("    Port:  %d\n",     config.physPort);
        FM_LOG_PRINT("    GLORT: 0x%x\n",   config.glort);
        FM_LOG_PRINT("    TRUNC: %d\n",     config.truncate);
        FM_LOG_PRINT("    VID:   %d\n",     config.vlan);
        FM_LOG_PRINT("    VPRI:  %d\n",     config.vlanPri);
    }


ABORT:

    return err;

}   /* end fm10000DbgDumpMirror */
