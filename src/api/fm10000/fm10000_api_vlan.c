/* vim:et:sw=4:ts=4:tw=79:
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_vlan.c
 * Creation Date:   December 3, 2012
 * Description:     FM10xxx specific VLAN functions
 *
 * Copyright (c) 2012 - 2015, Intel Corporation
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

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** SetPerPortProperties
 * \ingroup intVlan
 *
 * \desc            Sets the VLAN membership and tagging state for a list of
 *                  physical ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN to be updated.
 *
 * \param[in]       numPorts is the number of entries in the port list.
 *
 * \param[in]       portList points to an array containing a list of physical
 *                  logical ports.
 *
 * \param[in]       state is TRUE if the port is to become a member of the
 *                  vlan, FALSE if it is to be removed from the vlan.
 *
 * \param[in]       tag is TRUE if the port is to transmit tagged frames
 *                  for this vlan, FALSE if it is to transmit untagged frames
 *                  for this vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetPerPortProperties(fm_int    sw,
                                      fm_uint16 vlanID,
                                      fm_int    numPorts,
                                      fm_int *  portList,
                                      fm_bool   state,
                                      fm_bool   tag)
{
    fm_switch *        switchPtr;
    fm10000_vlanEntry *ventryExt;
    fm_int             index;
    fm_int             physPort;
    fm_status          status;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, numPorts=%d, portList=%p, state=%d, tag=%d\n",
                 sw,
                 vlanID,
                 numPorts,
                 (void *) portList,
                 state,
                 tag);

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, vlanID);
    status    = FM_OK;

    FM_TAKE_L2_LOCK(sw);

    for (index = 0 ; index < numPorts ; index++)
    {
        status = fmMapLogicalPortToPhysical(switchPtr, portList[index], &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

        /* Set vlan membership. */
        FM_PORTMASK_SET_BIT(&ventryExt->member, physPort, state);

        /* Set vlan tag. */
        FM_PORTMASK_SET_BIT(&ventryExt->tag, physPort, tag);
    }

    status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);


ABORT:

    FM_DROP_L2_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end SetPerPortProperties */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000AllocateVlanTableDataStructures
 * \ingroup intVlan
 *
 * \desc            Allocates the state structure for VLAN management.
 *                  Called through the AllocateVlanTableDataStructures
 *                  function pointer.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AllocateVlanTableDataStructures(fm_switch *switchPtr)
{
    fm_int             entry;
    fm_int             size;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY( FM_LOG_CAT_VLAN,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  switchPtr ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    for (entry = 0; entry < switchPtr->vlanTableSize; entry++)
    {
        size = sizeof(fm10000_vlanEntry);
        ventryExt = fmAlloc(size);
        if (ventryExt == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_NO_MEM);
        }

        FM_CLEAR(*ventryExt);

        switchPtr->vidTable[entry].vlanExt = ventryExt;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000AllocateVlanTableDataStructures */




/*****************************************************************************/
/** fm10000FreeVlanTableDataStructures
 * \ingroup intVlan
 *
 * \desc            Free the state structure for VLAN management. Called
 *                  through the FreeVlanTableDataStructures function pointer.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeVlanTableDataStructures(fm_switch *switchPtr)
{
    fm_int entry;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "switchPtr=%p, sw=%d\n",
                 (void *) switchPtr,
                 switchPtr ? switchPtr->switchNumber : -1);

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    for (entry = 0; entry < switchPtr->vlanTableSize; entry++)
    {
        if (switchPtr->vidTable[entry].vlanExt != NULL)
        {
            fmFree(switchPtr->vidTable[entry].vlanExt);
            switchPtr->vidTable[entry].vlanExt = NULL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000FreeVlanTableDataStructures */




/*****************************************************************************/
/** fm10000InitVlanTable
 * \ingroup intVlan
 *
 * \desc            Initializes each entry of the VLAN table cache to match
 *                  chip defaults. Called through the InitVlanTable function
 *                  pointer.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitVlanTable(fm_switch *switchPtr)
{
    fm_int             entry;
    fm10000_vlanEntry *ventryExt;
    fm_uint32          ingressVidEntry[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_uint32          egressVidEntry[FM10000_EGRESS_VID_TABLE_WIDTH];

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "swstate=%p, sw=%d\n",
                 (void *) switchPtr,
                 switchPtr ? switchPtr->switchNumber : -1);

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * Make sure VLAN 0 is defined and is full.
     **************************************************/
    FM_CLEAR(ingressVidEntry);
    FM_ARRAY_SET_FIELD64( ingressVidEntry,
                          FM10000_INGRESS_VID_TABLE,
                          membership,
                          0xFFFFFFFFFFFF );

    fmRegCacheWriteSingle1D( switchPtr->switchNumber,
                             &fm10000CacheIngressVidTable,
                             ingressVidEntry,
                             0,
                             FALSE );

    FM_CLEAR(egressVidEntry);
    FM_ARRAY_SET_FIELD64( egressVidEntry,
                          FM10000_EGRESS_VID_TABLE,
                          membership,
                          0xFFFFFFFFFFFF );

    fmRegCacheWriteSingle1D( switchPtr->switchNumber,
                             &fm10000CacheEgressVidTable,
                             egressVidEntry,
                             0,
                             FALSE );

    /***************************************************
     * Initalize all vlans
     **************************************************/

    for (entry = 0 ; entry < switchPtr->vlanTableSize ; entry++)
    {
        ventryExt = switchPtr->vidTable[entry].vlanExt;

        if (ventryExt != NULL)
        {
            /* Clear everything on power-up. This includes defaulting all
               triggers, indices, and other values to zero. */
            FM_CLEAR(*ventryExt);

            /* If any entries should NOT be zero, initialize them here */
            ventryExt->egressVid = entry;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000InitVlanTable */




/*****************************************************************************/
/** fm10000CreateVlan
 * \ingroup intVlan
 *
 * \desc            Create a VLAN.
 *                  This function only performs switch-specific portions of
 *                  the vlan creation process.  Generic portions are done
 *                  in the API fmCreateVlan function.
 *                  Called through the CreateVlan function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is the reserved VLAN.
 * \return          FM_ERR_INVALID_ARGUMENT if vlanID already exists.
 *
 *****************************************************************************/
fm_status fm10000CreateVlan(fm_int sw, fm_uint16 vlanID)
{
    fm_status          status;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d, vlanID=%u\n", sw, vlanID);

    ventryExt = GET_VLAN_EXT(sw, vlanID);

    /***************************************************************
     * Initialize VLAN table entry fields.
     **************************************************************/
    FM_PORTMASK_DISABLE_ALL(&ventryExt->member);
    FM_PORTMASK_DISABLE_ALL(&ventryExt->tag);
    ventryExt->trigger   = 0;
    ventryExt->mtuIndex  = 0;
    ventryExt->statIndex = 0;

    status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    status = fmAddInternalPortsToVlan(sw, vlanID);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000CreateVlan */




/*****************************************************************************/
/** fm10000DeleteVlan
 * \ingroup intVlan
 *
 * \desc            Delete a VLAN.
 *                  This function only performs switch-specific portions of
 *                  the vlan deletion process.  Generic portions are done
 *                  in the API fmDeleteVlan function.
 *                  Called through the DeleteVlan function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteVlan(fm_int sw, fm_uint16 vlanID)
{
    fm_status          status;
    fm_vlanEntry *     ventry;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d, vlanID=%u\n", sw, vlanID);

    /* Do this here as the VLAN is not valid if this is done at the end.
       This will free any counters allocated to this vlan. */
    status = fmFreeVLANCounters(sw, vlanID);

    /* do not return an error if there were no counters assigned to this VLAN */
    if (status == FM_ERR_NO_VLANCOUNTER)
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    if ( GET_PROPERTY()->maFlushOnVlanChange )
    {
        /* Age MA Table entries for this VLAN. */
        status = fmFlushVlanAddresses(sw, (fm_uint) vlanID);

        /* Ignore the error if the MAC table feature is unsupported. */
        if (status == FM_ERR_UNSUPPORTED)
        {
            status = FM_OK;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }

    /* Remove any multicast group listener/addresses that use this vlanID */
    status = fmMcastDeleteVlanNotify(sw, vlanID);

    /* Ignore the error if multicast features are unsupported. */
    if (status == FM_ERR_UNSUPPORTED)
    {
        status = FM_OK;
    }
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    /* Restore VLAN table entry to hardware default values. */
    ventry->valid        = FALSE;
    ventryExt->trigger   = 0;
    ventryExt->mtuIndex  = 0;
    ventryExt->statIndex = 0;
    FM_PORTMASK_DISABLE_ALL(&ventryExt->member);
    FM_PORTMASK_DISABLE_ALL(&ventryExt->tag);

    status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000DeleteVlan */




/*****************************************************************************/
/** fm10000WriteVlanEntryV2
 * \ingroup intVlan
 *
 * \desc            Writes the hardware registers for this vlan entry.
 *
 * \param[in]       sw is the switch to access.
 *
 * \param[in]       vlanID is the vlan number
 *
 * \param[in]       stpInstance is the spanning-tree instance. If -1, the
 *                  instance will be retrieved from the software tables.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteVlanEntryV2(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    stpInstance)
{
    fm_status          status;
    fm_vlanEntry *     ventry;
    fm10000_vlanEntry *ventryExt;
    fm_switch *        switchPtr;
    fm_uint32          regVals[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm_int             fid;
    fm_bool            regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, stpInstance=%d\n",
                 sw,
                 vlanID,
                 stpInstance);

    switchPtr = GET_SWITCH_PTR(sw);
    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    regLockTaken = FALSE;

    if (stpInstance < 0)
    {
        status = fmFindInstanceForVlan(sw, (fm_int) vlanID, &stpInstance);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }

    switch (switchPtr->vlanLearningMode)
    {
        case FM_VLAN_LEARNING_MODE_INDEPENDENT:
            fid = vlanID;
            break;

        case FM_VLAN_LEARNING_MODE_SHARED:
            fid = switchPtr->sharedLearningVlan;
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_VLAN,
                         "Invalid vlan learning mode %d configured\n",
                         switchPtr->vlanLearningMode);
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }

    /**************************************************
     * Update the VLAN tables in hardware.
     **************************************************/

    FM_CLEAR(regVals);
    FM_ARRAY_SET_PORTMASK( regVals, FM10000_INGRESS_VID_TABLE, membership, &ventryExt->member);
    FM_ARRAY_SET_FIELD(    regVals, FM10000_INGRESS_VID_TABLE, FID,        fid );
    FM_ARRAY_SET_FIELD(    regVals, FM10000_INGRESS_VID_TABLE, MST_Index,  stpInstance );
    FM_ARRAY_SET_FIELD(    regVals, FM10000_INGRESS_VID_TABLE, vcnt,       ventryExt->statIndex );
    FM_ARRAY_SET_BIT(      regVals, FM10000_INGRESS_VID_TABLE, reflect,    ventry->reflect );
    FM_ARRAY_SET_BIT(      regVals, FM10000_INGRESS_VID_TABLE, TrapIGMP,   ventry->trapIGMP );

    TAKE_REG_LOCK(sw);

    regLockTaken = TRUE;

    /* write to this one register (single indexed) */
    status = fmRegCacheWriteSingle1D(sw,
                                     &fm10000CacheIngressVidTable,
                                     regVals,
                                     vlanID,
                                     FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* set the EGRESS_VID_TABLE[].membership[] using
     * EGRESS_FID_TABLE[].Forwarding[] & real VLAN membership to make sure that
     * any packet that are being untrapped by the trigger do not bypass the
     * egress STP state checks.
     */
    FM_CLEAR(regVals);
    FM_ARRAY_SET_PORTMASK( regVals, FM10000_EGRESS_VID_TABLE, membership, &ventryExt->member);
    FM_ARRAY_SET_FIELD(    regVals, FM10000_EGRESS_VID_TABLE, FID,        fid );
    FM_ARRAY_SET_FIELD(    regVals, FM10000_EGRESS_VID_TABLE, MST_Index,  stpInstance );
    FM_ARRAY_SET_FIELD(    regVals, FM10000_EGRESS_VID_TABLE, MTU_Index,  ventryExt->mtuIndex );
    FM_ARRAY_SET_FIELD(    regVals, FM10000_EGRESS_VID_TABLE, TrigID,     ventryExt->trigger );

    /* write to this one register (single indexed) */
    status = fmRegCacheWriteSingle1D(sw,
                                     &fm10000CacheEgressVidTable,
                                     regVals,
                                     vlanID,
                                     FALSE );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* Write VLANTAG_TABLE. */
    status = fm10000WriteTagEntry(sw, vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);


ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000WriteVlanEntryV2 */




/*****************************************************************************/
/** fm10000WriteVlanEntry
 * \ingroup intVlan
 *
 * \desc            Writes the hardware registers for this vlan entry.
 *                  Called through the WriteVlanEntry function pointer.
 *
 * \param[in]       sw is the switch to access.
 *
 * \param[in]       vlanID is the vlan number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteVlanEntry(fm_int    sw, fm_uint16 vlanID)
{
    fm_status          status;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d, vlanID=%u\n", sw, vlanID);

    status = fm10000WriteVlanEntryV2(sw, vlanID, -1);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000WriteVlanEntry */




/*****************************************************************************/
/** fm10000WriteTagEntry
 * \ingroup intVlan
 *
 * \desc            Writes tagging configuration to the hardware.
 *                  Called through the WriteTagEntry function pointer.
 *
 * \param[in]       sw is the switch to access.
 *
 * \param[in]       vlanID is the vlan number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteTagEntry(fm_int sw, fm_uint16 vlanID)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm10000_vlanEntry *ventryExt;
    fm_uint32          regVals[FM10000_MOD_VLAN_TAG_VID1_MAP_WIDTH];

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d, vlanID=%u\n", sw, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    TAKE_REG_LOCK(sw);

    /* write MOD_VLAN_TAG_VID1_MAP registers */
    FM_CLEAR(regVals);
    FM_ARRAY_SET_PORTMASK( regVals, FM10000_MOD_VLAN_TAG_VID1_MAP, Tag, &ventryExt->tag);
    FM_ARRAY_SET_FIELD(    regVals, FM10000_MOD_VLAN_TAG_VID1_MAP, VID, ventryExt->egressVid );

    /* write to this one register (single indexed) */
    status = fmRegCacheWriteSingle1D(sw,
                                     &fm10000CacheModVlanTagVid1Map,
                                     regVals,
                                     vlanID,
                                     FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

ABORT:

    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000WriteTagEntry */




/*****************************************************************************/
/** fm10000ConfigureVlanLearningMode
 * \ingroup intSwitch
 *
 * \desc            Configures the VLAN learning mode by setting the
 *                  appropriate hardware registers.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       mode is the vlan learning mode.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConfigureVlanLearningMode(fm_int sw, fm_vlanLearningMode mode)
{
    fm_status status;
    fm_uint16 vlan;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN, "sw=%d, mode=%d\n", sw, mode);

    status = FM_OK;

    switch (mode)
    {
        case FM_VLAN_LEARNING_MODE_INDEPENDENT:
            break;

        case FM_VLAN_LEARNING_MODE_SHARED:
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_VLAN,
                         "Invalid vlan learning mode %d configured\n",
                         mode);
            FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_UNSUPPORTED);
    }

#if (!defined(FV_CODE) && !defined(FAST_API_BOOT))
    for (vlan = 1 ; vlan < FM_MAX_VLAN ; vlan++)
    {
        status = fm10000WriteVlanEntryV2(sw, vlan, -1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }
#endif

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000ConfigureVlanLearningMode */




/*****************************************************************************/
/** fm10000SetVlanMembership
 * \ingroup intVlan
 *
 * \desc            Function to set port vlan membership.
 * 
 * \note            Assumes that the caller has taken the L2_LOCK
 *                  beforehand.
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
fm_status fm10000SetVlanMembership(fm_int        sw,
                                   fm_vlanEntry *entry,
                                   fm_int        port,
                                   fm_bool       state)
{
    fm_status          status;
    fm_int             physPort;
    fm10000_vlanEntry *ventryExt;
    fm_switch        * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, entry=%p, port=%d, state=%d\n",
                 sw,
                 (void *) entry,
                 port,
                 state);

    if ( (entry == NULL) || (entry->vlanExt == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Should add debug build assert verifying the lock is captured. */

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, entry->vlanId);

    status = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    FM_PORTMASK_SET_BIT(&ventryExt->member, physPort, state);

    FM_LOG_DEBUG( FM_LOG_CAT_VLAN,
                  "fm10000SetVlanMembership: port=%d, val=%d, "
                  "set membership=0x%x\n",
                  port,
                  state,
                  FM_PORTMASK_GET_BIT(&ventryExt->member, physPort) );

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000SetVlanMembership */




/*****************************************************************************/
/** fm10000GetVlanMembership
 * \ingroup intVlan
 *
 * \desc            Function to retrieve vlan membership state.
 * 
 * \note            Assumes that the caller has taken the L2_LOCK
 *                  beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry is the vlan entry on which to operate
 *
 * \param[in]       port is the logical port number
 *
 * \param[out]      state contains TRUE if the port is a vlan member,
 *                  FALSE if it is not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetVlanMembership(fm_int        sw,
                                   fm_vlanEntry *entry,
                                   fm_int        port,
                                   fm_bool *     state)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_int             physPort;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, entry=%p, port=%d, state=%p\n",
                 sw,
                 (void *) entry,
                 port,
                 (void *) state);

    if ( (entry == NULL) || (entry->vlanExt == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, entry->vlanId);

    status = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    *state = FM_PORTMASK_GET_BIT(&ventryExt->member, physPort);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000GetVlanMembership */




/*****************************************************************************/
/** fm10000SetVlanTag
 * \ingroup intVlan
 *
 * \desc            Function to set port vlan tagging mode.
 * 
 * \note            This function assumes that the caller has taken the
 *                  L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanSel defines which VLAN to select. Only VLAN1 is
 *                  supported.
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
fm_status fm10000SetVlanTag(fm_int        sw,
                            fm_vlanSelect vlanSel,
                            fm_vlanEntry *entry,
                            fm_int        port,
                            fm_bool       tag)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm10000_vlanEntry *ventryExt;
    fm_int             physPort;
    fm_islTagFormat    islTagFormat;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanSel=%d, entry=%p, port=%d, tag=%d\n",
                 sw,
                 vlanSel,
                 (void *) entry,
                 port,
                 tag);

    if (tag)
    {
        status = fm10000GetPortAttribute(sw,
                                         port,
                                         FM_PORT_ACTIVE_MAC,
                                         FM_PORT_LANE_NA,
                                         FM_PORT_ISL_TAG_FORMAT,
                                         &islTagFormat);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

        if (islTagFormat &&
            !GET_PROPERTY()->allowFtagVlanTagging)
        {
            FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_UNSUPPORTED);
        }
    }

    if ( (entry == NULL) || (entry->vlanExt == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    if (vlanSel != FM_VLAN_SELECT_VLAN1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_UNSUPPORTED);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, entry->vlanId);

    status = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    FM_PORTMASK_SET_BIT(&ventryExt->tag, physPort, tag);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000SetVlanTag */




/*****************************************************************************/
/** fm10000GetVlanTag
 * \ingroup intVlan
 *
 * \desc            Function to retrieve port vlan tagging mode.
 * 
 * \note            This function assumes that the caller has taken the
 *                  L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanSel defines which VLAN to select. Only VLAN1 is
 *                  supported.
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
fm_status fm10000GetVlanTag(fm_int        sw,
                            fm_vlanSelect vlanSel,
                            fm_vlanEntry *entry,
                            fm_int        port,
                            fm_bool *     tag)
{
    fm_status          status;
    fm_switch  *       switchPtr;
    fm_int             physPort;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanSel=%d, entry=%p, port=%d, tag=%p\n",
                 sw,
                 vlanSel,
                 (void *) entry,
                 port,
                 (void *) tag);

    if ( (entry == NULL) || (entry->vlanExt == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
    }

    if (vlanSel != FM_VLAN_SELECT_VLAN1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_UNSUPPORTED);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    ventryExt = GET_VLAN_EXT(sw, entry->vlanId);

    status = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    *tag = FM_PORTMASK_GET_BIT(&ventryExt->tag, physPort);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_OK);

}   /* end fm10000GetVlanTag */




/*****************************************************************************/
/** fm10000SetVlanTrigger
 * \ingroup intVlan
 *
 * \desc            Sets the trigger ID for the vlan.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       vlanID is the vlan Id to operate on.
 *
 * \param[in]       triggerId is the trigger Id to set for the vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetVlanTrigger(fm_int sw, fm_uint16 vlanID, fm_int triggerId)
{
    fm_uint32           regVals[FM10000_INGRESS_VID_TABLE_WIDTH];
    fm10000_vlanEntry  *ventryExt;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d vlanID=%u triggerId=%d\n",
                 sw,
                 vlanID,
                 triggerId);

    ventryExt = GET_VLAN_EXT(sw, vlanID);

    TAKE_REG_LOCK(sw);

    ventryExt->trigger = triggerId;

    /* write to this one register (single indexed) */
    err = fmRegCacheReadSingle1D(sw,
                                 &fm10000CacheEgressVidTable,
                                 regVals,
                                 vlanID,
                                 FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

    FM_ARRAY_SET_FIELD(regVals,
                       FM10000_EGRESS_VID_TABLE,
                       TrigID,
                       triggerId);

    /* write to this one register (single indexed) */
    err = fmRegCacheWriteSingle1D(sw,
                                  &fm10000CacheEgressVidTable,
                                  regVals,
                                  vlanID,
                                  FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);

ABORT:
    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_VLAN, err);

}   /* end fm10000SetVlanTrigger */




/*****************************************************************************/
/** fm10000AddVlanPortList
 * \ingroup intVlan
 *
 * \desc            Adds a list of ports to a VLAN.
 *                  Called through the AddVlanPortList function pointer.
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
 *
 *****************************************************************************/
fm_status fm10000AddVlanPortList(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    numPorts,
                                fm_int *  portList,
                                fm_bool   tag)
{
    fm_switch * switchPtr;
    fm_int      physPortList[FM10000_NUM_PORTS];
    fm_int      lagPortList[FM_MAX_NUM_LAGS];
    fm_int      numPhysPorts;
    fm_int      numLagPorts;
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, numPorts=%d, portList=%p, tag=%d\n",
                 sw,
                 vlanID,
                 numPorts,
                 (void *) portList,
                 tag);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Extract the list of physical ports. */
    status = fmExtractVlanPhysicalPortList(sw,
                                           numPorts,
                                           portList,
                                           &numPhysPorts,
                                           physPortList,
                                           FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* Extract the list of LAG ports. */
    status = fmExtractVlanLagPortList(sw,
                                      numPorts,
                                      portList,
                                      &numLagPorts,
                                      lagPortList,
                                      FM_MAX_NUM_LAGS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* If per-LAG management is enabled, set the VLAN membership
     * and tagging properties for each of the LAG ports. */
    if (switchPtr->perLagMgmt && numLagPorts != 0)
    {
        status = fmSetLagListVlanMembership(sw,
                                            vlanID,
                                            numLagPorts,
                                            lagPortList,
                                            TRUE,
                                            tag);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }

    status = SetPerPortProperties(sw,
                                  vlanID,
                                  numPhysPorts,
                                  physPortList,
                                  TRUE,
                                  tag);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000AddVlanPortList */




/*****************************************************************************/
/** fm10000DeleteVlanPortList
 * \ingroup intVlan
 *
 * \desc            Deletes a list of ports from a VLAN.
 *                  Called through the DeleteVlanPortList function pointer.
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
 *
 *****************************************************************************/
fm_status fm10000DeleteVlanPortList(fm_int    sw,
                                    fm_uint16 vlanID,
                                    fm_int    numPorts,
                                    fm_int *  portList)
{
    fm_switch * switchPtr;
    fm_int      physPortList[FM10000_NUM_PORTS];
    fm_int      lagPortList[FM_MAX_NUM_LAGS];
    fm_int      numPhysPorts;
    fm_int      numLagPorts;
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, numPorts=%d, portList=%p\n",
                 sw,
                 vlanID,
                 numPorts,
                 (void *) portList);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Extract the list of physical ports. */
    status = fmExtractVlanPhysicalPortList(sw,
                                           numPorts,
                                           portList,
                                           &numPhysPorts,
                                           physPortList,
                                           FM10000_NUM_PORTS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* Extract the list of LAG ports. */
    status = fmExtractVlanLagPortList(sw,
                                      numPorts,
                                      portList,
                                      &numLagPorts,
                                      lagPortList,
                                      FM_MAX_NUM_LAGS);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);

    /* If per-LAG management is enabled, set the VLAN membership
     * and tagging properties for each of the LAG ports. */
    if (switchPtr->perLagMgmt && numLagPorts != 0)
    {
        status = fmSetLagListVlanMembership(sw,
                                            vlanID,
                                            numLagPorts,
                                            lagPortList,
                                            FALSE,
                                            FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);
    }

    status = SetPerPortProperties(sw,
                                  vlanID,
                                  numPhysPorts,
                                  physPortList,
                                  FALSE,
                                  FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, status);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000DeleteVlanPortList */




/*****************************************************************************/
/** fm10000GetVlanAttribute
 * \ingroup intVlan
 *
 * \desc            Get a VLAN attribute.
 *                  Called through the GetVlanAttribute function pointer.
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
fm_status fm10000GetVlanAttribute(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    attr,
                                  void *    value)
{
    fm_vlanEntry *     ventry;
    fm10000_vlanEntry *ventryExt;
    fm_status          status;
    fm10000_switch *   switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, attr=%d, value=%p\n",
                 sw,
                 vlanID,
                 attr,
                 (void *) value);

    switchExt = GET_SWITCH_EXT(sw);
    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);
    status    = FM_OK;

    switch (attr)
    {
        case FM_VLAN_REFLECT:
            *( (fm_bool *) value ) = ventry->reflect;
            break;

        case FM_VLAN_MTU:
            *( (fm_int *) value ) = ventryExt->mtuIndex;
            break;

        case FM_VLAN_ROUTABLE:
            *( (fm_bool *) value) = ventry->routable;
            break;

        case FM_VLAN_IGMP_TRAPPING:
            *( (fm_bool *) value) = ventry->trapIGMP;
            break;

        case FM_VLAN_FID2_IVL:
            status = FM_ERR_UNSUPPORTED;
            break;

        case FM_VLAN_ARP_TRAPPING:
            status = FM_ERR_UNSUPPORTED;
            break;

        case FM_VLAN_UCAST_FLOODING:
            status = FM_ERR_UNSUPPORTED;
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000GetVlanAttribute */




/*****************************************************************************/
/** fm10000SetVlanAttribute
 * \ingroup intVlan
 *
 * \desc            Set a VLAN attribute.
 *                  Called through the SetVlanAttribute function pointer.
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
fm_status fm10000SetVlanAttribute(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    attr,
                                  void *    value)
{
    fm_vlanEntry *      ventry;
    fm10000_vlanEntry * ventryExt;
    fm_status           status;
    fm_bool             routable;
    fm10000_switch *    switchExt;
    fm_fm10000MapVIDCfg mapVidCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, attr=%d, value=%p\n",
                 sw,
                 vlanID,
                 attr,
                 (void *) value);

    switchExt = GET_SWITCH_EXT(sw);
    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    switch (attr)
    {
        case FM_VLAN_REFLECT:
            ventry->reflect = *( (fm_bool *) value );

            status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
            break;

        case FM_VLAN_MTU:
            {
                fm_int mtuIndex = *( (fm_int *) value);

                if ( (mtuIndex < 0) || ( mtuIndex >= (fm_int) FM10000_MTU_TABLE_ENTRIES ) )
                {
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, FM_ERR_INVALID_ARGUMENT);
                }

                ventryExt->mtuIndex = mtuIndex;

                status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
            }
            break;

        case FM_VLAN_ROUTABLE:
            routable = *( (fm_bool *) value);

            FM_CLEAR(mapVidCfg);

            status = fm10000GetMapVID(sw,
                                     vlanID,
                                     &mapVidCfg,
                                     TRUE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

            if (routable)
            {
                mapVidCfg.mapVid   = vlanID;
                mapVidCfg.routable = 1;
            }
            else
            {
                mapVidCfg.mapVid   = 0;
                mapVidCfg.routable = 0;
            }

            status = fm10000SetMapVID(sw,
                                     vlanID,
                                     &mapVidCfg,
                                     FM_FM10000_MAP_VID_ALL,
                                     TRUE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

            ventry->routable = routable;
            break;

        case FM_VLAN_IGMP_TRAPPING:
            ventry->trapIGMP = *( (fm_bool *) value );

            status = fm10000WriteVlanEntryV2(sw, vlanID, -1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
            break;

        case FM_VLAN_FID2_IVL:
            status = FM_ERR_UNSUPPORTED;
            break;

        case FM_VLAN_UCAST_FLOODING:
            status = FM_ERR_UNSUPPORTED;
            break;

        case FM_VLAN_ARP_TRAPPING:
            status = FM_ERR_UNSUPPORTED;
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000SetVlanAttribute */




/*****************************************************************************/
/** fm10000GetVlanPortAttribute
 * \ingroup intVlan
 *
 * \desc            Gets a VLAN port attribute.
 *                  Called through the GetVlanPortAttribute function pointer.
 *
 * \note            This function assumes that vlan and port numbers have been
 *                  validated.
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
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fm10000GetVlanPortAttribute(fm_int    sw,
                                      fm_uint16 vlanID,
                                      fm_int    port,
                                      fm_int    attr,
                                      void *    value)
{
    fm_switch *        switchPtr;
    fm_vlanEntry *     ventry;
    fm10000_vlanEntry *ventryExt;
    fm_int             physPort;
    fm_status          status;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, port=%d, attr=%d, value=%p\n",
                 sw,
                 vlanID,
                  port,
                  attr,
                  (void *) value);

    switchPtr = GET_SWITCH_PTR(sw);
    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    status = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

    switch (attr)
    {
        case FM_VLAN_PORT_MEMBERSHIP:
            *( (fm_bool *) value ) = FM_PORTMASK_GET_BIT(&ventryExt->member, physPort);
            break;

        case FM_VLAN_PORT_TAG1:
            *( (fm_bool *) value ) = FM_PORTMASK_GET_BIT(&ventryExt->tag, physPort);
            break;

        case FM_VLAN_PORT_TAG2:
            status = FM_ERR_UNSUPPORTED;
            break;

        case FM_VLAN_PORT_STP_STATE:
            status = fmGetVlanPortStateInternal(sw, vlanID, port, (fm_int *) value);
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000GetVlanPortAttribute */




/*****************************************************************************/
/** fm10000SetVlanCounterID
 * \ingroup intVlan
 *
 * \desc            Sets the counter ID for this VLAN.
 *                  Called through the SetVlanCounterID function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       vlanID is the VLAN to set the counter on.
 *
 * \param[in]       vcnt is the counter ID set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetVlanCounterID(fm_int sw, fm_uint vlanID, fm_uint vcnt)
{
    fm_status          status;
    fm_vlanEntry *     ventry;
    fm10000_vlanEntry *ventryExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, vlanID=%u, vcnt=%d\n",
                 sw,
                 vlanID,
                 vcnt);

    ventry    = GET_VLAN_PTR(sw, vlanID);
    ventryExt = GET_VLAN_EXT(sw, vlanID);

    ventryExt->statIndex = vcnt;

    status = fm10000WriteVlanEntryV2(sw, vlanID, -1);

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000SetVlanCounterID */




/*****************************************************************************/
/** fm10000SetVlanPortState
 * \ingroup intVlan
 *
 * \desc            Wrapper to call fmSetSpanningTreePortState.
 *                  Called through the SetVlanPortState function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       vlanID is the VLAN number (used in MULTIPLE mode).
 *
 * \param[in]       port is the port to operate on.
 *
 * \param[in]       state is the state to apply.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetVlanPortState(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    port,
                                  fm_int    state)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     instance;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, port=%d, state=%d\n",
                 sw,
                 port,
                 state);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->stpMode == FM_SPANNING_TREE_PER_VLAN)
    {
        status = FM_ERR_INVALID_STP_MODE;
    }
    else if (switchPtr->stpMode == FM_SPANNING_TREE_SHARED)
    {
        /* Shared mode only works on instance 0 */
        status = fmSetSpanningTreePortState(sw, 0, port, state);
    }
    else if (switchPtr->stpMode == FM_SPANNING_TREE_MULTIPLE)
    {
        status = fmFindInstanceForVlan(sw, vlanID, &instance);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);

        status = fmSetSpanningTreePortState(sw, instance, port, state);
    }
    else
    {
        FM_LOG_FATAL(FM_LOG_CAT_VLAN,
                     "Invalid internal spanning tree mode %d\n",
                     switchPtr->stpMode);
        status = FM_ERR_INVALID_STP_MODE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000SetVlanPortState */




/*****************************************************************************/
/** fm10000GetVlanPortState
 * \ingroup intVlan
 *
 * \desc            Wrapper to call fmGetSpanningTreePortState.
 *                  Called through the GetVlanPortState function pointer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       vlanID is the VLAN number (used in MULTIPLE mode).
 *
 * \param[in]       port is the port to operate on.
 *
 * \param[out]      state points to caller allocated storage where the
 *                  spanning tree state is written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetVlanPortState(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    port,
                                  fm_int *  state)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     instance;

    FM_LOG_ENTRY(FM_LOG_CAT_VLAN,
                 "sw=%d, port=%d, state=%p\n",
                 sw,
                 port,
                 (void *) state);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (switchPtr->stpMode)
    {
        case FM_SPANNING_TREE_PER_VLAN:
            status = FM_ERR_INVALID_STP_MODE;
            break;

        case FM_SPANNING_TREE_SHARED:
            /* Shared mode only works on instance 0. */
            status = fmGetSpanningTreePortState(sw, 0, port, state);
            break;

        case FM_SPANNING_TREE_MULTIPLE:
            status = fmFindInstanceForVlan(sw, vlanID, &instance);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VLAN, status);
            status = fmGetSpanningTreePortState(sw, instance, port, state);
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_VLAN,
                         "Invalid internal spanning tree mode %d\n",
                         switchPtr->stpMode);
            status = FM_ERR_INVALID_STP_MODE;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VLAN, status);

}   /* end fm10000GetVlanPortState */
