/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_acl.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with ACLs
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


/*****************************************************************************/
/** fmExtendedActionToLegacyAction
 * \ingroup intAcl
 *
 * \desc            Convert an extended action to a legacy action.
 *
 * \param[in]       in is the extended action to convert.
 *
 * \param[in]       cond is the condition associated with the action, which
 *                  is needed to disambiguate some extended actions.
 *
 * \param[out]      out points to caller-allocated storage where this function
 *                  is to place the legacy action.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if there is no legacy action
 *                  corresponding to the extended action.
 *
 *****************************************************************************/
static fm_status fmExtendedActionToLegacyAction(fm_aclActionExt in,
                                                fm_aclCondition cond,
                                                fm_aclAction *  out)
{
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "in = %llu, cond = %lld, out = %p\n",
                 in,
                 cond,
                 (void *) out);

    if (in == FM_ACL_ACTIONEXT_PERMIT && cond == 0)
    {
        *out = FM_ACL_ACTION_PERMIT_ALL;
    }
    else if (in == FM_ACL_ACTIONEXT_PERMIT)
    {
        *out = FM_ACL_ACTION_PERMIT;
    }
    else if (in == FM_ACL_ACTIONEXT_DENY && cond == 0)
    {
        *out = FM_ACL_ACTION_DENY_ALL;
    }
    else if (in == FM_ACL_ACTIONEXT_DENY)
    {
        *out = FM_ACL_ACTION_DENY;
    }
    else if (in == FM_ACL_ACTIONEXT_TRAP)
    {
        *out = FM_ACL_ACTION_TRAP;
    }
    else if (in == FM_ACL_ACTIONEXT_COUNT)
    {
        *out = FM_ACL_ACTION_COUNT;
    }
    else if (in == FM_ACL_ACTIONEXT_MIRROR)
    {
        *out = FM_ACL_ACTION_MONITOR;
    }
    else
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_UNSUPPORTED);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmExtendedActionToLegacyAction */




/*****************************************************************************/
/** fmCleanupACLRules
 * \ingroup intAcl
 *
 * \desc            Do resource cleanup when ACL rules are deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port the rule is being removed from, or
 *                  -1 for all ports
 *
 * \param[in]       aclEntry is the set of rules associated with the port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmCleanupACLRules(fm_int sw, fm_int port, fm_acl *aclEntry)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw = %d, port = %d, aclEntry = %p\n",
                 sw,
                 port,
                 (void *) aclEntry);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->CleanupACLRule != NULL)
    {
        for (fmTreeIterInit(&it, &aclEntry->rules) ;
             err == FM_OK &&
             ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
        {
            err = switchPtr->CleanupACLRule(sw, port, (fm_aclRule *) nextValue);
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        /* this means the iteration has ended normally */
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmCleanupACLRules */




/*****************************************************************************/
/** FreeAcl
 * \ingroup intAcl
 *
 * \desc            Deallocate an ACL object.
 *
 * \param[in]       aclEntry points to the ACL object to free.
 *
 * \return          None.
 *
 *****************************************************************************/
static void FreeAcl(fm_acl *aclEntry)
{
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    for (fmTreeIterInit(&it, &aclEntry->rules) ;
         (fmTreeIterNext(&it, &nextKey, &nextValue)) == FM_OK ; )
    {
        fmDeleteBitArray(&(((fm_aclRule *) nextValue)->associatedPorts));
    }

    fmTreeDestroy(&aclEntry->rules, fmFree);
    fmTreeDestroy(&aclEntry->addedRules, NULL);
    fmTreeDestroy(&aclEntry->removedRules, NULL);
    fmDeleteBitArray(&aclEntry->associatedPorts);
    fmFree(aclEntry);

}   /* end FreeAcl */




/*****************************************************************************/
/** VoidFreeAcl
 * \ingroup intAcl
 *
 * \desc            Calls ''FreeAcl'' for an ACL object identified by an
 *                  opaque pointer.
 *
 * \param[in]       ptr points to the ACL object.
 *
 * \return          None.
 *
 *****************************************************************************/
static void VoidFreeAcl(void *ptr)
{
    FreeAcl( (fm_acl *) ptr );

}   /* end VoidFreeAcl */




/*****************************************************************************/
/** AllocateAcl
 * \ingroup intAcl
 *
 * \desc            Allocate an ACL object.
 *
 * \param[in]       maxPorts is the maximum number of ports that the ACL
 *                  can be associated with.
 *
 * \return          Pointer to the allocated ACL object if successful.
 * \return          NULL if no memory available for the ACL object.
 *
 *****************************************************************************/
static fm_acl *AllocateAcl(fm_int maxPorts)
{
    fm_acl *aclEntry = (fm_acl *) fmAlloc( sizeof(fm_acl) );

    if (aclEntry != NULL)
    {
        FM_CLEAR(*aclEntry);
        fmTreeInit(&aclEntry->rules);
        fmTreeInit(&aclEntry->addedRules);
        fmTreeInit(&aclEntry->removedRules);

        if (fmCreateBitArray(&aclEntry->associatedPorts,
                             maxPorts * FM_ACL_TYPE_MAX) != FM_OK)
        {
            fmFree(aclEntry);
            aclEntry = NULL;
        }
    }

    return aclEntry;

}   /* end AllocateAcl */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCreateACL
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create an ACL. The ACL will be created with an empty
 *                  rule list. The ACL will be assigned the default precedence
 *                  of ''FM_ACL_DEFAULT_PRECEDENCE''.
 *                  The ACL applies to all scenarios.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl already created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmCreateACL(fm_int sw, fm_int acl)
{
    fm_status       err;
    const fm_uint32 always = (FM_ACL_SCENARIO_ANY_FRAME_TYPE |
                              FM_ACL_SCENARIO_ANY_ROUTING_TYPE |
                              FM_ACL_SCENARIO_ANY_TUNNEL_TYPE);

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d\n",
                     sw,
                     acl);

    err = fmCreateACLExt(sw, acl, always, FM_ACL_DEFAULT_PRECEDENCE);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmCreateACL */




/*****************************************************************************/
/** fmCreateACLExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create an ACL. The ACL will be created with an empty rule
 *                  list. The ACL will be assigned the specified precedence.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       scenarios is a bitmask indicating when this ACL is valid
 *                  (see ''ACL Scenario Masks'').  For the FM2000 family, this 
 *                  argument is ignored or use ''fmCreateACL'' instead.
 *
 * \param[in]       precedence is the ACL precedence level (0 to
 *                  ''FM_MAX_ACL_PRECEDENCE''). This argument is only used
 *                  for the FM4000 family.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl already created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_PRECEDENCE if precedence is out of range.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid scenario
 *
 *****************************************************************************/
fm_status fmCreateACLExt(fm_int    sw,
                         fm_int    acl,
                         fm_uint32 scenarios,
                         fm_int    precedence)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, scenarios = %u, precedence = %u\n",
                     sw,
                     acl,
                     scenarios,
                     precedence);

    err = fmCreateACLInt(sw, acl, scenarios, precedence, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmCreateACLExt */




/*****************************************************************************/
/** fmCreateACLInt
 * \ingroup intAcl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create an ACL. The ACL will be created with an empty rule
 *                  list. The ACL will be assigned the specified precedence.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       scenarios is a bitmask indicating when this ACL is valid
 *                  (see ''ACL Scenario Masks'').  For the FM2000 family, this 
 *                  argument is ignored or use ''fmCreateACL'' instead.
 *
 * \param[in]       precedence is the ACL precedence level (0 to
 *                  ''FM_MAX_ACL_PRECEDENCE''). This argument is only used
 *                  for the FM4000 family.
 * 
 * \param[in]       internal is a flag defining if the ACL was managed by one
 *                  of the ACL subsystem (VN, NAT, Flow,...) or if this ACL
 *                  is created by the application. Internal ACLs are not
 *                  processed by typical Non Disruptive Compile/Apply sequence
 *                  and must be compiled and appplied individually using
 *                  FM_ACL_COMPILE_FLAG_INTERNAL and FM_ACL_APPLY_FLAG_INTERNAL
 *                  Compile/Apply flags.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl already created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_PRECEDENCE if precedence is out of range.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid scenario
 *
 *****************************************************************************/
fm_status fmCreateACLInt(fm_int    sw,
                         fm_int    acl,
                         fm_uint32 scenarios,
                         fm_int    precedence,
                         fm_bool   internal)
{
    fm_acl *   aclEntry;
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_bool    aclAllocated;
    fm_bool    treeInserted;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, acl = %d, scenarios = %u, "
                 "precedence = %u, internal = %d\n",
                 sw,
                 acl,
                 scenarios,
                 precedence,
                 internal);

    aclAllocated = FALSE;
    aclEntry = NULL;
    treeInserted = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    /************************************************************************* 
     * a scenario must have at least one of the frame type bits set and at
     * least one of the routing type bits set to be valid.
     * Tunneling type bits are not required and will default to no tunnels.
     *************************************************************************/
    if ( (scenarios & FM_ACL_SCENARIO_ANY_FRAME_TYPE) == 0 ||
         ( (scenarios & FM_ACL_SCENARIO_ANY_ROUTING_TYPE) == 0  &&
           (scenarios & FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE) == 0 &&
           (scenarios & FM_ACL_SCENARIO_VNTAG) == 0 ) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    aclEntry = AllocateAcl(switchPtr->numCardinalPorts);

    if (aclEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclAllocated = TRUE;

    /* set scenarios and precedence */
    aclEntry->scenarios  = scenarios;
    aclEntry->precedence = precedence;
    aclEntry->internal = internal;

    /* Enable top key by default */
    if (switchPtr->switchFamily == FM_SWITCH_FAMILY_FM6000)
    {
        aclEntry->aclTopKeySupport = FM_ENABLED;
    }

    aclEntry->instance = FM_ACL_NO_INSTANCE;

    err = fmTreeInsert(&switchPtr->aclInfo.acls, acl, aclEntry);

    if (err == FM_ERR_ALREADY_EXISTS)
    {
        err = FM_ERR_INVALID_ACL;
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    treeInserted = TRUE;

    if (switchPtr->CreateACL != NULL)
    {
        /* for SWAG */
        err = switchPtr->CreateACL(sw, acl, scenarios, precedence);

        /* Try to return a clean state by removing the ACL from switches that
         * actually created it (if some). Sorting order of creation and deletion
         * should be the same. */
        if (err != FM_OK)
        {
            switchPtr->DeleteACL(sw, acl);
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    if (err != FM_OK)
    {
        if (treeInserted)
        {
            fmTreeRemoveCertain(&switchPtr->aclInfo.acls, acl, NULL);
        }

        if (aclAllocated)
        {
            FreeAcl(aclEntry);
        }
    }

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmCreateACLInt */




/*****************************************************************************/
/** fmDeleteACL
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete an ACL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl not created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmDeleteACL(fm_int sw, fm_int acl)
{
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, "sw = %d, acl = %d\n", sw, acl);

    err = fmDeleteACLInt(sw, acl, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteACL */




/*****************************************************************************/
/** fmDeleteACLInt
 * \ingroup intAcl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete an ACL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 * 
 * \param[in]       internal refer to the ACL internal flag. This is used for
 *                  validation.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl not created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmDeleteACLInt(fm_int sw, fm_int acl, fm_bool internal)
{
    fm_status  err;
    fm_acl *   aclEntry;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, acl = %d, internal = %d\n",
                 sw, acl, internal);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (aclEntry->internal != internal)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->DeleteACL != NULL)
    {
        /* for SWAG (should never fail) */
        err = switchPtr->DeleteACL(sw, acl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeRemove(&switchPtr->aclInfo.acls, acl, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* for FM2000 */
    err = fmCleanupACLRules(sw, -1, aclEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    FreeAcl(aclEntry);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteACLInt */




/*****************************************************************************/
/** fmSetACLAttribute
 * \ingroup acl
 * 
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Set an ACL attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       attr is the ACL attribute (see 'ACL Attributes') to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl not created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fmSetACLAttribute(fm_int sw, fm_int acl, fm_int attr, void *value)
{
    fm_status  err;
    fm_acl *   aclEntry;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, attr = %d, value = %p\n",
                     sw,
                     acl,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ValidateACLAttribute != NULL)
    {
        err = switchPtr->ValidateACLAttribute(sw, attr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->SetACLAttribute != NULL)
    {
        /* for SWAG (should never fail since the attribute was validated) */
        err = switchPtr->SetACLAttribute(sw, acl, attr, value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (attr == FM_ACL_MODE)
    {
        aclEntry->aclMode = *( (fm_int *) value );
    }
    else if (attr == FM_ACL_INCR_SUP_COND)
    {
        aclEntry->aclIncremental.supportedCondition = *( (fm_aclCondition *) value );
    }
    else if (attr == FM_ACL_INCR_SUP_COND_MASK)
    {
        FM_MEMCPY_S(&aclEntry->aclIncremental.supportedConditionMask,
                    sizeof(aclEntry->aclIncremental.supportedConditionMask),
                    value,
                    sizeof(fm_aclValue));
    }
    else if (attr == FM_ACL_INCR_NUM_ACTIONS)
    {
        aclEntry->aclIncremental.numAction = *( (fm_uint32 *) value );
    }
    else if (attr == FM_ACL_TABLE_SELECTION)
    {
        aclEntry->aclTable = *( (fm_aclTable *) value );
    }
    else if (attr == FM_ACL_KEEP_UNUSED_KEYS)
    {
        aclEntry->aclKeepUnusedKeys = *( (fm_bool *) value );
    }
    else if (attr == FM_ACL_TOP_KEY_SUPPORT)
    {
        aclEntry->aclTopKeySupport = *( (fm_bool *) value );
    }
    else if (attr == FM_ACL_INSTANCE)
    {
        aclEntry->instance = *( (fm_int *) value );
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmSetACLAttribute */




/*****************************************************************************/
/** fmGetACLAttribute
 * \ingroup acl
 * 
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Get an ACL attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       attr is the ACL attribute (see 'ACL Attributes') to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl not created.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fmGetACLAttribute(fm_int sw, fm_int acl, fm_int attr, void *value)
{
    fm_status  err;
    fm_acl *   aclEntry;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, attr = %d, value = %p\n",
                     sw,
                     acl,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ValidateACLAttribute != NULL)
    {
        err = switchPtr->ValidateACLAttribute(sw, attr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (attr == FM_ACL_MODE)
    {
        *( (fm_int *) value ) = aclEntry->aclMode;
    }
    else if (attr == FM_ACL_INCR_SUP_COND)
    {
        *( (fm_aclCondition *) value ) = aclEntry->aclIncremental.supportedCondition;
    }
    else if (attr == FM_ACL_INCR_SUP_COND_MASK)
    {
        FM_MEMCPY_S(value,
                    sizeof(fm_aclValue),
                    &aclEntry->aclIncremental.supportedConditionMask,
                    sizeof(aclEntry->aclIncremental.supportedConditionMask));
    }
    else if (attr == FM_ACL_INCR_NUM_ACTIONS)
    {
        *( (fm_uint32 *) value ) = aclEntry->aclIncremental.numAction;
    }
    else if (attr == FM_ACL_TABLE_SELECTION)
    {
        *( (fm_aclTable *) value ) = aclEntry->aclTable;
    }
    else if (attr == FM_ACL_KEEP_UNUSED_KEYS)
    {
        *( (fm_bool *) value ) = aclEntry->aclKeepUnusedKeys;
    }
    else if (attr == FM_ACL_TOP_KEY_SUPPORT)
    {
        *( (fm_bool *) value ) = aclEntry->aclTopKeySupport;
    }
    else if (attr == FM_ACL_SLICE_USAGE)
    {
        /* No SWAG support */
        FM_API_CALL_FAMILY(err,
                           switchPtr->GetAclSliceUsage,
                           sw,
                           acl,
                           (fm_aclSliceUsage *) value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    else if (attr == FM_ACL_INSTANCE)
    {
        *( (fm_int *) value ) = aclEntry->instance;
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLAttribute */




/*****************************************************************************/
/** fmGetACLRuleAttribute
 * \ingroup acl
 * 
 * \chips           FM6000, FM10000
 *
 * \desc            Get an attribute of a specific ACL rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 * 
 * \param[in]       attr is the ACL Rule attribute to get
 *                  (see ''ACL Rule Attributes'').
 *
 * \param[out]      value points to caller-supplied storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no matching ACLs found.
 * \return          FM_ERR_NOT_FOUND if ACL rule was not found.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid attribute.
 *
 *****************************************************************************/
fm_status fmGetACLRuleAttribute(fm_int sw,
                                fm_int acl,
                                fm_int rule,
                                fm_int attr,
                                void * value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, "
                     "attr = %d, value = %p\n",
                     sw,
                     acl,
                     rule,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* No SWAG Support for now */
    FM_API_CALL_FAMILY(err,
                       switchPtr->GetACLRuleAttribute,
                       sw,
                       acl,
                       rule,
                       attr,
                       value);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleAttribute */




/*****************************************************************************/
/** fmAddACLRule
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a rule with the given condition, value and action to
 *                  an ACL.
 *
 * \note            Use of ''fmAddACLRuleExt'' is recommended over this
 *                  function, because it allows more than one action per rule 
 *                  and supports additional types of actions.  
 *                  fmAddACLRule exists for backwards compatibility.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number. The rule number serves as the 
 *                  precedence when multiple rules hit on a frame. The lower 
 *                  the rule number, the higher the precedence. On devices
 *                  other than the FM2000, higher number rows (rules) in the 
 *                  FFU slice (ACL) have higher precedence. When rules are 
 *                  written to the hardware, they are written in reverse rule 
 *                  number order.
 *
 * \param[in]       cond is a condition (see 'fm_aclCondition').
 *
 * \param[in]       value points to the ''fm_aclValue'' structure
 *                  to match against for the given condition.
 *
 * \param[in]       action is one of the valid actions (see 'fm_aclAction').
 *
 * \param[in]       param is a parameter associated with the
 *                  ''FM_ACL_ACTION_MONITOR'' action. Its value is the port
 *                  number to which monitored traffic should be sent.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmAddACLRule(fm_int             sw,
                       fm_int             acl,
                       fm_int             rule,
                       fm_aclCondition    cond,
                       const fm_aclValue *value,
                       fm_aclAction       action,
                       fm_aclParam        param)
{
    fm_aclCondition newCond = cond;
    fm_aclActionExt newAction;
    fm_aclParamExt  newParam;
    fm_status       err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw=%d acl=%d rule=%d cond=%lld value=%p action=%d param=%"
                     FM_FORMAT_64 "X\n",
                     sw,
                     acl,
                     rule,
                     cond,
                     (void *) value,
                     action,
                     param);

    FM_CLEAR(newParam);

    switch (action)
    {
        case FM_ACL_ACTION_PERMIT:
            newAction = FM_ACL_ACTIONEXT_PERMIT;
            break;

        case FM_ACL_ACTION_PERMIT_ALL:
            newAction = FM_ACL_ACTIONEXT_PERMIT;
            newCond   = 0;
            break;

        case FM_ACL_ACTION_DENY:
            newAction = FM_ACL_ACTIONEXT_DENY;
            break;

        case FM_ACL_ACTION_DENY_ALL:
            newAction = FM_ACL_ACTIONEXT_DENY;
            newCond   = 0;
            break;

        case FM_ACL_ACTION_TRAP:
            newAction = FM_ACL_ACTIONEXT_TRAP;
            break;

        case FM_ACL_ACTION_COUNT:
            newAction = FM_ACL_ACTIONEXT_COUNT;
            break;

        case FM_ACL_ACTION_MONITOR:
            newAction = FM_ACL_ACTIONEXT_MIRROR;
            break;

        default:
            FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);

    }   /* end switch (action) */

    newParam.mirrorPort = (fm_uint16) param;

    err = fmAddACLRuleExt(sw,
                          acl,
                          rule,
                          newCond,
                          value,
                          newAction,
                          &newParam);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmAddACLRule */




/*****************************************************************************/
/** fmAddACLRuleExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a rule with the given condition, value, action and
 *                  option to an ACL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number. The rule number serves as the 
 *                  precedence when multiple rules hit on a frame. The lower 
 *                  the rule number, the higher the precedence. On devices
 *                  other than the FM2000, higher number rows (rules) in the 
 *                  FFU slice (ACL) have higher precedence. When rules are 
 *                  written to the hardware, they are written in reverse rule 
 *                  number order.
 *
 * \param[in]       cond is a condition (see 'fm_aclCondition').
 *
 * \param[in]       value points to the fm_aclValue structure (see 'fm_aclValue')
 *                  to match against for the given condition.
 *
 * \param[in]       action is a mask of actions (see 'fm_aclActionExt').
 *
 * \param[in]       param is a parameter associated with the action (see
 *                  ''fm_aclParamExt'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmAddACLRuleExt(fm_int                sw,
                          fm_int                acl,
                          fm_int                rule,
                          fm_aclCondition       cond,
                          const fm_aclValue *   value,
                          fm_aclActionExt       action,
                          const fm_aclParamExt *param)
{
    fm_acl *        aclEntry;
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_aclRule *    aclRule;
    fm_portSet *    portSetEntry;
    fm_bool         portSetLockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, cond = %lld, value = %p, "
                     "action = %llu, param = %p\n",
                     sw,
                     acl,
                     rule,
                     cond,
                     (const void *) value,
                     action,
                     (const void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (aclEntry->aclMode == FM_ACL_MODE_INCREMENTAL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((value == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ValidateACLRule != NULL)
    {
        /* for FM2000 */
        err = switchPtr->ValidateACLRule(sw, cond, value, action, param);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) fmAlloc( sizeof(fm_aclRule) );

    if (aclRule == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_CLEAR(*aclRule);

    aclRule->cond   = cond;
    aclRule->value  = *value;
    aclRule->action = action;
    aclRule->param  = *param;
    aclRule->state = FM_ACL_RULE_ENTRY_STATE_VALID;

    err = fmTreeInsert(&aclEntry->rules, rule, aclRule);

    if ( (err == FM_OK) && (switchPtr->AddACLRule != NULL) )
    {
        /* for SWAG */
        err = switchPtr->AddACLRule(sw, acl, rule, cond, value, action, param);

        /* Try to return a clean state by removing the ACL rule from switches
         * that actually created it (if some). Sorting order of creation and 
         * deletion should be the same. */
        if (err != FM_OK)
        {
            switchPtr->DeleteACLRule(sw, acl, rule);
            fmTreeRemoveCertain(&aclEntry->rules, rule, NULL);
        }
    }

    /************************************************************************* 
     * For ingress port set condition, fill the ingress port bit array in the
     * rule stucture with the port set bit array point by the portSet value for
     * dynamic port set or filled ingress port bit array according to static
     * port set. 
     *************************************************************************/
    if ( (err == FM_OK) && (cond & FM_ACL_MATCH_INGRESS_PORT_SET) )
    {
        /* Protect the software state of portSetTree and its entries */
        TAKE_PORTSET_LOCK(sw);
        portSetLockTaken = TRUE;

        /* dynamic port set */
        if (fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                       value->portSet & FM_PORTSET_MASK,
                       (void**) &portSetEntry) == FM_OK)
        {
            if (portSetEntry)
            {
                err = fmCreateBitArray(&aclRule->associatedPorts,
                                       portSetEntry->associatedPorts.bitCount);
                if (err == FM_OK)
                {
                    fmCopyBitArray(&aclRule->associatedPorts,
                                   &portSetEntry->associatedPorts);
                }
                else
                {
                    fmTreeRemoveCertain(&aclEntry->rules, rule, NULL);
                }

            }   /* end if (portSetEntry) */

        }   /* end if (fmTreeFind(&switchPtr->portSetInfo.portSet, ...) */

        DROP_PORTSET_LOCK(sw);
        portSetLockTaken = FALSE;

    }   /* end if ( (err == FM_OK) && (cond & FM_ACL_MATCH_INGRESS_PORT_SET) ) */

    if (err != FM_OK)
    {
        fmFree(aclRule);

        if (err == FM_ERR_ALREADY_EXISTS)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }
    }
    else
    {
        fmTreeInsert(&aclEntry->addedRules, rule, NULL);
        fmTreeRemove(&aclEntry->removedRules, rule, NULL);
    }

ABORT:
    if (portSetLockTaken)
    {
        DROP_PORTSET_LOCK(sw);
    }

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmAddACLRuleExt */




/*****************************************************************************/
/** fmDeleteACLRule
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Remove a rule from an ACL.
 *
 * \note            This function was formerly known as ''fmRemoveACLRule''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmDeleteACLRule(fm_int sw, fm_int acl, fm_int rule)
{
    fm_acl *   aclEntry;
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    void *     value;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d\n",
                     sw,
                     acl,
                     rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (aclEntry->aclMode == FM_ACL_MODE_INCREMENTAL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&aclEntry->rules, rule, &value);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->DeleteACLRule != NULL)
    {
        /* for SWAG */
        err = switchPtr->DeleteACLRule(sw, acl, rule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeRemoveCertain(&aclEntry->rules, rule, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (switchPtr->CleanupACLRule != NULL)
    {
        /* for FM2000 */
        err = switchPtr->CleanupACLRule(sw, -1, (fm_aclRule *) value);
    }

    fmTreeInsert(&aclEntry->removedRules, rule, NULL);
    fmTreeRemove(&aclEntry->addedRules, rule, NULL);

    fmDeleteBitArray(&(((fm_aclRule *) value)->associatedPorts));
     
    fmFree(value);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteACLRule */




/*****************************************************************************/
/** fmUpdateACLRule
 * \ingroup acl
 * 
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Update an ACL rule with a new condition and action. The
 *                  rule must have been previously compiled with 
 *                  ''fmCompileACL'' and applied with ''fmApplyACL''.
 *
 * \note            When a rule is updated, it will momentarily "blink
 *                  out" while its configuration is changed. If a packet
 *                  that would normally match this rule, both before and
 *                  after the update, transits the device as the rule is 
 *                  being updated, the packet may be processed by the device
 *                  as if the rule did not exist at all. 
 * 
 * \note            On FM3000 and FM4000 devices, this function is only 
 *                  available for ACLs whose ''FM_ACL_MODE'' attribute is set 
 *                  to FM_ACL_MODE_INCREMENTAL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[in]       cond indicates the conditions on which the rule should
 *                  match (see ''fm_aclCondition'').
 *
 * \param[in]       value points to the ''fm_aclValue'' structure
 *                  to match against for the given condition.
 *
 * \param[in]       action is the set of actions to take upon a match (see 
 *                  ''fm_aclActionExt'').
 *
 * \param[in]       param is a parameter associated with the action (see
 *                  ''fm_aclParamExt'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if acl is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_UNSUPPORTED if acl is not set to incremental mode
 *                  (FM3000 and FM4000 only), or if one or more of the 
 *                  selected conditions in cond are not eligible for 
 *                  incremental updating or chip does not support this feature.
 * \return          FM_ERR_INVALID_ACL_IMAGE if a valid set of ACLs have not 
 *                  been previously successfully compiled with ''fmCompileACL''.
 *
 *****************************************************************************/
fm_status fmUpdateACLRule(fm_int                sw,
                          fm_int                acl,
                          fm_int                rule,
                          fm_aclCondition       cond,
                          const fm_aclValue *   value,
                          fm_aclActionExt       action,
                          const fm_aclParamExt *param)
{
    fm_acl *         aclEntry;
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_aclRule *     aclRule;
    void *           voidRule;
    fm_portSet *     portSetEntry;
    fm_bool          portSetLockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, cond = %lld, value = %p, "
                     "action = %llu, param = %p\n",
                     sw,
                     acl,
                     rule,
                     cond,
                     (const void *) value,
                     action,
                     (const void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * If any ACL rules are for load balancing, we
     * will need to take the LBG lock. But that lock
     * is lower precedence than the ACL lock, so we
     * must take it before taking the ACL lock.
     **************************************************/

    err = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    TAKE_LAG_LOCK(sw);
        
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((value == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeFind(&aclEntry->rules, rule, &voidRule);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = ( (fm_aclRule *) voidRule );

    if ( (cond & FM_ACL_MATCH_L4_SRC_PORT_RANGE) ||
         (cond & FM_ACL_MATCH_L4_DST_PORT_RANGE) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule->cond   = cond;
    aclRule->value  = *value;
    aclRule->action = action;
    aclRule->param  = *param;

    fmClearBitArray(&aclRule->associatedPorts);
    
    /************************************************************************* 
     * For ingress port set condition, fill the ingress port bit array in the
     * rule structure with the port set bit array point by the portSet value for
     * dynamic port set or filled ingress port bit array according to static
     * port set. 
     *************************************************************************/
     
    if ( cond & FM_ACL_MATCH_INGRESS_PORT_SET )
    {
        /* Protect the software state of portSetTree and its entries */
        TAKE_PORTSET_LOCK(sw);
        portSetLockTaken = TRUE;

        /* dynamic port set */
        err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                         value->portSet & FM_PORTSET_MASK,
                         (void**) &portSetEntry);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if ( portSetEntry &&
             (portSetEntry->associatedPorts.bitCount ==
              aclRule->associatedPorts.bitCount) )
        {
            fmCopyBitArray(&aclRule->associatedPorts,
                           &portSetEntry->associatedPorts);
        }

        DROP_PORTSET_LOCK(sw);
        portSetLockTaken = FALSE;
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateACLRule,
                       sw,
                       acl,
                       rule);

ABORT:
    if (portSetLockTaken)
    {
        DROP_PORTSET_LOCK(sw);
    }

    FM_DROP_ACL_LOCK(sw);
    DROP_LAG_LOCK(sw);
    fmReleaseWriteLock(&switchPtr->routingLock);
    fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmUpdateACLRule */




/*****************************************************************************/
/** fmSetACLRuleState
 * \ingroup acl
 * 
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Update the "valid" state of an ACL rule. Can be used
 *                  to turn on or off previously compiled and applied rules.
 *
 * \note            On FM3000 and FM4000 devices, this function is only 
 *                  available for ACLs whose ''FM_ACL_MODE'' attribute is set 
 *                  to FM_ACL_MODE_INCREMENTAL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[in]       state defines if this rule is valid or not.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if acl is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_UNSUPPORTED if acl is not set to incremental mode
 *                  (FM3000 and FM4000 only) or chip does not support this 
 *                  feature.
 * \return          FM_ERR_INVALID_ACL_IMAGE if a valid set of ACLs have not 
 *                  been previously successfully compiled with ''fmCompileACL''.
 *
 *****************************************************************************/
fm_status fmSetACLRuleState(fm_int           sw,
                            fm_int           acl,
                            fm_int           rule,
                            fm_aclEntryState state)
{
    fm_acl *         aclEntry;
    fm_status        err = FM_OK;
    fm_switch *      switchPtr;
    fm_aclRule *     aclRule;
    void *           voidRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, state = %d\n",
                     sw,
                     acl,
                     rule,
                     state);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&aclEntry->rules, rule, &voidRule);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = ( (fm_aclRule *) voidRule );

    aclRule->state = state;

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetACLRuleState,
                       sw,
                       acl,
                       rule);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmSetACLRuleState */




/*****************************************************************************/
/** fmGetACLRuleState
 * \ingroup acl
 * 
 * \chips           FM3000, FM4000, FM6000, FM10000
 * 
 * \desc            Get the state of a rule.
 * 
 * \note            On FM3000 and FM4000 devices, this function is only 
 *                  available for ACLs whose ''FM_ACL_MODE'' attribute is set 
 *                  to FM_ACL_MODE_INCREMENTAL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[out]      state points to caller-allocated storage in which this
 *                  function will place the rule state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if acl is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_UNSUPPORTED if acl is not set to incremental mode
 *                  (FM3000 AND FM4000 only).
 *
 *****************************************************************************/
fm_status fmGetACLRuleState(fm_int            sw,
                            fm_int            acl,
                            fm_int            rule,
                            fm_aclEntryState* state)
{
    fm_acl *         aclEntry;
    fm_status        err = FM_OK;
    fm_aclRule *     aclRule;
    void *           voidRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, state = %p\n",
                     sw,
                     acl,
                     rule,
                     (void *) state);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (state == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeFind(&aclEntry->rules, rule, &voidRule);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = ( (fm_aclRule *) voidRule );

    *state = aclRule->state;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleState */




/*****************************************************************************/
/** fmGetACLRuleFirst
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            DEPRECATED in favor of ''fmGetACLRuleFirstExt''. Get the 
 *                  first valid rule in an ACL. Report the rule's condition, 
 *                  value, action and parameter.
 *
 * \note            This function is not thread-safe, because it returns
 *                  value as a pointer to an internal data structure which
 *                  could change at any time.  Use ''fmGetACLRuleFirstExt''
 *                  instead.
 *
 * \note            This function was formerly known as ''fmGetFirstACLRule''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[out]      firstRule is a pointer to storage where the first rule
 *                  number is to be stored by this function.
 *
 * \param[out]      cond points to caller-allocated storage where this function
 *                  should store the first rule's condition (see 'fm_aclCondition').
 *
 * \param[out]      value is the address of a pointer to the fm_aclValue
 *                  structure (see 'fm_aclValue') that is used to match against
 *                  the given condition. This function will fill in the pointer
 *                  with the first rule's value.
 *
 * \param[out]      action points to caller-allocated storage where this function
 *                  should store the first rule's action (see 'fm_aclAction').
 *
 * \param[out]      param points to caller-allocated storage where this function
 *                  should store the first rule's parameter (see 'fm_aclParam').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_RULES_IN_ACL if acl has no rules.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmGetACLRuleFirst(fm_int           sw,
                            fm_int           acl,
                            fm_int *         firstRule,
                            fm_aclCondition *cond,
                            fm_aclValue **   value,
                            fm_aclAction *   action,
                            fm_aclParam *    param)
{
    fm_acl *        aclEntry;
    fm_status       err;
    fm_aclActionExt extendedAction;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_aclRule *    aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, firstRule = %p, cond = %p, value = %p, "
                     "action = %p, param = %p\n",
                     sw,
                     acl,
                     (void *) firstRule,
                     (void *) cond,
                     (void *) value,
                     (void *) action,
                     (void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((firstRule == NULL) || (cond == NULL) || (value == NULL) ||
        (action == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmTreeIterInit(&it, &aclEntry->rules);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_RULES_IN_ACL;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) nextValue;

    *firstRule     = (fm_int) nextKey;
    *cond          = aclRule->cond;
    *value         = &aclRule->value;
    extendedAction = aclRule->action;
    *param         = aclRule->param.mirrorPort;

    err = fmExtendedActionToLegacyAction(extendedAction, *cond, action);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleFirst */




/*****************************************************************************/
/** fmGetACLRule
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the parameters of a specific ACL rule. Report the
 *                  rule's condition, value, action, option and parameter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[out]      cond points to caller-allocated storage where this function
 *                  should store the rule's condition (see 'fm_aclCondition').
 *
 * \param[out]      value points to caller-allocated storage for the
 *                  ''fm_aclValue'' structure that is used to match against
 *                  the given condition.
 *
 * \param[out]      action points to caller-allocated storage where this
 *                  function should store the rule's action (see
 *                  ''fm_aclActionExt'').
 *
 * \param[out]      param points to caller-allocated storage where this
 *                  function should store the rule's action-dependent parameter
 *                  (see 'fm_aclParamExt').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if acl is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetACLRule(fm_int           sw,
                       fm_int           acl,
                       fm_int           rule,
                       fm_aclCondition *cond,
                       fm_aclValue *    value,
                       fm_aclActionExt *action,
                       fm_aclParamExt * param)
{
    fm_acl *    aclEntry;
    fm_status   err = FM_OK;
    void *      voidRule;
    fm_aclRule *aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d\n",
                     sw,
                     acl,
                     rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((cond == NULL) || (value == NULL) ||
        (action == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeFind(&aclEntry->rules, rule, &voidRule);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) voidRule;

    *cond   = aclRule->cond;
    *value  = aclRule->value;
    *action = aclRule->action;
    *param  = aclRule->param;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRule */




/*****************************************************************************/
/** fmGetACLRuleFirstExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the first valid rule in an ACL. Report the rule's
 *                  condition, value, action, option and parameter.
 *
 * \note            This function was formerly known as ''fmGetFirstACLRuleExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[out]      firstRule is a pointer to storage where the first rule
 *                  number is to be stored by this function.
 *
 * \param[out]      cond points to caller-allocated storage where this function
 *                  should store the first rule's condition
 *                  (see 'fm_aclCondition').
 *
 * \param[out]      value points to caller-allocated storage for the
 *                  fm_aclValue structure (see 'fm_aclValue') that is used to
 *                  match against the given condition.
 *
 * \param[out]      action points to caller-allocated storage where this
 *                  function should store the first rule's action
 *                  (see 'fm_aclActionExt').
 *
 * \param[out]      param points to caller-allocated storage where this
 *                  function should store the first rule's parameter
 *                  (see 'fm_aclParamExt').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_RULES_IN_ACL if acl has no rules.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmGetACLRuleFirstExt(fm_int           sw,
                               fm_int           acl,
                               fm_int *         firstRule,
                               fm_aclCondition *cond,
                               fm_aclValue *    value,
                               fm_aclActionExt *action,
                               fm_aclParamExt * param)
{
    fm_acl *        aclEntry;
    fm_status       err;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_aclRule *    aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, firstRule = %p, cond = %p, value = %p, "
                     "action = %p, param = %p\n",
                     sw,
                     acl,
                     (void *) firstRule,
                     (void *) cond,
                     (void *) value,
                     (void *) action,
                     (void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((firstRule == NULL) ||(cond == NULL) || (value == NULL) ||
        (action == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmTreeIterInit(&it, &aclEntry->rules);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_RULES_IN_ACL;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) nextValue;

    *firstRule = (fm_int) nextKey;
    *cond      = aclRule->cond;
    *value     = aclRule->value;
    *action    = aclRule->action;
    *param     = aclRule->param;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleFirstExt */




/*****************************************************************************/
/** fmGetACLRuleNext
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            DEPRECATED in favor of ''fmGetACLRuleNextExt''. Get the 
 *                  next valid rule in an ACL.
 *
 * \note            This function is not thread-safe, because it returns
 *                  value as a pointer to an internal data structure which
 *                  could change at any time.  Use ''fmGetACLRuleNextExt''
 *                  instead.
 *
 * \note            This function was formerly known as ''fmGetNextACLRule''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       currentRule is last valid rule number found by a previous
 *                  call to this function or to ''fmGetACLRuleFirst''.
 *
 * \param[out]      nextRule is a pointer to storage where the next rule number
 *                  is to be stored by this function.
 *
 * \param[out]      cond points to caller-allocated storage where this function
 *                  should store the next rule's condition (see 'fm_aclCondition').
 *
 * \param[out]      value is the address of a pointer to the fm_aclValue
 *                  structure (see 'fm_aclValue') that is used to match against
 *                  the given condition. This function will fill in the pointer
 *                  with the next rule's value.
 *
 * \param[out]      action points to caller-allocated storage where this function
 *                  should store the next rule's action (see 'fm_aclAction').
 *
 * \param[out]      param points to caller-allocated storage where this function
 *                  should store the next rule's parameter (see 'fm_aclParam').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_RULES_IN_ACL if acl has no more rules.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmGetACLRuleNext(fm_int           sw,
                           fm_int           acl,
                           fm_int           currentRule,
                           fm_int *         nextRule,
                           fm_aclCondition *cond,
                           fm_aclValue **   value,
                           fm_aclAction *   action,
                           fm_aclParam *    param)
{
    fm_acl *        aclEntry;
    fm_status       err;
    fm_aclActionExt extendedAction;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_aclRule *    aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, currentRule = %d, nextRule = %p, "
                     "cond = %p, value = %p, action = %p, param = %p\n",
                     sw,
                     acl,
                     currentRule,
                     (void *) nextRule,
                     (void *) cond,
                     (void *) value,
                     (void *) action,
                     (void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((nextRule == NULL) || (cond == NULL) || (value == NULL) ||
        (action == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeSuccessor(&aclEntry->rules, currentRule, &nextKey, &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_RULES_IN_ACL;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) nextValue;

    *nextRule      = (fm_int) nextKey;
    *cond          = aclRule->cond;
    *value         = &aclRule->value;
    extendedAction = aclRule->action;
    *param         = aclRule->param.mirrorPort;

    err = fmExtendedActionToLegacyAction(extendedAction, *cond, action);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleNext */




/*****************************************************************************/
/** fmGetACLRuleNextExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the next valid rule in an ACL.
 *
 * \note            This function was formerly known as ''fmGetNextACLRuleExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       currentRule is last valid rule number found by a previous
 *                  call to this function or to ''fmGetACLRuleFirst''.
 *
 * \param[out]      nextRule is a pointer to storage where the next rule number
 *                  is to be stored by this function.
 *
 * \param[out]      cond points to caller-allocated storage where this function
 *                  should store the next rule's condition
 *                  (see 'fm_aclCondition').
 *
 * \param[out]      value points to caller-allocated storage for the
 *                  fm_aclValue structure (see 'fm_aclValue') that is used to
 *                  match against the given condition.
 *
 * \param[out]      action points to caller-allocated storage where this
 *                  function should store the next rule's action
 *                  (see 'fm_aclActionExt').
 *
 * \param[out]      param points to caller-allocated storage where this
 *                  function should store the next rule's parameter
 *                  (see 'fm_aclParamExt').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_RULES_IN_ACL if acl has no more rules.
 * \return          FM_ERR_UNSUPPORTED if cond is not supported.
 *
 *****************************************************************************/
fm_status fmGetACLRuleNextExt(fm_int           sw,
                              fm_int           acl,
                              fm_int           currentRule,
                              fm_int *         nextRule,
                              fm_aclCondition *cond,
                              fm_aclValue *    value,
                              fm_aclActionExt *action,
                              fm_aclParamExt * param)
{
    fm_acl *    aclEntry;
    fm_status   err;
    fm_uint64   nextKey;
    void *      nextValue;
    fm_aclRule *aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, currentRule = %d, nextRule = %p, "
                     "cond = %p, value = %p, action = %p, param = %p\n",
                     sw,
                     acl,
                     currentRule,
                     (void *) nextRule,
                     (void *) cond,
                     (void *) value,
                     (void *) action,
                     (void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ((nextRule == NULL) || (cond == NULL) || (value == NULL) ||
        (action == NULL) || (param == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeSuccessor(&aclEntry->rules, currentRule, &nextKey, &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_RULES_IN_ACL;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) nextValue;

    *nextRule = (fm_int) nextKey;
    *cond     = aclRule->cond;
    *value    = aclRule->value;
    *action   = aclRule->action;
    *param    = aclRule->param;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleNextExt */




/*****************************************************************************/
/** fmGetACL
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Gets the scenarios and precedence that were specified
 *                  when an ACL was created with fmCreateACLExt.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[out]      args is where the scenarios and precedence are returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetACL(fm_int           sw,
                   fm_int           acl,
                   fm_aclArguments *args)
{
    fm_status err = FM_OK;
    fm_acl *  aclEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, "sw = %d, acl = %d\n", sw, acl);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (args == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    args->scenarios  = aclEntry->scenarios;
    args->precedence = aclEntry->precedence;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACL */




/*****************************************************************************/
/** fmGetACLFirst
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the first valid ACL in a switch.
 *
 * \note            This function was formerly known as ''fmGetFirstACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstAcl is set to the number of the first ACL.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_ACLS if switch has no ACLs.
 *
 *****************************************************************************/
fm_status fmGetACLFirst(fm_int  sw,
                        fm_int *firstAcl)
{
    fm_status       err = FM_OK;
    fm_aclInfo *    aclInfo;
    fm_treeIterator it;
    fm_uint64       nextKey;
    fm_acl *        aclEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    if (firstAcl == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclInfo = &GET_SWITCH_PTR(sw)->aclInfo;

    for (fmTreeIterInit(&it, &aclInfo->acls) ;
         (err = fmTreeIterNext(&it, &nextKey, (void**) &aclEntry)) == FM_OK ; )
    {
        if (aclEntry->internal == FALSE)
        {
            break;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_ERR_NO_ACLS;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    *firstAcl = (fm_int) nextKey;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLFirst */




/*****************************************************************************/
/** fmGetACLNext
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the next valid ACL in a switch.
 *
 * \note            This function was formerly known as ''fmGetNextACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentAcl is the last valid ACL number found by a previous
 *                  call to this function or to ''fmGetACLFirst''.
 *
 * \param[out]      nextAcl is set to the number of the next ACL.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_ACLS if switch has no more ACLs.
 *
 *****************************************************************************/
fm_status fmGetACLNext(fm_int  sw,
                       fm_int  currentAcl,
                       fm_int *nextAcl)
{
    fm_status       err = FM_OK;
    fm_aclInfo *    aclInfo;
    fm_treeIterator it;
    fm_uint64       nextKey;
    fm_acl *        aclEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, currentAcl = %d\n",
                     sw,
                     currentAcl);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    if (nextAcl == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclInfo = &GET_SWITCH_PTR(sw)->aclInfo;

    for (fmTreeIterInitFromSuccessor(&it, &aclInfo->acls, currentAcl) ;
         (err = fmTreeIterNext(&it, &nextKey, (void**) &aclEntry)) == FM_OK ; )
    {
        if (aclEntry->internal == FALSE)
        {
            break;
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_ERR_NO_ACLS;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    *nextAcl = (fm_int) nextKey;

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLNext */




/*****************************************************************************/
/** fmAddACLPort
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Associates an ACL with a port. This function must be called
 *                  multiple times to associate an ACL with multiple ports.
 *
 * \note            This function associates an entire ACL with a port. However,
 *                  each rule in an ACL may be individually associated with
 *                  a distinct set of ingress ports using the
 *                  ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition (FM3000 and
 *                  FM4000 only) or ''FM_ACL_MATCH_INGRESS_PORT_SET'' (all 
 *                  devices except FM2000) in ''fmAddACLRule'' or 
 *                  ''fmAddACLRuleExt'', which overrides any ingress ACL port 
 *                  association made with this function.
 *
 * \note            For FM6000 and FM10000 devices, it is not supported to combine this
 *                  function with ''FM_ACL_MATCH_INGRESS_PORT_SET'' rule
 *                  condition within one ACL as this is mutually exclusive.
 *
 * \note            For FM6000 and FM10000 devices, there are limited hardware
 *                  resources for mapping port numbers to ACLs. If the
 *                  specified ACL configuration requires more hardware
 *                  resources than are available, ''fmCompileACL'' will
 *                  report an error.
 *
 * \note            This function was formerly known as ''fmSetPortACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       port is the port number to which the acl should be associated.
 *
 * \param[in]       type is the acl type (see 'fm_aclType').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_UNSUPPORTED if the type is not supported.
 *
 *****************************************************************************/
fm_status fmAddACLPort(fm_int sw, fm_int acl, fm_int port, fm_aclType type)
{
    fm_acl *   aclEntry;
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_bool    first = FALSE;   /* initialization makes compiler happy */
    fm_int     firstAcl;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, port = %d, type = %d\n",
                     sw,
                     acl,
                     port,
                     type);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (type >= FM_ACL_TYPE_MAX)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Only support ingress rules on FM2000.
     **************************************************/
    if (type != FM_ACL_TYPE_INGRESS &&
        switchPtr->switchFamily == FM_SWITCH_FAMILY_FM2000)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->CanonicalizeACLPort != NULL)
    {
        /* for FM2000 */
        err = switchPtr->CanonicalizeACLPort(sw, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->PostSetPortACL != NULL)
    {
        /**************************************************
         * Only need to check this if we are going to call
         * PostSetPortACL later, but we have to check this
         * before we set the bit below.  (for FM2000)
         **************************************************/
        first = (fmGetPortACLFirst(sw, port, &firstAcl) == FM_ERR_NO_ACLS);
    }

    /* all associations use the canonical port */
    err = fmSetAclAssociatedPort(sw, aclEntry, port, type, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    aclEntry->numberOfPorts[type]++;

    if (type == FM_ACL_TYPE_INGRESS)
    {
        aclEntry->aclPortType = FM_ACL_PORT_TYPE_INGRESS;
    }
    else if (type == FM_ACL_TYPE_EGRESS)
    {
        aclEntry->aclPortType = FM_ACL_PORT_TYPE_EGRESS;
    }

    if (switchPtr->PostSetPortACL != NULL)
    {
        /* for FM2000 */
        err = switchPtr->PostSetPortACL(sw, port, first);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->AddACLPort != NULL)
    {
        /* for SWAG */
        err = switchPtr->AddACLPort(sw, acl, port, type);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmAddACLPort */




/*****************************************************************************/
/** fmAddACLPortExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Associates an ACL with a port.  This function is
 *                  100% semantically equivalent to ''fmAddACLPort'', but
 *                  takes the port and type together as a structure,
 *                  for symmetry with ''fmDeleteACLPortExt''. This function 
 *                  must be called multiple times to associate an ACL with 
 *                  multiple ports.
 *
 * \note            This function associates an entire ACL with a port. However,
 *                  each rule in an ACL may be individually associated with
 *                  a distinct set of ingress ports using the
 *                  ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition (FM3000 and
 *                  FM4000 only) or ''FM_ACL_MATCH_INGRESS_PORT_SET'' (all 
 *                  devices except FM2000) in ''fmAddACLRule'' or 
 *                  ''fmAddACLRuleExt'', which overrides any ingress ACL port 
 *                  association made with this function.
 *
 * \note            For FM6000 and FM10000 devices, it is not supported to combine this
 *                  function with ''FM_ACL_MATCH_INGRESS_PORT_SET'' rule
 *                  condition within one ACL as this is mutually exclusive.
 *
 * \note            For FM6000 and FM10000 devices, there are limited hardware
 *                  resources for mapping port numbers to ACLs. If the
 *                  specified ACL configuration requires more hardware
 *                  resources than are available, ''fmCompileACL'' will report
 *                  an error.
 *
 * \note            This function was formerly known as ''fmSetPortACLExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       portAndType encodes the port number to which the acl should
 *                  be associated, and the acl type (see 'fm_aclPortAndType').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_UNSUPPORTED if the type is not supported.
 *
 *****************************************************************************/
fm_status fmAddACLPortExt(fm_int                   sw,
                          fm_int                   acl,
                          const fm_aclPortAndType *portAndType)
{
    fm_status err;

    if (portAndType)
    {
        FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                         "sw = %d, acl = %d, portAndType->port = %d, "
                         "portAndType->type = %d\n",
                         sw,
                         acl,
                         portAndType->port,
                         portAndType->type);
    }
    else
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmAddACLPort(sw, acl, portAndType->port, portAndType->type);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmAddACLPortExt */




/*****************************************************************************/
/** fmDeleteACLPort
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Remove an ACL's ingress association with a port.
 *
 * \note            This function was formerly known as ''fmDeletePortACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       port is the port number from which to remove the ACL.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmDeleteACLPort(fm_int sw, fm_int acl, fm_int port)
{
    fm_aclPortAndType portAndType;
    fm_status         err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, port = %d\n",
                     sw,
                     acl,
                     port);

    portAndType.port = port;
    portAndType.type = FM_ACL_TYPE_INGRESS;

    err = fmDeleteACLPortExt(sw, acl, &portAndType);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteACLPort */




/*****************************************************************************/
/** fmDeleteACLPortExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Remove an ACL's association with a port.
 *
 * \note            This function was formerly known as ''fmDeletePortACLExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       portAndType encodes is the port number from which to
 *                  remove the ACL, and the ACL type (see 'fm_aclPortAndType').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmDeleteACLPortExt(fm_int                   sw,
                             fm_int                   acl,
                             const fm_aclPortAndType *portAndType)
{
    fm_acl *   aclEntry;
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_int     firstAcl;
    fm_bool    last;
    fm_int     port;
    fm_aclType type;
    fm_bool    bValue;

    if (portAndType)
    {
        FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                         "sw = %d, acl = %d, portAndType->port = %d, "
                         "portAndType->type = %d\n",
                         sw,
                         acl,
                         portAndType->port,
                         portAndType->type);
    }
    else
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);

    port      = portAndType->port;
    type      = portAndType->type;

    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (type >= FM_ACL_TYPE_MAX)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->CanonicalizeACLPort != NULL)
    {
        /* for FM2000 */
        err = switchPtr->CanonicalizeACLPort(sw, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmGetAclAssociatedPort(sw, aclEntry, port, type, &bValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (bValue != TRUE)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmSetAclAssociatedPort(sw, aclEntry, port, type, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    aclEntry->numberOfPorts[type]--;

    if (aclEntry->numberOfPorts[type] == 0)
    {
        aclEntry->aclPortType = FM_ACL_PORT_TYPE_NONE;
    }

    err = fmCleanupACLRules(sw, port, aclEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);


    if (switchPtr->CleanupPortACL != NULL)
    {
        /* for FM2000 */
        last = (fmGetPortACLFirst(sw, port, &firstAcl) == FM_ERR_NO_ACLS);
        err  = switchPtr->CleanupPortACL(sw, port, last);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (switchPtr->DeleteACLPort != NULL)
    {
        /* for SWAG */
        err = switchPtr->DeleteACLPort(sw, acl, portAndType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmDeleteACLPortExt */




/*****************************************************************************/
/** fmClearACLPort
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Remove all port associations for an ACL.
 *
 * \note            This function was formerly known as ''fmClearPortACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmClearACLPort(fm_int sw, fm_int acl)
{
    fm_acl *   aclEntry;
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, "sw = %d, acl = %d\n", sw, acl);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmClearBitArray(&aclEntry->associatedPorts);

    aclEntry->numberOfPorts[FM_ACL_TYPE_INGRESS] = 0;
    aclEntry->numberOfPorts[FM_ACL_TYPE_EGRESS] = 0;
    aclEntry->aclPortType = FM_ACL_PORT_TYPE_NONE;

    if ( (err == FM_OK) && (switchPtr->ClearACLPort != NULL) )
    {
        /* for SWAG */
        err = switchPtr->ClearACLPort(sw, acl);
    }


ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmClearACLPort */




/*****************************************************************************/
/** fmGetPortACLFirst
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the first ACL associated with a given port.
 *
 * \note            This function was formerly known as ''fmGetFirstPortACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the number of the port to search for ACLs. May
 *                  not be the CPU interface port.
 *
 * \param[out]      firstACL points to caller-allocated storage where this
 *                  function should place the port's first ACL number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no ACLs associated with this port.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetPortACLFirst(fm_int  sw,
                            fm_int  port,
                            fm_int *firstACL)
{
    fm_aclAclAndType aclAndType;
    fm_status        err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, port = %d, firstACL = %p\n",
                     sw,
                     port,
                     (void *) firstACL);

    if (firstACL == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    err       = fmGetPortACLFirstExt(sw, port, &aclAndType);
    *firstACL = aclAndType.acl;

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetPortACLFirst */




/*****************************************************************************/
/** fmGetPortACLFirstExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the first ACL and type associated with a given port.
 *
 * \note            This function was formerly known as ''fmGetFirstPortACLExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the number of the port to search for ACLs. May
 *                  not be the CPU interface port.
 *
 * \param[out]      aclAndType points to caller-allocated storage where this
 *                  function should place the port's first ACL number and
 *                  type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no ACLs associated with this port.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetPortACLFirstExt(fm_int            sw,
                               fm_int            port,
                               fm_aclAclAndType *aclAndType)
{
    fm_acl *        aclEntry;
    fm_int          acl;
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_treeIterator it;
    fm_int          startBit;
    fm_int          foundBit;
    fm_aclPortAndType portTuple;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, port = %d, aclAndType = %p\n",
                     sw,
                     port,
                     (void *) aclAndType);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, DISALLOW_CPU);
    FM_TAKE_ACL_LOCK(sw);

    if (aclAndType == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->CanonicalizeACLPort != NULL)
    {
        /* for FM2000 */
        err = switchPtr->CanonicalizeACLPort(sw, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    startBit = fmPackAclAssociatedPort(sw, port, 0);

    for (fmTreeIterInit(&it, &switchPtr->aclInfo.acls) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        acl      = (fm_int) nextKey;
        aclEntry = (fm_acl *) nextValue;

        if (aclEntry->internal)
        {
            continue;
        }

        err = fmFindBitInBitArray(&aclEntry->associatedPorts,
                                  startBit,
                                  TRUE,
                                  &foundBit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (foundBit >= 0)
        {
            fmUnpackAclAssociatedPort(sw, foundBit, &portTuple);
            if (portTuple.port == port)
            {
                aclAndType->acl  = acl;
                aclAndType->type = portTuple.type;
                break;
            }
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_ERR_NO_ACLS;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetPortACLFirstExt */




/*****************************************************************************/
/** fmGetPortACLNext
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the next ACL associated with a given port.
 *
 * \note            This function was formerly known as ''fmGetNextPortACL''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the number of the port to search for ACLs. May
 *                  not be the CPU interface port.
 *
 * \param[in]       currentACL is the last ACL found for this port by a previous
 *                  call to this function or to ''fmGetPortACLFirst''.
 *
 * \param[out]      nextACL points to caller-allocated storage where this
 *                  function should place the port's next ACL number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no more ACLs associated with this port.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetPortACLNext(fm_int  sw,
                           fm_int  port,
                           fm_int  currentACL,
                           fm_int *nextACL)
{
    fm_aclAclAndType aclAndType;
    fm_status        err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, port = %d, currentACL = %d nextACL=%p\n",
                     sw,
                     port,
                     currentACL,
                     (void *) nextACL);

    if (nextACL == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    aclAndType.acl  = currentACL;
    aclAndType.type = FM_ACL_TYPE_MAX - 1;

    err      = fmGetPortACLNextExt(sw, port, &aclAndType);
    *nextACL = aclAndType.acl;

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetPortACLNext */




/*****************************************************************************/
/** fmGetPortACLNextExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the next ACL and type associated with a given port.
 *
 * \note            This function was formerly known as ''fmGetNextPortACLExt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the number of the port to search for ACLs. May
 *                  not be the CPU interface port.
 *
 * \param[in,out]   aclAndType should contain on entry the last ACL and type
 *                  found for this port by a previous call to this function or
 *                  to ''fmGetPortACLFirstExt''.  On exit, contains the port's
 *                  next ACL number and type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no more ACLs associated with this port.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetPortACLNextExt(fm_int            sw,
                              fm_int            port,
                              fm_aclAclAndType *aclAndType)
{
    fm_acl *        aclEntry;
    fm_int          acl;
    fm_status       err;
    fm_switch *     switchPtr;
    fm_uint64       nextKey;
    void *          nextValue;
    fm_treeIterator it;
    fm_aclType      startType;
    fm_int          startBit;
    fm_int          foundBit;
    fm_bool         isLastType;
    fm_int          currentACL;
    fm_aclPortAndType portTuple;

    if (aclAndType)
    {
        FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                         "sw = %d, port = %d, "
                         "aclAndType->acl = %d, aclAndType->type = %d\n",
                         sw,
                         port,
                         aclAndType->acl,
                         aclAndType->type);
    }
    else
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, DISALLOW_CPU);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->CanonicalizeACLPort != NULL)
    {
        /* for FM2000 */
        err = switchPtr->CanonicalizeACLPort(sw, &port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    isLastType = (aclAndType->type == FM_ACL_TYPE_MAX - 1);
    currentACL = aclAndType->acl;

    for (err = ( isLastType ?
                 fmTreeIterInitFromSuccessor(&it,
                                             &switchPtr->aclInfo.acls,
                                             currentACL) :
                 fmTreeIterInitFromKey(&it,
                                       &switchPtr->aclInfo.acls,
                                       currentACL) ) ;
         err == FM_OK &&
             ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        acl      = (fm_int) nextKey;
        aclEntry = (fm_acl *) nextValue;

        if (aclEntry->internal)
        {
            continue;
        }

        startType = (acl == currentACL) ? aclAndType->type + 1 : 0;
        startBit  = fmPackAclAssociatedPort(sw, port, startType);

        err = fmFindBitInBitArray(&aclEntry->associatedPorts,
                                  startBit,
                                  TRUE,
                                  &foundBit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (foundBit >= 0)
        {
            fmUnpackAclAssociatedPort(sw, foundBit, &portTuple);
            if (portTuple.port == port)
            {
                aclAndType->acl  = acl;
                aclAndType->type = portTuple.type;
                break;
            }
        }
    }

    if (err == FM_ERR_NO_MORE)
    {
        err = FM_ERR_NO_ACLS;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetPortACLNextExt */




/*****************************************************************************/
/** fmGetACLPortFirst
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the first port and type associated with a given ACL.
 *
 * \note            This function was formerly known as ''fmGetFirstACLPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number to search.
 *
 * \param[out]      portAndType is set to the port number and association
 *                  type of the first port associated with this ACL.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_MORE if acl has no associated ports.
 *
 *****************************************************************************/
fm_status fmGetACLPortFirst(fm_int             sw,
                            fm_int             acl,
                            fm_aclPortAndType *portAndType)
{
    fm_status err = FM_OK;
    fm_acl *  aclEntry;
    fm_int    foundBit;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL, "sw = %d, acl = %d\n", sw, acl);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (portAndType == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmFindBitInBitArray(&aclEntry->associatedPorts, 0, TRUE, &foundBit);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (foundBit == -1)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmUnpackAclAssociatedPort(sw, foundBit, portAndType);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLPortFirst */




/*****************************************************************************/
/** fmGetACLPortNext
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Find the next port and type associated with a given ACL.
 *
 * \note            This function was formerly known as ''fmGetNextACLPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number to search.
 *
 * \param[in,out]   portAndType should contain on entry the last port and type
 *                  found for this ACL by a previous call to this function or
 *                  to ''fmGetACLPortFirst''.  On exit, contains the ACL's
 *                  next port number and association type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_NO_MORE if acl has no more associated ports.
 *
 *****************************************************************************/
fm_status fmGetACLPortNext(fm_int             sw,
                           fm_int             acl,
                           fm_aclPortAndType *portAndType)
{
    fm_status err = FM_OK;
    fm_acl *  aclEntry;
    fm_int    startBit;
    fm_int    foundBit;

    if (portAndType)
    {
        FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                         "sw = %d, acl = %d, "
                         "portAndType->port = %d, portAndType->type = %d\n",
                         sw,
                         acl,
                         portAndType->port,
                         portAndType->type);
    }
    else
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    startBit =
        fmPackAclAssociatedPort(sw, portAndType->port, portAndType->type) + 1;

    err = fmFindBitInBitArray(&aclEntry->associatedPorts,
                              startBit,
                              TRUE,
                              &foundBit);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (foundBit == -1)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmUnpackAclAssociatedPort(sw, foundBit, portAndType);

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLPortNext */




/*****************************************************************************/
/** fmGetACLCount
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the frame count associated with an
 *                  ''FM_ACL_ACTION_COUNT'' or ''FM_ACL_ACTIONEXT_COUNT'' 
 *                  ACL rule.
 *
 * \note            On non-FM6000 devices, this function is only applicable
 *                  to ACLs being used as ingress ACLs. For ACLs being used
 *                  as egress ACLs, see ''fmGetACLEgressCount''.
 *                                                                      \lb\lb
 *                  On FM6000 devices, this function is applicable to ACLs
 *                  being used as ingress or egress ACLs. In that case, 
 *                  egress ACL counters don't use the rule argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number for which to retrieve the count.
 *
 * \param[in]       rule is the rule number for which to retrieve the count.
 *                  This argument is not used for FM6000 egress ACLs.
 *
 * \param[out]      frameCount points to caller-allocated storage where this
 *                  function should place the frame count.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_RULE if acl has no such rule.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is not a counting rule.
 *
 *****************************************************************************/
fm_status fmGetACLCount(fm_int     sw,
                        fm_int     acl,
                        fm_int     rule,
                        fm_uint64 *frameCount)
{
    fm_aclCounters counters;
    fm_status      err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d frameCount=%p\n",
                     sw,
                     acl,
                     rule,
                     (void *) frameCount);

    if (frameCount == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmGetACLCountExt(sw, acl, rule, &counters);

    if (err == FM_OK)
    {
        *frameCount = counters.cntPkts;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLCount */




/*****************************************************************************/
/** fmGetACLCountExt
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  ''FM_ACL_ACTION_COUNT'' or ''FM_ACL_ACTIONEXT_COUNT'' 
 *                  ACL rule.
 *
 * \note            On non-FM6000 devices, this function is only applicable
 *                  to ACLs being used as ingress ACLs. For ACLs being used
 *                  as egress ACLs, see ''fmGetACLEgressCount''.
 *                                                                      \lb\lb
 *                  On FM6000 devices, this function is applicable to ACLs
 *                  being used as ingress or egress ACLs. In that case, 
 *                  egress ACL counters don't use the rule argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number for which to retrive the count.
 *
 * \param[in]       rule is the rule number for which to retrive the count.
 *                  This argument is not used for FM6000 egress ACLs.
 *
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_aclCounters'' where this function should place the
 *                  counter values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ACL_RULE if acl has no such rule.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is not a counting rule.
 *
 *****************************************************************************/
fm_status fmGetACLCountExt(fm_int          sw,
                           fm_int          acl,
                           fm_int          rule,
                           fm_aclCounters *counters)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, "
                     "counters = %p\n",
                     sw,
                     acl,
                     rule,
                     (void *) counters);

    if (counters == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetACLCountExt,
                       sw,
                       acl,
                       rule,
                       counters);

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLCountExt */




/*****************************************************************************/
/** fmResetACLCount
 * \ingroup acl
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Reset an ACL counter.
 * 
 * \note            On non-FM6000 devices, this function is only applicable
 *                  to ACLs being used as ingress ACLs. For ACLs being used
 *                  as egress ACLs, see ''fmResetACLEgressCount''.
 *                                                                      \lb\lb
 *                  On FM6000 devices, this function is applicable to ACLs
 *                  being used as ingress or egress ACLs. In that case, 
 *                  egress ACL counters don't use the rule argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number for which to reset the count.
 *
 * \param[in]       rule is the rule number for which to reset the count.
 *                  This argument is not used for FM6000 egress ACLs.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_ACL_RULE if acl has no such rule.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is not a counting rule.
 *
 *****************************************************************************/
fm_status fmResetACLCount(fm_int sw,
                          fm_int acl,
                          fm_int rule)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d\n",
                     sw,
                     acl,
                     rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetACLCount,
                       sw,
                       acl,
                       rule);

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmResetACLCount */




/*****************************************************************************/
/** fmGetACLEgressCount
 * \ingroup acl
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Returns the value of the egress ACL counter for a
 *                  specific port.
 *                                                                      \lb\lb
 *                  The octet count returned in the ''fm_aclCounters'' 
 *                  structure reflects the number of octets in the frame
 *                  when it was received by the chip (even though this
 *                  counter pertains to the frame as it egresses). In
 *                  multi-switch systems, if the frame ingressed the chip
 *                  on an Inter-Switch Link (ISL), the frame will likely
 *                  include an ISL tag, which increases the frame
 *                  length by 4 bytes. These additional 4 bytes will be
 *                  included in the octet count.
 *                                                                      \lb\lb
 *                  On FM4000 devices, this functionality only works if you
 *                  have enabled the FM_STAT_GROUP_EGRESS_ACL group in the
 *                  ''FM_PORT_STAT_GROUP_ENABLE'' port attribute.
 *                                                                      \lb\lb
 *                  On FM10000 devices, this function only returns the number
 *                  of frames dropped due to an Egress ACL drop action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the port to get the counter for.
 *
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_aclCounters'' where this function should place the
 *                  counter values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmGetACLEgressCount(fm_int          sw,
                              fm_int          logicalPort,
                              fm_aclCounters *counters)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, logicalPort = %d, counters = %p\n",
                     sw,
                     logicalPort,
                     (void*) counters);

    if (counters == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetACLEgressCount,
                       sw,
                       logicalPort,
                       counters);

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLEgressCount */




/*****************************************************************************/
/** fmResetACLEgressCount
 * \ingroup acl
 *
 * \chips           FM3000, FM4000, FM10000
 *
 * \desc            Clear the egress ACL counter for a
 *                  specific port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the port to clear the counter for.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 *
 *****************************************************************************/
fm_status fmResetACLEgressCount(fm_int sw,
                                fm_int logicalPort)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, logicalPort = %d\n",
                     sw,
                     logicalPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetACLEgressCount,
                       sw,
                       logicalPort);

    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmResetACLEgressCount */




/*****************************************************************************/
/** fmCompileACL
 * \ingroup acl
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Compile the complete set of configured and enabled ACLs.
 *                  ACLs must be compiled after access lists and their
 *                  associated rules are created. After successfully
 *                  compiling, the ACLs will not take effect until they
 *                  are written to the hardware by calling ''fmApplyACL''.
 *
 * \note            For FM6000 and FM10000 devices, if the flags argument
 *                  has the ''FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE'' bit set,
 *                  this function merely validates that the ACL configuration
 *                  can be successfully programmed into the hardware. No 
 *                  change is actually made to the internal ACL data 
 *                  structures.
 *
 * \note            This function is not needed for FM2000 family devices. If
 *                  called for an FM2000, it will return FM_OK without doing 
 *                  anything.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      statusText points to caller-allocated storage where this
 *                  function should place compiler error message text
 *                  (if FM_ERR_ACL_COMPILE is returned) or informative
 *                  statistics about the compilation (if FM_OK is returned).
 *
 * \param[in]       statusTextLength is the number of bytes pointed to by
 *                  text.
 *
 * \param[in]       flags is a bitmask. See ''ACL Compiler Flags''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_COMPILE if the ACL compiler was unable to
 *                  produce a valid ACL "binary image" from the current ACL
 *                  configuration.
 *
 *****************************************************************************/
fm_status fmCompileACL(fm_int    sw,
                       fm_text   statusText,
                       fm_int    statusTextLength,
                       fm_uint32 flags)
{
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, "
                     "statusText = %p, "
                     "statusTextLength = %d, "
                     "flags = 0x%x\n",
                     sw,
                     (void *) statusText,
                     statusTextLength,
                     flags);

    err = fmCompileACLExt(sw, statusText, statusTextLength, flags, NULL);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmCompileACL */




/*****************************************************************************/
/** fmCompileACLExt
 * \ingroup acl
 * 
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Compile the complete set of configured and enabled ACLs.
 *                  ACLs must be compiled after access lists and their
 *                  associated rules are created. After successfully
 *                  compiling, the ACLs will not take effect until they
 *                  are written to the hardware by calling ''fmApplyACL''.
 *
 * \note            For FM6000 and FM10000 devices, if the flags argument
 *                  has the ''FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE'' bit set,
 *                  this function merely validates that the ACL configuration
 *                  can be successfully programmed into the hardware. No 
 *                  change is actually made to the internal ACL data 
 *                  structures.
 *
 * \note            This function is not needed for FM2000 family devices. If
 *                  called for an FM2000, it will return FM_OK without doing 
 *                  anything.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      statusText points to caller-allocated storage where this
 *                  function should place compiler error message text
 *                  (if FM_ERR_ACL_COMPILE is returned) or informative
 *                  statistics about the compilation (if FM_OK is returned).
 *
 * \param[in]       statusTextLength is the number of bytes pointed to by
 *                  text.
 *
 * \param[in]       flags is a bitmask. See ''ACL Compiler Flags''.
 * 
 * \param[in,out]   value points to the compiler extended flags entered.
 *                  The data type of value is dependent on the flags selected.
 *                  See ''ACL Compiler Flags'' for a description of the data
 *                  type required for each extended flag.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ACL_COMPILE if the ACL compiler was unable to
 *                  produce a valid ACL "binary image" from the current ACL
 *                  configuration.
 *
 *****************************************************************************/
fm_status fmCompileACLExt(fm_int    sw,
                          fm_text   statusText,
                          fm_int    statusTextLength,
                          fm_uint32 flags,
                          void *    value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, "
                     "statusText = %p, "
                     "statusTextLength = %d, "
                     "flags = 0x%x, "
                     "value = %p\n",
                     sw,
                     (void *) statusText,
                     statusTextLength,
                     flags,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_TAKE_ACL_LOCK(sw);

    if (switchPtr->ACLCompile == NULL)
    {
        err = FM_OK;            /* no-op for FM2000 */
    }
    else
    {
        err = switchPtr->ACLCompile(sw,
                                    statusText,
                                    statusTextLength,
                                    flags,
                                    value);
    }

    FM_DROP_ACL_LOCK(sw);
    fmReleaseWriteLock(&switchPtr->routingLock);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmCompileACLExt */




/*****************************************************************************/
/** fmApplyACL
 * \ingroup acl
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Apply an ACL "binary image," created with a prior call to
 *                  ''fmCompileACL'', to the hardware.
 *
 * \note            This function is not needed for FM2000 family devices. If
 *                  called for an FM2000, it will return FM_OK without doing 
 *                  anything.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags is a bitmask. See ''ACL Apply Flags''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL_IMAGE if compiled ACLs are invalid.
 * \return          FM_ERR_ACLS_TOO_BIG if the compiled ACLs will not fit
 *                  in the portion of the FFU allocated to ACLs.
 *
 *****************************************************************************/
fm_status fmApplyACL(fm_int sw, fm_uint32 flags)
{
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, flags = 0x%x\n",
                     sw,
                     flags);

    err = fmApplyACLExt(sw, flags, NULL);
    
    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);
}   /* end fmApplyACL */




/*****************************************************************************/
/*  fmApplyACLExt
 * \ingroup intAcl
 *
 * \desc            Apply an ACL "binary image," created with a call to
 *                  ''fmCompileACL'', to the FM4000 device.
 *
 * \note            This function is not needed for FM2000 family devices. If
 *                  called for an FM2000, it will return FM_OK without doing 
 *                  anything.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags is a bitmask. See ''ACL Apply Flags''.
 *
 * \param[in,out]   value points to the compiler extended flags entered.
 *                  The data type of value is dependent on the flags selected.
 *                  See ''ACL Apply Flags'' for a description of the data
 *                  type required for each extended flag.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL_IMAGE if compiled ACLs are invalid.
 * \return          FM_ERR_ACLS_TOO_BIG if the compiled ACLs will not fit
 *                  in the portion of the FFU allocated to ACLs.
 *
 *****************************************************************************/
fm_status fmApplyACLExt(fm_int sw, fm_uint32 flags, void *value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, flags = 0x%x\n",
                 sw,
                 flags);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * If any ACL rules are for load balancing, we
     * will need to take the LBG lock. But that lock
     * is lower precedence than the ACL lock, so we
     * must take it before taking the ACL lock.
     **************************************************/

    err = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    TAKE_LAG_LOCK(sw);
    
    FM_TAKE_ACL_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ACLApplyExt == NULL)
    {
        err = FM_OK;            /* no-op for FM2000 */
    }
    else
    {
        err = switchPtr->ACLApplyExt(sw, flags, value);
    }

    FM_DROP_ACL_LOCK(sw);
    DROP_LAG_LOCK(sw);
    fmReleaseWriteLock(&switchPtr->routingLock);
    fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    
ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmApplyACLExt */




/*****************************************************************************/
/** fmGetACLRuleIngressPortFirst
 * \ingroup acl
 *
 * \desc            Find the first ingress port associated with a given rule
 *                  as part of the ''FM_ACL_MATCH_INGRESS_PORT_SET'' or
 *                  ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition..
 *
 * \note            This function was formerly known as 
 *                  ''fmGetFirstACLRuleIngressPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[out]      firstPort points to caller-allocated storage where
 *                  this function should place the first ingress port number
 *                  of the rule.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if the rule does not exist.
 * \return          FM_ERR_NO_ACL_RULE_INGRESS_PORT if no port available.
 *
 *****************************************************************************/
fm_status fmGetACLRuleIngressPortFirst(fm_int           sw,
                                       fm_int           acl,
                                       fm_int           rule,
                                       fm_int *         firstPort)
{
    fm_acl *        aclEntry;
    fm_status       err;
    void *          value;
    fm_aclRule *    aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, firstPort = %p\n",
                     sw,
                     acl,
                     rule,
                     (void *) firstPort);

    if (firstPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeFind(&aclEntry->rules, rule, &value);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) value;

    if (aclRule->cond & FM_ACL_MATCH_INGRESS_PORT_SET)
    {
        err = fmFindPortInBitArray(sw,
                                   &aclRule->associatedPorts,
                                   -1,
                                   firstPort,
                                   FM_ERR_NO_ACL_RULE_INGRESS_PORT);
    }
    else if (aclRule->cond & FM_ACL_MATCH_INGRESS_PORT_MASK)
    {
        err = fmFindPortInBitMask(sw,
                                  aclRule->value.ingressPortMask,
                                  -1,
                                  firstPort,
                                  FM_ERR_NO_ACL_RULE_INGRESS_PORT);
    }
    else
    {
        *firstPort = -1;
        err = FM_ERR_NO_ACL_RULE_INGRESS_PORT;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleIngressPortFirst */




/*****************************************************************************/
/** fmGetACLRuleIngressPortNext
 * \ingroup acl
 *
 * \desc            Find the next ingress port associated with a given rule
 *                  as part of the ''FM_ACL_MATCH_INGRESS_PORT_SET'' or
 *                  ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition.
 *
 * \note            This function was formerly known as 
 *                  ''fmGetNextACLRuleIngressPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 *
 * \param[in]       currentPort is the current port number, as returned
 *                  by a previous call to this function or to
 *                  ''fmGetACLRuleIngressPortFirst''.
 *
 * \param[out]      nextPort points to caller-allocated storage where
 *                  this function should place the next ingress port number
 *                  of the rule.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if the rule does not exist.
 * \return          FM_ERR_NO_ACL_RULE_INGRESS_PORT if no port available.
 *
 *****************************************************************************/
fm_status fmGetACLRuleIngressPortNext(fm_int           sw,
                                      fm_int           acl,
                                      fm_int           rule,
                                      fm_int           currentPort,
                                      fm_int *         nextPort)
{
    fm_acl *        aclEntry;
    fm_status       err;
    void *          value;
    fm_aclRule *    aclRule;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ACL,
                     "sw = %d, acl = %d, rule = %d, currentPort = %d, "
                     "nextPort = %p\n",
                     sw,
                     acl,
                     rule,
                     currentPort,
                     (void *) nextPort);

    if (nextPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_ACL_ID(sw, acl);
    VALIDATE_ACL_RULE_ID(sw, rule);
    FM_TAKE_ACL_LOCK(sw);

    GET_ACL_ENTRY(sw, aclEntry, acl);
    if (aclEntry == NULL)
    {
        err = FM_ERR_INVALID_ACL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmTreeFind(&aclEntry->rules, rule, &value);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_ACL_RULE;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    aclRule = (fm_aclRule *) value;

    if (aclRule->cond & FM_ACL_MATCH_INGRESS_PORT_SET)
    {
        err = fmFindPortInBitArray(sw,
                                   &aclRule->associatedPorts,
                                   currentPort,
                                   nextPort,
                                   FM_ERR_NO_ACL_RULE_INGRESS_PORT);
    }
    else if (aclRule->cond & FM_ACL_MATCH_INGRESS_PORT_MASK)
    {
        err = fmFindPortInBitMask(sw,
                                  aclRule->value.ingressPortMask,
                                  currentPort,
                                  nextPort,
                                  FM_ERR_NO_ACL_RULE_INGRESS_PORT);
    }
    else
    {
        *nextPort = -1;
        err = FM_ERR_NO_ACL_RULE_INGRESS_PORT;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ACL, err);

}   /* end fmGetACLRuleIngressPortNext */




/******************************************************************************
 * the following are all the internal functions used by the ACL
 * subsystem (see internal/fm_api_acl_int.h)
 *****************************************************************************/


/*****************************************************************************/
/** fmInitACLTable
 * \ingroup intAcl
 *
 * \desc            initializes ACL table
 *
 * \param[in]       switchPtr points to the switch whose ACL table is being
 *                  initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitACLTable(fm_switch *switchPtr)
{
    fm_aclInfo *  ai;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "switchPtr = %p\n", (void *) switchPtr);

    ai = &switchPtr->aclInfo;

    fmTreeInit(&ai->acls);

    /* ACL is disabled by default */
    ai->enabled = FALSE;

    /* Initialise the mapper tree */
    fmTreeInit(&ai->mappers);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end fmInitACLTable */



/*****************************************************************************/
/** fmDestroyACLTable
 * \ingroup intAcl
 *
 * \desc            Destroys the ACL table.
 *
 * \param[in]       ai points to the ACL Info Table
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDestroyACLTable(fm_aclInfo *ai)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "ai = %p\n", (void *) ai);

    if (!fmTreeIsInitialized(&ai->acls))
    {
        /* Initialization has not been called due to
         * switch bring up error, so when cleanup
         * just return here.
         */
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
    }

    fmTreeDestroy(&ai->acls, VoidFreeAcl);
    fmTreeDestroy(&ai->mappers, fmFree);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end fmDestroyACLTable */




/*****************************************************************************/
/** fmPackAclAssociatedPort
 * \ingroup intAcl
 *
 * \desc            Returns the bit position of a (port, type) in an ACL's
 *                  associatedPorts array.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \param[in]       type is the acl type (INGRESS or EGRESS).
 *
 * \return          Bit position of the tuple in the array.
 *
 *****************************************************************************/
fm_int fmPackAclAssociatedPort(fm_int sw, fm_int port, fm_aclType type)
{
    return FM_PACK_ASSOCIATED_PORT(GET_PORT_INDEX(sw, port), type);
}



/*****************************************************************************/
/** fmUnpackAclAssociatedPort
 * \ingroup intAcl
 *
 * \desc            Returns a (port, type) tuple given its bit position in
 *                  an ACL's associatedPorts array.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bitNo is the bit position.
 *
 * \param[out]      tuple points to a caller-supplied location to receive
 *                  the the port number and acl type.
 *
 * \return          Bit position of the tuple in the array.
 *
 *****************************************************************************/
void fmUnpackAclAssociatedPort(fm_int   sw,
                               fm_int   bitNo,
                               fm_aclPortAndType * tuple)
{
    tuple->port = GET_LOGICAL_PORT(sw, FM_UNPACK_ASSOCIATED_PORT(bitNo));
    tuple->type = FM_UNPACK_ASSOCIATED_TYPE(bitNo);
}



/*****************************************************************************/
/** fmGetAclAssociatedPort
 * \ingroup intAcl
 *
 * \desc            Returns a bit in an ACL's associated port bit array.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       aclEntry points to the entry in the ACL tree.
 *
 * \param[in]       port is the port number.
 *
 * \param[in]       type is the acl type (INGRESS or EGRESS).
 * 
 * \param[out]      bitValue points to a caller-supplied variable to receive
 *                  the value of the specified bit (TRUE or FALSE).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetAclAssociatedPort(fm_int     sw,
                                 fm_acl   * aclEntry,
                                 fm_int     port,
                                 fm_aclType type,
                                 fm_bool  * bitValue)
{
    fm_int      bitNo;
    fm_status   err;

    bitNo = fmPackAclAssociatedPort(sw, port, type);

    err = fmGetBitArrayBit(&aclEntry->associatedPorts, bitNo, bitValue);

    return err;

}   /* end fmGetAclAssociatedPort */




/*****************************************************************************/
/** fmSetAclAssociatedPort
 * \ingroup intAcl
 *
 * \desc            Sets a bit in an ACL's associated port bit array.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       aclEntry points to the entry in the ACL tree.
 *
 * \param[in]       port is the port number.
 *
 * \param[in]       type is the acl type (INGRESS or EGRESS).
 * 
 * \param[in]       bitValue is TRUE if the bit is to be set, or FALSE
 *                  if it is to be cleared.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetAclAssociatedPort(fm_int     sw,
                                 fm_acl   * aclEntry,
                                 fm_int     port,
                                 fm_aclType type,
                                 fm_bool    bitValue)
{
    fm_int      bitNo;
    fm_status   err;

    if (!fmIsCardinalPort(sw, port))
    {
        FM_LOG_ERROR(FM_LOG_CAT_ACL,
                     "port %d is not a cardinal port\n",
                     port);
        return FM_ERR_INVALID_PORT;
    }

    bitNo = fmPackAclAssociatedPort(sw, port, type);

    err = fmSetBitArrayBit(&aclEntry->associatedPorts, bitNo, bitValue);

    return err;

}   /* end fmSetAclAssociatedPort */




/*****************************************************************************/
/** fmIsVNTunnelInUseByACLs
 * \ingroup intAcl
 *
 * \desc            Returns TRUE if one or more ACL rules reference the
 *                  specified virtual network tunnel, FALSE if not.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel ID.
 *
 * \param[out]      inUse points to caller-provided storage into which the
 *                  function writes TRUE if the tunnel is in use, FALSE if not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmIsVNTunnelInUseByACLs(fm_int sw, fm_int tunnelId, fm_bool *inUse)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_ACL,
                  "sw = %d, tunnelId = %d, inUse = %p\n",
                  sw,
                  tunnelId,
                  (void *) inUse );

    switchPtr = GET_SWITCH_PTR(sw);

    FM_TAKE_ACL_LOCK(sw);

    if (switchPtr->IsVNTunnelInUseByACLs != NULL)
    {
        err = switchPtr->IsVNTunnelInUseByACLs(sw, tunnelId, inUse);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    else
    {
        *inUse = FALSE;
    }

ABORT:
    FM_DROP_ACL_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmIsVNTunnelInUseByACLs */




/*****************************************************************************/
/** fmGetAclFfuRuleUsage
 * \ingroup intAcl
 *
 * \desc            Returns the number of FFU rules currently used by the ACL
 *                  subsystem and also the number of rules available. This
 *                  service is internally used by the mailbox API. The caller
 *                  must take the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      ffuRuleUsed points to caller-provided storage into
 *                  which the function writes the number of FFU rules currently
 *                  configured in the system.
 *
 * \param[out]      ffuRuleAvailable points to caller-provided storage into
 *                  which the function writes the number of FFU rules still
 *                  available.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetAclFfuRuleUsage(fm_int  sw,
                               fm_int *ffuRuleUsed,
                               fm_int *ffuRuleAvailable)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_ACL,
                  "sw = %d, ffuRuleUsed = %p, ffuRuleAvailable = %p\n",
                  sw,
                  (void *) ffuRuleUsed,
                  (void *) ffuRuleAvailable );

    switchPtr = GET_SWITCH_PTR(sw);

    FM_TAKE_ACL_LOCK(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetAclFfuRuleUsage,
                       sw,
                       ffuRuleUsed,
                       ffuRuleAvailable);

    FM_DROP_ACL_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmGetAclFfuRuleUsage */

