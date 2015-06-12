/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_trigger.c 
 * Creation Date:   October 24, 2008 
 * Description:     Application exposed functions for managing low-level
 *                  trigger resources.
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
/** fmInitTriggerCondition
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Initialize a trigger condition structure to default
 *                  values. Calling this function is not mandatory. The
 *                  default values are based on most common usage of
 *                  trigger conditions. Refer to the source code of this
 *                  function for default values. 
 *                  
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      cond is a pointer to the fm_triggerCondition structure to
 *                  be initialized.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if cond is not a valid pointer
 * 
 *****************************************************************************/
fm_status fmInitTriggerCondition(fm_int sw, fm_triggerCondition *cond)
{
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, cond = %p\n", 
                 sw, 
                 (void *) cond);

    FM_NOT_USED(sw);

    if (cond == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_ERR_INVALID_ARGUMENT);
    }

    cond->cfg.matchSA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchDA =             FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchHitSA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchHitDA =          FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchHitSADA =        FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchVlan =           FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchFFU =            FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchSwitchPri =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchEtherType =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;
    cond->cfg.matchDestGlort =      FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL;

    cond->cfg.matchFrameClassMask = (FM_TRIGGER_FRAME_CLASS_UCAST |
                                     FM_TRIGGER_FRAME_CLASS_MCAST |
                                     FM_TRIGGER_FRAME_CLASS_BCAST);

    cond->cfg.matchRoutedMask =     (FM_TRIGGER_SWITCHED_FRAMES |
                                     FM_TRIGGER_ROUTED_FRAMES);

    cond->cfg.matchFtypeMask =      FM_TRIGGER_FTYPE_NORMAL;

    cond->cfg.matchRandomNumber =   FALSE;
    cond->cfg.matchTx =             FM_TRIGGER_TX_MASK_DOESNT_CONTAIN;

    cond->cfg.rxPortset =           FM_PORT_SET_NONE;
    cond->cfg.txPortset =           FM_PORT_SET_NONE;
    cond->cfg.HAMask =              (FM_TRIGGER_HA_FORWARD_DGLORT |
                                     FM_TRIGGER_HA_FORWARD_FLOOD |
                                     FM_TRIGGER_HA_FORWARD_FID);

    /* Initialize all parameter values to 0 */
    FM_MEMSET_S(&cond->param, sizeof(cond->param), 0, sizeof(cond->param));

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_OK);

}   /* end fmInitTriggerCondition */




/*****************************************************************************/
/** fmInitTriggerAction
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Initializes a trigger action structure to default
 *                  values. Calling this function is not mandatory. The
 *                  default configuration is no action.
 *                  
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      action is a pointer to the fm_triggerAction structure to be
 *                  initialized.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if cond is not a valid pointer
 * 
 *****************************************************************************/
fm_status fmInitTriggerAction(fm_int sw, fm_triggerAction *action)
{
    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, action = %p\n", 
                 sw, 
                 (void *) action);

    FM_NOT_USED(sw);

    if (action == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_ERR_INVALID_ARGUMENT);
    }

    action->cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_ASIS;
    action->cfg.trapAction =       FM_TRIGGER_TRAP_ACTION_ASIS;
    action->cfg.mirrorAction =     FM_TRIGGER_MIRROR_ACTION_NONE;
    action->cfg.switchPriAction =  FM_TRIGGER_SWPRI_ACTION_ASIS;
    action->cfg.vlan1Action =      FM_TRIGGER_VLAN_ACTION_ASIS;
    action->cfg.learningAction =   FM_TRIGGER_LEARN_ACTION_ASIS;
    action->cfg.rateLimitAction =  FM_TRIGGER_RATELIMIT_ACTION_ASIS;

    /* Initialize all parameter values to 0 */
    FM_MEMSET_S(&action->param, sizeof(action->param), 0, sizeof(action->param));

    /* Initialize the portset to none */
    action->param.newDestPortset = FM_PORT_SET_NONE;
    action->param.dropPortset    = FM_PORT_SET_NONE;

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_OK);

}   /* end fmInitTriggerAction */




/*****************************************************************************/
/** fmCreateTrigger
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Create a new trigger for exclusive use by the
 *                  application. It is up to the application to
 *                  ensure that such triggers do not modify any core
 *                  API functionality. 
 *                  
 * \note            The created trigger will have an invalid condition and an
 *                  empty action until the ''fmSetTriggerCondition'' and
 *                  ''fmSetTriggerAction'' are called.
 *                  
 * \note            API internal triggers are created with group IDs lower
 *                  than 10000. In order not to disrupt API behavior, it is
 *                  strongly suggested that the triggers created by the
 *                  application be created with group IDs above 10000.
 *                  
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group in which the trigger should be created. 
 *                  The rules created within a group are mutually
 *                  exclusive, i.e. only one can hit. In the event that two
 *                  rules in two different groups have conflicting actions,
 *                  the group with the lowest precedence wins.
 * 
 * \param[in]       rule is the rule number within a group. Lowered numbered
 *                  rules have more precedence. 
 * 
 * \return          FM_OK if successful.
 *                  FM_ERR_ALREADY_EXISTS a trigger already exists for the
 *                  provided group/rule combination.
 *                  FM_ERR_TRIGGER_UNAVAILABLE if there are no more free
 *                  triggers.
 * 
 *****************************************************************************/
fm_status fmCreateTrigger(fm_int sw, fm_int group, fm_int rule)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d\n",
                 sw,
                 group, 
                 rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->CreateTrigger, 
                       sw, 
                       group, 
                       rule, 
                       FALSE, 
                       NULL);

    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmCreateTrigger */




/*****************************************************************************/
/** fmDeleteTrigger
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Delete a trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group in which the trigger to delete is
 *                  located. 
 * 
 * \param[in]       rule is the rule number within a group to delete. If this
 *                  was the last rule in a group, this group simply doesn't
 *                  exist anymore.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND a trigger does not exist for the provided
 *                  group/rule combination.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is
 *                  internal (internal triggers cannot be modified/deleted). 
 * 
 *****************************************************************************/
fm_status fmDeleteTrigger(fm_int sw, fm_int group, fm_int rule)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d\n",
                 sw,
                 group, 
                 rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteTrigger, sw, group, rule, FALSE);

    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmDeleteTrigger */




/*****************************************************************************/
/** fmSetTriggerCondition
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Set the conditions for a given trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to config.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to config.
 *
 * \param[in]       cond is a pointer to the condition vector to apply for
 *                  this trigger.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if condition is NULL.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is
 *                  internal (internal triggers cannot be modified/deleted).
 * 
 *****************************************************************************/
fm_status fmSetTriggerCondition(fm_int                     sw, 
                                fm_int                     group, 
                                fm_int                     rule, 
                                const fm_triggerCondition *cond)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, cond = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) cond);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (cond == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetTriggerCondition, 
                       sw, 
                       group, 
                       rule, 
                       cond,
                       FALSE);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmSetTriggerCondition */




/*****************************************************************************/
/** fmSetTriggerAction
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Set the actions for a given trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to configure.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to configure.
 *
 * \param[in]       action is a pointer to the action vector to apply for
 *                  this trigger.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if action is NULL.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is
 *                  internal (internal triggers cannot be modified/deleted).
 * 
 *****************************************************************************/
fm_status fmSetTriggerAction(fm_int                  sw, 
                             fm_int                  group, 
                             fm_int                  rule, 
                             const fm_triggerAction *action)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, action = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) action);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (action == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetTriggerAction, 
                       sw, 
                       group, 
                       rule, 
                       action,
                       FALSE);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmSetTriggerAction */




/*****************************************************************************/
/** fmGetTrigger
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Get the condition and action configurations for a trigger. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the trigger group ID of the trigger to retrieve.
 * 
 * \param[in]       rule is the trigger rule ID (within group) of the trigger
 *                  to retrieve.
 * 
 * \param[out]      cond is a pointer to the caller-supplied storage where
 *                  this trigger's condition vector should be stored. 
 * 
 * \param[out]      action is a pointer to the caller-supplied storage where
 *                  this trigger's action vector should be stored. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if cond or action is NULL.
 * 
 *****************************************************************************/
fm_status fmGetTrigger(fm_int               sw, 
                       fm_int               group, 
                       fm_int               rule, 
                       fm_triggerCondition *cond,
                       fm_triggerAction *   action)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, action = %p\n",
                 sw,
                 group, 
                 rule,
                 (void *) action);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (cond == NULL || action == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetTrigger, sw, group, rule, cond, action);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTrigger */




/*****************************************************************************/
/** fmSetTriggerRateLimiter
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Set a rate limiter's configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rateLimiterId is the rate limiter ID (handle) to set.
 * 
 * \param[in]       cfg points to the configuration to apply to the rate limiter.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if cfg is null.
 * \return          FM_ERR_NOT_FOUND if this rate limiter handle does not exist.
 * \return          FM_ERR_INTERNAL_RESOURCE if the rate limiter is internal
 *                  (internal resources cannot be modified or deleted).
 *
 *****************************************************************************/
fm_status fmSetTriggerRateLimiter(fm_int             sw,
                                  fm_int             rateLimiterId,
                                  fm_rateLimiterCfg *cfg)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, rateLimiterId = %d, capacity = %d, rate = %d\n",
                 sw,
                 rateLimiterId, 
                 cfg ? cfg->capacity : 0,
                 cfg ? cfg->rate : 0);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (cfg == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetTriggerRateLimiter, 
                       sw, 
                       rateLimiterId, 
                       cfg,
                       FALSE);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmSetTriggerRateLimiter */




/*****************************************************************************/
/** fmGetTriggerRateLimiter
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Get a rate limiter's configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rateLimiterId is the rate limiter ID (handle) to set.
 * 
 * \param[out]      cfg is a pointer to the caller-supplied storage where
 *                  the configuration of the rate limiter should be stored. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if this rate limiter handle does not exist.
 * \return          FM_ERR_INVALID_ARGUMENT if cfg is NULL.
 *
 *****************************************************************************/
fm_status fmGetTriggerRateLimiter(fm_int sw,
                                  fm_int rateLimiterId,
                                  fm_rateLimiterCfg *cfg)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, rateLimiterId = %d, cfg = %p\n",
                 sw,
                 rateLimiterId,
                 (void *) cfg);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (cfg == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetTriggerRateLimiter, 
                       sw, 
                       rateLimiterId, 
                       cfg);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerRateLimiter */




/*****************************************************************************/
/** fmAllocateTriggerResource
 * \ingroup trigger
 *
 * \chips           FM10000
 *
 * \desc            Allocate a resource for use with triggers. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to allocate.
 * 
 * \param[out]      value is a pointer to the the caller-supplied storage
 *                  where the value for the allocated resource should
 *                  be stored. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_FREE_TRIG_RES if there are no more free
 *                  resources for the specified resource type. 
 * \return          FM_ERR_INVALID_ARGUMENT if the value is NULL or
 *                  if the resource type does not exist.
 *
 *****************************************************************************/
fm_status fmAllocateTriggerResource(fm_int sw, 
                                    fm_triggerResourceType res, 
                                    fm_uint32 *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %p\n",
                 sw,
                 res, 
                 (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->AllocateTriggerResource, 
                       sw, 
                       res, 
                       value, 
                       FALSE);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmAllocateTriggerResource */




/*****************************************************************************/
/** fmFreeTriggerResource
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Free a trigger resource. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to free.
 * 
 * \param[out]      value is the the value of the resource type to free.
 *                  
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type
 *                  does not exist.
 * \return          FM_ERR_INTERNAL_RESOURCE if the resource is internal (
 *                  internal resources cannot be deleted). 
 *
 *****************************************************************************/
fm_status fmFreeTriggerResource(fm_int sw, 
                                fm_triggerResourceType res, 
                                fm_uint32 value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %d\n",
                 sw,
                 res, 
                 value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->FreeTriggerResource, 
                       sw, 
                       res, 
                       value, 
                       FALSE);

    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmFreeTriggerResource */




/*****************************************************************************/
/** fmGetTriggerResourceFirst
 * \ingroup trigger
 * 
 * \chips           FM10000
 * 
 * \desc            Find the first resource allocated for a given type.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to get.
 * 
 * \param[out]      value is a pointer to the caller-supplied storage
 *                  where the first trigger resource of type res should be
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger resource.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type is invalid
 *                  or value pointer is NULL. 
 *
 *****************************************************************************/
fm_status fmGetTriggerResourceFirst(fm_int sw, 
                                    fm_triggerResourceType res, 
                                    fm_uint32 *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, value = %p\n",
                 sw,
                 res, 
                 (void *) value );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( (res >= FM_TRIGGER_RES_MAX ) || (value == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetTriggerResourceFirst, sw, res, value);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerResourceFirst */




/*****************************************************************************/
/** fmGetTriggerResourceNext
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Find the next resource allocated for a given type.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       res is the resource type to get.
 * 
 * \param[in]       curValue is the current resource value of type res.
 * 
 * \param[out]      nextValue is a pointer to the caller-supplied storage
 *                  where the next trigger resource of type res should be
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger resource.
 * \return          FM_ERR_INVALID_ARGUMENT if the resource type is invalid, or
 *                  if curValue does not exit, or if nextValue pointer is NULL. 
 *
 *****************************************************************************/
fm_status fmGetTriggerResourceNext(fm_int sw, 
                                   fm_triggerResourceType res, 
                                   fm_uint32 curValue,
                                   fm_uint32 *nextValue)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, res = %d, curValue = %d, nextValue = %p\n",
                 sw,
                 res,
                 curValue, 
                 (void *) nextValue );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( (res >= FM_TRIGGER_RES_MAX ) || (nextValue == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetTriggerResourceNext, 
                       sw, 
                       res, 
                       curValue, 
                       nextValue);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerResourceNext */




/*****************************************************************************/
/** fmGetTriggerFirst
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Find the first trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      group is a pointer to the caller-supplied storage where
 *                  the first trigger's group should be stored. 
 * 
 * \param[out]      rule is a pointer to the caller-supplied storage where
 *                  the first trigger's rule should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there is no trigger.
 * \return          FM_ERR_INVALID_ARGUMENT if the group or rule is NULL. 
 *
 *****************************************************************************/
fm_status fmGetTriggerFirst(fm_int sw, fm_int *group, fm_int *rule)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %p, rule = %p\n",
                 sw,
                 (void *) group, 
                 (void *) rule );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( (group == NULL) || (rule == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetTriggerFirst, sw, group, rule);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerFirst */




/*****************************************************************************/
/** fmGetTriggerNext
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Get the trigger after the specified trigger.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       curGroup is the group of the current trigger. 
 * 
 * \param[in]       curRule is the rule of the current trigger 
 * 
 * \param[out]      nextGroup is a pointer to the caller-supplied storage
 *                  where the next trigger's group should be stored. 
 * 
 * \param[out]      nextRule is a pointer to the caller-supplied storage
 *                  where the next trigger's rule should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_NO_MORE if there is no more trigger.
 * \return          FM_ERR_INVALID_ARGUMENT if the nextGroup or nextRule
 *                  is NULL. 
 *
 *****************************************************************************/
fm_status fmGetTriggerNext(fm_int sw, 
                           fm_int curGroup, 
                           fm_int curRule,
                           fm_int *nextGroup, 
                           fm_int *nextRule)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, curGroup = %d, curRule = %d, "
                 "nextGroup = %p, nextRule = %p\n",
                 sw,
                 curGroup, 
                 curRule,
                 (void *) nextGroup,
                 (void *) nextRule);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( (nextGroup == NULL) || (nextRule == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetTriggerNext, 
                       sw, 
                       curGroup, 
                       curRule, 
                       nextGroup, 
                       nextRule);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerNext */




/*****************************************************************************/
/** fmSetTriggerAttribute
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Set a trigger attribute.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group of the trigger. 
 * 
 * \param[in]       rule is the rule of the trigger. 
 * 
 * \param[in]       attr is the trigger (See ''Trigger Attributes'')
 *                  attribute to set. 
 * 
 * \param[in]       value points to the attribute value to set. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the attribute does not exist.
 * \return          FM_ERR_INTERNAL_RESOURCE if the trigger is
 *                  internal (internal triggers cannot be modified/deleted).
 *
 *****************************************************************************/
fm_status fmSetTriggerAttribute(fm_int sw, 
                                fm_int group, 
                                fm_int rule,
                                fm_int attr, 
                                void *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, "
                 "attr = %d, value = %p\n",
                 sw,
                 group, 
                 rule,
                 attr,
                 (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->SetTriggerAttribute, 
                       sw, 
                       group, 
                       rule,
                       attr, 
                       value,
                       FALSE);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmSetTriggerAttribute */




/*****************************************************************************/
/** fmGetTriggerAttribute
 * \ingroup trigger
 * 
 * \chips           FM10000
 *
 * \desc            Get a trigger attribute.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       group is the group of the trigger. 
 * 
 * \param[in]       rule is the rule of the trigger. 
 * 
 * \param[in]       attr is the trigger (See ''Trigger Attributes'')
 *                  attribute to get. 
 * 
 * \param[in]       value is a pointer to the caller-supplied storage where
 *                  the value of the attribute should be stored. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_TRIG if group/rule is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the attribute does not exist. 
 *
 *****************************************************************************/
fm_status fmGetTriggerAttribute(fm_int sw, 
                                fm_int group, 
                                fm_int rule,
                                fm_int attr, 
                                void *value)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_TRIGGER, 
                 "sw = %d, group = %d, rule = %d, "
                 "attr = %d, value = %p\n",
                 sw,
                 group, 
                 rule,
                 attr,
                 (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (value == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TRIGGER, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetTriggerAttribute, 
                       sw, 
                       group, 
                       rule,
                       attr, 
                       value);

ABORT:
    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, err);

}   /* end fmGetTriggerAttribute */




/*****************************************************************************/
/** fmAllocateTrigger
 * \ingroup lowlevTrig4k
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Allocate a trigger for exclusive use by the
 *                  application. It is up to the application to ensure that 
 *                  such triggers do not modify any core API functionality.  
 *                  See the datasheet for information on programming
 *                  the trigger hardware registers.
 *                                                                      \lb\lb
 *                  Instead of this function, you may use 
 *                  ''fmAllocateTriggerExt'', which allows you to specify a 
 *                  name for the trigger for diagnostic purposes.
 *                  
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      trigger points to caller-supplied storage where this
 *                  function should place the hardware trigger number used as
 *                  an index into the trigger hardware registers
 *
 * \param[out]      info points to a structure that gives hints to the 
 *                  allocator. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_RATELIMITER_UNAVAILABLE if info specifies use of
 *                  a rate limiter, but there are no rate limiters available.
 *
 *****************************************************************************/
fm_status fmAllocateTrigger(fm_int sw, 
                            fm_int *trigger, 
                            fm_triggerRequestInfo *info)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TRIGGER,
                     "sw=%d, trigger=%p, info=%p\n",
                     sw, (void *) trigger, (void *) info);

    if (!trigger || !info)
    {
        FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->AllocateTrigger, 
                       sw, 
                       "NA", 
                       trigger, 
                       info);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TRIGGER, err);

}   /* end fmAllocateTrigger */




/*****************************************************************************/
/** fmAllocateTriggerExt
 * \ingroup lowlevTrig4k
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Allocate a trigger for exclusive use by the
 *                  application. It is up to the application to ensure that 
 *                  such triggers do not modify any core API functionality.  
 *                  See the datasheet for information on programming
 *                  the trigger hardware registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       name is the name of the trigger, which is used for 
 *                  diagnostic purposes only and is displayed by
 *                  ''fmDbgDumpTriggers''.
 *
 * \param[out]      trigger points to caller-supplied storage where this
 *                  function should place the hardware trigger number used as
 *                  an index into the trigger hardware registers
 *
 * \param[out]      info points to a structure that gives hints to the 
 *                  allocator. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_RATELIMITER_UNAVAILABLE if info specifies use of
 *                  a rate limiter, but there are no rate limiters available.
 *
 *****************************************************************************/
fm_status fmAllocateTriggerExt(fm_int   sw, 
                               fm_text  name, 
                               fm_int * trigger, 
                               fm_triggerRequestInfo *info)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TRIGGER,
                     "sw=%d, trigger=%p, info=%p\n",
                     sw, (void *) trigger, (void *) info);

    if (!trigger || !info || !name)
    {
        FM_LOG_EXIT(FM_LOG_CAT_TRIGGER, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->AllocateTrigger, 
                       sw, 
                       name, 
                       trigger, 
                       info);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TRIGGER, err);

}   /* end fmAllocateTriggerExt */




/*****************************************************************************/
/** fmFreeTrigger
 * \ingroup lowlevTrig4k
 *
 * \chips           FM3000, FM4000
 *
 * \desc            Frees a trigger previously allocated by the application
 *                  with a call to ''fmAllocateTrigger''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       trigger is the trigger number returned by a previous call 
 *                  to ''fmAllocateTrigger''. 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_TRIG if trigger is invalid.
 *
 *****************************************************************************/
fm_status fmFreeTrigger(fm_int sw, fm_int trigger)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TRIGGER,
                     "sw=%d, trigger=%d\n",
                     sw, trigger);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->FreeTrigger, sw, trigger);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TRIGGER, err);

}   /* end fmFreeTrigger */

