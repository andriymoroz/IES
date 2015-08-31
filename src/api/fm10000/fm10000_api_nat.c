/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_nat.c
 * Creation Date:   March 13, 2014
 * Description:     FM10000 NAT API.
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


/*****************************************************************************/
/** fmFreeNatRuleCond
 * \ingroup intNat
 *
 * \desc            Free a fm_fm10000NatRuleCond structure.
 *
 * \param[in]       key points to the key.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeNatRuleCond(void *key, void *value)
{
    FM_NOT_USED(key);

    fmFree(value);

}   /* end fmFreeNatRuleCond */




/*****************************************************************************/
/** fmFreeNatPrefixRule
 * \ingroup intNat
 *
 * \desc            Free a fm_tree structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeNatPrefixRule(void *value)
{
    fm_tree *natPrefixRule = (fm_tree*)value;
    if (natPrefixRule != NULL)
    {
        fmTreeDestroy(natPrefixRule, NULL);
        fmFree(value);
    }

}   /* end fmFreeNatPrefixRule */




/*****************************************************************************/
/** fmFreeNatPrefix
 * \ingroup intNat
 *
 * \desc            Free a fm_fm10000NatPrefix structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeNatPrefix(void *value)
{
    fm_fm10000NatPrefix *natPrefix = (fm_fm10000NatPrefix*)value;
    if (natPrefix != NULL)
    {
        fmTreeDestroy(&natPrefix->aclRule, fmFreeNatPrefixRule);
        fmFree(value);
    }

}   /* end fmFreeNatPrefix */




/*****************************************************************************/
/** fmFreeTable
 * \ingroup intNat
 *
 * \desc            Free a fm_fm10000NatTable structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeNatTable(void *value)
{
    fm_fm10000NatTable *natTable = (fm_fm10000NatTable*)value;
    if (natTable != NULL)
    {
        fmTreeDestroy(&natTable->tunnels, fmFree);
        fmTreeDestroy(&natTable->rules, fmFree);
        fmTreeDestroy(&natTable->prefilters, fmFree);
        fmCustomTreeDestroy(&natTable->ruleCondition, fmFreeNatRuleCond);
        fmDeleteBitArray(&natTable->ruleInUse);
        fmTreeDestroy(&natTable->prefixs, fmFreeNatPrefix);

        fmFree(value);
    }

}   /* end fmFreeNatTable */




/*****************************************************************************/
/** fmCompareNatRules
 * \ingroup intNat
 *
 * \desc            Compare NAT Rule entries.
 *
 * \param[in]       first points to the first rule.
 *
 * \param[in]       second points to the second rule.
 *
 * \return          -1 if the first rule sorts before the second.
 * \return           0 if the rule are identical.
 * \return           1 if the first rule sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareNatRules(const void *first, const void *second)
{
    fm_fm10000NatRuleCond *firstRule  = (fm_fm10000NatRuleCond *) first;
    fm_fm10000NatRuleCond *secondRule = (fm_fm10000NatRuleCond *) second;

    if (firstRule->condition < secondRule->condition)
    {
        return -1;
    }
    else if (firstRule->condition > secondRule->condition)
    {
        return 1;
    }

    /* Same condition mask */
    if (firstRule->condition & FM_NAT_MATCH_DST_IP)
    {
        if (ntohl(firstRule->cndParam.dstIpMask.addr[0]) < ntohl(secondRule->cndParam.dstIpMask.addr[0]))
        {
            return -1;
        }
        else if (ntohl(firstRule->cndParam.dstIpMask.addr[0]) > ntohl(secondRule->cndParam.dstIpMask.addr[0]))
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_SRC_IP)
    {
        if (ntohl(firstRule->cndParam.srcIpMask.addr[0]) < ntohl(secondRule->cndParam.srcIpMask.addr[0]))
        {
            return -1;
        }
        else if (ntohl(firstRule->cndParam.srcIpMask.addr[0]) > ntohl(secondRule->cndParam.srcIpMask.addr[0]))
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_DST_IP)
    {
        if (ntohl(firstRule->cndParam.dstIp.addr[0]) < ntohl(secondRule->cndParam.dstIp.addr[0]))
        {
            return -1;
        }
        else if (ntohl(firstRule->cndParam.dstIp.addr[0]) > ntohl(secondRule->cndParam.dstIp.addr[0]))
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_SRC_IP)
    {
        if (ntohl(firstRule->cndParam.srcIp.addr[0]) < ntohl(secondRule->cndParam.srcIp.addr[0]))
        {
            return -1;
        }
        else if (ntohl(firstRule->cndParam.srcIp.addr[0]) > ntohl(secondRule->cndParam.srcIp.addr[0]))
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_SRC_MAC)
    {
        if (firstRule->cndParam.smac < secondRule->cndParam.smac)
        {
            return -1;
        }
        else if (firstRule->cndParam.smac > secondRule->cndParam.smac)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_DST_MAC)
    {
        if (firstRule->cndParam.dmac < secondRule->cndParam.dmac)
        {
            return -1;
        }
        else if (firstRule->cndParam.dmac > secondRule->cndParam.dmac)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_VLAN)
    {
        if (firstRule->cndParam.vlan < secondRule->cndParam.vlan)
        {
            return -1;
        }
        else if (firstRule->cndParam.vlan > secondRule->cndParam.vlan)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_VLAN2)
    {
        if (firstRule->cndParam.vlan2 < secondRule->cndParam.vlan2)
        {
            return -1;
        }
        else if (firstRule->cndParam.vlan2 > secondRule->cndParam.vlan2)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_PROTOCOL)
    {
        if (firstRule->cndParam.protocol < secondRule->cndParam.protocol)
        {
            return -1;
        }
        else if (firstRule->cndParam.protocol > secondRule->cndParam.protocol)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_L4_SRC_PORT)
    {
        if (firstRule->cndParam.l4Src < secondRule->cndParam.l4Src)
        {
            return -1;
        }
        else if (firstRule->cndParam.l4Src > secondRule->cndParam.l4Src)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_L4_DST_PORT)
    {
        if (firstRule->cndParam.l4Dst < secondRule->cndParam.l4Dst)
        {
            return -1;
        }
        else if (firstRule->cndParam.l4Dst > secondRule->cndParam.l4Dst)
        {
            return 1;
        }
    }

    if (firstRule->condition & FM_NAT_MATCH_SRC_PORT)
    {
        if (firstRule->cndParam.logicalPort < secondRule->cndParam.logicalPort)
        {
            return -1;
        }
        else if (firstRule->cndParam.logicalPort > secondRule->cndParam.logicalPort)
        {
            return 1;
        }
    }

    return 0;

}   /* end fmCompareNatRules */




/*****************************************************************************/
/** FreeNatCfgStruct
 * \ingroup intNat
 *
 * \desc            Free a fm_fm10000NatCfg structure.
 *
 * \param[in,out]   natCfg points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FreeNatCfgStruct(fm_fm10000NatCfg *natCfg)
{
    if (natCfg)
    {
        fmTreeDestroy(&natCfg->tables, fmFreeNatTable);

        fmFree(natCfg);
    }

}   /* end FreeNatCfgStruct */




/*****************************************************************************/
/** InitializeNatCfgStruct
 * \ingroup intNat
 *
 * \desc            Initialize a fm_fm10000NatCfg structure.
 *
 * \param[in,out]   natCfg points to the structure to initialize.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void InitializeNatCfgStruct(fm_fm10000NatCfg *natCfg)
{
    fmTreeInit(&natCfg->tables);

}   /* end InitializeNatCfgStruct */




/*****************************************************************************/
/** TranslateNatCondToTunCond
 * \ingroup intNat
 *
 * \desc            Translate natCondition type to tunnel condition.
 *
 * \param[in]       natCond is the condition(s) to be translated.
 *
 * \param[out]      tunCond is the translated condition(s).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateNatCondToTunCond(fm_natCondition     natCond,
                                           fm_tunnelCondition *tunCond)
{
    *tunCond = 0;
    if (natCond & FM_NAT_MATCH_SRC_MAC)
    {
        *tunCond |= FM_TUNNEL_MATCH_SMAC;
        natCond &= ~FM_NAT_MATCH_SRC_MAC;
    }

    if (natCond & FM_NAT_MATCH_DST_MAC)
    {
        *tunCond |= FM_TUNNEL_MATCH_DMAC;
        natCond &= ~FM_NAT_MATCH_DST_MAC;
    }

    if (natCond & FM_NAT_MATCH_SRC_IP)
    {
        *tunCond |= FM_TUNNEL_MATCH_SIP;
        natCond &= ~FM_NAT_MATCH_SRC_IP;
    }

    if (natCond & FM_NAT_MATCH_DST_IP)
    {
        *tunCond |= FM_TUNNEL_MATCH_DIP;
        natCond &= ~FM_NAT_MATCH_DST_IP;
    }

    if (natCond & FM_NAT_MATCH_PROTOCOL)
    {
        *tunCond |= FM_TUNNEL_MATCH_PROT;
        natCond &= ~FM_NAT_MATCH_PROTOCOL;
    }

    if (natCond & FM_NAT_MATCH_L4_SRC_PORT)
    {
        *tunCond |= FM_TUNNEL_MATCH_L4SRC;
        natCond &= ~FM_NAT_MATCH_L4_SRC_PORT;
    }

    if (natCond & FM_NAT_MATCH_L4_DST_PORT)
    {
        *tunCond |= FM_TUNNEL_MATCH_L4DST;
        natCond &= ~FM_NAT_MATCH_L4_DST_PORT;
    }

    if (natCond & FM_NAT_MATCH_VNI)
    {
        *tunCond |= FM_TUNNEL_MATCH_VNI;
        natCond &= ~FM_NAT_MATCH_VNI;
    }

    if (natCond & FM_NAT_MATCH_UDP)
    {
        *tunCond |= FM_TUNNEL_MATCH_UDP;
        natCond &= ~FM_NAT_MATCH_UDP;
    }

    if (natCond & FM_NAT_MATCH_TCP)
    {
        *tunCond |= FM_TUNNEL_MATCH_TCP;
        natCond &= ~FM_NAT_MATCH_TCP;
    }

    if (natCond)
    {
        return FM_ERR_UNSUPPORTED;
    }
    else
    {
        return FM_OK;
    }


}   /* end TranslateNatCondToTunCond */




/*****************************************************************************/
/** TranslateNatActToTunAct
 * \ingroup intNat
 *
 * \desc            Translate NAT Action field and structure to the Tunnel one.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       action is the actions(s) to be translated.
 *
 * \param[in]       actParam is the actions(s) parameters to be translated.
 *
 * \param[out]      tunAction is the translated actions(s).
 *
 * \param[out]      tunActParam is the translated actions(s) parameters.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateNatActToTunAct(fm_int                sw,
                                         fm_natAction          action,
                                         fm_natActionParam *   actParam,
                                         fm_tunnelAction *     tunAction,
                                         fm_tunnelActionParam *tunActParam)
{
    fm_uint32 glort;
    fm_status err = FM_OK;

    *tunAction = 0;
    if (action & FM_NAT_ACTION_SET_DMAC)
    {
        *tunAction |= FM_TUNNEL_SET_DMAC;
        tunActParam->dmac = actParam->dmac;
        action &= ~FM_NAT_ACTION_SET_DMAC;
    }

    if (action & FM_NAT_ACTION_SET_SMAC)
    {
        *tunAction |= FM_TUNNEL_SET_SMAC;
        tunActParam->smac = actParam->smac;
        action &= ~FM_NAT_ACTION_SET_SMAC;
    }

    if (action & FM_NAT_ACTION_SET_DIP)
    {
        *tunAction |= FM_TUNNEL_SET_DIP;
        tunActParam->dip = actParam->dstIp;
        action &= ~FM_NAT_ACTION_SET_DIP;
    }

    if (action & FM_NAT_ACTION_SET_SIP)
    {
        *tunAction |= FM_TUNNEL_SET_SIP;
        tunActParam->sip = actParam->srcIp;
        action &= ~FM_NAT_ACTION_SET_SIP;
    }

    if (action & FM_NAT_ACTION_SET_L4SRC)
    {
        *tunAction |= FM_TUNNEL_SET_L4SRC;
        tunActParam->l4Src = actParam->l4Src;
        action &= ~FM_NAT_ACTION_SET_L4SRC;
    }

    if (action & FM_NAT_ACTION_SET_L4DST)
    {
        *tunAction |= FM_TUNNEL_SET_L4DST;
        tunActParam->l4Dst = actParam->l4Dst;
        action &= ~FM_NAT_ACTION_SET_L4DST;
    }

    if (action & FM_NAT_ACTION_SET_TTL)
    {
        *tunAction |= FM_TUNNEL_SET_TTL;
        tunActParam->ttl = actParam->ttl;
        action &= ~FM_NAT_ACTION_SET_TTL;
    }

    if (action & FM_NAT_ACTION_COUNT)
    {
        *tunAction |= FM_TUNNEL_COUNT;
        action &= ~FM_NAT_ACTION_COUNT;
    }

    if (action & FM_NAT_ACTION_TUNNEL)
    {
        *tunAction |= FM_TUNNEL_ENCAP_FLOW;
        tunActParam->encapFlow = actParam->tunnel;
        action &= ~FM_NAT_ACTION_TUNNEL;

        if (action & FM_NAT_ACTION_SET_TUNNEL_VNI)
        {
            *tunAction |= FM_TUNNEL_SET_VNI;
            tunActParam->vni = actParam->vni;
            action &= ~FM_NAT_ACTION_SET_TUNNEL_VNI;
        }

        if (action & FM_NAT_ACTION_SET_TUNNEL_NGE)
        {
            *tunAction |= FM_TUNNEL_SET_NGE;
            tunActParam->ngeMask = actParam->ngeMask;
            FM_MEMCPY_S(tunActParam->ngeData,
                        sizeof(tunActParam->ngeData),
                        actParam->ngeData,
                        sizeof(actParam->ngeData));
            action &= ~FM_NAT_ACTION_SET_TUNNEL_NGE;
        }
    }
    else if (action & FM_NAT_ACTION_SET_TUNNEL_VSI)
    {
        *tunAction |= FM_TUNNEL_SET_DGLORT;
        err = fmGetLogicalPortGlort(sw, actParam->logicalPort, &glort);
        if (err != FM_OK)
        {
            return err;
        }
        tunActParam->dglort = glort;
        action &= ~FM_NAT_ACTION_SET_TUNNEL_VSI;
    }

    if (action)
    {
        return FM_ERR_UNSUPPORTED;
    }
    else
    {
        return FM_OK;
    }


}   /* end TranslateNatActToTunAct */




/*****************************************************************************/
/** TranslateNatCondToAclCond
 * \ingroup intNat
 *
 * \desc            Translate natCondition type to ACL Condition.
 *
 * \param[in]       natCond is the condition(s) to be translated.
 *
 * \param[out]      aclCond is the translated condition(s).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateNatCondToAclCond(fm_natCondition  natCond,
                                           fm_aclCondition *aclCond)
{
    *aclCond = 0;
    if (natCond & FM_NAT_MATCH_SRC_MAC)
    {
        *aclCond |= FM_ACL_MATCH_SRC_MAC;
        natCond &= ~FM_NAT_MATCH_SRC_MAC;
    }

    if (natCond & FM_NAT_MATCH_DST_MAC)
    {
        *aclCond |= FM_ACL_MATCH_DST_MAC;
        natCond &= ~FM_NAT_MATCH_DST_MAC;
    }

    if (natCond & FM_NAT_MATCH_VLAN)
    {
        *aclCond |= FM_ACL_MATCH_VLAN;
        natCond &= ~FM_NAT_MATCH_VLAN;
    }

    if (natCond & FM_NAT_MATCH_VLAN2)
    {
        *aclCond |= FM_ACL_MATCH_VLAN2;
        natCond &= ~FM_NAT_MATCH_VLAN2;
    }

    if (natCond & FM_NAT_MATCH_SRC_IP)
    {
        *aclCond |= FM_ACL_MATCH_SRC_IP;
        natCond &= ~FM_NAT_MATCH_SRC_IP;
    }

    if (natCond & FM_NAT_MATCH_DST_IP)
    {
        *aclCond |= FM_ACL_MATCH_DST_IP;
        natCond &= ~FM_NAT_MATCH_DST_IP;
    }

    if (natCond & FM_NAT_MATCH_PROTOCOL)
    {
        *aclCond |= FM_ACL_MATCH_PROTOCOL;
        natCond &= ~FM_NAT_MATCH_PROTOCOL;
    }

    if (natCond & FM_NAT_MATCH_L4_SRC_PORT)
    {
        *aclCond |= FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK;
        natCond &= ~FM_NAT_MATCH_L4_SRC_PORT;
    }

    if (natCond & FM_NAT_MATCH_L4_DST_PORT)
    {
        *aclCond |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
        natCond &= ~FM_NAT_MATCH_L4_DST_PORT;
    }

    if (natCond & FM_NAT_MATCH_SRC_PORT)
    {
        *aclCond |= FM_ACL_MATCH_SRC_PORT;
        natCond &= ~FM_NAT_MATCH_SRC_PORT;
    }

    if (natCond)
    {
        return FM_ERR_UNSUPPORTED;
    }
    else
    {
        return FM_OK;
    }


}   /* end TranslateNatCondToAclCond */




/*****************************************************************************/
/** SetNatTunnelDefault
 * \ingroup intNat
 *
 * \desc            Configure default tunnel mode and behaviour.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunnel engine on which to operate
 *
 * \param[in]       tunnelDefault refer to the various tunnel configuration
 *                  to set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetNatTunnelDefault(fm_int               sw,
                                     fm_int               te,
                                     fm_natTunnelDefault *tunnelDefault)
{
    fm_status                err = FM_OK;
    fm_fm10000TeTunnelCfg    teTunnelCfg;
    fm_fm10000TeDefTunnelSel teTunnelCfgSel;

    teTunnelCfgSel = 0;

    if (tunnelDefault->type == FM_TUNNEL_TYPE_NGE)
    {
        teTunnelCfg.l4DstNge = tunnelDefault->ngeCfg.l4Dst;
        teTunnelCfg.ngeMask = tunnelDefault->ngeCfg.ngeMask;
        teTunnelCfg.ngeTime = tunnelDefault->ngeCfg.ngeTime;
        FM_MEMCPY_S(teTunnelCfg.ngeData,
                    sizeof(teTunnelCfg.ngeData),
                    tunnelDefault->ngeCfg.ngeData,
                    sizeof(tunnelDefault->ngeCfg.ngeData));
        teTunnelCfg.encapProtocol = tunnelDefault->ngeCfg.protocol;

        teTunnelCfgSel |= (FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE |
                           FM10000_TE_DEFAULT_TUNNEL_NGE_DATA |
                           FM10000_TE_DEFAULT_TUNNEL_NGE_MASK |
                           FM10000_TE_DEFAULT_TUNNEL_NGE_TIME |
                           FM10000_TE_DEFAULT_TUNNEL_PROTOCOL);
    }
    else if ( (tunnelDefault->type != FM_TUNNEL_TYPE_VXLAN) &&
              (tunnelDefault->type != FM_TUNNEL_TYPE_NVGRE) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    teTunnelCfg.ttl = tunnelDefault->ttl;
    teTunnelCfg.tos = tunnelDefault->tos;
    teTunnelCfg.deriveOuterTOS = tunnelDefault->deriveOuterTOS;
    teTunnelCfg.dmac = tunnelDefault->dmac;
    teTunnelCfg.smac = tunnelDefault->smac;

    teTunnelCfgSel |= (FM10000_TE_DEFAULT_TUNNEL_TTL |
                       FM10000_TE_DEFAULT_TUNNEL_TOS |
                       FM10000_TE_DEFAULT_TUNNEL_DMAC |
                       FM10000_TE_DEFAULT_TUNNEL_SMAC);


    err = fm10000SetTeDefaultTunnel(sw,
                                    te,
                                    &teTunnelCfg,
                                    teTunnelCfgSel,
                                    TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

ABORT:

    return err;

}   /* end SetNatTunnelDefault */




/*****************************************************************************/
/** MoveNatIndex
 * \ingroup intNat
 *
 * \desc            Move Nat Index in a non disruptive way from the source
 *                  position to the destination. This function also update
 *                  the structure to refer to the new position.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   natTable refer to the table on which to operate.
 *
 * \param[in]       src is the Nat Index to move.
 *
 * \param[in]       dst is the Nat Index destination.
 *
 * \param[in]       natRuleTree is optional and refer to the NatRule that
 *                  belongs to the source Nat Index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MoveNatIndex(fm_int              sw,
                              fm_fm10000NatTable *natTable,
                              fm_int              src,
                              fm_int              dst,
                              fm_tree            *natRuleTree)
{
    fm_status            err = FM_OK;
    fm_uint64            natRuleId;
    fm_fm10000NatRule *  natRule;
    fm_treeIterator      itEntry;
    fm_aclCondition      movedAclCond;
    fm_aclValue          movedAclCondParam;
    fm_aclActionExt      movedAclAction;
    fm_aclParamExt       movedAclActParam;
    void *               value;
    fm_uint64            prefix;
    fm_fm10000NatPrefix *natPrefix;
    fm_tree *            natRuleTreeTmp;

    /* Move this rule to the new position */
    err = fmGetACLRule(sw,
                       natTable->acl,
                       src,
                       &movedAclCond,
                       &movedAclCondParam,
                       &movedAclAction,
                       &movedAclActParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmUpdateACLRule(sw,
                          natTable->acl,
                          dst,
                          movedAclCond,
                          &movedAclCondParam,
                          movedAclAction,
                          &movedAclActParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmSetACLRuleState(sw,
                            natTable->acl,
                            dst,
                            FM_ACL_RULE_ENTRY_STATE_VALID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmSetBitArrayBit(&natTable->ruleInUse,
                           dst,
                           TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* Disable old position */
    err = fmSetACLRuleState(sw,
                            natTable->acl,
                            src,
                            FM_ACL_RULE_ENTRY_STATE_INVALID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmUpdateACLRule(sw,
                          natTable->acl,
                          src,
                          0,
                          &movedAclCondParam,
                          FM_ACL_ACTIONEXT_PERMIT,
                          &movedAclActParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmSetBitArrayBit(&natTable->ruleInUse,
                           src,
                           FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* Find the nat rule(s) that uses this acl rule by walking over all of
     * them. */
    if (natRuleTree == NULL)
    {
        for (fmTreeIterInit(&itEntry, &natTable->rules) ;
              (err = fmTreeIterNext(&itEntry,
                                    &natRuleId,
                                    (void**) &natRule)) == FM_OK ; )
        {
            if (natRule->aclRule == src)
            {
                natRule->aclRule = dst;
            }
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        err = FM_OK;
    }
    /* Use a tree that already contains the list of the nat rules to update. */
    else
    {
        for (fmTreeIterInit(&itEntry, natRuleTree) ;
              (err = fmTreeIterNext(&itEntry,
                                    &natRuleId,
                                    &value)) == FM_OK ; )
        {
            err = fmTreeFind(&natTable->rules, natRuleId, (void**) &natRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            natRule->aclRule = dst;
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        err = FM_OK;
    }

    /* Update the prefix tree if needed */
    if ( (((movedAclCond & FM_ACL_MATCH_SRC_IP) != 0) &&
          (movedAclCondParam.srcIpMask.addr[0] != 0xFFFFFFFF)) ||
         (((movedAclCond & FM_ACL_MATCH_DST_IP) != 0) &&
          (movedAclCondParam.dstIpMask.addr[0] != 0xFFFFFFFF)))
    {
        /* Compute the prefix */
        prefix = 0LL;
        if (movedAclCond & FM_ACL_MATCH_DST_IP)
        {
            prefix |= ((fm_uint64)ntohl(movedAclCondParam.dstIpMask.addr[0]) << 32);
        }

        if (movedAclCond & FM_ACL_MATCH_SRC_IP)
        {
            prefix |= (fm_uint64)ntohl(movedAclCondParam.srcIpMask.addr[0]);
        }

        err = fmTreeFind(&natTable->prefixs, prefix, (void**)&natPrefix);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeFind(&natPrefix->aclRule, src, (void**)&natRuleTreeTmp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeInsert(&natPrefix->aclRule, dst, natRuleTreeTmp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeRemoveCertain(&natPrefix->aclRule, src, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    }


ABORT:

    return err;

}   /* end MoveNatIndex */




/*****************************************************************************/
/** ShiftNatIndex
 * \ingroup intNat
 *
 * \desc            Shift Nat Index in a non disruptive way from the start
 *                  position up to the end. The end position will be freed
 *                  after the completion of this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   natTable refer to the table on which to operate.
 *
 * \param[in]       start is the first Nat Index that must be filled.
 *
 * \param[in]       end is the position that must be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ShiftNatIndex(fm_int              sw,
                               fm_fm10000NatTable *natTable,
                               fm_int              start,
                               fm_int              end)
{
    fm_status            err = FM_OK;
    fm_fm10000NatPrefix *natPrefix;
    fm_treeIterator      itEntry;
    fm_treeIterator      itRule;
    fm_uint64            nextPrefix;
    fm_uint64            nextRule;
    fm_tree *            natRule;
    fm_int               freeSlot;

    freeSlot = start;

    /* Scan the prefix in ascending order */
    if (start > end)
    {
        for (fmTreeIterInit(&itEntry, &natTable->prefixs) ;
              (err = fmTreeIterNext(&itEntry,
                                    &nextPrefix,
                                    (void**) &natPrefix)) == FM_OK ; )
        {
            fmTreeIterInit(&itRule, &natPrefix->aclRule);
            err = fmTreeIterNext(&itRule, &nextRule, (void**)&natRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Crossed over the end position? */
            if ((fm_int)nextRule < end)
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            /* The free slot must be filled */
            else if ((fm_int)nextRule < freeSlot)
            {
                /* Move that position to the free slot */
                err = MoveNatIndex(sw, natTable, nextRule, freeSlot, natRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                freeSlot = nextRule;

                /* Reached our goal */
                if (freeSlot == end)
                {
                    break;
                }
            }

        }
        if ( (err != FM_ERR_NO_MORE) &&
             (err != FM_OK) )
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        err = FM_OK;
    }
    /* Scan the prefix in reverse order */
    else
    {
        for (fmTreeIterInitBackwards(&itEntry, &natTable->prefixs) ;
              (err = fmTreeIterNext(&itEntry,
                                    &nextPrefix,
                                    (void**) &natPrefix)) == FM_OK ; )
        {
            fmTreeIterInitBackwards(&itRule, &natPrefix->aclRule);
            err = fmTreeIterNext(&itRule, &nextRule, (void**)&natRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Crossed over the end position? */
            if ((fm_int)nextRule > end)
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            /* The free slot must be filled */
            else if ((fm_int)nextRule > freeSlot)
            {
                /* Move that position to the free slot */
                err = MoveNatIndex(sw, natTable, nextRule, freeSlot, natRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                freeSlot = nextRule;

                /* Reached our goal */
                if (freeSlot == end)
                {
                    break;
                }
            }

        }
        if ( (err != FM_ERR_NO_MORE) &&
             (err != FM_OK) )
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        err = FM_OK;
    }


ABORT:

    return err;

}   /* end ShiftNatIndex */




/*****************************************************************************/
/** FreeNatIndex
 * \ingroup intNat
 *
 * \desc            Free an ACL rule index by non disruptively moving the rule
 *                  and kept proper sorting.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   natTable refer to the table on which to operate.
 *
 * \param[out]      natIndex is the free position found.
 *
 * \param[in]       prefix is the prefix position that belongs to the position
 *                  to free.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeNatIndex(fm_int              sw,
                              fm_fm10000NatTable *natTable,
                              fm_int             *natIndex,
                              fm_uint64           prefix)
{
    fm_status            err = FM_OK;
    fm_fm10000NatPrefix *natPrefix;
    fm_treeIterator      itEntry;
    fm_treeIterator      itRule;
    fm_uint64            nextPrefix;
    fm_uint64            nextRule;
    fm_int               lowLimit;
    fm_int               highLimit;
    fm_int               freeSize;
    fm_int               prefixLowLimit;
    fm_int               prefixHighLimit;
    fm_int               prefixLowFree;
    fm_int               prefixHighFree;
    void *               value;
    fm_int               highFree;
    fm_int               lowFree;

    /* Initialize the limit with the global prefix area */
    lowLimit = natTable->prefixLimit;
    highLimit = natTable->ruleInUse.bitCount;

    err = fmTreeFind(&natTable->prefixs, prefix, (void**) &natPrefix);
    if (err == FM_ERR_NOT_FOUND)
    {
        /* Find limits surrounding the prefix to add */
        for (fmTreeIterInitBackwards(&itEntry, &natTable->prefixs) ;
              (err = fmTreeIterNext(&itEntry,
                                    &nextPrefix,
                                    (void**) &natPrefix)) == FM_OK ; )
        {
            if (nextPrefix < prefix)
            {
                fmTreeIterInit(&itRule, &natPrefix->aclRule);

                err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                highLimit = (fm_int)nextRule;
                break;
            }
            else
            {
                fmTreeIterInitBackwards(&itRule, &natPrefix->aclRule);

                err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                lowLimit = (fm_int)nextRule;
            }
        }
        if ( (err != FM_ERR_NO_MORE) &&
             (err != FM_OK) )
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        err = FM_OK;

        freeSize = (highLimit - lowLimit) - 1;

        /* Space available without having to move anything */
        if (freeSize > 0)
        {
            *natIndex = (highLimit - ((freeSize + 1) / 2));
            return err;
        }

    }
    else if (err != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    else
    {
        /* Find limit for this specific prefix */
        fmTreeIterInit(&itRule, &natPrefix->aclRule);

        err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        prefixLowLimit = (fm_int)nextRule;

        fmTreeIterInitBackwards(&itRule, &natPrefix->aclRule);

        err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        prefixHighLimit = (fm_int)nextRule;

        /* Try to pick a free slot available in this specific prefix range */
        err = fmFindBitInBitArray(&natTable->ruleInUse,
                                  prefixLowLimit,
                                  FALSE,
                                  &lowFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if ( (lowFree > 0) && (lowFree <= prefixHighLimit) )
        {
            *natIndex = lowFree;
            return err;
        }

        /* Find limit based on surrending prefixes */
        err = fmTreePredecessor(&natTable->prefixs,
                                prefix,
                                &nextPrefix,
                                (void**) &natPrefix);
        if (err == FM_OK)
        {
            fmTreeIterInit(&itRule, &natPrefix->aclRule);

            err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            highLimit = (fm_int)nextRule;
        }
        else if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeSuccessor(&natTable->prefixs,
                              prefix,
                              &nextPrefix,
                              (void**) &natPrefix);
        if (err == FM_OK)
        {
            fmTreeIterInitBackwards(&itRule, &natPrefix->aclRule);

            err = fmTreeIterNext(&itRule, &nextRule, (void**)&value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            lowLimit = (fm_int)nextRule;
        }
        else if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        /* Compute which side has the more spaces available */
        prefixLowFree = (prefixLowLimit - lowLimit) - 1;
        prefixHighFree = (highLimit - prefixHighLimit) - 1;

        /* If space is available, just pick the best slot */
        if ( (prefixLowFree > 0) && (prefixLowFree > prefixHighFree) )
        {
            *natIndex = (prefixLowLimit - 1);
            return err;
        }
        else if (prefixHighFree > 0)
        {
            *natIndex = (prefixHighLimit + 1);
            return err;
        }
    }

    /* This step is only performed if no free slot are available. In that case
     * one free slot must be created by moving rules. For performance
     * enhancement, the code first try to find the closest free slot. */
    err = fmFindBitInBitArray(&natTable->ruleInUse,
                              highLimit,
                              FALSE,
                              &highFree);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmFindLastBitInBitArray(&natTable->ruleInUse,
                                  lowLimit,
                                  FALSE,
                                  &lowFree);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* Should have space on non full prefix range as validated before */
    if (lowFree <= natTable->prefixLimit)
    {
        lowFree = -1;
    }

    /* Should never happen, must be catch before */
    if ( (highFree < 0) && (lowFree < 0) )
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    /* No space currently available on less precedence prefix, use other side */
    else if (highFree < 0)
    {
        err = ShiftNatIndex(sw, natTable, lowFree, lowLimit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        *natIndex = lowLimit;
        return err;
    }
    /* No space currently available on high precedence prefix, use other side */
    else if (lowFree < 0)
    {
        err = ShiftNatIndex(sw, natTable, highFree, highLimit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        *natIndex = highLimit;
        return err;
    }
    /* Space available on both side, choose the one with the most space */
    else
    {
        if ( (highFree - highLimit) > (lowLimit - lowFree) )
        {
            err = ShiftNatIndex(sw, natTable, lowFree, lowLimit);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            *natIndex = lowLimit;
            return err;
        }
        else
        {
            err = ShiftNatIndex(sw, natTable, highFree, highLimit);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            *natIndex = highLimit;
            return err;
        }
    }

ABORT:

    return err;

}   /* end FreeNatIndex */




/*****************************************************************************/
/** FindProperNatIndex
 * \ingroup intNat
 *
 * \desc            Find the best position for the rule to be inserted. If the
 *                  current position is found to be not optimal, it will be
 *                  updated. The position is related to SIP/DIP Prefix to make
 *                  sure 8-bit prefix have lower precedence than 24-bit and so
 *                  on.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   natTable refer to the table on which to operate.
 *
 * \param[in,out]   natIndex refer to the current position that may be updated.
 *
 * \param[in]       aclCond is the translated ACL condition.
 *
 * \param[in]       aclCondParam refer to the translated ACL condition
 *                  parameters.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FindProperNatIndex(fm_int              sw,
                                    fm_fm10000NatTable *natTable,
                                    fm_int             *natIndex,
                                    fm_aclCondition     aclCond,
                                    fm_aclValue        *aclCondParam)
{
    fm_status err = FM_OK;
    fm_int    foundBit;
    fm_int    bitCount;
    fm_int    ruleToMove;
    fm_int    i;
    fm_bool   bitValue;
    fm_uint64 prefix;

    /* Full SIP && DIP Prefix are inserted in high priority slot without any
     * sorting between each of them. Same apply for rule without SIP && DIP
     * condition. */
    if ( (((aclCond & FM_ACL_MATCH_SRC_IP) == 0) ||
          (aclCondParam->srcIpMask.addr[0] == 0xFFFFFFFF)) &&
         (((aclCond & FM_ACL_MATCH_DST_IP) == 0) ||
          (aclCondParam->dstIpMask.addr[0] == 0xFFFFFFFF)))
    {
        /* Everything Ok */
        if (natTable->prefixLimit >= *natIndex)
        {
            return FM_OK;
        }
        else
        {
            /* Move rules with partial mask to free one slot on the highest
             * precedence block */
            err = FreeNatIndex(sw, natTable, natIndex, 0xFFFFFFFFFFFFFFFFLL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            natTable->prefixLimit = *natIndex;
        }
    }
    else
    {
        err = fmFindBitInBitArray(&natTable->ruleInUse,
                                  natTable->prefixLimit + 1,
                                  FALSE,
                                  &foundBit);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        /* Prefix area is full, need to move the prefix limit value */
        if (foundBit == -1)
        {
            /* Move the limit to the middle point to reassign same free slot
             * number for both full mask and partial one. */
            err = fmGetBitArrayNonZeroBitCount(&natTable->ruleInUse, &bitCount);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            ruleToMove = ((natTable->ruleInUse.bitCount + 1 - bitCount) / 2);

            for (i = 0 ; i < ruleToMove ; i++)
            {
                err = fmGetBitArrayBit(&natTable->ruleInUse,
                                       natTable->prefixLimit - i,
                                       &bitValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                /* Rule at that position, move it */
                if (bitValue)
                {
                    err = fmFindBitInBitArray(&natTable->ruleInUse,
                                              0,
                                              FALSE,
                                              &foundBit);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                    /* Protection on situation that should never happen */
                    if ( (foundBit == -1) ||
                         (foundBit > (natTable->prefixLimit - ruleToMove)) )
                    {
                        err = FM_FAIL;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                    }

                    err = MoveNatIndex(sw,
                                       natTable,
                                       natTable->prefixLimit - i,
                                       foundBit,
                                       NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }
            }

            natTable->prefixLimit -= ruleToMove;

        }

        /* Build the prefix and find a position that belongs to this one */
        prefix = 0LL;
        if (aclCond & FM_ACL_MATCH_DST_IP)
        {
            prefix |= ((fm_uint64)ntohl(aclCondParam->dstIpMask.addr[0]) << 32);
        }

        if (aclCond & FM_ACL_MATCH_SRC_IP)
        {
            prefix |= (fm_uint64)ntohl(aclCondParam->srcIpMask.addr[0]);
        }

        err = FreeNatIndex(sw, natTable, natIndex, prefix);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

ABORT:

    return err;

}   /* end FindProperNatIndex */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000NatInit
 * \ingroup intNat
 *
 * \desc            Initialize the NAT members of the FM10000 switch
 *                  extension.
 *
 * \note            This function is called from fm10000InitSwitch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000NatInit(fm_int sw)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    if (switchExt->natCfg != NULL)
    {
        FreeNatCfgStruct(switchExt->natCfg);
        switchExt->natCfg = NULL;
    }

    switchExt->natCfg = fmAlloc( sizeof(fm_fm10000NatCfg) );

    if (switchExt->natCfg == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    FM_CLEAR(*switchExt->natCfg);

    InitializeNatCfgStruct(switchExt->natCfg);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000NatInit */




/*****************************************************************************/
/** fm10000NatFree
 * \ingroup intNat
 *
 * \desc            Free the memory used by the NAT members of the
 *                  FM10000 switch extension.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000NatFree(fm_int sw)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    if (switchExt->natCfg != NULL)
    {
        FreeNatCfgStruct(switchExt->natCfg);
        switchExt->natCfg = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000NatFree */




/*****************************************************************************/
/** fm10000CreateNatTable
 * \ingroup intNat
 *
 * \desc            Create a NAT Table. The NAT Table will be created based on
 *                  configuration as specified with natParam.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       natParam refer to the table parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if matching condition is not supported.
 * \return          FM_ERR_TUNNEL_NO_FREE_GROUP if no free slot available.
 * \return          FM_ERR_TUNNEL_LOOKUP_FULL if lookup table is full.
 * \return          FM_ERR_TUNNEL_GLORT_FULL if te glort table is full.
 * \return          FM_ERR_ACLS_TOO_BIG if the compiled NAT table will not fit
 *                  in the portion of the FFU allocated to ACLs.
 *
 *****************************************************************************/
fm_status fm10000CreateNatTable(fm_int sw, fm_int table, fm_natParam *natParam)
{
    fm_switch *          switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *     switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status            err = FM_OK;
    fm_int               group;
    fm_tunnelParam       tunnelParam;
    fm_int               aclSize;
    fm_aclCondition      aclCond;
    fm_aclValue          aclCondMask;
    fm_aclActionExt      aclAct;
    fm_aclParamExt       aclActParam;
    fm_int               aclId;
    fm_bool              keepUnusedKey;
    fm_bool              defaultDglort;
    fm_int               i;
    fm_fm10000NatTable * natTable;
    fm_char              statusText[1024];
    const fm_uint32      allScenarios = (FM_ACL_SCENARIO_ANY_FRAME_TYPE |
                                         FM_ACL_SCENARIO_ANY_ROUTING_TYPE |
                                         FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE);

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    tunnelParam.te = (natParam->direction) ? 0 : 1;
    tunnelParam.size = natParam->size;
    tunnelParam.useUser = FALSE;

    if ( (natParam->direction == FALSE) && (natParam->tunnel == TRUE) )
    {
        tunnelParam.encap = FALSE;
    }
    else
    {
        tunnelParam.encap = TRUE;
    }
    tunnelParam.tepSize = 0;

    if (natParam->mode == FM_NAT_MODE_PERFORMANCE)
    {
        tunnelParam.hashKeyConfig = 0;
    }
    else
    {
        err = TranslateNatCondToTunCond(natParam->ruleCond,
                                        &tunnelParam.hashKeyConfig);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if (tunnelParam.hashKeyConfig == 0)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
    }

    err = fm10000CreateTunnel(sw, &group, &tunnelParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    if (natParam->mode == FM_NAT_MODE_PERFORMANCE)
    {
        aclSize = natParam->size;
        err = TranslateNatCondToAclCond(natParam->ruleCond,
                                        &aclCond);
        if (err != FM_OK)
        {
            fm10000DeleteTunnel(sw, group);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
    }
    else
    {
        aclSize = FM10000_MAX_RULE_PER_ACL_PART;
        err = TranslateNatCondToAclCond(natParam->preFilterCond,
                                        &aclCond);
        if (err != FM_OK)
        {
            fm10000DeleteTunnel(sw, group);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
    }

    defaultDglort = TRUE;
    err = fm10000SetTunnelAttribute(sw,
                                    group,
                                    0,
                                    FM_TUNNEL_SET_DEFAULT_DGLORT,
                                    &defaultDglort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    aclId = FM10000_NAT_BASE_ACL + group;

    /* Match on GLORT and non GLORT type to cover PEP port */
    err = fmCreateACLInt(sw,
                         aclId,
                         allScenarios,
                         FM_ACL_DEFAULT_PRECEDENCE,
                         TRUE);
    if (err != FM_OK)
    {
        fm10000DeleteTunnel(sw, group);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    keepUnusedKey = TRUE;
    err = fmSetACLAttribute(sw,
                            aclId,
                            FM_ACL_KEEP_UNUSED_KEYS,
                            &keepUnusedKey);
    if (err != FM_OK)
    {
        fm10000DeleteTunnel(sw, group);
        fmDeleteACLInt(sw, aclId, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* Create an ACL with the specified size and all the keys. This ACL will
     * always uses the same number of resource during the whole process. Rule
     * will be inserted/removed using update function for performance
     * enhancement. */
    FM_MEMSET_S(&aclCondMask, sizeof(fm_aclValue), 0, sizeof(fm_aclValue));

    aclCondMask.srcMask = 0xffffffffffffLL;
    aclCondMask.dstMask = 0xffffffffffffLL;
    aclCondMask.vlanIdMask = 0xfff;
    aclCondMask.vlanId2Mask = 0xfff;
    aclCondMask.srcIpMask.isIPv6 = FALSE;
    aclCondMask.srcIpMask.addr[0] = 0xffffffff;
    aclCondMask.dstIpMask.isIPv6 = FALSE;
    aclCondMask.dstIpMask.addr[0] = 0xffffffff;
    aclCondMask.protocolMask = 0xff;
    aclCondMask.L4SrcMask = 0xffff;
    aclCondMask.L4DstMask = 0xffff;

    aclAct = FM_ACL_ACTIONEXT_PERMIT;
    for (i = 0 ; i < aclSize ; i++)
    {
        err = fmAddACLRuleExt(sw,
                              aclId,
                              i,
                              aclCond,
                              &aclCondMask,
                              aclAct,
                              &aclActParam);
        if (err != FM_OK)
        {
            fm10000DeleteTunnel(sw, group);
            fmDeleteACLInt(sw, aclId, TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
    }

    err = fmCompileACLExt(sw,
                          statusText,
                          sizeof(statusText),
                          FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                          FM_ACL_COMPILE_FLAG_INTERNAL,
                          (void*) &aclId);
    if (err != FM_OK)
    {
        fm10000DeleteTunnel(sw, group);
        fmDeleteACLInt(sw, aclId, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    err = fmApplyACLExt(sw,
                        FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                        FM_ACL_APPLY_FLAG_INTERNAL,
                        (void*) &aclId);
    if (err != FM_OK)
    {
        fm10000DeleteTunnel(sw, group);
        fmDeleteACLInt(sw, aclId, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    /* Every rule are disabled by default */
    for (i = 0 ; i < aclSize ; i++)
    {
        err = fmSetACLRuleState(sw, aclId, i, FM_ACL_RULE_ENTRY_STATE_INVALID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    natTable = fmAlloc( sizeof(fm_fm10000NatTable) );

    if (natTable == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    FM_CLEAR(*natTable);

    natTable->prefixLimit = (aclSize - 1);
    natTable->acl = aclId;
    natTable->tunnelGrp = group;
    fmTreeInit(&natTable->tunnels);
    fmTreeInit(&natTable->rules);
    fmTreeInit(&natTable->prefilters);
    fmTreeInit(&natTable->prefixs);
    fmCustomTreeInit(&natTable->ruleCondition, fmCompareNatRules);

    err = fmTreeInsert(&switchExt->natCfg->tables, table, natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmCreateBitArray(&natTable->ruleInUse, aclSize);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000CreateNatTable */




/*****************************************************************************/
/** fm10000DeleteNatTable
 * \ingroup intNat
 *
 * \desc            Remove a NAT Table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteNatTable(fm_int sw, fm_int table)
{
    fm_switch *            switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *       switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status              err = FM_OK;
    fm_fm10000NatTable *   natTable;
    fm_char                statusText[1024];
    fm_customTreeIterator  itEntry;
    fm_fm10000NatRuleCond *natRuleCondKey;
    fm_fm10000NatRuleCond *natRuleCondVal;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d, table = %d\n", sw, table);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmDeleteACLInt(sw, natTable->acl, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmCompileACLExt(sw,
                          statusText,
                          sizeof(statusText),
                          FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                          FM_ACL_COMPILE_FLAG_INTERNAL,
                          (void*) &natTable->acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmApplyACLExt(sw,
                        FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                        FM_ACL_APPLY_FLAG_INTERNAL,
                        (void*) &natTable->acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* Iterate through all rules that might uses ECMP Group and remove such
     * group before removing the tunnel. */
    for (fmCustomTreeIterInit(&itEntry, &natTable->ruleCondition) ;
          (err = fmCustomTreeIterNext(&itEntry,
                                     (void**) &natRuleCondKey,
                                     (void**) &natRuleCondVal)) == FM_OK ; )
    {
        if (natRuleCondVal->ecmpGrp >= 0)
        {
            err = fmDeleteECMPGroup(sw, natRuleCondVal->ecmpGrp);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    err = FM_OK;

    err = fm10000DeleteTunnel(sw, natTable->tunnelGrp);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeRemoveCertain(&switchExt->natCfg->tables, table, fmFreeNatTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000DeleteNatTable */




/*****************************************************************************/
/** fm10000SetNatTunnelDefault
 * \ingroup intNat
 *
 * \desc            Configure default tunnel mode and behaviour. This must only
 *                  be configured if NAT rules specifies action
 *                  FM_NAT_ACTION_TUNNEL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelDefault refer to the various tunnel configuration
 *                  to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range
 *                  value.
 *
 *****************************************************************************/
fm_status fm10000SetNatTunnelDefault(fm_int               sw,
                                     fm_natTunnelDefault *tunnelDefault)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT, "sw = %d\n", sw);

    err = SetNatTunnelDefault(sw, 0, tunnelDefault);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = SetNatTunnelDefault(sw, 1, tunnelDefault);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000SetNatTunnelDefault */




/*****************************************************************************/
/** fm10000CreateNatTunnel
 * \ingroup intNat
 *
 * \desc            Create a NAT Tunnel entry. This created entry can be
 *                  referenced using NAT rules action FM_NAT_ACTION_TUNNEL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       tunnel is the tunnel ID.
 *
 * \param[in]       param refer to the tunnel parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_TYPE if configured tunnel type is invalid.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 *
 *****************************************************************************/
fm_status fm10000CreateNatTunnel(fm_int        sw,
                                 fm_int        table,
                                 fm_int        tunnel,
                                 fm_natTunnel *param)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *        switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status               err = FM_OK;
    fm_fm10000NatTable *    natTable;
    fm_tunnelEncapFlow      encapField;
    fm_tunnelEncapFlowParam encapParam;
    fm_fm10000NatTunnel *   natTunnel;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, tunnel = %d\n",
                 sw, table, tunnel);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* SIP and DIP are always defined */
    encapField = FM_TUNNEL_ENCAP_FLOW_SIP;
    encapParam.dip = param->dip;
    encapParam.sip = param->sip;
    encapParam.shared = TRUE;
    encapParam.type = switchPtr->natInfo->tunnelDefault.type;

    if ( (encapParam.type == FM_TUNNEL_TYPE_VXLAN) ||
         (encapParam.type == FM_TUNNEL_TYPE_NGE) )
    {
        if (param->l4Src != FM_NAT_TUNNEL_DEFAULT)
        {
            encapField |= FM_TUNNEL_ENCAP_FLOW_L4SRC;
            encapParam.l4Src = param->l4Src;
        }
    }

    if (param->tos != FM_NAT_TUNNEL_DEFAULT)
    {
        encapField |= FM_TUNNEL_ENCAP_FLOW_TOS;
        encapParam.tos = param->tos;
    }

    if (param->ttl != FM_NAT_TUNNEL_DEFAULT)
    {
        encapField |= FM_TUNNEL_ENCAP_FLOW_TTL;
        encapParam.ttl = param->ttl;
    }

    if (param->l4Dst != FM_NAT_TUNNEL_DEFAULT)
    {
        encapField |= FM_TUNNEL_ENCAP_FLOW_L4DST;
        encapParam.l4Dst = param->l4Dst;
    }

    if ( (encapParam.type == FM_TUNNEL_TYPE_NGE) &&
         (param->ngeMask != FM_NAT_TUNNEL_DEFAULT) )
    {
        encapField |= FM_TUNNEL_ENCAP_FLOW_NGE;
        encapParam.ngeMask = param->ngeMask;
        FM_MEMCPY_S(encapParam.ngeData,
                    sizeof(encapParam.ngeData),
                    param->ngeData,
                    sizeof(param->ngeData));
    }

    err = fm10000AddTunnelEncapFlow(sw,
                                    natTable->tunnelGrp,
                                    tunnel,
                                    encapField,
                                    &encapParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    natTunnel = fmAlloc( sizeof(fm_fm10000NatTunnel) );

    if (natTunnel == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    FM_CLEAR(*natTunnel);

    natTunnel->encapFlow = tunnel;

    err = fmTreeInsert(&natTable->tunnels, tunnel, natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000CreateNatTunnel */




/*****************************************************************************/
/** fm10000DeleteNatTunnel
 * \ingroup intNat
 *
 * \desc            Delete a NAT Tunnel entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       tunnel is the tunnel ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteNatTunnel(fm_int sw, fm_int table, fm_int tunnel)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_fm10000NatTable *natTable;
    fm_fm10000NatTunnel *natTunnel;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, tunnel = %d\n",
                 sw, table, tunnel);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->tunnels, tunnel, (void**) &natTunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fm10000DeleteTunnelEncapFlow(sw,
                                       natTable->tunnelGrp,
                                       natTunnel->encapFlow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeRemoveCertain(&natTable->tunnels,
                              tunnel,
                              fmFree);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000DeleteNatTunnel */




/*****************************************************************************/
/** fm10000AddNatRule
 * \ingroup intNat
 *
 * \desc            Add a NAT rule with the given condition, and action to
 *                  a table. Adding multiple NAT rule with the exact same
 *                  condition but different action will automatically creates
 *                  ECMP Group to hash over the possibility.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       rule is the rule ID.
 *
 * \param[in]       condition is a condition mask (see 'fm_natCondition').
 *
 * \param[in]       cndParam points to the fm_natConditionParam structure
 *                  (see 'fm_natConditionParam') to match against for the
 *                  given condition.
 *
 * \param[in]       action is an action mask (see 'fm_natAction').
 *
 * \param[in]       actParam points to the fm_natActionParam structure
 *                  (see 'fm_natActionParam') to carry extended action
 *                  configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if rule action does not match the table
 *                  configuration.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TABLE_FULL if the table is full.
 * \return          FM_ERR_TUNNEL_CONFLICT if tunnel condition does not match
 *                  hash lookup tunnel group condition.
 * \return          FM_ERR_TUNNEL_COUNT_FULL if no more count resources available.
 * \return          FM_ERR_TUNNEL_FLOW_FULL if no more flow resources available.
 * \return          FM_ERR_TUNNEL_BIN_FULL if no more entries can be inserted into
 *                  that specific hash lookup bin.
 * \return          FM_ERR_TUNNEL_NO_ENCAP_FLOW if specified tunnel is invalid.
 *
 *****************************************************************************/
fm_status fm10000AddNatRule(fm_int                sw,
                            fm_int                table,
                            fm_int                rule,
                            fm_natCondition       condition,
                            fm_natConditionParam *cndParam,
                            fm_natAction          action,
                            fm_natActionParam *   actParam)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *        switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status               err = FM_OK;
    fm_fm10000NatTable *    natTable;
    fm_natTable *           publicNatTable;
    fm_tunnelCondition      tunCond;
    fm_tunnelConditionParam tunCondParam;
    fm_tunnelAction         tunAction;
    fm_tunnelActionParam    tunActParam;
    fm_fm10000NatRule *     natRule;
    fm_aclCondition         aclCond;
    fm_aclValue             aclCondParam;
    fm_aclActionExt         aclAction;
    fm_aclParamExt          aclActParam;
    fm_int                  natIndex;
    fm_fm10000NatRuleCond * natRuleCondKey;
    fm_fm10000NatRuleCond * natRuleCondVal;
    fm_int                  retEcmpGrp;
    fm_ecmpNextHop          ecmpNextHop[2];
    fm_uint64               prefix;
    fm_fm10000NatPrefix *   natPrefix;
    fm_tree *               natRuleTree;
    fm_int                  i;
    fm_int                  aclNumber;
    fm_bool                 referenced;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, rule = %d\n",
                 sw, table, rule);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&switchPtr->natInfo->tables,
                     table,
                     (void **) &publicNatTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    /* Invalid case handling */
    if ( (action & FM_NAT_ACTION_TUNNEL) &&
         ((publicNatTable->natParam.direction == FALSE)||
          (publicNatTable->natParam.tunnel == FALSE)) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    if ( (action & FM_NAT_ACTION_SET_TUNNEL_VSI) &&
         ((publicNatTable->natParam.direction == TRUE)||
          (publicNatTable->natParam.tunnel == FALSE)||
          (action & FM_NAT_ACTION_TUNNEL)) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }

    if (publicNatTable->natParam.mode == FM_NAT_MODE_RESOURCE)
    {
        err = TranslateNatCondToTunCond(condition, &tunCond);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        tunCondParam.vni = cndParam->vni;
        tunCondParam.dmac = cndParam->dmac;
        tunCondParam.smac = cndParam->smac;
        tunCondParam.vlan = cndParam->vlan;
        tunCondParam.dip = cndParam->dstIp;
        tunCondParam.sip = cndParam->srcIp;
        tunCondParam.l4Src = cndParam->l4Src;
        tunCondParam.l4Dst = cndParam->l4Dst;
        tunCondParam.protocol = cndParam->protocol;

        if (condition & FM_NAT_MATCH_SRC_IP)
        {
            /* Force Full IP Masking on resource mode */
            cndParam->srcIpMask.isIPv6 = cndParam->srcIp.isIPv6;
            cndParam->srcIpMask.addr[0] = 0xffffffff;

            if (cndParam->srcIpMask.isIPv6)
            {
                for (i = 1 ; i < 4 ; i++)
                {
                    cndParam->srcIpMask.addr[i] = 0xffffffff;
                }
            }
        }

        if (condition & FM_NAT_MATCH_DST_IP)
        {
            /* Force Full IP Masking on resource mode */
            cndParam->dstIpMask.isIPv6 = cndParam->dstIp.isIPv6;
            cndParam->dstIpMask.addr[0] = 0xffffffff;

            if (cndParam->dstIpMask.isIPv6)
            {
                for (i = 1 ; i < 4 ; i++)
                {
                    cndParam->dstIpMask.addr[i] = 0xffffffff;
                }
            }
        }

        err = TranslateNatActToTunAct(sw,
                                      action,
                                      actParam,
                                      &tunAction,
                                      &tunActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fm10000AddTunnelRule(sw,
                                   natTable->tunnelGrp,
                                   rule,
                                   tunCond,
                                   &tunCondParam,
                                   tunAction,
                                   &tunActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        natRule = fmAlloc( sizeof(fm_fm10000NatRule) );

        if (natRule == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        natRule->aclRule = -1;
        natRule->tunnelRule = rule;

        err = fmTreeInsert(&natTable->rules, rule, natRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    /* FM_NAT_MODE_PERFORMANCE */
    else
    {
        err = fmFindBitInBitArray(&natTable->ruleInUse,
                                  0,
                                  FALSE,
                                  &natIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if (natIndex < 0)
        {
            err = FM_ERR_TABLE_FULL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        err = TranslateNatCondToAclCond(condition, &aclCond);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if ( (condition | publicNatTable->natParam.ruleCond) !=
              publicNatTable->natParam.ruleCond)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        aclCondParam.src = cndParam->smac;
        aclCondParam.srcMask = 0xFFFFFFFFFFFFLL;
        aclCondParam.dst = cndParam->dmac;
        aclCondParam.dstMask = 0xFFFFFFFFFFFFLL;
        aclCondParam.vlanId = cndParam->vlan;
        aclCondParam.vlanIdMask = 0xFFF;
        aclCondParam.vlanId2 = cndParam->vlan2;
        aclCondParam.vlanId2Mask = 0xFFF;
        aclCondParam.protocol = cndParam->protocol;
        aclCondParam.protocolMask = 0xFF;
        aclCondParam.L4SrcStart = cndParam->l4Src;
        aclCondParam.L4SrcMask = 0xFFFF;
        aclCondParam.L4DstStart = cndParam->l4Dst;
        aclCondParam.L4DstMask = 0xFFFF;
        aclCondParam.logicalPort = cndParam->logicalPort;

        /* Force IPv4 Matching and apply Mask */
        if (condition & FM_NAT_MATCH_SRC_IP)
        {
            /* Can't match on IPv6 for now */
            if (cndParam->srcIp.isIPv6 || cndParam->srcIpMask.isIPv6)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            cndParam->srcIp.addr[0] = cndParam->srcIp.addr[0] &
                                      cndParam->srcIpMask.addr[0];
        }
        if (condition & FM_NAT_MATCH_DST_IP)
        {
            /* Can't match on IPv6 for now */
            if (cndParam->dstIp.isIPv6 || cndParam->dstIpMask.isIPv6)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            cndParam->dstIp.addr[0] = cndParam->dstIp.addr[0] &
                                      cndParam->dstIpMask.addr[0];
        }

        aclCondParam.srcIp = cndParam->srcIp;
        aclCondParam.srcIpMask = cndParam->srcIpMask;
        aclCondParam.dstIp = cndParam->dstIp;
        aclCondParam.dstIpMask = cndParam->dstIpMask;

        err = TranslateNatActToTunAct(sw,
                                      action,
                                      actParam,
                                      &tunAction,
                                      &tunActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fm10000AddTunnelRule(sw,
                                   natTable->tunnelGrp,
                                   rule,
                                   0,
                                   &tunCondParam,
                                   tunAction,
                                   &tunActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        natRuleCondKey = fmAlloc( sizeof(fm_fm10000NatRuleCond) );

        if (natRuleCondKey == NULL)
        {
            fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        FM_CLEAR(*natRuleCondKey);

        natRuleCondKey->condition = condition;
        natRuleCondKey->cndParam = *cndParam;

        err = fmCustomTreeFind(&natTable->ruleCondition,
                               natRuleCondKey,
                               (void**) &natRuleCondVal);
        /* Add this action to the current ECMP Group (or create it) */
        if (err == FM_OK)
        {
            /* Node already part of the tree */
            fmFree(natRuleCondKey);

            /* Add this tunnel rule to the current ecmp group */
            if (natRuleCondVal->ecmpGrp >= 0)
            {
                ecmpNextHop[0].type = FM_NEXTHOP_TYPE_TUNNEL;
                ecmpNextHop[0].data.tunnel.tunnelGrp = natTable->tunnelGrp;
                ecmpNextHop[0].data.tunnel.tunnelRule = rule;
                err = fmAddECMPGroupNextHopsV2(sw,
                                               natRuleCondVal->ecmpGrp,
                                               1,
                                               ecmpNextHop);
                if (err != FM_OK)
                {
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                /* Routing Lock is required to block either Routing or ACL
                 * from updating the ACL Ecmp Mapping structure. No need to
                 * take the ACL Lock since Routing Lock is always required
                 * to do any modification. */
                fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
                /* Find the acl rule that refer to this ecmp group */
                err = fm10000ValidateAclEcmpId(sw,
                                               natRuleCondVal->ecmpGrp,
                                               &referenced,
                                               &aclNumber,
                                               &natIndex);
                fmReleaseWriteLock(&switchPtr->routingLock);
                if (err != FM_OK)
                {
                    fmDeleteECMPGroupNextHopsV2(sw,
                                                natRuleCondVal->ecmpGrp,
                                                1,
                                                ecmpNextHop);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                /* Only one rule must refer to this ecmp group and it must be
                 * under this specific ACL. */
                if ( (referenced == FALSE) ||
                     (aclNumber != natTable->acl) )
                {
                    fmDeleteECMPGroupNextHopsV2(sw,
                                                natRuleCondVal->ecmpGrp,
                                                1,
                                                ecmpNextHop);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    err = FM_FAIL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }
            }
            /* Move from single entry to a group */
            else
            {
                err = fmTreeFind(&natTable->rules,
                                 natRuleCondVal->rule,
                                 (void**) &natRule);
                if (err != FM_OK)
                {
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                err = fmCreateECMPGroupV2(sw, &retEcmpGrp, NULL);
                if (err != FM_OK)
                {
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                /* Add previously defined rule first */
                ecmpNextHop[0].type = FM_NEXTHOP_TYPE_TUNNEL;
                ecmpNextHop[0].data.tunnel.tunnelGrp = natTable->tunnelGrp;
                ecmpNextHop[0].data.tunnel.tunnelRule = natRuleCondVal->rule;

                /* Add new rule after */
                ecmpNextHop[1].type = FM_NEXTHOP_TYPE_TUNNEL;
                ecmpNextHop[1].data.tunnel.tunnelGrp = natTable->tunnelGrp;
                ecmpNextHop[1].data.tunnel.tunnelRule = rule;
                err = fmAddECMPGroupNextHopsV2(sw,
                                               retEcmpGrp,
                                               2,
                                               ecmpNextHop);
                if (err != FM_OK)
                {
                    fmDeleteECMPGroup(sw, retEcmpGrp);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                aclAction = FM_ACL_ACTIONEXT_ROUTE;
                aclActParam.groupId = retEcmpGrp;

                err = FindProperNatIndex(sw,
                                         natTable,
                                         &natIndex,
                                         aclCond,
                                         &aclCondParam);
                if (err != FM_OK)
                {
                    fmDeleteECMPGroup(sw, retEcmpGrp);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                /* Use this new position to insert the updated entry that
                 * include this ECMP group. */
                err = fmUpdateACLRule(sw,
                                      natTable->acl,
                                      natIndex,
                                      aclCond,
                                      &aclCondParam,
                                      aclAction,
                                      &aclActParam);
                if (err != FM_OK)
                {
                    fmDeleteECMPGroup(sw, retEcmpGrp);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                err = fmSetACLRuleState(sw,
                                        natTable->acl,
                                        natIndex,
                                        FM_ACL_RULE_ENTRY_STATE_VALID);
                if (err != FM_OK)
                {
                    fmDeleteECMPGroup(sw, retEcmpGrp);
                    fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                /* Update the tree with the ECMP Group instead of the rule */
                natRuleCondVal->ecmpGrp = retEcmpGrp;
                natRuleCondVal->rule = -1;

                err = fmSetBitArrayBit(&natTable->ruleInUse,
                                       natIndex,
                                       TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                /* Remove the original rule ACL usage */
                err = fmSetACLRuleState(sw,
                                        natTable->acl,
                                        natRule->aclRule,
                                        FM_ACL_RULE_ENTRY_STATE_INVALID);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                err = fmUpdateACLRule(sw,
                                      natTable->acl,
                                      natRule->aclRule,
                                      0,
                                      &aclCondParam,
                                      FM_ACL_ACTIONEXT_PERMIT,
                                      &aclActParam);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                err = fmSetBitArrayBit(&natTable->ruleInUse,
                                       natRule->aclRule,
                                       FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                /* Update the prefix tree if needed */
                if ( (((aclCond & FM_ACL_MATCH_SRC_IP) != 0) &&
                      (aclCondParam.srcIpMask.addr[0] != 0xFFFFFFFF)) ||
                     (((aclCond & FM_ACL_MATCH_DST_IP) != 0) &&
                      (aclCondParam.dstIpMask.addr[0] != 0xFFFFFFFF)))
                {
                    /* Compute the prefix */
                    prefix = 0LL;
                    if (aclCond & FM_ACL_MATCH_DST_IP)
                    {
                        prefix |= ((fm_uint64)ntohl(aclCondParam.dstIpMask.addr[0]) << 32);
                    }

                    if (aclCond & FM_ACL_MATCH_SRC_IP)
                    {
                        prefix |= (fm_uint64)ntohl(aclCondParam.srcIpMask.addr[0]);
                    }

                    err = fmTreeFind(&natTable->prefixs, prefix, (void**)&natPrefix);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                    err = fmTreeFind(&natPrefix->aclRule, natRule->aclRule, (void**)&natRuleTree);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                    err = fmTreeInsert(&natPrefix->aclRule, natIndex, natRuleTree);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                    err = fmTreeRemoveCertain(&natPrefix->aclRule, natRule->aclRule, NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                }

                natRule->aclRule = natIndex;
            }
        }
        /* New entry */
        else if (err == FM_ERR_NOT_FOUND)
        {
            aclAction = FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
            aclActParam.tunnelGroup = natTable->tunnelGrp;
            aclActParam.tunnelRule = rule;

            err = FindProperNatIndex(sw,
                                     natTable,
                                     &natIndex,
                                     aclCond,
                                     &aclCondParam);
            if (err != FM_OK)
            {
                fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                fmFree(natRuleCondKey);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }

            err = fmUpdateACLRule(sw,
                                  natTable->acl,
                                  natIndex,
                                  aclCond,
                                  &aclCondParam,
                                  aclAction,
                                  &aclActParam);
            if (err != FM_OK)
            {
                fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                fmFree(natRuleCondKey);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }

            err = fmSetACLRuleState(sw,
                                    natTable->acl,
                                    natIndex,
                                    FM_ACL_RULE_ENTRY_STATE_VALID);
            if (err != FM_OK)
            {
                fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
                fmFree(natRuleCondKey);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }

            natRuleCondKey->ecmpGrp = -1;
            natRuleCondKey->rule = rule;

            err = fmCustomTreeInsert(&natTable->ruleCondition,
                                     natRuleCondKey,
                                     (void*) natRuleCondKey);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            err = fmSetBitArrayBit(&natTable->ruleInUse,
                                   natIndex,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }
        /* Unhandled error */
        else
        {
            fm10000DeleteTunnelRule(sw, natTable->tunnelGrp, rule);
            fmFree(natRuleCondKey);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        natRule = fmAlloc( sizeof(fm_fm10000NatRule) );

        if (natRule == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        natRule->aclRule = natIndex;
        natRule->tunnelRule = rule;

        err = fmTreeInsert(&natTable->rules, rule, natRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        /* Only non full SIP and/or DIP masking are inserted into the prefixs
         * tree. */
        if ( (((aclCond & FM_ACL_MATCH_SRC_IP) != 0) &&
              (aclCondParam.srcIpMask.addr[0] != 0xFFFFFFFF)) ||
             (((aclCond & FM_ACL_MATCH_DST_IP) != 0) &&
              (aclCondParam.dstIpMask.addr[0] != 0xFFFFFFFF)))
        {
            /* Compute the prefix */
            prefix = 0LL;
            if (aclCond & FM_ACL_MATCH_DST_IP)
            {
                prefix |= ((fm_uint64)ntohl(aclCondParam.dstIpMask.addr[0]) << 32);
            }

            if (aclCond & FM_ACL_MATCH_SRC_IP)
            {
                prefix |= (fm_uint64)ntohl(aclCondParam.srcIpMask.addr[0]);
            }

            err = fmTreeFind(&natTable->prefixs, prefix, (void**)&natPrefix);
            if (err == FM_ERR_NOT_FOUND)
            {
                /* Prefix does not exist, create it */
                natPrefix = fmAlloc( sizeof(fm_fm10000NatPrefix) );

                if (natPrefix == NULL)
                {
                    err = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                fmTreeInit(&natPrefix->aclRule);

                err = fmTreeInsert(&natTable->prefixs, prefix, natPrefix);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            err = fmTreeFind(&natPrefix->aclRule, natIndex, (void**)&natRuleTree);
            if (err == FM_ERR_NOT_FOUND)
            {
                /* ACL Rule does not exist, create it */
                natRuleTree = fmAlloc( sizeof(fm_tree) );

                if (natRuleTree == NULL)
                {
                    err = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }

                fmTreeInit(natRuleTree);

                err = fmTreeInsert(&natPrefix->aclRule, natIndex, natRuleTree);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            err = fmTreeInsert(natRuleTree, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        }
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000AddNatRule */




/*****************************************************************************/
/** fm10000DeleteNatRule
 * \ingroup intNat
 *
 * \desc            Remove a NAT rule from the table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       rule is the rule ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteNatRule(fm_int sw, fm_int table, fm_int rule)
{
    fm_switch *            switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *       switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status              err = FM_OK;
    fm_fm10000NatTable *   natTable;
    fm_natTable *          publicNatTable;
    fm_natRule *           publicNatRule;
    fm_fm10000NatRule *    natRule;
    fm_aclCondition        aclCond;
    fm_aclValue            aclCondParam;
    fm_aclActionExt        aclAction;
    fm_aclParamExt         aclActParam;
    fm_fm10000NatRuleCond  natRuleCondKey;
    fm_fm10000NatRuleCond *natRuleCondVal;
    fm_int                 firstTunnel;
    fm_nextHop             firstNextHop;
    fm_ecmpNextHop         ecmpNextHop;
    fm_bool                lastRule;
    fm_uint64              prefix;
    fm_fm10000NatPrefix *  natPrefix;
    fm_tree *              natRuleTree;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, rule = %d\n",
                 sw, table, rule);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&switchPtr->natInfo->tables,
                     table,
                     (void **) &publicNatTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules,
                     rule,
                     (void **) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    if (publicNatTable->natParam.mode == FM_NAT_MODE_RESOURCE)
    {
        err = fm10000DeleteTunnelRule(sw,
                                      natTable->tunnelGrp,
                                      natRule->tunnelRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeRemoveCertain(&natTable->rules, rule, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    /* FM_NAT_MODE_PERFORMANCE */
    else
    {
        err = fmTreeFind(&publicNatTable->rules,
                         rule,
                         (void **) &publicNatRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        natRuleCondKey.condition = publicNatRule->condition;
        natRuleCondKey.cndParam = publicNatRule->cndParam;

        err = fmCustomTreeFind(&natTable->ruleCondition,
                               &natRuleCondKey,
                               (void**) &natRuleCondVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        /* Indicate if the rule must be removed */
        lastRule = TRUE;

        if (natRuleCondVal->ecmpGrp >= 0)
        {
            ecmpNextHop.type = FM_NEXTHOP_TYPE_TUNNEL;
            ecmpNextHop.data.tunnel.tunnelGrp = natTable->tunnelGrp;
            ecmpNextHop.data.tunnel.tunnelRule = natRule->tunnelRule;

            err = fmDeleteECMPGroupNextHopsV2(sw,
                                              natRuleCondVal->ecmpGrp,
                                              1,
                                              &ecmpNextHop);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Delete the ECMP Group if this was the last NextHop */
            err = fmGetECMPGroupNextHopFirst(sw,
                                             natRuleCondVal->ecmpGrp,
                                             &firstTunnel,
                                             &firstNextHop);
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            /* FM_OK */
            else
            {
                /* Other rule(s) share this ECMP Group */
                lastRule = FALSE;
            }
        }

        if (lastRule)
        {
            /* Deletion of a rule is performed by updating the state but also
             * clearing the rule to remove the ECMP node from the ACL tree. */
            err = fmSetACLRuleState(sw,
                                    natTable->acl,
                                    natRule->aclRule,
                                    FM_ACL_RULE_ENTRY_STATE_INVALID);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            aclCond = 0;
            aclAction = FM_ACL_ACTIONEXT_PERMIT;
            err = fmUpdateACLRule(sw,
                                  natTable->acl,
                                  natRule->aclRule,
                                  aclCond,
                                  &aclCondParam,
                                  aclAction,
                                  &aclActParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Can't delete an ECMP group while it is currently used */
            if (natRuleCondVal->ecmpGrp >= 0)
            {
                err = fmDeleteECMPGroup(sw, natRuleCondVal->ecmpGrp);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
        }

        err = fm10000DeleteTunnelRule(sw,
                                      natTable->tunnelGrp,
                                      natRule->tunnelRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if (lastRule)
        {
            err = fmSetBitArrayBit(&natTable->ruleInUse,
                                   natRule->aclRule,
                                   FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Only remove it from the condition tree if this is the last
             * rule of the group. */
            err = fmCustomTreeRemoveCertain(&natTable->ruleCondition,
                                            &natRuleCondKey,
                                            fmFreeNatRuleCond);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        /* Only non full SIP and/or DIP masking are inserted into the prefixs
         * tree. */
        if ( (((publicNatRule->condition & FM_NAT_MATCH_SRC_IP) != 0) &&
              (publicNatRule->cndParam.srcIpMask.addr[0] != 0xFFFFFFFF)) ||
             (((publicNatRule->condition & FM_NAT_MATCH_DST_IP) != 0) &&
              (publicNatRule->cndParam.dstIpMask.addr[0] != 0xFFFFFFFF)))
        {
            /* Compute the prefix */
            prefix = 0LL;
            if (publicNatRule->condition & FM_NAT_MATCH_DST_IP)
            {
                prefix |= ((fm_uint64)ntohl(publicNatRule->cndParam.dstIpMask.addr[0]) << 32);
            }

            if (publicNatRule->condition & FM_NAT_MATCH_SRC_IP)
            {
                prefix |= (fm_uint64)ntohl(publicNatRule->cndParam.srcIpMask.addr[0]);
            }

            err = fmTreeFind(&natTable->prefixs, prefix, (void**)&natPrefix);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            err = fmTreeFind(&natPrefix->aclRule,
                             natRule->aclRule,
                             (void**)&natRuleTree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            err = fmTreeRemove(natRuleTree,
                               rule,
                               NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

            /* Remove the nat rule tree if this rule was the last one that
             * belongs to this ACL rule. */
            if (fmTreeSize(natRuleTree) == 0)
            {
                err = fmTreeRemoveCertain(&natPrefix->aclRule,
                                          natRule->aclRule,
                                          fmFreeNatPrefixRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

                /* If the prefix is now empty, remove it. */
                if (fmTreeSize(&natPrefix->aclRule) == 0)
                {
                    err = fmTreeRemoveCertain(&natTable->prefixs,
                                              prefix,
                                              fmFreeNatPrefix);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
                }
            }
        }

        err = fmTreeRemoveCertain(&natTable->rules, rule, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000DeleteNatRule */




/*****************************************************************************/
/** fm10000AddNatPrefilter
 * \ingroup intNat
 *
 * \desc            Add a NAT prefilter entry with the given condition to
 *                  a table. This service is only available on table configured
 *                  with NAT mode ''FM_NAT_MODE_RESOURCE''. Any of the
 *                  prefilter entry hit will initiate a lookup in the second
 *                  part of the table populated with NAT rules.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       entry is the prefilter ID.
 *
 * \param[in]       condition is a condition mask (see 'fm_natCondition').
 *
 * \param[in]       cndParam points to the fm_natConditionParam structure
 *                  (see 'fm_natConditionParam') to match against for the
 *                  given condition.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if rule action does not match the table
 *                  configuration.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TABLE_FULL if the table is full.
 *
 *****************************************************************************/
fm_status fm10000AddNatPrefilter(fm_int                sw,
                                 fm_int                table,
                                 fm_int                entry,
                                 fm_natCondition       condition,
                                 fm_natConditionParam *cndParam)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *        switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status               err = FM_OK;
    fm_fm10000NatTable *    natTable;
    fm_natTable *           publicNatTable;
    fm_fm10000NatPrefilter *natPrefilter;
    fm_aclCondition         aclCond;
    fm_aclValue             aclCondParam;
    fm_aclActionExt         aclAction;
    fm_aclParamExt          aclActParam;
    fm_int                  natIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, entry = %d\n",
                 sw, table, entry);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&switchPtr->natInfo->tables,
                     table,
                     (void **) &publicNatTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    if (publicNatTable->natParam.mode == FM_NAT_MODE_RESOURCE)
    {
        /* Prefilter rules are not sorted in any way so this pick the first
         * slot available and use it. */
        err = fmFindBitInBitArray(&natTable->ruleInUse,
                                  0,
                                  FALSE,
                                  &natIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if (natIndex < 0)
        {
            err = FM_ERR_TABLE_FULL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        err = TranslateNatCondToAclCond(condition, &aclCond);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        if ( (condition | publicNatTable->natParam.preFilterCond) !=
              publicNatTable->natParam.preFilterCond)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        aclCondParam.src = cndParam->smac;
        aclCondParam.srcMask = 0xFFFFFFFFFFFFLL;
        aclCondParam.dst = cndParam->dmac;
        aclCondParam.dstMask = 0xFFFFFFFFFFFFLL;
        aclCondParam.vlanId = cndParam->vlan;
        aclCondParam.vlanIdMask = 0xFFF;
        aclCondParam.vlanId2 = cndParam->vlan2;
        aclCondParam.vlanId2Mask = 0xFFF;
        aclCondParam.protocol = cndParam->protocol;
        aclCondParam.protocolMask = 0xFF;
        aclCondParam.L4SrcStart = cndParam->l4Src;
        aclCondParam.L4SrcMask = 0xFFFF;
        aclCondParam.L4DstStart = cndParam->l4Dst;
        aclCondParam.L4DstMask = 0xFFFF;
        aclCondParam.logicalPort = cndParam->logicalPort;

        /* Force IPv4 Matching and apply Mask */
        if (condition & FM_NAT_MATCH_SRC_IP)
        {
            /* Can't match on IPv6 for now */
            if (cndParam->srcIp.isIPv6 || cndParam->srcIpMask.isIPv6)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            cndParam->srcIp.addr[0] = cndParam->srcIp.addr[0] &
                                      cndParam->srcIpMask.addr[0];
        }
        if (condition & FM_NAT_MATCH_DST_IP)
        {
            /* Can't match on IPv6 for now */
            if (cndParam->dstIp.isIPv6 || cndParam->dstIpMask.isIPv6)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
            }
            cndParam->dstIp.addr[0] = cndParam->dstIp.addr[0] &
                                      cndParam->dstIpMask.addr[0];
        }

        aclCondParam.srcIp = cndParam->srcIp;
        aclCondParam.srcIpMask = cndParam->srcIpMask;
        aclCondParam.dstIp = cndParam->dstIp;
        aclCondParam.dstIpMask = cndParam->dstIpMask;

        aclAction = FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
        aclActParam.tunnelGroup = natTable->tunnelGrp;
        aclActParam.tunnelRule = 0;

        err = fmUpdateACLRule(sw,
                              natTable->acl,
                              natIndex,
                              aclCond,
                              &aclCondParam,
                              aclAction,
                              &aclActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmSetACLRuleState(sw,
                                natTable->acl,
                                natIndex,
                                FM_ACL_RULE_ENTRY_STATE_VALID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmSetBitArrayBit(&natTable->ruleInUse,
                               natIndex,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        natPrefilter = fmAlloc( sizeof(fm_fm10000NatPrefilter) );

        if (natPrefilter == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
        }

        natPrefilter->aclRule = natIndex;

        err = fmTreeInsert(&natTable->prefilters, entry, natPrefilter);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    /* FM_NAT_MODE_PERFORMANCE */
    else
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000AddNatPrefilter */




/*****************************************************************************/
/** fm10000DeleteNatPrefilter
 * \ingroup intNat
 *
 * \desc            Delete a NAT prefilter entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       entry is the prefilter ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteNatPrefilter(fm_int sw, fm_int table, fm_int entry)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *        switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status               err = FM_OK;
    fm_fm10000NatTable *    natTable;
    fm_natTable *           publicNatTable;
    fm_fm10000NatPrefilter *natPrefilter;
    fm_aclCondition         aclCond;
    fm_aclValue             aclCondParam;
    fm_aclActionExt         aclAction;
    fm_aclParamExt          aclActParam;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, entry = %d\n",
                 sw, table, entry);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&switchPtr->natInfo->tables,
                     table,
                     (void **) &publicNatTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->prefilters,
                     entry,
                     (void **) &natPrefilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    if (publicNatTable->natParam.mode == FM_NAT_MODE_RESOURCE)
    {
        err = fmSetACLRuleState(sw,
                                natTable->acl,
                                natPrefilter->aclRule,
                                FM_ACL_RULE_ENTRY_STATE_INVALID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        aclCond = 0;
        aclAction = FM_ACL_ACTIONEXT_PERMIT;
        err = fmUpdateACLRule(sw,
                              natTable->acl,
                              natPrefilter->aclRule,
                              aclCond,
                              &aclCondParam,
                              aclAction,
                              &aclActParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmSetBitArrayBit(&natTable->ruleInUse,
                               natPrefilter->aclRule,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

        err = fmTreeRemoveCertain(&natTable->prefilters, entry, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }
    /* FM_NAT_MODE_PERFORMANCE */
    else
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000DeleteNatPrefilter */




/*****************************************************************************/
/** fm10000GetNatRuleCount
 * \ingroup intNat
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  ''FM_NAT_ACTION_COUNT'' NAT rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       rule is the rule ID.
 *
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_tunnelCounters'' where this function should place the
 *                  counter values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have count action.
 *
 *****************************************************************************/
fm_status fm10000GetNatRuleCount(fm_int             sw,
                                 fm_int             table,
                                 fm_int             rule,
                                 fm_tunnelCounters *counters)
{
    fm_switch *         switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *    switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status           err = FM_OK;
    fm_fm10000NatTable *natTable;
    fm_fm10000NatRule * natRule;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, rule = %d\n",
                 sw, table, rule);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules,
                     rule,
                     (void **) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fm10000GetTunnelRuleCount(sw,
                                    natTable->tunnelGrp,
                                    natRule->tunnelRule,
                                    counters);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000GetNatRuleCount */




/*****************************************************************************/
/** fm10000ResetNatRuleCount
 * \ingroup intNat
 *
 * \desc            Reset the frame and octet counts associated with an
 *                  ''FM_NAT_ACTION_COUNT'' NAT rule action.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       table is the table ID.
 *
 * \param[in]       rule is the rule ID.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_NO_COUNT if the rule does not have count action.
 *
 *****************************************************************************/
fm_status fm10000ResetNatRuleCount(fm_int sw, fm_int table, fm_int rule)
{
    fm_switch *         switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *    switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status           err = FM_OK;
    fm_fm10000NatTable *natTable;
    fm_fm10000NatRule * natRule;

    FM_LOG_ENTRY(FM_LOG_CAT_NAT,
                 "sw = %d, table = %d, rule = %d\n",
                 sw, table, rule);

    err = fmTreeFind(&switchExt->natCfg->tables, table, (void**) &natTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fmTreeFind(&natTable->rules,
                     rule,
                     (void **) &natRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);

    err = fm10000ResetTunnelRuleCount(sw,
                                      natTable->tunnelGrp,
                                      natRule->tunnelRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_NAT, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_NAT, err);

}   /* end fm10000ResetNatRuleCount */
