/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stp.c
 * Creation Date:   January 16, 2008
 * Description:     Spanning tree instance API
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

/*****************************************************************************/
/** StpInstancePreamble
 * \ingroup intStp
 *
 * \desc            A helper functioned performing the initial validation of
 *                  arguments and capture of the state lock.
 *
 * \param[in]       sw is the switch being operated on.
 *
 * \param[in]       stpInstance is the stpInstance number or -1 if unused.
 *
 * \param[in]       vlanID is the vlan number or -1 if unused.
 *
 * \param[in]       multipleModeOnly is TRUE if the switch must be in
 *                  multiple spanning tree mode in order for the function
 *                  to continue. FALSE means it is potentially valid to continue
 *                  in other spanning tree modes, provided that the other validity
 *                  checks pass.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
static fm_status StpInstancePreamble(fm_int  sw, 
                                     fm_int  stpInstance, 
                                     fm_int  vlanID,
                                     fm_bool multipleModeOnly)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_timestamp timeout;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "sw=%d stpInstance=%d vlanID=%d\n",
                 sw, 
                 stpInstance, 
                 vlanID);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr    = GET_SWITCH_PTR(sw);
    timeout.sec  = 30;
    timeout.usec = 0;

    /***************************************************
     * First check the mode to see if it is valid.
     **************************************************/
    if (multipleModeOnly)
    {
        if (switchPtr->stpMode != FM_SPANNING_TREE_MULTIPLE)
        {
            FM_LOG_EXIT(FM_LOG_CAT_STP, FM_ERR_INVALID_STP_MODE);
        }
    }

    if ( (stpInstance != FM_DEFAULT_STP_INSTANCE)
         && (switchPtr->stpMode != FM_SPANNING_TREE_MULTIPLE) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STP, FM_ERR_INVALID_STP_MODE);
    }

    /***************************************************
     * Now validate the spanning-tree instance ID
     **************************************************/
    if (stpInstance != -1)
    {
        VALIDATE_STP_INSTANCE(sw, stpInstance);
    }

    /***************************************************
     * Next validate the VLAN ID.
     **************************************************/
    if (vlanID != -1)
    {
        if ( VLAN_OUT_OF_BOUNDS(vlanID) )
        {
            FM_LOG_EXIT(FM_LOG_CAT_STP, FM_ERR_INVALID_VLAN);
        }
    }

    /***************************************************
     * All input parameters look good, take the L2 lock.
     **************************************************/
    err = FM_TAKE_L2_LOCK(sw);

    /* This should never happen given our timeout */
    if (err == FM_ERR_LOCK_TIMEOUT)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STP,
                     "State lock capture timed out in %lld seconds, "
                     "potential deadlock detected.\n", timeout.sec);
    }

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end StpInstancePreamble */




/*****************************************************************************/
/** StpInstancePostamble
 * \ingroup intStp
 *
 * \desc            A helper functioned performing the cleanup phase, namely
 *                  releasing the lock if indicated, and returning the error.
 *
 * \param[in]       sw is the switch being operated on.
 *
 * \param[in]       preambleError is the result of the err from the call to
 *                  StpInstancePreamble.
 *
 * \param[in]       err points to the current error condition.  This overrides
 *                  any errors that may occur within this function.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
static fm_status StpInstancePostamble(fm_int    sw,
                                      fm_status preambleError,
                                      fm_status err)
{
    fm_status  err2 = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "sw=%d preambleError=%d (%s) err=%d (%s)\n",
                 sw, 
                 preambleError, 
                 fmErrorMsg(preambleError),
                 err, 
                 fmErrorMsg(err));

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
    if (preambleError == FM_OK)
    {
        err2 = FM_DROP_L2_LOCK(sw);
    }

    /***************************************************
     * This list of errors comes from the possible
     * error states in StpInstancePostamble where
     * the switch lock is not taken.
     **************************************************/
    if ( (preambleError == FM_OK) ||
         ( (preambleError != FM_ERR_INVALID_SWITCH) &&
           (preambleError != FM_ERR_SWITCH_NOT_UP) ) )
    {
        UNPROTECT_SWITCH(sw);
    }

    FM_LOG_EXIT( FM_LOG_CAT_STP, ( (err != FM_OK) ? err : err2 ) );

}   /* end StpInstancePostamble */




/*****************************************************************************/
/** StpInstanceFree
 * \ingroup intStp
 *
 * \desc            A helper function used in conjunction with fmTreeRemove
 *                  to deallocate the instance state.
 *
 * \param[in]       ptr points to an object of type fm_stpInstanceInfo who is
 *                  being deallocated.
 *
 *****************************************************************************/
static void StpInstanceFree(void *ptr)
{
    fm_status err;
    fm_stpInstanceInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "ptr=%p\n", ptr);

    info = (fm_stpInstanceInfo *) ptr;

    err = fmDeleteBitArray(&info->vlans);

    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STP,
                     "Unable to delete VLAN membership bit array\n");
    }

    fmFree(info->states);
    fmFree(info);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_STP);

}   /* end StpInstanceFree */




/*****************************************************************************/
/** StpInstanceAllocate
 * \ingroup intStp
 *
 * \desc            A helper functioned used to create an initialized instance
 *                  object.
 *
 * \param[in]       sw is the switch to allocate the instance for.
 *
 * \param[in]       stpInstance is the instance number to create.
 *
 * \return          A pointer to the allocated instance.
 *
 *****************************************************************************/
static fm_stpInstanceInfo *StpInstanceAllocate(fm_int sw, fm_int stpInstance)
{
    fm_switch *switchPtr;
    fm_int              cpi;
    fm_int              port;
    fm_int              defaultStpState;
    fm_stpInstanceInfo *newInstance;
    fm_status           err = FM_OK;
    fm_bool             cleanupPortStates = FALSE;
    fm_bool             cleanupInstance = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "sw=%d, stpInstance=%d\n", sw, stpInstance);

    switchPtr = GET_SWITCH_PTR(sw);

    newInstance = (fm_stpInstanceInfo *) fmAlloc( sizeof(fm_stpInstanceInfo) );

    if (newInstance == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, FM_ERR_NO_MEM);
    }
    else
    {
        cleanupInstance = TRUE;
    }

    newInstance->instance = stpInstance;

    /***************************************************
     * We need enough states for all physical ports and
     * the CPU port.
     *************************************************/

    newInstance->states = 
        (fm_int *) fmAlloc( sizeof(fm_int) * (switchPtr->numCardinalPorts) );

    if (newInstance->states == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, FM_ERR_NO_MEM);
    }
    else
    {
        cleanupPortStates = TRUE;
    }

    /* CPU port is always forwarding */
    cpi = GET_PORT_INDEX(sw, switchPtr->cpuPort);
    newInstance->states[cpi] = FM_STP_STATE_FORWARDING;

    /* Get default state for instance ports */
    defaultStpState = GET_PROPERTY()->defStateVlanNonMember;

    for ( cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        port = GET_LOGICAL_PORT(sw, cpi);
        if ( port == switchPtr->cpuPort )
        {
            /* Nothing to do for the CPU port here */
            continue;
        }
        else if ( fmIsInternalPort(sw, port) )
        {
            newInstance->states[cpi] = FM_STP_STATE_FORWARDING;
        }
        else
        {
            /* Set default state for all ports. */
            newInstance->states[cpi] = defaultStpState;
        }
    }

    /* Initialize the bit array of VLAN membership */
    err = fmCreateBitArray(&newInstance->vlans, FM_MAX_VLAN);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /* Normal exit path */
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_STP, 
                       newInstance, 
                       "instance=%p\n",
                       (void *) newInstance);

ABORT:
    /***************************************************
     * Perform cleanup actions if needed and return NULL
     * to indicate an error.
     **************************************************/

    if (cleanupPortStates)
    {
        fmFree(newInstance->states);
    }

    if (cleanupInstance)
    {
        fmFree(newInstance);
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_STP, NULL, "NULL instance\n");

}   /* end StpInstanceAllocate */




/*****************************************************************************/
/** fmFindInstanceForVlan
 * \ingroup intStp
 *
 * \desc            Searches all instances until it finds the one that contains
 *                  the given VLAN.  Assumes that a VLAN can only be a member
 *                  of a single instance.  Assumes argument checking has already
 *                  occurred.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       vlanID is the VLAN to search for.
 *
 * \param[out]      instance points to caller allocated storage where the
 *                  found instance number is written to.  -1 when not found.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFindInstanceForVlan(fm_int sw, fm_int vlanID, fm_int *instance)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instanceInfo;
    fm_treeIterator     treeIter;
    fm_bool             isMember;
    fm_uint64           key;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "sw=%d vlanID=%d instance=%p\n",
                 sw, 
                 vlanID, 
                 (void *) instance);

    stpInfo   = GET_STP_INFO(sw);

    /* vlan vlanID is not member of any STP instance */
    *instance = -1;

    fmTreeIterInit(&treeIter, stpInfo);

    while (TRUE)
    {
        err = fmTreeIterNext(&treeIter, &key, (void **) &instanceInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

        err = fmGetBitArrayBit(&instanceInfo->vlans, vlanID, &isMember);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

        if (isMember)
        {
            *instance = (fm_int) key;

            FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);
        }
    }


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmFindInstanceForVlan */




/*****************************************************************************/
/** AddSpanningTreeVlanInternal
 * \ingroup intStp
 *
 * \desc            Used to add VLANs to spanning tree instances.  This may
 *                  only be used when the switch attribute FM_SPANNING_TREE_MODE
 *                  is set to FM_SPANNING_TREE_MULTIPLE.  If otherwise, an
 *                  error will be returned.  This is the internal version
 *                  which does not error check modifications to the default
 *                  instance (0).
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       stpInstance is the instance number to add to, from 0 to
 *                  a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').  If the instance argument
 *                  is NULL, a lookup is performed to find the instance structure.
 *
 * \param[in]       instance points to the instance structure representing
 *                  the instance.  If stpInstance is NULL, then this value
 *                  is used to skip the lookup.
 *
 * \param[in]       vlanID is the vlan to add, from 0 to (FM_MAX_VLAN - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_STP_MODE if the mode is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the instance is invalid.
 * \return          FM_ERR_INVALID_VLAN if the VLAN is invalid.
 *
 *****************************************************************************/
fm_status AddSpanningTreeVlanInternal(fm_int sw,
                                      fm_int stpInstance,
                                      fm_stpInstanceInfo *instance,
                                      fm_int vlanID)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_bool             isMember;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d stpInstance=%d vlanID=%d\n",
                 sw, stpInstance, vlanID);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Find the instance if needed.
     **************************************************/

    if ( !instance )
    {
        err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
        if (err == FM_ERR_NOT_FOUND)
        {
            /* Return Invalid Argument instead of Key not Found */
            err = FM_ERR_INVALID_ARGUMENT;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    err = fmGetBitArrayBit(&instance->vlans, vlanID, &isMember);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /***************************************************
     * If the VLAN is already a member, do nothing.
     **************************************************/

    if (isMember)
    {
        err = FM_OK;
        goto ABORT;
    }

    err = fmSetBitArrayBit(&instance->vlans, vlanID, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /* Refresh the state */
    err = fmRefreshStpStateInternal(switchPtr, instance, vlanID, -1);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end AddSpanningTreeVlanInternal */




/*****************************************************************************/
/** DeleteSpanningTreeVlanInternal
 * \ingroup intStp
 *
 * \desc            Used to delete VLANs from spanning tree instances.  This may
 *                  only be used when the switch attribute FM_SPANNING_TREE_MODE
 *                  is set to FM_SPANNING_TREE_MULTIPLE.  If otherwise, an
 *                  error will be returned.  This is the internal version which
 *                  does not error check modifications to the default instance.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       stpInstance is the instance number to delete from, from 0 to
 *                  a chip-specific maximum value (see ''FM10000_MAX_STP_INSTANCE'').
 *
 * \param[in]       vlanID is the vlan to delete, from 0 to (FM_MAX_VLAN - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_STP_MODE if the mode is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the instance is invalid.
 * \return          FM_ERR_INVALID_VLAN if the VLAN is invalid.
 *
 *****************************************************************************/
static fm_status DeleteSpanningTreeVlanInternal(fm_int sw,
                                                fm_int stpInstance,
                                                fm_int vlanID)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d stpInstance=%d vlanID=%d\n",
                 sw, stpInstance, vlanID);

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Find the instance and delete the VLAN.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmSetBitArrayBit(&instance->vlans, vlanID, FALSE);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end DeleteSpanningTreeVlanInternal */




/*****************************************************************************/
/** InitDefaultInstanceVlans
 * \ingroup intStp
 *
 * \desc            This function is used to initialize the default spanning
 *                  tree instance's VLANs.
 *
 * \param[in,out]   vlans points to the bit array of VLANs to be initialized.
 *
 * \param[in]       reservedVlan is the number of the reserved VLAN, if any.
 *                  Use ~0 if no reserved VLAN is needed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitDefaultInstanceVlans(fm_bitArray *vlans,
                                          fm_uint      reservedVlan)
{
    fm_status  err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "vlans=%p\n", 
                 (void *) vlans);

    /**************************************************
     * Include all VLANs in the default instance except
     * the reserved VLAN.
     **************************************************/
     
    if (reservedVlan == 1)
    {
        err = fmSetBitArrayBlock(vlans, 
                                reservedVlan + 1, 
                                FM_MAX_VLAN - 1, 
                                TRUE);
    }
    else if (reservedVlan == FM_MAX_VLAN - 1)
    {
        err = fmSetBitArrayBlock(vlans, 1, reservedVlan - 1, TRUE);
    }
    else if (reservedVlan > 1 && reservedVlan < FM_MAX_VLAN - 1)
    {
        err = fmSetBitArrayBlock(vlans, 1, reservedVlan - 1, TRUE);
        
        if (err == FM_OK)
        {
            err = fmSetBitArrayBlock(vlans, 
                                     reservedVlan + 1, 
                                     FM_MAX_VLAN - 1, 
                                     TRUE);
        }
    }
    else
    {
        err = fmSetBitArrayBlock(vlans, 1, FM_MAX_VLAN - 1, TRUE);
    }
    
    
    FM_LOG_EXIT(FM_LOG_CAT_STP, err);
    
}   /* end InitDefaultInstanceVlans */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmRefreshStpStateInternal
 * \ingroup intStp
 *
 * \desc            A helper function to refresh the hardware STP state of
 *                  a subset of a given instance.  This is called by 
 *                  fmRefreshStpState once the switch and instance pointers
 *                  are looked up.  This is an optimization for all the
 *                  local calls to fmRefreshStpState where said information
 *                  has already been looked up.
 *
 * \param[out]      switchPtr is a pointer to the switch being operated on. 
 *
 * \param[out]      instance is a pointer to the STP instance info
 *                  structure for the instance.
 *
 * \param[in]       vlanID is the VLAN to refresh, or -1 for all. 
 *
 * \param[in]       port is the port to refresh, or -1 for all. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRefreshStpStateInternal(fm_switch *switchPtr, 
                                    fm_stpInstanceInfo *instance, 
                                    fm_int vlanID, 
                                    fm_int port)
{
    fm_int      sw = switchPtr->switchNumber;
    fm_status   err = FM_OK;
    fm_int      currentVlan;
    fm_int      nextVlan;
    fm_int      firstIndex;
    fm_int      lastIndex;
    fm_int      cpi;
    fm_int      state;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "switchPtr=%p, stpInstance=%p, vlanID=%d, port=%d\n",
                 (void *) switchPtr, (void *) instance, vlanID, port);

    if (port == -1)
    {
        firstIndex = 1;
        lastIndex  = switchPtr->numCardinalPorts - 1;
    }
    else
    {
        firstIndex = GET_PORT_INDEX(sw, port);
        lastIndex  = firstIndex;
    }

    if ( switchPtr->RefreshSpanningTree && !switchPtr->useEgressVIDasFID )
    {
        FM_API_CALL_FAMILY(err, 
                           switchPtr->RefreshSpanningTree,
                           switchPtr->switchNumber,
                           instance,
                           vlanID,
                           port);
    }
    else
    {
        /* Otherwise standard iteration (slow) */

        if (vlanID == -1)
        {
            err = fmFindBitInBitArray(&instance->vlans, 0, TRUE, &currentVlan);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
        }
        else
        {
            currentVlan = vlanID;
        }

        while (currentVlan != -1)
        {
            /* Note that the reserved VLANs are left alone */
            if ( (currentVlan != switchPtr->reservedVlan) &&
                 ( (FM_FM4000_RESERVED_VLAN == 0) ||
                   (currentVlan != FM_FM4000_RESERVED_VLAN) ) )
            {
                for (cpi = firstIndex ; cpi <= lastIndex ; cpi++)
                {
                    port = GET_LOGICAL_PORT(sw, cpi);

                    /* Force forwarding state for internal ports */
                    if ( fmIsInternalPort(sw, port) )
                    {
                        state = FM_STP_STATE_FORWARDING;
                    }
                    else
                    {
                        state = instance->states[cpi];
                    }

                    err = fmSetVlanPortStateInternal(sw,
                                                     currentVlan,
                                                     port,
                                                     state);

                    if (err == FM_ERR_PORT_IS_INTERNAL)
                    {
                        /* Port is already in forwarding */
                        err = FM_OK;
                    }
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
                }
                
            }   /* end (currentVlan != FM_FM4000_RESERVED_VLAN) ) )... */

            if (vlanID == -1)
            {
                err = fmFindBitInBitArray(&instance->vlans,
                                          currentVlan + 1,
                                          TRUE,
                                          &nextVlan);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

                currentVlan = nextVlan;
            }
            else
            {
                currentVlan = -1;
                
            }   /* end if (vlanID == -1) */
            
        }   /* end while (currentVlan != -1) */

    }   /* end if ( switchPtr->RefreshSpanningTree &&... */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmRefreshStpStateInternal */




/*****************************************************************************/
/** fmRefreshStpState
 * \ingroup intStp
 *
 * \desc            A helper function to refresh the hardware STP state of
 *                  a subset of a given instance.
 *
 * \param[in]       sw is the switch being operated on.
 *
 * \param[in]       stpInstance is the instance number to update.
 *
 * \param[in]       vlanID is the vlan to update or -1 to update all VLANs in
 *                  that instance.
 *
 * \param[in]       port is the port to update or -1 to update all ports.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmRefreshStpState(fm_int sw,
                            fm_int stpInstance,
                            fm_int vlanID,
                            fm_int port)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY(FM_LOG_CAT_STP,
                 "sw=%d stpInstance=%d vlanID=%d port=%d\n",
                 sw, stpInstance, vlanID, port);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmRefreshStpStateInternal(switchPtr, instance, vlanID, port);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmRefreshStpState */




/*****************************************************************************/
/** fmAllocateStpInstanceTreeDataStructures
 * \ingroup intStp
 *
 * \desc            Called by switch initialization code to allocate state for
 *                  the STP instance module.
 *
 * \param[in]       switchPtr points to the switch state structure who is being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocateStpInstanceTreeDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "switchPtr=%p\n", (void *) switchPtr);
    
    FM_NOT_USED(switchPtr);

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmAllocateStpInstanceTreeDataStructures */




/*****************************************************************************/
/** fmFreeStpInstanceTreeDataStructures
 * \ingroup intStp
 *
 * \desc            Called by switch initialization code to free state for
 *                  the STP instance module.
 *
 * \param[in]       switchPtr points to the switch state structure who is being
 *                  removed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeStpInstanceTreeDataStructures(fm_switch *switchPtr)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "switchPtr=%p\n", (void *) switchPtr);

    FM_NOT_USED(switchPtr);

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmFreeStpInstanceTreeDataStructures */




/*****************************************************************************/
/** fmInitStpInstanceTree
 * \ingroup intStp
 *
 * \desc            Called by switch initialization code to initialize the
 *                  state for spanning tree instance management.  This consists
 *                  mainly of initializing the tree of instances.
 *
 * \param[in]       switchPtr points to the switch state structure who is being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitStpInstanceTree(fm_switch *switchPtr)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;
    fm_bool             cleanupInstance = FALSE;
    fm_uint             reservedVlan;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "switchPtr=%p\n", (void *) switchPtr);

    stpInfo = GET_STP_INFO(switchPtr->switchNumber);

    fmTreeInit(stpInfo);

    /***************************************************
     * Create the default instance and add all VLANs to
     * it.
     **************************************************/

    instance = StpInstanceAllocate(switchPtr->switchNumber, 
                                   FM_DEFAULT_STP_INSTANCE);

    if (!instance)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }
    else
    {
        cleanupInstance = TRUE;
    }

    switch (switchPtr->switchFamily)
    {
        case FM_SWITCH_FAMILY_FM4000:
        case FM_SWITCH_FAMILY_REMOTE_FM4000:
            reservedVlan = FM_FM4000_RESERVED_VLAN;
            break;

        default:
            reservedVlan = ~0;
            break;
    }

    /* All VLANs are a member of the default instance */
    err = InitDefaultInstanceVlans(&instance->vlans, reservedVlan);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmTreeInsert(stpInfo, 0, instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /* Store this to avoid repeated lookups */
    switchPtr->defaultSTPInstance = instance;

    /* Normal exit path */
    FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);

ABORT:
    if (cleanupInstance)
    {
        StpInstanceFree(instance);
    }

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmInitStpInstanceTree */




/*****************************************************************************/
/** fmDestroyStpInstanceTree
 * \ingroup intStp
 *
 * \desc            Free the memory used by STP.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmDestroyStpInstanceTree(fm_switch *switchPtr)
{
    fm_tree *stpTree = GET_STP_INFO(switchPtr->switchNumber);

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "switchPtr=%p\n", (void *) switchPtr);

    if ( !fmTreeIsInitialized(stpTree) )
    {
        /* Initialization has not been called due to
         * switch bring up error, so when cleanup
         * just return here.
         */
        FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);
    }

    fmTreeDestroy(stpTree, StpInstanceFree);

    /* For extra safety... */
    switchPtr->defaultSTPInstance = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_STP, FM_OK);

}   /* end fmDestroyStpInstanceTree */




/*****************************************************************************/
/** fmResetMultipleSpanningTreeState
 * \ingroup intStp
 *
 * \desc            Called by the attribute set code when changing STP modes.
 *                  This method resets the current multiple spanning tree
 *                  configuration to the state where all VLANs are a member of
 *                  the lone instance 0, with all ports in disabled state.
 *
 * \param[in]       sw points to the switch number whose STP state is being
 *                  reset.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmResetMultipleSpanningTreeState(fm_int sw)
{
    fm_status     err = FM_OK;
    fm_switch *   switchPtr;
    fm_tree *     stpInfo;
    fm_int        cpi;
    fm_int        port;
    fm_int        vlan;
    fm_int        reservedVlan;
    fm_int        defaultStpState;
    fm_switchInfo swInfo;
    

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "sw=%d\n", sw);

    err = StpInstancePreamble(sw, -1, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    err = fmGetSwitchInfoInternal(sw, &swInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /***************************************************
     * Delete all instances and reconstruct tree.
     **************************************************/

    fmTreeDestroy(stpInfo, StpInstanceFree);

    err = fmInitStpInstanceTree(switchPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /* Get default state for a port that is not a member of a VLAN. */
    defaultStpState = GET_PROPERTY()->defStateVlanNonMember;

    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);
        if (fmIsInternalPort(sw, port))
        {
            switchPtr->defaultSTPInstance->states[cpi] = FM_STP_STATE_FORWARDING;
        }
        else
        {
            switchPtr->defaultSTPInstance->states[cpi] = defaultStpState;
        }
    }

    if ( (swInfo.switchFamily == FM_SWITCH_FAMILY_FM4000) ||
         (swInfo.switchFamily == FM_SWITCH_FAMILY_REMOTE_FM4000) )
    {
        reservedVlan = FM_FM4000_RESERVED_VLAN;
    }
    else
    {
        reservedVlan = 0;
    }
    
    if (switchPtr->ResetVlanSpanningTreeState)
    {
        for (vlan = 1 ; vlan < FM_MAX_VLAN ; vlan++)
        {
            if (vlan != reservedVlan)
            {
                FM_API_CALL_FAMILY(err, 
                                   switchPtr->ResetVlanSpanningTreeState, 
                                   sw, 
                                   vlan);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
        
            }   /* end if (vlan != reservedVlan) */
            
        }   /* end for (vlan = 1 ; vlan < FM_MAX_VLAN ; vlan++) */
        
    }   /* end if (switchPtr->ResetVlanSpanningTreeState) */

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmResetMultipleSpanningTreeState */




/*****************************************************************************/
/** fmCreateSpanningTree
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a spanning tree instance.
 *
 * \note            This function can only be used when the 
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to 
 *                  ''FM_SPANNING_TREE_MULTIPLE'', otherwise, an error will be
 *                  returned.
 *
 * \note            Spanning tree instance 0 always exists as the default
 *                  spanning tree and does not need to be created.
 *
 * \param[in]       sw is the switch to create the instance on.
 *
 * \param[in]       stpInstance is the instance number to create and may range
 *                  from 1 to a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 *
 *****************************************************************************/
fm_status fmCreateSpanningTree(fm_int sw, fm_int stpInstance)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *newInstance;
    fm_bool             cleanupInstance = FALSE;
    fm_bool             cleanupFromTree = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP, "sw=%d stpInstance=%d\n", sw, stpInstance);

    err = StpInstancePreamble(sw, stpInstance, -1, TRUE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Search to see if it is not there already.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &newInstance);

    if (err == FM_OK)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Allocate and add the instance.
     **************************************************/

    if (switchPtr->CreateSpanningTree != NULL)
    {
        err = switchPtr->CreateSpanningTree(sw, stpInstance);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    newInstance = StpInstanceAllocate(sw, stpInstance);

    if (!newInstance)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, FM_ERR_NO_MEM);
    }
    else
    {
        cleanupInstance = TRUE;
    }

    err = fmTreeInsert(stpInfo, stpInstance, newInstance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    cleanupFromTree = TRUE;

    /* Ensure that the hardware state is refreshed */
    err = fmRefreshStpStateInternal(switchPtr, newInstance, -1, -1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

ABORT:
    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/

    if ( err != FM_OK )
    {
        /***************************************************
         * Since fmTreeRemove will cleanup the instance via
         * StpInstanceFree, we only obey cleanupInstance
         * if we are not removing from the tree.
         **************************************************/

        if ( cleanupFromTree )
        {
            err = fmTreeRemove(stpInfo, stpInstance, StpInstanceFree);
        }
        else if ( cleanupInstance )
        {
            StpInstanceFree(newInstance);
        }
    }

    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmCreateSpanningTree */




/*****************************************************************************/
/** fmDeleteSpanningTreeInternal
 * \ingroup intStp
 *
 * \desc            Implements the core of the spanning tree delete code,
 *                  given the state lock has already been take and the instance
 *                  found.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      instance is pointer to the instance info structure.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDeleteSpanningTreeInternal(fm_int sw, fm_stpInstanceInfo *instance)
{
    fm_switch *         switchPtr;
    fm_status           status = FM_OK;
    fm_int              currentVlan;
    fm_int              nextVlan;
    fm_uint64           stpInstance = instance->instance;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *defaultInstance;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, 
                 "sw=%d, instance=%p\n", 
                 sw, 
                 (void *) instance);

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    defaultInstance = switchPtr->defaultSTPInstance;

    /***************************************************
     * If we are in SWAG, delete the instance for each 
     * of the switches.
     **************************************************/

    if (switchPtr->DeleteSpanningTree != NULL)
    {
        status = switchPtr->DeleteSpanningTree(sw, stpInstance);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);
    }

    /***************************************************
     * Reset the state for all member VLANs.
     **************************************************/

    status = fmFindBitInBitArray(&instance->vlans, 0, TRUE, &currentVlan);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

    while (currentVlan != -1)
    {
        if (switchPtr->switchFamily != FM_SWITCH_FAMILY_SWAG)
        {
            /***************************************************
             * The software state is cleared by the fact that
             * the STP instance structure is freed at the end
             * of this function.  The hardware state for each
             * VLAN should be cleared though.
             **************************************************/
            FM_API_CALL_FAMILY(status,
                               switchPtr->DeleteSpanningTreeVlan,
                               sw,
                               stpInstance,
                               currentVlan);
            if (status == FM_ERR_UNSUPPORTED)
            {
                /* Don't return an error for CHIP specific code that doesn't
                 * support the DeleteSpanningTreeVlan function */
                status = FM_OK;
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);
        }

        /***************************************************
         * All VLANs by default are added back to the 
         * default instance.
         **************************************************/
        status = AddSpanningTreeVlanInternal(sw, 
                                             FM_DEFAULT_STP_INSTANCE,
                                             defaultInstance,
                                             currentVlan);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        status = fmFindBitInBitArray(&instance->vlans, 
                                     currentVlan + 1,
                                     TRUE,
                                     &nextVlan);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, status);

        currentVlan = nextVlan;
    }

    /***************************************************
     * Free the instance structure.
     **************************************************/

    status = fmTreeRemoveCertain(stpInfo, 
                                 stpInstance, 
                                 StpInstanceFree);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_STP, status);

}   /* end fmDeleteSpanningTreeInternal */




/*****************************************************************************/
/** fmDeleteSpanningTree
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a spanning tree instance.
 *
 * \note            This function can only be used when the 
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to 
 *                  ''FM_SPANNING_TREE_MULTIPLE'', otherwise, an error will be
 *                  returned.
 *
 * \note            Spanning tree instance 0 always exists as the default
 *                  spanning tree and cannot be deleted.
 *
 * \param[in]       sw is the switch to delete the instance on.
 *
 * \param[in]       stpInstance is the instance number to delete and may range
 *                  from 1 to a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if the stpInstance is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteSpanningTree(fm_int sw, fm_int stpInstance)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP, "sw=%d stpInstance=%d\n", sw, stpInstance);

    err = StpInstancePreamble(sw, stpInstance, -1, TRUE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);
        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Additional validation: one cannot delete the
     * default instance, it always exists.
     **************************************************/

    if (stpInstance == FM_DEFAULT_STP_INSTANCE)
    {
        err = StpInstancePostamble(sw, err, FM_ERR_INVALID_ARGUMENT);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Find and free the instance.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
    if (err == FM_ERR_NOT_FOUND)
    {
        /* Return Invalid Argument instead of Key not Found */
        err = FM_ERR_INVALID_ARGUMENT;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /* Now call the core delete code */
    err = fmDeleteSpanningTreeInternal(sw, instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmDeleteSpanningTree */




/*****************************************************************************/
/** fmAddSpanningTreeVlan
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a VLAN to a spanning tree instance.
 *
 * \note            This function implicitly deletes the VLAN from the previous
 *                  spanning tree instance, since a VLAN can only be associated
 *                  with one spanning instance at a time.
 *
 * \note            This function can only be used when the 
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to 
 *                  ''FM_SPANNING_TREE_MULTIPLE'', otherwise, an error will be
 *                  returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the instance number to add the VLAN to and
 *                  may range from 0 to a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').
 *
 * \param[in]       vlanID is the VLAN to add and may range from 1 to 
 *                  (''FM_MAX_VLAN'' - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is not set
 *                  to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is invalid.
 *
 *****************************************************************************/
fm_status fmAddSpanningTreeVlan(fm_int sw,
                                fm_int stpInstance,
                                fm_int vlanID)
{
    fm_status  err = FM_OK;
    fm_int     currentInstance;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d vlanID=%d\n",
                     sw,
                     stpInstance,
                     vlanID);

    err = StpInstancePreamble(sw, stpInstance, vlanID, TRUE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * Find the old instance for the VLAN.
     **************************************************/

    err = fmFindInstanceForVlan(sw, vlanID, &currentInstance);

    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STP, 
                     "VLAN %d was not found in any instance!\n",
                     vlanID);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, FM_ERR_INVALID_VLAN);
    }

    /***************************************************
     * If the VLAN already belongs to this instance,
     * then do nothing.
     **************************************************/

    if (currentInstance == stpInstance)
    {
        err = FM_OK;
        goto ABORT;
    }

    /***************************************************
     * Find the new instance and add the VLAN.
     **************************************************/

    err = AddSpanningTreeVlanInternal(sw, stpInstance, NULL, vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /***************************************************
     * Remove it from the old instance
     **************************************************/

    err = DeleteSpanningTreeVlanInternal(sw, currentInstance, vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    if (switchPtr->AddSpanningTreeVlan != NULL)
    {
        err = switchPtr->AddSpanningTreeVlan(sw, stpInstance, vlanID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmAddSpanningTreeVlan */




/*****************************************************************************/
/** fmDeleteSpanningTreeVlan
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a VLAN from a spanning tree instance. The VLAN will
 *                  automatically be added back to the default spanning tree
 *                  instance (instance 0).
 *
 * \note            This function can only be used when the 
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to 
 *                  ''FM_SPANNING_TREE_MULTIPLE'', otherwise, an error will be
 *                  returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the spanning tree instance from which to
 *                  delete the VLAN and may range from 1 to a chip-specific
 *                  maximum value (see ''FM10000_MAX_STP_INSTANCE'').
 *
 * \param[in]       vlanID is the VLAN to delete and may range from 1 to
 *                  (''FM_MAX_VLAN'' - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 * \return          FM_ERR_INVALID_VLAN if vlanID is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteSpanningTreeVlan(fm_int sw,
                                   fm_int stpInstance,
                                   fm_int vlanID)
{
    fm_status           err = FM_OK;
    fm_int              currentInstance;
    fm_switch *         switchPtr;
    fm_stpInstanceInfo *defaultInstance;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d vlanID=%d\n",
                     sw,
                     stpInstance,
                     vlanID);

    err = StpInstancePreamble(sw, stpInstance, vlanID, TRUE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    defaultInstance = switchPtr->defaultSTPInstance;

    /***************************************************
     * Find the old instance for the VLAN.
     **************************************************/

    err = fmFindInstanceForVlan(sw, vlanID, &currentInstance);

    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STP, "VLAN %d was not found in any instance!\n",
                     vlanID);

        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    if (currentInstance != stpInstance)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Find the instance and delete the VLAN.
     **************************************************/

    err = DeleteSpanningTreeVlanInternal(sw, stpInstance, vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    /***************************************************
     * Add it back to the default instance.
     **************************************************/

    err = AddSpanningTreeVlanInternal(sw, 
                                      FM_DEFAULT_STP_INSTANCE, 
                                      defaultInstance, 
                                      vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    if (switchPtr->DeleteSpanningTreeVlan != NULL)
    {
        err = switchPtr->DeleteSpanningTreeVlan(sw, stpInstance, vlanID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmDeleteSpanningTreeVlan */




/*****************************************************************************/
/** fmFindSpanningTreeByVlan
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the spanning tree instance to which a VLAN belongs.
 *
 * \note            This function can only be used when the 
 *                  ''FM_SPANNING_TREE_MODE'' switch attribute is set to 
 *                  ''FM_SPANNING_TREE_MULTIPLE'', otherwise, an error will be
 *                  returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN whose spanning tree instance is to
 *                  be returned.
 *
 * \param[out]      instance points to caller-allocated storage where
 *                  this function should place the VLAN's spanning tree 
 *                  instance number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 * \return          FM_ERR_INVALID_VLAN if the VLAN is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmFindSpanningTreeByVlan (fm_int  sw,
                                    fm_int  vlanID,
                                    fm_int *instance)
{
    fm_status  err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d vlanID=%d\n",
                     sw,
                     vlanID);

    err = StpInstancePreamble(sw, -1, vlanID, TRUE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Find the instance for the VLAN.
     **************************************************/

    err = fmFindInstanceForVlan (sw, vlanID, instance);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_STP, "VLAN %d was not found in any instance!\n",
                     vlanID);

        goto ABORT;
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmFindSpanningTreeByVlan */ 




/*****************************************************************************/
/** fmSetSpanningTreePortState
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set the spanning tree state of a port in a spanning tree
 *                  instance.
 *
 * \note            If the ''FM_SPANNING_TREE_MODE'' switch attribute is not
 *                  set to ''FM_SPANNING_TREE_MULTIPLE'', this function may
 *                  only be called with stpInstance set to 0. Otherwise,
 *                  error ''FM_ERR_INVALID_STP_MODE'' will be returned.
 *
 * \note            Use ''fmSetVlanPortState'' to set the spanning tree
 *                  state for LAG logical ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the spanning tree instance number. In
 *                  multiple spanning tree mode, this instance number may
 *                  range from 0 to a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').
 *                  For all other spanning tree modes, 0 is the only valid
 *                  instance number.
 *
 * \param[in]       port is the logical port number to set the state for.
 *                  The logical port number must represent a physical port
 *                  and not a virtual port like a LAG or a multicast group.
 *
 * \param[in]       stpState is the desired spanning tree state. See
 *                  ''Spanning Tree States'' for possible values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmSetSpanningTreePortState(fm_int sw,
                                     fm_int stpInstance,
                                     fm_int port,
                                     fm_int stpState)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;
    fm_int              currentStpState;
    fm_int              cpi;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d port=%d stpState=%d\n",
                     sw,
                     stpInstance,
                     port,
                     stpState);

    err = StpInstancePreamble(sw, stpInstance, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    switch (stpState)
    {
        case FM_STP_STATE_DISABLED:
        case FM_STP_STATE_LISTENING:
        case FM_STP_STATE_LEARNING:
        case FM_STP_STATE_FORWARDING:
        case FM_STP_STATE_BLOCKING:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    if ( !fmIsCardinalPort(sw, port) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Find the instance.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);

    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmGetSpanningTreePortState(sw, stpInstance, port, &currentStpState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    if (fmIsInternalPort(sw, port))
    {
        /* Cannot change the STP state of an internal port, the state
         * is forced to FM_STP_STATE_FORWARDING */

        if (!GET_PROPERTY()->stpEnIntPortCtrl)
        {

            if (currentStpState != FM_STP_STATE_FORWARDING)
            {
                /* This case will occur when creating vlan */
            }
            else
            {
                err = FM_ERR_PORT_IS_INTERNAL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
            }
        }
    }

    if (currentStpState != stpState)
    {
        /***************************************************
         * Replicate the state across all the member VLANs.
         **************************************************/

        if (switchPtr->SetSpanningTreePortState != NULL)
        {
            err = switchPtr->SetSpanningTreePortState(sw,
                                                      stpInstance, 
                                                      port, 
                                                      stpState);
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

        if (switchPtr->switchFamily != FM_SWITCH_FAMILY_SWAG)
        {
            cpi = GET_PORT_INDEX(sw, port);    
            instance->states[cpi] = stpState;
            err = fmRefreshStpStateInternal(switchPtr, instance, -1, port);
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmSetSpanningTreePortState */




/*****************************************************************************/
/** fmGetSpanningTreePortState
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the spanning tree state of a port in a spanning tree
 *                  instance.
 *
 * \note            If the ''FM_SPANNING_TREE_MODE'' switch attribute is not
 *                  set to ''FM_SPANNING_TREE_MULTIPLE'', this function may
 *                  only be called with stpInstance set to 0. Otherwise,
 *                  error ''FM_ERR_INVALID_STP_MODE'' will be returned.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the spanning tree instance number. In
 *                  multiple spanning tree mode, this instance number may
 *                  range from 1 to a chip-specific maximum value (see
 *                  ''FM10000_MAX_STP_INSTANCE'').
 *                  For all other spanning tree modes, 0 is the only valid
 *                  instance number.
 *
 * \param[in]       port is the logical port number to get the state for.  The
 *                  logical port number must represent a physical port and
 *                  not a virtual port like a LAG or a multicast group.
 *
 * \param[out]      stpState points to caller allocated storage where the port's
 *                  spanning tree state is written to.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_STP_MODE if ''FM_SPANNING_TREE_MODE'' is 
 *                  not set to ''FM_SPANNING_TREE_MULTIPLE''.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_NOT_FOUND if stpInstance does not exist.
 *
 *****************************************************************************/
fm_status fmGetSpanningTreePortState(fm_int  sw,
                                     fm_int  stpInstance,
                                     fm_int  port,
                                     fm_int *stpState)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;
    fm_int              cpi;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d port=%d stpState=%p\n",
                     sw,
                     stpInstance,
                     port,
                     (void *) stpState);

    err = StpInstancePreamble(sw, stpInstance, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    stpInfo   = GET_STP_INFO(sw);

    /* Should move this into the preamble */
    if ( !fmIsCardinalPort(sw, port) )
    {
        err = StpInstancePostamble(sw, err, FM_ERR_INVALID_PORT);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Find the instance.
     **************************************************/

    if (switchPtr->GetSpanningTreePortState != NULL)
    {
        err = switchPtr->GetSpanningTreePortState(sw,
                                                  stpInstance, 
                                                  port, 
                                                  stpState);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);
    }
    else
    {
        err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

        cpi = GET_PORT_INDEX(sw, port);
        *stpState = instance->states[cpi];
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmGetSpanningTreePortState */




/*****************************************************************************/
/** fmGetSpanningTreeFirst
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing spanning tree instance number,
 *                  which will always be instance 0.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstStpInstance points to caller-allocated storage where
 *                  this function should place the first existing spanning
 *                  tree instance number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetSpanningTreeFirst(fm_int  sw,
                                 fm_int *firstStpInstance)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;
    fm_treeIterator     iter;
    fm_uint64           firstKey;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d firstStpInstance=%p\n",
                     sw,
                     (void *) firstStpInstance);

    err = StpInstancePreamble(sw, -1, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);
        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Use the tree iterator to find the instance.
     **************************************************/

    fmTreeIterInit(&iter, stpInfo);

    err = fmTreeIterNext(&iter, &firstKey, (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    *firstStpInstance = (fm_int) firstKey;

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmGetSpanningTreeFirst */




/*****************************************************************************/
/** fmGetSpanningTreeNext
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing spanning tree instance number,
 *                  following a prior call to this function or to
 *                  ''fmGetSpanningTreeFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentStpInstance is the last spanning tree instance
 *                  number found by a previous call to this function or to
 *                  ''fmGetSpanningTreeFirst''.
 *
 * \param[out]      nextStpInstance points to caller-allocated storage where
 *                  this function should place the number of the next spanning
 *                  tree instance. Will be set to -1 if no more spanning tree
 *                  instances are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if no more spanning tree instances are found.
 *
 *****************************************************************************/
fm_status fmGetSpanningTreeNext(fm_int  sw,
                                fm_int  currentStpInstance,
                                fm_int *nextStpInstance)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;
    fm_uint64           nextKey;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d currentStpInstance=%d nextStpInstance=%p\n",
                     sw,
                     currentStpInstance,
                     (void *) nextStpInstance);

    err = StpInstancePreamble(sw, -1, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Use the tree iterator to find the instance.
     **************************************************/

    err = fmTreeSuccessor(stpInfo,
                          currentStpInstance,
                          &nextKey,
                          (void **) &instance);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    *nextStpInstance = (fm_int) nextKey;

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmGetSpanningTreeNext */




/*****************************************************************************/
/** fmGetSpanningTreeVlanFirst
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first VLAN in a spanning tree instance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the number of the spanning tree instance
 *                  that should be searched for VLANs.
 *
 * \param[out]      firstVlan points to caller-allocated storage where this
 *                  function should place the first VLAN in the spanning tree
 *                  instance. Will be set to -1 if no VLANs are found in the
 *                  spanning tree instance.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance does not exist.
 * \return          FM_ERR_NO_MORE if no VLANs are found.
 *
 *****************************************************************************/
fm_status fmGetSpanningTreeVlanFirst(fm_int  sw,
                                     fm_int  stpInstance,
                                     fm_int *firstVlan)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d firstVlan=%p\n",
                     sw,
                     stpInstance,
                     (void *) firstVlan);

    err = StpInstancePreamble(sw, stpInstance, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Find the instance and search the members.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
    if (err == FM_ERR_NOT_FOUND)
    {
        /* For consistency, return Invalid-Argument instead of Key-not-found */
        err = FM_ERR_INVALID_ARGUMENT;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmFindBitInBitArray(&instance->vlans, 0, TRUE, firstVlan);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    if (*firstVlan == -1)
    {
        err = FM_ERR_NO_MORE;
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmGetSpanningTreeVlanFirst */




/*****************************************************************************/
/** fmGetSpanningTreeVlanNext
 * \ingroup stp
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next VLAN in a spanning tree instance
 *                  following a prior call to this function or to
 *                  ''fmGetSpanningTreeVlanFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       stpInstance is the number of the spanning tree instance
 *                  that should be searched for VLANs.
 *
 * \param[in]       currentVlan is the last VLAN number found by a previous
 *                  call to this function or to ''fmGetSpanningTreeVlanFirst''.
 *
 * \param[out]      nextVlan points to caller-allocated storage where
 *                  this function should place the number of the next VLAN
 *                  found in the spanning tree instance. Will be set to -1 if
 *                  no more VLANs are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if stpInstance does not exist.
 * \return          FM_ERR_NO_MORE if no VLANs are found in the spanning tree
 *                  instance.
 *
 *****************************************************************************/
fm_status fmGetSpanningTreeVlanNext(fm_int  sw,
                                    fm_int  stpInstance,
                                    fm_int  currentVlan,
                                    fm_int *nextVlan)
{
    fm_status           err = FM_OK;
    fm_tree *           stpInfo;
    fm_stpInstanceInfo *instance;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STP,
                     "sw=%d stpInstance=%d currentVlan=%d nextVlan=%p\n",
                     sw,
                     stpInstance,
                     currentVlan,
                     (void *) nextVlan);

    err = StpInstancePreamble(sw, stpInstance, -1, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);
    }

    stpInfo   = GET_STP_INFO(sw);

    /***************************************************
     * Find the instance and search the members.
     **************************************************/

    err = fmTreeFind(stpInfo, stpInstance, (void **) &instance);
    if (err == FM_ERR_NOT_FOUND)
    {
        /* For consistency, return invalid-argument instead of key-not-found */
        err = FM_ERR_INVALID_ARGUMENT;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    err = fmFindBitInBitArray(&instance->vlans,
                              currentVlan + 1,
                              TRUE,
                              nextVlan);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STP, err);

    if (*nextVlan == -1)
    {
        err = FM_ERR_NO_MORE;
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT_API(FM_LOG_CAT_STP, err);

}   /* end fmGetSpanningTreeVlanNext */




/*****************************************************************************/
/** fmRefreshSpanningTreeStateForVlan
 * \ingroup intStp
 *
 * \desc            Called by other API components to update a VLAN's STP
 *                  state.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       vlanID is the VLAN to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRefreshSpanningTreeStateForVlan(fm_int sw, fm_int vlanID)
{
    fm_status  err = FM_OK;
    fm_int     instance;

    FM_LOG_ENTRY(FM_LOG_CAT_STP, "sw=%d vlanID=%d\n", sw, vlanID);

    err = StpInstancePreamble(sw, -1, vlanID, FALSE);

    if (err != FM_OK)
    {
        err = StpInstancePostamble(sw, err, err);

        FM_LOG_EXIT(FM_LOG_CAT_STP, err);
    }

    /***************************************************
     * Find the instance for the VLAN.
     **************************************************/

    err = fmFindInstanceForVlan(sw, vlanID, &instance);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmRefreshStpState(sw, instance, vlanID, -1);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    /***************************************************
     * Cleanup, release locks, etc.
     **************************************************/
ABORT:
    err = StpInstancePostamble(sw, FM_OK, err);

    FM_LOG_EXIT(FM_LOG_CAT_STP, err);

}   /* end fmRefreshSpanningTreeStateForVlan */




/*****************************************************************************/
/** fmDbgDumpSpanningTree
 * \ingroup diagMisc 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the hardware spanning tree tables.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       instance is the instance number to dump. Use -1 for all
 *                  instances.
 *
 *
 *****************************************************************************/
void fmDbgDumpSpanningTree(fm_int sw, fm_int instance)
{
    fm_switch *switchPtr;

    VALIDATE_SWITCH_NO_RETURN(sw);

    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpSpanningTree, sw, instance);

    UNPROTECT_SWITCH(sw);

}   /* end fmDbgDumpSpanningTree */

