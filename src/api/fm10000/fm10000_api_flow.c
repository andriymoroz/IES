/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_flow.c
 * Creation Date:   December 6, 2013
 * Description:     FM10000 OpenFlow API interface.
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

/* Simple helper macro that counts and returns the number of bits set in
 * value. */
#define FM_COUNT_SET_BITS(value, bits)                                         \
    {                                                                          \
        fm_int modifValue = value;                                             \
        for ((bits) = 0; modifValue; (bits)++)                                 \
        {                                                                      \
            modifValue &= modifValue - 1;                                      \
        }                                                                      \
    }

#define FM10000_DEFAULT_TEP_SIZE  512

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
/** AddDefaultRule
 * \ingroup intFlow
 *
 * \desc            Add a flow entry to the specified table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to add this flow.
 *
 * \param[out]      flowId points to a caller-allocated variable to store the
 *                  flow ID associated to the default flow.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED for an unsupported feature.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if acl is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_INVALID_ACL_IMAGE if a valid set of ACLs have not
 *                  been previously successfully compiled with ''fmCompileACL''.
 *
 *****************************************************************************/
static fm_status AddDefaultRule(fm_int sw, fm_int tableIndex, fm_int *flowId)
{
    fm_status       err = FM_OK;
    fm_flowParam    param;
    fm_flowValue    val;
    fm10000_switch *switchExt;
    fm_aclActionExt action;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->flowInfo.table[tableIndex].countSupported == FM_ENABLED)
    {
        action = FM_FLOW_ACTION_DEFAULT | FM_FLOW_ACTION_COUNT;
    }
    else
    {
        action = FM_FLOW_ACTION_DEFAULT;
    }

    /* Default rule: match on any condition -> redirect to CPU */
    param.logicalPort = switchExt->flowInfo.defaultLogicalPort;
    err = fm10000AddFlow(sw, tableIndex, 0, 0,
                         0,
                         &val,
                         action,
                         &param,
                         FM_FLOW_STATE_ENABLED,
                         flowId);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end AddDefaultRule */




/*****************************************************************************/
/** InitFlowApiCommon
 * \ingroup intFlow
 *
 * \desc            Initialize the flow API module.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitFlowApiCommon(fm_int sw)
{
    fm_status       err;
    fm_int          i;
    fm_switch      *switchPtr;
    fm10000_switch *switchExt;
    fm_flowValue   *condMasks;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    err = fmCreateBitArray(&switchExt->flowInfo.balanceGrpInUse,
                           switchPtr->maxArpEntries + 1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    for (i = 0 ; i < FM10000_FLOW_PORTSET_NUM ; i++)
    {
        switchExt->flowInfo.portSetMap[i] = -1;
    }

    /***************************************************
     * Special treatment for the Forward-Normal action.
     * We do a Set Flood dest to the flood glort to let
     * the frame go through the normal L2/L3 switch pipeline.
     **************************************************/
    switchExt->flowInfo.fwdNormalLogicalPort = FM_PORT_FLOOD;

    /* Drop Logical port */
    switchExt->flowInfo.dropLogicalPort = FM_PORT_DROP;

    for (i = 0 ; i < FM_FLOW_MAX_TABLE_TYPE ; i++)
    {
        condMasks = &switchExt->flowInfo.table[i].condMasks;
        FM_MEMSET_S(condMasks, sizeof(fm_flowValue), 0, sizeof(fm_flowValue));
        condMasks->srcMask = FM_LITERAL_U64(0xffffffffffff);
        condMasks->dstMask = FM_LITERAL_U64(0xffffffffffff);
        condMasks->vlanIdMask = 0xffff;
        condMasks->vlanPriMask = 0x0f;
        condMasks->srcIpMask.addr[3] = 0xffffffff;
        condMasks->srcIpMask.addr[2] = 0xffffffff;
        condMasks->srcIpMask.addr[1] = 0xffffffff;
        condMasks->srcIpMask.addr[0] = 0xffffffff;
        condMasks->srcIpMask.isIPv6 = FALSE;
        condMasks->dstIpMask.addr[3] = 0xffffffff;
        condMasks->dstIpMask.addr[2] = 0xffffffff;
        condMasks->dstIpMask.addr[1] = 0xffffffff;
        condMasks->dstIpMask.addr[0] = 0xffffffff;
        condMasks->dstIpMask.isIPv6 = FALSE;
        condMasks->tosMask = 0xff;
        condMasks->frameType = FM_ACL_FRAME_TYPE_IPV4;
        condMasks->L4SrcMask = 0xffff;
        condMasks->L4DstMask = 0xffff;
        condMasks->switchPriMask = 0xf;
        condMasks->vlanId2Mask = 0xffff;
        condMasks->vlanPri2Mask = 0x0f;
        condMasks->vlanTag = FM_ACL_VLAN_TAG_TYPE_NONE;
        condMasks->fragType = FM_ACL_FRAG_COMPLETE;
        condMasks->protocolMask = 0xff;
        condMasks->tcpFlagsMask = 0x3f;
        condMasks->ethTypeMask = 0xffff;
        condMasks->ttlMask = 0xff;
        FM_MEMSET_S(condMasks->L4DeepInspectionMask,
                    sizeof(condMasks->L4DeepInspectionMask),
                    0xff,
                    sizeof(condMasks->L4DeepInspectionMask));
        FM_MEMSET_S(condMasks->L2DeepInspectionMask,
                    sizeof(condMasks->L2DeepInspectionMask),
                    0xff,
                    sizeof(condMasks->L2DeepInspectionMask));
        switchExt->flowInfo.table[i].countSupported = FM_ENABLED;
        switchExt->flowInfo.table[i].group = -1;
        switchExt->flowInfo.table[i].encap = TRUE;
        switchExt->flowInfo.table[i].scenario = (FM_ACL_SCENARIO_ANY_FRAME_TYPE |
                                                 FM_ACL_SCENARIO_ANY_ROUTING_TYPE);
    }

    switchExt->flowInfo.initialized = TRUE;

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);

}   /* end InitFlowApiCommon */




/*****************************************************************************/
/** InitFlowApi
 * \ingroup intFlow
 *
 * \desc            Initialize the flow API module.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED for an unsupported feature.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 *
 *****************************************************************************/
static fm_status InitFlowApi(fm_int sw)
{
    fm_status            err = FM_OK;
    fm_int               i;
    fm_switch *          switchPtr;
    fm10000_switch *     switchExt;
    fm_int               logicalPort;
    fm_int               mcastGroupId;
    fm_multicastListener listener;
    fm_bool              l2SwitchOnly = TRUE;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /***************************************************
     * A packet can be sent to the CPU for the following
     * reasons or actions:
     * 1- DEFAULT (no hit in the table)
     * 2- TRAP to CPU
     * 3- FORWARD to CPU (redirect to CPU)
     *
     * The application needs to know what action
     * triggered the sending of the packet to the CPU.
     * To do so, we make use of unused multicast groups
     * in such a way that the destination glort is
     * different for each action.
     **************************************************/

    /* Create MCAST groups */
    for (i = 0; i < 3; i++)
    {
        err = fmCreateMcastGroup(sw, &mcastGroupId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmSetMcastGroupAttribute(sw,
                                       mcastGroupId,
                                       FM_MCASTGROUP_L2_SWITCHING_ONLY,
                                       &l2SwitchOnly);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmActivateMcastGroup(sw, mcastGroupId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        FM_CLEAR(listener);
        listener.vlan = 1;
        listener.port = switchPtr->cpuPort; /* CPU port */
        err = fmAddMcastGroupListener(sw, mcastGroupId, &listener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmGetMcastGroupPort(sw, mcastGroupId, &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (i == 0)
        {
            /* DEFAULT action */
            switchExt->flowInfo.defaultLogicalPort = logicalPort;
        }
        else if (i == 1)
        {
            /* TRAP action */
            switchExt->flowInfo.trapLogicalPort = logicalPort;
        }
        else
        {
            /* FORWARD-TO-CPU action */
            switchExt->flowInfo.fwdToCpuLogicalPort = logicalPort;
        }
    }

    err = InitFlowApiCommon(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end InitFlowApi */




/*****************************************************************************/
/** TranslateFlowToACLAction
 * \ingroup intFlow
 *
 * \desc            Translate the flow actions into ACL one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       action refers to the actual flow action that must be
 *                  translated.
 *
 * \param[out]      aclAction refers to the ACL action that stores result.
 *
 * \param[in,out]   param refers to the actual flow action arguments that
 *                  must be translated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid action set.
 *
 *****************************************************************************/
static fm_status TranslateFlowToACLAction(fm_int           sw,
                                          fm_flowAction   *action,
                                          fm_aclActionExt *aclAction,
                                          fm_aclParamExt  *param)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_uint32       glort;
    fm_LBGMode      lbgMode;
    fm_int          arpBaseIndex;
    fm_int          pathCount;
    fm_int          pathCountType;
    fm_bool         aclMirror;

    switchExt = GET_SWITCH_EXT(sw);
    *aclAction = 0;
    aclMirror = FALSE;

    /* Special case: a Forward to cpu is done using a
       special logical port */
    if ( (*action & FM_FLOW_ACTION_FORWARD) && (param->logicalPort == 0) )
    {
        param->logicalPort = switchExt->flowInfo.fwdToCpuLogicalPort;
    }

    /* ''FM_FLOW_ACTION_POP_VLAN'' action must always be paired with
     * ''FM_FLOW_ACTION_SET_VLAN'' to specify the new vlan that
     * would be used for filtering. */
    if ( (*action & FM_FLOW_ACTION_POP_VLAN) &&
            !(*action & FM_FLOW_ACTION_SET_VLAN) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /***************************************************
     *   Translate Flow action to an ACL action
     ***************************************************/

    if ( *action & FM_FLOW_ACTION_FORWARD )
    {
        *aclAction |= FM_ACL_ACTIONEXT_REDIRECT;
        err = fmGetLogicalPortGlort(sw, param->logicalPort, &glort);
        if (err)
        {
            return err;
        }
    }

    if ( *action & FM_FLOW_ACTION_FORWARD_NORMAL )
    {
        param->logicalPort = switchExt->flowInfo.fwdNormalLogicalPort;
        /* Translate the Forward-Normal action into a Set Flood Dest */
        *aclAction |= FM_ACL_ACTIONEXT_SET_FLOOD_DEST;
    }

    if ( *action & FM_FLOW_ACTION_DROP )
    {
        param->logicalPort = switchExt->flowInfo.dropLogicalPort;
        /* Translate the drop action into a Redirect */
        *aclAction |= FM_ACL_ACTIONEXT_REDIRECT;
    }

    if ( *action & FM_FLOW_ACTION_TRAP )
    {
        param->logicalPort = switchExt->flowInfo.trapLogicalPort;
        /* Translate the trap action into a Redirect */
        *aclAction |= FM_ACL_ACTIONEXT_REDIRECT;
    }

    if ( *action & FM_FLOW_ACTION_DEFAULT )
    {
        param->logicalPort = switchExt->flowInfo.defaultLogicalPort;
        /* Translate the default action into a Redirect */
        *aclAction |= FM_ACL_ACTIONEXT_REDIRECT;
    }

    if ( *action & FM_FLOW_ACTION_COUNT )
    {
        *aclAction |= FM_ACL_ACTIONEXT_COUNT;
    }

    if ( *action & FM_FLOW_ACTION_PUSH_VLAN )
    {
        *aclAction |= FM_ACL_ACTIONEXT_PUSH_VLAN;
    }

    if ( *action & FM_FLOW_ACTION_POP_VLAN )
    {
        *aclAction |= FM_ACL_ACTIONEXT_POP_VLAN;
    }

    if ( *action & FM_FLOW_ACTION_SET_VLAN )
    {
        *aclAction |= FM_ACL_ACTIONEXT_SET_VLAN;
    }

    if ( *action & FM_FLOW_ACTION_UPD_OR_ADD_VLAN )
    {
        *aclAction |= FM_ACL_ACTIONEXT_UPD_OR_ADD_VLAN;
    }

    if ( *action & FM_FLOW_ACTION_REDIRECT_TUNNEL )
    {
        *aclAction |= FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
    }

    if ( *action & FM_FLOW_ACTION_BALANCE )
    {
        *aclAction |= FM_ACL_ACTIONEXT_ROUTE;
        err = fm10000GetECMPGroupArpInfo(sw,
                                         param->groupId,
                                         NULL,
                                         &arpBaseIndex,
                                         &pathCount,
                                         &pathCountType);
        if (err)
        {
            return err;
        }
    }

    if ( *action & FM_FLOW_ACTION_ROUTE )
    {
        *aclAction |= FM_ACL_ACTIONEXT_ROUTE;
        err = fm10000GetECMPGroupArpInfo(sw,
                                         param->groupId,
                                         NULL,
                                         &arpBaseIndex,
                                         &pathCount,
                                         &pathCountType);
        if (err)
        {
            return err;
        }
    }

    if ( *action & FM_FLOW_ACTION_PERMIT )
    {
        *aclAction |= FM_ACL_ACTIONEXT_PERMIT;
    }

    if ( *action & FM_FLOW_ACTION_DENY )
    {
        *aclAction |= FM_ACL_ACTIONEXT_DENY;
    }

    if ( *action & FM_FLOW_ACTION_SET_VLAN_PRIORITY )
    {
        *aclAction |= FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY;
    }

    if ( *action & FM_FLOW_ACTION_SET_SWITCH_PRIORITY )
    {
        *aclAction |= FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY;
    }

    if ( *action & FM_FLOW_ACTION_SET_DSCP )
    {
        *aclAction |= FM_ACL_ACTIONEXT_SET_DSCP;
    }

    if ( *action & FM_FLOW_ACTION_LOAD_BALANCE )
    {
        *aclAction |= FM_ACL_ACTIONEXT_LOAD_BALANCE;
        err = fm10000GetLBGAttribute(sw,
                                     param->lbgNumber,
                                     FM_LBG_GROUP_MODE,
                                     &lbgMode);
        if (err)
        {
            return err;
        }
    }

    if ( *action & FM_FLOW_ACTION_SET_FLOOD_DEST )
    {
        *aclAction |= FM_ACL_ACTIONEXT_SET_FLOOD_DEST;
        err = fmGetLogicalPortGlort(sw, param->logicalPort, &glort);
        if (err)
        {
            return err;
        }
    }

    if ( *action & FM_FLOW_ACTION_MIRROR_GRP )
    {
        *aclAction |= FM_ACL_ACTIONEXT_MIRROR_GRP;
        err = fmGetMirrorAttribute(sw, param->mirrorGrp, FM_MIRROR_ACL, &aclMirror);
        if (err == FM_OK)
        {
             if (!aclMirror)
             {
                 err = FM_ERR_INVALID_ARGUMENT;
             }
        }
    }

    return err;

}   /* end TranslateFlowToACLAction */





/*****************************************************************************/
/** TranslateACLToFlowAction
 * \ingroup intFlow
 *
 * \desc            Translate the flow actions into ACL one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       aclAction refers to the actual ACL param that must be
 *                  translated.
 *
 * \param[out]      action refers to the flow action that stores result.
 *
 * \param[in,out]   param refers to the actual flow action arguments that
 *                  must be translated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid action set.
 *
 *****************************************************************************/
static fm_status TranslateACLToFlowAction(fm_int           sw,
                                          fm_aclActionExt *aclAction,
                                          fm_aclParamExt  *param,
                                          fm_flowAction   *action)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;

    switchExt = GET_SWITCH_EXT(sw);
    *action = 0;

    /***************************************************
     *   Translate Flow action to an ACL action
     ***************************************************/

    if ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT )
    {
        *action |= FM_FLOW_ACTION_FORWARD;
    }

    if ( ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT ) &&
         ( param->logicalPort == switchExt->flowInfo.fwdNormalLogicalPort ) )
    {
        /* Translate the Forward-Normal action into a Redirect */
        *action &= ~FM_FLOW_ACTION_FORWARD;
        *action |= FM_FLOW_ACTION_FORWARD_NORMAL;
    }

    if ( ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT ) &&
         ( param->logicalPort == switchExt->flowInfo.dropLogicalPort) )
    {
        /* Translate the drop action into a Redirect */
        *action &= ~FM_FLOW_ACTION_FORWARD;
        *action |= FM_FLOW_ACTION_DROP;
    }

    if ( ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT ) &&
         ( param->logicalPort == switchExt->flowInfo.trapLogicalPort ) )
    {
        /* Translate the trap action into a Redirect */
        *action &= ~FM_FLOW_ACTION_FORWARD;
        *action |= FM_FLOW_ACTION_TRAP;
    }

    if ( ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT ) &&
         ( param->logicalPort == switchExt->flowInfo.defaultLogicalPort ) )
    {
        /* Translate the default action into a Redirect */
        *action &= ~FM_FLOW_ACTION_FORWARD;
        *action |= FM_FLOW_ACTION_DEFAULT;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_COUNT )
    {
        *action |= FM_FLOW_ACTION_COUNT;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_PUSH_VLAN )
    {
        *action |= FM_FLOW_ACTION_PUSH_VLAN;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_POP_VLAN )
    {
        *action |= FM_FLOW_ACTION_POP_VLAN;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_SET_VLAN )
    {
        *action |= FM_FLOW_ACTION_SET_VLAN;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_UPD_OR_ADD_VLAN )
    {
        *action |= FM_FLOW_ACTION_UPD_OR_ADD_VLAN;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL )
    {
        *action |= FM_FLOW_ACTION_REDIRECT_TUNNEL;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_ROUTE )
    {
        *action |= FM_FLOW_ACTION_BALANCE;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_ROUTE )
    {
        *action |= FM_FLOW_ACTION_ROUTE;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_PERMIT )
    {
        *action |= FM_FLOW_ACTION_PERMIT;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_DENY )
    {
        *action |= FM_FLOW_ACTION_DENY;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY )
    {
        *action |= FM_FLOW_ACTION_SET_VLAN_PRIORITY;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY )
    {
        *action |= FM_FLOW_ACTION_SET_SWITCH_PRIORITY;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_SET_DSCP )
    {
        *action |= FM_FLOW_ACTION_SET_DSCP;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_LOAD_BALANCE )
    {
        *action |= FM_FLOW_ACTION_LOAD_BALANCE;
    }

    if ( *aclAction & FM_ACL_ACTIONEXT_SET_FLOOD_DEST )
    {
        *action |= FM_FLOW_ACTION_SET_FLOOD_DEST;
    }
    if ( *aclAction & FM_ACL_ACTIONEXT_MIRROR_GRP )
    {
        *action |= FM_FLOW_ACTION_MIRROR_GRP;
    }

    return err;

}   /* end TranslateACLToFlowAction */




/*****************************************************************************/
/** ConfigureDeepInspectionProfile
 * \ingroup intFlow
 *
 * \desc            Configure Deep Inspection Profile for
 *                  ''FM_FLOW_MATCH_TCP_FLAGS'' usage.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureDeepInspectionProfile(fm_int sw)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_parserDiCfg  parserDiCfg;

    switchExt = GET_SWITCH_EXT(sw);

    /* Initialize Deep Inspection Profile - Done just once */
    if (switchExt->flowInfo.deepInspectionProfileInitialized == FALSE)
    {
        FM_CLEAR(parserDiCfg);
        parserDiCfg.index                             = FM10000_FLOW_DI_PROFILE;
        parserDiCfg.parserDiCfgFields.protocol        = 0x06; /* TCP */
        parserDiCfg.parserDiCfgFields.l4Port          = 0;
        parserDiCfg.parserDiCfgFields.l4Compare       = FALSE;
        parserDiCfg.parserDiCfgFields.captureTcpFlags = TRUE;
        parserDiCfg.parserDiCfgFields.enable          = TRUE;
        parserDiCfg.parserDiCfgFields.wordOffset      = 0x76543210;

        err = fm10000SetSwitchAttribute(sw,
                                        FM_SWITCH_PARSER_DI_CFG,
                                        &parserDiCfg);
        if (err == FM_OK)
        {
            switchExt->flowInfo.deepInspectionProfileInitialized = TRUE;
        }
    }

    return err;

}   /* end ConfigureDeepInspectionProfile */




/*****************************************************************************/
/** TranslateFlowToACLCondition
 * \ingroup intFlow
 *
 * \desc            Translate the flow condition into ACL one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       condition refers to the actual flow condition that must be
 *                  translated.
 *
 * \param[out]      aclCondition refers to the ACL condition that stores result.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateFlowToACLCondition(fm_int            sw,
                                             fm_flowCondition *condition,
                                             fm_aclCondition  *aclCondition)
{
    fm_status err = FM_OK;

    *aclCondition = 0;

    /***************************************************
     *   Translate Flow condition to an ACL condition
     ***************************************************/

    if ( *condition & FM_FLOW_MATCH_SRC_MAC )
    {
        *aclCondition |= FM_ACL_MATCH_SRC_MAC;
    }

    if ( *condition & FM_FLOW_MATCH_DST_MAC )
    {
        *aclCondition |= FM_ACL_MATCH_DST_MAC;
    }

    if ( *condition & FM_FLOW_MATCH_ETHERTYPE )
    {
        *aclCondition |= FM_ACL_MATCH_ETHERTYPE;
    }

    if ( *condition & FM_FLOW_MATCH_VLAN )
    {
        *aclCondition |= FM_ACL_MATCH_VLAN;
    }

    if ( *condition & FM_FLOW_MATCH_VLAN_PRIORITY )
    {
        *aclCondition |= FM_ACL_MATCH_PRIORITY;
    }

    if ( *condition & FM_FLOW_MATCH_SRC_IP )
    {
        *aclCondition |= FM_ACL_MATCH_SRC_IP;
    }

    if ( *condition & FM_FLOW_MATCH_DST_IP )
    {
        *aclCondition |= FM_ACL_MATCH_DST_IP;
    }

    if ( *condition & FM_FLOW_MATCH_PROTOCOL )
    {
        *aclCondition |= FM_ACL_MATCH_PROTOCOL;
    }

    if ( *condition & FM_FLOW_MATCH_L4_SRC_PORT )
    {
        *aclCondition |= FM_ACL_MATCH_L4_SRC_PORT;
    }

    if ( *condition & FM_FLOW_MATCH_L4_DST_PORT )
    {
        *aclCondition |= FM_ACL_MATCH_L4_DST_PORT;
    }

    if ( *condition & FM_FLOW_MATCH_INGRESS_PORT_SET )
    {
        *aclCondition |= FM_ACL_MATCH_INGRESS_PORT_SET;
    }

    if ( *condition & FM_FLOW_MATCH_TOS )
    {
        *aclCondition |= FM_ACL_MATCH_TOS;
    }

    if ( *condition & FM_FLOW_MATCH_FRAME_TYPE )
    {
        *aclCondition |= FM_ACL_MATCH_FRAME_TYPE;
    }

    if ( *condition & FM_FLOW_MATCH_SRC_PORT )
    {
        *aclCondition |= FM_ACL_MATCH_SRC_PORT;
    }

    if ( *condition & FM_FLOW_MATCH_TCP_FLAGS )
    {
        err = ConfigureDeepInspectionProfile(sw);
        *aclCondition |= FM_ACL_MATCH_TCP_FLAGS;
    }

    if ( *condition & FM_FLOW_MATCH_L4_DEEP_INSPECTION )
    {
        *aclCondition |= FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT;
    }

    if ( *condition & FM_FLOW_MATCH_L2_DEEP_INSPECTION )
    {
        *aclCondition |= FM_ACL_MATCH_NON_IP_PAYLOAD;
    }

    if ( *condition & FM_FLOW_MATCH_SWITCH_PRIORITY )
    {
        *aclCondition |= FM_ACL_MATCH_SWITCH_PRIORITY;
    }

    if ( *condition & FM_FLOW_MATCH_VLAN_TAG_TYPE )
    {
        *aclCondition |= FM_ACL_MATCH_VLAN_TAG_TYPE;
    }

    if ( *condition & FM_FLOW_MATCH_VLAN2 )
    {
        *aclCondition |= FM_ACL_MATCH_VLAN2;
    }

    if ( *condition & FM_FLOW_MATCH_PRIORITY2 )
    {
        *aclCondition |= FM_ACL_MATCH_PRIORITY2;
    }

    if ( *condition & FM_FLOW_MATCH_FRAG )
    {
        *aclCondition |= FM_ACL_MATCH_FRAG;
    }

    if ( *condition & FM_FLOW_MATCH_LOGICAL_PORT )
    {
        *aclCondition |= FM_ACL_MATCH_SRC_GLORT;
    }

    if ( *condition & FM_FLOW_MATCH_TTL )
    {
        *aclCondition |= FM_ACL_MATCH_TTL;
    }

    return err;

}   /* end TranslateFlowToACLCondition */




/*****************************************************************************/
/** TranslateACLToFlowCondition
 * \ingroup intFlow
 *
 * \desc            Translate the flow condition into ACL one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       aclCondition refers to the actual ACL condition that must be
 *                  translated.
 *
 * \param[out]      condition refers to the flow condition that stores result.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateACLToFlowCondition(fm_int            sw,
                                             fm_aclCondition  *aclCondition,
                                             fm_flowCondition *condition)
{
    fm_status err = FM_OK;

    *condition = 0;

    /***************************************************
     *   Translate Flow condition to an ACL condition
     ***************************************************/

    if ( *aclCondition & FM_ACL_MATCH_SRC_MAC )
    {
        *condition |= FM_FLOW_MATCH_SRC_MAC;
    }

    if ( *aclCondition & FM_ACL_MATCH_DST_MAC )
    {
        *condition |= FM_FLOW_MATCH_DST_MAC;
    }

    if ( *aclCondition & FM_ACL_MATCH_ETHERTYPE )
    {
        *condition |= FM_FLOW_MATCH_ETHERTYPE;
    }

    if ( *aclCondition & FM_ACL_MATCH_VLAN )
    {
        *condition |= FM_FLOW_MATCH_VLAN;
    }

    if ( *aclCondition & FM_ACL_MATCH_PRIORITY )
    {
        *condition |= FM_FLOW_MATCH_VLAN_PRIORITY;
    }

    if ( *aclCondition & FM_ACL_MATCH_SRC_IP )
    {
        *condition |= FM_FLOW_MATCH_SRC_IP;
    }

    if ( *aclCondition & FM_ACL_MATCH_DST_IP )
    {
        *condition |= FM_FLOW_MATCH_DST_IP;
    }

    if ( *aclCondition & FM_ACL_MATCH_PROTOCOL )
    {
        *condition |= FM_FLOW_MATCH_PROTOCOL;
    }

    if ( *aclCondition & FM_ACL_MATCH_L4_SRC_PORT )
    {
        *condition |= FM_FLOW_MATCH_L4_SRC_PORT;
    }

    if ( *aclCondition & FM_ACL_MATCH_L4_DST_PORT )
    {
        *condition |= FM_FLOW_MATCH_L4_DST_PORT;
    }

    if ( *aclCondition & FM_ACL_MATCH_INGRESS_PORT_SET )
    {
        *condition |= FM_FLOW_MATCH_INGRESS_PORT_SET;
    }

    if ( *aclCondition & FM_ACL_MATCH_TOS )
    {
        *condition |= FM_FLOW_MATCH_TOS;
    }

    if ( *aclCondition & FM_ACL_MATCH_FRAME_TYPE )
    {
        *condition |= FM_FLOW_MATCH_FRAME_TYPE ;
    }

    if ( *aclCondition & FM_ACL_MATCH_SRC_PORT )
    {
        *condition |= FM_FLOW_MATCH_SRC_PORT;
    }

    if ( *aclCondition & FM_ACL_MATCH_TCP_FLAGS )
    {
        *condition |= FM_FLOW_MATCH_TCP_FLAGS;
    }

    if ( *aclCondition & FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT )
    {
        *condition |= FM_FLOW_MATCH_L4_DEEP_INSPECTION;
    }

    if ( *aclCondition & FM_ACL_MATCH_NON_IP_PAYLOAD )
    {
        *condition |= FM_FLOW_MATCH_L2_DEEP_INSPECTION;
    }

    if ( *aclCondition & FM_ACL_MATCH_SWITCH_PRIORITY )
    {
        *condition |= FM_FLOW_MATCH_SWITCH_PRIORITY;
    }

    if ( *aclCondition & FM_ACL_MATCH_VLAN_TAG_TYPE )
    {
        *condition |= FM_FLOW_MATCH_VLAN_TAG_TYPE;
    }

    if ( *aclCondition & FM_ACL_MATCH_VLAN2 )
    {
        *condition |= FM_FLOW_MATCH_VLAN2;
    }

    if ( *aclCondition & FM_ACL_MATCH_PRIORITY2 )
    {
        *condition |= FM_FLOW_MATCH_PRIORITY2;
    }

    if ( *aclCondition & FM_ACL_MATCH_FRAG )
    {
        *condition |= FM_FLOW_MATCH_FRAG;
    }

    if ( *aclCondition & FM_ACL_MATCH_SRC_GLORT )
    {
        *condition |= FM_FLOW_MATCH_LOGICAL_PORT;
    }

    if ( *aclCondition & FM_ACL_MATCH_TTL )
    {
        *condition |= FM_FLOW_MATCH_TTL;
    }

    return err;

}   /* end TranslateACLToFlowCondition */




/*****************************************************************************/
/** TranslateConditionMask
 * \ingroup intFlow
 *
 * \desc            Translate the flow condition mask into ACL one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   condition refers to the actual flow condition mask that must
 *                  be translated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid condition pointer.
 *
 *****************************************************************************/
static fm_status TranslateConditionMask(fm_int            sw,
                                        fm_aclCondition  *condition)
{
    fm_status err = FM_OK;

    if (condition == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Translate conditions */
    if (*condition & FM_ACL_MATCH_L4_SRC_PORT)
    {
        *condition &= ~FM_ACL_MATCH_L4_SRC_PORT;
        *condition |= FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK;
    }

    if (*condition & FM_ACL_MATCH_L4_DST_PORT)
    {
        *condition &= ~FM_ACL_MATCH_L4_DST_PORT;
        *condition |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
    }

    if (*condition & FM_ACL_MATCH_INGRESS_PORT_SET)
    {
        *condition &= ~FM_ACL_MATCH_INGRESS_PORT_SET;
        *condition |= FM_ACL_MATCH_SOURCE_MAP;
    }

ABORT:
    return err;

}   /* end TranslateConditionMask */




/*****************************************************************************/
/** TranslateL4Port
 * \ingroup intFlow
 *
 * \desc            Translate the FM_ACL_MATCH_L4_***_PORT condition into
 *                  FM_ACL_MATCH_L4_***_PORT_WITH_MASK.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       l4Src indicates if the translated L4 port condition is the
 *                  source or the destination.
 *
 * \param[in,out]   condition refers to the actual flow condition that must be
 *                  translated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT invalid condition set.
 *
 *****************************************************************************/
static fm_status TranslateL4Port(fm_int            sw,
                                 fm_bool           l4Src,
                                 fm_aclCondition  *condition)
{
    fm_status err = FM_OK;

    /* L4 Src/Dst ports must be paired with protocols. */
    if ((*condition & FM_ACL_MATCH_PROTOCOL) == 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (l4Src)
    {
        *condition &= ~FM_ACL_MATCH_L4_SRC_PORT;
        *condition |= FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK;
    }
    else
    {
        *condition &= ~FM_ACL_MATCH_L4_DST_PORT;
        *condition |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
    }

    return err;

}   /* end TranslateL4Port */




/*****************************************************************************/
/** TranslatePortSet
 * \ingroup intFlow
 *
 * \desc            Translate the FM_ACL_MATCH_INGRESS_PORT_SET condition into
 *                  mapped value using condition FM_ACL_MATCH_SOURCE_MAP. This
 *                  is intended to have a better control of the different
 *                  portSet allocated while making sure that key usage is
 *                  constant.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   condition refers to the actual flow condition that must be
 *                  translated.
 *
 * \param[in,out]   condVal refers to the actual flow condition arguments that
 *                  must be translated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslatePortSet(fm_int            sw,
                                  fm_aclCondition  *condition,
                                  fm_aclValue      *condVal)
{
    fm_switch           *switchPtr;
    fm10000_switch      *switchExt;
    fm_sourceMapperValue srcMapValue[FM10000_FFU_MAP_SRC_ENTRIES];
    fm_sourceMapperValue newSrcMapValue;
    fm_int               nEntries;
    fm_portSet          *portSetEntry;
    fm_int               logicalPort;
    fm_int               i;
    fm_int               j;
    fm_status            err = FM_OK;
    fm_bool              portSetLockTaken = FALSE;
    fm_bitArray          portSetPorts;
    fm_bool              portSetPortsCreated = FALSE;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    /* Special Port Set don't use any mapper resources */
    if (condVal->portSet >= 0)
    {
        /* Try to match the actual configured portSet and use it in case mapped
         * value already exist. */
        for (i = 0 ; i < FM10000_FLOW_PORTSET_NUM ; i++)
        {
            if (switchExt->flowInfo.portSetMap[i] == condVal->portSet)
            {
                /* Translate the portSet into a mapped one. */
                *condition &= ~FM_ACL_MATCH_INGRESS_PORT_SET;
                *condition |= FM_ACL_MATCH_SOURCE_MAP;
                condVal->mappedSourcePort = 1 << i;
                condVal->mappedSourcePortMask = 1 << i;

                /* Keep track of the usage of each portSet */
                switchExt->flowInfo.portSetCnt[i]++;
                break;
            }
        }

        /* Did not find any matching entry */
        if (i == FM10000_FLOW_PORTSET_NUM)
        {
            for (j = 0 ; j < FM10000_FLOW_PORTSET_NUM ; j++)
            {
                if (switchExt->flowInfo.portSetMap[j] == -1)
                {
                    break;
                }
            }

            /* All portSet already used. */
            if (j == FM10000_FLOW_PORTSET_NUM)
            {
                return FM_ERR_NO_PORT_SET;
            }

            err = fmCreateBitArray(&portSetPorts,
                                   switchPtr->numCardinalPorts);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            portSetPortsCreated = TRUE;

            /* Protect the software state of portSetTree and its entries */
            TAKE_PORTSET_LOCK(sw);
            portSetLockTaken = TRUE;

            /* Is this portSet defined in the switch? */
            err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                             condVal->portSet,
                             (void**) &portSetEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            /* Copy the portSet content into a local copy */
            err = fmCopyBitArray(&portSetPorts,
                                 &portSetEntry->associatedPorts);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            DROP_PORTSET_LOCK(sw);
            portSetLockTaken = FALSE;

            err = fmGetMapper(sw,
                              FM_MAPPER_SOURCE,
                              &nEntries,
                              srcMapValue,
                              FM10000_FFU_MAP_SRC_ENTRIES);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            /* Translate the portSet into a mapped one. */
            *condition &= ~FM_ACL_MATCH_INGRESS_PORT_SET;
            *condition |= FM_ACL_MATCH_SOURCE_MAP;
            condVal->mappedSourcePort = 1 << j;
            condVal->mappedSourcePortMask = 1 << j;

            switchExt->flowInfo.portSetMap[j] = condVal->portSet;
            switchExt->flowInfo.portSetCnt[j] = 1;

            /* Extract all ports from the portSet configuration */
            err = fmFindPortInBitArray(sw,
                                       &portSetPorts,
                                       -1,
                                       &logicalPort,
                                       FM_ERR_NO_PORT_SET_PORT);
            i = 0;

            while (err == FM_OK)
            {
                while ( (i < nEntries) &&
                        (srcMapValue[i].sourcePort < logicalPort) )
                {
                    i++;
                }

                /* Update a logical port mapped value in case this ingress
                 * port is already part of another portSet or add this port
                 * into the mapper structure in case it is the first time
                 * this port is part of a portSet. */
                if ( (i < nEntries) &&
                     (srcMapValue[i].sourcePort == logicalPort) )
                {
                    err = fmDeleteMapperEntry(sw,
                                              FM_MAPPER_SOURCE,
                                              &srcMapValue[i],
                                              FM_MAPPER_ENTRY_MODE_CACHE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    srcMapValue[i].mappedSourcePortValue |= 1 << j;

                    err = fmAddMapperEntry(sw,
                                           FM_MAPPER_SOURCE,
                                           &srcMapValue[i],
                                           FM_MAPPER_ENTRY_MODE_APPLY);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                else
                {
                    newSrcMapValue.sourcePort = logicalPort;
                    newSrcMapValue.mappedSourcePortValue = 1 << j;

                    err = fmAddMapperEntry(sw,
                                           FM_MAPPER_SOURCE,
                                           &newSrcMapValue,
                                           FM_MAPPER_ENTRY_MODE_APPLY);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }

                err = fmFindPortInBitArray(sw,
                                           &portSetPorts,
                                           logicalPort,
                                           &logicalPort,
                                           FM_ERR_NO_PORT_SET_PORT);
            }

            err = FM_OK;
        }
    }

ABORT:
    if (portSetLockTaken)
    {
        DROP_PORTSET_LOCK(sw);
    }
    if (portSetPortsCreated)
    {
        fmDeleteBitArray(&portSetPorts);
    }

    return err;

}   /* end TranslatePortSet */




/*****************************************************************************/
/** RemoveMappedEntry
 * \ingroup intFlow
 *
 * \desc            Remove and adjust usage of mapped entries.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       aclCond refers to the actual flow condition of the rule that
 *                  must be removed
 *
 * \param[in]       aclValue refers to the actual flow condition arguments of
 *                  the rule that must be removed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RemoveMappedEntry(fm_int           sw,
                                   fm_aclCondition  aclCond,
                                   fm_aclValue     *aclValue)
{
    fm10000_switch      *switchExt;
    fm_status            err = FM_OK;
    fm_sourceMapperValue srcMapValue[FM10000_FFU_MAP_SRC_ENTRIES];
    fm_int               nEntries;
    fm_int               i;
    fm_int               j;

    switchExt = GET_SWITCH_EXT(sw);

    /* Rule contain mapped port condition. */
    if (aclCond & FM_ACL_MATCH_SOURCE_MAP)
    {
        /* Find portSet linked to this rule and reduce usage. */
        for (i = 0 ; i < FM10000_FLOW_PORTSET_NUM; i++)
        {
            if (aclValue->portSet == switchExt->flowInfo.portSetMap[i])
            {
                switchExt->flowInfo.portSetCnt[i]--;
                break;
            }
        }
        if (i == FM10000_FLOW_PORTSET_NUM)
        {
            return FM_FAIL;
        }

        /* Usage of 0 means that we should remove this portSet from the
         * system. */
        if (switchExt->flowInfo.portSetCnt[i] == 0)
        {
            err = fmGetMapper(sw,
                              FM_MAPPER_SOURCE,
                              &nEntries,
                              srcMapValue,
                              FM10000_FFU_MAP_SRC_ENTRIES);
            if (err != FM_OK)
            {
                return err;
            }

            /* Update every port mapper entry and remove bit related to this
             * removed portSet. */
            for (j = 0 ; j < nEntries ; j++)
            {
                if (srcMapValue[j].mappedSourcePortValue & (1 << i))
                {
                    /* Last portSet configured for this ingress port. */
                    if ((srcMapValue[j].mappedSourcePortValue & ~(1 << i)) == 0)
                    {
                        err = fmDeleteMapperEntry(sw,
                                                  FM_MAPPER_SOURCE,
                                                  &srcMapValue[j],
                                                  FM_MAPPER_ENTRY_MODE_APPLY);
                        if (err != FM_OK)
                        {
                            return err;
                        }
                    }
                    /* Update this ingress port. */
                    else
                    {
                        err = fmDeleteMapperEntry(sw,
                                                  FM_MAPPER_SOURCE,
                                                  &srcMapValue[j],
                                                  FM_MAPPER_ENTRY_MODE_CACHE);
                        if (err != FM_OK)
                        {
                            return err;
                        }

                        srcMapValue[j].mappedSourcePortValue &= ~(1 << i);
                        err = fmAddMapperEntry(sw,
                                               FM_MAPPER_SOURCE,
                                               &srcMapValue[j],
                                               FM_MAPPER_ENTRY_MODE_APPLY);
                        if (err != FM_OK)
                        {
                            return err;
                        }
                    }
                }
            }
            /* Return portSet to the active pool */
            switchExt->flowInfo.portSetMap[i] = -1;
        }
    }

    return err;

}   /* end RemoveMappedEntry */




/*****************************************************************************/
/** VerifyFlowConditionsMasks
 * \ingroup intFlow
 *
 * \desc            Verify conditions masks defined for a flow: compare it with
 *                  conditions masks defined for a table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table to delete.
 *
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify the flow. See ''Flow Condition Masks'' for the
 *                  definitions of each bit in the mask.
 *
 * \param[in]       condVal points to a ''fm_flowValue'' structure containing
 *                  the values and masks to match against for the specified
 *                  condition. Values are ignored in this functions. Only masks
 *                  for specified conditions are verified.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 * \return          FM_ERR_UNSUPORTED if provided conditions masks cannot be
 *                  supported by a flow table.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid arguments.
 *
 *****************************************************************************/
static fm_status VerifyFlowConditionsMasks(fm_int           sw,
                                           fm_int           tableIndex,
                                           fm_flowCondition condition,
                                           fm_flowValue    *condVal)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_int          i;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw=%d, tableIndex=%d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);

    if (condVal == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition | switchExt->flowInfo.table[tableIndex].condition) !=
            switchExt->flowInfo.table[tableIndex].condition )
    {
        /* Flow condition must be a subset of a table condition. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_DST_IP) &&
            (((condVal->dstIpMask.addr[0] |
               switchExt->flowInfo.table[tableIndex].condMasks.dstIpMask.addr[0]) !=
               switchExt->flowInfo.table[tableIndex].condMasks.dstIpMask.addr[0]) ) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_SRC_IP) &&
            (((condVal->srcIpMask.addr[0] |
               switchExt->flowInfo.table[tableIndex].condMasks.srcIpMask.addr[0]) !=
               switchExt->flowInfo.table[tableIndex].condMasks.srcIpMask.addr[0]) ) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_DST_MAC) &&
            ( (condVal->dstMask |
               switchExt->flowInfo.table[tableIndex].condMasks.dstMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.dstMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_SRC_MAC) &&
            ( (condVal->srcMask |
               switchExt->flowInfo.table[tableIndex].condMasks.srcMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.srcMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_TOS) &&
            ( (condVal->tosMask |
               switchExt->flowInfo.table[tableIndex].condMasks.tosMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.tosMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_VLAN) &&
            ( (condVal->vlanIdMask |
               switchExt->flowInfo.table[tableIndex].condMasks.vlanIdMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.vlanIdMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_VLAN_PRIORITY) &&
            ( (condVal->vlanPriMask |
               switchExt->flowInfo.table[tableIndex].condMasks.vlanPriMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.vlanPriMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (condition & FM_FLOW_MATCH_L4_DEEP_INSPECTION)
    {
        for (i = 0 ; i < FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES; i++)
        {
            if ( (condVal->L4DeepInspectionMask[i] |
                  switchExt->flowInfo.table[tableIndex].condMasks.L4DeepInspectionMask[i]) !=
                 switchExt->flowInfo.table[tableIndex].condMasks.L4DeepInspectionMask[i] )
            {
                /* Flow condition mask must be a subset of a table condition mask. */
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
        }
    }

    if (condition & FM_FLOW_MATCH_L2_DEEP_INSPECTION)
    {
        for (i = 0 ; i < FM_MAX_ACL_NON_IP_PAYLOAD_BYTES; i++)
        {
            if ( (condVal->L2DeepInspectionMask[i] |
                  switchExt->flowInfo.table[tableIndex].condMasks.L2DeepInspectionMask[i]) !=
                 switchExt->flowInfo.table[tableIndex].condMasks.L2DeepInspectionMask[i] )
            {
                /* Flow condition mask must be a subset of a table condition mask. */
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
        }
    }

    if ( (condition & FM_FLOW_MATCH_SWITCH_PRIORITY) &&
            ( (condVal->switchPriMask |
               switchExt->flowInfo.table[tableIndex].condMasks.switchPriMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.switchPriMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_VLAN2) &&
            ( (condVal->vlanId2Mask |
               switchExt->flowInfo.table[tableIndex].condMasks.vlanId2Mask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.vlanId2Mask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_PRIORITY2) &&
            ( (condVal->vlanPri2Mask |
               switchExt->flowInfo.table[tableIndex].condMasks.vlanPri2Mask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.vlanPri2Mask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (condition & FM_FLOW_MATCH_TTL) &&
            ( (condVal->ttlMask |
               switchExt->flowInfo.table[tableIndex].condMasks.ttlMask) !=
              switchExt->flowInfo.table[tableIndex].condMasks.ttlMask) )
    {
        /* Flow condition mask must be a subset of a table condition mask. */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end VerifyFlowConditionsMasks */




/*****************************************************************************/
/** TranslateFlowToTEAction
 * \ingroup intFlow
 *
 * \desc            Translate the flow actions into tunnel and encap actions.
 *
 * \param[in]       action refers to the actual flow action that must be
 *                  translated.
 *
 * \param[out]      tunnelAction refers to the tunnel action that stores result.
 *
 * \param[out]      encapAction refers to the encap action that stores result.
 *
 * \param[in]       tunnelEncapFlowParam points to the encapsulation flow
 *                  parameters.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateFlowToTEAction(fm_flowAction      *action,
                                         fm_tunnelAction    *tunnelAction,
                                         fm_tunnelEncapFlow *encapAction,
                                         fm_tunnelEncapFlowParam const *tunnelEncapFlowParam)
{
    fm_status err = FM_OK;

    *tunnelAction = 0;
    *encapAction = 0;

    /* Translate flow action to a tunnel action */
    if ( *action & FM_FLOW_ACTION_SET_DIP )
    {
        *tunnelAction |= FM_TUNNEL_SET_DIP;
    }

    if ( *action & FM_FLOW_ACTION_SET_SIP )
    {
        *tunnelAction |= FM_TUNNEL_SET_SIP;
    }

    if ( *action & FM_FLOW_ACTION_SET_L4DST )
    {
        *tunnelAction |= FM_TUNNEL_SET_L4DST;
    }

    if ( *action & FM_FLOW_ACTION_SET_L4SRC )
    {
        *tunnelAction |= FM_TUNNEL_SET_L4SRC;
    }

    if ( *action & FM_FLOW_ACTION_SET_TTL )
    {
        *tunnelAction |= FM_TUNNEL_SET_TTL;
    }

    if ( *action & FM_FLOW_ACTION_SET_DMAC )
    {
        *tunnelAction |= FM_TUNNEL_SET_DMAC;
    }

    if ( *action & FM_FLOW_ACTION_SET_SMAC )
    {
        *tunnelAction |= FM_TUNNEL_SET_SMAC;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_VNI )
    {
        *tunnelAction |= FM_TUNNEL_SET_VNI;
    }

    if ( *action & FM_FLOW_ACTION_COUNT )
    {
        *tunnelAction |= FM_TUNNEL_COUNT;
    }

    if ( *action & FM_FLOW_ACTION_DECAP_KEEP )
    {
        *tunnelAction |= FM_TUNNEL_DECAP_KEEP_OUTER_HDR;
    }

    if ( *action & FM_FLOW_ACTION_DECAP_MOVE )
    {
        *tunnelAction |= FM_TUNNEL_DECAP_MOVE_OUTER_HDR;
    }

    /* Translate flow action to a encap action */
    if (tunnelEncapFlowParam->type == FM_TUNNEL_TYPE_GPE_NSH)
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_GPE_NSH_ALL;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_SIP )
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_SIP;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_TTL )
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_TTL;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_L4DST )
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_L4DST;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_L4SRC )
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_L4SRC;
    }

    if ( *action & FM_FLOW_ACTION_ENCAP_NGE )
    {
        *encapAction |= FM_TUNNEL_ENCAP_FLOW_NGE;
    }

    return err;

}   /* end TranslateTEAction */




/*****************************************************************************/
/** TranslateTEToFlowAction
 * \ingroup intFlow
 *
 * \desc            Translate the flow actions into tunnel and encap actions.
 *
 * \param[in]       tunnelAction refers to the tunnel action that must be
 *                  translated.
 *
 * \param[in]       encapAction refers to the encap action that must be
 *                  translated.
 *
 * \param[out]      action refers to the actual flow action that stores
 *                  the results.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateTEToFlowAction(fm_tunnelAction    *tunnelAction,
                                         fm_tunnelEncapFlow *encapAction,
                                         fm_flowAction      *action)
{
    fm_status err = FM_OK;

    *action = 0;

    /* Translate flow action to a tunnel action */
    if ( *tunnelAction & FM_TUNNEL_SET_DIP )
    {
        *action |= FM_FLOW_ACTION_SET_DIP;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_SIP )
    {
        *action |= FM_FLOW_ACTION_SET_SIP;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_L4DST )
    {
        *action |= FM_FLOW_ACTION_SET_L4DST;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_L4SRC )
    {
        *action |= FM_FLOW_ACTION_SET_L4SRC;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_TTL )
    {
        *action |= FM_FLOW_ACTION_SET_TTL;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_DMAC )
    {
        *action |= FM_FLOW_ACTION_SET_DMAC;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_SMAC )
    {
        *action |= FM_FLOW_ACTION_SET_SMAC;
    }

    if ( *tunnelAction & FM_TUNNEL_SET_VNI )
    {
        *action |= FM_FLOW_ACTION_ENCAP_VNI;
    }

    if ( *tunnelAction & FM_TUNNEL_COUNT )
    {
        *action |= FM_FLOW_ACTION_COUNT;
    }

    if ( *tunnelAction & FM_TUNNEL_DECAP_KEEP_OUTER_HDR )
    {
        *action |= FM_FLOW_ACTION_DECAP_KEEP;
    }

    if ( *tunnelAction & FM_TUNNEL_DECAP_MOVE_OUTER_HDR )
    {
        *action |= FM_FLOW_ACTION_DECAP_MOVE;
    }

    /* Translate flow action to a encap action */
    if ( *encapAction & FM_TUNNEL_ENCAP_FLOW_SIP )
    {
        *action |= FM_FLOW_ACTION_ENCAP_SIP;
    }

    if ( *encapAction & FM_TUNNEL_ENCAP_FLOW_TTL )
    {
        *action |= FM_FLOW_ACTION_ENCAP_TTL;
    }

    if ( *encapAction & FM_TUNNEL_ENCAP_FLOW_L4DST )
    {
        *action |= FM_FLOW_ACTION_ENCAP_L4DST;
    }

    if ( *encapAction & FM_TUNNEL_ENCAP_FLOW_L4SRC )
    {
        *action |= FM_FLOW_ACTION_ENCAP_L4SRC;
    }

    if ( *encapAction & FM_TUNNEL_ENCAP_FLOW_NGE )
    {
        *action |= FM_FLOW_ACTION_ENCAP_NGE;
    }

    return err;

}   /* end TranslateFlowAction */




/*****************************************************************************/
/** TranslateFlowToTECondition
 * \ingroup intFlow
 *
 * \desc            Translate the flow condition into tunnel one.
 *
 * \param[in]       condition refers to the actual flow condition that must be
 *                  translated.
 *
 * \param[out]      tunnelCondition refers to the tunnel condition that
 *                  stores result.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateFlowToTECondition(fm_flowCondition   *condition,
                                            fm_tunnelCondition *tunnelCondition)
{
    fm_status err = FM_OK;

    *tunnelCondition = 0;

    if ( *condition & FM_FLOW_MATCH_SRC_MAC )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_SMAC;
    }

    if ( *condition & FM_FLOW_MATCH_DST_MAC )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_DMAC;
    }

    if ( *condition & FM_FLOW_MATCH_VLAN )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_VLAN;
    }

    if ( *condition & FM_FLOW_MATCH_SRC_IP )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_SIP;
    }

    if ( *condition & FM_FLOW_MATCH_DST_IP )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_DIP;
    }

    if ( *condition & FM_FLOW_MATCH_PROTOCOL )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_PROT;
    }

    if ( *condition & FM_FLOW_MATCH_L4_SRC_PORT )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_L4SRC;
    }

    if ( *condition & FM_FLOW_MATCH_L4_DST_PORT )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_L4DST;
    }

    if ( *condition & FM_FLOW_MATCH_VNI )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_VNI;
    }

    if ( *condition & FM_FLOW_MATCH_VSI_TEP )
    {
        *tunnelCondition |= FM_TUNNEL_MATCH_VSI_TEP;
    }

    return err;

}   /* end TranslateTECondition */





/*****************************************************************************/
/** TranslateTEToFlowCondition
 * \ingroup intFlow
 *
 * \desc            Translate the flow condition into tunnel one.
 *
 * \param[in]       tunnelCondition refers to the actual ACL condition that must be
 *                  translated.
 *
 * \param[out]      condition refers to the flow condition that
 *                  stores result.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateTEToFlowCondition(fm_tunnelCondition *tunnelCondition,
                                            fm_flowCondition   *condition)
{
    fm_status err = FM_OK;

    *condition = 0;

    if ( *tunnelCondition & FM_TUNNEL_MATCH_SMAC )
    {
        *condition |= FM_FLOW_MATCH_SRC_MAC;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_DMAC )
    {
        *condition |= FM_FLOW_MATCH_DST_MAC ;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_VLAN )
    {
        *condition |= FM_FLOW_MATCH_VLAN;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_SIP )
    {
        *condition |= FM_FLOW_MATCH_SRC_IP;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_DIP )
    {
        *condition |= FM_FLOW_MATCH_DST_IP;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_PROT )
    {
        *condition |= FM_FLOW_MATCH_PROTOCOL;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_L4SRC )
    {
        *condition |= FM_FLOW_MATCH_L4_SRC_PORT;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_L4DST )
    {
        *condition |= FM_FLOW_MATCH_L4_DST_PORT ;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_VNI )
    {
        *condition |= FM_FLOW_MATCH_VNI ;
    }

    if ( *tunnelCondition & FM_TUNNEL_MATCH_VSI_TEP )
    {
        *condition |= FM_FLOW_MATCH_VSI_TEP ;
    }

    return err;

}   /* end TranslateTECondition */




/*****************************************************************************/
/** ACLActionsToPreallocate
 * \ingroup intFlow
 *
 * \desc            Return ACL actions to preallocate.
 *
 * \param[in]       maxAction is the number of actions to preallocate.
 *
 * \param[in]       countEnabled indicate whether the table supports counting.
 *
 * \param[out]      actions refers to result that stores ACL actions to
 *                  preallocate.
 *
 * \param[in]       param is not used.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ACLActionsToPreallocate(fm_int           maxAction,
                                         fm_bool          countEnabled,
                                         fm_aclActionExt *actions,
                                         fm_aclParamExt  *param)
{
    fm_status err;

    err = FM_OK;

    switch (maxAction)
    {
        case 1:
            *actions = FM_ACL_ACTIONEXT_REDIRECT;
            break;
        case 2:
            *actions = FM_ACL_ACTIONEXT_REDIRECT |
                       FM_ACL_ACTIONEXT_SET_VLAN;
            break;
        case 3:
            *actions = FM_ACL_ACTIONEXT_LOG |
                       FM_ACL_ACTIONEXT_SET_VLAN |
                       FM_ACL_ACTIONEXT_SET_FLOOD_DEST;
            break;
        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (countEnabled)
    {
        *actions = *actions | FM_ACL_ACTIONEXT_COUNT;
    }

    return err;

}   /* end ACLActionsToPreallocate */




/*****************************************************************************/
/** CountActionSlicesNeeded
 * \ingroup intFlow
 *
 * \desc            Count number of action slieces needed in FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       action is a bit mask of ACL actions.
 *
 * \param[in]       param is a parameter associated with the action.
 *
 * \param[out]      slicesCount refers to result that stores number of action
 *                  slices needed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CountActionSlicesNeeded(fm_int          sw,
                                         fm_aclActionExt action,
                                         fm_aclParamExt  param,
                                         fm_int         *slicesCount)
{
    fm_aclRule rule;
    fm_status  err;

    FM_CLEAR(rule);
    rule.action = action;
    rule.param = param;

    err = fm10000CountActionSlicesNeeded(sw, NULL, &rule, slicesCount);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    return err;

}   /* end CountActionSlicesNeeded */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000CreateFlowTCAMTable
 * \ingroup intFlow
 *
 * \desc            Creates a flow table in the FFU TCAM.
 *
 * \note            Flow TCAM tables and ACLs share the same number space for
 *                  tableIndex numbers and acl numbers used in fmCreateACLExt
 *                  function. Unique numbers must be provided when using both
 *                  Flow API and ACL API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is used to order this table with respect to other
 *                  TCAM tables. The higher the tableIndex, the higher the
 *                  precedence. tableIndex must be less than
 *                  ''FM_FLOW_MAX_TABLE_TYPE''.
 *
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify flows that may be added to this table. See
 *                  ''Flow Condition Masks'' for the definitions of each bit
 *                  in the mask.
 *
 * \param[in]       maxEntries is the size of the flow table and must be
 *                  no larger than ''FM10000_MAX_RULE_PER_FLOW_TABLE''.
 *
 * \param[in]       maxAction is the maximum number of actions that can be
 *                  set for a flow in created table. FM_FLOW_ACTION_COUNT
 *                  is excluded from maxAction calculation.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the specified condition is not
 *                  supported for this table.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 *
 *****************************************************************************/
fm_status fm10000CreateFlowTCAMTable(fm_int           sw,
                                     fm_int           tableIndex,
                                     fm_flowCondition condition,
                                     fm_uint32        maxEntries,
                                     fm_uint32        maxAction)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_int          acl;
    fm_aclValue     aclConditionMask;
    fm_aclParamExt  aclParam;
    fm_char         statusText[1024];
    fm10000_switch *switchExt;
    fm_uint32       i;
    fm_int          defFlowId;
    fm_uint32       addedEntries;
    fm_bool         keepUnusedKey;
    fm_flowValue   *condMasks;
    fm_aclActionExt action;
    fm_aclCondition aclCondition;
    fm_bool         aclCreated;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw=%d, tableIndex=%d, condition=%llx, maxEntries=%d, "
                 "maxAction= %d\n",
                 sw,
                 tableIndex,
                 (fm_uint64) condition,
                 maxEntries,
                 maxAction);

    acl        = FM10000_FLOW_BASE_ACL + tableIndex;
    switchPtr  = GET_SWITCH_PTR(sw);
    switchExt  = GET_SWITCH_EXT(sw);
    aclCreated = FALSE;
    action     = 0;
    FM_CLEAR(aclParam);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].created == TRUE)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = ACLActionsToPreallocate(maxAction,
                                  switchExt->flowInfo.table[tableIndex].countSupported,
                                  &action,
                                  &aclParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = CountActionSlicesNeeded(sw,
                                  action,
                                  aclParam,
                                  &switchExt->flowInfo.table[tableIndex].preallocatedSlices);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.table[tableIndex].maxAction = maxAction;

    condMasks = &switchExt->flowInfo.table[tableIndex].condMasks;

    if (condMasks == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (maxEntries > FM10000_MAX_RULE_PER_FLOW_TABLE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( ( condition & (FM_FLOW_MATCH_VNI |
                        FM_FLOW_MATCH_VSI_TEP) ) != 0 )
    {
        /* Above listed conditions are TE table specific */
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switchExt->flowInfo.table[tableIndex].condition = condition;

    err = TranslateFlowToACLCondition(sw, &condition, &aclCondition);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = TranslateConditionMask(sw, &aclCondition);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmCreateACLInt(sw,
                         acl,
                         switchExt->flowInfo.table[tableIndex].scenario,
                         FM_ACL_DEFAULT_PRECEDENCE,
                         TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    aclCreated = TRUE;

    if (switchExt->flowInfo.table[tableIndex].withPriority)
    {
        addedEntries = 1;
        keepUnusedKey = TRUE;

        err = fmSetACLAttribute(sw,
                                acl,
                                FM_ACL_KEEP_UNUSED_KEYS,
                                &keepUnusedKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        addedEntries = maxEntries;
    }

    /* Create an incremental ACL emulation according to required
     * parameters */

    /* Set condition mask */
    err = fmConvertFlowToACLValue(condMasks, &aclConditionMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (aclConditionMask.dstIp.isIPv6)
    {
        aclConditionMask.dstIpMask.isIPv6 = TRUE;
        aclConditionMask.frameType = FM_ACL_FRAME_TYPE_IPV6;
    }
    else if (aclConditionMask.srcIp.isIPv6)
    {
        aclConditionMask.srcIpMask.isIPv6 = TRUE;
        aclConditionMask.frameType = FM_ACL_FRAME_TYPE_IPV6;
    }
    else
    {
        aclConditionMask.dstIpMask.isIPv6 = FALSE;
        aclConditionMask.srcIpMask.isIPv6 = FALSE;
        aclConditionMask.frameType = FM_ACL_FRAME_TYPE_IPV4;
    }

    aclConditionMask.srcGlortMask = 0xffff;

    aclParam.logicalPort = switchPtr->cpuPort;

    for (i = 0 ; i < addedEntries ; i++)
    {
        err = fmAddACLRuleExt(sw,
                              acl,
                              i,
                              aclCondition,
                              &aclConditionMask,
                              action,
                              &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmCompileACLExt(sw,
                          statusText,
                          sizeof(statusText),
                          FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                          FM_ACL_COMPILE_FLAG_INTERNAL,
                          (void*) &acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmApplyACLExt(sw,
                        FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                        FM_ACL_APPLY_FLAG_INTERNAL,
                        (void*) &acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    for (i = 0 ; i < addedEntries ; i++)
    {
        err = fmSetACLRuleState(sw, acl, i, FM_ACL_RULE_ENTRY_STATE_INVALID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Table with Priority flow does not need any preallocated flows but since
     * the ACL is configured in a way that it kept the configured keys, no
     * need to worry about key optimization anymore. */
    if (switchExt->flowInfo.table[tableIndex].withPriority)
    {
        err = fmDeleteACLRule(sw, acl, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmCompileACLExt(sw,
                              statusText,
                              sizeof(statusText),
                              FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                              FM_ACL_COMPILE_FLAG_INTERNAL,
                              (void*) &acl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmApplyACLExt(sw,
                            FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                            FM_ACL_APPLY_FLAG_INTERNAL,
                            (void*) &acl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Allocate memory needed to manage this tableIndex */
    err = fmCreateBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                           maxEntries);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.table[tableIndex].lastCnt =
        fmAlloc(maxEntries * sizeof(fm_flowCounters));
    if (switchExt->flowInfo.table[tableIndex].lastCnt == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].lastCnt,
                maxEntries * sizeof(fm_flowCounters),
                0,
                maxEntries * sizeof(fm_flowCounters));

    switchExt->flowInfo.table[tableIndex].useBit =
        fmAlloc(maxEntries * sizeof(fm_bool));
    if (switchExt->flowInfo.table[tableIndex].useBit == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].useBit,
                maxEntries * sizeof(fm_bool),
                0,
                maxEntries * sizeof(fm_bool));

    switchExt->flowInfo.table[tableIndex].mapping =
        fmAlloc(maxEntries * sizeof(fm_int));
    if (switchExt->flowInfo.table[tableIndex].mapping == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].mapping,
                maxEntries * sizeof(fm_int),
                -1,
                maxEntries * sizeof(fm_int));

    switchExt->flowInfo.table[tableIndex].useCnt =
        fmAlloc(maxEntries * sizeof(fm_int));
    if (switchExt->flowInfo.table[tableIndex].useCnt == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].useCnt,
                maxEntries * sizeof(fm_int),
                0,
                maxEntries * sizeof(fm_int));

    if ( switchExt->flowInfo.table[tableIndex].withDefault &&
            switchExt->flowInfo.table[tableIndex].withPriority == FALSE )
    {
        /* Install the Default rule */
        err = AddDefaultRule(sw, tableIndex, &defFlowId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switchExt->flowInfo.table[tableIndex].type = FM_FLOW_TCAM_TABLE;
    switchExt->flowInfo.table[tableIndex].created = TRUE;

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);

ABORT:

    if (aclCreated)
    {
        fmDeleteACLInt(sw, acl, TRUE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000CreateFlowTCAMTable */




/*****************************************************************************/
/** fm10000DeleteFlowTCAMTable
 * \ingroup intFlow
 *
 * \desc            Deletes a flow table in the FFU TCAM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the tableIndex to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex already used.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if at least a flow is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fm10000DeleteFlowTCAMTable(fm_int sw,
                                     fm_int tableIndex)
{
    fm_status       err = FM_OK;
    fm_int          acl;
    fm_int          flowIndex;
    fm_char         statusText[1024];
    fm10000_switch *switchExt;
    fm_aclCondition aclCond;
    fm_aclValue     aclValue;
    fm_aclActionExt aclAction;
    fm_aclParamExt  aclParam;
    fm_int          mappedFlow;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw=%d, tableIndex=%d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);
    acl = FM10000_FLOW_BASE_ACL + tableIndex;

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].created == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate that every flow are unused first */
    for (flowIndex = 0 ;
         flowIndex < switchExt->flowInfo.table[tableIndex].idInUse.bitCount ;
         flowIndex++)
    {
        if (switchExt->flowInfo.table[tableIndex].useCnt[flowIndex])
        {
            err = FM_ERR_FFU_RESOURCE_IN_USE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }
    }

    err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                              0,
                              TRUE,
                              &flowIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    while (flowIndex >= 0)
    {
        mappedFlow = switchExt->flowInfo.table[tableIndex].mapping[flowIndex];

        /* Adjust the mapped value */
        err = fmGetACLRule(sw,
                           acl,
                           mappedFlow,
                           &aclCond,
                           &aclValue,
                           &aclAction,
                           &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = RemoveMappedEntry(sw, aclCond, &aclValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Clear flow ID bit */
        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               flowIndex,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Find next flow ID */
        err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                                  0,
                                  TRUE,
                                  &flowIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmDeleteACLInt(sw, acl, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmCompileACLExt(sw,
                          statusText,
                          sizeof(statusText),
                          FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                          FM_ACL_COMPILE_FLAG_INTERNAL,
                          (void*) &acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmApplyACLExt(sw,
                        FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                        FM_ACL_APPLY_FLAG_INTERNAL,
                        (void*) &acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    fmFree(switchExt->flowInfo.table[tableIndex].lastCnt);
    fmFree(switchExt->flowInfo.table[tableIndex].useBit);
    fmFree(switchExt->flowInfo.table[tableIndex].mapping);
    fmFree(switchExt->flowInfo.table[tableIndex].useCnt);
    fmDeleteBitArray(&switchExt->flowInfo.table[tableIndex].idInUse);

    switchExt->flowInfo.table[tableIndex].lastCnt = NULL;
    switchExt->flowInfo.table[tableIndex].useBit = NULL;
    switchExt->flowInfo.table[tableIndex].mapping = NULL;

    switchExt->flowInfo.table[tableIndex].created = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DeleteFlowTCAMTable */



/*****************************************************************************/
/** fm10000CreateFlowTETable
 * \ingroup intFlow
 *
 * \desc            Creates a flow table in the Tunnel Engine.
 *
 * \note            The frame must be redirected by
 *                  ''FM_FLOW_ACTION_REDIRECT_TUNNEL'' or
 *                  ''FM_FLOW_ACTION_BALANCE'' from TCAM table to be processed
 *                  in TE table.
 *
 * \note            The TE tables with non zero condition are based on TE hash
 *                  groups. Such tables support only exact matches (not masked).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is used to order this table with respect to other
 *                  TCAM tables. The higher the tableIndex, the higher the
 *                  precedence. tableIndex must be less than
 *                  ''FM_FLOW_MAX_TABLE_TYPE''.
 *
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify flows that may be added to this table. See
 *                  ''Flow Condition Masks'' for the definitions of each bit
 *                  in the mask.
 *
 * \param[in]       maxEntries is the size of the flow table and must be
 *                  no larger than ''FM10000_MAX_RULE_PER_FLOW_TE_TABLE''.
 *
 * \param[in]       maxAction is not used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if the specified condition is not
 *                  supported for this table.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000CreateFlowTETable(fm_int           sw,
                                   fm_int           tableIndex,
                                   fm_flowCondition condition,
                                   fm_uint32        maxEntries,
                                   fm_uint32        maxAction)
{
    fm_status          err = FM_OK;
    fm10000_switch    *switchExt;
    fm_tunnelParam     tunnelParam;
    fm_tunnelCondition tunnelCondition;
    fm_int             group;
    fm_bool            defaultDglort = TRUE;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw=%d, tableIndex=%d, condition=%llx, maxEntries=%d, "
                 "maxAction= %d\n",
                 sw,
                 tableIndex,
                 (fm_uint64) condition,
                 maxEntries,
                 maxAction);

    FM_NOT_USED(maxAction);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (maxEntries > FM10000_MAX_RULE_PER_FLOW_TE_TABLE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].created == TRUE)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( ( condition & (FM_FLOW_MATCH_ETHERTYPE |
                        FM_FLOW_MATCH_VLAN_PRIORITY |
                        FM_FLOW_MATCH_INGRESS_PORT_SET |
                        FM_FLOW_MATCH_TOS |
                        FM_FLOW_MATCH_FRAME_TYPE |
                        FM_FLOW_MATCH_SRC_PORT |
                        FM_FLOW_MATCH_TCP_FLAGS |
                        FM_FLOW_MATCH_L4_DEEP_INSPECTION |
                        FM_FLOW_MATCH_L2_DEEP_INSPECTION |
                        FM_FLOW_MATCH_SWITCH_PRIORITY |
                        FM_FLOW_MATCH_VLAN_TAG_TYPE |
                        FM_FLOW_MATCH_VLAN2 |
                        FM_FLOW_MATCH_PRIORITY2 |
                        FM_FLOW_MATCH_FRAG |
                        FM_FLOW_MATCH_LOGICAL_PORT |
                        FM_FLOW_MATCH_TTL) ) != 0 )
    {
        /* Above listed conditions are TCAM table specific */
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switchExt->flowInfo.table[tableIndex].maxAction = maxAction;

    switchExt->flowInfo.table[tableIndex].condition = condition;
    err = TranslateFlowToTECondition(&condition, &tunnelCondition);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    FM_MEMSET_S(&tunnelParam, sizeof(fm_tunnelParam), 0, sizeof(fm_tunnelParam));

    tunnelParam.te = switchExt->flowInfo.table[tableIndex].te;
    tunnelParam.encap = switchExt->flowInfo.table[tableIndex].encap;
    tunnelParam.size = maxEntries;
    tunnelParam.hashKeyConfig = tunnelCondition;

    /* TEP Matching required some GLORTs to carry the meta data */
    if ((tunnelParam.encap == FALSE) && (condition & FM_FLOW_MATCH_VSI_TEP))
    {
        tunnelParam.tepSize = FM10000_DEFAULT_TEP_SIZE;
    }

    err = fm10000CreateTunnel(sw, &group, &tunnelParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.table[tableIndex].group = group;

    err = fm10000SetTunnelAttribute(sw,
                                    group,
                                    0,
                                    FM_TUNNEL_SET_DEFAULT_DGLORT,
                                    &defaultDglort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    /* Allocate memory needed to manage this tableIndex */
    err = fmCreateBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                           maxEntries);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.table[tableIndex].lastCnt =
        fmAlloc(maxEntries * sizeof(fm_flowCounters));
    if (switchExt->flowInfo.table[tableIndex].lastCnt == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].lastCnt,
                maxEntries * sizeof(fm_flowCounters),
                0,
                maxEntries * sizeof(fm_flowCounters));

    switchExt->flowInfo.table[tableIndex].useBit =
        fmAlloc(maxEntries * sizeof(fm_bool));
    if (switchExt->flowInfo.table[tableIndex].useBit == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].useBit,
                maxEntries * sizeof(fm_bool),
                0,
                maxEntries * sizeof(fm_bool));

    switchExt->flowInfo.table[tableIndex].mapping =
        fmAlloc(maxEntries * sizeof(fm_int));
    if (switchExt->flowInfo.table[tableIndex].mapping == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].mapping,
                maxEntries * sizeof(fm_int),
                -1,
                maxEntries * sizeof(fm_int));

    switchExt->flowInfo.table[tableIndex].useCnt =
        fmAlloc(maxEntries * sizeof(fm_int));
    if (switchExt->flowInfo.table[tableIndex].useCnt == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S(switchExt->flowInfo.table[tableIndex].useCnt,
                maxEntries * sizeof(fm_int),
                0,
                maxEntries * sizeof(fm_int));

    switchExt->flowInfo.table[tableIndex].type = FM_FLOW_TE_TABLE;
    switchExt->flowInfo.table[tableIndex].created = TRUE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000CreateFlowTETable */




/*****************************************************************************/
/** fm10000DeleteFlowTETable
 * \ingroup intFlow
 *
 * \desc            Deletes a flow table in the Tunnel Engine.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the tableIndex to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if at least a flow is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fm10000DeleteFlowTETable(fm_int sw,
                                   fm_int tableIndex)
{
    fm_status       err = FM_OK;
    fm_int          flowIndex;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw=%d, tableIndex=%d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].created == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate that every flow are unused first */
    for (flowIndex = 0 ;
         flowIndex < switchExt->flowInfo.table[tableIndex].idInUse.bitCount ;
         flowIndex++)
    {
        if (switchExt->flowInfo.table[tableIndex].useCnt[flowIndex])
        {
            err = FM_ERR_FFU_RESOURCE_IN_USE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }
    }

    err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                              0,
                              TRUE,
                              &flowIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    while (flowIndex >= 0)
    {
        /* Clear flow ID bit */
        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               flowIndex,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Find next flow ID */
        err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                                  0,
                                  TRUE,
                                  &flowIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fm10000DeleteTunnel(sw, switchExt->flowInfo.table[tableIndex].group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.table[tableIndex].group = -1;

    fmFree(switchExt->flowInfo.table[tableIndex].lastCnt);
    fmFree(switchExt->flowInfo.table[tableIndex].useBit);
    fmFree(switchExt->flowInfo.table[tableIndex].mapping);
    fmFree(switchExt->flowInfo.table[tableIndex].useCnt);
    fmDeleteBitArray(&switchExt->flowInfo.table[tableIndex].idInUse);

    switchExt->flowInfo.table[tableIndex].lastCnt = NULL;
    switchExt->flowInfo.table[tableIndex].useBit = NULL;
    switchExt->flowInfo.table[tableIndex].mapping = NULL;

    switchExt->flowInfo.table[tableIndex].created = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DeleteFlowTETable */




/*****************************************************************************/
/** fm10000AddFlow
 * \ingroup intFlow
 *
 * \desc            Add a flow entry to the specified table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which this flow should
 *                  be added.
 *
 * \param[in]       priority is the priority for this flow within the table.
 *                  For TE table this functionality is not available.
 *
 * \param[in]       precedence is the inter-table flow precedence.
 *
 * \param[in]       condition is a bit mask of matching conditions used to
 *                  identify the flow. See ''Flow Condition Masks'' for the
 *                  definitions of each bit in the mask. For TE table, this
 *                  mask must equal the condition mask selected for a table.
 *
 * \param[in]       condVal points to a ''fm_flowValue'' structure containing
 *                  the values to match against for the specified condition.
 *
 * \param[in]       action is a bit mask of actions that will occur when a
 *                  packet is identified that matches the flow condition.
 *                  See ''Flow Action Masks'' for definitions of each bit in
 *                  the mask.
 *
 * \param[in]       param points to an ''fm_flowParam'' structure containing
 *                  values used by some actions.
 *
 * \param[in]       flowState is the initial state of the flow (enabled or
 *                  not). A flow in a TE table is always enabled.
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
 *                  produce a valid ACL "binary image" from the current ACL
 *                  configuration.
 *
 *****************************************************************************/
fm_status fm10000AddFlow(fm_int           sw,
                         fm_int           tableIndex,
                         fm_uint16        priority,
                         fm_uint32        precedence,
                         fm_flowCondition condition,
                         fm_flowValue    *condVal,
                         fm_flowAction    action,
                         fm_flowParam    *param,
                         fm_flowState     flowState,
                         fm_int          *flowId)
{
    fm_status                err = FM_OK;
    fm_int                   acl;
    fm_int                   flowIndex;
    fm_aclEntryState         ruleState;
    fm_int                   mappedFlow;
    fm10000_switch          *switchExt;
    fm_aclParamExt           aclParam;
    fm_aclValue              aclValue;
    fm_aclActionExt          aclAction;
    fm_aclCondition          aclCondition = 0;
    fm_tunnelCondition       tunnelCondition;
    fm_tunnelConditionParam  tunnelConditionParam;
    fm_tunnelAction          tunnelAction;
    fm_tunnelActionParam     tunnelActionParam;
    fm_tunnelEncapFlow       tunnelEncapFlow;
    fm_tunnelEncapFlowParam  tunnelEncapFlowParam;
    fm_int                   tunnelRule = -1;
    fm_char                  statusText[1024];
    fm_flowTableType         tableType;
    fm_int                   ortoActions;
    fm_uint32                glort;
    fm_int                   slicesCount;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, priority = %d, precedence = %d, "
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

    FM_NOT_USED(precedence);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    tableType = switchExt->flowInfo.table[tableIndex].type;

    if (tableType == FM_FLOW_TCAM_TABLE)
    {
        err = fmConvertFlowToACLValue(condVal, &aclValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToACLCondition(sw, &condition, &aclCondition);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if ((action & ~(FM_FLOW_ACTION_DEFAULT |
                        FM_FLOW_ACTION_FORWARD |
                        FM_FLOW_ACTION_FORWARD_NORMAL |
                        FM_FLOW_ACTION_TRAP |
                        FM_FLOW_ACTION_DROP |
                        FM_FLOW_ACTION_COUNT |
                        FM_FLOW_ACTION_REDIRECT_TUNNEL |
                        FM_FLOW_ACTION_BALANCE |
                        FM_FLOW_ACTION_ROUTE |
                        FM_FLOW_ACTION_PERMIT |
                        FM_FLOW_ACTION_DENY |
                        FM_FLOW_ACTION_SET_VLAN |
                        FM_FLOW_ACTION_PUSH_VLAN |
                        FM_FLOW_ACTION_POP_VLAN |
                        FM_FLOW_ACTION_UPD_OR_ADD_VLAN |
                        FM_FLOW_ACTION_SET_VLAN_PRIORITY |
                        FM_FLOW_ACTION_SET_SWITCH_PRIORITY |
                        FM_FLOW_ACTION_SET_DSCP |
                        FM_FLOW_ACTION_LOAD_BALANCE |
                        FM_FLOW_ACTION_SET_FLOOD_DEST |
                        FM_FLOW_ACTION_MIRROR_GRP)) != 0)
        {
            /* Only the above listed actions are supported */
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PERMIT) |
                          (action & FM_FLOW_ACTION_DENY),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_FORWARD) |
                          (action & FM_FLOW_ACTION_FORWARD_NORMAL) |
                          (action & FM_FLOW_ACTION_DROP) |
                          (action & FM_FLOW_ACTION_TRAP) |
                          (action & FM_FLOW_ACTION_DEFAULT) |
                          (action & FM_FLOW_ACTION_REDIRECT_TUNNEL) |
                          (action & FM_FLOW_ACTION_BALANCE) |
                          (action & FM_FLOW_ACTION_ROUTE) |
                          (action & FM_FLOW_ACTION_LOAD_BALANCE) |
                          (action & FM_FLOW_ACTION_SET_FLOOD_DEST),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PUSH_VLAN) |
                          (action & FM_FLOW_ACTION_POP_VLAN),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PUSH_VLAN) |
                          (action & FM_FLOW_ACTION_SET_VLAN),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_POP_VLAN) &&
            ((action & FM_FLOW_ACTION_SET_VLAN) == 0) )
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_COUNT) &&
                (switchExt->flowInfo.table[tableIndex].countSupported == FM_DISABLED) )
        {
            /* Action count is not supported for this table. */
            err = FM_ERR_INVALID_ACL_RULE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = VerifyFlowConditionsMasks(sw, tableIndex, condition, condVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if ( (action & FM_FLOW_ACTION_DEFAULT) == 0 )
        {
            /* Regular flow */
            /* Find first free flow index at the beginning of the table */
            err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                                      0,
                                      FALSE,
                                      &flowIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }
        else
        {
            /* The default flow is added at the end of the table */
            err = fmFindLastBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                                          switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1,
                                          FALSE,
                                          &flowIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if (flowIndex < 0)
        {
            FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_ERR_TABLE_FULL);
        }

        err = fmConvertFlowToACLParam(param, &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Translate tableIndex. */
        if (action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
        {
            if ( (param->tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (param->tableIndex < 0) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
            aclParam.tunnelGroup = switchExt->flowInfo.table[param->tableIndex].group;
        }

        /* Translate balanceGroup or ecmpGroup to groupId. */
        if (action & FM_FLOW_ACTION_BALANCE)
        {
            aclParam.groupId = param->balanceGroup;
        }
        else if (action & FM_FLOW_ACTION_ROUTE)
        {
            aclParam.groupId = param->ecmpGroup;
        }

        /* Translate conditions */
        if (aclCondition & FM_ACL_MATCH_L4_SRC_PORT)
        {
            err = TranslateL4Port(sw, TRUE, &aclCondition);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if (aclCondition & FM_ACL_MATCH_L4_DST_PORT)
        {
            err = TranslateL4Port(sw, FALSE, &aclCondition);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        /* Translate mapped conditions */
        if (aclCondition & FM_ACL_MATCH_INGRESS_PORT_SET)
        {
            err = TranslatePortSet(sw, &aclCondition, &aclValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if (aclCondition & FM_ACL_MATCH_SRC_GLORT)
        {
            err = fmGetLogicalPortGlort(sw, condVal->logicalPort, &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            aclValue.srcGlort = glort;
            aclValue.srcGlortMask = 0xffff;

            if (fmIsLagPort(sw, condVal->logicalPort) && ((glort & 0x1f) == 0))
            {
                aclValue.srcGlortMask &= ~0x1f;
            }
        }

        /* Translate actions. */
        err = TranslateFlowToACLAction(sw, &action, &aclAction, &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = CountActionSlicesNeeded(sw, aclAction, aclParam, &slicesCount);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (slicesCount > switchExt->flowInfo.table[tableIndex].preallocatedSlices)
        {
            err = FM_ERR_INVALID_ACL_PARAM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        acl = FM10000_FLOW_BASE_ACL + tableIndex;

        if (switchExt->flowInfo.table[tableIndex].withPriority)
        {
            mappedFlow = (((~priority) & 0xffff) << 14) | ((~flowIndex) & 0x3fff);

            switchExt->flowInfo.table[tableIndex].mapping[flowIndex] = mappedFlow;

            err = fmAddACLRuleExt(sw,
                                  acl,
                                  mappedFlow,
                                  aclCondition,
                                  &aclValue,
                                  aclAction,
                                  &aclParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            /* Apply Without compile */
            err = fmApplyACLExt(sw,
                                FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                                FM_ACL_APPLY_FLAG_INTERNAL,
                                (void*) &acl);
            if ( err != FM_OK )
            {
                FM_LOG_WARNING(FM_LOG_CAT_FLOW,
                               "Non disruptive ACL apply failed with error=%d. "
                               "Performing disruptive compile and apply.\n",
                               err);

                err = fmCompileACL(sw,
                                   statusText,
                                   sizeof(statusText),
                                   0);
                if ( err != FM_OK )
                {

                    err = fmDeleteACLRule(sw,
                                          acl,
                                          mappedFlow);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = fmCompileACL(sw,
                                       statusText,
                                       sizeof(statusText),
                                       0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = fmApplyACL(sw, 0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }

                err = fmApplyACL(sw, 0);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }

            if (flowState == FM_FLOW_STATE_STANDBY)
            {
                err = fmSetACLRuleState(sw,
                                        acl,
                                        mappedFlow,
                                        FM_ACL_RULE_ENTRY_STATE_INVALID);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
        }
        else
        {
            err = fmUpdateACLRule(sw,
                                  acl,
                                  flowIndex,
                                  aclCondition,
                                  &aclValue,
                                  aclAction,
                                  &aclParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            switchExt->flowInfo.table[tableIndex].mapping[flowIndex] = flowIndex;

            ruleState = (flowState == FM_FLOW_STATE_ENABLED) ?
                FM_ACL_RULE_ENTRY_STATE_VALID : FM_ACL_RULE_ENTRY_STATE_INVALID;

            err = fmSetACLRuleState(sw, acl, flowIndex, ruleState);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               flowIndex,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Return the rule number associated to that flow */
        *flowId = flowIndex;

        /* Proper return */
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);
    }
    else if (tableType == FM_FLOW_TE_TABLE)
    {
        if ((action & ~(FM_FLOW_ACTION_SET_DIP |
                        FM_FLOW_ACTION_SET_SIP |
                        FM_FLOW_ACTION_SET_L4DST |
                        FM_FLOW_ACTION_SET_L4SRC |
                        FM_FLOW_ACTION_SET_TTL |
                        FM_FLOW_ACTION_SET_DMAC |
                        FM_FLOW_ACTION_SET_SMAC |
                        FM_FLOW_ACTION_ENCAP_VNI |
                        FM_FLOW_ACTION_ENCAP_SIP |
                        FM_FLOW_ACTION_ENCAP_TTL |
                        FM_FLOW_ACTION_ENCAP_L4DST |
                        FM_FLOW_ACTION_ENCAP_L4SRC |
                        FM_FLOW_ACTION_ENCAP_NGE |
                        FM_FLOW_ACTION_COUNT |
                        FM_FLOW_ACTION_DECAP_KEEP |
                        FM_FLOW_ACTION_DECAP_MOVE |
                        FM_FLOW_ACTION_FORWARD )) != 0)
        {
            /* Only the above listed actions are supported */
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( switchExt->flowInfo.table[tableIndex].condition != condition )
        {
            /* The rule condition must match table condition. */
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( flowState != FM_FLOW_STATE_ENABLED )
        {
            /* FM_FLOW_STATE_STANDBY is not supported. */
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_COUNT) &&
                (switchExt->flowInfo.table[tableIndex].countSupported == FM_DISABLED) )
        {
            /* Action count is not supported for this table. */
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( ( (action & FM_FLOW_ACTION_ENCAP_VNI) == 0) &&
             (param->tunnelType == FM_TUNNEL_TYPE_GPE_NSH) )
        {
            /* VNI must be provided for NSH as it cannot be defaulted. */
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = fmConvertFlowToTEParams(param,
                                      &tunnelActionParam,
                                      &tunnelEncapFlowParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertFlowToTEValue(condVal, &tunnelConditionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToTEAction(&action,
                                      &tunnelAction,
                                      &tunnelEncapFlow,
                                      &tunnelEncapFlowParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToTECondition(&condition, &tunnelCondition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (action & FM_FLOW_ACTION_FORWARD)
        {
            if (switchExt->flowInfo.table[tableIndex].encap)
            {
                /* Action only supported on decap group */
                err = FM_ERR_TUNNEL_CONFLICT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
            err = fmGetLogicalPortGlort(sw, param->logicalPort, &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            tunnelAction |= FM_TUNNEL_SET_DGLORT;
            tunnelActionParam.dglort = glort;
        }

        err = fmFindBitInBitArray(&switchExt->flowInfo.table[tableIndex].idInUse,
                                  0,
                                  FALSE,
                                  &tunnelRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               tunnelRule,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if ( tunnelEncapFlow != 0 )
        {
            err = fm10000AddTunnelEncapFlow(sw,
                                       switchExt->flowInfo.table[tableIndex].group,
                                       tunnelRule,
                                       tunnelEncapFlow,
                                       &tunnelEncapFlowParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            tunnelAction |= FM_TUNNEL_ENCAP_FLOW;
            tunnelActionParam.encapFlow = tunnelRule;
            switchExt->flowInfo.table[tableIndex].mapping[tunnelRule] = tunnelRule;
        }

        err = fm10000AddTunnelRule(sw,
                              switchExt->flowInfo.table[tableIndex].group,
                              tunnelRule,
                              tunnelCondition,
                              &tunnelConditionParam,
                              tunnelAction,
                              &tunnelActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Return the rule number associated to that flow */
        *flowId = tunnelRule;

        /* Proper return */
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    if (tableType == FM_FLOW_TCAM_TABLE)
    {
        /* Flow was not added properly, clear any mapper related configuration
        * for this rule. */
        RemoveMappedEntry(sw, aclCondition, &aclValue);
    }
    else if ( (tableType == FM_FLOW_TE_TABLE) &&
              (tunnelRule >= 0) &&
              (switchExt->flowInfo.table[tableIndex].mapping[tunnelRule] != -1) )
    {
        fm10000DeleteTunnelEncapFlow(sw,
                                     switchExt->flowInfo.table[tableIndex].group,
                                     tunnelRule);
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000AddFlow */





/*****************************************************************************/
/** fm10000GetFlow
 * \ingroup intFlow
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
 * \param[out]      condition points to user-allocated storage where this flow
 *                  condition mask should be stored.
 *
 * \param[out]      condVal points to user-allocated storage where this flow
 *                  condition values should be stored.
 *
 * \param[out]      action points to user-allocated storage where this flow
 *                  action mask should be stored.
 *
 * \param[out]      param points to user-allocated storage where this flow
 *                  action values should be stored.
 *
 * \param[out]      priority points to user-allocated storage where this flow
 *                  priority should be stored.
 *
 * \param[out]      precedence points to user-allocated storage where this flow
 *                  precedence should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is not valid.
 * \return          FM_ERR_INVALID_ACL_RULE if action is not valid.
 * \return          FM_ERR_ACL_DISABLED if the ACL subsystem is disabled.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFlow(fm_int             sw,
                         fm_int             tableIndex,
                         fm_int             flowId,
                         fm_flowCondition * condition,
                         fm_flowValue *     condVal,
                         fm_flowAction *    action,
                         fm_flowParam *     param,
                         fm_int *           priority,
                         fm_int *           precedence)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_flowTableInfo *  flowTableInfo;
    fm_flowTableType         tableType;
    fm_int                   acl;
    fm_int                   rule;
    fm_aclCondition          aclCond;
    fm_aclValue              aclValue;
    fm_aclActionExt          aclAction;
    fm_aclParamExt           aclParam;
    fm_int                   tunnelRule = -1;
    fm_tunnelCondition       tunnelCondition;
    fm_tunnelConditionParam  tunnelConditionParam;
    fm_tunnelAction          tunnelAction = 0;
    fm_tunnelActionParam     tunnelActionParam;
    fm_tunnelEncapFlow       tunnelEncapFlow = 0;
    fm_tunnelEncapFlowParam  tunnelEncapFlowParam;
    fm_int                   logicalPort;
    fm_int                   TEIndex;

    FM_NOT_USED(precedence);

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( condVal == NULL || param == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
         (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowTableInfo = &( switchExt->flowInfo.table[tableIndex] );
    tableType     = switchExt->flowInfo.table[tableIndex].type;

    if ( tableType == FM_FLOW_TCAM_TABLE )
    {
        if (switchExt->flowInfo.table[tableIndex].mapping == NULL)
        {
            /* This table is not intialized. */
            err = FM_ERR_INVALID_ACL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_CLEAR(aclValue);
        FM_CLEAR(aclParam);

        acl  = FM10000_FLOW_BASE_ACL + tableIndex;
        rule = flowTableInfo->mapping[flowId];

        err = fmGetACLRule(sw,
                           acl,
                           rule,
                           &aclCond,
                           &aclValue,
                           &aclAction,
                           &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);


        /* Translate conditions */
        if (aclCond & FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK)
        {
            /* L4 Src/Dst ports must be paired with protocols. */
            if ((aclCond & FM_ACL_MATCH_PROTOCOL) == 0)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }

            aclCond &= ~FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK;
            aclCond |= FM_ACL_MATCH_L4_SRC_PORT;
        }

        if (aclCond & FM_ACL_MATCH_L4_DST_PORT_WITH_MASK)
        {
            /* L4 Src/Dst ports must be paired with protocols. */
            if ((aclCond & FM_ACL_MATCH_PROTOCOL) == 0)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }

            aclCond &= ~FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
            aclCond |= FM_ACL_MATCH_L4_DST_PORT;
        }

        err = TranslateACLToFlowCondition(sw, &aclCond, condition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertACLToFlowValue(&aclValue, condVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (*condition & FM_FLOW_MATCH_LOGICAL_PORT)
        {
            err = fmGetGlortLogicalPort(sw, aclValue.srcGlort, &logicalPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        /* Translate actions */
        err = TranslateACLToFlowAction(sw, &aclAction, &aclParam, action);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertACLToFlowParam(&aclParam, param);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (*action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
        {
            for (TEIndex = 0 ; TEIndex < FM_FLOW_MAX_TABLE_TYPE ; TEIndex++)
            {
                if (switchExt->flowInfo.table[TEIndex].group == aclParam.tunnelGroup)
                {
                    break;
                }
            }
            param->tableIndex = TEIndex;
        }

        /* Translate groupId to balanceGroup or ecmpGroup. */
        if (*action & FM_FLOW_ACTION_BALANCE)
        {
            param->balanceGroup = aclParam.groupId;
        }
        else if (*action & FM_FLOW_ACTION_ROUTE)
        {
            param->ecmpGroup = aclParam.groupId;
        }

        /* Get flow priority from rule value */
        if (switchExt->flowInfo.table[tableIndex].withPriority)
        {
            if (priority == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
            *priority = (((~rule) >> 14) & 0xffff);
        }
    }
    else if ( tableType == FM_FLOW_TE_TABLE )
    {
        /* Associate the flowId as the tunnel rule number */
        tunnelRule = flowId;

        FM_CLEAR(tunnelEncapFlowParam);
        FM_CLEAR(tunnelConditionParam);
        FM_CLEAR(tunnelActionParam);

        /* Get the Tunnel Rule associated with this flow */
        err = fm10000GetTunnelRule(sw,
                                   flowTableInfo->group,
                                   tunnelRule,
                                   &tunnelCondition,
                                   &tunnelConditionParam,
                                   &tunnelAction,
                                   &tunnelActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        tunnelEncapFlow = 0;
        if (tunnelAction & FM_TUNNEL_ENCAP_FLOW)
        {
            /* Get the encapFlow action bit mask */
            err = fm10000GetTunnelEncapFlow(sw,
                                            flowTableInfo->group,
                                            tunnelRule,
                                            &tunnelEncapFlow,
                                            &tunnelEncapFlowParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = fmConvertTEToFlowValue(&tunnelConditionParam,
                                     condVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateTEToFlowCondition(&tunnelCondition, condition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertTEParamsToFlow(&tunnelActionParam,
                                      &tunnelEncapFlowParam,
                                      param);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateTEToFlowAction(&tunnelAction,
                                      &tunnelEncapFlow,
                                      action);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (tunnelAction & FM_TUNNEL_SET_DGLORT)
        {
            err = fmGetGlortLogicalPort(sw,
                                        tunnelActionParam.dglort,
                                        &logicalPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            *action |= FM_FLOW_ACTION_FORWARD;
            param->logicalPort = logicalPort;
        }

    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}




/*****************************************************************************/
/** fm10000GetFlowTableType
 * \ingroup intFlow
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
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFlowTableType(fm_int             sw,
                                  fm_int             tableIndex,
                                  fm_flowTableType * flowTableType)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    *flowTableType = switchExt->flowInfo.table[tableIndex].type;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
}




/*****************************************************************************/
/** fm10000GetFlowFirst
 * \ingroup intFlow
 *
 * \desc            Gets the first created flow table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstTable points to user-allocated memory where
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
fm_status fm10000GetFlowFirst(fm_int   sw,
                              fm_int * firstTable)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_flowInfo *  flowInfo;
    fm_int              group;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d\n",
                 sw);

    switchExt = GET_SWITCH_EXT(sw);

    if ( firstTable == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowInfo = &switchExt->flowInfo;

    for ( group = 0; group < FM_FLOW_MAX_TABLE_TYPE; group++ )
    {
        if ( flowInfo->table[group].created )
        {
            break;
        }
    }

    if ( group == FM_FLOW_MAX_TABLE_TYPE )
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        *firstTable = group;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
}



/*****************************************************************************/
/** fm10000GetFlowNext
 * \ingroup intFlow
 *
 * \desc            Gets the next created flow table after finding the first
 *                  one with a call to ''fm10000GetFlowFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentTable is the last table found by a previous call
 *                  made to ''fm10000GetFlowFirst''.
 *
 * \param[out]      nextTable points to user-allocated memory where
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
fm_status fm10000GetFlowNext(fm_int   sw,
                             fm_int   currentTable,
                             fm_int * nextTable)
{
    fm_status           err = FM_OK;
    fm10000_switch *    switchExt;
    fm10000_flowInfo *  flowInfo;
    fm_int              group;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, currentTable = %d\n",
                 sw,
                 currentTable);

    switchExt = GET_SWITCH_EXT(sw);

    if ( nextTable == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( currentTable < 0 || currentTable >= FM_FLOW_MAX_TABLE_TYPE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowInfo = &switchExt->flowInfo;

    for ( group = currentTable + 1; group < FM_FLOW_MAX_TABLE_TYPE; group++ )
    {
        if ( flowInfo->table[group].created )
        {
            break;
        }
    }

    if ( group == FM_FLOW_MAX_TABLE_TYPE )
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        *nextTable = group;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
}





/*****************************************************************************/
/** fm10000GetFlowRuleFirst
 * \ingroup intFlow
 *
 * \desc            Gets the first created flow from a defined flow table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the flow table on which to operate.
 *
 * \param[out]      firstRule points to user-allocated memory where
 *                  the firts in use flowId should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFlowRuleFirst(fm_int   sw,
                                  fm_int   tableIndex,
                                  fm_int * firstRule)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_flowTableInfo *  flowInfoTable;
    fm_bool                  isInUse = 0;
    fm_int                   bitArraySize;
    fm_int                   rule;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d\n",
                 sw,
                 tableIndex);

    switchExt = GET_SWITCH_EXT(sw);

    if ( firstRule == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( tableIndex < 0 || tableIndex >= FM_FLOW_MAX_TABLE_TYPE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowInfoTable = &switchExt->flowInfo.table[tableIndex];
    bitArraySize  = flowInfoTable->idInUse.bitCount;

    for ( rule = 0; rule < bitArraySize; rule++ )
    {
        err = fmGetBitArrayBit(&flowInfoTable->idInUse,
                               rule,
                               &isInUse);

        if ( isInUse )
        {
            break;
        }
    }

    if ( rule == bitArraySize )
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        *firstRule = rule;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
}




/*****************************************************************************/
/** fm10000GetFlowRuleNext
 * \ingroup intFlow
 *
 * \desc            Gets the next created flow from a defined flow table
 *                  after finding the first one with a call to ''fm10000GetFlowRuleFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the flow table on which to operate.
 *
 * \param[in]       currentRule is the last flow found by a previous call
 *                  to ''fm10000GetFlowRuleFirst''
 *
 * \param[out]      nextRule points to user-allocated memory where
 *                  the rule instance should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFlowRuleNext(fm_int   sw,
                                 fm_int   tableIndex,
                                 fm_int   currentRule,
                                 fm_int * nextRule)
{
    fm_status                err = FM_OK;
    fm10000_switch *         switchExt;
    fm10000_flowTableInfo *  flowInfoTable;
    fm_bool                  isInUse = 0;
    fm_int                   bitArraySize;
    fm_int                   rule;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, currentTable = %d, currentRule = %d\n",
                 sw,
                 tableIndex,
                 currentRule);

    switchExt = GET_SWITCH_EXT(sw);

    if ( nextRule == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( tableIndex < 0 || tableIndex >= FM_FLOW_MAX_TABLE_TYPE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowInfoTable = &switchExt->flowInfo.table[tableIndex];
    bitArraySize  = flowInfoTable->idInUse.bitCount;

    if ( currentRule < 0 || currentRule >= bitArraySize )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    for ( rule = currentRule + 1; rule < bitArraySize; rule++ )
    {
        err = fmGetBitArrayBit(&flowInfoTable->idInUse,
                               rule,
                               &isInUse);

        if ( isInUse )
        {
            break;
        }
    }

    if ( rule == bitArraySize)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        *nextRule = rule;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
}





/*****************************************************************************/
/** fm10000ModifyFlow
 * \ingroup intFlow
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
 *
 * \param[in]       precedence is the inter-table flow precedence.
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
 * \return          FM_ERR_UNSUPPORTED if tableIndex is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if flowId is invalid.
 * \return          FM_ERR_INVALID_ACL if tableIndex is invalid.
 * \return          FM_ERR_INVALID_ACL_RULE if action is not valid.
 * \return          FM_ERR_INVALID_ACL_PARAM if param is not valid.
 * \return          FM_ERR_TUNNEL_CONFLICT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000ModifyFlow(fm_int           sw,
                            fm_int           tableIndex,
                            fm_int           flowId,
                            fm_uint16        priority,
                            fm_uint32        precedence,
                            fm_flowCondition condition,
                            fm_flowValue    *condVal,
                            fm_flowAction    action,
                            fm_flowParam    *param)
{
    fm_status                err = FM_OK;
    fm_int                   acl;
    fm_bool                  bitValue;
    fm10000_switch          *switchExt;
    fm_aclCondition          aclCond;
    fm_aclValue              aclValue;
    fm_aclValue              aclVal;
    fm_aclActionExt          aclAction;
    fm_aclParamExt           aclParam;
    fm_int                   mappedFlow;
    fm_aclCondition          aclCondition = 0;
    fm_tunnelCondition       tunnelCondition;
    fm_tunnelConditionParam  tunnelConditionParam;
    fm_tunnelAction          tunnelAction;
    fm_tunnelActionParam     tunnelActionParam;
    fm_tunnelEncapFlow       tunnelEncapFlow;
    fm_tunnelEncapFlowParam  tunnelEncapFlowParam;
    fm_flowTableType         tableType;
    fm_int                   ortoActions;
    fm_uint32                glort;
    fm_int                   slicesCount;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d, priority = %d, "
                 "precedence = %d, action = %llx, param = %p\n",
                 sw,
                 tableIndex,
                 flowId,
                 priority,
                 precedence,
                 action,
                 (void *) param);

    FM_NOT_USED(priority);
    FM_NOT_USED(precedence);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
        (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate the flow ID */
    err = fmGetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                           flowId,
                           &bitValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (!bitValue)
    {
        /* Invalid flow ID */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    tableType = switchExt->flowInfo.table[tableIndex].type;

    if (tableType == FM_FLOW_TCAM_TABLE)
    {
        err = fmConvertFlowToACLValue(condVal, &aclValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToACLCondition(sw, &condition, &aclCondition);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if ((action & ~(FM_FLOW_ACTION_DEFAULT |
                        FM_FLOW_ACTION_FORWARD |
                        FM_FLOW_ACTION_FORWARD_NORMAL |
                        FM_FLOW_ACTION_TRAP |
                        FM_FLOW_ACTION_DROP |
                        FM_FLOW_ACTION_COUNT |
                        FM_FLOW_ACTION_REDIRECT_TUNNEL |
                        FM_FLOW_ACTION_BALANCE |
                        FM_FLOW_ACTION_ROUTE |
                        FM_FLOW_ACTION_PERMIT |
                        FM_FLOW_ACTION_DENY |
                        FM_FLOW_ACTION_SET_VLAN |
                        FM_FLOW_ACTION_PUSH_VLAN |
                        FM_FLOW_ACTION_POP_VLAN |
                        FM_FLOW_ACTION_SET_VLAN_PRIORITY |
                        FM_FLOW_ACTION_SET_SWITCH_PRIORITY |
                        FM_FLOW_ACTION_SET_DSCP |
                        FM_FLOW_ACTION_LOAD_BALANCE |
                        FM_FLOW_ACTION_SET_FLOOD_DEST)) != 0)
        {
            /* Only the above listed actions are supported */
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PERMIT) |
                          (action & FM_FLOW_ACTION_DENY),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_FORWARD) |
                          (action & FM_FLOW_ACTION_FORWARD_NORMAL) |
                          (action & FM_FLOW_ACTION_DROP) |
                          (action & FM_FLOW_ACTION_TRAP) |
                          (action & FM_FLOW_ACTION_DEFAULT) |
                          (action & FM_FLOW_ACTION_REDIRECT_TUNNEL) |
                          (action & FM_FLOW_ACTION_BALANCE) |
                          (action & FM_FLOW_ACTION_ROUTE) |
                          (action & FM_FLOW_ACTION_LOAD_BALANCE) |
                          (action & FM_FLOW_ACTION_SET_FLOOD_DEST),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PUSH_VLAN) |
                          (action & FM_FLOW_ACTION_POP_VLAN),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        FM_COUNT_SET_BITS((action & FM_FLOW_ACTION_PUSH_VLAN) |
                          (action & FM_FLOW_ACTION_SET_VLAN),
                          ortoActions);
        if (ortoActions > 1)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_POP_VLAN) &&
            ((action & FM_FLOW_ACTION_SET_VLAN) == 0) )
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_COUNT) &&
                (switchExt->flowInfo.table[tableIndex].countSupported == FM_DISABLED) )
        {
            /* Action count is not supported for this table. */
            err = FM_ERR_INVALID_ACL_RULE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = VerifyFlowConditionsMasks(sw, tableIndex, condition, condVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        acl = FM10000_FLOW_BASE_ACL + tableIndex;
        mappedFlow = switchExt->flowInfo.table[tableIndex].mapping[flowId];

        /* Adjust the mapped value */
        err = fmGetACLRule(sw,
                           acl,
                           mappedFlow,
                           &aclCond,
                           &aclVal,
                           &aclAction,
                           &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Remove any mapper usage for the rule that must be modified. */
        err = RemoveMappedEntry(sw, aclCond, &aclVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertFlowToACLParam(param, &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Translate tableIndex. */
        if (action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
        {
            if ( (param->tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (param->tableIndex < 0) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
            aclParam.tunnelGroup = switchExt->flowInfo.table[param->tableIndex].group;
        }

        /* Translate balanceGroup or ecmpGroup to groupId. */
        if (action & FM_FLOW_ACTION_BALANCE)
        {
            aclParam.groupId = param->balanceGroup;
        }
        else if (action & FM_FLOW_ACTION_ROUTE)
        {
            aclParam.groupId = param->ecmpGroup;
        }

        /* Translate conditions */
        if (aclCondition & FM_ACL_MATCH_L4_SRC_PORT)
        {
            err = TranslateL4Port(sw, TRUE, &aclCondition);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if (aclCondition & FM_ACL_MATCH_L4_DST_PORT)
        {
            err = TranslateL4Port(sw, FALSE, &aclCondition);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        /* Translate mapped conditions */
        if (aclCondition & FM_ACL_MATCH_INGRESS_PORT_SET)
        {
            err = TranslatePortSet(sw, &aclCondition, &aclValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if (aclCondition & FM_ACL_MATCH_SRC_GLORT)
        {
            err = fmGetLogicalPortGlort(sw, condVal->logicalPort, &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            aclValue.srcGlort = glort;
            aclValue.srcGlortMask = 0xffff;

            if (fmIsLagPort(sw, condVal->logicalPort) && ((glort & 0x1f) == 0))
            {
                aclValue.srcGlortMask &= ~0x1f;
            }
        }

        /* Translate actions. */
        err = TranslateFlowToACLAction(sw, &action, &aclAction, &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = CountActionSlicesNeeded(sw, aclAction, aclParam, &slicesCount);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (slicesCount > switchExt->flowInfo.table[tableIndex].preallocatedSlices)
        {
            err = FM_ERR_INVALID_ACL_PARAM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = fmUpdateACLRule(sw,
                              acl,
                              mappedFlow,
                              aclCondition,
                              &aclValue,
                              aclAction,
                              &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Normal exit */
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);
    }
    else if (tableType == FM_FLOW_TE_TABLE)
    {
        if ((action & ~(FM_FLOW_ACTION_SET_DIP |
                        FM_FLOW_ACTION_SET_SIP |
                        FM_FLOW_ACTION_SET_L4DST |
                        FM_FLOW_ACTION_SET_L4SRC |
                        FM_FLOW_ACTION_SET_TTL |
                        FM_FLOW_ACTION_SET_DMAC |
                        FM_FLOW_ACTION_SET_SMAC |
                        FM_FLOW_ACTION_ENCAP_VNI |
                        FM_FLOW_ACTION_ENCAP_SIP |
                        FM_FLOW_ACTION_ENCAP_TTL |
                        FM_FLOW_ACTION_ENCAP_L4DST |
                        FM_FLOW_ACTION_ENCAP_L4SRC |
                        FM_FLOW_ACTION_ENCAP_NGE |
                        FM_FLOW_ACTION_COUNT |
                        FM_FLOW_ACTION_DECAP_KEEP |
                        FM_FLOW_ACTION_DECAP_MOVE |
                        FM_FLOW_ACTION_FORWARD )) != 0)
        {
            /* Only the above listed actions are supported */
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( switchExt->flowInfo.table[tableIndex].condition != condition )
        {
            /* The rule condition must match table condition. */
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( (action & FM_FLOW_ACTION_COUNT) &&
                (switchExt->flowInfo.table[tableIndex].countSupported == FM_DISABLED) )
        {
            /* Action count is not supported for this table. */
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        if ( ( (action & FM_FLOW_ACTION_ENCAP_VNI) == 0) &&
             (param->tunnelType == FM_TUNNEL_TYPE_GPE_NSH) )
        {
            /* VNI must be provided for NSH as it cannot be defaulted. */
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        err = fmConvertFlowToTEParams(param,
                                      &tunnelActionParam,
                                      &tunnelEncapFlowParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = fmConvertFlowToTEValue(condVal, &tunnelConditionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToTEAction(&action,
                                      &tunnelAction,
                                      &tunnelEncapFlow,
                                      &tunnelEncapFlowParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = TranslateFlowToTECondition(&condition, &tunnelCondition);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (action & FM_FLOW_ACTION_FORWARD)
        {
            if (switchExt->flowInfo.table[tableIndex].encap)
            {
                /* Action only supported on decap group */
                err = FM_ERR_TUNNEL_CONFLICT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
            err = fmGetLogicalPortGlort(sw, param->logicalPort, &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            tunnelAction |= FM_TUNNEL_SET_DGLORT;
            tunnelActionParam.dglort = glort;
        }

        /* Tunnel Encap Flow Update */
        if ( ( tunnelEncapFlow != 0 ) &&
                ( switchExt->flowInfo.table[tableIndex].mapping[flowId] != -1 ) )
        {
            err = fm10000UpdateTunnelEncapFlow(sw,
                                          switchExt->flowInfo.table[tableIndex].group,
                                          flowId,
                                          tunnelEncapFlow,
                                          &tunnelEncapFlowParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            tunnelAction |= FM_TUNNEL_ENCAP_FLOW;
            tunnelActionParam.encapFlow = flowId;
        }

        /* Tunnel Encap Flow Creation */
        if ( ( tunnelEncapFlow != 0 ) &&
                ( switchExt->flowInfo.table[tableIndex].mapping[flowId] == -1 ) )
        {
            err = fm10000AddTunnelEncapFlow(sw,
                                       switchExt->flowInfo.table[tableIndex].group,
                                       flowId,
                                       tunnelEncapFlow,
                                       &tunnelEncapFlowParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            tunnelAction |= FM_TUNNEL_ENCAP_FLOW;
            tunnelActionParam.encapFlow = flowId;
            switchExt->flowInfo.table[tableIndex].mapping[flowId] = flowId;
        }

        /* Tunnel Rule Update */
        err = fm10000UpdateTunnelRule(sw,
                                 switchExt->flowInfo.table[tableIndex].group,
                                 flowId,
                                 tunnelCondition,
                                 &tunnelConditionParam,
                                 tunnelAction,
                                 &tunnelActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Tunnel Encap Flow Deletion */
        if ( ( tunnelEncapFlow == 0 ) &&
                ( switchExt->flowInfo.table[tableIndex].mapping[flowId] != -1 ) )
        {
            err =  fm10000DeleteTunnelEncapFlow(sw,
                                           switchExt->flowInfo.table[tableIndex].group,
                                           switchExt->flowInfo.table[tableIndex].mapping[flowId]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            switchExt->flowInfo.table[tableIndex].mapping[flowId] = -1;
        }

        /* Proper return */
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, FM_OK);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    if (tableType == FM_FLOW_TCAM_TABLE)
    {
        /* Update fail, clear any mapper related configuration for this rule. */
        RemoveMappedEntry(sw, aclCondition, &aclValue);
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000ModifyFlow */




/*****************************************************************************/
/** fm10000DeleteFlow
 * \ingroup intFlow
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
 *                  produce a valid ACL "binary image" from the current ACL
 *                  configuration.
 * \return          FM_ERR_FFU_RESOURCE_IN_USE if this flowId is currently
 *                  referenced by another module.
 *
 *****************************************************************************/
fm_status fm10000DeleteFlow(fm_int sw, fm_int tableIndex, fm_int flowId)
{
    fm_status        err = FM_OK;
    fm_int           acl;
    fm_bool          bitValue;
    fm_switch *      switchPtr;
    fm10000_switch * switchExt;
    fm_flowCondition condition;
    fm_flowValue     condMasks;
    fm_aclCondition  aclCond;
    fm_aclValue      aclValue;
    fm_aclActionExt  aclAction;
    fm_aclParamExt   aclParam;
    fm_int           mappedFlow;
    fm_char          statusText[1024];

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 tableIndex,
                 flowId);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
        (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate the flow ID */
    err = fmGetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                           flowId,
                           &bitValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (!bitValue)
    {
        /* Invalid flow ID */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].useCnt[flowId])
    {
        err = FM_ERR_FFU_RESOURCE_IN_USE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TCAM_TABLE)
    {
        acl = FM10000_FLOW_BASE_ACL + tableIndex;
        mappedFlow = switchExt->flowInfo.table[tableIndex].mapping[flowId];

        err = fmSetACLRuleState(sw,
                                acl,
                                mappedFlow,
                                FM_ACL_RULE_ENTRY_STATE_INVALID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        /* Adjust the mapped value */
        err = fmGetACLRule(sw,
                           acl,
                           mappedFlow,
                           &aclCond,
                           &aclValue,
                           &aclAction,
                           &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        err = RemoveMappedEntry(sw, aclCond, &aclValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (switchExt->flowInfo.table[tableIndex].withPriority)
        {
            err = fmDeleteACLRule(sw,
                                  acl,
                                  mappedFlow);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            /* Apply Without compile */
            err = fmApplyACLExt(sw,
                                FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                                FM_ACL_APPLY_FLAG_INTERNAL,
                                (void*) &acl);
            if ( err != FM_OK )
            {
                FM_LOG_WARNING(FM_LOG_CAT_FLOW,
                               "Non disruptive ACL apply failed with error=%d. "
                               "Performing disruptive compile and apply.\n",
                               err);

                err = fmCompileACL(sw,
                                   statusText,
                                   sizeof(statusText),
                                   0);
                if ( err != FM_OK )
                {
                    err = fmAddACLRuleExt(sw,
                                          acl,
                                          mappedFlow,
                                          aclCond,
                                          &aclValue,
                                          aclAction,
                                          &aclParam);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = fmCompileACL(sw,
                                       statusText,
                                       sizeof(statusText),
                                       0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = fmApplyACL(sw, 0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

                    err = FM_ERR_ACL_COMPILE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }

                err = fmApplyACL(sw, 0);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }
        }
        else
        {
            /* Reset ACL rule */

            condition = switchExt->flowInfo.table[tableIndex].condition;
            condMasks = switchExt->flowInfo.table[tableIndex].condMasks;

            err = TranslateFlowToACLCondition(sw, &condition, &aclCond);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            err = TranslateConditionMask(sw, &aclCond);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            err = fmConvertFlowToACLValue(&condMasks, &aclValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            aclValue.dstIpMask.isIPv6 = FALSE;
            aclValue.frameType = FM_ACL_FRAME_TYPE_IPV4;
            aclValue.srcGlortMask = 0xffff;

            if ( (switchExt->flowInfo.table[tableIndex].countSupported == FM_ENABLED) &&
                 (aclAction & FM_ACL_ACTIONEXT_COUNT) )
            {
                /* Set action count only if the original rule already used it
                   to avoid time consuming full ACL recompile/apply sequence */
                aclAction = FM_ACL_ACTIONEXT_REDIRECT | FM_ACL_ACTIONEXT_COUNT;
            }
            else
            {
                aclAction = FM_ACL_ACTIONEXT_REDIRECT;
            }

            aclParam.logicalPort = switchPtr->cpuPort;

            err = fmUpdateACLRule(sw,
                                  acl,
                                  mappedFlow,
                                  aclCond,
                                  &aclValue,
                                  aclAction,
                                  &aclParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        /* Clear flow ID bit */
        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               flowId,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TE_TABLE)
    {
        err = fm10000DeleteTunnelRule(sw,
                                 switchExt->flowInfo.table[tableIndex].group,
                                 flowId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        if (switchExt->flowInfo.table[tableIndex].mapping[flowId] != -1)
        {
            err =  fm10000DeleteTunnelEncapFlow(sw,
                                           switchExt->flowInfo.table[tableIndex].group,
                                           switchExt->flowInfo.table[tableIndex].mapping[flowId]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            switchExt->flowInfo.table[tableIndex].mapping[flowId] = -1;
        }

        err = fmSetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                               flowId,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DeleteFlow */




/*****************************************************************************/
/** fm10000SetFlowState
 * \ingroup intFlow
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
fm_status fm10000SetFlowState(fm_int       sw,
                              fm_int       tableIndex,
                              fm_int       flowId,
                              fm_flowState flowState)
{
    fm_status        err = FM_OK;
    fm_int           acl;
    fm_int           rule;
    fm_aclEntryState ruleState;
    fm10000_switch  *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d, flowState = %d\n",
                 sw,
                 tableIndex,
                 flowId,
                 flowState);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].type != FM_FLOW_TCAM_TABLE)
    {
        /* Only TCAM tables are supported. */
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
         (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    ruleState = (flowState == FM_FLOW_STATE_ENABLED) ?
                FM_ACL_RULE_ENTRY_STATE_VALID : FM_ACL_RULE_ENTRY_STATE_INVALID;

    acl = FM10000_FLOW_BASE_ACL + tableIndex;
    rule = switchExt->flowInfo.table[tableIndex].mapping[flowId];
    err = fmSetACLRuleState(sw, acl, rule, ruleState);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000SetFlowState */




/*****************************************************************************/
/** fm10000GetFlowCount
 * \ingroup intFlow
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
fm_status fm10000GetFlowCount(fm_int           sw,
                              fm_int           tableIndex,
                              fm_int           flowId,
                              fm_flowCounters *counters)
{
    fm_status         err = FM_OK;
    fm_int            acl;
    fm_int            rule;
    fm10000_switch   *switchExt;
    fm_aclCounters    aclCounters;
    fm_tunnelCounters tunnelCounters;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d, counters = %p\n",
                 sw,
                 tableIndex,
                 flowId,
                 (void *) counters);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
         (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TCAM_TABLE)
    {
        if (switchExt->flowInfo.table[tableIndex].mapping == NULL)
        {
            /* This table is not intialized. */
            err = FM_ERR_INVALID_ACL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
        }

        acl = FM10000_FLOW_BASE_ACL + tableIndex;
        rule = switchExt->flowInfo.table[tableIndex].mapping[flowId];
        err = fmGetACLCountExt(sw, acl, rule, &aclCounters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        fmConvertACLToFlowCounters(&aclCounters, counters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TE_TABLE)
    {
        err = fm10000GetTunnelRuleCount(sw,
                                   switchExt->flowInfo.table[tableIndex].group,
                                   flowId,
                                   &tunnelCounters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        fmConvertTEToFlowCounters(&tunnelCounters, counters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000GetFlowCount */




/*****************************************************************************/
/** fm10000ResetFlowCount
 * \ingroup intFlow
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
fm_status fm10000ResetFlowCount(fm_int sw,
                                fm_int tableIndex,
                                fm_int flowId)
{
    fm_status       err = FM_OK;
    fm_int          acl;
    fm_int          rule;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
        (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TCAM_TABLE)
    {
        acl = FM10000_FLOW_BASE_ACL + tableIndex;
        rule = switchExt->flowInfo.table[tableIndex].mapping[flowId];
        err = fmResetACLCount(sw, acl, rule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TE_TABLE)
    {
        err = fm10000ResetTunnelRuleCount(sw,
                                     switchExt->flowInfo.table[tableIndex].group,
                                     flowId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    switchExt->flowInfo.table[tableIndex].lastCnt[flowId].cntPkts = 0;
    switchExt->flowInfo.table[tableIndex].useBit[flowId] = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000ResetFlowCount */




/*****************************************************************************/
/** fm10000GetFlowUsed
 * \ingroup intFlow
 *
 * \desc            Determine if a packet has been received that matches
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
fm_status fm10000GetFlowUsed(fm_int   sw,
                             fm_int   tableIndex,
                             fm_int   flowId,
                             fm_bool  clear,
                             fm_bool *used)
{
    fm_status         err = FM_OK;
    fm_int            acl;
    fm_int            rule;
    fm10000_switch   *switchExt;
    fm_flowCounters   counters;
    fm_aclCounters    aclCounters;
    fm_tunnelCounters tunnelCounters;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d, clear = %d, "
                 "used = %p\n",
                 sw,
                 tableIndex,
                 flowId,
                 clear,
                 (void *) used);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
        (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TCAM_TABLE)
    {
        acl = FM10000_FLOW_BASE_ACL + tableIndex;
        rule = switchExt->flowInfo.table[tableIndex].mapping[flowId];
        err = fmGetACLCountExt(sw, acl, rule, &aclCounters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        fmConvertACLToFlowCounters(&aclCounters, &counters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else if (switchExt->flowInfo.table[tableIndex].type == FM_FLOW_TE_TABLE)
    {
        err = fm10000GetTunnelRuleCount(sw,
                                   switchExt->flowInfo.table[tableIndex].group,
                                   flowId,
                                   &tunnelCounters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

        fmConvertTEToFlowCounters(&tunnelCounters, &counters);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if (counters.cntPkts !=
        switchExt->flowInfo.table[tableIndex].lastCnt[flowId].cntPkts)
    {
        switchExt->flowInfo.table[tableIndex].useBit[flowId] = TRUE;
    }
    switchExt->flowInfo.table[tableIndex].lastCnt[flowId].cntPkts =
        counters.cntPkts;

    *used = switchExt->flowInfo.table[tableIndex].useBit[flowId];

    if (clear)
    {
        switchExt->flowInfo.table[tableIndex].useBit[flowId] = FALSE;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000GetFlowUsed */




/*****************************************************************************/
/** fm10000SetFlowAttribute
 * \ingroup intFlow
 *
 * \desc            Set a Flow API attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the
 *                  attribute belongs.
 *
 * \param[in]       attr is the flow api attribute to set
 *                  (see 'Flow Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported or table
 *                  exists and attribute should be set before creation of table.
 * \return          FM_ERR_INVALID_ARGUMENT if argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFlowAttribute(fm_int sw,
                                  fm_int tableIndex,
                                  fm_int attr,
                                  void  *value)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_tunnelParam  tunnelParam;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, attr = %d, value = %p\n",
                 sw,
                 tableIndex,
                 attr,
                 value);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switch (attr)
    {
        case FM_FLOW_TABLE_WITH_PRIORITY:
            /* Priority configuration must be done prior to the ACL creation. */
            if (switchExt->flowInfo.table[tableIndex].created == FALSE)
            {
                switchExt->flowInfo.table[tableIndex].withPriority =
                    *( (fm_bool *) value );
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_WITH_DEFAULT_ACTION:
            /* Default action configuration must be done prior to the ACL
             * creation. */
            if (switchExt->flowInfo.table[tableIndex].created == FALSE)
            {
                switchExt->flowInfo.table[tableIndex].withDefault =
                    *( (fm_bool *) value );
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_COND_MATCH_MASKS:
            /* Table condition matching masks must be set prior to the ACL
             * creation. */
            if (switchExt->flowInfo.table[tableIndex].created == FALSE)
            {
                if ( ( ( (fm_flowValue *) value )->srcMask |
                            FM_LITERAL_U64(0xffffffffffff) ) !=
                        FM_LITERAL_U64(0xffffffffffff) )
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                if ( ( ( (fm_flowValue *) value )->dstMask |
                            FM_LITERAL_U64(0xffffffffffff) ) !=
                        FM_LITERAL_U64(0xffffffffffff) )
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                if ( ( (fm_flowValue *) value )->vlanPriMask > 0xf)
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                if ( ( (fm_flowValue *) value )->vlanPri2Mask > 0xf)
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                switchExt->flowInfo.table[tableIndex].condMasks =
                    *( (fm_flowValue *) value );
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_WITH_COUNT:
            /* Specifying if a table supports action count must be done prior to
               the ACL creation. */
            if (switchExt->flowInfo.table[tableIndex].created == FALSE)
            {
                switchExt->flowInfo.table[tableIndex].countSupported =
                    *( (fm_bool *) value );
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_TUNNEL_ENGINE:
            /* Tunnel Engine must be set prior to the TE table creation. */
            err = fm10000GetTunnel(sw,
                              switchExt->flowInfo.table[tableIndex].group,
                              &tunnelParam);
            if (err == FM_ERR_TUNNEL_INVALID_ENTRY)
            {
                if ( ( *((fm_int *) value) < 0 ) ||
                     ( *((fm_int *) value) >= FM10000_TE_DGLORT_MAP_ENTRIES_1 ) )
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
                }
                switchExt->flowInfo.table[tableIndex].te =
                    *( (fm_int *) value );
                err = FM_OK;
            }
            else if (err == FM_OK)
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_TUNNEL_ENCAP:
            /* Specifying if a flow table is Encap (TRUE) or Decap (FALSE).
               This attribute must be configured prior to the TE table
               creation. */
            err = fm10000GetTunnel(sw,
                              switchExt->flowInfo.table[tableIndex].group,
                              &tunnelParam);
            if (err == FM_ERR_TUNNEL_INVALID_ENTRY)
            {
                switchExt->flowInfo.table[tableIndex].encap =
                    *( (fm_bool *) value );
                err = FM_OK;
            }
            else if (err == FM_OK)
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_FLOW_TABLE_SCENARIO:
            /* Specifying scenario must be done prior to the ACL creation. */
            if (switchExt->flowInfo.table[tableIndex].created == FALSE)
            {
                switchExt->flowInfo.table[tableIndex].scenario =
                    *( (fm_uint32 *) value );
                err = FM_OK;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000SetFlowAttribute */




/*****************************************************************************/
/** fm10000GetFlowAttribute
 * \ingroup intFlow
 *
 * \desc            Get a Flow API attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is the table instance to which the
 *                  attribute belongs.
 *
 * \param[in]       attr is the flow api attribute to get
 *                  (see 'Flow Attributes').
 *
 * \param[in]       value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFlowAttribute(fm_int sw,
                                  fm_int tableIndex,
                                  fm_int attr,
                                  void  *value)
{
    fm_status        err = FM_OK;
    fm10000_switch * switchExt;
    fm_flowTableType tableType;
    fm_int           bitCount;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, attr = %d, value = %p\n",
                 sw,
                 tableIndex,
                 attr,
                 value);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switch (attr)
    {
        case FM_FLOW_DEF_ACTION_CODE:
            err = fmGetLogicalPortGlort(sw,
                                        switchExt->flowInfo.defaultLogicalPort,
                                        (fm_uint32 *) value);
            break;

        case FM_FLOW_TRAP_ACTION_CODE:
            err = fmGetLogicalPortGlort(sw,
                                        switchExt->flowInfo.trapLogicalPort,
                                        (fm_uint32 *) value);
            break;

        case FM_FLOW_FWD_TO_CPU_ACTION_CODE:
            err = fmGetLogicalPortGlort(sw,
                                        switchExt->flowInfo.fwdToCpuLogicalPort,
                                        (fm_uint32 *) value);
            break;

        case FM_FLOW_TABLE_WITH_PRIORITY:
            *( (fm_bool *) value ) =
                switchExt->flowInfo.table[tableIndex].withPriority;
            break;

        case FM_FLOW_TABLE_WITH_DEFAULT_ACTION:
            *( (fm_bool *) value ) =
                switchExt->flowInfo.table[tableIndex].withDefault;
            break;

        case FM_FLOW_TABLE_COND_MATCH_MASKS:
            *( (fm_flowValue *) value ) =
                switchExt->flowInfo.table[tableIndex].condMasks;
            break;

        case FM_FLOW_TABLE_WITH_COUNT:
            *( (fm_bool *) value ) =
                switchExt->flowInfo.table[tableIndex].countSupported;
            break;

        case FM_FLOW_TABLE_TUNNEL_ENGINE:
            *( (fm_int *) value ) =
                switchExt->flowInfo.table[tableIndex].te;
            break;

        case FM_FLOW_TABLE_TUNNEL_ENCAP:
            *( (fm_bool *) value ) =
                switchExt->flowInfo.table[tableIndex].encap;
            break;

        case FM_FLOW_TABLE_TUNNEL_GROUP:
            *( (fm_int *) value ) =
                switchExt->flowInfo.table[tableIndex].group;
            break;

        case FM_FLOW_TABLE_SCENARIO:
            *( (fm_uint32 *) value ) =
                switchExt->flowInfo.table[tableIndex].scenario;
            break;

        case FM_FLOW_TABLE_CONDITION:
            *( (fm_flowCondition *) value ) =
                switchExt->flowInfo.table[tableIndex].condition;
            break;

        case FM_FLOW_TABLE_MAX_ACTIONS:
            *( (fm_uint32 *) value ) =
                switchExt->flowInfo.table[tableIndex].maxAction;
            break;

        case FM_FLOW_TABLE_MAX_ENTRIES:
            err = fmGetBitArrayBitCount(
                    &switchExt->flowInfo.table[tableIndex].idInUse,
                    (fm_int *) value);
            break;

        case FM_FLOW_TABLE_EMPTY_ENTRIES:
            err = fmGetBitArrayBitCount(
                    &switchExt->flowInfo.table[tableIndex].idInUse,
                    (fm_int *) value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            err = fmGetBitArrayNonZeroBitCount(
                    &switchExt->flowInfo.table[tableIndex].idInUse,
                    &bitCount);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            *( (fm_int *) value ) -= bitCount;
            break;

        case FM_FLOW_TABLE_SUPPORTED_ACTIONS:
            err = fmGetFlowTableType(sw,
                                     tableIndex,
                                     &tableType);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

            switch (tableType)
            {
                case FM_FLOW_TCAM_TABLE:
                    *( (fm_flowAction *) value ) =
                            FM10000_FLOW_SUPPORTED_TCAM_ACTIONS;
                    break;

                case FM_FLOW_BST_TABLE:
                    *( (fm_flowAction *) value ) =
                            FM10000_FLOW_SUPPORTED_BST_ACTIONS;
                    break;

                case FM_FLOW_TE_TABLE:
                    *( (fm_flowAction *) value ) =
                            FM10000_FLOW_SUPPORTED_TE_ACTIONS;
                    break;

                default:
                    err = FM_ERR_UNSUPPORTED;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);
            }

            break;


        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000GetFlowAttribute */




/*****************************************************************************/
/** fm10000CreateFlowBalanceGrp
 * \ingroup intFlow
 *
 * \desc            Create variable sized ECMP balance group for use with the
 *                  ''FM_FLOW_ACTION_BALANCE'' action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      groupId is identifier of created group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 *
 *****************************************************************************/
fm_status fm10000CreateFlowBalanceGrp(fm_int  sw,
                                      fm_int *groupId)
{
    fm_status        err;
    fm10000_switch  *switchExt;
    fm_ecmpGroupInfo info;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(&info, sizeof(fm_ecmpGroupInfo), 0, sizeof(fm_ecmpGroupInfo));

    info.numFixedEntries = 0;

    err = fmCreateECMPGroupV2(sw, groupId, &info);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmSetBitArrayBit(&switchExt->flowInfo.balanceGrpInUse,
                           *groupId,
                           TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FLOW, err);

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

ABORT:

    fmDeleteECMPGroup(sw, *groupId);
    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000CreateFlowBalanceGrp */




/*****************************************************************************/
/** fm10000DeleteFlowBalanceGrp
 * \ingroup intFlow
 *
 * \desc            Delete ECMP balance group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is identifier of group to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the ECMP group is in use.
 *
 *****************************************************************************/
fm_status fm10000DeleteFlowBalanceGrp(fm_int sw,
                                      fm_int groupId)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_bool         bitValue;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW, "sw = %d, groupId = %d\n", sw, groupId);

    switchExt = GET_SWITCH_EXT(sw);

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmGetBitArrayBit(&switchExt->flowInfo.balanceGrpInUse,
                           groupId,
                           &bitValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (bitValue == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmDeleteECMPGroup(sw, groupId);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    err = fmSetBitArrayBit(&switchExt->flowInfo.balanceGrpInUse,
                           groupId,
                           FALSE);
    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DeleteFlowBalanceGrp */




/*****************************************************************************/
/** fm10000AddFlowBalanceGrpEntry
 * \ingroup intFlow
 *
 * \desc            Add entry to ECMP balance group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is identifier of group that entry will be added to.
 *
 * \param[in]       tableIndex is flow TE table index part of the entry.
 *                  Specified flow TE table should be in direct mode and must
 *                  not use the USER field.
 *
 * \param[in]       flowId is flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IS_FULL if the group has reached its
 *                  maximum capacity.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_NO_MEM if the function is unable to allocate
 *                  needed memory.
 * \return          FM_ERR_ALREADY_EXISTS if entry being added already exists
 *                  in the ECMP group.
 *
 *****************************************************************************/
fm_status fm10000AddFlowBalanceGrpEntry(fm_int sw,
                                        fm_int groupId,
                                        fm_int tableIndex,
                                        fm_int flowId)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_ecmpNextHop  nextHop;
    fm_bool         bitValue;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, groupId = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 groupId,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (groupId >= FM_MAX_ARPS) || (groupId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
    }

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId >= FM10000_MAX_RULE_PER_FLOW_TE_TABLE) || (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmGetBitArrayBit(&switchExt->flowInfo.balanceGrpInUse,
                           groupId,
                           &bitValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (bitValue == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate the tableIndex */
    if (switchExt->flowInfo.table[tableIndex].created == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    /* Validate the flowId */
    err = fmGetBitArrayBit(&switchExt->flowInfo.table[tableIndex].idInUse,
                           flowId,
                           &bitValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (bitValue == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(&nextHop, sizeof(fm_ecmpNextHop), 0, sizeof(fm_ecmpNextHop));
    nextHop.type = FM_NEXTHOP_TYPE_TUNNEL;
    nextHop.data.tunnel.tunnelGrp = switchExt->flowInfo.table[tableIndex].group;
    nextHop.data.tunnel.tunnelRule = flowId;

    err = fmAddECMPGroupNextHopsV2(sw, groupId, 1, &nextHop);

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000AddFlowBalanceGrpEntry */




/*****************************************************************************/
/** fm10000DeleteFlowBalanceGrpEntry
 * \ingroup intFlow
 *
 * \desc            Delete entry from ECMP balance group.
 *
 * \note            If an entry cannot be found in the ECMP group's entries
 *                  table, that entry will simply be ignored.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is identifier of group that entry will be deleted
 *                  form.
 *
 * \param[in]       tableIndex is flow TE table index part of the entry.
 *
 * \param[in]       flowId is flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used and the
 *                  last entry in the group is being deleted.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fm10000DeleteFlowBalanceGrpEntry(fm_int sw,
                                           fm_int groupId,
                                           fm_int tableIndex,
                                           fm_int flowId)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_ecmpNextHop  nextHop;
    fm_bool         bitValue;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, groupId = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 groupId,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (groupId >= FM_MAX_ARPS) || (groupId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
    }

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId >= FM10000_MAX_RULE_PER_FLOW_TE_TABLE) || (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);
    }

    /* Initialize the flow Api - Done just once */
    if (switchExt->flowInfo.initialized == FALSE)
    {
        err = InitFlowApi(sw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    err = fmGetBitArrayBit(&switchExt->flowInfo.balanceGrpInUse,
                           groupId,
                           &bitValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    if (bitValue == FALSE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    FM_MEMSET_S(&nextHop, sizeof(fm_ecmpNextHop), 0, sizeof(fm_ecmpNextHop));
    nextHop.type = FM_NEXTHOP_TYPE_TUNNEL;
    nextHop.data.tunnel.tunnelGrp = switchExt->flowInfo.table[tableIndex].group;
    nextHop.data.tunnel.tunnelRule = flowId;

    err = fmDeleteECMPGroupNextHopsV2(sw, groupId, 1, &nextHop);

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DeleteFlowBalanceGrpEntry */




/*****************************************************************************/
/** fm10000FreeFlowResource
 * \ingroup intFlow
 *
 * \desc            Free the allocated flow resources.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeFlowResource(fm_int sw)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_int          i;

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->flowInfo.initialized)
    {
        for (i = 0 ; i < FM_FLOW_MAX_TABLE_TYPE ; i++)
        {
            if (switchExt->flowInfo.table[i].lastCnt)
            {
                fmFree(switchExt->flowInfo.table[i].lastCnt);
                fmFree(switchExt->flowInfo.table[i].useBit);
                fmFree(switchExt->flowInfo.table[i].mapping);
                fmDeleteBitArray(&switchExt->flowInfo.table[i].idInUse);
            }
        }
        fmDeleteBitArray(&switchExt->flowInfo.balanceGrpInUse);

        switchExt->flowInfo.initialized = FALSE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000FreeFlowResource */




/*****************************************************************************/
/** fm10000InitFlowApiForSWAG
 * \ingroup intFlow
 *
 * \desc            Initialize Flow API for SWAG.
 *
 * \param[in]       sw is the switch on which to operate.
 * \param[in]       defaultMcastGroupId is multicast group id for default
 *                  action.
 * \param[in]       trapMcastGroupId is multicast group id for trap action.
 * \param[in]       fwdToCpuMcastGroupId is multicast group id for
 *                  forward-to-cpu action.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitFlowApiForSWAG(fm_int sw,
                                    fm_int defaultMcastGroupId,
                                    fm_int trapMcastGroupId,
                                    fm_int fwdToCpuMcastGroupId)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;
    fm_int          logicalPort;

    switchExt = GET_SWITCH_EXT(sw);

    /* DEFAULT action */
    err = fmGetMcastGroupPort(sw, defaultMcastGroupId, &logicalPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.defaultLogicalPort = logicalPort;

    /* TRAP action */
    err = fmGetMcastGroupPort(sw, trapMcastGroupId, &logicalPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.trapLogicalPort = logicalPort;

    /* FORWARD-TO-CPU action */
    err = fmGetMcastGroupPort(sw, fwdToCpuMcastGroupId, &logicalPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);

    switchExt->flowInfo.fwdToCpuLogicalPort = logicalPort;

    err = InitFlowApiCommon(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000InitFlowApiForSWAG */




/*****************************************************************************/
/** fm10000AddFlowUser
 * \ingroup intFlow
 *
 * \desc            Add a user for this particular flow. This is used to track
 *                  the usage of a particular flow and prevent the deletion
 *                  of this latter if currently used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is flow table index part of the entry.
 *
 * \param[in]       flowId is flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if tableIndex or flowId are invalid.
 *
 *****************************************************************************/
fm_status fm10000AddFlowUser(fm_int sw,
                             fm_int tableIndex,
                             fm_int flowId)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
         (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switchExt->flowInfo.table[tableIndex].useCnt[flowId]++;

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000AddFlowUser */




/*****************************************************************************/
/** fm10000DelFlowUser
 * \ingroup intFlow
 *
 * \desc            Delete a user for this particular flow. This is used to
 *                  track the usage of a particular flow and prevent the
 *                  deletion of this latter if currently used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tableIndex is flow table index part of the entry.
 *
 * \param[in]       flowId is flow id part of the entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if tableIndex or flowId are invalid.
 *
 *****************************************************************************/
fm_status fm10000DelFlowUser(fm_int sw,
                             fm_int tableIndex,
                             fm_int flowId)
{
    fm_status       err = FM_OK;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d, tableIndex = %d, flowId = %d\n",
                 sw,
                 tableIndex,
                 flowId);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (tableIndex >= FM_FLOW_MAX_TABLE_TYPE) || (tableIndex < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    if ( (flowId > switchExt->flowInfo.table[tableIndex].idInUse.bitCount - 1) ||
         (flowId < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    switchExt->flowInfo.table[tableIndex].useCnt[flowId]--;

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000DelFlowUser */




/*****************************************************************************/
/** fm10000GetFlowTableIndexUnused
 * \ingroup intFlow
 *
 * \desc            Gets the first unused flow table index.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tableIndex points to caller-supplied location where
 *                  the unused table index should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if tableIndex is invalid.
 * \return          FM_ERR_NO_MORE if no unused tables indexes have been found.
 *
 *****************************************************************************/
fm_status fm10000GetFlowTableIndexUnused(fm_int  sw,
                                         fm_int *tableIndex)
{
    fm_status         err;
    fm10000_switch *  switchExt;
    fm10000_flowInfo *flowInfo;
    fm_bool           found;
    fm_int            index;

    FM_LOG_ENTRY(FM_LOG_CAT_FLOW,
                 "sw = %d\n",
                 sw);

    err       = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);
    found     = FALSE;

    if ( tableIndex == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_FLOW, err);
    }

    flowInfo = &switchExt->flowInfo;

    for (index = 0 ; index < FM_FLOW_MAX_TABLE_TYPE ; index++)
    {
        if (flowInfo->table[index].created == FALSE)
        {
            *tableIndex = index;
            found = TRUE;
            break;
        }
    }

    if (found == FALSE)
    {
        err = FM_ERR_NO_MORE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000GetFlowTableIndexUnused */




/*****************************************************************************/
/** fm10000GetFlowTableSupportedActions
 * \ingroup intFlow
 *
 * \desc            Gets the bit mask of actions supported by given flow table
 *                  type.
 *
 * \note            This functions assumes that the FLOW lock have already
 *                  been taken.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flowTableType is the flow table type.
 *
 * \param[out]      flowAction points to caller-supplied storage where the flow
 *                  table action mask should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if flowTableType is not supported.
 *
 *****************************************************************************/
fm_status fm10000GetFlowTableSupportedActions(fm_int           sw,
                                              fm_flowTableType flowTableType,
                                              fm_flowAction *  flowAction)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_FLOW,
                     "sw = %d flowTableType=%d\n",
                     sw,
                     flowTableType);

    switch (flowTableType)
    {
        case FM_FLOW_TCAM_TABLE:
            *flowAction = FM10000_FLOW_SUPPORTED_TCAM_ACTIONS;
            break;

        case FM_FLOW_BST_TABLE:
            *flowAction = FM10000_FLOW_SUPPORTED_BST_ACTIONS;
            break;

        case FM_FLOW_TE_TABLE:
            *flowAction = FM10000_FLOW_SUPPORTED_TE_ACTIONS;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_FLOW, err);

}   /* end fm10000GetFlowTableSupportedActions */
