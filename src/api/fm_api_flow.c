/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_flow.c
 * Creation Date:   July 20, 2010 
 * Description:     OpenFlow API interface.
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
/** fmCreateFlowTCAMTable
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Creates a flow table in the FFU TCAM. Calling this function
 *                  also automatically adds a low priority catch-all flow that
 *                  redirects traffic to the CPU port if the tableIndex does
 *                  not support priority (see ''FM_FLOW_TABLE_WITH_PRIORITY''), 
 *                  and the ''FM_FLOW_TABLE_WITH_DEFAULT_ACTION'' flow
 *                  attribute is enabled.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is used to order this table with respect to other
 *                  TCAM tables. The lower the tableIndex, the higher the
 *                  precedence. By default, TCAM tables are lower precedence
 *                  than the BST. tableIndex must be no larger than
 *                  ''FM_FLOW_MAX_TABLE_TYPE''.
 * 
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify flows that may be added to this table. See 
 *                  ''Flow Condition Masks'' for the definitions of each bit 
 *                  in the mask and ''Flow Condition Mask Examples''. For FM3000
 *                  and FM4000 devices, this argument is ignored and all
 *                  available conditions are enabled by default.
 * 
 * \param[in]       maxEntries is the size of the flow table and must be
 *                  no larger than FM4000_MAX_RULE_PER_FLOW_TABLE
 *                  (FM3000/FM4000 devices) or 
 *                  FM6000_MAX_RULE_PER_FLOW_TCAM_TABLE (FM6000 devices) or
 *                  ''FM10000_MAX_RULE_PER_FLOW_TABLE'' (FM10000 devices).
 * 
 * \param[in]       maxAction is not used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the specified condition is not
 *                  supported for this table.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 *
 *****************************************************************************/
fm_status fmCreateFlowTCAMTable(fm_int           sw, 
                                fm_int           tableIndex, 
                                fm_flowCondition condition,
                                fm_uint32        maxEntries,
                                fm_uint32        maxAction)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d, condition=%llx, maxEntries=%d, maxAction= %d\n", 
                     sw, 
                     tableIndex, 
                     (fm_uint64) condition,
                     maxEntries,
                     maxAction);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateFlowTCAMTable,
                       sw,
                       tableIndex,
                       condition,
                       maxEntries,
                       maxAction);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmCreateFlowTCAMTable */




/*****************************************************************************/
/** fmDeleteFlowTCAMTable
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes a flow table in the FFU TCAM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the index to remove.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if at least a flow is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fmDeleteFlowTCAMTable(fm_int sw, 
                                fm_int tableIndex)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d\n", 
                     sw, 
                     tableIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlowTCAMTable,
                       sw,
                       tableIndex);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlowTCAMTable */




/*****************************************************************************/
/** fmCreateFlowBSTTable
 * \ingroup flow
 *
 * \chips           FM6000
 *
 * \desc            Creates a flow table in the BST.
 * 
 * \note            The flow attribute ''FM_FLOW_TABLE_WITH_DEFAULT_ACTION''
 *                  must not be enabled, or this function will fail.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is used to order this table with respect to other
 *                  BST tables. The lower the tableIndex, the higher the
 *                  precedence. By default, TCAM tables are lower precedence
 *                  than the BST. tableIndex must be no larger than
 *                  ''FM_FLOW_MAX_TABLE_TYPE''.
 * 
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify flows that may be added to this table. See 
 *                  ''Flow Condition Masks'' for the definitions of each bit 
 *                  in the mask and ''Flow Condition Mask Examples''.
 * 
 * \param[in]       maxEntries is the size of the flow table and must be
 *                  no larger than FM6000_MAX_RULE_PER_FLOW_BST_TABLE
 *                  (FM6000 devices).
 * 
 * \param[in]       maxAction is not used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the specified condition is not
 *                  supported for this table.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 *
 *****************************************************************************/
fm_status fmCreateFlowBSTTable(fm_int           sw, 
                               fm_int           tableIndex, 
                               fm_flowCondition condition,
                               fm_uint32        maxEntries,
                               fm_uint32        maxAction)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d, condition=%llx, maxEntries=%d, maxAction= %d\n", 
                     sw, 
                     tableIndex, 
                     (fm_uint64) condition,
                     maxEntries,
                     maxAction);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateFlowBSTTable,
                       sw,
                       tableIndex,
                       condition,
                       maxEntries,
                       maxAction);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmCreateFlowBSTTable */




/*****************************************************************************/
/** fmDeleteFlowBSTTable
 * \ingroup flow
 *
 * \chips           FM6000
 *
 * \desc            Deletes a flow table in the BST.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the index to remove.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not used.
 *
 *****************************************************************************/
fm_status fmDeleteFlowBSTTable(fm_int sw, 
                                fm_int tableIndex)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d\n", 
                     sw, 
                     tableIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlowBSTTable,
                       sw,
                       tableIndex);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlowBSTTable */




/*****************************************************************************/
/** fmCreateFlowTETable
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Creates a flow table in the Tunnel Engine.
 * 
 * \note            The frame must be redirected by 
 *                  ''FM_FLOW_ACTION_REDIRECT_TUNNEL'' or
 *                  ''FM_FLOW_ACTION_BALANCE'' from TCAM table to be processed
 *                  in TE table.
 * 
 * \note            TE tables with a non-zero condition are based on TE hash
 *                  groups. Such tables do not support ''FM_FLOW_ACTION_COUNT'',
 *                  and support only exact matches (not masked).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is used to order this table with respect to other
 *                  TE tables. The lower the tableIndex, the higher the
 *                  precedence. 
 *                   
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify flows that may be added to this table. See 
 *                  ''Flow Condition Masks'' for the definitions of each bit 
 *                  in the mask and ''Flow Condition Mask Examples''.
 * 
 * \param[in]       maxEntries is the size of the flow table. 
 * 
 * \param[in]       maxAction is not used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the specified condition is not
 *                  supported for this table.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmCreateFlowTETable(fm_int           sw, 
                              fm_int           tableIndex, 
                              fm_flowCondition condition,
                              fm_uint32        maxEntries,
                              fm_uint32        maxAction)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d, condition=%llx, maxEntries=%d, maxAction= %d\n", 
                     sw, 
                     tableIndex, 
                     (fm_uint64) condition,
                     maxEntries,
                     maxAction);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateFlowTETable,
                       sw,
                       tableIndex,
                       condition,
                       maxEntries,
                       maxAction);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmCreateFlowTETable */




/*****************************************************************************/
/** fmDeleteFlowTETable
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Deletes a flow table in the Tunnel Engine.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the index to remove.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if at least a flow is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fmDeleteFlowTETable(fm_int sw, 
                              fm_int tableIndex)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw=%d, tableIndex=%d\n", 
                     sw, 
                     tableIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlowTETable,
                       sw,
                       tableIndex);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlowTETable */




/*****************************************************************************/
/** fmAddFlow
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a flow entry to the specified table.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which this flow should
 *                  be added.
 * 
 * \param[in]       priority is the priority for this flow within the table.
 *                  Flow Attribute ''FM_FLOW_TABLE_WITH_PRIORITY'' must be
 *                  set to activate this functionality on a tableIndex
 *                  basis. For BST and TE tables, this functionality is not
 *                  available.
 * 
 * \param[in]       precedence is the inter-table flow precedence. This is
 *                  currently not supported.
 * 
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify the flow. See ''Flow Condition Masks'' for the
 *                  definitions of each bit in the mask. For FM6000 and
 *                  FM10000, this mask must be a subset of condition mask 
 *                  selected for a table.
 *                  For BST and TE tables, this mask must equal the condition
 *                  mask selected for a table.
 * 
 * \param[in]       condVal points to a ''fm_flowValue'' structure containing
 *                  the values and masks to match against for the specified
 *                  condition. For FM6000 and FM10000, the flow specific masks 
 *                  must be subsets of masks specified for the table by the 
 *                  attribute ''FM_FLOW_TABLE_COND_MATCH_MASKS''.
 *                  For a BST table, all specified masks must be equal to 
 *                  masks defined for the table by the attribute
 *                  ''FM_FLOW_TABLE_COND_MATCH_MASKS''.
 * 
 * \param[in]       action is a bit mask of actions that will occur when a
 *                  packet is identified that matches the flow condition.
 *                  See ''Flow Action Masks'' for definitions of each bit in 
 *                  the mask.
 * 
 * \param[in]       param points to a ''fm_flowParam'' structure containing
 *                  values used by some actions.
 * 
 * \param[in]       flowState is the initial state of the flow (enabled or
 *                  not). A flow in a BST or TE table is always enabled.
 * 
 * \param[out]      flowId points to caller-allocated storage where this
 *                  function will store the handle identifying this flow entry.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex or action is not
 *                  supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_TABLE_FULL if there is no more room in the table to
 *                  add this flow.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if action is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_TUNNEL_CONFLICT if an argument is invalid.
 * \return          FM_ERR_ACL_COMPILE if the ACL compiler was unable to
 *                  produce a valid image from the current ACL configuration.
 *
 *****************************************************************************/
fm_status fmAddFlow(fm_int           sw, 
                    fm_int           tableIndex,
                    fm_uint16        priority,
                    fm_uint32        precedence, 
                    fm_flowCondition condition,
                    fm_flowValue *   condVal,
                    fm_flowAction    action,
                    fm_flowParam *   param,
                    fm_flowState     flowState,
                    fm_int *         flowId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, priority = %d, precedence = %d,"
                     "condition = %llx, condVal = %p, "
                     "action = %llx, param = %p, flowState=%d\n",
                     sw,
                     tableIndex,
                     priority,
                     precedence,
                     condition,
                     (void *) condVal,
                     action,
                     (void *) param,
                     flowState);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddFlow,
                       sw,
                       tableIndex,
                       priority,
                       precedence,
                       condition,
                       condVal,
                       action,
                       param,
                       flowState,
                       flowId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmAddFlow */




/*****************************************************************************/
/** fmGetFlow
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets a flow entry from the specified table.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which this flow should
 *                  be added.
 * 
 * \param[in]       flowId is the handle identifying this flow entry
 * 
 * \param[out]      flowCond points to caller-supplied storage where the flow
 *                  condition mask should be stored.
 * 
 * \param[out]      flowValue points to caller-supplied storage where the flow
 *                  condition values should be stored.
 * 
 * \param[out]      flowAction points to caller-supplied storage where the flow
 *                  action mask should be stored.
 * 
 * \param[out]      flowParam points to caller-supplied storage where the flow
 *                  action values should be stored.
 * 
 * \param[out]      priority points to caller-supplied storage where the flow
 *                  priority should be stored.
 * 
 * \param[out]      precedence points to caller-supplied storage where the flow
 *                  precedence should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex or action is not
 *                  supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if action is not valid.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 *
 *****************************************************************************/
fm_status fmGetFlow(fm_int             sw, 
                    fm_int             tableIndex,
                    fm_int             flowId,
                    fm_flowCondition * flowCond,
                    fm_flowValue *     flowValue,
                    fm_flowAction *    flowAction,
                    fm_flowParam *     flowParam,
                    fm_int *           priority,
                    fm_int *           precedence)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d\n",
                     sw,
                     tableIndex,
                     flowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlow,
                       sw,
                       tableIndex,
                       flowId,
                       flowCond,
                       flowValue,
                       flowAction,
                       flowParam,
                       priority,
                       precedence);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlow */




/*****************************************************************************/
/** fmGetFlowTableType
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets a flow table type associated with the specified table.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which this flow should
 *                  be added.
 * 
 * \param[out]      flowTableType points to user allocated storage
 *                  where this flow table type should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex or action is not
 *                  supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmGetFlowTableType(fm_int             sw, 
                             fm_int             tableIndex,
                             fm_flowTableType * flowTableType)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d",
                     sw,
                     tableIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowTableType,
                       sw,
                       tableIndex,
                       flowTableType);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowTableType */



/*****************************************************************************/
/** fmModifyFlow
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Modify a flow entry in the specified flow table. The flow
 *                  entry is updated atomically without affecting traffic.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 * 
 * \param[in]       flowId is the flow instance to be modified.
 * 
 * \param[in]       priority is the priority for this flow within the table.
 *                  This argument is only effective if the 
 *                  ''FM_FLOW_TABLE_WITH_PRIORITY'' flow attribute is 
 *                  enabled on the specified tableIndex.
 * 
 * \param[in]       precedence is the inter-table flow precedence. This is
 *                  currently not supported.
 * 
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify the flow. See ''Flow Condition Masks'' for the
 *                  definitions of each bit in the mask.
 * 
 * \param[in]       condVal points to a ''fm_flowValue'' structure containing
 *                  the values to match against for the specified condition.
 * 
 * \param[in]       action is a bit mask of actions that will occur when a
 *                  packet is identified that matches the flow condition.
 *                  See ''Flow Action Masks'' for definitions of each bit in 
 *                  the mask.
 * 
 * \param[in]       param points to a ''fm_flowParam'' structure containing
 *                  values used by some actions.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if flowId is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if action is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_TUNNEL_CONFLICT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmModifyFlow(fm_int           sw, 
                       fm_int           tableIndex,
                       fm_int           flowId,
                       fm_uint16        priority,
                       fm_uint32        precedence, 
                       fm_flowCondition condition,
                       fm_flowValue *   condVal,
                       fm_flowAction    action,
                       fm_flowParam *   param)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d, priority = %d,"
                     "precedence = %d, action = %llx, param = %p\n",
                     sw,
                     tableIndex,
                     flowId,
                     priority,
                     precedence,
                     action,
                     (void *) param);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ModifyFlow,
                       sw,
                       tableIndex,
                       flowId,
                       priority,
                       precedence,
                       condition,
                       condVal,
                       action,
                       param);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmModifyFlow */



/*****************************************************************************/
/** fmGetFlowFirst
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets the first created flow table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tableIndex points to caller-supplied memory where
 *                  the table instance to which the flow belongs should
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NO_MORE if no created tables have been found.
 *
 *****************************************************************************/
fm_status fmGetFlowFirst(fm_int   sw,
                         fm_int * tableIndex)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d\n",
                     sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowFirst,
                       sw,
                       tableIndex);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowFirst */




/*****************************************************************************/
/** fmGetFlowNext
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets the next created flow table after finding the first
 *                  one with a call to ''fmGetFlowFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       currentTable is the last table found by a previous call
 *                  made to ''fmGetFlowFirst''.
 *
 * \param[out]      nextTable points to caller-supplied memory where
 *                  the table instance to which the flow belongs should
 *                  stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NO_MORE if no created tables have been found.
 *
 *****************************************************************************/
fm_status fmGetFlowNext(fm_int   sw,
                        fm_int   currentTable,
                        fm_int * nextTable)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, currentTable = %d\n",
                     sw,
                     currentTable);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowNext,
                       sw,
                       currentTable,
                       nextTable);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowNext */





/*****************************************************************************/
/** fmGetFlowRuleFirst
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets the first created flow from a defined flow table.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       tableIndex is the flow table on which to operate.
 *
 * \param[out]      firstRule points to caller-supplied memory where
 *                  the firts in use flowId should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NO_MORE if no created rules have been found.
 *
 *****************************************************************************/
fm_status fmGetFlowRuleFirst(fm_int   sw,
                             fm_int   tableIndex,
                             fm_int * firstRule)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d\n",
                     sw,
                     tableIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowRuleFirst,
                       sw,
                       tableIndex,
                       firstRule);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowRuleFirst */




/*****************************************************************************/
/** fmGetFlowRuleNext
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Gets the next created flow from a defined flow table
 *                  after finding the first one with a call to ''fmGetFlowRuleFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       tableIndex is the flow table on which to operate.
 * 
 * \param[in]       currentRule is the last rule id found by a previous
 *                  call to ''fmGetFlowRuleFirst''
 *
 * \param[in]       nextRule points to caller-supplied memory where
 *                  the next rule instance should stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NO_MORE if no created rules have been found.
 *
 *****************************************************************************/
fm_status fmGetFlowRuleNext(fm_int   sw,
                            fm_int   tableIndex,
                            fm_int   currentRule,
                            fm_int * nextRule)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, currentRule = %d\n",
                     sw,
                     tableIndex,
                     currentRule);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowRuleNext,
                       sw,
                       tableIndex,
                       currentRule,
                       nextRule);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowRuleNext */






/*****************************************************************************/
/** fmDeleteFlow
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a flow entry from the specified table.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the flow table instance from which to delete
 *                  the flow.
 * 
 * \param[in]       flowId is the flow instance to be deleted.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not valid.
 * \return          FM_ERR_ACL_COMPILE if the ACL compiler was unable to
 *                  produce a valid image from the current ACL configuration.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if this flowId is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fmDeleteFlow(fm_int sw, fm_int tableIndex, fm_int flowId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d\n",
                     sw,
                     tableIndex,
                     flowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlow,
                       sw,
                       tableIndex,
                       flowId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlow */




/*****************************************************************************/
/** fmSetFlowState
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Change the state of a flow (Enable or Standby).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 * 
 * \param[in]       flowId is the flow instance.
 * 
 * \param[in]       flowState is the flow's new state (see ''fm_flowState'').
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex identifies a table that
 *                  does not support enabling/disabling individual flows.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if flowId is not valid.
 *
 *****************************************************************************/
fm_status fmSetFlowState(fm_int       sw, 
                         fm_int       tableIndex, 
                         fm_int       flowId, 
                         fm_flowState flowState)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d, flowState = %d\n",
                     sw,
                     tableIndex,
                     flowId,
                     flowState);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetFlowState,
                       sw,
                       tableIndex,
                       flowId,
                       flowState);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmSetFlowState */




/*****************************************************************************/
/** fmGetFlowCount
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve counters for a flow.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 * 
 * \param[in]       flowId is the flow instance.
 * 
 * \param[out]      counters points to caller-allocated storage where this
 *                  function will store the counter values.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex does not identify a
 *                  a flow table that supports counters.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if flowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *                  
 *****************************************************************************/
fm_status fmGetFlowCount(fm_int           sw, 
                         fm_int           tableIndex, 
                         fm_int           flowId,
                         fm_flowCounters *counters)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d, counters = %p\n",
                     sw,
                     tableIndex,
                     flowId,
                     (void *) counters);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowCount,
                       sw,
                       tableIndex,
                       flowId,
                       counters);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowCount */




/*****************************************************************************/
/** fmResetFlowCount
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Resets a flow's counters to zero.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 * 
 * \param[in]       flowId is the flow instance.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex identifies a flow table
 *                  that does not support counters.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if flowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmResetFlowCount(fm_int sw, 
                           fm_int tableIndex, 
                           fm_int flowId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d\n",
                     sw,
                     tableIndex,
                     flowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->ResetFlowCount,
                       sw,
                       tableIndex,
                       flowId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmResetFlowCount */




/*****************************************************************************/
/** fmGetFlowUsed
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Determine whether a packet has been received that matches
 *                  the specified flow. This function can be used to age out 
 *                  unused flow entries.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 * 
 * \param[in]       flowId is the flow instance.
 * 
 * \param[in]       clear should be set to TRUE to reset the flow's "used"
 *                  flag after it has been read.
 * 
 * \param[out]      used points to caller-allocated storage where the flow's
 *                  "used" flag will be stored. If clear is set to TRUE,
 *                  used will reflect the value of the "used" flag prior to
 *                  it being cleared.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if flowId is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmGetFlowUsed(fm_int   sw, 
                        fm_int   tableIndex, 
                        fm_int   flowId,
                        fm_bool  clear,
                        fm_bool *used)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, flowId = %d, clear = %d, used = %p\n",
                     sw,
                     tableIndex,
                     flowId,
                     clear,
                     (void *) used);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowUsed,
                       sw,
                       tableIndex,
                       flowId,
                       clear,
                       used);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowUsed */






/*****************************************************************************/
/** fmSetFlowAttribute
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a flow attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the
 *                  attribute belongs.
 * 
 * \param[in]       attr is the flow attribute to set (see 'Flow Attributes').
 * 
 * \param[in]       value points to the attribute value to set.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported or table
 *                  exists and attribute should be set before creation of table.
 * \return          FM_ERR_INVALID_ARGUMENT if argument is invalid.
 *
 *****************************************************************************/
fm_status fmSetFlowAttribute(fm_int sw, 
                             fm_int tableIndex, 
                             fm_int attr,
                             void * value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, attr = %d, value = %p\n",
                     sw,
                     tableIndex,
                     attr,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetFlowAttribute,
                       sw,
                       tableIndex,
                       attr,
                       value);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmSetFlowAttribute */




/*****************************************************************************/
/** fmGetFlowAttribute
 * \ingroup flow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a flow attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the
 *                  attribute belongs.
 * 
 * \param[in]       attr is the flow attribute to get (see 'Flow Attributes').
 * 
 * \param[in]       value points to caller-allocated storage where this
 *                  function should place the retrieved attribute value.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmGetFlowAttribute(fm_int sw, 
                             fm_int tableIndex, 
                             fm_int attr,
                             void * value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, tableIndex = %d, attr = %d, value = %p\n",
                     sw,
                     tableIndex,
                     attr,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetFlowAttribute,
                       sw,
                       tableIndex,
                       attr,
                       value);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmGetFlowAttribute */




/*****************************************************************************/
/** fmCreateFlowBalanceGrp
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Creates a variable sized ECMP balance group for use with
 *                  the ''FM_FLOW_ACTION_BALANCE'' action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      groupId receives the identifier of the created group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 *
 *****************************************************************************/
fm_status fmCreateFlowBalanceGrp(fm_int  sw,
                                 fm_int *groupId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d\n",
                     sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateFlowBalanceGrp,
                       sw,
                       groupId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmCreateFlowBalanceGrp */




/*****************************************************************************/
/** fmDeleteFlowBalanceGrp
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Deletes an ECMP balance group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the identifier of the group to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the ECMP group is in use.
 *
 *****************************************************************************/
fm_status fmDeleteFlowBalanceGrp(fm_int sw,
                                 fm_int groupId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, groupId = %d\n",
                     sw,
                     groupId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlowBalanceGrp,
                       sw,
                       groupId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlowBalanceGrp */




/*****************************************************************************/
/** fmAddFlowBalanceGrpEntry
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Adds an entry to an ECMP balance group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the identifier of the group to which the
 *                  entry will be added.
 *
 * \param[in]       tableIndex is the flow TE table index part of the entry.
 *                  The specified flow TE table should be in direct mode,
 *                  and must not use the USER field.
 *
 * \param[in]       flowId is the flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IS_FULL if the group has reached its
 *                  maximum capacity.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_NO_MEM if the function is unable to allocate
 *                  needed memory.
 * \return          FM_ERR_ALREADY_EXISTS if the entry being added already
 *                  exists in the ECMP group.
 *
 *****************************************************************************/
fm_status fmAddFlowBalanceGrpEntry(fm_int sw,
                                   fm_int groupId,
                                   fm_int tableIndex,
                                   fm_int flowId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, groupId = %d, tableIndex = %d, flowId = %d\n",
                     sw,
                     groupId,
                     tableIndex,
                     flowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AddFlowBalanceGrpEntry,
                       sw,
                       groupId,
                       tableIndex,
                       flowId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmAddFlowBalanceGrpEntry */




/*****************************************************************************/
/** fmDeleteFlowBalanceGrpEntry
 * \ingroup flow
 *
 * \chips           FM10000
 *
 * \desc            Deletes an entry from an ECMP balance group.
 *
 * \note            If an entry cannot be found in the ECMP group's entries 
 *                  table, that entry will simply be ignored.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the identifier of the group from which the
 *                  entry will be deleted.
 *
 * \param[in]       tableIndex is the flow TE table index part of the entry.
 *
 * \param[in]       flowId is the flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used and the
 *                  last entry in the group is being deleted.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is a fixed-size
 *                  group.
 *
 *****************************************************************************/
fm_status fmDeleteFlowBalanceGrpEntry(fm_int sw,
                                      fm_int groupId,
                                      fm_int tableIndex,
                                      fm_int flowId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW, 
                     "sw = %d, groupId = %d, tableIndex = %d, flowId = %d\n",
                     sw,
                     groupId,
                     tableIndex,
                     flowId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_FLOW_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteFlowBalanceGrpEntry,
                       sw,
                       groupId,
                       tableIndex,
                       flowId);

    DROP_FLOW_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_FLOW, err);

}   /* end fmDeleteFlowBalanceGrpEntry */




/*****************************************************************************/
/** fmConvertFlowToACLParam
 * \ingroup intFlow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert fm_flowParam to fm_aclParamExt.
 * 
 * \param[in]       flowParam refers to the flow param that must be converted.
 * 
 * \param[out]      aclParam refers to the ACL param that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertFlowToACLParam(fm_flowParam   *flowParam,
                                  fm_aclParamExt *aclParam)
{
    fm_status err = FM_OK;

    if (flowParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (aclParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(aclParam, sizeof(fm_aclParamExt), 0, sizeof(fm_aclParamExt));

    /* Translate actions params */
    aclParam->vlan = flowParam->vlan;
    aclParam->logicalPort = flowParam->logicalPort;
    aclParam->condition = flowParam->condition;
    aclParam->dmac = flowParam->dmac;
    aclParam->smac = flowParam->smac;
    aclParam->tunnelRule = flowParam->flowId;
    aclParam->vlanPriority = flowParam->vlanPriority;
    aclParam->switchPriority = flowParam->switchPriority;
    aclParam->dscp = flowParam->dscp;
    aclParam->lbgNumber = flowParam->lbgNumber;
    aclParam->mirrorGrp = flowParam->mirrorGrp;
    
ABORT:
    return err;

}   /* end fmConvertFlowToACLParam */




/*****************************************************************************/
/** fmConvertACLToFlowParam
 * \ingroup intFlow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert fm_flowParam to fm_aclParamExt.
 * 
 * \param[in]       flowParam refers to the flow param that must be converted.
 * 
 * \param[out]      aclParam refers to the ACL param that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertACLToFlowParam(fm_aclParamExt *aclParam, 
                                  fm_flowParam   *flowParam)
{
    fm_status err = FM_OK;

    if (flowParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (aclParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(flowParam, sizeof(fm_flowParam), 0, sizeof(fm_flowParam));

    /* Translate actions params */
    flowParam->vlan = aclParam->vlan;
    flowParam->logicalPort = aclParam->logicalPort;
    flowParam->condition = aclParam->condition;
    flowParam->dmac = aclParam->dmac;
    flowParam->smac = aclParam->smac;
    flowParam->flowId = aclParam->tunnelRule;
    flowParam->vlanPriority = aclParam->vlanPriority;
    flowParam->switchPriority = aclParam->switchPriority;
    flowParam->dscp = aclParam->dscp;
    flowParam->lbgNumber = aclParam->lbgNumber;
    flowParam->mirrorGrp = aclParam->mirrorGrp;
    
ABORT:
    return err;

}   /* end fmConvertACLToFlowParam */




/*****************************************************************************/
/** fmConvertFlowToTEParams
 * \ingroup intFlow
 *
 * \chips           FM10000
 *
 * \desc            Convert fm_flowParam to fm_tunnelActionParam and 
 *                  fm_tunnelEncapFlowParam.
 * 
 * \param[in]       flowParam refers to the flow param that must be converted.
 * 
 * \param[out]      tunnelParam refers to the TE param that stores result.
 * 
 * \param[out]      encapParam refers to the TE encap param that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertFlowToTEParams(fm_flowParam            *flowParam,
                                  fm_tunnelActionParam    *tunnelParam,
                                  fm_tunnelEncapFlowParam *encapParam)
{
    fm_status err = FM_OK;

    if (flowParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (tunnelParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (encapParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(tunnelParam, 
                sizeof(fm_tunnelActionParam), 
                0, 
                sizeof(fm_tunnelActionParam));
    FM_MEMSET_S(encapParam, 
                sizeof(fm_tunnelEncapFlowParam), 
                0, 
                sizeof(fm_tunnelEncapFlowParam));

    /* Translate actions params */
    tunnelParam->dip = flowParam->dip;
    tunnelParam->sip = flowParam->sip;
    tunnelParam->l4Dst = flowParam->l4Dst;
    tunnelParam->l4Src = flowParam->l4Src;
    tunnelParam->ttl = flowParam->ttl;
    tunnelParam->dmac = flowParam->dmac;
    tunnelParam->smac = flowParam->smac;
    tunnelParam->vlan = flowParam->vlan;
    tunnelParam->vni = flowParam->outerVni;
    
    /* Translate encap params */
    encapParam->type = flowParam->tunnelType;
    encapParam->dip = flowParam->outerDip;
    encapParam->sip = flowParam->outerSip;
    encapParam->ttl = flowParam->outerTtl;
    encapParam->l4Dst = flowParam->outerL4Dst;
    encapParam->l4Src = flowParam->outerL4Src;
    encapParam->ngeMask = flowParam->outerNgeMask;
    FM_MEMCPY_S(encapParam->ngeData,
                sizeof(encapParam->ngeData),
                flowParam->outerNgeData,
                sizeof(flowParam->outerNgeData));

    if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
    {
        encapParam->gpeVni    = flowParam->outerVni;
        encapParam->nshLength = flowParam->outerNshLength;
        encapParam->nshCritical = flowParam->outerNshCritical;
        encapParam->nshMdType = flowParam->outerNshMdType;
        encapParam->nshSvcPathId = flowParam->outerNshSvcPathId;
        encapParam->nshSvcIndex = flowParam->outerNshSvcIndex;
        encapParam->nshDataMask = flowParam->outerNshDataMask;

        FM_MEMCPY_S(encapParam->nshData,
                    sizeof(encapParam->nshData),
                    flowParam->outerNshData,
                    sizeof(flowParam->outerNshData));
    }

ABORT:
    return err;

}   /* end fmConvertFlowToTEParams */




/*****************************************************************************/
/** fmConvertTEParamsToFlow
 * \ingroup intFlow
 *
 * \chips           FM10000
 *
 * \desc            Convert fm_flowParam to fm_tunnelActionParam and 
 *                  fm_tunnelEncapFlowParam.
 * 
 * \param[in]       tunnelParam refers to the TE param that must be converted.
 * 
 * \param[in]       encapParam refers to the TE encap param that must be converted.
 * 
 * \param[out]      flowParam refers to the flow param that store the results.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertTEParamsToFlow(fm_tunnelActionParam    *tunnelParam,
                                  fm_tunnelEncapFlowParam *encapParam,
                                  fm_flowParam            *flowParam)
{
    fm_status err = FM_OK;

    if (flowParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (tunnelParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (encapParam == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(flowParam, 
                sizeof(fm_flowParam), 
                0, 
                sizeof(fm_flowParam));

    /* Translate actions params */
    flowParam->dip      = tunnelParam->dip;
    flowParam->sip      = tunnelParam->sip;
    flowParam->l4Dst    = tunnelParam->l4Dst;
    flowParam->l4Src    = tunnelParam->l4Src;
    flowParam->ttl      = tunnelParam->ttl;
    flowParam->dmac     = tunnelParam->dmac;
    flowParam->smac     = tunnelParam->smac;
    flowParam->vlan     = tunnelParam->vlan;
    flowParam->outerVni = tunnelParam->vni;
    
    /* Translate encap params */
    flowParam->tunnelType = encapParam->type;
    flowParam->outerDip = encapParam->dip;
    flowParam->outerSip = encapParam->sip;
    flowParam->outerTtl = encapParam->ttl;
    flowParam->outerL4Dst = encapParam->l4Dst;
    flowParam->outerL4Src = encapParam->l4Src;
    flowParam->outerNgeMask = encapParam->ngeMask;
    FM_MEMCPY_S(flowParam->outerNgeData,
                sizeof(flowParam->outerNgeData),
                encapParam->ngeData,
                sizeof(encapParam->ngeData));

    if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
    {
        flowParam->outerVni          = encapParam->gpeVni;
        flowParam->outerNshLength    = encapParam->nshLength;
        flowParam->outerNshCritical  = encapParam->nshCritical;
        flowParam->outerNshMdType    = encapParam->nshMdType;
        flowParam->outerNshSvcPathId = encapParam->nshSvcPathId;
        flowParam->outerNshSvcIndex  = encapParam->nshSvcIndex;
        flowParam->outerNshDataMask  = encapParam->nshDataMask;

        FM_MEMCPY_S(flowParam->outerNshData,
                    sizeof(flowParam->outerNshData),
                    encapParam->nshData,
                    sizeof(encapParam->nshData));
    }

ABORT:
    return err;

}   /* end fmConvertTEParamsToFlow */




/*****************************************************************************/
/** fmConvertFlowToACLValue
 * \ingroup intFlow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert fm_flowValue to fm_aclValue.
 * 
 * \param[in]       flowValue refers to the flow value that must be converted.
 * 
 * \param[out]      aclValue refers to the ACL value that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertFlowToACLValue(fm_flowValue   *flowValue,
                                  fm_aclValue    *aclValue)
{
    fm_status err = FM_OK;

    if (flowValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (aclValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(aclValue, sizeof(fm_aclValue), 0, sizeof(fm_aclValue));

    /* Translate conditions values */
    aclValue->src = flowValue->src;
    aclValue->srcMask = flowValue->srcMask;
    aclValue->dst = flowValue->dst;
    aclValue->dstMask = flowValue->dstMask;
    aclValue->ethType = flowValue->ethType;
    aclValue->ethTypeMask = flowValue->ethTypeMask;
    aclValue->vlanId = flowValue->vlanId;
    aclValue->vlanIdMask = flowValue->vlanIdMask;
    aclValue->vlanPri = flowValue->vlanPri;
    aclValue->vlanPriMask = flowValue->vlanPriMask;
    aclValue->srcIp = flowValue->srcIp;
    aclValue->srcIpMask = flowValue->srcIpMask;
    aclValue->dstIp = flowValue->dstIp;
    aclValue->dstIpMask = flowValue->dstIpMask;
    aclValue->protocol = flowValue->protocol;
    aclValue->protocolMask = flowValue->protocolMask;
    aclValue->L4SrcStart = flowValue->L4SrcStart;
    aclValue->L4SrcEnd = flowValue->L4SrcEnd;
    aclValue->L4SrcMask = flowValue->L4SrcMask;
    aclValue->L4DstStart = flowValue->L4DstStart;
    aclValue->L4DstEnd = flowValue->L4DstEnd;
    aclValue->L4DstMask = flowValue->L4DstMask;
    aclValue->ingressPortMask = flowValue->ingressPortMask;
    aclValue->portSet = flowValue->portSet;
    aclValue->logicalPort = flowValue->logicalPort;
    aclValue->tos = flowValue->tos;
    aclValue->tosMask = flowValue->tosMask;
    aclValue->frameType = flowValue->frameType;
    aclValue->table1Condition = flowValue->table1Condition;
    aclValue->table1ConditionMask = flowValue->table1ConditionMask;
    aclValue->tcpFlags = flowValue->tcpFlags;
    aclValue->tcpFlagsMask = flowValue->tcpFlagsMask;
    aclValue->switchPri = flowValue->switchPri;
    aclValue->switchPriMask = flowValue->switchPriMask;
    aclValue->vlanTag = flowValue->vlanTag;
    aclValue->vlanId2 = flowValue->vlanId2;
    aclValue->vlanId2Mask = flowValue->vlanId2Mask;
    aclValue->vlanPri2 = flowValue->vlanPri2;
    aclValue->vlanPri2Mask = flowValue->vlanPri2Mask;
    aclValue->fragType = flowValue->fragType;

    FM_MEMCPY_S(aclValue->L4DeepInspectionExt,
                sizeof(aclValue->L4DeepInspectionExt),
                flowValue->L4DeepInspection,
                sizeof(flowValue->L4DeepInspection));
    FM_MEMCPY_S(aclValue->L4DeepInspectionExtMask,
                sizeof(aclValue->L4DeepInspectionExtMask),
                flowValue->L4DeepInspectionMask,
                sizeof(flowValue->L4DeepInspectionMask));

    FM_MEMCPY_S(aclValue->nonIPPayload,
                sizeof(aclValue->nonIPPayload),
                flowValue->L2DeepInspection,
                sizeof(flowValue->L2DeepInspection));
    FM_MEMCPY_S(aclValue->nonIPPayloadMask,
                sizeof(aclValue->nonIPPayloadMask),
                flowValue->L2DeepInspectionMask,
                sizeof(flowValue->L2DeepInspectionMask));

ABORT:
    return err;

}   /* end fmConvertFlowToACLValue */




/*****************************************************************************/
/** fmConvertACLToFlowValue
 * \ingroup intFlow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert fm_flowValue to fm_aclValue.
 * 
 * \param[in]       aclValue refers to the ACL value that must be converted.
 * 
 * \param[out]      flowValue refers to the flow value that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertACLToFlowValue(fm_aclValue    *aclValue,
                                  fm_flowValue   *flowValue)
{
    fm_status err = FM_OK;

    if (flowValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (aclValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(flowValue, sizeof(fm_flowValue), 0, sizeof(fm_flowValue));

    /* Translate conditions values */
    flowValue->src = aclValue->src;
    flowValue->srcMask = aclValue->srcMask;
    flowValue->dst = aclValue->dst;
    flowValue->dstMask = aclValue->dstMask;
    flowValue->ethType = aclValue->ethType;
    flowValue->ethTypeMask = aclValue->ethTypeMask;
    flowValue->vlanId = aclValue->vlanId;
    flowValue->vlanIdMask = aclValue->vlanIdMask;
    flowValue->vlanPri = aclValue->vlanPri;
    flowValue->vlanPriMask = aclValue->vlanPriMask;
    flowValue->srcIp = aclValue->srcIp;
    flowValue->srcIpMask = aclValue->srcIpMask;
    flowValue->dstIp = aclValue->dstIp;
    flowValue->dstIpMask = aclValue->dstIpMask;
    flowValue->protocol = aclValue->protocol;
    flowValue->protocolMask = aclValue->protocolMask;
    flowValue->L4SrcStart = aclValue->L4SrcStart;
    flowValue->L4SrcEnd = aclValue->L4SrcEnd;
    flowValue->L4SrcMask = aclValue->L4SrcMask;
    flowValue->L4DstStart = aclValue->L4DstStart;
    flowValue->L4DstEnd = aclValue->L4DstEnd;
    flowValue->L4DstMask = aclValue->L4DstMask;
    flowValue->ingressPortMask = aclValue->ingressPortMask;
    flowValue->portSet = aclValue->portSet;
    flowValue->logicalPort = aclValue->logicalPort;
    flowValue->tos = aclValue->tos;
    flowValue->tosMask = aclValue->tosMask;
    flowValue->frameType = aclValue->frameType;
    flowValue->table1Condition = aclValue->table1Condition;
    flowValue->table1ConditionMask = aclValue->table1ConditionMask;
    flowValue->tcpFlags = aclValue->tcpFlags;
    flowValue->tcpFlagsMask = aclValue->tcpFlagsMask;

    flowValue->switchPri = aclValue->switchPri;
    flowValue->switchPriMask = aclValue->switchPriMask;
    flowValue->vlanTag = aclValue->vlanTag;
    flowValue->vlanId2 = aclValue->vlanId2;
    flowValue->vlanId2Mask = aclValue->vlanId2Mask;
    flowValue->vlanPri2 = aclValue->vlanPri2;
    flowValue->vlanPri2Mask = aclValue->vlanPri2Mask;
    flowValue->fragType = aclValue->fragType;

    FM_MEMCPY_S(flowValue->L4DeepInspection,
                sizeof(flowValue->L4DeepInspection),
                aclValue->L4DeepInspectionExt,
                sizeof(aclValue->L4DeepInspectionExt));
    FM_MEMCPY_S(flowValue->L4DeepInspectionMask,
                sizeof(flowValue->L4DeepInspectionMask),
                aclValue->L4DeepInspectionExtMask,
                sizeof(aclValue->L4DeepInspectionExtMask));

    FM_MEMCPY_S(flowValue->L2DeepInspection,
                sizeof(flowValue->L2DeepInspection),
                aclValue->nonIPPayload,
                sizeof(aclValue->nonIPPayload));
    FM_MEMCPY_S(flowValue->L2DeepInspectionMask,
                sizeof(flowValue->L2DeepInspectionMask),
                aclValue->nonIPPayloadMask,
                sizeof(aclValue->nonIPPayloadMask));

ABORT:
    return err;

}   /* end fmConvertACLToFlowValue */







/*****************************************************************************/
/** fmConvertFlowToTEValue
 * \ingroup intFlow
 *
 * \chips           FM10000
 *
 * \desc            Convert fm_flowValue to fm_tunnelConditionParam.
 * 
 * \param[in]       flowValue refers to the flow value that must be converted.
 * 
 * \param[out]      teValue refers to the TE value that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertFlowToTEValue(fm_flowValue            *flowValue,
                                 fm_tunnelConditionParam *teValue)
{
    fm_status err = FM_OK;

    if (flowValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (teValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(teValue, 
                sizeof(fm_tunnelConditionParam), 
                0, 
                sizeof(fm_tunnelConditionParam));

    /* Translate conditions values */
    teValue->smac = flowValue->src;
    teValue->dmac = flowValue->dst;
    teValue->vlan = flowValue->vlanId;
    teValue->sip = flowValue->srcIp;
    teValue->dip = flowValue->dstIp;
    teValue->protocol = flowValue->protocol;
    teValue->l4Src = flowValue->L4SrcStart;
    teValue->l4Dst = flowValue->L4DstStart;
    teValue->vni = flowValue->vni;
    teValue->vsiTep = flowValue->vsiTep;

ABORT:
    return err;

}   /* end fmConvertFlowToTEValue */




/*****************************************************************************/
/** fmConvertFlowToTEValue
 * \ingroup intFlow
 *
 * \chips           FM10000
 *
 * \desc            Convert fm_flowValue to fm_tunnelConditionParam.
 * 
 * \param[in]       flowValue refers to the flow value that must be converted.
 * 
 * \param[out]      teValue refers to the TE value that stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertTEToFlowValue(fm_tunnelConditionParam *teValue,
                                 fm_flowValue            *flowValue)
{
    fm_status err = FM_OK;

    if (flowValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (teValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(flowValue, 
                sizeof(fm_flowValue), 
                0, 
                sizeof(fm_flowValue));

    /* Translate conditions values */
    flowValue->src = teValue->smac;
    flowValue->srcMask = 0xffffffffffffLL;
    flowValue->dst = teValue->dmac;
    flowValue->dstMask = 0xffffffffffffLL;
    flowValue->vlanId = teValue->vlan;
    flowValue->vlanIdMask = 0xfff;
    flowValue->srcIp = teValue->sip;
    flowValue->srcIpMask.isIPv6 = flowValue->srcIp.isIPv6;
    flowValue->srcIpMask.addr[0] = 0xffffffff;
    flowValue->srcIpMask.addr[1] = 0xffffffff;
    flowValue->srcIpMask.addr[2] = 0xffffffff;
    flowValue->srcIpMask.addr[3] = 0xffffffff;
    flowValue->dstIp = teValue->dip;
    flowValue->dstIpMask.isIPv6 = flowValue->dstIp.isIPv6;
    flowValue->dstIpMask.addr[0] = 0xffffffff;
    flowValue->dstIpMask.addr[1] = 0xffffffff;
    flowValue->dstIpMask.addr[2] = 0xffffffff;
    flowValue->dstIpMask.addr[3] = 0xffffffff;
    flowValue->protocol = teValue->protocol;
    flowValue->protocolMask = 0xff;
    flowValue->L4SrcStart = teValue->l4Src;
    flowValue->L4SrcMask = 0xffff;
    flowValue->L4DstStart = teValue->l4Dst;
    flowValue->L4DstMask = 0xffff;
    flowValue->vni = teValue->vni;
    flowValue->vsiTep = teValue->vsiTep;

ABORT:
    return err;

}   /* end fmConvertFlowToTEValue */




/*****************************************************************************/
/** fmConvertACLToFlowCounters
 * \ingroup intFlow
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert fm_aclCounters to fm_flowCounters.
 * 
 * \param[in]       aclCounters refers to the ACL counters struct that must 
 *                  be converted.
 * 
 * \param[out]      flowCounters refers to the flow counters struct that 
 *                  stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertACLToFlowCounters(fm_aclCounters   *aclCounters,
                                     fm_flowCounters  *flowCounters)
{
    fm_status err = FM_OK;

    if (aclCounters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (flowCounters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(flowCounters, sizeof(fm_flowCounters), 0, sizeof(fm_flowCounters));

    flowCounters->cntPkts = aclCounters->cntPkts;
    flowCounters->cntOctets = aclCounters->cntOctets;
    
ABORT:
    return err;

}   /* end fmConvertACLToFlowCounters */




/*****************************************************************************/
/** fmConvertTEToFlowCounters
 * \ingroup intFlow
 *
 * \chips           FM10000
 *
 * \desc            Convert fm_tunnelCounters to fm_flowCounters.
 * 
 * \param[in]       tunnelCounters refers to the TE counters struct that must 
 *                  be converted.
 * 
 * \param[out]      flowCounters refers to the flow counters struct that 
 *                  stores result.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer.
 *
 *****************************************************************************/
fm_status fmConvertTEToFlowCounters(fm_tunnelCounters   *tunnelCounters,
                                    fm_flowCounters     *flowCounters)
{
    fm_status err = FM_OK;

    if (tunnelCounters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    if (flowCounters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    
    FM_MEMSET_S(flowCounters, sizeof(fm_flowCounters), 0, sizeof(fm_flowCounters));

    flowCounters->cntPkts = tunnelCounters->cntPkts;
    flowCounters->cntOctets = tunnelCounters->cntOctets;
    
ABORT:
    return err;

}   /* end fmConvertTEToFlowCounters */
