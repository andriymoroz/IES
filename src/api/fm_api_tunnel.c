/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_tunnel.c
 * Creation Date:   January 14, 2014
 * Description:     Tunnel API.
 *
 * Copyright (c) 2014, Intel Corporation
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
/** fmCreateTunnel
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Create a Tunnel Group. The Group will be created with an
 *                  empty rule and flow encap list. This function will return
 *                  a group handler that would be used to specify this set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      group points to caller-allocated storage where this
 *                  function should place the group handler.
 * 
 * \param[in]       tunnelParam refer to the group parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 * \return          FM_ERR_TUNNEL_TEP_SIZE if tep size is set on direct lookup
 *                  type.
 * \return          FM_ERR_TUNNEL_NO_FREE_GROUP if no free slot available.
 * \return          FM_ERR_TUNNEL_LOOKUP_FULL if lookup table is full.
 * \return          FM_ERR_TUNNEL_GLORT_FULL if te glort table is full.
 * \return          FM_ERR_LOG_PORT_REQUIRED if the specified tunnel engine
 *                  is not attached to a logical port.
 *
 *****************************************************************************/
fm_status fmCreateTunnel(fm_int sw, fm_int *group, fm_tunnelParam *tunnelParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateTunnel,
                       sw,
                       group,
                       tunnelParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmCreateTunnel */




/*****************************************************************************/
/** fmDeleteTunnel
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Delete a Tunnel Group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the handler that identify the entity to be deleted.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteTunnel(fm_int sw, fm_int group)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteTunnel,
                       sw,
                       group);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmDeleteTunnel */




/*****************************************************************************/
/** fmGetTunnel
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the Tunnel Group parameter as specified on creation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[out]      tunnelParam points to caller-allocated storage where this
 *                  function should place the group parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 *
 *****************************************************************************/
fm_status fmGetTunnel(fm_int sw, fm_int group, fm_tunnelParam *tunnelParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnel,
                       sw,
                       group,
                       tunnelParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnel */




/*****************************************************************************/
/** fmGetTunnelFirst
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the First Tunnel Group handler.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstGroup points to caller-allocated storage where this
 *                  function should place the first group handler.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer.
 * \return          FM_ERR_NO_MORE if no tunnel group are actually created.
 *
 *****************************************************************************/
fm_status fmGetTunnelFirst(fm_int sw, fm_int *firstGroup)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelFirst,
                       sw,
                       firstGroup);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelFirst */




/*****************************************************************************/
/** fmGetTunnelNext
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Find the next Tunnel Group handler.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       currentGroup is the last Group found by a previous
 *                  call to this function or to ''fmGetTunnelFirst''.
 *
 * \param[out]      nextGroup points to caller-allocated storage where this
 *                  function should place the next group handler.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 * \return          FM_ERR_NO_MORE if switch has no more tunnel group.
 *
 *****************************************************************************/
fm_status fmGetTunnelNext(fm_int sw, fm_int currentGroup, fm_int *nextGroup)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelNext,
                       sw,
                       currentGroup,
                       nextGroup);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelNext */




/*****************************************************************************/
/** fmAddTunnelEncapFlow
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Add a Tunnel Encap Flow to a group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id used to identify that entity.
 * 
 * \param[in]       field is an encap flow action mask (see 'fm_tunnelEncapFlow').
 * 
 * \param[in]       param is a parameter associated with the action (see
 *                  ''fm_tunnelEncapFlowParam'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 * \return          FM_ERR_TUNNEL_TYPE if configured tunnel type is invalid.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_UNSUPPORTED if count action is set on unshared encap flow.
 *
 *****************************************************************************/
fm_status fmAddTunnelEncapFlow(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   encapFlow,
                               fm_tunnelEncapFlow       field,
                               fm_tunnelEncapFlowParam *param)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d, field = 0x%x\n",
                     sw, group, encapFlow, field);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddTunnelEncapFlow,
                       sw,
                       group,
                       encapFlow,
                       field,
                       param);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmAddTunnelEncapFlow */




/*****************************************************************************/
/** fmDeleteTunnelEncapFlow
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Delete a Tunnel Encap Flow.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id used to identify that entity.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 * \return          FM_ERR_TUNNEL_IN_USE if provided encap flow is currently in
 *                  use by one or multiple tunnel rule.
 *
 *****************************************************************************/
fm_status fmDeleteTunnelEncapFlow(fm_int sw, fm_int group, fm_int encapFlow)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d\n",
                     sw, group, encapFlow);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteTunnelEncapFlow,
                       sw,
                       group,
                       encapFlow);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmDeleteTunnelEncapFlow */




/*****************************************************************************/
/** fmUpdateTunnelEncapFlow
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Update a Tunnel Encap Flow in a non disruptive and atomic
 *                  way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id to update.
 * 
 * \param[in]       field is an updated encap flow action mask
 *                  (see 'fm_tunnelEncapFlow').
 * 
 * \param[in]       param is a parameter associated with the updated action
 *                  (see ''fm_tunnelEncapFlowParam'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 * \return          FM_ERR_TUNNEL_TYPE if configured tunnel type is invalid.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_UNSUPPORTED if count action is set on unshared encap flow.
 *
 *****************************************************************************/
fm_status fmUpdateTunnelEncapFlow(fm_int                   sw,
                                  fm_int                   group,
                                  fm_int                   encapFlow,
                                  fm_tunnelEncapFlow       field,
                                  fm_tunnelEncapFlowParam *param)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d, field = 0x%x\n",
                     sw, group, encapFlow, field);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateTunnelEncapFlow,
                       sw,
                       group,
                       encapFlow,
                       field,
                       param);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmUpdateTunnelEncapFlow */




/*****************************************************************************/
/** fmGetTunnelEncapFlow
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the Tunnel Encap Flow action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id to retrieve.
 * 
 * \param[out]      field points to caller-allocated storage where this
 *                  function should place the encap flow action mask.
 * 
 * \param[out]      param points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 *
 *****************************************************************************/
fm_status fmGetTunnelEncapFlow(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   encapFlow,
                               fm_tunnelEncapFlow *     field,
                               fm_tunnelEncapFlowParam *param)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d\n",
                     sw, group, encapFlow);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelEncapFlow,
                       sw,
                       group,
                       encapFlow,
                       field,
                       param);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelEncapFlow */




/*****************************************************************************/
/** fmGetTunnelEncapFlowFirst
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the First Tunnel Encap Flow action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[out]      firstEncapFlow points to caller-allocated storage where this
 *                  function should place the first encap flow id retrieved.
 * 
 * \param[out]      field points to caller-allocated storage where this
 *                  function should place the encap flow action mask.
 * 
 * \param[out]      param points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 * \return          FM_ERR_NO_MORE if no encap flow are actually created.
 *
 *****************************************************************************/
fm_status fmGetTunnelEncapFlowFirst(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int *                 firstEncapFlow,
                                    fm_tunnelEncapFlow *     field,
                                    fm_tunnelEncapFlowParam *param)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d\n",
                     sw, group);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelEncapFlowFirst,
                       sw,
                       group,
                       firstEncapFlow,
                       field,
                       param);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelEncapFlowFirst */




/*****************************************************************************/
/** fmGetTunnelEncapFlowNext
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Find the next Tunnel Encap Flow action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       currentEncapFlow is the last encap flow found by a previous
 *                  call to this function or to ''fmGetTunnelEncapFlowFirst''.
 * 
 * \param[out]      nextEncapFlow points to caller-allocated storage where this
 *                  function should place the next encap flow id retrieved.
 * 
 * \param[out]      field points to caller-allocated storage where this
 *                  function should place the encap flow action mask.
 * 
 * \param[out]      param points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 * \return          FM_ERR_NOT_FOUND if currentEncapFlow is invalid.
 * \return          FM_ERR_NO_MORE if switch has no more encap flow.
 *
 *****************************************************************************/
fm_status fmGetTunnelEncapFlowNext(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   currentEncapFlow,
                                   fm_int *                 nextEncapFlow,
                                   fm_tunnelEncapFlow *     field,
                                   fm_tunnelEncapFlowParam *param)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, currentEncapFlow = %d\n",
                     sw, group, currentEncapFlow);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelEncapFlowNext,
                       sw,
                       group,
                       currentEncapFlow,
                       nextEncapFlow,
                       field,
                       param);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelEncapFlowNext */




/*****************************************************************************/
/** fmAddTunnelRule
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Add a Tunnel rule to a group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id used to identify that entity.
 * 
 * \param[in]       cond is a rule condition mask (see 'fm_tunnelCondition').
 *                  This is only used on rules inserted into hash lookup
 *                  group type.
 * 
 * \param[in]       condParam refer to the parameter associated with the
 *                  match condition (see ''fm_tunnelConditionParam'').
 * 
 * \param[in]       action is a rule action mask (see 'fm_tunnelAction').
 * 
 * \param[in]       actParam refer to the parameter associated with the action
 *                  (see ''fm_tunnelActionParam'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_TUNNEL_CONFLICT if tunnel condition does not match
 *                  hash lookup tunnel group condition.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_TUNNEL_GROUP_FULL if no more rule can be added on this
 *                  tunnel group.
 * \return          FM_ERR_TUNNEL_NO_ENCAP_FLOW if specified encap flow is invalid.
 * \return          FM_ERR_TUNNEL_BIN_FULL if no more entries can be inserted into
 *                  that specific hash lookup bin.
 *
 *****************************************************************************/
fm_status fmAddTunnelRule(fm_int                   sw,
                          fm_int                   group,
                          fm_int                   rule,
                          fm_tunnelCondition       cond,
                          fm_tunnelConditionParam *condParam,
                          fm_tunnelAction          action,
                          fm_tunnelActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d, cond = 0x%x, action = 0x%x\n",
                     sw, group, rule, cond, action);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddTunnelRule,
                       sw,
                       group,
                       rule,
                       cond,
                       condParam,
                       action,
                       actParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmAddTunnelRule */




/*****************************************************************************/
/** fmDeleteTunnelRule
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Delete a Tunnel rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id used to identify that entity.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteTunnelRule(fm_int sw, fm_int group, fm_int rule)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteTunnelRule,
                       sw,
                       group,
                       rule);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmDeleteTunnelRule */




/*****************************************************************************/
/** fmUpdateTunnelRule
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Update a Tunnel rule in a non disruptive and atomic way.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to update.
 * 
 * \param[in]       cond is an updated rule condition mask
 *                  (see 'fm_tunnelCondition'). This is only used on rules
 *                  inserted into hash lookup group type.
 * 
 * \param[in]       condParam refer to the updated parameter associated with the
 *                  match condition (see ''fm_tunnelConditionParam'').
 * 
 * \param[in]       action is an updated rule action mask (see 'fm_tunnelAction').
 * 
 * \param[in]       actParam refer to the updated parameter associated with the
 *                  action (see ''fm_tunnelActionParam'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_TUNNEL_CONFLICT if tunnel condition does not match
 *                  hash lookup tunnel group condition.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_TUNNEL_NO_ENCAP_FLOW if specified encap flow is invalid.
 * \return          FM_ERR_TUNNEL_BIN_FULL if no more entries can be inserted into
 *                  that specific hash lookup bin.
 *
 *****************************************************************************/
fm_status fmUpdateTunnelRule(fm_int                   sw,
                             fm_int                   group,
                             fm_int                   rule,
                             fm_tunnelCondition       cond,
                             fm_tunnelConditionParam *condParam,
                             fm_tunnelAction          action,
                             fm_tunnelActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d, cond = 0x%x, action = 0x%x\n",
                     sw, group, rule, cond, action);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateTunnelRule,
                       sw,
                       group,
                       rule,
                       cond,
                       condParam,
                       action,
                       actParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmUpdateTunnelRule */




/*****************************************************************************/
/** fmGetTunnelRule
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the Tunnel rule condition, action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to retrieve.
 * 
 * \param[out]      cond points to caller-allocated storage where this
 *                  function should place the condition mask.
 * 
 * \param[out]      condParam points to caller-allocated storage where this
 *                  function should place the condition parameters.
 * 
 * \param[out]      action points to caller-allocated storage where this
 *                  function should place the action mask.
 * 
 * \param[out]      actParam points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 *
 *****************************************************************************/
fm_status fmGetTunnelRule(fm_int                   sw,
                          fm_int                   group,
                          fm_int                   rule,
                          fm_tunnelCondition *     cond,
                          fm_tunnelConditionParam *condParam,
                          fm_tunnelAction *        action,
                          fm_tunnelActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelRule,
                       sw,
                       group,
                       rule,
                       cond,
                       condParam,
                       action,
                       actParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelRule */




/*****************************************************************************/
/** fmGetTunnelRuleFirst
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get the First Tunnel rule condition, action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[out]      firstRule points to caller-allocated storage where this
 *                  function should place the first rule id retrieved.
 * 
 * \param[out]      cond points to caller-allocated storage where this
 *                  function should place the condition mask.
 * 
 * \param[out]      condParam points to caller-allocated storage where this
 *                  function should place the condition parameters.
 * 
 * \param[out]      action points to caller-allocated storage where this
 *                  function should place the action mask.
 * 
 * \param[out]      actParam points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 * \return          FM_ERR_NO_MORE if no rule are actually created.
 *
 *****************************************************************************/
fm_status fmGetTunnelRuleFirst(fm_int                   sw,
                               fm_int                   group,
                               fm_int *                 firstRule,
                               fm_tunnelCondition *     cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction *        action,
                               fm_tunnelActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d\n",
                     sw, group);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelRuleFirst,
                       sw,
                       group,
                       firstRule,
                       cond,
                       condParam,
                       action,
                       actParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelRuleFirst */




/*****************************************************************************/
/** fmGetTunnelRuleNext
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Find the next Tunnel rule condition, action and parameters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       currentRule is the last rule id found by a previous
 *                  call to this function or to ''fmGetTunnelRuleFirst''.
 * 
 * \param[out]      nextRule points to caller-allocated storage where this
 *                  function should place the next rule id retrieved.
 * 
 * \param[out]      cond points to caller-allocated storage where this
 *                  function should place the condition mask.
 * 
 * \param[out]      condParam points to caller-allocated storage where this
 *                  function should place the condition parameters.
 * 
 * \param[out]      action points to caller-allocated storage where this
 *                  function should place the action mask.
 * 
 * \param[out]      actParam points to caller-allocated storage where this
 *                  function should place the action parameters.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group is invalid.
 * \return          FM_ERR_NOT_FOUND if currentRule is invalid.
 * \return          FM_ERR_NO_MORE if switch has no more rule.
 *
 *****************************************************************************/
fm_status fmGetTunnelRuleNext(fm_int                   sw,
                              fm_int                   group,
                              fm_int                   currentRule,
                              fm_int *                 nextRule,
                              fm_tunnelCondition *     cond,
                              fm_tunnelConditionParam *condParam,
                              fm_tunnelAction *        action,
                              fm_tunnelActionParam *   actParam)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, currentRule = %d\n",
                     sw, group, currentRule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelRuleNext,
                       sw,
                       group,
                       currentRule,
                       nextRule,
                       cond,
                       condParam,
                       action,
                       actParam);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelRuleNext */




/*****************************************************************************/
/** fmGetTunnelRuleCount
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  ''FM_TUNNEL_COUNT'' tunnel rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to retrieve.
 * 
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_tunnelCounters'' where this function should place the
 *                  counter values.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have count action.
 *
 *****************************************************************************/
fm_status fmGetTunnelRuleCount(fm_int             sw,
                               fm_int             group,
                               fm_int             rule,
                               fm_tunnelCounters *counters)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelRuleCount,
                       sw,
                       group,
                       rule,
                       counters);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelRuleCount */




/*****************************************************************************/
/** fmGetTunnelEncapFlowCount
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  ''FM_TUNNEL_ENCAP_FLOW_COUNTER'' encap flow action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id to retrieve.
 * 
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_tunnelCounters'' where this function should place the
 *                  counter values.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the encap flow does not have count action.
 *
 *****************************************************************************/
fm_status fmGetTunnelEncapFlowCount(fm_int             sw,
                                    fm_int             group,
                                    fm_int             encapFlow,
                                    fm_tunnelCounters *counters)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d\n",
                     sw, group, encapFlow);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelEncapFlowCount,
                       sw,
                       group,
                       encapFlow,
                       counters);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelEncapFlowCount */




/*****************************************************************************/
/** fmGetTunnelRuleUsed
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Retrieve the used bit associated with a specified tunnel
 *                  rule. This is only supported if the rule is part of a
 *                  direct lookup tunnel type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to retrieve.
 * 
 * \param[out]      used points to a caller-allocated storage where this
 *                  function should set the current rule usage.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if the rule is not part of a direct
 *                  lookup tunnel type.
 *
 *****************************************************************************/
fm_status fmGetTunnelRuleUsed(fm_int   sw,
                              fm_int   group,
                              fm_int   rule,
                              fm_bool *used)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelRuleUsed,
                       sw,
                       group,
                       rule,
                       used);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelRuleUsed */




/*****************************************************************************/
/** fmResetTunnelRuleCount
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Reset the frame and octet counter associated with an
 *                  ''FM_TUNNEL_COUNT'' tunnel rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to reset.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have count action.
 *
 *****************************************************************************/
fm_status fmResetTunnelRuleCount(fm_int sw, fm_int group, fm_int rule)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetTunnelRuleCount,
                       sw,
                       group,
                       rule);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmResetTunnelRuleCount */




/*****************************************************************************/
/** fmResetTunnelEncapFlowCount
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Reset the frame and octet counter associated with an
 *                  ''FM_TUNNEL_ENCAP_FLOW_COUNTER'' encap flow action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       encapFlow is the encap flow id to reset.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or encap flow is invalid.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the encap flow does not have count action.
 *
 *****************************************************************************/
fm_status fmResetTunnelEncapFlowCount(fm_int sw,
                                      fm_int group,
                                      fm_int encapFlow)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, encapFlow = %d\n",
                     sw, group, encapFlow);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetTunnelEncapFlowCount,
                       sw,
                       group,
                       encapFlow);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmResetTunnelEncapFlowCount */




/*****************************************************************************/
/** fmResetTunnelRuleUsed
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Reset the used bit associated with a specified tunnel
 *                  rule. This is only supported if the rule is part of a
 *                  direct lookup tunnel type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id to reset.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if the rule is not part of a direct
 *                  lookup tunnel type.
 *
 *****************************************************************************/
fm_status fmResetTunnelRuleUsed(fm_int sw, fm_int group, fm_int rule)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d\n",
                     sw, group, rule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetTunnelRuleUsed,
                       sw,
                       group,
                       rule);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmResetTunnelRuleUsed */




/*****************************************************************************/
/** fmSetTunnelAttribute
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Set a Tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id on which to operate.
 * 
 * \param[in]       attr is the Tunnel attribute (see 'Tunnel Attributes') to
 *                  set.
 * 
 * \param[in]       value points to the attribute value to set.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fmSetTunnelAttribute(fm_int sw,
                               fm_int group,
                               fm_int rule,
                               fm_int attr,
                               void * value)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d, attr = %d\n",
                     sw, group, rule, attr);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetTunnelAttribute,
                       sw,
                       group,
                       rule,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmSetTunnelAttribute */




/*****************************************************************************/
/** fmGetTunnelAttribute
 * \ingroup tunnel
 *
 * \chips           FM10000
 *
 * \desc            Get a Tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 * 
 * \param[in]       rule is the id on which to operate.
 * 
 * \param[in]       attr is the Tunnel attribute (see 'Tunnel Attributes') to
 *                  get.
 * 
 * \param[out]      value points to the attribute value to get.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fmGetTunnelAttribute(fm_int sw,
                               fm_int group,
                               fm_int rule,
                               fm_int attr,
                               void * value)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE,
                     "sw = %d, group = %d, rule = %d, attr = %d\n",
                     sw, group, rule, attr);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetTunnelAttribute,
                       sw,
                       group,
                       rule,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmGetTunnelAttribute */




/*****************************************************************************/
/** fmDbgDumpTunnel
 * \ingroup diagTunnel
 *
 * \chips           FM10000
 *
 * \desc            Display the tunnel engine status of a switch
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpTunnel(fm_int sw)
{
    fm_status       err;
    fm_switch *     switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_TE, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgDumpTunnel,
                       sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_TE, err);

}   /* end fmDbgDumpTunnel */
