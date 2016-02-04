/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mirror.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with mirroring and ACLs
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define VALIDATE_PORT_MIRROR_GROUP(sw, grp)                 \
    if (( (grp) < 0 ) ||                                    \
        ( (grp) >= GET_SWITCH_PTR(sw)->mirrorTableSize ) )  \
    {                                                       \
        if (swProtected)                                    \
        {                                                   \
            UNPROTECT_SWITCH(sw);                           \
        }                                                   \
        return FM_ERR_INVALID_PORT_MIRROR_GROUP;            \
    }

#define GET_PORT_MIRROR_GROUP(sw, ptr, grp) \
    if (!swProtected)                       \
    {                                       \
        return FM_ERR_INVALID_SWITCH;       \
    }                                       \
    VALIDATE_PORT_MIRROR_GROUP(sw, grp);    \
    (ptr) = &GET_SWITCH_PTR(sw)->mirrorGroups[grp];

#define VALIDATE_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp) \
    if (( (grp) < 0 ) ||                                    \
        ( (grp) >= GET_SWITCH_PTR(sw)->mirrorTableSize ) )  \
    {                                                       \
        return FM_ERR_INVALID_PORT_MIRROR_GROUP;            \
    }

#define GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, ptr, grp) \
    VALIDATE_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp);    \
    (ptr) = &GET_SWITCH_PTR(sw)->mirrorGroups[grp];

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

fm_status GetMirrorDirection(fm_mirrorType mirrorType,
                             fm_bool *     ingress,
                             fm_bool *     egress);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** GetMirrorDirection
 * \ingroup intMirror
 *
 * \desc            This function return the direction of any mirror type.
 *
 * \param[in]       mirrorType is the type to process.
 * 
 * \param[out]      ingress points to caller-provided storage into which
 *                  the ingress direction is stored.
 * 
 * \param[out]      egress points to caller-provided storage into which
 *                  the egress direction is stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status GetMirrorDirection(fm_mirrorType mirrorType,
                             fm_bool *     ingress,
                             fm_bool *     egress)
{
    fm_status err = FM_OK;

    switch (mirrorType)
    {
        case FM_MIRROR_TYPE_INGRESS:
            *ingress = TRUE;
            *egress  = FALSE;
            break;

        case FM_MIRROR_TYPE_EGRESS:
        case FM_MIRROR_TYPE_TX_EGRESS:
            *ingress = FALSE;
            *egress  = TRUE;
            break;

        case FM_MIRROR_TYPE_BIDIRECTIONAL:
        case FM_MIRROR_TYPE_RX_INGRESS_TX_EGRESS:
            *ingress = TRUE;
            *egress  = TRUE;
            break;

        case FM_MIRROR_TYPE_REDIRECT:
            *ingress = TRUE;
            *egress  = FALSE;
            break;

        default:
            *ingress = FALSE;
            *egress  = FALSE;
            err = FM_ERR_INVALID_ARGUMENT;

    }   /* end switch (mirrorType) */

    return err;

}   /* end GetMirrorDirection */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAllocatePortMirrorDataStructures
 * \ingroup intMirror
 *
 * \desc            This function initializes the state holding structures for
 *                  port mirroring.
 *
 * \param[in]       switchPtr points to the switch state structure being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocatePortMirrorDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;
    fm_int    nbytes;
    fm_int    i;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr ? switchPtr->switchNumber : -1) );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    nbytes = sizeof(fm_portMirrorGroup) * switchPtr->mirrorTableSize; 

    switchPtr->mirrorGroups = (fm_portMirrorGroup *) fmAlloc(nbytes);

    if (switchPtr->mirrorGroups == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_NO_MEM);
    }

    memset(switchPtr->mirrorGroups, 0, nbytes);

    for (i = 0 ; i < switchPtr->mirrorTableSize ; i++)
    {
        err = fmCreateBitArray(&switchPtr->mirrorGroups[i].ingressPortUsed,
                               switchPtr->numCardinalPorts);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

        err = fmCreateBitArray(&switchPtr->mirrorGroups[i].egressPortUsed,
                               switchPtr->numCardinalPorts);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

        err = fmCreateBitArray(&switchPtr->mirrorGroups[i].mirrorLogicalPortMask,
                               switchPtr->numCardinalPorts);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmAllocatePortMirrorDataStructures */




/*****************************************************************************/
/** fmFreePortMirrorDataStructures
 * \ingroup intMirror
 *
 * \desc            This function initializes the state holding structures for
 *                  port mirroring.
 *
 * \param[in]       switchPtr points to the switch state structure being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreePortMirrorDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;
    fm_int    i;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr != NULL) ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < switchPtr->mirrorTableSize ; i++)
    {
        err = fmDeleteBitArray(&switchPtr->mirrorGroups[i].ingressPortUsed);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

        err = fmDeleteBitArray(&switchPtr->mirrorGroups[i].egressPortUsed);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

        err = fmDeleteBitArray(&switchPtr->mirrorGroups[i].mirrorLogicalPortMask);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);
        }

    }

    if (switchPtr->mirrorGroups)
    {
        fmFree(switchPtr->mirrorGroups);
        switchPtr->mirrorGroups = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmFreePortMirrorDataStructures */




/*****************************************************************************/
/** fmInitPortMirror
 * \ingroup intMirror
 *
 * \desc            This function initializes the port mirror group state.
 *
 * \param[in]       switchPtr points to the switch state structure being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitPortMirror(fm_switch *switchPtr)
{
    fm_int i;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr != NULL) ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < switchPtr->mirrorTableSize ; i++)
    {
        switchPtr->mirrorGroups[i].used              = FALSE;
        switchPtr->mirrorGroups[i].mirrorLogicalPort = 0;
        switchPtr->mirrorGroups[i].mirrorType        = FM_MIRROR_TYPE_INGRESS;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_OK);

}   /* end fmInitPortMirror */




/*****************************************************************************/
/** fmCreateMirrorInt
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a port mirror group.
 * 
 * \note            In a SWAG architecture, an encapsulation VLAN must always
 *                  be configured prior to adding any mirrored ports or vlans.
 *                  This encapsulating VLAN must be configured using mirror
 *                  attributes ''FM_MIRROR_VLAN'' and ''FM_MIRROR_VLAN_PRI''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group to create. The first available
 *                  group number is 0.
 *
 * \param[in]       mirrorPort is the destination logical port number to
 *                  mirror traffic to.
 *
 * \param[in]       mirrorType is the type of mirror.
 *
 * \param[in]       mirrorUsageType is the usage type of mirror.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or already created.
 * \return          FM_ERR_INVALID_PORT if mirrorPort is invalid.
 * \return          FM_ERR_UNSUPPORTED if mirrorType is unsupported.
 *
 *****************************************************************************/
fm_status fmCreateMirrorInt(fm_int             sw,
                            fm_int             group,
                            fm_int             mirrorPort,
                            fm_mirrorType      mirrorType,
                            fm_mirrorUsageType mirrorUsageType)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_switch *         switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                "sw=%d group=%d mirrorPort=%d mirrorType=%d\n",
                 sw,
                 group,
                 mirrorPort,
                 mirrorType);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (mirrorUsageType == FM_MIRROR_USAGE_TYPE_APP) && 
          ( (group < 0) || 
            (group >= (switchPtr->mirrorTableSize - switchPtr->maxSflows) ) ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_PORT_MIRROR_GROUP);
    }

    if ( (mirrorUsageType == FM_MIRROR_USAGE_TYPE_SFLOW) &&
          ( (group < (switchPtr->mirrorTableSize - switchPtr->maxSflows) ) ||
            (group >= switchPtr->mirrorTableSize ) ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_PORT_MIRROR_GROUP);
    }


    if (!fmIsValidPort(sw, mirrorPort, (ALLOW_CPU|ALLOW_REMOTE) ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_PORT);
    }

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    err = fmClearBitArray(&grp->mirrorLogicalPortMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmClearBitArray(&grp->ingressPortUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmClearBitArray(&grp->egressPortUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    grp->groupId             = group;
    grp->mirrorLogicalPort   = mirrorPort;
    grp->mirrorType          = mirrorType;
    grp->mirrorUsageType     = mirrorUsageType;
    grp->mirrorPortType      = FM_PORT_IDENTIFIER_PORT_NUMBER;
    grp->overlayMode         = TRUE;
    grp->egressPriority      = FM_MIRROR_PRIORITY_ORIGINAL;
    grp->truncateFrames      = FALSE;
    grp->sample              = FM_MIRROR_SAMPLE_RATE_DISABLED;
    grp->ffuFilter           = FALSE;
    grp->egressSrcPort       = FM_MIRROR_TX_EGRESS_PORT_FIRST;
    grp->truncateOtherFrames = FALSE;
    grp->encapVlan           = FM_MIRROR_NO_VLAN_ENCAP;
    grp->encapVlanPri        = 0;
    grp->trapCodeId          = 0;
    fmTreeInit(&grp->vlan1s);
    fmTreeInit(&grp->vlan2s);

    FM_API_CALL_FAMILY(err, switchPtr->CreateMirror, sw, grp);

    if (err != FM_OK)
    {
        fmTreeDestroy(&grp->vlan1s, NULL);
        fmTreeDestroy(&grp->vlan2s, NULL);
    }
    else
    {
        grp->used = TRUE;
    }


ABORT:
    DROP_MIRROR_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmCreateMirrorInt */




/*****************************************************************************/
/** fmCreateMirror
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a port mirror group.
 * 
 * \note            In a SWAG architecture, an encapsulation VLAN must always
 *                  be configured prior to adding any mirrored ports or vlans.
 *                  This encapsulating VLAN must be configured using mirror
 *                  attributes ''FM_MIRROR_VLAN'' and ''FM_MIRROR_VLAN_PRI''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group to create. The first available
 *                  group number is 0.
 *
 * \param[in]       mirrorPort is the destination logical port number to
 *                  mirror traffic to.
 *
 * \param[in]       mirrorType is the type of mirror (see 'fm_mirrorType').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or already created.
 * \return          FM_ERR_INVALID_PORT if mirrorPort is invalid.
 * \return          FM_ERR_UNSUPPORTED if mirrorType is unsupported.
 *
 *****************************************************************************/
fm_status fmCreateMirror(fm_int        sw,
                         fm_int        group,
                         fm_int        mirrorPort,
                         fm_mirrorType mirrorType)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d mirrorPort=%d mirrorType=%d\n",
                     sw,
                     group,
                     mirrorPort,
                     mirrorType);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmCreateMirrorInt(sw, 
                            group, 
                            mirrorPort,
                            mirrorType,
                            FM_MIRROR_USAGE_TYPE_APP);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmCreateMirror */




/*****************************************************************************/
/** fmDeleteMirrorInt
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_ACL_PARAM if at least one ACL refers to
 *                  this mirror group. You must delete or update all the ACL
 *                  rules that refer to this mirror group before deleting
 *                  the group.
 * 
 *****************************************************************************/
fm_status fmDeleteMirrorInt(fm_int sw, fm_int group)
{
    fm_portMirrorGroup *grp;
    fm_switch *         switchPtr;
    fm_status           err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR, "sw=%d group=%d\n", sw, group);

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    /* ACL lock and state lock need to be taken prior to the mirror lock for
     * lock inversion prevention. The ACL lock is needed on fmDeleteMirrorInt()
     * because some validation is done at the ACL level to make sure this group
     * is currently unused. The state lock is needed when notifying the CRM
     * state machine about suspending/resuming TCAM checking during update of
     * ACL trigger mask. */ 
    FM_TAKE_ACL_LOCK(sw);
    FM_TAKE_STATE_LOCK(sw);
    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    grp->used = 0;

    if (switchPtr->WritePortMirrorGroup != NULL)
    {
        /* write the entry to hardware */
        err = switchPtr->WritePortMirrorGroup(sw, grp);
        if (err != FM_OK)
        {
            grp->used = 1;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }
    }

    if (switchPtr->DeleteMirror != NULL)
    {
        err = switchPtr->DeleteMirror(sw, grp);
        if (err != FM_OK)
        {
            grp->used = 1;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
        }
    }

    fmTreeDestroy(&grp->vlan1s, NULL);
    fmTreeDestroy(&grp->vlan2s, NULL);


ABORT:
    DROP_MIRROR_LOCK(sw);
    FM_DROP_STATE_LOCK(sw);
    FM_DROP_ACL_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorInt */




/*****************************************************************************/
/** fmDeleteMirror
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_ACL_PARAM if at least one ACL refers to
 *                  this mirror group. You must delete or update all the ACL
 *                  rules that refer to this mirror group before deleting
 *                  the group.
 * 
 *****************************************************************************/
fm_status fmDeleteMirror(fm_int sw, fm_int group)
{
    fm_status           err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR, "sw=%d group=%d\n", sw, group);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmDeleteMirrorInt(sw, group);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirror */




/*****************************************************************************/
/** fmSetMirrorDestination
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set or change the destination port for a mirror group.
 *
 * \note            On FM6000 devices, to set a destination port 
 *                  mask, use ''fmSetMirrorDestinationExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group whose destination port is to be
 *                  changed.
 *
 * \param[in]       mirrorPort is the destination logical port number to
 *                  mirror traffic to.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or doesn't exist.
 * \return          FM_ERR_INVALID_PORT if mirrorPort is invalid.
 *
 *****************************************************************************/
fm_status fmSetMirrorDestination(fm_int sw,
                                 fm_int group,
                                 fm_int mirrorPort)
{
    fm_portMirrorGroup *grp = NULL;
    fm_status           err;
    fm_switch *         switchPtr;
    fm_int              oldMirrorLogicalPort;
    fm_bool             oldOverlayMode;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d mirrorPort=%d\n",
                     sw,
                     group,
                     mirrorPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, mirrorPort, ALLOW_CPU|ALLOW_REMOTE);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    /* Capture original state */
    oldMirrorLogicalPort = grp->mirrorLogicalPort;
    oldOverlayMode       = grp->overlayMode;

    grp->mirrorLogicalPort = mirrorPort;
    grp->overlayMode       = TRUE;

    switchPtr = GET_SWITCH_PTR(sw);

    /* SWAG Handling */
    if (switchPtr->SetMirrorDestination != NULL)
    {
        err = switchPtr->SetMirrorDestination(sw, grp, mirrorPort);
    }
    else
    {
        /* write the entry to hardware */
        FM_API_CALL_FAMILY(err,
                           switchPtr->WritePortMirrorGroup,
                           sw,
                           grp);
    }

    /* Restore original state on error */
    if (err != FM_OK)
    {
        grp->mirrorLogicalPort = oldMirrorLogicalPort;
        grp->overlayMode       = oldOverlayMode;

        /* SWAG Handling */
        if (switchPtr->SetMirrorDestination != NULL)
        {
            switchPtr->SetMirrorDestination(sw, grp, oldMirrorLogicalPort);
        }
        else if (switchPtr->WritePortMirrorGroup != NULL)
        {
            /* write the entry to hardware */
            switchPtr->WritePortMirrorGroup(sw, grp);
        }
    }

ABORT:
    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmSetMirrorDestination */




/*****************************************************************************/
/** fmSetMirrorDestinationExt
 * \ingroup mirror
 *
 * \chips           FM6000
 *
 * \desc            Set or change the destination port(s) for a mirror
 *                  group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group whose mirror port is to be
 *                  changed.
 *
 * \param[in]       mirrorPortId points to an ''fm_portIdentifier'' structure
 *                  that identifies the port or ports to which traffic is
 *                  to be mirrored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if mirrorPortId is NULL or has
 *                  invalid contents.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or doesn't exist.
 * \return          FM_ERR_INVALID_PORT if mirrorPort is invalid.
 *
 *****************************************************************************/
fm_status fmSetMirrorDestinationExt(fm_int             sw,
                                    fm_int             group,
                                    fm_portIdentifier *mirrorPortId)
{
    fm_portMirrorGroup *  grp;
    fm_status             err;
    fm_switch *           switchPtr;
    fm_bool               mirrorLockTaken = FALSE;
    fm_int                mirrorPort = FM_LOGICAL_PORT_NONE;
    fm_bitArray           oldMirrorPortMask;
    fm_bool               oldOverlayMode;
    fm_int                oldMirrorLogicalPort;
    fm_portIdentifierType oldMirrorPortType;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MIRROR,
                      "sw = %d, group = %d, mirrorPortId = %p\n",
                      sw,
                      group,
                      (void *) mirrorPortId );

    if (mirrorPortId == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (mirrorPortId->identifierType == FM_PORT_IDENTIFIER_PORT_NUMBER)
    {
        /* Note that this has to happen outside the mirror lock, because
         * the macro will return from this function on error. */
        mirrorPort = mirrorPortId->port;

        VALIDATE_LOGICAL_PORT(sw, mirrorPort, ALLOW_CPU | ALLOW_REMOTE);
    }

    GET_PORT_MIRROR_GROUP(sw, grp, group);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    TAKE_MIRROR_LOCK(sw);
    mirrorLockTaken = TRUE;

    oldOverlayMode = grp->overlayMode;

    switch (mirrorPortId->identifierType)
    {
        case FM_PORT_IDENTIFIER_PORT_NUMBER:
            grp->overlayMode = TRUE;
            break;

        case FM_PORT_IDENTIFIER_PORT_MASK:
            grp->overlayMode = FALSE;

            /* Backup the original port mask */
            err = fmCreateBitArray(&oldMirrorPortMask,
                                   grp->mirrorLogicalPortMask.bitCount);
            if (err != FM_OK)
            {
                grp->overlayMode = oldOverlayMode;
                goto ABORT;
            }

            err = fmCopyBitArray(&oldMirrorPortMask,
                                 &grp->mirrorLogicalPortMask);
            if (err != FM_OK)
            {
                fmDeleteBitArray(&oldMirrorPortMask);
                grp->overlayMode = oldOverlayMode;
                goto ABORT;
            }

            /* Remove the original port mask first */
            err = fmDeleteBitArray(&grp->mirrorLogicalPortMask);
            if (err != FM_OK)
            {
                fmDeleteBitArray(&oldMirrorPortMask);
                grp->overlayMode = oldOverlayMode;
                goto ABORT;
            }

            /* Create the new port mask */
            err = fmCreateBitArray(&grp->mirrorLogicalPortMask,
                                   mirrorPortId->portMask.bitCount);
            if (err != FM_OK)
            {
                fmCreateBitArray(&grp->mirrorLogicalPortMask,
                                 oldMirrorPortMask.bitCount);
                fmCopyBitArray(&grp->mirrorLogicalPortMask,
                               &oldMirrorPortMask);
                fmDeleteBitArray(&oldMirrorPortMask);
                grp->overlayMode = oldOverlayMode;
                goto ABORT;
            }

            /* Populate the new port mask */
            err = fmCopyBitArray(&grp->mirrorLogicalPortMask,
                                 &mirrorPortId->portMask);
            if (err != FM_OK)
            {
                fmDeleteBitArray(&grp->mirrorLogicalPortMask);
                fmCreateBitArray(&grp->mirrorLogicalPortMask,
                                 oldMirrorPortMask.bitCount);
                fmCopyBitArray(&grp->mirrorLogicalPortMask,
                               &oldMirrorPortMask);
                fmDeleteBitArray(&oldMirrorPortMask);
                grp->overlayMode = oldOverlayMode;
                goto ABORT;
            }
            
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    oldMirrorLogicalPort = grp->mirrorLogicalPort;
    oldMirrorPortType = grp->mirrorPortType;

    grp->mirrorLogicalPort = mirrorPort;
    grp->mirrorPortType = mirrorPortId->identifierType;

    switchPtr = GET_SWITCH_PTR(sw);

    /* SWAG Handling */
    if (switchPtr->SetMirrorDestination != NULL)
    {
        err = switchPtr->SetMirrorDestination(sw,
                                              grp,
                                              mirrorPort);
    }
    else
    {
        /* write the entry to hardware */
        FM_API_CALL_FAMILY(err,
                           switchPtr->WritePortMirrorGroup,
                           sw,
                           grp);
    }

    /* Restore original state on error */
    if (err != FM_OK)
    {
        if (mirrorPortId->identifierType == FM_PORT_IDENTIFIER_PORT_MASK)
        {
            fmDeleteBitArray(&grp->mirrorLogicalPortMask);
            fmCreateBitArray(&grp->mirrorLogicalPortMask,
                             oldMirrorPortMask.bitCount);
            fmCopyBitArray(&grp->mirrorLogicalPortMask,
                           &oldMirrorPortMask);
            fmDeleteBitArray(&oldMirrorPortMask);
        }
        grp->overlayMode       = oldOverlayMode;
        grp->mirrorLogicalPort = oldMirrorLogicalPort;
        grp->mirrorPortType    = oldMirrorPortType;

        /* SWAG Handling */
        if (switchPtr->SetMirrorDestination != NULL)
        {
            switchPtr->SetMirrorDestination(sw, grp, oldMirrorLogicalPort);
        }
        else if (switchPtr->WritePortMirrorGroup != NULL)
        {
            /* write the entry to hardware */
            switchPtr->WritePortMirrorGroup(sw, grp);
        }
    }
    else if (grp->overlayMode == FALSE)
    {
        err = fmDeleteBitArray(&oldMirrorPortMask);
    }


ABORT:

    if (mirrorLockTaken)
    {
        DROP_MIRROR_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmSetMirrorDestinationExt */




/*****************************************************************************/
/** fmGetMirrorDestination
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the destination port for a mirror group.
 *
 * \note            On FM6000 devices, for mirror groups that use 
 *                  a destination port mask, see ''fmGetMirrorDestinationExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group whose mirror port is to be
 *                  retrieved.
 *
 * \param[out]      mirrorPort points to caller-provided storage into which
 *                  the destination logical port number will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or doesn't exist.
 * \return          FM_ERR_INVALID_PORT if mirrorPort is invalid because
 *                  a port mask is being used instead of a port number, in 
 *                  which case ''fmGetMirrorDestinationExt'' should be used.
 * \return          FM_ERR_INVALID_ARGUMENT if mirrorPort is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorDestination(fm_int  sw,
                                 fm_int  group,
                                 fm_int *mirrorPort)
{
    fm_portMirrorGroup *grp = NULL;
    fm_status           err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MIRROR,
                      "sw=%d group=%d mirrorPort=%p\n",
                      sw,
                      group,
                      (void *) mirrorPort );

    if (!mirrorPort)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
    }
    else
    {
        if (grp->overlayMode)
        {
            *mirrorPort = grp->mirrorLogicalPort;
            err         = FM_OK;
        }
        else
        {
            err = FM_ERR_INVALID_PORT;
        }
    }

    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorDestination */




/*****************************************************************************/
/** fmGetMirrorDestinationExt
 * \ingroup mirror
 *
 * \chips           FM6000
 *
 * \desc            Get the destination port(s) for a mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group whose destination port is to be
 *                  retrieved.
 *
 * \param[out]      mirrorPortId points to an ''fm_portIdentifier'' structure
 *                  into which the mirror destination port information will be
 *                  written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if mirrorPortId is NULL.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or doesn't exist.
 *
 *****************************************************************************/
fm_status fmGetMirrorDestinationExt(fm_int             sw,
                                    fm_int             group,
                                    fm_portIdentifier *mirrorPortId)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_bool             mirrorLockTaken = FALSE;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MIRROR,
                      "sw = %d, group = %d, mirrorPortId = %p\n",
                      sw,
                      group,
                      (void *) mirrorPortId );

    if (mirrorPortId == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    GET_PORT_MIRROR_GROUP(sw, grp, group);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    TAKE_MIRROR_LOCK(sw);
    mirrorLockTaken = TRUE;

    /* Note that if the group is not in overlay mode, grp->mirrorLogicalPort
     * will be FM_LOGICAL_PORT_NONE. */
    mirrorPortId->port = grp->mirrorLogicalPort;

    if (grp->overlayMode)
    {
        mirrorPortId->identifierType = FM_PORT_IDENTIFIER_PORT_NUMBER;
    }
    else
    {
        mirrorPortId->identifierType = FM_PORT_IDENTIFIER_PORT_MASK;

        fmDeleteBitArray(&mirrorPortId->portMask);

        err = fmCreateBitArray(&mirrorPortId->portMask,
                               grp->mirrorLogicalPortMask.bitCount);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

        err = fmCopyBitArray(&mirrorPortId->portMask,
                             &grp->mirrorLogicalPortMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    err = FM_OK;

ABORT:

    if (mirrorLockTaken)
    {
        DROP_MIRROR_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorDestinationExt */




/*****************************************************************************/
/** fmAddMirrorPort
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a mirror group.  Traffic to or from the
 *                  specified port will be mirrored to the mirror group's
 *                  mirror port.
 *                                                                      \lb\lb
 *                  Whether the port's ingress or egress traffic is mirrored
 *                  is dependent on the ''fm_mirrorType'' specified when the
 *                  group was created with ''fmCreateMirror''. To specify
 *                  whether the port's ingress or egress traffic should be
 *                  mirrored, use ''fmAddMirrorPortExt''.
 *                                                                      \lb\lb
 *                  Once a port has been added to a mirror group, it cannot
 *                  be added again to the same mirror group or any other mirror
 *                  group using this function; however, see 
 *                  ''fmAddMirrorPortExt''.
 *
 * \note            The port cannot be a LAG. To mirror a LAG, add all the
 *                  LAG's physical member ports to the mirror group.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorAddPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the port should
 *                  be added.
 *
 * \param[in]       port is the port number of the port to be added to the
 *                  mirror group.  This port's traffic will be mirrored to
 *                  the group's mirror port. The port cannot be a LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_PORT_ALREADY_MIRRORED if specified port is already
 *                  being mirrored by another mirror group.
 * \return          FM_ERR_MIRROR_NO_VLAN_ENCAP if an encap vlan is required
 *                  for the current platform, e.g. SWAG.
 *
 *****************************************************************************/
fm_status fmAddMirrorPort(fm_int sw, fm_int group, fm_int port)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_int              i;
    fm_int              firstPort;
    fm_int              nextPort;
    fm_switch *         switchPtr;
    fm_bool             mirrorLockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d port=%d\n",
                     sw,
                     group,
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    switchPtr = GET_SWITCH_PTR(sw);

    GET_PORT_MIRROR_GROUP(sw, grp, group);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    TAKE_MIRROR_LOCK(sw);
    mirrorLockTaken = TRUE;

    /* check if the port has been added to other mirror group */
    for (i = 0 ; (i < switchPtr->mirrorTableSize) ; i++)
    {
        /* Consider only groups in use. */
        if (switchPtr->mirrorGroups[i].used)
        {
            if ( ( err = fmGetMirrorPortFirst(sw, i, &firstPort) ) != FM_OK )
            {
                continue;
            }

            if (firstPort == port)
            {
                err = FM_ERR_PORT_ALREADY_MIRRORED;
                goto ABORT;
            }

            while (firstPort != -1)
            {
                if ( ( err = fmGetMirrorPortNext(sw, i, firstPort, &nextPort) )
                    == FM_OK )
                {
                    if (nextPort == port)
                    {
                        err = FM_ERR_PORT_ALREADY_MIRRORED;
                        goto ABORT;
                    }
                }

                firstPort = nextPort;
            }
        }
    }

    err = fmAddMirrorPortExt(sw, group, port, grp->mirrorType);

ABORT:
    if (mirrorLockTaken)
    {
        DROP_MIRROR_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorPort */




/*****************************************************************************/
/** fmAddMirrorPortInternal
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a mirror group and indicate whether its
 *                  ingress traffic, egress traffic or both should be mirrored.
 *                                                                      \lb\lb
 *                  Unlike ''fmAddMirrorPort'', this function can be called
 *                  multiple times for the same port. Doing so is useful for
 *                  changing the mirror type when the same group is specified.
 *
 * \note            For the FM2000 and FM4000 switch families, a port can be
 *                  added to two different mirror groups, however both mirrors
 *                  will operate only if one is an ingress mirror and the other
 *                  is an egress mirror. If the two groups mirror in the
 *                  same direction for a given frame, only one of the mirrors
 *                  will work for that frame.
 *
 * \note            For the FM6000 switch family, a port can be added to
 *                  multiple mirror groups and all mirrors will operate
 *                  correctly, subject to hardware and microcode limitations.
 *
 * \note            The mirror group must have been created in a call to
 *                  ''fmCreateMirror'' with a ''fm_mirrorType'' of
 *                  ''FM_MIRROR_TYPE_BIDIRECTIONAL'', or a type that matches
 *                  the mirrorType argument to this function.
 *
 * \note            The port cannot be a LAG. To mirror a LAG, add all the
 *                  LAG's physical member ports to the mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the port should
 *                  be added.
 *
 * \param[in]       port is the port number of the port to be added to the
 *                  mirror group.  This port's traffic will be mirrored to
 *                  the group's mirror port. The port cannot be a LAG.
 *
 * \param[in]       mirrorType indicates whether port's ingress traffic,
 *                  egress traffic or both are mirrored (see ''fm_mirrorType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range,
 *                  does not exist or was not created with a ''fm_mirrorType''
 *                  of ''FM_MIRROR_TYPE_BIDIRECTIONAL'' or that matches 
 *                  mirrorType.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_MIRROR_NO_VLAN_ENCAP if an encap vlan is required
 *                  for the current platform, e.g. SWAG.
 *
 *****************************************************************************/
fm_status fmAddMirrorPortInternal(fm_int        sw,
                                  fm_int        group,
                                  fm_int        port,
                                  fm_mirrorType mirrorType)
{
    fm_portMirrorGroup *grp;
    fm_status           err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
    fm_switch *         switchPtr;
    fm_port *           portPtr;
    fm_bool             grpIngress;
    fm_bool             grpEgress;
    fm_bool             portIngress;
    fm_bool             portEgress;
    fm_bool             oldPortIngress;
    fm_bool             oldPortEgress;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                 "sw=%d group=%d port=%d, type=%d\n",
                 sw,
                 group,
                 port,
                 mirrorType);

    if (!fmIsValidPort(sw, port, ALLOW_CPU) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_PORT);
    }

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        goto ABORT;
    }

    err = GetMirrorDirection(grp->mirrorType, &grpIngress, &grpEgress);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = GetMirrorDirection(mirrorType, &portIngress, &portEgress);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Validate the port mirrorType to be a subset of the group. */
    if ( (portIngress && !grpIngress) ||
         (portEgress  && !grpEgress) )
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = switchPtr->portTable[port];

    switch (portPtr->portType)
    {
        case FM_PORT_TYPE_CPU:
        case FM_PORT_TYPE_PHYSICAL:
        case FM_PORT_TYPE_TE:
        case FM_PORT_TYPE_LOOPBACK:
            break;
        default:
            err = FM_ERR_INVALID_PORT;
            goto ABORT; /* do nothing for other ports */
    }

    err = fmGetBitArrayBit(&grp->ingressPortUsed,
                           portPtr->portIndex,
                           &oldPortIngress);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmGetBitArrayBit(&grp->egressPortUsed,
                           portPtr->portIndex,
                           &oldPortEgress);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);


    err = fmSetBitArrayBit(&grp->ingressPortUsed,
                           portPtr->portIndex,
                           portIngress);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmSetBitArrayBit(&grp->egressPortUsed,
                           portPtr->portIndex,
                           portEgress);
    if (err != FM_OK)
    {
        /* Restore back old value */
        fmSetBitArrayBit(&grp->ingressPortUsed,
                         portPtr->portIndex,
                         oldPortIngress);
        goto ABORT;
    }

    /* SWAG Handling */
    if (switchPtr->AddMirrorPort != NULL)
    {
        err = switchPtr->AddMirrorPort(sw, grp, port, mirrorType);
    }
    else
    {
         /* write the entry to hardware */
        FM_API_CALL_FAMILY(err,
                           switchPtr->WritePortMirrorGroup,
                           sw,
                           grp);
    }

    /* Restore original state on error */
    if (err != FM_OK)
    {
        fmSetBitArrayBit(&grp->ingressPortUsed,
                         portPtr->portIndex,
                         oldPortIngress);
        fmSetBitArrayBit(&grp->egressPortUsed,
                         portPtr->portIndex,
                         oldPortEgress);

        if (switchPtr->WritePortMirrorGroup != NULL)
        {
            /* write the entry to hardware */
            switchPtr->WritePortMirrorGroup(sw, grp);
        }
    }

ABORT:
    DROP_MIRROR_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorPortInternal */




/*****************************************************************************/
/** fmAddMirrorPortExt
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a mirror group and indicate whether its
 *                  ingress traffic, egress traffic or both should be mirrored.
 *                                                                      \lb\lb
 *                  Unlike ''fmAddMirrorPort'', this function can be called
 *                  multiple times for the same port. Doing so is useful for
 *                  changing the mirror type when the same group is specified.
 *
 * \note            For the FM2000 and FM4000 switch families, a port can be
 *                  added to two different mirror groups, however both mirrors
 *                  will operate only if one is an ingress mirror and the other
 *                  is an egress mirror. If the two groups mirror in the
 *                  same direction for a given frame, only one of the mirrors
 *                  will work for that frame.
 *
 * \note            For the FM6000 switch family, a port can be added to
 *                  multiple mirror groups and all mirrors will operate
 *                  correctly, subject to hardware and microcode limitations.
 *
 * \note            The mirror group must have been created in a call to
 *                  ''fmCreateMirror'' with a ''fm_mirrorType'' of
 *                  ''FM_MIRROR_TYPE_BIDIRECTIONAL'', or a type that matches
 *                  the mirrorType argument to this function.
 *
 * \note            The port cannot be a LAG. To mirror a LAG, add all the
 *                  LAG's physical member ports to the mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the port should
 *                  be added.
 *
 * \param[in]       port is the port number of the port to be added to the
 *                  mirror group.  This port's traffic will be mirrored to
 *                  the group's mirror port. The port cannot be a LAG.
 *
 * \param[in]       mirrorType indicates whether port's ingress traffic,
 *                  egress traffic or both are mirrored (see ''fm_mirrorType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range,
 *                  does not exist or was not created with a ''fm_mirrorType''
 *                  of ''FM_MIRROR_TYPE_BIDIRECTIONAL'' or that matches 
 *                  mirrorType.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_MIRROR_NO_VLAN_ENCAP if an encap vlan is required
 *                  for the current platform, e.g. SWAG.
 *
 *****************************************************************************/
fm_status fmAddMirrorPortExt(fm_int        sw,
                             fm_int        group,
                             fm_int        port,
                             fm_mirrorType mirrorType)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d port=%d, type=%d\n",
                     sw,
                     group,
                     port,
                     mirrorType);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    err = fmAddMirrorPortInternal(sw,
                                  group,
                                  port,
                                  mirrorType);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorPortExt */




/*****************************************************************************/
/** fmDeleteMirrorPortInt
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port from a mirror group so that its traffic
 *                  is no longer mirrored.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorRemovePort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number from which the port should
 *                  be removed.
 *
 * \param[in]       port is the port number of the port to be removed from the
 *                  mirror group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteMirrorPortInt(fm_int sw, fm_int group, fm_int port)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_switch *         switchPtr;
    fm_port *           portPtr;
    fm_bool             oldPortIngress;
    fm_bool             oldPortEgress;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                 "sw=%d group=%d port=%d\n",
                 sw,
                 group,
                 port);

    if (!fmIsValidPort(sw, port, ALLOW_CPU) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_PORT);
    }

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    err = fmGetBitArrayBit(&grp->ingressPortUsed,
                           portPtr->portIndex,
                           &oldPortIngress);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmGetBitArrayBit(&grp->egressPortUsed,
                           portPtr->portIndex,
                           &oldPortEgress);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmSetBitArrayBit(&grp->ingressPortUsed,
                           portPtr->portIndex,
                           FALSE);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmSetBitArrayBit(&grp->egressPortUsed,
                           portPtr->portIndex,
                           FALSE);
    if (err != FM_OK)
    {
        /* Restore back old value */
        fmSetBitArrayBit(&grp->ingressPortUsed,
                         portPtr->portIndex,
                         oldPortIngress);
        goto ABORT;
    }

    /* SWAG Handling */
    if (switchPtr->DeleteMirrorPort != NULL)
    {
        err = switchPtr->DeleteMirrorPort(sw, grp, port);
    }
    else
    {
        /* write the entry to hardware */
        FM_API_CALL_FAMILY(err,
                           switchPtr->WritePortMirrorGroup,
                           sw,
                           grp);
    }

    /* Restore original state on error */
    if (err != FM_OK)
    {
        fmSetBitArrayBit(&grp->ingressPortUsed,
                         portPtr->portIndex,
                         oldPortIngress);
        fmSetBitArrayBit(&grp->egressPortUsed,
                         portPtr->portIndex,
                         oldPortEgress);

        if (switchPtr->WritePortMirrorGroup != NULL)
        {
            /* write the entry to hardware */
            switchPtr->WritePortMirrorGroup(sw, grp);
        }
    }


ABORT:
    DROP_MIRROR_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorPortInt */




/*****************************************************************************/
/** fmDeleteMirrorPort
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port from a mirror group so that its traffic
 *                  is no longer mirrored.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorRemovePort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number from which the port should
 *                  be removed.
 *
 * \param[in]       port is the port number of the port to be removed from the
 *                  mirror group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteMirrorPort(fm_int sw, fm_int group, fm_int port)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d port=%d\n",
                     sw,
                     group,
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmDeleteMirrorPortInt(sw, group, port);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorPort */




/*****************************************************************************/
/** fmAddMirrorVlanInternal
 * \ingroup intmirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            By default, when a mirror group is created, it will
 *                  mirror all traffic regardless of the VLAN on
 *                  which that traffic appears. If this function is called,
 *                  the mirror group will only mirror traffic on the specified
 *                  VLAN in the specified direction. A mirror group can be
 *                  made to mirror traffic on multiple VLANs by calling this
 *                  function multiple times, once for each VLAN of interest.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the VLAN should
 *                  be associated.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the VLAN to be associated to the mirror group. 
 *                  This VLAN's traffic will be mirrored to the group's mirror 
 *                  destination port if the ingress/egress port condition
 *                  matches.
 *
 * \param[in]       direction indicates whether the VLAN's ingress traffic,
 *                  egress traffic or both are mirrored (see 
 *                  ''fm_mirrorVlanType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * 
 *****************************************************************************/
fm_status fmAddMirrorVlanInternal(fm_int            sw,
                                  fm_int            group,
                                  fm_vlanSelect     vlanSel,
                                  fm_uint16         vlanID,
                                  fm_mirrorVlanType direction)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_switch *         switchPtr;
    fm_tree *           vlanTree;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d vlanID=%d direction=%d\n",
                     sw,
                     group,
                     vlanSel,
                     vlanID,
                     direction);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    vlanTree = (vlanSel == FM_VLAN_SELECT_VLAN2) ?  &grp->vlan2s : &grp->vlan1s;

    err = fmTreeInsert(vlanTree, vlanID, (void*) direction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddMirrorVlan,
                       sw,
                       grp,
                       vlanSel,
                       vlanID,
                       direction);

    if (err != FM_OK)
    {
        fmTreeRemoveCertain(vlanTree, vlanID, NULL);
    }

ABORT:
    DROP_MIRROR_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorVlanInternal */




/*****************************************************************************/
/** fmAddMirrorVlan
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            By default, when a mirror group is created, it will
 *                  mirror all traffic regardless of the VLAN on
 *                  which that traffic appears. If this function is called,
 *                  the mirror group will only mirror traffic on the specified
 *                  VLAN in the specified direction. A mirror group can be
 *                  made to mirror traffic on multiple VLANs by calling this
 *                  function multiple times, once for each VLAN of interest.
 *                                                                      \lb\lb
 *                  This function operates only VLAN1 in multiply tagged
 *                  frames and is deprecated in favor of ''fmAddMirrorVlanExt''
 *                  which can operate on either VLAN1 or VLAN2.
 * 
 * \note            For the FM10000 switch family, a vlan can only be added to
 *                  a single mirror group and the only direction available is
 *                  FM_MIRROR_VLAN_EGRESS.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the VLAN should
 *                  be associated.
 *
 * \param[in]       vlanID is the VLAN to be associated to the mirror group. 
 *                  This VLAN's traffic will be mirrored to the group's mirror 
 *                  destination port if the ingress/egress port condition
 *                  matches.
 *
 * \param[in]       direction indicates whether the VLAN's ingress traffic,
 *                  egress traffic or both are mirrored (see 
 *                  ''fm_mirrorVlanType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_ARGUMENT if direction is not supported.
 * \return          FM_ERR_MIRROR_NO_VLAN_ENCAP if an encap vlan is required
 *                  for the current platform, e.g. SWAG.
 * 
 *****************************************************************************/
fm_status fmAddMirrorVlan(fm_int            sw,
                          fm_int            group,
                          fm_uint16         vlanID,
                          fm_mirrorVlanType direction)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanID=%d, direction=%d\n",
                     sw,
                     group,
                     vlanID,
                     direction);

    err = fmAddMirrorVlanInternal(sw,
                                  group,
                                  FM_VLAN_SELECT_VLAN1,
                                  vlanID,
                                  direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorVlan */




/*****************************************************************************/
/** fmAddMirrorVlanExt
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            By default, when a mirror group is created, it will
 *                  mirror all traffic regardless of the VLAN on
 *                  which that traffic appears. If this function is called,
 *                  the mirror group will only mirror traffic on the specified
 *                  VLAN in the specified direction. A mirror group can be
 *                  made to mirror traffic on multiple VLANs by calling this
 *                  function multiple times, once for each VLAN of interest.
 * 
 * \note            For the FM10000 switch family, a vlan can only be added to
 *                  a single mirror group and the only direction available is
 *                  FM_MIRROR_VLAN_EGRESS.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number to which the VLAN should
 *                  be associated.
 *
 * \param[in]       vlanSel indicates which VLAN field in the frame the mirror
 *                  group should operate on.
 *
 * \param[in]       vlanID is the VLAN to be associated to the mirror group. 
 *                  This VLAN's traffic will be mirrored to the group's mirror 
 *                  destination port if the ingress/egress port condition
 *                  matches.
 *
 * \param[in]       direction indicates whether the VLAN's ingress traffic,
 *                  egress traffic or both are mirrored (see 
 *                  ''fm_mirrorVlanType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_ARGUMENT if direction is not supported.
 * \return          FM_ERR_MIRROR_NO_VLAN_ENCAP if an encap vlan is required
 *                  for the current platform, e.g. SWAG.
 * 
 *****************************************************************************/
fm_status fmAddMirrorVlanExt(fm_int            sw,
                             fm_int            group,
                             fm_vlanSelect     vlanSel,
                             fm_uint16         vlanID,
                             fm_mirrorVlanType direction)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d vlanID=%d direction=%d\n",
                     sw,
                     group,
                     vlanSel,
                     vlanID,
                     direction);

    err = fmAddMirrorVlanInternal(sw, group, vlanSel, vlanID, direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmAddMirrorVlanExt */




/*****************************************************************************/
/** fmDeleteMirrorVlanInternal
 * \ingroup intmirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Delete a VLAN filter condition from a mirror group 
 *                  previously established with a call to ''fmAddMirrorVlan''. 
 *                  As long as the mirror group is operating on at least one 
 *                  other VLAN, traffic on the deleted VLAN will no longer be
 *                  mirrored.
 *                                                                      \lb\lb
 *                  When all VLANs associated with the mirror group have been
 *                  deleted using this function, the mirror group will mirror
 *                  all traffic regardless of VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number from which the VLAN should
 *                  be deleted.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the vlan to be deleted from the mirror group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 *
 *****************************************************************************/
fm_status fmDeleteMirrorVlanInternal(fm_int        sw, 
                                     fm_int        group, 
                                     fm_vlanSelect vlanSel,
                                     fm_uint16     vlanID)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_switch *         switchPtr;
    fm_tree *           vlanTree;
    fm_intptr           direction;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d vlanID=%d\n",
                     sw,
                     group,
                     vlanSel,
                     vlanID);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    vlanTree = (vlanSel == FM_VLAN_SELECT_VLAN2) ?  &grp->vlan2s : &grp->vlan1s;

    err = fmTreeFind(vlanTree, vlanID, (void**)&direction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    err = fmTreeRemoveCertain(vlanTree, vlanID, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteMirrorVlan,
                       sw,
                       grp,
                       vlanSel,
                       vlanID);

    if (err != FM_OK)
    {
        fmTreeInsert(vlanTree, vlanID, (void*) direction);
    }

ABORT:
    DROP_MIRROR_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorVlanInternal */




/*****************************************************************************/
/** fmDeleteMirrorVlan
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Delete a VLAN filter condition from a mirror group 
 *                  previously established with a call to ''fmAddMirrorVlan''. 
 *                  As long as the mirror group is operating on at least one 
 *                  other VLAN, traffic on the deleted VLAN will no longer be
 *                  mirrored.
 *                                                                      \lb\lb
 *                  When all VLANs associated with the mirror group have been
 *                  deleted using this function, the mirror group will mirror
 *                  all traffic regardless of VLAN.
 *                                                                      \lb\lb
 *                  This function operates only VLAN1 in multiply tagged
 *                  frames and is deprecated in favor of ''fmDeleteMirrorVlanExt''
 *                  which can operate on either VLAN1 or VLAN2.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number from which the VLAN should
 *                  be deleted.
 *
 * \param[in]       vlanID is the vlan to be deleted from the mirror group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 *
 *****************************************************************************/
fm_status fmDeleteMirrorVlan(fm_int sw, fm_int group, fm_uint16 vlanID)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanID=%d\n",
                     sw,
                     group,
                     vlanID);

    err = fmDeleteMirrorVlanInternal(sw, group, FM_VLAN_SELECT_VLAN1, vlanID);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorVlan */




/*****************************************************************************/
/** fmDeleteMirrorVlanExt
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Delete a VLAN filter condition from a mirror group 
 *                  previously established with a call to ''fmAddMirrorVlanExt''. 
 *                  As long as the mirror group is operating on at least one 
 *                  other VLAN, traffic on the deleted VLAN will no longer be
 *                  mirrored.
 *                                                                      \lb\lb
 *                  When all VLANs associated with the mirror group have been
 *                  deleted using this function, the mirror group will mirror
 *                  all traffic regardless of VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group number from which the VLAN should
 *                  be deleted.
 *
 * \param[in]       vlanSel indicates which VLAN field in the frame the mirror
 *                  group is operating on.
 *
 * \param[in]       vlanID is the vlan to be deleted from the mirror group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if vlanID is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 *
 *****************************************************************************/
fm_status fmDeleteMirrorVlanExt(fm_int        sw, 
                                fm_int        group, 
                                fm_vlanSelect vlanSel,
                                fm_uint16     vlanID)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d vlanID=%d\n",
                     sw,
                     group,
                     vlanSel,
                     vlanID);

    err = fmDeleteMirrorVlanInternal(sw, group, vlanSel, vlanID);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmDeleteMirrorVlanExt */



/*****************************************************************************/
/** fmGetMirrorVlanFirstInternal
 * \ingroup intmirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the first VLAN in a mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first VLAN.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[out]      firstID points to caller-allocated storage where this
 *                  function should place the first VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstID or direction is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanFirstInternal(fm_int             sw,
                                       fm_int             group,
                                       fm_vlanSelect      vlanSel,
                                       fm_uint16 *        firstID,
                                       fm_mirrorVlanType *direction)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_treeIterator     itVlan;
    fm_uint64           key;
    void               *value;
    fm_tree *           vlanTree;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d firstID=%p direction=%p\n",
                     sw,
                     group,
                     vlanSel,
                     (void *) firstID,
                     (void *) direction);

    if (!firstID || !direction)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    vlanTree = (vlanSel == FM_VLAN_SELECT_VLAN2) ?  &grp->vlan2s : &grp->vlan1s;

    fmTreeIterInit(&itVlan, vlanTree);
    err = fmTreeIterNext(&itVlan, &key, &value);
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_ERR_NO_VLANS_IN_MIRROR_GROUP;
    }
    *firstID = key;
    *direction = (fm_mirrorVlanType) value;

ABORT:
    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanFirstinternal */




/*****************************************************************************/
/** fmGetMirrorVlanFirst
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the first VLAN in a mirror group.
 *                                                                      \lb\lb
 *                  This function is deprecated in favor of 
 *                  ''fmGetMirrorVlanFirstExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first VLAN.
 *
 * \param[out]      firstID points to caller-allocated storage where this
 *                  function should place the first VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstID or direction is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanFirst(fm_int             sw,
                               fm_int             group,
                               fm_uint16 *        firstID,
                               fm_mirrorVlanType *direction)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d firstID=%p direction=%p\n",
                     sw,
                     group,
                     (void *) firstID,
                     (void *) direction);

    err = fmGetMirrorVlanFirstInternal(sw,
                                       group,
                                       FM_VLAN_SELECT_VLAN1,
                                       firstID,
                                       direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanFirst */




/*****************************************************************************/
/** fmGetMirrorVlanFirstExt
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the first VLAN in a mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first VLAN.
 *
 * \param[in]       vlanSel indicates which VLAN field in the frame the mirror
 *                  group is operating on.
 *
 * \param[out]      firstID points to caller-allocated storage where this
 *                  function should place the first VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstID or direction is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanFirstExt(fm_int             sw,
                                  fm_int             group,
                                  fm_vlanSelect      vlanSel,
                                  fm_uint16 *        firstID,
                                  fm_mirrorVlanType *direction)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d firstID=%p direction=%p\n",
                     sw,
                     group,
                     vlanSel,
                     (void *) firstID,
                     (void *) direction);

    err = fmGetMirrorVlanFirstInternal(sw, group, vlanSel, firstID, direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanFirst */




/*****************************************************************************/
/** fmGetMirrorVlanNextInternal
 * \ingroup intMirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the next VLAN in a mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next VLAN.
 *
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       startID is the last VLAN number found by a previous
 *                  call to this function or to ''fmGetMirrorVlanFirst''.
 *
 * \param[out]      nextID points to caller-allocated storage where this
 *                  function should place the next VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanNextInternal(fm_int             sw,
                                      fm_int             group,
                                      fm_vlanSelect      vlanSel,
                                      fm_uint16          startID,
                                      fm_uint16 *        nextID,
                                      fm_mirrorVlanType *direction)
{
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_treeIterator     itVlan;
    fm_uint64           key;
    void *              value;
    fm_tree *           vlanTree;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%dstartID=%d nextID=%p direction=%p\n",
                     sw,
                     group,
                     vlanSel,
                     startID,
                     (void *) nextID,
                     (void *) direction);

    if (!nextID || !direction)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    vlanTree = (vlanSel == FM_VLAN_SELECT_VLAN2) ?  &grp->vlan2s : &grp->vlan1s;

    err = fmTreeIterInitFromSuccessor(&itVlan, vlanTree, startID);
    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_VLAN;
    }
    else
    {
        err = fmTreeIterNext(&itVlan, &key, &value);
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_VLANS_IN_MIRROR_GROUP;
        }
        *nextID = key;
        *direction = (fm_mirrorVlanType) value;
    }

ABORT:
    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanNextInternal */




/*****************************************************************************/
/** fmGetMirrorVlanNext
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the next VLAN in a mirror group.
 *                                                                      \lb\lb
 *                  This function is deprecated in favor of 
 *                  ''fmGetMirrorVlanNextExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next VLAN.
 *
 * \param[in]       startID is the last VLAN number found by a previous
 *                  call to this function or to ''fmGetMirrorVlanFirst''.
 *
 * \param[out]      nextID points to caller-allocated storage where this
 *                  function should place the next VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanNext(fm_int             sw,
                              fm_int             group,
                              fm_uint16          startID,
                              fm_uint16 *        nextID,
                              fm_mirrorVlanType *direction)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d startID=%d nextID=%p direction=%p\n",
                     sw,
                     group,
                     startID,
                     (void *) nextID,
                     (void *) direction);

    err = fmGetMirrorVlanNextInternal(sw,
                                      group,
                                      FM_VLAN_SELECT_VLAN1,
                                      startID,
                                      nextID,
                                      direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanNext */




/*****************************************************************************/
/** fmGetMirrorVlanNextExt
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the next VLAN in a mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next VLAN.
 *
 * \param[in]       vlanSel indicates which VLAN field in the frame the mirror
 *                  group is operating on.
 *
 * \param[in]       startID is the last VLAN number found by a previous
 *                  call to this function or to ''fmGetMirrorVlanFirst''.
 *
 * \param[out]      nextID points to caller-allocated storage where this
 *                  function should place the next VLAN from the mirror
 *                  group.
 * 
 * \param[out]      direction points to caller-allocated storage where this
 *                  function should place the configured direction of the
 *                  returned VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_VLANS_IN_MIRROR_GROUP if there are no VLANs
 *                  in the mirror group.
 *
 *****************************************************************************/
fm_status fmGetMirrorVlanNextExt(fm_int             sw,
                                 fm_int             group,
                                 fm_vlanSelect      vlanSel,
                                 fm_uint16          startID,
                                 fm_uint16 *        nextID,
                                 fm_mirrorVlanType *direction)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d vlanSel=%d startID=%d nextID=%p direction=%p\n",
                     sw,
                     group,
                     vlanSel,
                     startID,
                     (void *) nextID,
                     (void *) direction);

    err = fmGetMirrorVlanNextInternal(sw,
                                      group,
                                      vlanSel,
                                      startID,
                                      nextID,
                                      direction);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorVlanNextExt */




/*****************************************************************************/
/** fmGetMirror
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve mirror group's type and mirror port
 *                  (destination port).
 *
 * \note            This function was formerly known as ''fmGetMirrorGroup''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mirrorGroup is the mirror group from which to retreive the
 *                  type and mirror port.
 *
 * \param[out]      mirrorPort the port all mirrored traffic is being directed to.
 *
 * \param[out]      mirrorType type or direction of traffic being mirrored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_INVALID_ARGUMENT if mirrorPort or mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirror(fm_int         sw,
                      fm_int         mirrorGroup,
                      fm_int *       mirrorPort,
                      fm_mirrorType *mirrorType)
{
    fm_portMirrorGroup *grp;
    fm_status           err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d mirrorGroup=%d mirrorPort=%p mirrorType=%p\n",
                     sw,
                     mirrorGroup,
                     (void *) mirrorPort,
                     (void *) mirrorType);

    if (!mirrorPort || !mirrorType)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, mirrorGroup);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
    }
    else
    {
        *mirrorPort = grp->mirrorLogicalPort;
        *mirrorType = grp->mirrorType;
    }

    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirror */




/*****************************************************************************/
/** fmGetMirrorFirst
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing mirror group's id, type and
 *                  mirror port (destination port).
 *
 * \note            This function was formerly known as ''fmGetMirrorGroupFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstGroup is the first mirror group that exists.
 *
 * \param[out]      mirrorPort the port all mirrored traffic is being directed to.
 *
 * \param[out]      mirrorType type or direction of traffic being mirrored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MIRROR_GROUPS_EXIST - no mirror groups exist
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if firstGroup, mirrorPort or
 *                  mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorFirst(fm_int         sw,
                           fm_int *       firstGroup,
                           fm_int *       mirrorPort,
                           fm_mirrorType *mirrorType)
{
    fm_switch *         switchPtr;
    fm_int              i;
    fm_portMirrorGroup *grp;
    fm_status           err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d firstGroup=%p mirrorPort=%p mirrorType=%p\n",
                     sw,
                     (void *) firstGroup,
                     (void *) mirrorPort,
                     (void *) mirrorType);

    if (!firstGroup || !mirrorPort || !mirrorType)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    TAKE_MIRROR_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (i = 0 ; i < switchPtr->mirrorTableSize ; i++)
    {
        grp = &switchPtr->mirrorGroups[(i)];

        if (grp->used)
        {
            *firstGroup = i;
            *mirrorPort = grp->mirrorLogicalPort;
            *mirrorType = grp->mirrorType;
            break;
        }
    }

    if (i >= switchPtr->mirrorTableSize)
    {
        err         = FM_ERR_NO_MIRROR_GROUPS_EXIST;
        *firstGroup = -1;
    }

    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorFirst */




/*****************************************************************************/
/** fmGetMirrorNext
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing mirror group's id, type and
 *                  mirror port (destination port).
 *
 * \note            This function was formerly known as ''fmGetMirrorGroupNext''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentGroup is the last mirror group that was found by a
 *                  previous call to this function or ''fmGetMirrorFirst''.
 *
 * \param[out]      nextGroup points to caller-allocated storage where this
 *                  function should place the next group found.
 *
 * \param[out]      mirrorPort the port all mirrored traffic is being directed to.
 *
 * \param[out]      mirrorType type or direction of traffic being mirrored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MIRROR_GROUPS_EXIST no more mirror groups exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if nextGroup, mirrorPort or
 *                  mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorNext(fm_int         sw,
                          fm_int         currentGroup,
                          fm_int *       nextGroup,
                          fm_int *       mirrorPort,
                          fm_mirrorType *mirrorType)
{
    fm_switch *         switchPtr;
    fm_int              i;
    fm_portMirrorGroup *grp;
    fm_status           err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d currentGroup=%d, nextGroup=%p "
                     "mirrorPort=%p mirrorType=%p\n",
                     sw,
                     currentGroup,
                     (void *) nextGroup,
                     (void *) mirrorPort,
                     (void *) mirrorType);

    if (!nextGroup || !mirrorPort || !mirrorType)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    TAKE_MIRROR_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (i = (currentGroup + 1) ;
         i < switchPtr->mirrorTableSize ;
         i++)
    {
        grp = &switchPtr->mirrorGroups[(i)];

        if (grp->used)
        {
            /* move to next group */
            *nextGroup  = i;
            *mirrorPort = grp->mirrorLogicalPort;
            *mirrorType = grp->mirrorType;
            break;
        }
    }

    if (i >= switchPtr->mirrorTableSize)
    {
        err        = FM_ERR_NO_MIRROR_GROUPS_EXIST;
        *nextGroup = -1;
    }

    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorNext */




/*****************************************************************************/
/** fmGetMirrorPortFirstInt
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in a mirror group. See also
 *                  ''fmGetMirrorPortFirstIntV2''.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorGetFirstPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first port.
 *
 * \param[out]      firstPort points to caller-allocated storage where this
 *                  function should place the first port from the mirror
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstPort is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortFirstInt(fm_int sw, fm_int group, fm_int *firstPort)
{
    fm_int              i;
    fm_int              j = -1;
    fm_portMirrorGroup *grp;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d firstPort=%p\n",
                     sw,
                     group,
                     (void *) firstPort);

    if (!firstPort)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    *firstPort = -1;

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    err = fmFindBitInBitArray(&grp->ingressPortUsed,
                              0,
                              TRUE,
                              &i);

    if (err == FM_OK)
    {
        err = fmFindBitInBitArray(&grp->egressPortUsed,
                                  0,
                                  TRUE,
                                  &j);
    }

    if (err == FM_OK)
    {
        /* take the lowest non-negative value from the two port bit arrays */
        if ((j >= 0) && ((i < 0) || (j < i))) 
        {
            i = j;
        }

        if (i >= 0)
        {
            *firstPort = GET_LOGICAL_PORT(sw, i);
        }
        else
        {
            err = FM_ERR_NO_PORTS_IN_MIRROR_GROUP;
        }

    }   /* end if (err == FM_OK) */


ABORT:
    DROP_MIRROR_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortFirstInt */




/*****************************************************************************/
/** fmGetMirrorPortFirst
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in a mirror group. See also
 *                  ''fmGetMirrorPortFirstV2''.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorGetFirstPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first port.
 *
 * \param[out]      firstPort points to caller-allocated storage where this
 *                  function should place the first port from the mirror
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstPort is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortFirst(fm_int sw, fm_int group, fm_int *firstPort)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d firstPort=%p\n",
                     sw,
                     group,
                     (void *) firstPort);

    if (!firstPort)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmGetMirrorPortFirstInt(sw, group, firstPort);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortFirst */




/*****************************************************************************/
/** fmGetMirrorPortNextInt
 * \ingroup intMirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in a mirror group. See also,
 *                  ''fmGetMirrorPortNextIntV2''.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorGetNextPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next port.
 *
 * \param[in]       currentPort is the last port number found by a previous
 *                  call to this function or to ''fmGetMirrorPortFirst''.
 *
 * \param[out]      nextPort points to caller-allocated storage where this
 *                  function should place the next port from the mirror
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no more ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if nextPort is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortNextInt(fm_int  sw,
                                 fm_int  group,
                                 fm_int  currentPort,
                                 fm_int *nextPort)
{
    fm_int              i;
    fm_int              j = -1;
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_int              cpi;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                 "sw=%d group=%d currentPort=%d nextPort=%p\n",
                 sw,
                 group,
                 currentPort,
                 (void *) nextPort);

    if (!nextPort)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    *nextPort = -1;

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    cpi = GET_PORT_INDEX(sw, currentPort);

    err = fmFindBitInBitArray(&grp->ingressPortUsed, cpi + 1, TRUE, &i);

    if (err == FM_OK)
    {
        err = fmFindBitInBitArray(&grp->egressPortUsed, cpi + 1, TRUE, &j);
    }

    /* take the lowest non-negative value from the two port bit arrays */ 
    if (err == FM_OK)
    {
        if ((j >= 0) && ((i < 0) || (j < i))) 
        {
            i = j;
        }

        if (i >= 0)
        {
            *nextPort = GET_LOGICAL_PORT(sw, i);
        }
        else
        {
            err = FM_ERR_NO_PORTS_IN_MIRROR_GROUP;
        }

    }   /* end if (err == FM_OK) */


ABORT:
    DROP_MIRROR_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortNextInt */




/*****************************************************************************/
/** fmGetMirrorPortNext
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in a mirror group. See also,
 *                  ''fmGetMirrorPortNextV2''.
 *
 * \note            This function can also be addressed by the legacy
 *                  synonym, ''fmMirrorGetNextPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next port.
 *
 * \param[in]       currentPort is the last port number found by a previous
 *                  call to this function or to ''fmGetMirrorPortFirst''.
 *
 * \param[out]      nextPort points to caller-allocated storage where this
 *                  function should place the next port from the mirror
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no more ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if nextPort is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortNext(fm_int  sw,
                              fm_int  group,
                              fm_int  currentPort,
                              fm_int *nextPort)
{
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d currentPort=%d nextPort=%p\n",
                     sw,
                     group,
                     currentPort,
                     (void *) nextPort);

    if (!nextPort)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    err = fmGetMirrorPortNextInt(sw, group, currentPort, nextPort);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortNext */




/*****************************************************************************/
/** fmGetMirrorPortFirstV2
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in a mirror group. Provided as
 *                  an alternative to ''fmGetMirrorPortFirst'' for when the
 *                  mirror type must also be returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  first port.
 *
 * \param[out]      firstPort points to caller-allocated storage where this
 *                  function should place the first port from the mirror
 *                  group.
 * 
 * \param[out]      mirrorType points to caller-allocated storage where this
 *                  function indicates whether the port's ingress traffic,
 *                  egress traffic or both are mirrored (see ''fm_mirrorType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if firstPort or mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortFirstV2(fm_int         sw,
                                 fm_int         group,
                                 fm_int *       firstPort,
                                 fm_mirrorType *mirrorType)
{
    fm_int              i;
    fm_int              j = -1;
    fm_int              cpi;
    fm_portMirrorGroup *grp;
    fm_status           err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d firstPort=%p mirrorType=%p\n",
                     sw,
                     group,
                     (void *) firstPort,
                     (void *) mirrorType);

    if (!firstPort || !mirrorType)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    *firstPort = -1;

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    err = fmFindBitInBitArray(&grp->ingressPortUsed,
                              0,
                              TRUE,
                              &i);

    if (err == FM_OK)
    {
        err = fmFindBitInBitArray(&grp->egressPortUsed,
                                  0,
                                  TRUE,
                                  &j);
    }

    if (err == FM_OK)
    {
        /* take the lowest non-negative value from the two port bit arrays */
        cpi = i;
        if ((j >= 0) && ((i < 0) || (j < i))) 
        {
            cpi = j;
        }

        if (cpi >= 0)
        {
            *firstPort = GET_LOGICAL_PORT(sw, cpi);

            if ( (cpi == i) &&
                 (cpi == j) )
            {
                *mirrorType = grp->mirrorType;
            }
            else if (cpi == i)
            {
                *mirrorType = FM_MIRROR_TYPE_INGRESS;
            }
            else
            {
                *mirrorType = FM_MIRROR_TYPE_EGRESS;
            }
        }
        else
        {
            err = FM_ERR_NO_PORTS_IN_MIRROR_GROUP;
        }

    }   /* end if (err == FM_OK) */


ABORT:
    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortFirstV2 */




/*****************************************************************************/
/** fmGetMirrorPortNextV2
 * \ingroup mirror
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in a mirror group. Provided as
 *                  an alternative to ''fmGetMirrorPortNext'' for when the
 *                  mirror type must also be returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next port.
 *
 * \param[in]       currentPort is the last port number found by a previous
 *                  call to this function or to ''fmGetMirrorPortFirstV2''.
 *
 * \param[out]      nextPort points to caller-allocated storage where this
 *                  function should place the next port from the mirror
 *                  group.
 * 
 * \param[out]      mirrorType points to caller-allocated storage where this
 *                  function indicates whether the port's ingress traffic,
 *                  egress traffic or both are mirrored (see ''fm_mirrorType'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no more ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if nextPort or mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortNextV2(fm_int         sw,
                                fm_int         group,
                                fm_int         currentPort,
                                fm_int *       nextPort,
                                fm_mirrorType *mirrorType)
{
    fm_int              i;
    fm_int              j = -1;
    fm_portMirrorGroup *grp;
    fm_status           err;
    fm_int              cpi;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MIRROR,
                     "sw=%d group=%d currentPort=%d nextPort=%p mirrorType=%p\n",
                     sw,
                     group,
                     currentPort,
                     (void *) nextPort,
                     (void *) mirrorType);

    if (!nextPort || !mirrorType)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    GET_PORT_MIRROR_GROUP(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    *nextPort = -1;

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    cpi = GET_PORT_INDEX(sw, currentPort);

    err = fmFindBitInBitArray(&grp->ingressPortUsed, cpi + 1, TRUE, &i);

    if (err == FM_OK)
    {
        err = fmFindBitInBitArray(&grp->egressPortUsed, cpi + 1, TRUE, &j);
    }

    /* take the lowest non-negative value from the two port bit arrays */ 
    cpi = i;
    if (err == FM_OK)
    {
        if ((j >= 0) && ((i < 0) || (j < i))) 
        {
            cpi = j;
        }

        if (cpi >= 0)
        {
            *nextPort = GET_LOGICAL_PORT(sw, cpi);

            if ( (cpi == i) &&
                 (cpi == j) )
            {
                *mirrorType = grp->mirrorType;
            }
            else if (cpi == i)
            {
                *mirrorType = FM_MIRROR_TYPE_INGRESS;
            }
            else
            {
                *mirrorType = FM_MIRROR_TYPE_EGRESS;
            }
        }
        else
        {
            err = FM_ERR_NO_PORTS_IN_MIRROR_GROUP;
        }

    }   /* end if (err == FM_OK) */


ABORT:
    DROP_MIRROR_LOCK(sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortNextV2 */



/*****************************************************************************/
/** fmGetMirrorPortListsInt
 * \ingroup intMirror
 *
 * \chips           FM10000
 *
 * \desc            Retrieve list of ports in the ingress and egress port list
 *                  of the mirror group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group from which to retrieve the
 *                  next port.
 *
 * \param[out]      numIngressPorts points to caller-allocated storage where 
 *                  the number of ports that are enabled to mirror ingress 
 *                  packets is stored.
 *
 * \param[out]      ingressPortList points to caller-allocated storage where 
 *                  the list of ports that are enabled to mirror ingress
 *                  packets are stored.
 *
 * \param[in]       maxIngressPorts is the size of the ingressPortList array.
 *
 * \param[out]      numEgressPorts points to caller-allocated storage where
 *                  the number of ports that are enabled to mirror egress
 *                  packets is stored.
 *
 * \param[out]      egressPortList points to caller-allocated storage where 
 *                  the list of ports that are enabled to mirror egress
 *                  packets are stored.
 *
 * \param[in]       maxEgressPorts is the size of the egressPortList array.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is out of range
 *                  or does not exist.
 * \return          FM_ERR_NO_PORTS_IN_MIRROR_GROUP if there are no more ports
 *                  in the mirror group.
 * \return          FM_ERR_INVALID_ARGUMENT if nextPort or mirrorType is NULL.
 *
 *****************************************************************************/
fm_status fmGetMirrorPortListsInt(fm_int  sw,
                                  fm_int  group,
                                  fm_int *numIngressPorts, 
                                  fm_int *ingressPortList,
                                  fm_int  maxIngressPorts,
                                  fm_int *numEgressPorts, 
                                  fm_int *egressPortList,
                                  fm_int  maxEgressPorts)
{
    fm_portMirrorGroup *grp;
    fm_status           err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR,
                 "sw=%d group=%d "
                 "numIngressPorts=%p ingressPortList=%p maxIngressPorts=%d "
                 "numEgressPorts=%p egressPortList=%p maxEgressPorts=%d\n", 
                 sw,
                 group,
                 (void *) numIngressPorts,
                 (void *) ingressPortList,
                 maxIngressPorts,
                 (void *) numEgressPorts,
                 (void *) egressPortList,
                 maxEgressPorts);

    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    TAKE_MIRROR_LOCK(sw);

    if (!grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        goto ABORT;
    }

    if (ingressPortList)
    {
        if (!numIngressPorts || maxIngressPorts <= 0)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
        }
    }
    else if (egressPortList)
    {
        if (!numEgressPorts || maxEgressPorts <= 0)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            goto ABORT;
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }
    
    if (ingressPortList)
    {
        err = fmBitArrayToPortList(sw,
                                   &grp->ingressPortUsed,
                                   numIngressPorts,
                                   ingressPortList,
                                   maxIngressPorts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    if (egressPortList)
    {
        err = fmBitArrayToPortList(sw,
                                   &grp->egressPortUsed,
                                   numEgressPorts,
                                   egressPortList,
                                   maxEgressPorts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

ABORT:
    DROP_MIRROR_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorPortListsInt */




/*****************************************************************************/
/** fmGetMirrorPortDest
 * \ingroup intMirror
 *
 * \desc            Returns the destination mirror port to which a specified
 *                  port is being mirrored, or -1 if the port is not being
 *                  mirrored.
 *
 * \note            Assumes that the switch state table is already protected.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to be checked.
 *
 * \param[in]       mirrorType contains the mirror type.
 *
 * \return          The mirror port number or -1 if the port is not being
 *                  mirrored.
 *
 *****************************************************************************/
fm_int fmGetMirrorPortDest(fm_int sw, fm_int port, fm_mirrorType mirrorType)
{
    fm_portMirrorGroup *grp;
    fm_int              group;
    fm_int              mirrorPort = -1;
    fm_status           err        = FM_OK;
    fm_bool             ingressPortVal;
    fm_bool             egressPortVal;
    fm_bool             portVal;
    fm_switch *         switchPtr;
    fm_port *           portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MIRROR, "sw=%d port=%d mirrorType=%d\n",
                 sw, port, mirrorType);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    /**************************************************
     * Search all groups until a match is found.
     **************************************************/

    TAKE_MIRROR_LOCK(sw);

    for (group = 0 ; group < switchPtr->mirrorTableSize ; group++)
    {
        grp     = &switchPtr->mirrorGroups[group];
        portVal = FALSE;

        /* Consider only groups in use. */
        if (grp->used)
        {
            /* Consider only groups with a matching mirrorType. */
            if ((grp->mirrorType == mirrorType) ||
                (grp->mirrorType == FM_MIRROR_TYPE_BIDIRECTIONAL) )
            {
                switch (mirrorType)
                {
                    /**************************************************
                     * Bidirectional must show the port mirrored both
                     * for ingress and egress.
                     **************************************************/

                    case FM_MIRROR_TYPE_BIDIRECTIONAL:
                    case FM_MIRROR_TYPE_RX_INGRESS_TX_EGRESS:

                        err = fmGetBitArrayBit(&grp->ingressPortUsed,
                                               portPtr->portIndex,
                                               &ingressPortVal);

                        if (err == FM_OK)
                        {
                            err = fmGetBitArrayBit(&grp->egressPortUsed,
                                                   portPtr->portIndex,
                                                   &egressPortVal);
                        }

                        if (err == FM_OK)
                        {
                            if ( (ingressPortVal == TRUE) && (egressPortVal == TRUE) )
                            {
                                /* Port is ingress and egress mirrored. */
                                portVal = TRUE;
                            }
                        }

                        break;

                    case FM_MIRROR_TYPE_INGRESS:
                    case FM_MIRROR_TYPE_REDIRECT:
                        err = fmGetBitArrayBit(&grp->ingressPortUsed,
                                               portPtr->portIndex,
                                               &portVal);
                        break;

                    case FM_MIRROR_TYPE_EGRESS:
                    case FM_MIRROR_TYPE_TX_EGRESS:
                        err = fmGetBitArrayBit(&grp->egressPortUsed,
                                               portPtr->portIndex,
                                               &portVal);
                        break;

                    default:
                        err = FM_ERR_INVALID_VALUE;
                        break;

                }   /* end switch (mirrorType) */

                if (err != FM_OK)
                {
                    break;
                }

                /**************************************************
                 * If port is mirrored in all the specified
                 * directions in this mirror group, then we are
                 * done.
                 **************************************************/

                if (portVal)
                {
                    mirrorPort = grp->mirrorLogicalPort;
                    break;
                }

            }   /* end if ( (grp->mirrorType == mirrorType) ||... */

        }   /* end if ( grp->used) */

    }   /* end for (group = 0 ; group < switchPtr->mirrorTableSize ; group++) */

    DROP_MIRROR_LOCK(sw);

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_MIRROR, mirrorPort, "mirrorPort=%d\n", mirrorPort);

}   /* end fmGetMirrorPortDest */



/*****************************************************************************/
/** fmSetMirrorAttributeInt
 * \ingroup intMirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Set a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to 
 *                  set.
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
fm_status fmSetMirrorAttributeInt(fm_int sw,
                                  fm_int group,
                                  fm_int attr,
                                  void * value)
{
    fm_status           err;
    fm_portMirrorGroup *grp;
    fm_switch          *switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, group = %d, attr = %d, value = %p\n",
                  sw,
                  group,
                  attr,
                  (void *) value );

    /* Validate arguments */
    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    /* Pre-validate the attribute */
    switch (attr)
    {
        case FM_MIRROR_PRIORITY:
        case FM_MIRROR_TRUNCATE:
        case FM_MIRROR_SAMPLE_RATE:
        case FM_MIRROR_ACL:
        case FM_MIRROR_TX_EGRESS_PORT:
        case FM_MIRROR_TRUNCATE_OTHERS:
        case FM_MIRROR_TRUNCATE_MASK:
        case FM_MIRROR_VLAN:
        case FM_MIRROR_VLAN_PRI:
        case FM_MIRROR_TRAPCODE_ID:
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
            break;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* Get the pointer to the mirror group */
    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    /* Get the ACL lock and the state lock prior to the mirror lock to keep
     * the right precedence. ACL Lock is needed to make sure the updated group
     * is not being referenced by any ACL/rule. The state lock is needed when
     * notifying the CRM state machine about suspending/resuming TCAM checking
     * during update of ACL trigger mask. */
    if (attr == FM_MIRROR_ACL)
    {
        FM_TAKE_ACL_LOCK(sw);
        FM_TAKE_STATE_LOCK(sw);
    }
    /* Get the mirror lock */
    TAKE_MIRROR_LOCK(sw);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* Call switch-specific code to handle the attribute change */
    FM_API_CALL_FAMILY(err, switchPtr->SetMirrorAttribute, sw, grp, attr, value);

ABORT:
    /* Drop the locks */
    DROP_MIRROR_LOCK(sw);

    if (attr == FM_MIRROR_ACL)
    {
        FM_DROP_STATE_LOCK(sw);
        FM_DROP_ACL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmSetMirrorAttributeInt */



/*****************************************************************************/
/** fmSetMirrorAttribute
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Set a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to 
 *                  set.
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
fm_status fmSetMirrorAttribute(fm_int sw,
                               fm_int group,
                               fm_int attr,
                               void * value)
{
    fm_status           err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MIRROR,
                      "sw = %d, group = %d, attr = %d, value = %p\n",
                      sw,
                      group,
                      attr,
                      (void *) value );

    /* Get the switch lock */
    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmSetMirrorAttributeInt(sw, group, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmSetMirrorAttribute */




/*****************************************************************************/
/** fmGetMirrorAttributeInt
 * \ingroup intMirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Get a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to 
 *                  get.
 *
 * \param[out]      value points to caller-provided storage into which the
 *                  attribute's value will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is invalid.
 * \return          FM_ERR_UNSUPPORTED if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fmGetMirrorAttributeInt(fm_int sw,
                                  fm_int group,
                                  fm_int attr,
                                  void * value)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_portMirrorGroup *grp;

    FM_LOG_ENTRY( FM_LOG_CAT_MIRROR,
                  "sw = %d, group = %d, attr = %d, value = %p\n",
                  sw,
                  group,
                  attr,
                  (void *) value );

    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
    }

    /* Pre-validate the attribute */
    switch (attr)
    {
        case FM_MIRROR_PRIORITY:
        case FM_MIRROR_TRUNCATE:
        case FM_MIRROR_SAMPLE_RATE:
        case FM_MIRROR_ACL:
        case FM_MIRROR_TX_EGRESS_PORT:
        case FM_MIRROR_TRUNCATE_OTHERS:
        case FM_MIRROR_TRUNCATE_MASK:
        case FM_MIRROR_VLAN:
        case FM_MIRROR_VLAN_PRI:
        case FM_MIRROR_TRAPCODE_ID:
        case FM_MIRROR_INGRESS_COUNTER:
        case FM_MIRROR_EGRESS_COUNTER:
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MIRROR, FM_ERR_INVALID_ARGUMENT);
            break;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* Get the pointer to the mirror group */
    GET_PORT_MIRROR_GROUP_NO_SWLOCK_CHECK(sw, grp, group);

    /* Get the mirror lock */
    TAKE_MIRROR_LOCK(sw);

    if (!grp || !grp->used)
    {
        err = FM_ERR_INVALID_PORT_MIRROR_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MIRROR, err);
    }

    /* Call switch-specific code to retrieve the attribute value */
    FM_API_CALL_FAMILY(err, switchPtr->GetMirrorAttribute, sw, grp, attr, value);

ABORT:
    /* Drop the locks */
    DROP_MIRROR_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorAttributeInt */



/*****************************************************************************/
/** fmGetMirrorAttribute
 * \ingroup mirror
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Get a mirror attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the mirror group on which to operate.
 *
 * \param[in]       attr is the mirror attribute (see 'Mirror Attributes') to 
 *                  get.
 *
 * \param[out]      value points to caller-provided storage into which the
 *                  attribute's value will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT_MIRROR_GROUP if group is invalid.
 * \return          FM_ERR_UNSUPPORTED if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fmGetMirrorAttribute(fm_int sw,
                               fm_int group,
                               fm_int attr,
                               void * value)
{
    fm_status           err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MIRROR,
                      "sw = %d, group = %d, attr = %d, value = %p\n",
                      sw,
                      group,
                      attr,
                      (void *) value );

    /* Get the switch lock */
    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmGetMirrorAttributeInt(sw, group, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MIRROR, err);

}   /* end fmGetMirrorAttribute */
