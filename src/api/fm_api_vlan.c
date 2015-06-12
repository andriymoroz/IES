/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_vlan.c
 * Creation Date:   Jan 1, 2005
 * Description:     Structures and functions for dealing with VLAN
 *                  configuration
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

#include <fm_sdk_int.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmSetVlanMembership
 * \ingroup intVlan
 *
 * \desc            Function to set port vlan membership.  This function
 *                  assumes that the caller has taken the L2_LOCK.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry is the vlan entry on which to operate
 *
 * \param[in]       port is the logical port number
 *
 * \param[in]       state is TRUE if the port is to become a member of the
 *                  vlan, FALSE if it is to be removed from the vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetVlanMembership(fm_int        sw,
                              fm_vlanEntry *entry,
                              fm_int        port,
                              fm_bool       state)
{
    fm_status err;
    fm_port * portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, 
                 "sw=%d entry=%p port=%d state=%d\n", 
                 sw, 
                 (void *) 
                 entry, 
                 port, 
                 state);

    portPtr = GET_PORT_PTR(sw, port);

    if (!portPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_PORT);
    }

    FM_API_CALL_FAMILY(err, 
                       portPtr->SetVlanMembership, 
                       sw, 
                       entry, 
                       port, 
                       state);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanMembership */




/*****************************************************************************/
/** fmSetVlanTag
 * \ingroup intVlan
 *
 * \desc            Function to set port vlan tagging mode.  This function
 *                  assumes that the caller has taken the L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       entry is the vlan entry on which to operate
 *
 * \param[in]       port is the logical port number
 *
 * \param[in]       tag is TRUE if the port is to transmit tagged frames
 *                  for this vlan, FALSE if it is to transmit untagged frames
 *                  for this vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetVlanTag(fm_int        sw,
                       fm_vlanSelect vlanSel,
                       fm_vlanEntry *entry,
                       fm_int        port,
                       fm_bool       tag)
{
    fm_status err;
    fm_port * portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, 
                 "sw=%d vlanSel=%d entry=%p port=%d tag=%d\n", 
                 sw, 
                 vlanSel,
                 (void *) entry,
                 port, 
                 tag);

    portPtr = GET_PORT_PTR(sw, port);

    if (!portPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_PORT);
    }

    FM_API_CALL_FAMILY(err, portPtr->SetVlanTag, sw, vlanSel, entry, port, tag);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanTag */




/*****************************************************************************/
/** fmGetVlanMembership
 * \ingroup intVlan
 *
 * \desc            Function to retrieve port vlan membership.  This function
 *                  assumes that the caller has taken the L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry is the vlan entry on which to operate
 *
 * \param[in]       port is the logical port number
 *
 * \param[out]      state is TRUE if the port is a member of the vlan,
 *                  FALSE if it is not a vlan member.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetVlanMembership(fm_int        sw,
                              fm_vlanEntry *entry,
                              fm_int        port,
                              fm_bool *     state)
{
    fm_status err;
    fm_port * portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d entry=%p port=%d state=%p\n",
                 sw,
                 (void *) entry,
                 port,
                 (void *) state);

    portPtr = GET_PORT_PTR(sw, port);

    if (!portPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_PORT);
    }

    FM_API_CALL_FAMILY(err, 
                       portPtr->GetVlanMembership, 
                       sw, 
                       entry, 
                       port, 
                       state);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanMembership */




/*****************************************************************************/
/** fmGetVlanTag
 * \ingroup intVlan
 *
 * \desc            Function to retrieve port vlan tagging mode.  The function
 *                  assumes that the caller has taken L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       entry is the vlan entry on which to operate
 *
 * \param[in]       port is the logical port number
 *
 * \param[out]      tag is TRUE if the port transmits tagged frames for this
 *                  vlan, FALSE if it transmits untagged frames for this vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetVlanTag(fm_int        sw,
                       fm_vlanSelect vlanSel,
                       fm_vlanEntry *entry,
                       fm_int        port,
                       fm_bool *     tag)
{
    fm_status err;
    fm_port * portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d entry=%p port=%d tag=%p\n",
                 sw,
                 (void *)
                 entry,
                 port,
                 (void *) tag);

    if (!entry)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    portPtr = GET_PORT_PTR(sw, port);

    FM_API_CALL_FAMILY(err, portPtr->GetVlanTag, sw, vlanSel, entry, port, tag);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanTag */




/*****************************************************************************/
/** fmAllocateVlanTableDataStructures
 * \ingroup intVlan
 *
 * \desc            Allocates the state structure for VLAN management.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocateVlanTableDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "switchPtr=%p<sw=%d>\n",
                 (void *) switchPtr,
                 switchPtr ? switchPtr->switchNumber : -1);

    if (!switchPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    i                   = sizeof(fm_vlanEntry) * switchPtr->vlanTableSize;
    switchPtr->vidTable = (fm_vlanEntry *) fmAlloc(i);

    if (switchPtr->vidTable == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
    }

    memset(switchPtr->vidTable, 0, i);

    FM_API_CALL_FAMILY(err, switchPtr->AllocateVlanTableDataStructures, switchPtr);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmAllocateVlanTableDataStructures */




/*****************************************************************************/
/** fmInitVlanTable
 * \ingroup intVlan
 *
 * \desc            Initializes each entry of the VLAN table cache to match
 *                  chip defaults
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitVlanTable(fm_switch *switchPtr)
{
    fm_status err = FM_OK;
    fm_int    entry;

    FM_LOG_ENTRY( FM_LOG_CAT_VLAN,
                  "swstate=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr != NULL) ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    for (entry = 0 ; entry < switchPtr->vlanTableSize ; entry++)
    {
        /* no VLANs are valid on power-on */
        switchPtr->vidTable[entry].valid = FALSE;
    }

    FM_API_CALL_FAMILY(err, switchPtr->InitVlanTable, switchPtr);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmInitVlanTable */




/*****************************************************************************/
/** fmFreeVlanTableDataStructures
 * \ingroup intVlan
 *
 * \desc            Deallocates the state structure for VLAN management.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeVlanTableDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "swstate=%p<sw=%d>\n",
                 (void *) switchPtr, switchPtr ? switchPtr->switchNumber : -1);

    if (!switchPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    if (switchPtr->vidTable)
    {
        FM_API_CALL_FAMILY(err, switchPtr->FreeVlanTableDataStructures, switchPtr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, err);
        fmFree(switchPtr->vidTable);
        switchPtr->vidTable = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fmFreeVlanTableDataStructures */




/*****************************************************************************/
/** fmCreateVlanInt
 * \ingroup intVlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or is
 *                  the FM4000 reserved VLAN, ''FM_FM4000_RESERVED_VLAN''.
 * \return          FM_ERR_VLAN_ALREADY_EXISTS if vlanID already exists.
 *
 *****************************************************************************/
fm_status fmCreateVlanInt(fm_int sw, fm_uint16 vlanID)
{
    fm_status     err;
    fm_vlanEntry *ventry;
    fm_switch *   switchPtr;
    fm_bool       lagLockTaken;
    fm_bool       l2LockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d vlanID=%u\n", sw, vlanID);

    lagLockTaken = FALSE;
    l2LockTaken  = FALSE;

    if ( VLAN_OUT_OF_BOUNDS(vlanID) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * When creating a VLAN, we add all the internal
     * ports of a SWAG to that VLAN so that those
     * pathways will be transparent to the VLAN.
     * However, the LAG lock must be taken before the
     * L2 lock, so we need to take it here.
     **************************************************/

    if (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG)
    {
        FM_FLAG_TAKE_LAG_LOCK(sw);
    }

    FM_TAKE_L2_LOCK(sw);

    l2LockTaken = TRUE;

    ventry = &switchPtr->vidTable[vlanID];

    if (ventry->valid)
    {
        err = FM_ERR_VLAN_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    ventry->valid    = TRUE;
    ventry->vlanId   = vlanID;
    ventry->reflect  = FALSE;
    ventry->trapIGMP = FALSE;    /* do not trap IGMP frames to CPU GLORT */
    ventry->routable = FALSE;    /* do not route by default */

    FM_API_CALL_FAMILY(err, switchPtr->CreateVlan, sw, vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

ABORT:

    if (l2LockTaken)
    {
        FM_DROP_L2_LOCK(sw);
    }

    if (lagLockTaken)
    {
        FM_FLAG_DROP_LAG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmCreateVlanInt */




/*****************************************************************************/
/** fmCreateVlan
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or is
 *                  the FM4000 reserved VLAN, ''FM_FM4000_RESERVED_VLAN''.
 * \return          FM_ERR_VLAN_ALREADY_EXISTS if vlanID already exists.
 *
 *****************************************************************************/
fm_status fmCreateVlan(fm_int sw, fm_uint16 vlanID)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d vlanID=%u\n", sw, vlanID);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmCreateVlanInt(sw, vlanID);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmCreateVlan */




/*****************************************************************************/
/** fmDeleteVlan
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 *
 *****************************************************************************/
fm_status fmDeleteVlan(fm_int sw, fm_uint16 vlanID)
{
    fm_status     err;
    fm_switch *   switchPtr;
    fm_vlanEntry *ventry;
    fm_int        i;
    fm_lagInfo  * lagInfoPtr;
    fm_lag *      lagPtr;
    fm_bool       routingLockTaken;
    fm_bool       l2LockTaken;
    fm_bool       lagLockTaken;

    routingLockTaken = FALSE;
    l2LockTaken = FALSE;
    lagLockTaken = FALSE;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d vlanID=%u\n", sw, vlanID);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfoPtr = GET_LAG_INFO_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    routingLockTaken = TRUE;

    if (switchPtr->perLagMgmt)
    {
        TAKE_LAG_LOCK(sw);
        lagLockTaken = TRUE;
    }


    FM_TAKE_L2_LOCK(sw);
    l2LockTaken = TRUE;

    ventry = &switchPtr->vidTable[vlanID];

    if (!ventry->valid)
    {
        err = FM_ERR_INVALID_VLAN;
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, switchPtr->DeleteVlan, sw, vlanID);
    if (switchPtr->perLagMgmt)
    {
        for (i = 0 ; i < FM_MAX_NUM_LAGS ; i++)
        {
            lagPtr = lagInfoPtr->lag[i];
            if (lagPtr)
            {
                if (lagPtr->vlanMembership)
                {
                    lagPtr->vlanMembership[vlanID] = 0;
                }
            }
        }
    }

ABORT:

    if (l2LockTaken)
    {
        FM_DROP_L2_LOCK(sw);
    }

    if (lagLockTaken)
    {
        DROP_LAG_LOCK(sw);
    }

    if (routingLockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmDeleteVlan */




/*****************************************************************************/
/** fmAddVlanPortInternal
 * \ingroup intVlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a VLAN.
 *                                                                      \lb\lb
 *                  The port may be a physical port or a LAG. If a LAG, all
 *                  member physical ports of the LAG will be added to the
 *                  VLAN. Note that new physical ports added to the LAG
 *                  afterward must be manually added to the VLAN by the
 *                  application. If a member port is removed from the LAG,
 *                  its previous VLAN membership will not be restored to
 *                  what it was previously.
 *
 * \note            When a port is added to a VLAN, by default it will be
 *                  in a disabled state so that traffic is not forwarded.
 *                  Typically, this function should be followed by a call
 *                  to ''fmSetVlanPortState'' to set the port's state
 *                  to ''FM_STP_STATE_FORWARDING''. Alternatively, the default
 *                  forwarding state can be overridden by specifying the
 *                  ''api.stp.defaultState.vlanMember'' API property.
 *
 * \note            See also, ''fmAddVlanPortList'' which can be used to add
 *                  several ports to the VLAN at one time with faster
 *                  performance than when using this function one port at
 *                  a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to which the port should be
 *                  added.
 *
 * \param[in]       port is the number of the port to be added to the VLAN. May
 *                  be a LAG logical port number. May also be the CPU
 *                  interface port.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmAddVlanPortInternal(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    port,
                                fm_bool   tag)
{
    fm_status     err;
    fm_vlanEntry *ventry;
    fm_switch *   switchPtr;
    fm_port *     portPtr;
    fm_int        defaultStpState;
    fm_int        members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int        numMembers;
    fm_int        cnt;


    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vlanID=%u port=%d tag=%d\n",
                 sw,
                 vlanID,
                 port,
                 tag);

    err       = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    if (switchPtr->AddVlanPort != NULL)
    {
        /* Use the switch-specific method if one is provided.
         * This currently applies only to SWAGs. */
        err = switchPtr->AddVlanPort(sw, vlanID, port, tag);
        goto ABORT;
    }

    ventry = GET_VLAN_PTR(sw, vlanID);

    if (switchPtr->perLagMgmt && portPtr->portType == FM_PORT_TYPE_LAG)
    {
        TAKE_LAG_LOCK(sw);
        err = fmSetLAGVlanMembership(sw, vlanID, port, TRUE, tag);
        DROP_LAG_LOCK(sw);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    err = fmGetLAGCardinalPortList(sw,
                                   port,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_TAKE_L2_LOCK(sw);

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        port = members[cnt];

        FM_LOG_DEBUG(FM_LOG_CAT_VLAN,
                     "Adding vlan=%u port=%d\n",
                     vlanID,
                     port);

        err = fmSetVlanMembership(sw, ventry, port, TRUE);
        if (err != FM_OK)
        {
            goto ABORT_DROP_L2_LOCK;
        }

        err = fmSetVlanTag(sw, FM_VLAN_SELECT_VLAN1, ventry, port, tag);
        if (err != FM_OK)
        {
            goto ABORT_DROP_L2_LOCK;
        }

        if ( (port != switchPtr->cpuPort) &&
            (switchPtr->stpMode == FM_SPANNING_TREE_PER_VLAN) )
        {
            /* Get default state for a port that's a member of a VLAN. */
            defaultStpState = fmGetIntApiProperty(
                FM_AAK_API_STP_DEF_STATE_VLAN_MEMBER,
                FM_AAD_API_STP_DEF_STATE_VLAN_MEMBER);

            if ( fmIsInternalPort(sw, port) )
            {
                defaultStpState = FM_STP_STATE_FORWARDING;
            }

            err = fmSetVlanPortState(sw, vlanID, port, defaultStpState);

            if (err == FM_ERR_PORT_IS_INTERNAL)
            {
                 /* Port is already in forwarding */
                err = FM_OK;
            }
            else if (err != FM_OK)
            {
                goto ABORT_DROP_L2_LOCK;
            }
        }

    }   /* end for (cnt = 0 ; cnt < numMembers ; cnt++) */

    if (err == FM_OK)
    {
        FM_API_CALL_FAMILY(err, switchPtr->WriteVlanEntry, sw, vlanID);
    }

ABORT_DROP_L2_LOCK:
    FM_DROP_L2_LOCK(sw);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmAddVlanPortInternal */




/*****************************************************************************/
/** fmAddVlanPort
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a VLAN.
 *                                                                      \lb\lb
 *                  The port may be a physical port or a LAG. If a LAG, all
 *                  member physical ports of the LAG will be added to the
 *                  VLAN. Note that new physical ports added to the LAG
 *                  afterward must be manually added to the VLAN by the
 *                  application. If a member port is removed from the LAG,
 *                  its previous VLAN membership will not be restored to
 *                  what it was previously.
 *
 * \note            When a port is added to a VLAN, by default it will be
 *                  in a disabled state so that traffic is not forwarded.
 *                  Typically, this function should be followed by a call
 *                  to ''fmSetVlanPortState'' to set the port's state
 *                  to ''FM_STP_STATE_FORWARDING''. Alternatively, the default
 *                  forwarding state can be overridden by specifying the
 *                  ''api.stp.defaultState.vlanMember'' API property.
 *
 * \note            See also, ''fmAddVlanPortList'' which can be used to add
 *                  several ports to the VLAN at one time with faster
 *                  performance than when using this function one port at
 *                  a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to which the port should be
 *                  added.
 *
 * \param[in]       port is the number of the port to be added to the VLAN. May
 *                  be a LAG logical port number. May also be the CPU
 *                  interface port.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmAddVlanPort(fm_int sw, fm_uint16 vlanID, fm_int port, fm_bool tag)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d tag=%d\n",
                     sw,
                     vlanID,
                     port,
                     tag);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    err = fmAddVlanPortInternal(sw, vlanID, port, tag);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmAddVlanPort */




/*****************************************************************************/
/** fmAddVlanPortList
 * \ingroup vlan
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds multiple ports to a VLAN at one time. This function
 *                  provides a performance optimization over adding ports
 *                  to the VLAN one at a time using ''fmAddVlanPort''.
 *
 * \note            When a port is added to a VLAN, by default it will be
 *                  in a disabled state so that traffic is not forwarded.
 *                  Typically, this function should be followed by a call
 *                  to ''fmSetVlanPortState'' to set the port's state
 *                  to ''FM_STP_STATE_FORWARDING''. Alternatively, the default
 *                  forwarding state can be overridden by specifying the
 *                  ''api.stp.defaultState.vlanMember'' API property.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to which the ports should be
 *                  added.
 *
 * \param[in]       numPorts is the number of ports in the list.
 * 
 * \param[in]       portList points to an array containing the list of ports
 *                  to be added to the VLAN.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if any of the ports is invalid.
 *
 *****************************************************************************/
fm_status fmAddVlanPortList(fm_int    sw,
                            fm_uint16 vlanID,
                            fm_int    numPorts,
                            fm_int *  portList,
                            fm_bool   tag)
{
    fm_switch * switchPtr;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u numPorts=%d tag=%d\n",
                     sw, vlanID, numPorts, tag);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddVlanPortList,
                       sw,
                       vlanID,
                       numPorts,
                       portList,
                       tag);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmAddVlanPortList */




/*****************************************************************************/
/** fmDeleteVlanPort
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port from a VLAN.
 *
 * \note            See also, ''fmDeleteVlanPortList'' which can be used to
 *                  delete several ports from the VLAN at one time with faster
 *                  performance than when using this function one port at
 *                  a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which the port should be
 *                  deleted.
 *
 * \param[in]       port is the number of the port to be deleted from the VLAN.
 *                  May be the CPU interface port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteVlanPort(fm_int sw, fm_uint16 vlanID, fm_int port)
{
    fm_status      err = FM_OK;
    fm_vlanEntry * ventry;
    fm_switch *    switchPtr;
    fm_port *      portPtr;
    fm_int         defaultStpState;
    fm_int         members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int         numMembers;
    fm_int         cnt;
    fm_flushParams flushParams;


    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d\n",
                     sw, vlanID, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    if (switchPtr->DeleteVlanPort != NULL)
    {
        /* Use the switch-specific method if one is provided.
         * This currently applies only to SWAGs. */
        err = switchPtr->DeleteVlanPort(sw, vlanID, port);
        goto ABORT;
    }

    ventry = GET_VLAN_PTR(sw, vlanID);

    if (switchPtr->perLagMgmt && portPtr->portType == FM_PORT_TYPE_LAG)
    {
        TAKE_LAG_LOCK(sw);
        err = fmSetLAGVlanMembership(sw, vlanID, port, FALSE, FALSE);
        DROP_LAG_LOCK(sw);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    err = fmGetLAGCardinalPortList(sw,
                                   port,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_TAKE_L2_LOCK(sw);

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        port = members[cnt];

        if ( (port != switchPtr->cpuPort)
             && (switchPtr->stpMode == FM_SPANNING_TREE_PER_VLAN) )
        {
            /* Get default state for a port that is not a member of a VLAN. */
            defaultStpState = fmGetIntApiProperty(
                FM_AAK_API_STP_DEF_STATE_VLAN_NON_MEMBER,
                FM_AAD_API_STP_DEF_STATE_VLAN_NON_MEMBER);

            /* Force the port state back to the non-member default. */
            err = fmSetVlanPortState(sw, vlanID, port, defaultStpState);
            if (err != FM_OK)
            {
                goto ABORT_DROP_L2_LOCK;
            }
        }

        err = fmSetVlanMembership(sw, ventry, port, FALSE);
        if (err != FM_OK)
        {
            goto ABORT_DROP_L2_LOCK;
        }
    }

    FM_API_CALL_FAMILY(err, switchPtr->WriteVlanEntry, sw, vlanID);
    if (err != FM_OK)
    {
        goto ABORT_DROP_L2_LOCK;
    }

    if (fmGetBoolApiProperty(FM_AAK_API_MA_FLUSH_ON_VLAN_CHANGE, 
                             FM_AAD_API_MA_FLUSH_ON_VLAN_CHANGE))
    {
        flushParams.port = port;
        flushParams.vid1 = vlanID;
        err = fmFlushAddresses(sw, FM_FLUSH_MODE_PORT_VLAN, flushParams);
    }

ABORT_DROP_L2_LOCK:
    FM_DROP_L2_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmDeleteVlanPort */




/*****************************************************************************/
/** fmDeleteVlanPortList
 * \ingroup vlan
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes multiple ports from a VLAN at one time. This 
 *                  function provides a performance optimization over deleting
 *                  ports from the VLAN one at a time using 
 *                  ''fmDeleteVlanPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which the ports should be
 *                  deleted.
 *
 * \param[in]       numPorts is the number of ports in the list.
 * 
 * \param[in]       portList points to an array containing the list of ports
 *                  to be removed from the VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if any of the ports is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteVlanPortList(fm_int    sw,
                               fm_uint16 vlanID,
                               fm_int    numPorts,
                               fm_int *  portList)
{
    fm_switch *    switchPtr;
    fm_status      err = FM_OK;
    fm_int         i;
    fm_flushParams flushParams;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u numPorts=%d\n",
                     sw, vlanID, numPorts);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteVlanPortList,
                       sw,
                       vlanID,
                       numPorts,
                       portList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    if (fmGetBoolApiProperty(FM_AAK_API_MA_FLUSH_ON_VLAN_CHANGE, 
                             FM_AAD_API_MA_FLUSH_ON_VLAN_CHANGE))
    {
        flushParams.vid1 = vlanID;
        for (i = 0; i < numPorts; i++)
        {
            flushParams.port = portList[i];
            err = fmFlushAddresses(sw, FM_FLUSH_MODE_PORT_VLAN, flushParams);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
        }
    }

    UNPROTECT_SWITCH(sw);

ABORT:
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmDeleteVlanPortList */




/*****************************************************************************/
/** fmChangeVlanPortInternal
 * \ingroup intVlan
 *
 * \desc            Change the tagging option for a port in a VLAN.
 *
 * \note            If the specified port is not a member of the VLAN, this
 *                  function will not fail, but the setting will not cause
 *                  any change in the behavior of the switch except for the
 *                  cpu port. Ports that are not member of a VLAN will get
 *                  their frames dropped due to egress VLAN boundary violation.
 *                  Trapped frames to the CPU do not have egress boundary
 *                  violation check and therefore changes to the CPU port will
 *                  modify the switch behavior. When the port is added to the
 *                  VLAN with a call to ''fmAddVlanPort'' the tagging behavior
 *                  for the port will be set as specified by the arguments to
 *                  that function.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the port number on which to operate. May
 *                  be the CPU interface port.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmChangeVlanPortInternal(fm_int        sw,
                                   fm_vlanSelect vlanSel,
                                   fm_uint16     vlanID,
                                   fm_int        port,
                                   fm_bool       tag)
{
    fm_status     err = FM_OK;
    fm_switch *   switchPtr;
    fm_port *     portPtr;
    fm_vlanEntry *ventry;
    fm_int        members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int        numMembers;
    fm_int        cnt;


    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d tag=%d\n",
                     sw, vlanID, port, tag);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    if (switchPtr->ChangeVlanPort != NULL)
    {
        err = switchPtr->ChangeVlanPort(sw, vlanSel, vlanID, port, tag);
        goto ABORT;
    }

    ventry = &switchPtr->vidTable[vlanID];

    if (switchPtr->perLagMgmt && portPtr->portType == FM_PORT_TYPE_LAG)
    {
        TAKE_LAG_LOCK(sw);
        err = fmSetLAGVlanTag(sw, vlanSel, vlanID, port, tag);
        DROP_LAG_LOCK(sw);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    err = fmGetLAGCardinalPortList(sw,
                                   port,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_TAKE_L2_LOCK(sw);

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        port = members[cnt];

        err = fmSetVlanTag(sw, vlanSel, ventry, port, tag);
        if (err != FM_OK)
        {
            goto ABORT_DROP_L2_LOCK;
        }
    }

    FM_API_CALL_FAMILY(err, switchPtr->WriteTagEntry, sw, vlanID);

ABORT_DROP_L2_LOCK:
    FM_DROP_L2_LOCK(sw);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmChangeVlanPortInternal */




/*****************************************************************************/
/** fmChangeVlanPort
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Change the tagging option for a port in a VLAN.
 *                                                                      \lb\lb
 *                  On FM6000 devices, this function specifies tagging for 
 *                  VLAN1. To operate on VLAN2 for these devices, see 
 *                  ''fmChangeVlanPortExt''.
 *
 * \note            If the specified port is not a member of the VLAN, this
 *                  function will not fail, but the setting will not cause
 *                  any change in the behavior of the switch except for the
 *                  CPU port. Ports that are not members of a VLAN will get
 *                  their frames dropped due to egress VLAN boundary violations.
 *                  Trapped frames to the CPU are not checked for egress boundary
 *                  violations and therefore changes to the CPU port will
 *                  modify the switch behavior. When the port is added to the
 *                  VLAN with a call to ''fmAddVlanPort'' the tagging behavior
 *                  for the port will be set as specified by the arguments to
 *                  that function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the port number on which to operate. May
 *                  be the CPU interface port.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmChangeVlanPort(fm_int    sw,
                           fm_uint16 vlanID,
                           fm_int    port,
                           fm_bool   tag)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d tag=%d\n",
                     sw,
                     vlanID,
                     port,
                     tag);

    err = fmChangeVlanPortInternal(sw, FM_VLAN_SELECT_VLAN1, vlanID, port, tag);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmChangeVlanPort */




/*****************************************************************************/
/** fmChangeVlanPortExt
 * \ingroup vlan
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Change the tagging option for a port in a VLAN.
 *
 * \note            If the specified port is not a member of the VLAN, this
 *                  function will not fail, but the setting will not cause
 *                  any change in the behavior of the switch except for the
 *                  CPU port. Ports that are not members of a VLAN will get
 *                  their frames dropped due to egress VLAN boundary violations.
 *                  Trapped frames to the CPU are not checked for egress boundary
 *                  violations and therefore changes to the CPU port will
 *                  modify the switch behavior. When the port is added to the
 *                  VLAN with a call to ''fmAddVlanPort'' the tagging behavior
 *                  for the port will be set as specified by the arguments to
 *                  that function.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel indicates which VLAN field in an egressing packet
 *                  to look at when deciding to include or exclude the
 *                  tag (see ''fm_vlanSelect''). For FM10000, the only valid
 *                  value for vlanSel is FM_VLAN_SELECT_VLAN1.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the port number on which to operate. May
 *                  be the CPU interface port.
 *
 * \param[in]       tag should be:
 *                      - TRUE to tag egressing frames on port.
 *                      - FALSE to not tag egressing frames on port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_UNSUPPORTED if an unsupported value for vlanSel is
 *                  used on FM10000.
 *
 *****************************************************************************/
fm_status fmChangeVlanPortExt(fm_int        sw,
                              fm_vlanSelect vlanSel,
                              fm_uint16     vlanID,
                              fm_int        port,
                              fm_bool       tag)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanSel=%d vlanID=%u port=%d tag=%d\n",
                     sw,
                     vlanSel,
                     vlanID,
                     port,
                     tag);

    err = fmChangeVlanPortInternal(sw, vlanSel, vlanID, port, tag);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmChangeVlanPortExt */




/*****************************************************************************/
/** fmGetVlanList
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the list of existing VLANs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nVlan points to caller-allocated storage where this
 *                  function should place the number of VLANs listed.
 *
 * \param[out]      vlanIDs points to caller-allocated array where this
 *                  function should place the list of existing VLAN numbers.
 *
 * \param[in]       maxVlans is the size of the vlanIDs array.  This function
 *                  will not retrieve more than maxVlans number of VLANs.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if the vlanIDs got filled.
 *
 *****************************************************************************/
fm_status fmGetVlanList(fm_int     sw,
                        fm_int *   nVlan,
                        fm_uint16 *vlanIDs,
                        fm_int     maxVlans)
{
    fm_int     vlan;
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d nVlan=%p vlanIDs=%p max=%d\n",
                     sw,
                     (void *) nVlan,
                     (void *) vlanIDs,
                     maxVlans);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* No lock required. */
    /* starting from vlan 1 since we only use vlan 0 internally */
    for (*nVlan = 0, vlan = 1 ;
         (*nVlan < maxVlans) && (vlan < switchPtr->vlanTableSize) ;
         vlan++)
    {
        if (switchPtr->vidTable[vlan].valid)
        {
            *vlanIDs++ = vlan;
            (*nVlan)++;
        }
    }

    if (*nVlan >= maxVlans)
    {
        err = FM_ERR_BUFFER_FULL;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanList */




/*****************************************************************************/
/** fmGetVlanPortList
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the list of ports in a particular VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which to get the port list.
 *
 * \param[out]      nPorts points to caller-allocated storage where this
 *                  function should place the number of ports listed.
 *
 * \param[out]      ports points to caller-allocated array where this
 *                  function should place the list of ports in the VLAN.
 *
 * \param[in]       maxPorts is the size of the ports array.  This function
 *                  will not retrieve more than maxPorts number of ports.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vlanID is invalid.
 * \return          FM_ERR_BUFFER_FULL if the ports got filled.
 *
 *****************************************************************************/
fm_status fmGetVlanPortList(fm_int    sw,
                            fm_uint16 vlanID,
                            fm_int *  nPorts,
                            fm_int *  ports,
                            fm_int    maxPorts)
{
    fm_status  err = FM_OK;
    fm_int     cpi;
    fm_int     port;
    fm_bool    isMember;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u nPorts=%p ports=%p max=%d\n",
                     sw,
                     vlanID,
                     (void *) nPorts,
                     (void *) ports,
                     maxPorts);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (*nPorts = 0, cpi = 0 ;
         (*nPorts < maxPorts) && (cpi < switchPtr->numCardinalPorts) ;
         cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        err = fmGetVlanMembership(sw,
                                  &switchPtr->vidTable[vlanID],
                                  port,
                                  &isMember);

        if (err != FM_OK)
        {
            goto ABORT;
        }

        if (isMember)
        {
            (*nPorts)++;

            if (*nPorts >= maxPorts)
            {
                err = FM_ERR_BUFFER_FULL;
                goto ABORT;
            }

            *ports++ = port;
        }
    }


ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortList */




/*****************************************************************************/
/** fmGetVlanFirst
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing VLAN number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstID points to caller-allocated storage where this
 *                  function should place the first existing VLAN number.
 *                  Will be set to -1 if no VLANs found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetVlanFirst(fm_int sw, fm_int *firstID)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_VLAN,
                             "sw=%d firstID=%p\n",
                             sw,
                             (void *) firstID);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* No lock required. */
    /* For Tahoe even if vlan 0 is created internally we will not return it here.
     * For Bali Vlan 0 is not used */
    for (*firstID = 1 ; (*firstID < switchPtr->vlanTableSize) ; (*firstID)++)
    {
        if ( ( switchPtr->vidTable[*firstID].valid ) &&
            ( switchPtr->reservedVlan != (fm_uint16) (*firstID) ) )
        {
            goto DONE;
        }

        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_VLAN, "firstID=%d\n", *firstID);
    }

    *firstID = -1;

DONE:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fmGetVlanFirst */




/*****************************************************************************/
/** fmGetVlanNext
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing VLAN number, following a prior
 *                  call to this function or to ''fmGetVlanFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startID is the last VLAN number found by a previous
 *                  call to this function or to ''fmGetVlanFirst''.
 *
 * \param[out]      nextID points to caller-allocated storage where
 *                  this function should place the number of the next VLAN.
 *                  Will be set to -1 if no more VLANs found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 *
 *****************************************************************************/
fm_status fmGetVlanNext(fm_int sw, fm_int startID, fm_int *nextID)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_VLAN,
                             "sw=%d startID=%d nextID=%p\n",
                             sw,
                             startID,
                             (void *) nextID);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, startID);

    switchPtr = GET_SWITCH_PTR(sw);

    /* No lock required. */

    for (*nextID = startID + 1 ;
         (*nextID < switchPtr->vlanTableSize) ;
         (*nextID)++)
    {
        if ( ( switchPtr->vidTable[*nextID].valid ) &&
            ( switchPtr->reservedVlan != (fm_uint16) (*nextID) ) )
        {
            goto DONE;
        }
    }

    *nextID = -1;

DONE:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fmGetVlanNext */




/*****************************************************************************/
/** fmGetVlanPortFirst
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the number of the VLAN that should be searched
 *                  for ports.
 *
 * \param[out]      firstPort points to caller-allocated storage where this
 *                  function should place the first port in the VLAN.
 *                  Will be set to -1 if no ports found in VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 *
 *****************************************************************************/
fm_status fmGetVlanPortFirst(fm_int sw, fm_int vlanID, fm_int *firstPort)
{
    fm_int     cpi;
    fm_status  err;
    fm_bool    isMember;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_VLAN,
                             "sw=%d vlanID=%d firstPort=%p\n",
                             sw,
                             vlanID,
                             (void *) firstPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        *firstPort = GET_LOGICAL_PORT(sw, cpi);

        err = fmGetVlanMembership(sw,
                                  &switchPtr->vidTable[vlanID],
                                  *firstPort,
                                  &isMember);

        if (err != FM_OK)
        {
            goto ABORT;
        }

        if (isMember)
        {
            goto ABORT;
        }
    }

    *firstPort = -1;
    err        = FM_OK;

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortFirst */




/*****************************************************************************/
/** fmGetVlanPortNext
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the number of the VLAN that should be searched
 *                  for ports.
 *
 * \param[in]       startPort is the last port number found by a previous
 *                  call to this function or to ''fmGetVlanPortFirst''. May
 *                  be the CPU interface port.
 *
 * \param[out]      nextPort points to caller-allocated storage where this
 *                  function should place the next port in the VLAN.
 *                  Will be set to -1 if no more ports found in VLAN.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if startPort is invalid.
 *
 *****************************************************************************/
fm_status fmGetVlanPortNext(fm_int  sw,
                            fm_int  vlanID,
                            fm_int  startPort,
                            fm_int *nextPort)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_status   err;
    fm_bool     isMember;

    FM_LOG_ENTRY_API_VERBOSE(FM_LOG_CAT_VLAN,
                             "sw=%d vlanID=%d startport=%d nextPort=%p\n",
                             sw,
                             vlanID,
                             startPort,
                             (void *) nextPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, startPort, ALLOW_CPU);

    switchPtr = GET_SWITCH_PTR(sw);

    cpi = GET_PORT_INDEX(sw, startPort);

    for (cpi = cpi + 1 ; cpi < switchPtr->numCardinalPorts; cpi++)
    {
        *nextPort = GET_LOGICAL_PORT(sw, cpi);

        err = fmGetVlanMembership(sw,
                                  &switchPtr->vidTable[vlanID],
                                  *nextPort,
                                  &isMember);

        if (err != FM_OK)
        {
            goto ABORT;
        }

        if (isMember)
        {
            goto ABORT;
        }
    }

    *nextPort = -1;
    err       = FM_OK;

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_VERBOSE(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortNext */




/*****************************************************************************/
/** fmGetVlanPortTagInternal
 * \ingroup intVlan
 *
 * \desc            Retrieve the tagging state for a port in a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       vlanID is the number of the VLAN on which to operate.
 *
 * \param[in]       port is the number of the port to be added to the VLAN. May
 *                  be the CPU interface port.
 *
 * \param[out]      tag points to caller-allocated storage where this
 *                  function should place the boolean tagging state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid or is not a member
 *                  of the VLAN.
 *
 *****************************************************************************/
fm_status fmGetVlanPortTagInternal(fm_int        sw,
                                   fm_vlanSelect vlanSel,
                                   fm_int        vlanID,
                                   fm_int        port,
                                   fm_bool      *tag)
{
    fm_status  err;
    fm_bool    isMember;
    fm_switch *switchPtr;
    fm_port *  portPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanSel=%d vlanID=%d port=%d tag=%p\n",
                     sw,
                     vlanSel,
                     vlanID,
                     port,
                     (void *) tag);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    if (portPtr->portType == FM_PORT_TYPE_LAG)
    {
        TAKE_LAG_LOCK(sw);

        if (switchPtr->perLagMgmt)
        {
            /* Retrieve the tagging state from the LAG itself. */
            err = fmGetLAGVlanTag(sw, vlanSel, vlanID, port, tag);
            DROP_LAG_LOCK(sw);
            goto ABORT;
        }
        else
        {
            /* For a LAG port, assuming all member ports
             * will have the same property, so we just
             * take from one of the physical member port.
             */
            err = fmGetFirstPhysicalMemberPort(sw, port, &port);
            DROP_LAG_LOCK(sw);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
        }

    }

    err = fmGetVlanMembership(sw,
                              &switchPtr->vidTable[vlanID],
                              port,
                              &isMember);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    if (!isMember)
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    err = fmGetVlanTag(sw, vlanSel, &switchPtr->vidTable[vlanID], port, tag);

    if (err != FM_OK)
    {
        goto ABORT;
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortTagInternal */




/*****************************************************************************/
/** fmGetVlanPortTag
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the tagging state for a port in a VLAN.
 *                                                                      \lb\lb
 *                  On FM6000 devices, this function retrieves the
 *                  tagging state for VLAN1. To operate on VLAN2 for these 
 *                  devices, see ''fmGetVlanPortTagExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the number of the VLAN on which to operate.
 *
 * \param[in]       port is the number of the port to be added to the VLAN. May
 *                  be the CPU interface port.
 *
 * \param[out]      tag points to caller-allocated storage where this
 *                  function should place the boolean tagging state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid or is not a member
 *                  of the VLAN.
 *
 *****************************************************************************/
fm_status fmGetVlanPortTag(fm_int sw, fm_int vlanID, fm_int port, fm_bool *tag)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%d port=%d tag=%p\n",
                     sw,
                     vlanID,
                     port,
                     (void *) tag);

    err = fmGetVlanPortTagInternal(sw, FM_VLAN_SELECT_VLAN1, vlanID, port, tag);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortTag */




/*****************************************************************************/
/** fmGetVlanPortTagExt
 * \ingroup vlan
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the tagging state for a port in a VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel indicates which VLAN field in an egressing packet
 *                  to look at when deciding to include or exclude the
 *                  tag (see ''fm_vlanSelect'').
 *
 * \param[in]       vlanID is the number of the VLAN on which to operate.
 *
 * \param[in]       port is the number of the port to be added to the VLAN. May
 *                  be the CPU interface port.
 *
 * \param[out]      tag points to caller-allocated storage where this
 *                  function should place the boolean tagging state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid or is not a member
 *                  of the VLAN.
 *
 *****************************************************************************/
fm_status fmGetVlanPortTagExt(fm_int        sw,
                              fm_vlanSelect vlanSel,
                              fm_int        vlanID,
                              fm_int        port,
                              fm_bool      *tag)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanSel=%d vlanID=%d port=%d tag=%p\n",
                     sw,
                     vlanSel,
                     vlanID,
                     port,
                     (void *) tag);

    err = fmGetVlanPortTagInternal(sw, vlanSel, vlanID, port, tag);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortTagExt */




/*****************************************************************************/
/** fmSetVlanPortStateInternal
 * \ingroup intVlan
 *
 * \desc            Set the spanning tree forwarding state of a port in a VLAN.
 *                  This version is only called from other API functions.  It
 *                  doesn't perform any locking prevention.  It is assumed that
 *                  the switch lock and the REG lock are taken.
 *
 * \note            This function should only be used when FM_SPANNING_TREE_MODE
 *                  is set to FM_SPANNING_TREE_SHARED or
 *                  FM_SPANNING_TREE_PER_VLAN (see ''Switch Attributes'').
 *                  For FM_SPANNING_TREE_MULTIPLE, use the ''Spanning Tree
 *                  Management'' functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the number of the port on which to operate. May
 *                  not be the CPU interface port.
 *
 * \param[in]       state is the spanning tree state (see 'Spanning Tree States')
 *                  to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmSetVlanPortStateInternal(fm_int    sw,
                                     fm_uint16 vlanID,
                                     fm_int    port,
                                     fm_int    state)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vlanID=%u port=%d state=%d\n",
                 sw, vlanID, port, state);

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * Perform any needed switch-specific operations
     **************************************************/

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetVlanPortState,
                       sw,
                       vlanID,
                       port,
                       state);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanPortStateInternal */




/*****************************************************************************/
/** fmSetVlanPortState
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set the spanning tree forwarding state of a port in a VLAN.
 *
 * \note            This function should only be used when the
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to
 *                  ''FM_SPANNING_TREE_SHARED'' or ''FM_SPANNING_TREE_PER_VLAN''.
 *                  For ''FM_SPANNING_TREE_MULTIPLE'', use the ''Spanning Tree
 *                  Management'' functions.
 *
 * \note            This function will operate correctly on FM6000 and FM10000
 *                  devices for backwards compatibility, but is deprecated for
 *                  those devices. Instead, use of the ''Spanning Tree Management''
 *                  API is strongly recommended.
 *
 * \note            See also, ''fmSetVlanPortListState'' which can be used to 
 *                  set the state of several ports at one time with faster
 *                  performance than when using this function one port at
 *                  a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the number of the port on which to operate. May
 *                  not be the CPU interface port.
 *
 * \param[in]       state is the spanning tree state (see 'Spanning Tree States')
 *                  to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_PORT_IS_INTERNAL if trying to change the state of
 *                  an internal port.
 *
 *****************************************************************************/
fm_status fmSetVlanPortState(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    port,
                             fm_int    state)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_int     members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int     numMembers;
    fm_int     cnt;
    fm_bool    l2LockTaken = FALSE;
    fm_int     currentStpState;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d state=%d\n",
                     sw,
                     vlanID,
                     port,
                     state);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( VLAN_OUT_OF_BOUNDS(vlanID) || (vlanID == switchPtr->reservedVlan) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    VALIDATE_FID_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_LAG);

    portPtr = GET_PORT_PTR(sw, port);

    if (switchPtr->perLagMgmt && portPtr->portType == FM_PORT_TYPE_LAG)
    {
        TAKE_LAG_LOCK(sw);
        err = fmSetLAGVlanPortState(sw, vlanID, port, state);
        DROP_LAG_LOCK(sw);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    err = fmGetLAGCardinalPortList(sw,
                                   port,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    FM_TAKE_L2_LOCK(sw);
    l2LockTaken = TRUE;

    /***************************************************
     * Perform any needed switch-specific operations
     **************************************************/

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        port = members[cnt];

        /* Cannot change the STP state of an internal port, the state
         * is forced to FM_STP_STATE_FORWARDING */
        if ( fmIsInternalPort(sw, port) )
        {
            if ( !fmGetBoolApiProperty(FM_AAK_API_STP_ENABLE_INTERNAL_PORT_CTRL, 
                                       FM_AAD_API_STP_ENABLE_INTERNAL_PORT_CTRL) )
            {
                err = fmGetVlanPortState(sw, vlanID, port, &currentStpState);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

                if (currentStpState != FM_STP_STATE_FORWARDING)
                {
                    /* This case will occur when creating vlan */
                }
                else
                {
                    err = FM_ERR_PORT_IS_INTERNAL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
                }
            }
        }
        
        err = fmSetVlanPortStateInternal(sw, vlanID, port, state);
        if (err != FM_OK)
        {
            break;
        }
    }

ABORT:

    if (l2LockTaken)
    {
        FM_DROP_L2_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanPortState */




/*****************************************************************************/
/** fmSetVlanPortListState
 * \ingroup vlan
 *
 * \chips           FM4000
 *
 * \desc            Sets the spanning tree forwarding state of multiple ports 
 *                  in a VLAN at one time. This function provides a 
 *                  performance optimization over setting the state of ports
 *                  in the VLAN one at a time using ''fmSetVlanPortState''.
 *
 * \note            This function should only be used when the
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to
 *                  ''FM_SPANNING_TREE_SHARED'' or ''FM_SPANNING_TREE_PER_VLAN''.
 *                  For ''FM_SPANNING_TREE_MULTIPLE'', use the ''Spanning Tree
 *                  Management'' functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       numPorts is the number of ports in the list.
 * 
 * \param[in]       portList points to an array containing the list of ports 
 *                  on which to operate. May not be the CPU interface port.
 *
 * \param[in]       state is the spanning tree state to set (see ''Spanning 
 *                  Tree States'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if any of the ports are invalid.
 *
 *****************************************************************************/
fm_status fmSetVlanPortListState(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    numPorts,
                                 fm_int *  portList,
                                 fm_int    state)
{
    fm_switch * switchPtr;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u numPorts=%d state=%d\n",
                     sw, vlanID, numPorts, state);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetVlanPortListState,
                       sw,
                       vlanID,
                       numPorts,
                       portList,
                       state);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanPortListState */




/*****************************************************************************/
/** fmGetVlanPortState
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            Retrieve the spanning tree forwarding state of a port in a
 *                  VLAN.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the number of the port on which to operate. May
 *                  not be the CPU interface port.
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function should place the spanning tree state
 *                  (see 'Spanning Tree States').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetVlanPortState(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    port,
                             fm_int *  state)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d state=%p\n",
                     sw,
                     vlanID,
                     port,
                     (void *) state);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_FID_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    err = fmGetVlanPortStateInternal(sw, vlanID, port, state);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortState */




/*****************************************************************************/
/** fmGetVlanPortStateInternal
 * \ingroup intVlan
 *
 * \desc            Retrieve the spanning tree forwarding state of a port in a
 *                  VLAN.
 *
 * \note            This function provides the core of ''fmGetVlanPortState''
 *                  but is an entry point to be called internally from the
 *                  API itself (e.g., from the MA Table maintenance task),
 *                  bypassing the ''FM_LOG_ENTRY_API'' logging call.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the number of the port on which to operate. May
 *                  not be the CPU interface port.
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function should place the spanning tree state
 *                  (see 'Spanning Tree States').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetVlanPortStateInternal(fm_int    sw,
                                     fm_uint16 vlanID,
                                     fm_int    port,
                                     fm_int *  state)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_int     stpInstance;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vlanID=%u port=%d state=%p\n",
                 sw,
                 vlanID,
                 port,
                 (void *) state);

    switchPtr = GET_SWITCH_PTR(sw);

    if (port == switchPtr->cpuPort)
    {
        /* Special case, always assume forwarding */
        *state = FM_STP_STATE_FORWARDING;
        err    = FM_OK;
    }
    else if (switchPtr->GetVlanPortState != NULL)
    {
        /* Otherwise, proceed to the chip specific functions */

        portPtr = GET_PORT_PTR(sw, port);

        if (portPtr->portType == FM_PORT_TYPE_LAG)
        {
            TAKE_LAG_LOCK(sw);
    
            if (switchPtr->perLagMgmt)
            {
                /* Retrieve the STP state from the LAG itself. */
                err = fmGetLAGVlanPortState(sw, vlanID, port, state);
                DROP_LAG_LOCK(sw);
                goto ABORT;
            }
            else
            {
                /* For a LAG port, assuming all member ports
                 * will have the same property, so we just
                 * take from one of the physical member port.
                 */
                err = fmGetFirstPhysicalMemberPort(sw, port, &port);
                DROP_LAG_LOCK(sw);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
            }
    
        }

        /***************************************************
         * Perform any needed switch-specific operations
         **************************************************/

        FM_API_CALL_FAMILY(err,
                           switchPtr->GetVlanPortState,
                           sw,
                           vlanID,
                           port,
                           state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    }
    else
    {
        err = fmFindInstanceForVlan(sw, vlanID, &stpInstance);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

        err = fmGetSpanningTreePortState(sw, stpInstance, port, state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortStateInternal */




/*****************************************************************************/
/** fmGetVlanAttribute
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a VLAN attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which to get the attribute.
 *
 * \param[in]       attr is the VLAN attribute (see 'VLAN Attributes') to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fmGetVlanAttribute(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    attr,
                             void *    value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u attr=%d value=%p\n",
                     sw,
                     vlanID,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetVlanAttribute,
                       sw,
                       vlanID,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanAttribute */




/*****************************************************************************/
/** fmSetVlanAttribute
 * \ingroup vlan
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a VLAN attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number for which to set the attribute.
 *
 * \param[in]       attr is the VLAN attribute (see 'VLAN Attributes') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fmSetVlanAttribute(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    attr,
                             void *    value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u attr=%d value=%p\n",
                     sw,
                     vlanID,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_TAKE_L2_LOCK(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetVlanAttribute,
                       sw,
                       vlanID,
                       attr,
                       value);

    FM_DROP_L2_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanAttribute */




/*****************************************************************************/
/** fmGetVlanPortAttribute
 * \ingroup intVlan
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Gets a VLAN port attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which to get the attribute.
 * 
 * \param[in]       port is the port number within the VLAN.
 *
 * \param[in]       attr is the VLAN port attribute (see 'VLAN Port Attributes')
 *                  to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is out of range or does not
 *                  exist.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LAG if LAG is invalid.
 * \return          FM_ERR_UNSUPPORTED if per-lag attributes not supported.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fmGetVlanPortAttribute(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    port,
                                 fm_int    attr,
                                 void *    value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d vlanID=%u port=%d attr=%d value=%p\n",
                     sw,
                     vlanID,
                     port,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU | ALLOW_LAG);

    switchPtr = GET_SWITCH_PTR(sw);

    if (fmIsCardinalPort(sw, port))
    {
        FM_API_CALL_FAMILY(err,
                           switchPtr->GetVlanPortAttribute,
                           sw,
                           vlanID,
                           port,
                           attr,
                           value);
    }
    else
    {
        TAKE_LAG_LOCK(sw);
        err = fmGetLAGVlanAttribute(sw, vlanID, port, attr, value);
        DROP_LAG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetVlanPortAttribute */




/*****************************************************************************/
/** fmExtractVlanPhysicalPortList
 * \ingroup intVlan
 *
 * \desc            Extracts a list of physical ports from a vlan port list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       numVlanPorts is the number of ports in the vlan port list.
 * 
 * \param[in]       vlanPortList points to an array containing the list of
 *                  logical ports from which the physical ports are to be
 *                  extracted.
 * 
 * \param[out]      numPhysPorts points to a caller-supplied variable in which
 *                  this function should store the number of ports returned
 *                  in the physical port list.
 * 
 * \param[out]      physPortList points to a caller-supplied array in which
 *                  this function should store the list of physical ports.
 * 
 * \param[in]       maxPhysPorts is the size of the physPortList array. The
 *                  function will not return more than the maximum number of
 *                  physical ports.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if one of the port numbers is invalid.
 * \return          FM_ERR_BUFFER_FULL if the physical port list is too small.
 *
 *****************************************************************************/
fm_status fmExtractVlanPhysicalPortList(fm_int   sw,
                                        fm_int   numVlanPorts,
                                        fm_int * vlanPortList,
                                        fm_int * numPhysPorts,
                                        fm_int * physPortList,
                                        fm_int   maxPhysPorts)
{
    fm_switch * switchPtr;
    fm_int      memberList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int      numMembers;
    fm_portmask portMask;
    fm_int      listIndex;
    fm_int      port;
    fm_int      cpi;
    fm_int      i;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d numPorts=%d maxPhysPorts=%d\n",
                 sw,
                 numVlanPorts,
                 maxPhysPorts);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_PORTMASK_DISABLE_ALL(&portMask);
    *numPhysPorts = 0;

    for (listIndex = 0 ; listIndex < numVlanPorts ; listIndex++)
    {
        port = vlanPortList[listIndex];

        /* Make sure it's a valid port type. */
        if ( !fmIsValidPort(sw, port, ALLOW_CPU | ALLOW_LAG) )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_VLAN,
                         "port %d is an unsupported type\n",
                         port);
            err = FM_ERR_INVALID_PORT;
            goto ABORT;
        }

        /* Get the logical ports for this logical port. */
        err = fmGetLAGCardinalPortList(sw,
                                       port,
                                       &numMembers,
                                       memberList,
                                       FM_MAX_NUM_LAG_MEMBERS);
        if (err != FM_OK)
        {
            goto ABORT;
        }

        /* Add each logical port to the port mask. */
        for (i = 0 ; i < numMembers ; i++)
        {
            fmEnablePortInPortMask(sw, &portMask, memberList[i]);
        }

    }   /* end for (listIndex = 0 ; listIndex < numVlanPorts ; listIndex++) */

    /* Build the output port list from the port mask. */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        if (FM_PORTMASK_IS_BIT_SET(&portMask, cpi))
        {
            if (*numPhysPorts >= maxPhysPorts)
            {
                err = FM_ERR_BUFFER_FULL;
                goto ABORT;
            }
            physPortList[*numPhysPorts] = GET_LOGICAL_PORT(sw, cpi);
            (*numPhysPorts)++;
        }
    }   /* end for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++) */

ABORT:
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VLAN,
                       err,
                       "numPhysPorts=%d\n",
                       *numPhysPorts);

}   /* end fmExtractVlanPhysicalPortList */




/*****************************************************************************/
/** fmExtractVlanLagPortList
 * \ingroup intVlan
 *
 * \desc            Extracts a list of LAG ports from a logical port list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       numVlanPorts is the number of ports in the logical port list.
 * 
 * \param[in]       vlanPortList points to an array containing the list of logical
 *                  ports from which the LAG ports are to be extracted.
 * 
 * \param[out]      numLagPorts points to a caller-supplied variable in which
 *                  this function should store the number of ports returned
 *                  in the LAG port list.
 * 
 * \param[out]      lagPortList points to a caller-supplied array in which
 *                  this function should store the list of LAG ports.
 * 
 * \param[in]       maxLagPorts is the size of the lagPortList array. This
 *                  function will not return more than the maximum number of
 *                  LAG ports.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if one of the port numbers is invalid.
 * \return          FM_ERR_BUFFER_FULL if the LAG port list is too small.
 *
 *****************************************************************************/
fm_status fmExtractVlanLagPortList(fm_int   sw,
                                   fm_int   numVlanPorts,
                                   fm_int * vlanPortList,
                                   fm_int * numLagPorts,
                                   fm_int * lagPortList,
                                   fm_int   maxLagPorts)
{
    fm_port *   portPtr;
    fm_int      listIndex;
    fm_int      port;
    fm_int      i;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d numPorts=%d maxLagPorts=%d\n",
                 sw,
                 numVlanPorts,
                 maxLagPorts);

    *numLagPorts = 0;

    for (listIndex = 0 ; listIndex < numVlanPorts ; listIndex++)
    {
        port = vlanPortList[listIndex];

        portPtr = GET_PORT_PTR(sw, port);
        if (portPtr == NULL)
        {
            err = FM_ERR_INVALID_PORT;
            goto ABORT;
        }

        if (portPtr->portType == FM_PORT_TYPE_LAG)
        {
            for (i = 0 ; i < *numLagPorts ; ++i)
            {
                if (lagPortList[i] == port)
                {
                    break;
                }
            }

            if (i == *numLagPorts)
            {
                if (i >= maxLagPorts)
                {
                    err = FM_ERR_BUFFER_FULL;
                    goto ABORT;
                }
                lagPortList[*numLagPorts] = port;
                (*numLagPorts)++;
            }

        }   /* end if (portPtr->portType == FM_PORT_TYPE_LAG) */

    }   /* end for (listIndex = 0 ; listIndex < numVlanPorts ; listIndex++) */

ABORT:
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VLAN,
                       err,
                       "numLagPorts=%d\n",
                       *numLagPorts);

}   /* end fmExtractVlanLagPortList */




/*****************************************************************************/
/** fmSetVlanCounterID
 * \ingroup intVlan
 *
 * \desc            Sets the counter field in a vlan table entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number for which to set the counter.
 *
 * \param[in]       vcnt is the counter id.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetVlanCounterID(fm_int sw, fm_uint vlanID, fm_uint vcnt)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vlanID=%u vcnt=%u\n",
                 sw,
                 vlanID,
                 vcnt);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_VLAN_ID(sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    FM_TAKE_L2_LOCK(sw);

    FM_API_CALL_FAMILY(err, switchPtr->SetVlanCounterID, sw, vlanID, vcnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

ABORT:
    FM_DROP_L2_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fmSetVlanCounterID */




/*****************************************************************************/
/* fmAddCVlan
 * \ingroup intVlan
 *
 * \desc            Add a customer VLAN and map it to an internal (service
 *                  provider) VLAN. Upon completion port is added to the
 *                  membership of sVlan. If sVlan does not exists before
 *                  the call to this function it is created. It is the
 *                  application's responsibility to add the service provider
 *                  port to the membership of sVlan.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the customer-facing port to operate on.
 *
 * \param[in]       cVlan is the VLAN number to map.
 *
 * \param[in]       sVlan is the normal (service provider) VLAN to map to.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if sVLAN is out of range.
 * \return          FM_ERR_INVALID_ARGUMENT if this mapping already exist
 *
 *****************************************************************************/
fm_status fmAddCVlan(fm_int sw, fm_int port, fm_uint16 cVlan, fm_uint16 sVlan)
{
    fm_status     err = FM_OK;
    fm_switch *   switchPtr;
    fm_vlanEntry *ventry;
    fm_int        mappedSVlan;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d port=%d cVlan=%u sVlan=%u\n",
                     sw, port, cVlan, sVlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    if ( VLAN_OUT_OF_BOUNDS(cVlan) || VLAN_OUT_OF_BOUNDS(sVlan) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    err = fmGetSVlanFromPortCVlan(sw, port, cVlan, &mappedSVlan);

    if (err == FM_OK)
    {
        /* (port, cVlan) has already been mapped.*/
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* Switch specific configurations */
    FM_API_CALL_FAMILY(err, switchPtr->AddCVlan, sw, port, cVlan, sVlan);

    if (err == FM_OK)
    {
        ventry = &switchPtr->vidTable[sVlan];

        /* If the service provider vlan sVlan does not exists, create it */
        if (!ventry->valid)
        {
            err = fmCreateVlan(sw, sVlan);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
        }

        /* Add port to the sVlan */
        err = fmAddVlanPort(sw, sVlan, port, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    }

ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmAddCVlan */




/*****************************************************************************/
/* fmDeleteCVlan
 * \ingroup intVlan
 *
 * \desc            Delete a customer VLAN. Upon completion port is removed
 *                  from the membership of the service provider VLAN. sVlan
 *                  continues to exists, and the corresponding service
 *                  provider port remains the member of the sVlan. If port
 *                  is no longer part of any customer VLAN, its configuration
 *                  is restored to the pre-CVLAN state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the customer-facing port to operate on.
 *
 * \param[in]       cVlan is the VLAN number to operate on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is out of range.
 * \return          FM_ERR_INVALID_VLAN if cVlan is out of range.
 * \return          FM_ERR_INVALID_ARGUMENT if this mapping doesn't exist.
 *
 *****************************************************************************/
fm_status fmDeleteCVlan(fm_int sw, fm_int port, fm_uint16 cVlan)
{
    fm_status     err = FM_OK;
    fm_switch *   switchPtr;
    fm_vlanEntry *ventry;
    fm_int        sVlan;
    fm_int        fPort, fCvlan, fSvlan;
    int           deletePortFromSvlanMembership;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d port=%d cVlan=%u\n",
                     sw, port, cVlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    if ( VLAN_OUT_OF_BOUNDS(cVlan) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* Retrieve the sVlan mapped to from (port, sVlan), since we need
    *  to remove port from the sVlan membership. */
    err = fmGetSVlanFromPortCVlan(sw, port, cVlan, &sVlan);

    if (err != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Switch specific configurations */
    FM_API_CALL_FAMILY(err, switchPtr->DeleteCVlan, sw, port, cVlan);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    ventry = &switchPtr->vidTable[sVlan];

    /* Sanity check. If the service provider vlan sVlan does not exists,
    *  it's an error. */
    if (!ventry->valid)
    {
        FM_LOG_FATAL(FM_LOG_CAT_VLAN, "sVlan %d does not exist!", sVlan);
    }

    /*********************************************************************
     *  Remove the port from the sVlan, only if port hasn't any other
     *  CVLANs associated to the same SVLAN (sVlan)
     *  Note that we do not delete sVlan since it is possible to have
     *  another port_cVlan pair that maps to the same sVlan.
     ********************************************************************/
    deletePortFromSvlanMembership = 1;
    err                           = fmGetCVlanFirst(sw, &fPort, &fCvlan);

    while (err == FM_OK)
    {
        if (fPort == port)
        {
            err = fmGetSVlanFromPortCVlan(sw, fPort, (fm_uint16) fCvlan, &fSvlan);

            if (err == FM_OK && fSvlan == sVlan)
            {
                deletePortFromSvlanMembership = 0;
                break;
            }
        }

        err = fmGetCVlanNext(sw, fPort, (fm_uint16) fCvlan, &fPort, &fCvlan);
    }

    if (deletePortFromSvlanMembership == 1)
    {
        err = fmDeleteVlanPort(sw, sVlan, port);
    }

ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmDeleteCVlan */




/*****************************************************************************/
/* fmGetSVlanFromPortCVlan
 * \ingroup intVlan
 *
 * \desc            Return the sVlan ID given a (port, cVlan) pair.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port to search for.
 *
 * \param[in]       cVlan is the cVlan to search for.
 *
 * \param[out]      sVlan is the user-allocated storage for the returned
 *                  sVlan ID. It is -1 if no mapping is found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if no mapping is found.
 * \return          FM_ERR_INVALID_PORT if port is out of range.
 * \return          FM_ERR_INVALID_VLAN if cVlan is out of range.
 *
 *****************************************************************************/
fm_status fmGetSVlanFromPortCVlan(fm_int    sw,
                                  fm_int    port,
                                  fm_uint16 cVlan,
                                  fm_int *  sVlan)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d port=%d cVlan=%u sVlan=%p\n",
                     sw, port, cVlan, (void *) sVlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    if ( VLAN_OUT_OF_BOUNDS(cVlan) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    if (!sVlan)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetSVlanFromPortCVlan,
                       sw,
                       port,
                       cVlan,
                       sVlan);

ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetSVlanFromPortCVlan */




/*****************************************************************************/
/* fmGetCVlanFirst
 * \ingroup intVlan
 *
 * \desc            Return the first C-VLAN. The customer VLANs are
 *                  returned by order of ports and then VLANs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstPort points to a caller-allocated memory where
 *                  this function will return the first port that as a
 *                  port/cvlan pair. The port will be set to -1 if there are
 *                  no port/cvlan pairs.
 *
 * \param[out]      firstCVlan points to a caller-allocated memory where
 *                  this function will return the first C-VLAN for that port.
 *                  The cVlan will be set to 0 if there are
 *                  no port/C-VLAN pairs.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no more port/C-VLAN pairs.
 *
 *****************************************************************************/
fm_status fmGetCVlanFirst(fm_int sw, fm_int *firstPort, fm_int *firstCVlan)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d firstPort=%p firstCVlan=%p\n",
                     sw, (void *) firstPort, (void *) firstCVlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!firstPort || !firstCVlan)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetCVlanFirst, sw, firstPort, firstCVlan);

ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetCVlanFirst */




/*****************************************************************************/
/* fmGetCVlanNext
 * \ingroup intVlan
 *
 * \desc            Return the next C-VLAN/port pair. The caller supplies
 *                  the last port/cvlan retrieve as argument the the
 *                  function return the next one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startPort is the last port number found by a previous call
 *                  to this function or to fmGetCVlanFirst.
 *
 * \param[in]       startCVlan is the last C-VLAN number found by a previous
 *                  call to this function or to fmGetCVlanFirst.
 *
 * \param[out]      nextPort points to a caller-allocated memory where
 *                  this function will return the port of the next port/C-VLAN
 *                  pair. The port will be set to -1 if there is no
 *                  more port/C-VLAN pairs.
 *
 * \param[out]      nextCVlan points to a caller-allocated memory where
 *                  this function will return the vlan of the next port/C-VLAN
 *                  pair. nextCVlan will be set to 0 if there are no more
 *                  port/C-VLAN pairs.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if startPort is out of range.
 * \return          FM_ERR_INVALID_VLAN if startCVlan is out of range.
 * \return          FM_ERR_NO_MORE if there are no more port/C-VLAN pairs.
 *
 *****************************************************************************/
fm_status fmGetCVlanNext(fm_int    sw,
                         fm_int    startPort,
                         fm_uint16 startCVlan,
                         fm_int *  nextPort,
                         fm_int *  nextCVlan)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN,
                     "sw=%d startPort=%d startCVlan=%u"
                     " nextPort=%p nextCVlan=%p\n",
                     sw,
                     startPort,
                     startCVlan,
                     (void *) nextPort,
                     (void *) nextCVlan);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    VALIDATE_LOGICAL_PORT(sw, startPort, ALLOW_CPU);

    if ( VLAN_OUT_OF_BOUNDS(startCVlan) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    if (!nextPort || !nextCVlan)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetCVlanNext,
                       sw,
                       startPort,
                       startCVlan,
                       nextPort,
                       nextCVlan);

ABORT:

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, err);

}   /* end fmGetCVlanNext */




/*****************************************************************************/
/* fmDbgDumpCVlanCounter
 * \ingroup diagMisc
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Dump cVlan information (verbose version)
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpCVlanCounter(fm_int sw)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VLAN, "sw=%d", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpCVlanCounter, sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fmDbgDumpCVlanCounter */




/*****************************************************************************/
/** fmAddInternalPortsToVlan
 * \ingroup intVlan
 *
 * \desc            Add all internal ports to a vlan.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to create.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAddInternalPortsToVlan(fm_int sw, fm_uint16 vlanID)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     cpi;
    fm_int     port;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d vlanID=%u\n", sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    status    = FM_OK;

    /* Add all internal ports to this vlan */
    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        if ( fmIsInternalPort(sw, port) )
        {
            status = fmAddVlanPortInternal(sw, vlanID, port, FALSE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fmAddInternalPortsToVlan */

