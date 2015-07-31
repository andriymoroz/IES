/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_stacking.c
 * Creation Date:   November 12, 2013
 * Description:     Prototypes for managing stacked intra and extra switch
 *                  aggregate systems on FM10000 devices.
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
/** fm10000CreateForwardingRule
 * \ingroup intStacking
 *
 * \desc            Creates a forwarding rule.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       ruleId points to caller allocated storage where the handle
 *                  of the new rule is written to.
 *
 * \param[in]       rule points to caller allocated storage containing the
 *                  rule to create.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_RESOURCES if there are no hardware resources
 *                  to accommodate the new forwarding rule.
 * \return          FM_ERR_NO_MEM if there is not enough memory to store
 *                  the forwarding rule data structure.
 *
 *****************************************************************************/
fm_status fm10000CreateForwardingRule(fm_int sw,
                                      fm_int *ruleId,
                                      fm_forwardRule *rule)
{
    fm_status                    err;
    fm_int                       camIndex;
    fm_glortCamEntry *           camEntry;
    fm_switch *                  switchPtr;
    fm_logicalPortInfo *         lportInfo;
    fm_forwardRuleInternal *     internalRule;
    fm10000_forwardRuleInternal *fwdExt;
    fm_stackingInfo *            stackingInfo;
    fm_port *                    portPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_STACKING,
                  "sw=%d, ruleId=%p, rule=%p\n",
                  sw,
                  (void *) ruleId,
                  (void *) rule );

    switchPtr = GET_SWITCH_PTR(sw);
    lportInfo = &switchPtr->logicalPortInfo;
    portPtr   = GET_PORT_PTR(sw, rule->logicalPort);
    fwdExt    = NULL;

    if (portPtr == NULL)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }

    stackingInfo = &switchPtr->stackingInfo;

    /***************************************************
     * Find an unused CAM entry.
     **************************************************/
    camIndex = fmFindUnusedCamEntry(sw);
    if (camIndex < 0)
    {
        err = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }

    camEntry = &lportInfo->camEntries[camIndex];

    /***************************************************
     * Create an FM10000 forwarding rule extension.
     **************************************************/
    fwdExt = (fm10000_forwardRuleInternal *) fmAlloc( sizeof(fm10000_forwardRuleInternal) );

    if (fwdExt == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }

    fwdExt->camEntry = camEntry;

    /***************************************************
     * Initialize the CAM entry.
     **************************************************/
    FM_MEMSET_S( camEntry, sizeof(fm_glortCamEntry), 0, sizeof(fm_glortCamEntry) );

    camEntry->camIndex = camIndex;
    camEntry->camKey   = rule->glort;
    camEntry->camMask  = ~rule->mask;   /* Hardware stores mask inverted from rule mask */

    if ( fmIsCardinalPort(sw, rule->logicalPort) )
    {
        camEntry->strict = FM_GLORT_ENTRY_TYPE_STRICT;
    }
    else
    {
        /* Needs to be hashed to support LAG on internal port for traffic
         * to be hashed across the internal LAG link. Especially
         * CPU traffic, since FTYPE is special delivery and won't
         * be hashed if set to ISL */
        camEntry->strict = FM_GLORT_ENTRY_TYPE_HASHED;
    }

    /***************************************************
     * This makes the assumption that the rule points
     * to a physical port that is not going to have its
     * dest table entry changing.
     **************************************************/
    camEntry->destIndex    = portPtr->destEntry->destIndex;
    camEntry->destCount    = 1;
    camEntry->useCount     = 1;

    /***************************************************
     * Write CAM entry to hardware.
     **************************************************/
    err = fm10000WriteGlortCamEntry(sw, camEntry, FM_UPDATE_CAM_AND_RAM);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING, "Wrote to CAM entry 0x%x\n", camIndex);

    /***************************************************
     * Get the underlying forwarding rule object.
     **************************************************/
    err = fmTreeFind( &stackingInfo->fwdRules,
                      *ruleId,
                      (void **) &internalRule );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    /***************************************************
     * Store pointer to extension.
     **************************************************/
    internalRule->extension = fwdExt;
    err                     = FM_OK;


ABORT:

    if (err != FM_OK)
    {
        if (fwdExt != NULL)
        {
            fmFree(fwdExt);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fm10000CreateForwardingRule */




/*****************************************************************************/
/** fm10000DeleteForwardingRule
 * \ingroup intStacking
 *
 * \desc            Deletes a rule given the rule ID.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       ruleId is the ID number of the rule to delete.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if id not recognized.
 *
 *****************************************************************************/
fm_status fm10000DeleteForwardingRule(fm_int sw, fm_int ruleId)
{
    fm_switch *                  switchPtr;
    fm_stackingInfo *            stackingInfo;
    fm_forwardRuleInternal *     internalRule;
    fm10000_forwardRuleInternal *fwdExt;
    fm_status                    err;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING, "sw=%d, id=%d\n", sw, ruleId);

    switchPtr    = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;

    err = fmTreeFind( &stackingInfo->fwdRules,
                      ruleId,
                      (void **) &internalRule );
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

    fwdExt = (fm10000_forwardRuleInternal *) internalRule->extension;

    /* this invalidates the CAM entry */
    fwdExt->camEntry->camKey = 0;
    fwdExt->camEntry->camMask = 0;

    err = fm10000WriteGlortCamEntry(sw, fwdExt->camEntry, FM_UPDATE_CAM_ONLY);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING,
                 "Reset CAM entry 0x%x\n",
                 fwdExt->camEntry->camIndex);

    /* mark it available */
    fwdExt->camEntry->useCount = 0;

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);

}   /* end fm10000DeleteForwardingRule */




/*****************************************************************************/
/** fm10000CreateLogicalPortForGlort
 * \ingroup intStacking
 *
 * \desc            Creates a logical port for the given glort.  This occurs
 *                  only if the glort is not local to the current switch and
 *                  a forwarding rule exists for the glort, otherwise an
 *                  error is returned.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       glort is the glort for which a logical port is to be created
 *
 * \param[out]      logicalPort is a pointer to an integer into which the
 *                  new logical port will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_GLORT_IN_USE if the specified is already being used
 *                  by others.
 * \return          FM_ERR_NO_FORWARDING_RULES if no forwarding rule associated
 *                  with glort.
 * \return          FM_ERR_NO_FREE_RESOURCES if logical port numbers available
 *                  for allocation.
 * \return          FM_ERR_NO_MEM if no memory available to store logical
 *                  port data structure.
 *
 *****************************************************************************/
fm_status fm10000CreateLogicalPortForGlort(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_int *  logicalPort)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm_stackingInfo *       stackingInfo;
    fm_port *               portPtr;
    fm_glortCamEntry *      camEntry;
    fm10000_port *          portExt;
    fm_treeIterator         iter;
    fm_forwardRuleInternal *tmpRule;
    fm_uint64               tmpId;
    fm_bool                 found;

    FM_LOG_ENTRY( FM_LOG_CAT_STACKING,
                  "sw=%d, glort=%d, logicalPort=%p\n",
                  sw,
                  glort,
                  (void *) logicalPort );

    switchPtr    = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;
    found        = FALSE;

    /* If there is an existing entry already, then return the existing one */
    err = fmGetGlortLogicalPort(sw, glort, logicalPort);
    if (err == FM_OK)
    {
        /* The port already exists, see if it is the same type */
        portPtr = GET_PORT_PTR(sw, *logicalPort);

        if (portPtr->portType == FM_PORT_TYPE_REMOTE)
        {
            /* Yes, return the existing port */
            FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);
        }
        else
        {
            /* The glort is being used for something else */
            FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_GLORT_IN_USE);
        }
    }

    /***************************************************
     * Search the tree for a forwarding rule with a
     * matching glort.
     **************************************************/
    fmTreeIterInit(&iter, &stackingInfo->fwdRules);

    err = fmTreeIterNext( &iter, &tmpId, (void **) &tmpRule );

    while (err != FM_ERR_NO_MORE)
    {
        if ( ( ~tmpRule->rule.mask & tmpRule->rule.glort ) ==
             ( ~tmpRule->rule.mask & glort ) )
        {
            found = TRUE;
            break;
        }

        err = fmTreeIterNext( &iter, &tmpId, (void **) &tmpRule );
    }

    if ( !found )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_STACKING,
                     "Glort 0x%x was not matched to a forwarding rule\n",
                     glort);

        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_FORWARDING_RULES);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING,
                 "Glort 0x%x was matched to forwarding rule #%lld\n",
                 glort,
                 tmpId);

    camEntry = ( (fm10000_forwardRuleInternal *) tmpRule->extension )->camEntry;

    /***************************************************
     * Find an unused logical port number.
     **************************************************/
    *logicalPort = fmFindUnusedLogicalPorts(sw, 1);
    if (*logicalPort == -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_FREE_RESOURCES);
    }

    /***************************************************
     * Create an entry for the remote logical port.
     **************************************************/
    portPtr = (fm_port *) fmAlloc(sizeof(fm_port));
    if (portPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_MEM);
    }

    FM_MEMSET_S( portPtr, sizeof(fm_port), 0, sizeof(fm_port) );

    portExt = (fm10000_port *) fmAlloc( sizeof(fm10000_port) );
    if (portExt == NULL)
    {
        fmFree(portPtr);
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_MEM);
    }

    FM_MEMSET_S( portExt, sizeof(fm10000_port), 0, sizeof(fm10000_port) );

    portPtr->switchPtr   = switchPtr;
    portPtr->portNumber  = *logicalPort;
    portPtr->portType    = FM_PORT_TYPE_REMOTE;
    portPtr->extension   = portExt;
    portPtr->lagIndex    = -1;
    portPtr->memberIndex = -1;
    portPtr->glort       = glort;
    portPtr->swagPort    = -1;
    portPtr->camEntry    = camEntry;

    /* Increase the CAM reference use count */
    portPtr->camEntry->useCount++;

    portPtr->destEntry  = NULL;
    portPtr->numDestEntries = 0;

    /* Initialize multicast group membership list */
    fmTreeInit(&portPtr->mcastGroupList);

    /* Set the default to down, the application must
     * bring it up, similar to physical port. */
    portPtr->mode = FM_PORT_STATE_DOWN;

    /***************************************************
     * Add it to the logical port table.
     **************************************************/
    switchPtr->portTable[*logicalPort] = portPtr;

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fm10000CreateLogicalPortForGlort */




/*****************************************************************************/
/** fm10000CreateLogicalPortForMailboxGlort
 * \ingroup intStacking
 *
 * \desc            Creates a logical port for the given mailbox glort.  
 *                  This occurs only if the glort is not local 
 *                  to the current switch.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       glort is the glort for which a logical port is to be created
 *
 * \param[out]      logicalPort is a pointer to an integer into which the
 *                  new logical port will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_GLORT_IN_USE if the specified is already being used
 *                  by others.
 * \return          FM_ERR_NO_FREE_RESOURCES if logical port numbers available
 *                  for allocation.
 * \return          FM_ERR_NO_MEM if no memory available to store logical
 *                  port data structure.
 *
 *****************************************************************************/
fm_status fm10000CreateLogicalPortForMailboxGlort(fm_int    sw,
                                                  fm_uint32 glort,
                                                  fm_int *  logicalPort)
{
    fm_status        err;
    fm_switch *      switchPtr;
    fm_stackingInfo *stackingInfo;
    fm_port *        portPtr;
    fm10000_port *   portExt;
    fm_bool          found;


    FM_LOG_ENTRY( FM_LOG_CAT_STACKING,
                  "sw=%d, glort=%d, logicalPort=%p\n",
                  sw,
                  glort,
                  (void *) logicalPort );

    switchPtr    = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;
    found        = FALSE;

    /* If there is an existing entry already, then return the existing one */
    err = fmGetGlortLogicalPort(sw, glort, logicalPort);
    if (err == FM_OK)
    {
        /* The port already exists, see if it is the same type */
        portPtr = GET_PORT_PTR(sw, *logicalPort);

        if (portPtr->portType == FM_PORT_TYPE_REMOTE)
        {
            /* Yes, return the existing port */
            FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);
        }
        else
        {
            /* The glort is being used for something else */
            FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_GLORT_IN_USE);
        }
    }

    err = FM_OK;

    /***************************************************
     * Find an unused logical port number.
     **************************************************/
    *logicalPort = fmFindUnusedLogicalPorts(sw, 1);
    if (*logicalPort == -1)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_FREE_RESOURCES);
    }

    /***************************************************
     * Create an entry for the remote logical port.
     **************************************************/
    portPtr = (fm_port *) fmAlloc(sizeof(fm_port));
    if (portPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_MEM);
    }

    FM_MEMSET_S( portPtr, sizeof(fm_port), 0, sizeof(fm_port) );

    portExt = (fm10000_port *) fmAlloc( sizeof(fm10000_port) );
    if (portExt == NULL)
    {
        fmFree(portPtr);
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_NO_MEM);
    }

    FM_MEMSET_S( portExt, sizeof(fm10000_port), 0, sizeof(fm10000_port) );

    portPtr->switchPtr   = switchPtr;
    portPtr->portNumber  = *logicalPort;
    portPtr->portType    = FM_PORT_TYPE_REMOTE;
    portPtr->extension   = portExt;
    portPtr->lagIndex    = -1;
    portPtr->memberIndex = -1;
    portPtr->glort       = glort;
    portPtr->swagPort    = -1;

    /* Remote port structures created for mailbox purpose does not have 
       CAM entries assigned, as these entries are not created dynamically.
       Mailbox CAM entries are created at the mailbox init stage to optimise
       GLORT_CAM/GLORT_RAM entries usage. */
    portPtr->camEntry    = NULL;

    /* Initialize multicast group membership list */
    fmTreeInit(&portPtr->mcastGroupList);

    /***************************************************
     * Add it to the logical port table.
     **************************************************/
    switchPtr->portTable[*logicalPort] = portPtr;

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fm10000CreateLogicalPortForMailboxGlort */




/*****************************************************************************/
/** fm10000RedirectCpuTrafficToPort
 * \ingroup intStacking
 *
 * \desc            Redirect CPU traffic to the specified logical port.
 *                  The logical port can be a physical port or a LAG.
 *
 * \note            This function may be called before the port table has
 *                  been initialized.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port.
 *                  port value -1 indicates current cpu port configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          Other 'Status Codes' ad appropriate in case of failure. 
 *
 *****************************************************************************/
fm_status fm10000RedirectCpuTrafficToPort(fm_int sw, fm_int port)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint64       rv1;
    fm_portmask     destMask;
    fm_status       err;
    fm_int          portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int          numPorts;
    fm_int          physPort;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    FM_PORTMASK_DISABLE_ALL(&destMask);

    if (port < 0)
    {
        if (port == -1)
        {
            /* Use the current configuration */
            port = switchPtr->cpuPort;
        }
        else
        {
            /* port number is invalid, return error */
            FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_PORT);
        }
    }
    else
    {
        switchPtr->cpuPort = port;
    }

    if ( fmIsCardinalPort(sw, port) )
    {
        err = fmEnablePortInPortMask(sw, &destMask, port);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

        /* We have to do a formal translation here, since the port table
         * may not exist yet. */
        err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);
        
        rv1 = (1 << physPort);
    }
    else if ( fmIsValidLagPort(sw, port) )
    {
        /* CPU traffic can't be filtered over internal LAG; hence, we can
         * only redirect the traffic to a single active member port, or 
         * result in duplicate packets. */
        err = fmGetLAGMemberPorts(sw,
                                  GET_PORT_PTR(sw, port)->lagIndex,
                                  &numPorts,
                                  portList,
                                  FM_MAX_NUM_LAG_MEMBERS,
                                  TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

        rv1 = 0;

        if (numPorts > 0)
        {
            err = fmMapLogicalPortToPhysical(switchPtr, portList[0], &physPort);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

            rv1 |= (FM_LITERAL_U64(1) << physPort);

            err = fmEnablePortInPortMask(sw, &destMask, portList[0]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);
        }
    }
    else
    {
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, FM_ERR_INVALID_PORT);
    }

    /* Change the destination mask for cpu port so traffic
     * matching CPU glort will be redirected to the specified port. */
    err = switchPtr->SetLogicalPortAttribute(sw,
                                             switchPtr->cpuPort,
                                             FM_LPORT_DEST_MASK,
                                             &destMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

    /* Update the CPU_TRAP_MASK so trapped frames go to the CPU. */
    err = switchPtr->WriteUINT64(sw, FM10000_CPU_TRAP_MASK_FH(0), rv1);

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fm10000RedirectCpuTrafficToPort */




/*****************************************************************************/
/**fm10000InformRedirectCPUPortLinkChange
 * \ingroup intStacking
 *
 * \desc            Inform a link change for a port. This will update
 *                  the port mask appropriately for the CPU port
 *                  when the traffic is redirected over a LAG.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port.
 *
 * \param[in]       linkStatus is the link status of the port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InformRedirectCPUPortLinkChange(fm_int            sw,
                                                 fm_int            port,
                                                 fm_portLinkStatus linkStatus)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_int     members[FM_MAX_NUM_LAG_MEMBERS];
    fm_int     numMembers;
    fm_int     cnt;

    FM_NOT_USED(linkStatus);

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->cpuPort == 0)
    {
        /* real CPU port is always up, no need to do anything */
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);
    }

    err = fmGetLAGCardinalPortList(sw,
                                   switchPtr->cpuPort,
                                   &numMembers,
                                   members,
                                   FM_MAX_NUM_LAG_MEMBERS);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);

    for (cnt = 0 ; cnt < numMembers ; cnt++)
    {
        if (members[cnt] == port)
        {
            /* Link change for a member, update */
            err = fm10000RedirectCpuTrafficToPort(sw, -1);
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fm10000InformRedirectCPUPortLinkChange */

