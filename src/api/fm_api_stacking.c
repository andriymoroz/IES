/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stacking.c 
 * Creation Date:   June 12, 2008 
 * Description:     Functions for managing stacked intra and extra switch
 *                  aggregate systems.
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

#define REQUIRED_LOCALS                                                  \
    fm_switch *      switchPtr;                                          \
    fm_stackingInfo *stackingInfo;                                       \
    fm_status        err         = FM_OK;                                \
    fm_status        preambleErr = FM_OK;                                \
    fm_bool          lockSwitch  = FALSE;

#define ALLOW_SW_UP      0x1
#define ALLOW_SW_DOWN    0x2
#define ALLOW_SW_UP_DOWN 0x3

#define PREAMBLE_FRAGMENT(mode)                                             \
    {                                                                       \
        fm_bool switchIsUp = FALSE;                                         \
        preambleErr = StackingPreamble(sw, lockSwitch,                      \
                                       &switchPtr, &stackingInfo);          \
        if ( preambleErr != FM_OK )                                         \
        {                                                                   \
            err = StackingPostamble(sw, lockSwitch, preambleErr, FM_OK);    \
            FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);                      \
        }                                                                   \
        switch (switchPtr->state)                                           \
        {                                                                   \
            case FM_SWITCH_STATE_UP:                                        \
            case FM_SWITCH_STATE_GOING_DOWN:                                \
                switchIsUp = TRUE;                                          \
                break;                                                      \
            default:                                                        \
                break;                                                      \
        }                                                                   \
        if (!switchIsUp && !(mode & ALLOW_SW_DOWN))                         \
        {                                                                   \
            err = StackingPostamble(sw,                                     \
                                    lockSwitch,                             \
                                    preambleErr,                            \
                                    FM_ERR_SWITCH_NOT_UP);                  \
            FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);                      \
        }                                                                   \
        if (switchIsUp && !(mode & ALLOW_SW_UP))                            \
        {                                                                   \
            err = StackingPostamble(sw,                                     \
                                    lockSwitch,                             \
                                    preambleErr,                            \
                                    FM_ERR_SWITCH_IS_NOT_DOWN);             \
            FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);                      \
        }                                                                   \
     }

#define POSTAMBLE_FRAGMENT()                                                \
    {                                                                       \
        err = StackingPostamble(sw, lockSwitch, preambleErr, err);          \
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);                          \
    }


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
/** StackingPreamble
 * \ingroup intStacking
 *
 * \desc            Called at the beginning of any user exposed stacking
 *                  function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lockSwitch indicates whether to lock or protect the switch.
 *
 * \param[out]      switchPtr points to caller-allocated storage where the
 *                  pointer to the switch state structure is written.
 *
 * \param[out]      infoPtr points to caller-allocated storage where the
 *                  pointer to the stacking information state structure is
 *                  written.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid
 *
 *****************************************************************************/
static fm_status StackingPreamble(fm_int            sw, 
                                  fm_bool           lockSwitch, 
                                  fm_switch **      switchPtr, 
                                  fm_stackingInfo **infoPtr)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, switchPtr=%p, infoPtr=%p\n",
                 sw, (void *) switchPtr, (void *) infoPtr);

    if ( !SWITCH_LOCK_EXISTS(sw) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_SWITCH);
    }

    if ( !switchPtr )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_SWITCH);
    }

    *switchPtr = GET_SWITCH_PTR(sw);

    if ( !*switchPtr )
    {
        /* This can get here when switch is inserted and removed
         * Since switch lock exists, but switch state is freed
         */
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_SWITCH);
    }


    if ( lockSwitch )
    {
        err = LOCK_SWITCH(sw);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }
    else
    {
        err = PROTECT_SWITCH(sw);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }

    /* Capture state lock here */

    *infoPtr = &(*switchPtr)->stackingInfo;

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);

}   /* end StackingPreamble */




/*****************************************************************************/
/** StackingPostamble
 * \ingroup intStacking
 *
 * \desc            Called at the end of any user exposed stacking function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lockSwitch indicates whether to unlock or unprotect the 
 *                  switch.
 *
 * \param[in]       preambleError is the error return from the preamble 
 *                  function
 *
 * \param[in]       retError is the error that the function body wants to
 *                  return.  The preamble error overrides the return error.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status StackingPostamble(fm_int    sw, 
                                   fm_bool   lockSwitch, 
                                   fm_status preambleError, 
                                   fm_status retError)
{
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, preambleError=%d retError=%d\n", 
                 sw, preambleError, retError);

    if ( (preambleError == FM_OK) && !lockSwitch )
    {
        /* Release state lock here */
    }

    if ( (preambleError == FM_OK) ) 
    {
        if ( lockSwitch )
        {
            err = UNLOCK_SWITCH(sw);
        }
        else 
        {
            err = UNPROTECT_SWITCH(sw);
        }
    }

    FM_LOG_EXIT( FM_LOG_CAT_STACKING,
                (preambleError != FM_OK) ? preambleError :
                ( (err != FM_OK) ? err : retError ) );

}   /* end StackingPostamble */




/*****************************************************************************/
/** CompareForwardingRules
 * \ingroup intStacking
 *
 * \desc            Comparator used to compare two forwarding rules for 
 *                  placement into the forwarding rules tree.
 *
 * \param[out]      a points to the first forwarding rule. 
 *
 * \param[out]      b points to the second forwarding rule. 
 *
 *
 * \return          -1 if a < b.
 * \return          0  if a == b.
 * \return          1  if a > b.
 *
 *****************************************************************************/
static fm_int CompareForwardingRules(const void *a, const void *b)
{
    fm_forwardRuleInternal *x = (fm_forwardRuleInternal *) a;
    fm_forwardRuleInternal *y = (fm_forwardRuleInternal *) b;

    if ((x->rule.glort & ~x->rule.mask) > (y->rule.glort & ~y->rule.mask))
    {
        return 1;
    }
    else if ((x->rule.glort & ~x->rule.mask) == (y->rule.glort & ~y->rule.mask))
    {
        return 0;
    }
    else
    {
        return -1;
    }

}   /* end CompareForwardingRules */




/*****************************************************************************/
/** DestroyForwardingRule
 * \ingroup intStacking
 *
 * \desc            Used to deallocate a forwarding rule.
 *
 * \param[out]      ptr is the object. 
 *
 *****************************************************************************/
static void DestroyForwardingRule(void *ptr)
{
    if (((fm_forwardRuleInternal *) ptr)->extension)
    {
        fmFree(((fm_forwardRuleInternal *) ptr)->extension);
    }
    fmFree(ptr);

}   /* end DestroyForwardingRule */




/*****************************************************************************/
/** ValidateStackGlortRange
 * \ingroup intStacking
 *
 * \desc            Used to validate glort range being changed.
 *
 * \param[in]       glortRange points to a ''fm_glortRange'' structure that
 *                  describes the glort space.
 *
 * \return          FM_OK if successful. 
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the specified range is not
 *                  large enough to accommodate the requested glorts.
 *
 *****************************************************************************/
fm_status ValidateStackGlortRange(fm_glortRange *glortRange)
{
    fm_uint32 maxGlort;

    maxGlort = glortRange->glortBase + glortRange->glortMask + 1;

    if ( (glortRange->portBaseGlort + glortRange->portCount) > maxGlort )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWAG, FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    if ( (glortRange->lagCount > 0) && (glortRange->lagBaseGlort < maxGlort) &&
         ((glortRange->lagBaseGlort + glortRange->lagCount) > maxGlort) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWAG, FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    if ( (glortRange->mcastCount > 0) && (glortRange->mcastBaseGlort < maxGlort) &&
         ((glortRange->mcastBaseGlort + glortRange->mcastCount) > maxGlort) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWAG, FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    if ( (glortRange->lbgCount > 0) && (glortRange->lbgBaseGlort < maxGlort) &&
         ((glortRange->lbgBaseGlort + glortRange->lbgCount) > maxGlort) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWAG, FM_ERR_GLORT_RANGE_TOO_SMALL);
    }

    return FM_OK;

}   /* end ValidateStackGlortRange */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmInitStacking
 * \ingroup intStacking
 *
 * \desc            Initializes the stacking subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitStacking(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_stackingInfo *   stackingInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;

    fmTreeInit(&stackingInfo->fwdRules);

    err = fmCreateBitArray(&stackingInfo->usedRuleIDs, 
                           FM_MAX_STACKING_FORWARDING_RULES);

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fmInitStacking */




/*****************************************************************************/
/** fmFreeStackingResources
 * \ingroup intStacking
 *
 * \desc            Frees any stacking resources upon switch state down.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeStackingResources(fm_int sw)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm_stackingInfo *       stackingInfo;
    fm_treeIterator         iter;
    fm_forwardRuleInternal *tmpRule;
    fm_uint64               tmpId;
    fm_int                  forwardingRuleID;


    FM_LOG_ENTRY(FM_LOG_CAT_STACKING, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;

    if (!fmTreeIsInitialized(&stackingInfo->fwdRules))
    {
        /* Initialization has not been called due to
         * switch bring up error, so when cleanup
         * just return here.
         */
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);
    }

    fmTreeIterInit(&iter, &stackingInfo->fwdRules); 

    err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 

    while (err != FM_ERR_NO_MORE)
    {
        /* Delete the rule in hardware to free claimed resources
         * This is mainly for checking CAM used when switch goes down
         */
        forwardingRuleID = tmpId;
        FM_API_CALL_FAMILY(err, 
                           switchPtr->DeleteForwardingRule,
                           sw,
                           forwardingRuleID);

        err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 
    }

    fmTreeDestroy(&stackingInfo->fwdRules, DestroyForwardingRule);

    err = fmDeleteBitArray(&stackingInfo->usedRuleIDs);

    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);


}   /* end fmFreeStackingResources */




/*****************************************************************************/
/** fmFindForwardingRulePortByGlort
 * \ingroup intStacking
 *
 * \desc            Find the logical port of the fowarding rule matching
 *                  matching a given glort
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       glort is glort value
 *
 * \param[out]      logicalPort is a pointer to hold the return logical port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFindForwardingRulePortByGlort(fm_int    sw, 
                                          fm_uint32 glort, 
                                          fm_int *  logicalPort)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm_stackingInfo *       stackingInfo;
    fm_treeIterator         iter;
    fm_forwardRuleInternal *tmpRule;
    fm_uint64               tmpId;
    fm_bool                 found = FALSE;


    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, glort=%d, logicalPort=%p\n",
                 sw, glort, (void *) logicalPort);

    switchPtr = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;

    fmTreeIterInit(&iter, &stackingInfo->fwdRules); 

    err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 

    while (err != FM_ERR_NO_MORE)
    {
        if ( ( ~tmpRule->rule.mask & tmpRule->rule.glort ) ==
             ( ~tmpRule->rule.mask & glort ) )
        {
            found = TRUE;
            break;
        }

        err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 
    }

    if ( !found )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_STACKING, 
                     "Glort 0x%x was not matched to a forwarding rule\n",
                     glort);

        /* return an error here */
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_FAIL);
    }

    *logicalPort = tmpRule->rule.logicalPort;
    
    FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_OK);

}   /* end fmFindForwardingRulePortByGlort */




/*****************************************************************************/
/** fmGetInternalPortFromRemotePort
 * \ingroup intStacking
 *
 * \desc            Find the logical port associated with the remote logical
 *                  port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       remotePort is the remote logical port number.
 *
 * \param[out]      internalPort is a pointer to hold the return logical port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT is input port is not a remote port.
 *
 *****************************************************************************/
fm_status fmGetInternalPortFromRemotePort(fm_int sw, 
                                          fm_int remotePort,
                                          fm_int *internalPort)
{

    fm_status               err = FM_OK;
    fm_status               tmpErr;
    fm_switch *             switchPtr;
    fm_uint32               glort;
    fm_port  *              portPtr;
    fm_port  *              internalPortPtr;
    fm_int                  port;
    fm_bool                 internalPortSearchDone;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, remotePort=%d, internalPort=%p\n",
                 sw, remotePort, (void *) internalPort);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr = switchPtr->portTable[remotePort];
    tmpErr  = FM_OK;

    if (portPtr != NULL)
    {
        switch (portPtr->portType)
        {
            case FM_PORT_TYPE_REMOTE:
                internalPortSearchDone = FALSE;
                while (!internalPortSearchDone)
                {
                    err = fmGetLogicalPortGlort(sw, remotePort, &glort);
    
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
        
                    /* For remote port, we find the internal port */
                    err = fmFindForwardingRulePortByGlort(sw, glort, &port);

                    /* If no forwarding rule was found, check if this glort
                       is assigned for mailbox purpose. */ 
                    if (err != FM_OK)
                    {
                        tmpErr = fmFindInternalPortByMailboxGlort(sw,
                                                                  glort,
                                                                  &port);

                        if (tmpErr == FM_OK)
                        {
                            err = FM_OK;
                        }
                    }

                    /* return error from finding forwarding rule function. */
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
        
                    *internalPort = port;
                    internalPortPtr = switchPtr->portTable[port];
                    if (internalPortPtr->portType == FM_PORT_TYPE_REMOTE)
                    {
                        remotePort = internalPortPtr->portNumber;
                    }
                    else
                    {
                        internalPortSearchDone = TRUE;
                    }
                }
                break;
            default:
                err = FM_ERR_INVALID_PORT;
                break;
        }
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_STACKING, err);

}   /* end fmGetInternalPortFromRemotePort */




/*****************************************************************************/
/** fmSetStackGlortRange
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deprecated in favor of ''fmSetStackGlortRangeExt''.
 *                                                                      \lb\lb
 *                  This function establishes the range of glorts that will be 
 *                  used by the switch. Each switch in a stack must get its 
 *                  own set of glorts so it can send and receive ISL tagged 
 *                  traffic without its glort space colliding with that of 
 *                  another switch. The specified range must be large enough 
 *                  to accommodate the number of logical ports on the switch.
 *                                                                      \lb\lb
 *                  Glorts for physical port are allocated starting from the
 *                  bottom of the range. LAGs created with ''fmCreateLAG'' or 
 *                  ''fmCreateLAGExt'' (which will be private to the switch) 
 *                  get glorts starting at glortBase + 0x100. Switch-private 
 *                  multicast groups created with ''fmCreateMcastGroup'' get 
 *                  glorts starting at glortBase + 0x400. Switch-private load 
 *                  balancing groups created with ''fmCreateLBG'' get glorts 
 *                  starting at glortBase + 0x400 + MaxMcastGlorts. The glort 
 *                  range should be specified large enough to accommodate any 
 *                  private LAGs, multicast groups and LBGs according to 
 *                  these hardcoded offset rules. If no private LAGs, multicast 
 *                  groups or LBGs will be created, then the range need be only 
 *                  large enough to accommodate the physical ports on the 
 *                  switch.
 *                                                                      \lb\lb
 *                  This function will automatically remap the switch's
 *                  logical ports to the new glort range.
 *
 * \note            This function can only be called when the switch is in the
 *                  down state. Set a switch down with ''fmSetSwitchState''.
 *                                                                      \lb\lb
 * \note            After calling this function, use the Stacking API version 
 *                  functions, ''fmCreateStackLAG'', ''fmCreateStackLBG'' and 
 *                  ''fmCreateStackMcastGroup'' to create stack-wide LAGs,
 *                  load balancing groups and multicast groups, respectively.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       glortBase is the first glort to use in the switch and 
 *                  should generally be a power of 2 to facilitate masked
 *                  glort ranges for more efficient CAM usage. Note that
 *                  while this argument is 32 bits wide, the hardware
 *                  supports only 16-bit glorts (the argument was made
 *                  oversized to accommodate future devices with wider
 *                  glorts). 
 *
 * \param[in]       mask identifies the range of glorts used by the switch
 *                  starting with glortBase. The mask is an "inverse mask"
 *                  where the 1 bits identify the wildcarded bits in the
 *                  glort range. The 1 bits in the mask must be contiguous. 
 *                  Note that while this argument is 32 bits wide, the hardware
 *                  supports only 16-bit glorts (the argument was made
 *                  oversized to accommodate future devices with wider
 *                  glorts).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if mask bits are not contiguous.
 * \return          FM_ERR_SWITCH_IS_NOT_DOWN if sw is not in the down state.
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the specified range is not
 *                  large enough to accommodate all logical ports on the switch.
 *
 *****************************************************************************/
fm_status fmSetStackGlortRange(fm_int sw, fm_uint32 glortBase, fm_uint32 mask)
{
    fm_int         cnt;
    fm_glortRange *glorts;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, glortBase=0x%x, mask=0x%x\n",
                 sw, glortBase, mask);

    for (cnt = 0 ; cnt < 32 ; cnt ++)
    {
        if (!(mask & (1<<cnt)))
        {
            break;
        }
    }
    for ( ; cnt < 32 ; cnt ++)
    {
        if ((mask & (1<<cnt)))
        {
            /* Cannot have non-continous mask */
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);
        }
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_DOWN);

    /* This could be moved to platform so we know exactly how 
     * large the minimum glort size required. But that requires another
     * switch pointer. So we can use just an approximate value
     */
    cnt = mask + 1;
    if ( cnt <= switchPtr->maxPhysicalPort)
    {
        err = FM_ERR_GLORT_RANGE_TOO_SMALL;
    } 
    else
    {
        glorts                 = &switchPtr->glortRange;
        glorts->glortBase      = glortBase;
        glorts->glortMask      = mask;
        glorts->portBaseGlort  = ~0;
        glorts->portCount      = ~0;
        glorts->lagBaseGlort   = ~0;
        glorts->lagCount       = ~0;
        glorts->mcastBaseGlort = ~0;
        glorts->mcastCount     = ~0;
        glorts->lbgBaseGlort   = ~0;
        glorts->lbgCount       = ~0;
        glorts->cpuPortCount   = 0;

        /* If the switch wants to perform extra processing, call the
         * switch-specific function
         */
        if (switchPtr->SetStackGlortRange != NULL)
        {
            err = switchPtr->SetStackGlortRange(sw);
        }
        else
        {
            err = FM_OK;
        }
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmSetStackGlortRange */




/*****************************************************************************/
/** fmGetStackGlortRange
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deprecated in favor of ''fmGetStackGlortRangeExt''.
 *                                                                      \lb\lb
 *                  This function gets the range of glorts used by a switch,
 *                  as set by ''fmSetStackGlortRange''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      glortBase points to caller-allocated storage where this
 *                  function should place the base glort of the switch.
 *
 * \param[out]      mask points to caller-allocated storage where this
 *                  function should place the glort range mask of the switch.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if glortBase or mask are NULL.
 *
 *****************************************************************************/
fm_status fmGetStackGlortRange(fm_int sw, 
                               fm_uint32 *glortBase, 
                               fm_uint32 *mask)
{
    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, glortBase=%p, mask=%p\n",
                 sw, (void *) glortBase, (void *) mask);

    if ( !glortBase || !mask )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP_DOWN);

    *glortBase = switchPtr->glortRange.glortBase;
    *mask      = switchPtr->glortRange.glortMask;

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackGlortRange */




/*****************************************************************************/
/** fmSetStackGlortRangeExt
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Establish the range of glorts that will be used by the
 *                  switch. Each switch in a stack must get its own set of
 *                  glorts so it can send and receive ISL tagged traffic 
 *                  without its glort space colliding with that of another 
 *                  switch. The specified range must be large enough to 
 *                  accommodate the number of logical ports on the switch.
 *                                                                      \lb\lb
 *                  This function will automatically remap the switch's
 *                  logical ports to the new glort range.
 *
 * \note            This function can only be called when the switch is in the
 *                  down state. Set a switch down with ''fmSetSwitchState''.
 *
 * \note            After this function has been called, the conventional
 *                  functions for creating LAGs (''fmCreateLAG'' and
 *                  ''fmCreateLAGExt''), load balancing groups (''fmCreateLBG''),
 *                  and multicast groups (''fmCreateMcastGroup'') will create
 *                  those entities using the glorts specified in the glortRange
 *                  argument to this function, and they will be private to
 *                  the switch. The Stacking API versions of these functions,
 *                  ''fmCreateStackLAG'', ''fmCreateStackLBG'', and 
 *                  ''fmCreateStackMcastGroup'', should be used to create 
 *                  stack-global LAGs, LBGs, and multicast groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       glortRange points to an ''fm_glortRange'' structure that
 *                  describes the glort space.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if glortRange is NULL.
 * \return          FM_ERR_INVALID_ARGUMENT if mask bits are not contiguous.
 * \return          FM_ERR_SWITCH_IS_NOT_DOWN if sw is not in the down state.
 * \return          FM_ERR_GLORT_RANGE_TOO_SMALL if the specified range is not
 *                  large enough to accommodate the requested glorts.
 *
 *****************************************************************************/
fm_status fmSetStackGlortRangeExt(fm_int sw, fm_glortRange *glortRange)
{
    fm_int        cnt;
    fm_glortRange oldGlorts;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY_API( FM_LOG_CAT_STACKING,
                     "sw=%d, glortRange=0x%p\n",
                     sw,
                     (void *) glortRange );

    if (glortRange == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING,
                 "sw %d, glortBase=0x%x, glortMask=0x%x, "
                 "portBaseGlort=0x%x, portCount=%d,\n"
                 "lagBaseGlort=0x%x, lagCount=%d, "
                 "mcastBaseGlort=0x%x, mcastCount=%d, "
                 "lbgBaseGlort=0x%x, lbgCount=%d\n",
                 sw,
                 glortRange->glortBase,
                 glortRange->glortMask,
                 glortRange->portBaseGlort,
                 glortRange->portCount,
                 glortRange->lagBaseGlort,
                 glortRange->lagCount,
                 glortRange->mcastBaseGlort,
                 glortRange->mcastCount,
                 glortRange->lbgBaseGlort,
                 glortRange->lbgCount);

    /* find the first zero bit in the mask */
    for (cnt = 0 ; cnt < 32 ; cnt ++)
    {
        if ( !( glortRange->glortMask & (1 << cnt) ) )
        {
            break;
        }
    }

    /* don't allow any one bits above the first zero bit */
    for ( ; cnt < 32 ; cnt ++)
    {
        if ( ( glortRange->glortMask & (1 << cnt) ) )
        {
            /* Cannot have non-continous mask */
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);
        }
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_DOWN);

    oldGlorts             = switchPtr->glortRange;
    switchPtr->glortRange = *glortRange;

    /* If the switch wants to perform extra processing, call the
     * switch-specific function
     */
    if (switchPtr->SetStackGlortRange != NULL)
    {
        err = switchPtr->SetStackGlortRange(sw);

        if (err != FM_OK)
        {
            switchPtr->glortRange = oldGlorts;
        }
    }
    else
    {
        err = ValidateStackGlortRange(glortRange);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_STACKING, err); 
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmSetStackGlortRangeExt */




/*****************************************************************************/
/** fmGetStackGlortRangeExt
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            This function gets the range of glorts used by a switch,
 *                  as set by ''fmSetStackGlortRangeExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      glortRange points to caller-allocated storage where this
 *                  function should place glort range information.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if glortRange is NULL.
 *
 *****************************************************************************/
fm_status fmGetStackGlortRangeExt(fm_int         sw, 
                                  fm_glortRange *glortRange)
{
    REQUIRED_LOCALS;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw=%d, glortRange=%p\n",
                     sw,
                     (void *) glortRange);

    if (glortRange == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP_DOWN);

    *glortRange = switchPtr->glortRange;

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackGlortRangeExt */




/*****************************************************************************/
/** fmGetStackGlort
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the glort assigned to the given logical port.
 *
 * \note            There is a 1-to-1 mapping between logical ports and
 *                  glorts.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[out]      glort points to caller-allocated storage where this
 *                  function should write the glort associated with 
 *                  remote port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if glort is NULL.
 *
 *****************************************************************************/
fm_status fmGetStackGlort(fm_int sw, fm_int port, fm_uint32 *glort)
{
    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, port=%d, glort=%p\n",
                 sw, port, (void *) glort);

    if ( !glort )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    if ( ( port < 0 ) || ( port >= FM_MAX_LOGICAL_PORT ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_PORT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP_DOWN);

    err = fmGetLogicalPortGlort(sw, port, glort);

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackGlort */




/*****************************************************************************/
/** fmCreateStackForwardingRule
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Specify how an ISL-tagged frame should be forwarded.
 *                  When a switch receives an ISL-tagged frame with a 
 *                  non-zero destination glort it will need to be able to 
 *                  map the glort to a destination port mask.  If the 
 *                  frame's destination does not reside on this switch, 
 *                  the switch must forward the frame to the next switch that 
 *                  is one hop closer to the final destination. This function
 *                  specifies the destination logical port for a range of 
 *                  glorts.
 *                                                                      \lb\lb
 *                  Forwarding rules should be designed so that an ISL tagged 
 *                  frame will traverse a stack of switches using an efficient 
 *                  path. Forwarding rules must be ordered inside the switch 
 *                  using a longest prefix match algorithm. It is the 
 *                  application's responsibility to specify the forwarding 
 *                  rules in the correct order.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      forwardingRuleID points to caller-allocated storage where 
 *                  this function should write the forwarding rule handle.
 *
 * \param[in]       rule points to the forwarding rule of type 
 *                  ''fm_forwardRule''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if forwardingRuleID or rule are
 *                  NULL.
 * \return          FM_ERR_NO_FREE_RESOURCES if there are no hardware resources
 *                  to accommodate the new forwarding rule.
 * \return          FM_ERR_NO_MEM if there is not enough memory to store
 *                  the forwarding rule data structure.
 *
 *****************************************************************************/
fm_status fmCreateStackForwardingRule(fm_int sw, 
                                      fm_int *forwardingRuleID, 
                                      fm_forwardRule *rule)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_stackingInfo *   stackingInfo;
    fm_bool             ruleInserted = FALSE;
 
    fm_forwardRuleInternal *newRule = NULL;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw=%d, forwardingRuleID=%p, rule=%p "
                     "(glort=0x%x, mask=0x%x, dest port=%d)\n",
                     sw, 
                     (void *) forwardingRuleID, 
                     (void *) rule,
                     (rule != NULL) ? rule->glort : (fm_uint32) ~0,
                     (rule != NULL) ? rule->mask : (fm_uint32) ~0,
                     (rule != NULL) ? rule->logicalPort : -1);

    if ( !forwardingRuleID || !rule )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    /* Use the standard macro here, so can use VALIDATE_LOGICAL_PORT */
    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, rule->logicalPort, ALLOW_LAG | ALLOW_CPU);

    switchPtr = GET_SWITCH_PTR(sw);
    stackingInfo = &switchPtr->stackingInfo;

    /* Allocate an ID */
    err = fmFindBitInBitArray(&stackingInfo->usedRuleIDs, 
                              0, 
                              FALSE, 
                              forwardingRuleID);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    err = fmSetBitArrayBit(&stackingInfo->usedRuleIDs, 
                           *forwardingRuleID, 
                           TRUE);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING, 
                 "Allocated forwardingRuleID %d\n", 
                 *forwardingRuleID);

    newRule = (fm_forwardRuleInternal *) fmAlloc(sizeof(fm_forwardRuleInternal));

    if (newRule == NULL)
    {
        (void)fmSetBitArrayBit(&stackingInfo->usedRuleIDs, 
                               *forwardingRuleID, 
                               FALSE);

        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);
    }

    /* Set to NULL so we know if we need to free it after */
    newRule->extension = NULL;

    err = fmTreeInsert(&stackingInfo->fwdRules,
                       *forwardingRuleID,
                       (void *) newRule);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    ruleInserted = TRUE;
    /* The inserted rule is a copy */
    newRule->rule = *rule;

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING,
                 "Inserted rule at %p into tree with key %d\n", 
                 (void *) newRule, *forwardingRuleID);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->CreateForwardingRule,
                       sw,
                       forwardingRuleID,
                       rule);

ABORT:
    if ( (err != FM_OK) && newRule )
    {
        (void)fmSetBitArrayBit(&stackingInfo->usedRuleIDs, 
                               *forwardingRuleID, 
                               FALSE);

        if (ruleInserted)
        {
            fmTreeRemoveCertain(&stackingInfo->fwdRules,
                                *forwardingRuleID,
                                DestroyForwardingRule); 
        }
        else
        {
            /* Not auto freed by tree removal */
            fmFree(newRule);
        }
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackForwardingRule */




/*****************************************************************************/
/** fmGetStackForwardingRule
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a forwarding rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       forwardingRuleID is the ID number of the rule to retrieve. 
 *
 * \param[out]      rule points to caller-allocated storage where this function
 *                  should copy the rule ''fm_forwardRule'' structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is NULL.
 * \return          FM_ERR_NOT_FOUND if forwardingRuleID is not recoginzed.
 *
 *****************************************************************************/
fm_status fmGetStackForwardingRule(fm_int sw, 
                                   fm_int forwardingRuleID, 
                                   fm_forwardRule *rule)
{
    fm_forwardRuleInternal *tmpRule;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, forwardingRuleID=%d, rule=%p\n",
                 sw, 
                 forwardingRuleID, 
                 (void *) rule);

    if ( !rule )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    err = fmTreeFind(&stackingInfo->fwdRules, 
                     forwardingRuleID,
                     (void **) &tmpRule);

    if (err == FM_OK)
    {
        *rule = tmpRule->rule;
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackForwardingRule */




/*****************************************************************************/
/** fmUpdateStackForwardingRule
 * \ingroup stacking
 *
 * \chips           FM6000
 *
 * \desc            Updates a forwarding rule previously created with
 *                  ''fmCreateStackForwardingRule''.
 *                                                                      \lb\lb
 *                  Forwarding rules should be designed so that an ISL tagged
 *                  frame will traverse a stack of switches using an efficient
 *                  path. Forwarding rules must be ordered inside the switch
 *                  using a longest prefix match algorithm. It is the
 *                  application's responsibility to specify the forwarding
 *                  rules in the correct order.
 *
 * \note            This function only supports a logical port change in
 *                  the forwarding rule. Glort and mask are left unchanged.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       forwardingRuleID points to caller-supplied storage where
 *                  the handle of the rule is written.
 *
 * \param[in]       newRule points to a structure describing the forwarding
 *                  rule.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if forwardingRuleID or newRule is
 *                  NULL.
 * \return          FM_ERR_NO_FORWARDING_RULES if there are no forwarding rules.
 * \return          FM_ERR_INVALID_PORT if port is invalid
 * \return          FM_ERR_NO_FREE_RESOURCES if there are no hardware resources
 *                  to accommodate the new forwarding rule.
 * \return          FM_ERR_NO_MEM if there is not enough memory to store
 *                  the forwarding rule data structure.
 *
 *****************************************************************************/
fm_status fmUpdateStackForwardingRule(fm_int          sw,
                                      fm_int *        forwardingRuleID,
                                      fm_forwardRule *newRule)
{
    fm_bool                 idAllocated = FALSE;

    fm_forwardRuleInternal *oldRuleInternal = NULL;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw=%d, forwardingRuleID=%p, newRule=%p "
                     "(glort=0x%x, mask=0x%x, dest port=%d)\n",
                     sw,
                     (void *) forwardingRuleID,
                     (void *) newRule,
                     (newRule != NULL) ? newRule->glort : (fm_uint32) ~0,
                     (newRule != NULL) ? newRule->mask : (fm_uint32) ~0,
                     (newRule != NULL) ? newRule->logicalPort : -1);

    /***************************************************
     * Validate arguments
     **************************************************/
    if ( (forwardingRuleID == NULL) || (newRule == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP_DOWN);

    if ( !fmIsValidPort(sw, newRule->logicalPort, ALLOW_LAG | ALLOW_CPU) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, FM_ERR_INVALID_PORT)
    }

    /***************************************************
     * Check whether forwarding rule is in a bit array -
     * the rule should exist!
     **************************************************/
    err = fmGetBitArrayBit(&stackingInfo->usedRuleIDs,
                           *forwardingRuleID,
                           &idAllocated);

    if ( (err == FM_OK) && (idAllocated == FALSE) )
        err = FM_ERR_NO_FORWARDING_RULES;

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    /***************************************************
     * Find existed rule and check what we need to change
     **************************************************/
    err = fmTreeFind(&stackingInfo->fwdRules,
                     *forwardingRuleID,
                     (void **)&oldRuleInternal);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    FM_LOG_DEBUG(FM_LOG_CAT_STACKING, "oldRule=%p, oldRuleExt=%p\n",
                 (void *)oldRuleInternal,
                 (oldRuleInternal) ? (void *)oldRuleInternal->extension : NULL);

    if ( (oldRuleInternal == NULL) || (oldRuleInternal->extension == NULL) )
    {
        /* rule does not exist or rule does not have
         * a FM6000 fwd rule extension (camEntry)! */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, FM_ERR_NO_FORWARDING_RULES);
    }

    if ( !fmIsValidPort(sw, oldRuleInternal->rule.logicalPort, ALLOW_LAG | ALLOW_CPU) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, FM_ERR_INVALID_PORT)
    }

    /* if rule is unchanged... */
    if ( (oldRuleInternal->rule.glort == newRule->glort) &&
         (oldRuleInternal->rule.mask == newRule->mask) &&
         (oldRuleInternal->rule.logicalPort == newRule->logicalPort) )
    {
        err = FM_OK;
        goto ABORT;
    }

    /***************************************************
     * Found an internal rule, the rule is valid, could try to
     * update this rule
     **************************************************/
    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateForwardingRule,
                       sw,
                       forwardingRuleID,
                       oldRuleInternal,
                       newRule);

ABORT:
    POSTAMBLE_FRAGMENT();
    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmUpdateStackForwardingRule */




/*****************************************************************************/
/** fmDeleteStackForwardingRule
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a forwarding rule previously created with
 *                  ''fmCreateStackForwardingRule''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       forwardingRuleID is the handle of the forwarding rule (as
 *                  returned by ''fmCreateStackForwardingRule'') to delete. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if forwardingRuleID is not recognized.
 *
 *****************************************************************************/
fm_status fmDeleteStackForwardingRule(fm_int sw, fm_int forwardingRuleID)
{
    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, forwardingRuleID=%d\n",
                 sw, 
                 forwardingRuleID);

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->DeleteForwardingRule,
                       sw,
                       forwardingRuleID);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    err = fmTreeRemove(&stackingInfo->fwdRules,
                       forwardingRuleID,
                       DestroyForwardingRule); 

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    err = fmSetBitArrayBit(&stackingInfo->usedRuleIDs, forwardingRuleID, FALSE);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

ABORT:
    POSTAMBLE_FRAGMENT();

}   /* end fmDeleteStackForwardingRule */




/*****************************************************************************/
/** fmGetStackForwardingRuleFirst
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first existing forwarding rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstRuleId points to caller-allocated storage where this
 *                  function should place the first existing forwarding rule
 *                  ID. If none found, firstRuleId will be set to -1.
 *
 * \param[out]      firstRule points to caller-allocated storage where this
 *                  function should copy the first exsiting forwarding rule's
 *                  ''fm_forwardRule'' structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if firstRuleId or firstRule is NULL.
 * \return          FM_ERR_NO_FORWARDING_RULES if there are no forwarding rules.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if a forwarding rule was
 *                  added or deleted while searching for the first rule.
 *                  
 *****************************************************************************/
fm_status fmGetStackForwardingRuleFirst(fm_int sw, 
                                        fm_int *firstRuleId, 
                                        fm_forwardRule *firstRule)
{
    fm_treeIterator         iter;
    fm_forwardRuleInternal *tmpRule;
    fm_uint64               tmpId;

    REQUIRED_LOCALS;

    if (!firstRuleId || !firstRule)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, firstRuleId=%p, firstRule=%p\n",
                 sw, 
                 (void *) firstRuleId, 
                 (void *) firstRule);

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    fmTreeIterInit(&iter, &stackingInfo->fwdRules);

    err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 

    if (err == FM_OK)
    {
        *firstRule = tmpRule->rule;
        *firstRuleId = tmpId;
    }
    else
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_FORWARDING_RULES;
        }
        
        *firstRuleId = -1;
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackForwardingRuleFirst */




/*****************************************************************************/
/** fmGetStackForwardingRuleNext
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next existing forwarding rule, following a 
 *                  prior call to this function or to 
 *                  ''fmGetStackForwardingRuleFirst''
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentRuleId is the last forwarding rule ID found by a 
 *                  previous call to this function or to 
 *                  ''fmGetStackForwardingRuleFirst''.
 *
 * \param[out]      nextRuleId points to caller-allocated storage where this 
 *                  function should place the ID of the next existing 
 *                  forwarding rule. Will be set to -1 if no more forwarding
 *                  rules found.
 *
 * \param[out]      nextRule points to caller-allocated storage where this
 *                  function should copy the next exsiting forwarding rule's
 *                  ''fm_forwardRule'' structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if nextRuleId or nextRule is NULL.
 * \return          FM_ERR_NO_FORWARDING_RULES if there are no more forwarding 
 *                  rules.
 * \return          FM_ERR_MODIFIED_WHILE_ITERATING if a forwarding rule was
 *                  added or deleted while searching for the next rule.
 *
 *****************************************************************************/
fm_status fmGetStackForwardingRuleNext(fm_int sw, 
                                       fm_int currentRuleId, 
                                       fm_int *nextRuleId, 
                                       fm_forwardRule *nextRule)
{
    fm_treeIterator         iter;
    fm_forwardRuleInternal *tmpRule;
    fm_uint64               tmpId;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, currentRuleId=%d, nextRuleId=%p, nextRule=%p\n",
                 sw, 
                 currentRuleId, 
                 (void *) nextRuleId, 
                 (void *) nextRule);

    if (!nextRuleId || !nextRule)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    err = fmTreeIterInitFromKey(&iter, 
                                &stackingInfo->fwdRules, 
                                currentRuleId);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    /* This gets the current value */
    err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);

    /* This gets the next value */
    err = fmTreeIterNext(&iter, &tmpId, (void **) &tmpRule); 

    if (err == FM_OK)
    {
        *nextRule = tmpRule->rule;
        *nextRuleId = tmpId;
    }
    else
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_FORWARDING_RULES;
        }
        
        *nextRuleId = -1;
    }
    
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_STACKING, err);


ABORT:
    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackForwardingRuleNext */




/*****************************************************************************/
/** fmGetStackForwardingRuleList
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the list of existing forwarding rule IDs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numForwardingRules points to caller-allocated storage where 
 *                  this function should place the number of forarding rules
 *                  listed.
 *
 * \param[out]      forwardingRuleIDs points to a caller-allocated array of 
 *                  size max where this function should place the list
 *                  of existing forwarding rules IDs.
 *
 * \param[in]       max is the size of the forwardingRuleIDs array. This 
 *                  function will not retrieve more than max number of 
 *                  forwarding rule IDs. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if forwardingRuleIDs or 
 *                  numForwardingRules is NULL.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate the 
 *                  entire list of existing forwarding rules IDs.
 *
 *****************************************************************************/
fm_status fmGetStackForwardingRuleList(fm_int sw,
                                       fm_int *numForwardingRules,
                                       fm_int *forwardingRuleIDs,
                                       fm_int max)
{
    fm_treeIterator         iter;
    fm_forwardRuleInternal *nextRule;
    fm_uint64               nextId;

    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, numForwardingRules=%p, forwardingRuleIDs=%p, max=%d\n",
                 sw, 
                 (void *) numForwardingRules, 
                 (void *) forwardingRuleIDs, 
                 max);

    if (!forwardingRuleIDs || !numForwardingRules)
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    fmTreeIterInit(&iter, &stackingInfo->fwdRules); 

    /* This gets the current value */
    err = fmTreeIterNext(&iter, &nextId, (void **) &nextRule); 

    for ( *numForwardingRules = 0 ; err == FM_OK ; )
    {
        if (*numForwardingRules < max)
        {
            forwardingRuleIDs[(*numForwardingRules)++] = (fm_int) nextId;
    
            err = fmTreeIterNext(&iter, &nextId, (void **) &nextRule); 
        }
        else
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }
    }

    /* This error is the only valid error */
    if ( err == FM_ERR_NO_MORE )
    {
        err = FM_OK;
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmGetStackForwardingRuleList */




/*****************************************************************************/
/** fmCreateStackLogicalPort
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Creates a logical port for the given glort.  This function
 *                  is used when the switch must be able to address a glort
 *                  that exists on a remote switch. 
 *                                                                      \lb\lb
 *                  If a logical port has already been assigned to this glort, 
 *                  then this function will return the existing logical port 
 *                  instead of allocating a new one.
 *
 * \note            A forwarding rule must exist for the specified glort
 *                  before a logical port can be associated with it.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       glort is the glort to which the new logical port is to
 *                  be assigned. Note that while this argument is 32 bits 
 *                  wide, the hardware supports only 16-bit glorts (the 
 *                  argument was made oversized to accommodate future devices 
 *                  with wider glorts).
 *
 * \param[out]      logicalPort is a pointer to caller-allocated memory where
 *                  this function should place the newly created logical
 *                  port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if logicalPort is NULL.
 * \return          FM_ERR_GLORT_IN_USE if the specified is already being used
 *                  by others.
 * \return          FM_ERR_NO_FORWARDING_RULES if there is no forwarding rule 
 *                  associated with glort.
 * \return          FM_ERR_NO_FREE_RESOURCES if no logical port numbers 
 *                  available for allocation.
 * \return          FM_ERR_NO_MEM if no memory available to store logical
 *                  port data structure.
 *
 *****************************************************************************/
fm_status fmCreateStackLogicalPort(fm_int sw, 
                                   fm_uint32 glort, 
                                   fm_int *logicalPort)
{
    REQUIRED_LOCALS;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, glort=%d, logicalPort=%p\n",
                 sw, glort, (void *) logicalPort);

    if ( !logicalPort )
    {
        FM_LOG_EXIT(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    PREAMBLE_FRAGMENT(ALLOW_SW_UP);

    err = fmCreateLogicalPortForGlort(sw, glort, logicalPort);
    
    if (err == FM_OK)
    {
         err = fmPlatformSetRemoteGlortToLogicalPortMapping(sw,
                                                            glort,
                                                            *logicalPort,
                                                            TRUE);
         if (err != FM_OK)
         {
             fmFreeLogicalPort(sw, *logicalPort);
         }
    }

    POSTAMBLE_FRAGMENT();

}   /* end fmCreateStackLogicalPort */



/*****************************************************************************/
/** fmDeleteStackLogicalPort
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a logical port created by ''fmCreateStackLogicalPort''. 
 *
 * \note            All references to this port must be removed before deleting
 *                  the logical port. Otherwise deleting the logical port with
 *                  attached resources will cause unpredictable behaviour. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the remote port on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if port is NULL.
 *
 *****************************************************************************/
fm_status fmDeleteStackLogicalPort(fm_int sw, 
                                   fm_int port)
{
    fm_status  err;
    fm_port   *portPtr;
    fm_switch *switchPtr;
    fm_uint32  glort;

    FM_LOG_ENTRY(FM_LOG_CAT_STACKING,
                 "sw=%d, port=%d\n",
                 sw, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_REMOTE);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = switchPtr->portTable[port];

    glort = 0;

    if (portPtr->portType == FM_PORT_TYPE_REMOTE)
    {
        glort = portPtr->glort;
        err = fmFreeLogicalPort(sw, port);
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }

    if (err == FM_OK)
    {
        err = fmPlatformSetRemoteGlortToLogicalPortMapping(sw,
                                                           glort,
                                                           port,
                                                           FALSE);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);
}   /* end fmDeleteStackLogicalPort */




/*****************************************************************************/
/** fmSetStackLogicalPortState
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Indicate the state of a remote port when there is a link
 *                  status change for the corresponding physical port on the
 *                  remote switch. This function should be called on every
 *                  switch (other than the switch that possesses the physical
 *                  port that went down) that uses the remote port, for example
 *                  in a stacked LAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the remote port on which to operate.
 *
 * \param[in]       mode indicates the port state (see 'Port States') to set.
 *                  Valid modes are ''FM_PORT_STATE_UP'' and 
 *                  ''FM_PORT_STATE_DOWN''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmSetStackLogicalPortState(fm_int sw, fm_int port, fm_int mode)
{
    fm_status err;
    fm_port * portPtr;
    fm_switch * switchPtr;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw=%d port=%d mode=%d\n",
                     sw,
                     port,
                     mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_REMOTE);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Allow switch to override standard processing */
    if (switchPtr->SetStackLogicalPortState != NULL)
    {
        err = switchPtr->SetStackLogicalPortState(sw, port, mode);
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);
    }

    portPtr   = switchPtr->portTable[port];

    if (portPtr->portType == FM_PORT_TYPE_REMOTE)
    {
        /* Save the setting */
        portPtr->mode       = mode;
        portPtr->submode    = mode;
        err                 = FM_OK;

        /* Notify LAG and others */
        if (mode != FM_PORT_STATE_UP)
        {
            portPtr->linkUp = FALSE;

            /* Don't check return code */
            (void) fmInformLAGPortDown(sw, port);
            
            if (switchPtr->UpdateMirrorGroups != NULL)
            {
                err = switchPtr->UpdateMirrorGroups(sw, port, FALSE);
            }
        }
        else
        {
            portPtr->linkUp = TRUE;

            (void) fmInformLAGPortUp(sw, port);

            if (switchPtr->UpdateMirrorGroups != NULL)
            {
                err = switchPtr->UpdateMirrorGroups(sw, port, TRUE);
            }
         }
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmSetStackLogicalPortState */





/*****************************************************************************/
/** fmGetStackLogicalPortState
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return the port state of a remote logical port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the remote port on which to operate.
 *
 * \param[in]       mode points to caller-allocated storage 
 *                  where this function should place the port state
                    (see 'Port States').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetStackLogicalPortState(fm_int sw, fm_int port, fm_int *mode)
{
    fm_status err;
    fm_port * portPtr;
    fm_switch * switchPtr;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw=%d port=%d mode=%p\n",
                     sw,
                     port,
                     (fm_voidptr) mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_REMOTE);

    switchPtr = GET_SWITCH_PTR(sw);

    portPtr   = switchPtr->portTable[port];

    if (portPtr->portType == FM_PORT_TYPE_REMOTE)
    {
        *mode      = portPtr->mode;
        err        = FM_OK;
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetStackLogicalPortState */




/*****************************************************************************/
/** fmAllocateStackMcastGroups
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Allocate a set of stacked multicast group numbers given a 
 *                  glort range. The function returns the base multicast group
 *                  number and the number of multicast group numbers allocated. 
 *                  The returned set of multicast group numbers can then
 *                  be used in subsequent calls to ''fmCreateStackMcastGroup''.
 *
 * \note            The returned set of multicast group numbers must be used
 *                  in step-sized increments:
 *                                                                      \lb\lb
 *                  baseMcastGroup + (N * step)
 *                                                                      \lb\lb
 *                  where N can range from 0 to (numMcastGroups - 1).
 *                                                                      \lb\lb
 *                  The returned base multicast group number might not be the 
 *                  same on different switches.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to reserve for stacked 
 *                  multicast groups.
 *
 * \param[in]       glortCount is the number of glorts to use for stacked 
 *                  multicast groups. This value must be a power of two.
 *
 * \param[out]      baseMcastGroup points to caller-allocated storage 
 *                  where this function should place the base multicast group
 *                  number.
 *
 * \param[out]      numMcastGroups points to caller-allocated storage 
 *                  where this function should place the number of multicast
 *                  group numbers allocated for the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage 
 *                  where this function should place the multicast group
 *                  number step size. When a stacked multicast group is created
 *                  with a call to ''fmCreateStackMcastGroup'', the multicast
 *                  group number passed to that function must be of the form
 *                  baseMcastGroup + (N * step) where N ranges from 0 to
 *                  (numMcastGroups - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if baseMcastGroup, numMcastGroups,
 *                  or step is NULL or any input parameter is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort in the given
 *                  glort range is being used.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more multicast group
 *                  resoures are available.
 *
 *****************************************************************************/
fm_status fmAllocateStackMcastGroups(fm_int     sw,
                                     fm_uint    startGlort,
                                     fm_uint    glortCount,
                                     fm_int    *baseMcastGroup,
                                     fm_int    *numMcastGroups,
                                     fm_int    *step)
{
    fm_status    err;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, startGlort = 0x%x, glortCount = %u, "
                     "baseMcastGroup = %p, numMcastGroups = %p, step = %p\n",
                     sw,
                     startGlort,
                     glortCount,
                     (void *) baseMcastGroup,
                     (void *) numMcastGroups,
                     (void *) step);

    if ( !baseMcastGroup ||  !numMcastGroups || !step)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAllocateMcastGroupsInt(sw,
                                   startGlort,
                                   glortCount,
                                   baseMcastGroup,
                                   numMcastGroups,
                                   step);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmAllocateStackMcastGroups */



/*****************************************************************************/
/** fmFreeStackMcastGroups
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Free stacked multicast group numbers previously allocated 
 *                  with ''fmAllocateStackMcastGroups''.
 *
 * \note            All stacked multicast groups associated with the allocated 
 *                  multicast group numbers must be deleted with a call to
 *                  ''fmDeleteMcastGroup'' before the stacked multicast group 
 *                  numbers can be freed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseMcastGroup is the base multicast group number 
 *                  previously returned by ''fmAllocateStackMcastGroups''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if baseMcastGroup is not 
 *                  found.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeStackMcastGroups(fm_int    sw,
                                 fm_int    baseMcastGroup)
{
    fm_status   err;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, baseMcastGroup = %d\n",
                     sw,
                     baseMcastGroup);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    err = fmFreeMcastGroupsInt(sw, baseMcastGroup);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmFreeStackMcastGroups */




/*****************************************************************************/
/** fmCreateStackMcastGroup
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Create a multicast group for use in a stacking 
 *                  configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group (handle) of the
 *                  desired multicast group.  Note that mcastGroup must be 
 *                  from the set preallocated by a call to 
 *                  ''fmAllocateStackMcastGroups''.  To create a multicast
 *                  group without a logical port, call
 *                  ''fmCreateStackMcastGroupExt'' instead, using
 *                  FM_HANDLE_NONE as the requested handle.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 * \return          FM_ERR_LOG_PORT_REQUIRED if mcastGroup is equal to
 *                  FM_LOGICAL_PORT_NONE.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more multicast group
 *                  resoures are available.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmCreateStackMcastGroup(fm_int sw, 
                                  fm_int mcastGroup)
{
    fm_status   err;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    if (mcastGroup == FM_LOGICAL_PORT_NONE)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_LOG_PORT_REQUIRED);
    }

    err = fmCreateMcastGroupInt(sw, &mcastGroup, TRUE, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackMcastGroup */




/*****************************************************************************/
/** fmCreateStackMcastGroupExt
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Create a multicast group for use in a stacking 
 *                  configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       requestedHandle is the multicast group handle identifying
 *                  the desired multicast group.  Note that mcastGroup must be 
 *                  from the set preallocated by a call to 
 *                  ''fmAllocateStackMcastGroups''.
 *                  To create a multicast group without using an existing
 *                  multicast group and its associated logical port/glort,
 *                  e.g., for RPF failure groups, specify FM_HANDLE_NONE
 *                  for the requested handle.  FM_LOGICAL_PORT_NONE may
 *                  also be used for this purpose, but this value has been
 *                  deprecated in favor of FM_HANDLE_NONE. (When creating an 
 *                  RPF failure group, set the ''FM_MCASTGROUP_ACTION'' 
 *                  multicast group attribute to the appropriate value.)
 *
 * \param[out]      mcastGroup points to caller-allocated memory into which
 *                  the multicast group's handle will be written.  If
 *                  requestedHandle was set to a valid handle, mcastGroup is
 *                  guaranteed to be equal to requestedHandle.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more multicast group
 *                  resoures are available.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmCreateStackMcastGroupExt(fm_int  sw,
                                     fm_int  requestedHandle,
                                     fm_int *mcastGroup)
{
    fm_status   err;
 
    FM_LOG_ENTRY_API( FM_LOG_CAT_STACKING,
                     "sw = %d, requestedHandle = %d, mcastGroup = %p\n",
                     sw,
                     requestedHandle,
                     (void *) mcastGroup );

    *mcastGroup = requestedHandle;

    err = fmCreateMcastGroupInt(sw, mcastGroup, TRUE, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackMcastGroup */




/*****************************************************************************/
/** fmAllocateStackLAGs
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Allocate a set of stacked link aggregation group numbers 
 *                  given a glort range. The function returns the base LAG
 *                  number and the number of LAG numbers allocated. 
 *                  The returned set of LAG numbers can then
 *                  be used in subsequent calls to ''fmCreateStackLAG''.
 *
 * \note            The returned set of LAG numbers must be used
 *                  in step-sized increments:
 *                                                                      \lb\lb
 *                  baseLagNumber + (N * step)
 *                                                                      \lb\lb
 *                  where N can range from 0 to (numLags - 1).
 *                                                                      \lb\lb
 *                  The returned base LAG number might not be the 
 *                  same on different switches.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to reserve for stacked 
 *                  LAGs.
 *
 * \param[in]       glortCount is the number of glorts to use for stacked 
 *                  LAGs. This value must be a power of two.
 *
 * \param[out]      baseLagNumber points to caller-allocated storage 
 *                  where this function should place the base LAG number.
 *
 * \param[out]      numLags points to caller-allocated storage 
 *                  where this function should place the number of LAG numbers
 *                  allocated for the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage where this 
 *                  function should place the LAG number step size. When a 
 *                  stacked LAG is created with a call to ''fmCreateStackLAG'', 
 *                  the LAG number passed to that function must be of the form
 *                  baseLagNumber + (N * step) where N ranges from 0 to
 *                  (numLags - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if baseLagNumber, numLags, or 
 *                  step is NULL or any input parameter is invalid.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  in the given glort range are being used.
 * \return          FM_ERR_NO_LAG_RESOURCES if no more LAG resources are 
 *                  available.
 *
 *****************************************************************************/
fm_status fmAllocateStackLAGs(fm_int     sw,
                              fm_uint    startGlort,
                              fm_uint    glortCount,
                              fm_int    *baseLagNumber,
                              fm_int    *numLags,
                              fm_int    *step)
{
    fm_status     err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, startGlort = 0x%x, glortCount = %u, "
                     "baseLagNumber = %p, numLags = %p, step = %p\n",
                     sw,
                     startGlort,
                     glortCount,
                     (void *) baseLagNumber,
                     (void *) numLags,
                     (void *) step);
    if ( !baseLagNumber ||  !numLags || ! step)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAllocateLAGsInt(sw,
                            startGlort,
                            glortCount,
                            TRUE,
                            baseLagNumber,
                            numLags,
                            step);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmAllocateStackLAGs */



/*****************************************************************************/
/** fmFreeStackLAGs
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Free stacked link aggregation group numbers previously
 *                  allocated with ''fmAllocateStackLAGs''.
 *
 * \note            All stacked LAGs associated with the allocated LAG
 *                  numbers must be deleted with a call to ''fmDeleteLAGExt''
 *                  before the stacked LAG numbers can be freed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLagNumber is the base LAG number previously returned
 *                  by ''fmAllocateStackLAGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if baseLagNumber is not found.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeStackLAGs(fm_int    sw,
                          fm_int    baseLagNumber)
{
    fm_status     err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, baseLagNumber = %d\n",
                     sw,
                     baseLagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmFreeLAGsInt(sw, baseLagNumber);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);


}   /* end fmFreeStackLAGs */




/*****************************************************************************/
/** fmCreateStackLAG
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a link aggregation group for use in a
 *                  stacking configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagNumber is the LAG number (handle) of the
 *                  desired LAG. Note that lagNumber must be from the set
 *                  preallocated by a call to ''fmAllocateStackLAGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_LAG if max LAGs (''FM_MAX_NUM_LAGS'') already
 *                  created.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lag structure.
 *
 *****************************************************************************/
fm_status fmCreateStackLAG(fm_int sw, fm_int lagNumber)
{
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, lagNumber = %d\n",
                     sw,
                     lagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmCreateLAGInt(sw, &lagNumber, TRUE);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackLAG */




/*****************************************************************************/
/** fmAllocateStackLBGs
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Allocate a set of stacked load balancing group numbers 
 *                  given a glort range. The function returns the base LBG
 *                  number (the first logical port reserved for the LBGs)
 *                  and the number of LBG numbers allocated. 
 *                  The returned set of LBG numbers can then
 *                  be used in subsequent calls to ''fmCreateStackLBG''.
 *
 * \note            The returned set of LBG numbers must be used
 *                  in step-sized increments:
 *                                                                      \lb\lb
 *                  baseLbgNumber + (N * step)
 *                                                                      \lb\lb
 *                  where N can range from 0 to (numLbgs - 1).
 *                                                                      \lb\lb
 *                  The returned base LBG number might not be the 
 *                  same on different switches.
 *                                                                           \lb
 *                  Resources from this allocation is reserved based on the current
 *                  LBG mode. If the caller allocates the LBGs with one LBG mode and
 *                  then uses the LBG in another mode, either resources is under-
 *                  utilized or insufficient resources is available thus causing
 *                  an error to return.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to reserve for stacked 
 *                  LBGs.
 *
 * \param[in]       glortCount is the number of glorts to use for stacked 
 *                  LBGs. This value must be a power of two.
 *
 * \param[out]      baseLbgNumber points to caller-allocated storage 
 *                  where this function should place the base LBG number.
 *                  The base LBG number represents the first logical port
 *                  reserved for the LBGs.
 *
 * \param[out]      numLbgs points to caller-allocated storage 
 *                  where this function should place the number of LBG numbers
 *                  allocated for the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage where this 
 *                  function should place the LBG number step size. When a 
 *                  stacked LBG is created with a call to ''fmCreateStackLBG'', 
 *                  the LBG number passed to that function must be of the form
 *                  baseLbgNumber + (N * step) where N ranges from 0 to
 *                  (numLbgs - 1).
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if baseLbgNumber, numLbgs, or 
 *                  step is NULL or any input parameter is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  LBG data structures.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  in the given glort range are being used.
 * \return          FM_ERR_NO_LBG_RESOURCES if no more LBG resoures are 
 *                  available.
 *
 *****************************************************************************/
fm_status fmAllocateStackLBGs(fm_int     sw,
                              fm_uint    startGlort,
                              fm_uint    glortCount,
                              fm_int    *baseLbgNumber,
                              fm_int    *numLbgs,
                              fm_int    *step)
{
    fm_status    err;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, startGlort = 0x%x, glortCount = %u, "
                     "baseLbgNumber = %p, numLbgs = %p, step = %p\n",
                     sw,
                     startGlort,
                     glortCount,
                     (void *) baseLbgNumber,
                     (void *) numLbgs,
                     (void *) step);

    if ( !baseLbgNumber ||  !numLbgs || !step)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    err = fmAllocateLBGsInt(sw,
                            startGlort,
                            glortCount,
                            baseLbgNumber,
                            numLbgs,
                            step);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmAllocateStackLBGs */



/*****************************************************************************/
/** fmFreeStackLBGs
 * \ingroup stacking 
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Free stacked load balancing group numbers previously 
 *                  allocated with ''fmAllocateStackLBGs''.
 *
 * \note            All stacked LBGs associated with the allocated LBG
 *                  numbers must be deleted with a call to ''fmDeleteLBG''
 *                  before the stacked LBG numbers can be freed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLbgNumber is the base LBG number previously returned
 *                  by ''fmAllocateStackLBGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LBG if baseLbgNumber is not found.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeStackLBGs(fm_int    sw,
                          fm_int    baseLbgNumber)
{
    fm_status   err;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, baseLbgNumber = %d\n",
                     sw,
                     baseLbgNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    FM_TAKE_LBG_LOCK(sw);

    err = fmFreeLBGsInt(sw, baseLbgNumber);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmFreeStackLBGs */



/*****************************************************************************/
/** fmCreateStackLBG
 * \ingroup stacking
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Create a load balancing group for use in a
 *                  stacking configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the LBG number (handle) of the desired LBG.  
 *                  Note that lbgNumber must be from the set preallocated by 
 *                  a call to ''fmAllocateStackLBGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lbg structure.
 *
 *****************************************************************************/
fm_status fmCreateStackLBG(fm_int sw, fm_int lbgNumber)
{
    fm_status    err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, lbgNumber = %d\n",
                     sw,
                     lbgNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    err = fmCreateLBGInt(sw, &lbgNumber, NULL, TRUE);

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackLBG */




/*****************************************************************************/
/** fmCreateStackLBGExt
 * \ingroup stacking
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Create a load balancing group for use in a stacking 
 *                  configuration.  This function is similar to 
 *                  ''fmCreateStackLBG'', but provides additional parameters
 *                  for the configuration of the resulting LBG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the LBG number (logical port) of the desired LBG.
 *                  Note that lbgNumber must be from the set preallocated by
 *                  a call to ''fmAllocateStackLBGs''.
 *
 * \param[in]       params points to the LBG parameter structure, used for
 *                  certain LBG modes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lbg structure.
 *
 *****************************************************************************/
fm_status fmCreateStackLBGExt(fm_int sw, 
                              fm_int lbgNumber, 
                              fm_LBGParams *params)
{
    fm_status    err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, lbgNumber = %d, params = %p\n",
                     sw,
                     lbgNumber,
                     (void *) params);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    err = fmCreateLBGInt(sw, &lbgNumber, params, TRUE);

    FM_DROP_LBG_LOCK(sw);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmCreateStackLBGExt */



/*****************************************************************************/
/** fmGetStackLBGHandle
 * \ingroup stacking
 *
 * \chips           FM10000
 *
 * \desc            Returns lbgHandle of the logical port belonging to a 
 *                  load-balancing group of mode ''FM_LBG_MODE_MAPPED_L234HASH''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port of the desired LBG. It is
 *                  same as the lbgNumber used in ''fmCreateStackLBGExt''.
 *
 * \param[out]      lbgHandle is the handle associated with the LBG logical 
 *                  port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lbg structure.
 *
 *****************************************************************************/
fm_status fmGetStackLBGHandle(fm_int  sw,
                              fm_int  lbgNumber,
                              fm_int *lbgHandle)
{
    fm_status    err = FM_OK;
    fm_port     *lbgPort;

    FM_LOG_ENTRY_API(FM_LOG_CAT_STACKING,
                     "sw = %d, lbgNumber = %d, lbgHandle = %p\n",
                     sw,
                     lbgNumber,
                     (void *) lbgHandle);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_LBG_LOCK(sw);

    lbgPort = GET_PORT_PTR(sw, lbgNumber);
    if (lbgPort != NULL)
    {
        *lbgHandle = lbgPort->lbgHandle;
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }

    FM_DROP_LBG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_STACKING, err);

}   /* end fmGetStackLBGHandle */

