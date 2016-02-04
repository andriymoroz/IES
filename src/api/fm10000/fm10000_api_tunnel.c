/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_tunnel.c
 * Creation Date:   January 15, 2014
 * Description:     FM10000 Tunnel API.
 *
 * Copyright (c) 2014 - 2016, Intel Corporation
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

/* Define TE_DATA maximum block size in different representation */
#define FM10000_TE_DATA_LENGTH_SIZE     FM_FIELD_UNSIGNED_MAX(FM10000_TE_LOOKUP, DataLength)
#define FM10000_TE_DATA_LENGTH_SIZE_32 (FM10000_TE_DATA_LENGTH_SIZE * FM10000_TE_DATA_WIDTH)

#define BUFFER_SIZE                     1024


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status DefragTeData(fm_int     sw,
                              fm_int     te,
                              fm_uint16  size,
                              fm_uint16 *index);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** PrintLine
 * \ingroup intTunnel
 *
 * \desc            Print range style debug line
 *
 * \param[in]       start is the first limit of the range.
 *
 * \param[in]       end is the last limit of the range.
 *
 * \param[in]       line refer to the buffer to append.
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintLine(fm_uint start, fm_uint end, const char *line)
{
    if (start == end)
    {
        FM_LOG_PRINT("      %05u: %s\n", start, line);
    }
    else
    {
        FM_LOG_PRINT("%05u-%05u: %s\n", start, end, line);
    }
}




/*****************************************************************************/
/** fmFreeLookupBin
 * \ingroup intTunnel
 *
 * \desc            Free a fm_fm10000TunnelLookupBin structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeLookupBin(void *value)
{
    fm_fm10000TunnelLookupBin *lookupBin = (fm_fm10000TunnelLookupBin*)value;
    if (lookupBin != NULL)
    {
        fmTreeDestroy(&lookupBin->rules, NULL);
        fmFree(value);
    }

}   /* end fmFreeLookupBin */




/*****************************************************************************/
/** fmFreeEncapFlow
 * \ingroup intTunnel
 *
 * \desc            Free a fm_fm10000EncapFlow structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeEncapFlow(void *value)
{
    fm_fm10000EncapFlow *encapFlow = (fm_fm10000EncapFlow*)value;
    if (encapFlow != NULL)
    {
        fmTreeDestroy(&encapFlow->rules, NULL);
        fmFree(value);
    }

}   /* end fmFreeEncapFlow */




/*****************************************************************************/
/** FreeTunnelCfgStruct
 * \ingroup intTunnel
 *
 * \desc            Free a fm_fm10000TunnelCfg structure.
 *
 * \param[in,out]   tunnelCfg points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FreeTunnelCfgStruct(fm_fm10000TunnelCfg *tunnelCfg)
{
    fm_int te;
    fm_int index;

    if (tunnelCfg)
    {
        for (te = 0 ; te < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; te++)
        {
            for (index = 0 ; index < FM10000_TE_DGLORT_MAP_ENTRIES_0 ; index++)
            {
                fmTreeDestroy(&tunnelCfg->tunnelGrp[te][index].rules, fmFree);
                fmTreeDestroy(&tunnelCfg->tunnelGrp[te][index].encapFlows,
                              fmFreeEncapFlow);
                fmTreeDestroy(&tunnelCfg->tunnelGrp[te][index].lookupBins,
                              fmFreeLookupBin);
            }

            fmDeleteBitArray(&tunnelCfg->cntInUse[te]);

            for (index = 0 ; index < FM10000_TE_DATA_ENTRIES_0 ; index++)
            {
                if (tunnelCfg->teDataCtrl[te].teDataBlkCtrl[index])
                {
                    fmFree(tunnelCfg->teDataCtrl[te].teDataBlkCtrl[index]);
                }
            }
        }

        fmFree(tunnelCfg);
    }

}   /* end FreeTunnelCfgStruct */




/*****************************************************************************/
/** InitializeTunnelCfgStruct
 * \ingroup intTunnel
 *
 * \desc            Initialize a fm_fm10000TunnelCfg structure.
 *
 * \param[in,out]   tunnelCfg points to the structure to initialize.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitializeTunnelCfgStruct(fm_fm10000TunnelCfg *tunnelCfg)
{
    fm_status err;
    fm_int te;
    fm_int index;

    for (te = 0 ; te < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; te++)
    {
        tunnelCfg->tunnelPort[te] = -1;

        for (index = 0 ; index < FM10000_TE_DGLORT_MAP_ENTRIES_0 ; index++)
        {
            fmTreeInit(&tunnelCfg->tunnelGrp[te][index].rules);
            fmTreeInit(&tunnelCfg->tunnelGrp[te][index].encapFlows);
            fmTreeInit(&tunnelCfg->tunnelGrp[te][index].lookupBins);
        }

        err = fmCreateBitArray(&tunnelCfg->cntInUse[te],
                               FM10000_TE_STATS_ENTRIES_0);
        if (err != FM_OK)
        {
            return err;
        }

        tunnelCfg->teDataCtrl[te].teDataSwapSize = FM10000_TUNNEL_TE_DATA_MIN_SWAP_SIZE;
        /* Entry 0 is not usable for both table. */
        tunnelCfg->teDataCtrl[te].teDataFreeEntryCount = FM10000_TE_DATA_ENTRIES_0 -
            (tunnelCfg->teDataCtrl[te].teDataSwapSize + 1);
        tunnelCfg->teDataCtrl[te].teDataHandlerFirstFreeEntry = 1;
        tunnelCfg->teDataCtrl[te].lastTeDataBlkCtrlIndex = 0;
    }

    return err;

}   /* end InitializeTunnelCfgStruct */




/*****************************************************************************/
/** TunnelToTeHashKey
 * \ingroup intTunnel
 *
 * \desc            Translate tunnel hash key to te one.
 *
 * \param[in]       hashKey is the tunnel hash key to translate
 *
 * \param[in]       hashParam is a pointer to the parameters. Set to NULL
 *                  if unknown.
 *
 * \return          Translated hash key from tunnel to te representation.
 *
 *****************************************************************************/
static fm_uint16 TunnelToTeHashKey(fm_tunnelCondition hashKey,
                                   fm_tunnelConditionParam const * const hashParam)
{
    fm_uint16 transKey = 0;

    if (hashKey & FM_TUNNEL_MATCH_VSI_TEP)
    {
        transKey |= FM10000_TE_KEY_VSI_TEP;
    }

    if (hashKey & FM_TUNNEL_MATCH_VNI)
    {
        transKey |= FM10000_TE_KEY_VNI;
    }

    if (hashKey & FM_TUNNEL_MATCH_DMAC)
    {
        transKey |= FM10000_TE_KEY_DMAC;
    }

    if (hashKey & FM_TUNNEL_MATCH_SMAC)
    {
        transKey |= FM10000_TE_KEY_SMAC;
    }

    if (hashKey & FM_TUNNEL_MATCH_VLAN)
    {
        transKey |= FM10000_TE_KEY_VLAN;
    }

    if (hashKey & FM_TUNNEL_MATCH_DIP)
    {
        transKey |= FM10000_TE_KEY_DIP;

        if ( (hashParam != NULL) &&
             (hashParam->dip.isIPv6) )
        {
            transKey |= FM10000_TE_KEY_IPV6;
        }
    }

    if (hashKey & FM_TUNNEL_MATCH_SIP)
    {
        transKey |= FM10000_TE_KEY_SIP;

        if ( (hashParam != NULL) &&
             (hashParam->sip.isIPv6) )
        {
            transKey |= FM10000_TE_KEY_IPV6;
        }
    }

    if (hashKey & FM_TUNNEL_MATCH_L4SRC)
    {
        transKey |= FM10000_TE_KEY_L4SRC;
    }

    if (hashKey & FM_TUNNEL_MATCH_L4DST)
    {
        transKey |= FM10000_TE_KEY_L4DST;
    }

    if (hashKey & FM_TUNNEL_MATCH_PROT)
    {
        transKey |= FM10000_TE_KEY_PROT;
    }

    return transKey;

}   /* end TunnelToTeHashKey */




/*****************************************************************************/
/** EncapFlowToTeData
 * \ingroup intTunnel
 *
 * \desc            Translate tunnel encap flow to low level teData.
 *
 * \param[in]       field is the action bitmask
 *
 * \param[in]       param refer to the value of each field.
 *
 * \param[out]      teData refer to the teData structure to fill.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status EncapFlowToTeData(fm_tunnelEncapFlow       field,
                                   fm_tunnelEncapFlowParam *param,
                                   fm_fm10000TeData *       teData)
{
    fm_status err;

    err = FM_OK;

    teData->blockType = FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA;

    switch (param->type)
    {
        case FM_TUNNEL_TYPE_VXLAN:
            teData->blockVal.tunnelVal.tunnelType = FM_FM10000_TE_TUNNEL_TYPE_VXLAN;
            break;

        case FM_TUNNEL_TYPE_NGE:
            teData->blockVal.tunnelVal.tunnelType = FM_FM10000_TE_TUNNEL_TYPE_NGE;
            break;

        case FM_TUNNEL_TYPE_NVGRE:
            teData->blockVal.tunnelVal.tunnelType = FM_FM10000_TE_TUNNEL_TYPE_NVGRE;
            break;

        case FM_TUNNEL_TYPE_GPE:
        case FM_TUNNEL_TYPE_GPE_NSH:
            teData->blockVal.tunnelVal.tunnelType = FM_FM10000_TE_TUNNEL_TYPE_GENERIC;
            break;

        default:
            return FM_ERR_TUNNEL_TYPE;
    }

    teData->blockVal.tunnelVal.tunnelConfig = 0;
    teData->blockVal.tunnelVal.dip = param->dip;

    if (param->dip.isIPv6)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_IPV6;
    }

    teData->blockVal.tunnelVal.counterIdx = 0;

    if (field & FM_TUNNEL_ENCAP_FLOW_SIP)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_SIP;
        teData->blockVal.tunnelVal.sip = param->sip;

        if (param->sip.isIPv6)
        {
            teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_IPV6;
        }
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_TOS)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_TOS;
        teData->blockVal.tunnelVal.tos = param->tos;
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_TTL)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_TTL;
        teData->blockVal.tunnelVal.ttl = param->ttl;
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_L4DST)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_L4DST;
        teData->blockVal.tunnelVal.l4Dst = param->l4Dst;
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_L4SRC)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_L4SRC;
        teData->blockVal.tunnelVal.l4Src = param->l4Src;
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_COUNTER)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_COUNTER;
    }

    /* NGE and GPE/NSH are mutually exclusive */
    if ( (field & (FM_TUNNEL_ENCAP_FLOW_NGE |
                   FM_TUNNEL_ENCAP_FLOW_NGE_TIME) ) &&
         (field & FM_TUNNEL_ENCAP_FLOW_GPE_NSH_ALL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_GPE_VNI)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE;

        teData->blockVal.tunnelVal.ngeMask |= FM10000_NGE_MASK_GPE_FLAGS_NEXT_PROT |
                                              FM10000_NGE_MASK_GPE_VNI;

        teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_GPE_VNI] =
            (param->gpeVni & 0xFFFFFF) << 8;

        if (param->type == FM_TUNNEL_TYPE_GPE)
        {
            /* GPE: NextProt=Ethernet(3), Flags=0x0C */
            teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT] = 0x0C000003;
        }
        else if (param->type == FM_TUNNEL_TYPE_GPE_NSH)
        {
            /* GPE: NextProt=NSH(4), Flags=0x0C */
            teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT] = 0x0C000004;
        }
    }

    if ( (field & FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR) &&
         (param->type == FM_TUNNEL_TYPE_GPE_NSH) )
    {
        if (param->nshLength > (FM_TUNNEL_NGE_DATA_SIZE -
                                FM_TUNNEL_GPE_HDR_SIZE) )
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE;

        teData->blockVal.tunnelVal.ngeMask |= FM10000_NGE_MASK_NSH_BASE_HDR;

        /* NSH: Flags=0x0, NextProt=3 */
        teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_NSH_BASE_HDR] =
            ( (param->nshCritical & 1) << 28 )  |
            ( (param->nshLength & 0x3F) << 16 ) |
            ( (param->nshMdType & 0xFF) << 8 ) |
            0x3;
    }

    if ( (field & FM_TUNNEL_ENCAP_FLOW_NSH_SERVICE_HDR) &&
         (param->type == FM_TUNNEL_TYPE_GPE_NSH) )
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE;

        teData->blockVal.tunnelVal.ngeMask |= FM10000_NGE_MASK_NSH_SERVICE_HDR;

        teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_NSH_SERVICE_HDR] =
            ( (param->nshSvcPathId & 0xFFFFFF) << 8) |
            (param->nshSvcIndex & 0xFF);
    }

    if ( (field & FM_TUNNEL_ENCAP_FLOW_NSH_DATA) &&
         (param->type == FM_TUNNEL_TYPE_GPE_NSH) )
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE;

        teData->blockVal.tunnelVal.ngeMask |=
            (param->nshDataMask & FM10000_TE_NSH_DATA_MASK) << FM10000_NGE_POS_NSH_DATA;

        FM_MEMCPY_S(&teData->blockVal.tunnelVal.ngeData[FM10000_NGE_POS_NSH_DATA],
                    sizeof(teData->blockVal.tunnelVal.ngeData[0]) *
                    FM_TUNNEL_NSH_DATA_SIZE,
                    param->nshData,
                    sizeof(param->nshData));
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_NGE)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE;
        teData->blockVal.tunnelVal.ngeMask = param->ngeMask;
        FM_MEMCPY_S(teData->blockVal.tunnelVal.ngeData,
                    sizeof(teData->blockVal.tunnelVal.ngeData),
                    param->ngeData,
                    sizeof(param->ngeData));
    }

    if (field & FM_TUNNEL_ENCAP_FLOW_NGE_TIME)
    {
        teData->blockVal.tunnelVal.tunnelConfig |= FM10000_TE_TUNNEL_ENCAP_NGE_TIME;
    }

ABORT:
    return err;

}   /* end EncapFlowToTeData */




/*****************************************************************************/
/** TunnelRuleToTeData
 * \ingroup intTunnel
 *
 * \desc            Translate tunnel rule to low level teData.
 *
 * \param[in]       tunnelGrp refer to the tunnel group this rule belong to.
 *
 * \param[in]       tunnelRule refer to the rule to translate.
 *
 * \param[out]      teData refer to the teData structure array to fill.
 *
 * \param[in]       teDataSize is the teData array size.
 *
 * \param[out]      teDataOut refer to the number of teData entries filled.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status TunnelRuleToTeData(fm_fm10000TunnelGrp * tunnelGrp,
                                    fm_fm10000TunnelRule *tunnelRule,
                                    fm_fm10000TeData *    teData,
                                    fm_int                teDataSize,
                                    fm_int *              teDataOut)
{
    fm_status                     err = FM_OK;
    fm_int                        teDataPos = 0;
    void *                        value;
    fm_fm10000EncapFlow *         encapFlow;
    fm_fm10000TeDataFlowEncapVal *fevPtr;

    if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
    {
        /* The bin size limit is virtual and can be increased if needed. */
        if (teDataSize <= teDataPos)
        {
            err = FM_ERR_TUNNEL_BIN_FULL;
            return err;
        }

        teData[teDataPos].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;

        teData[teDataPos].blockVal.flowKeyVal.searchKeyConfig =
            TunnelToTeHashKey(tunnelRule->condition,
                              &tunnelRule->condParam);

        /* Process the search key specific bit */
        if (tunnelRule->condition & FM_TUNNEL_MATCH_UDP)
        {
            /* Can't match on Protocol + UDP or TCP */
            if (tunnelRule->condition & FM_TUNNEL_MATCH_PROT)
            {
                err = FM_ERR_TUNNEL_CONFLICT;
                return err;
            }
            teData[teDataPos].blockVal.flowKeyVal.searchKeyConfig |= FM10000_TE_KEY_UDP;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_TCP)
        {
            /* Can't match on Protocol + UDP or TCP */
            if (tunnelRule->condition & FM_TUNNEL_MATCH_PROT)
            {
                err = FM_ERR_TUNNEL_CONFLICT;
                return err;
            }
            teData[teDataPos].blockVal.flowKeyVal.searchKeyConfig |= FM10000_TE_KEY_TCP;
        }

        /* Convert the condition param to the block val type */
        if (tunnelRule->condition & FM_TUNNEL_MATCH_VSI_TEP)
        {
            teData[teDataPos].blockVal.flowKeyVal.vsiTep = tunnelRule->condParam.vsiTep;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_VNI)
        {
            teData[teDataPos].blockVal.flowKeyVal.vni = tunnelRule->condParam.vni;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_DMAC)
        {
            teData[teDataPos].blockVal.flowKeyVal.dmac = tunnelRule->condParam.dmac;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_SMAC)
        {
            teData[teDataPos].blockVal.flowKeyVal.smac = tunnelRule->condParam.smac;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_VLAN)
        {
            teData[teDataPos].blockVal.flowKeyVal.vlan = tunnelRule->condParam.vlan;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_DIP)
        {
            teData[teDataPos].blockVal.flowKeyVal.dip = tunnelRule->condParam.dip;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_SIP)
        {
            teData[teDataPos].blockVal.flowKeyVal.sip = tunnelRule->condParam.sip;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_L4SRC)
        {
            teData[teDataPos].blockVal.flowKeyVal.l4Src = tunnelRule->condParam.l4Src;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_L4DST)
        {
            teData[teDataPos].blockVal.flowKeyVal.l4Dst = tunnelRule->condParam.l4Dst;
        }

        if (tunnelRule->condition & FM_TUNNEL_MATCH_PROT)
        {
            teData[teDataPos].blockVal.flowKeyVal.protocol = tunnelRule->condParam.protocol;
        }

        teDataPos++;
    }

    if (tunnelGrp->tunnelParam.encap)
    {
        /* The bin size limit is virtual and can be increased if needed. */
        if (teDataSize <= teDataPos)
        {
            err = FM_ERR_TUNNEL_BIN_FULL;
            return err;
        }

        teData[teDataPos].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP;

        teData[teDataPos].blockVal.flowEncapVal.encapConfig = 0;

        /* Translate the encap action from tunnel structure to low level
         * representation */
        if (tunnelRule->action & FM_TUNNEL_SET_VNI)
        {
            /* Can only set a VNI if the rule refer to an encap flow */
            if ((tunnelRule->action & FM_TUNNEL_ENCAP_FLOW) == 0)
            {
                return FM_ERR_TUNNEL_NO_ENCAP_FLOW;
            }
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_VNI;
            teData[teDataPos].blockVal.flowEncapVal.vni = tunnelRule->actParam.vni;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_DMAC)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_DMAC;
            teData[teDataPos].blockVal.flowEncapVal.dmac = tunnelRule->actParam.dmac;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_SMAC)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_SMAC;
            teData[teDataPos].blockVal.flowEncapVal.smac = tunnelRule->actParam.smac;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_VLAN)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_VLAN;
            teData[teDataPos].blockVal.flowEncapVal.vlan = tunnelRule->actParam.vlan;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_DIP)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_DIP;
            teData[teDataPos].blockVal.flowEncapVal.dip = tunnelRule->actParam.dip;

            if (tunnelRule->actParam.dip.isIPv6)
            {
                teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_IPV6;
            }
        }

        if (tunnelRule->action & FM_TUNNEL_SET_SIP)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_SIP;
            teData[teDataPos].blockVal.flowEncapVal.sip = tunnelRule->actParam.sip;

            if (tunnelRule->actParam.sip.isIPv6)
            {
                teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_IPV6;
            }
        }

        if (tunnelRule->action & FM_TUNNEL_SET_L4SRC)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_L4SRC;
            teData[teDataPos].blockVal.flowEncapVal.l4Src = tunnelRule->actParam.l4Src;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_L4DST)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_L4DST;
            teData[teDataPos].blockVal.flowEncapVal.l4Dst = tunnelRule->actParam.l4Dst;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_TTL)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_TTL;
            teData[teDataPos].blockVal.flowEncapVal.ttl = tunnelRule->actParam.ttl;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_GPE_NSH_ALL)
        {
            /* NGE and GPE/NSH are mutually exclusive */
            if (tunnelRule->action & (FM_TUNNEL_SET_NGE |
                                      FM_TUNNEL_SET_NGE_TIME) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Can only set a VNI if the rule refer to an encap flow */
            if ((tunnelRule->action & FM_TUNNEL_ENCAP_FLOW) == 0)
            {
                err = FM_ERR_TUNNEL_NO_ENCAP_FLOW;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* EncapFlow needed to know the Tunnel Type*/
            err = fmTreeFind(&tunnelGrp->encapFlows,
                             tunnelRule->actParam.encapFlow,
                             &value);
            if (err == FM_ERR_NOT_FOUND)
            {
                /* Specified encap flow not currently specify in that group */
                err =  FM_ERR_TUNNEL_NO_ENCAP_FLOW;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            encapFlow = (fm_fm10000EncapFlow *) value;

            if (tunnelRule->action & FM_TUNNEL_SET_GPE_VNI)
            {
                fevPtr = &teData[teDataPos].blockVal.flowEncapVal;
                fevPtr->encapConfig |= FM10000_TE_FLOW_ENCAP_NGE;

                fevPtr->ngeMask |= FM10000_NGE_MASK_GPE_FLAGS_NEXT_PROT |
                                   FM10000_NGE_MASK_GPE_VNI;
                fevPtr->ngeData[FM10000_NGE_POS_GPE_VNI] =
                    (tunnelRule->actParam.gpeVni & 0xFFFFFF) << 8;

                if (encapFlow->param.type == FM_TUNNEL_TYPE_GPE)
                {
                    /* GPE: NextProt=Ethernet(3), Flags=0x0C */
                    fevPtr->ngeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT] = 0x0C000003;
                }
                else if (encapFlow->param.type == FM_TUNNEL_TYPE_GPE_NSH)
                {
                    /* GPE: NextProt=NSH(4), Flags=0x0C */
                    fevPtr->ngeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT] = 0x0C000004;
                }
            }

            if ( (tunnelRule->action & FM_TUNNEL_SET_NSH_BASE_HDR) &&
                 (encapFlow->param.type == FM_TUNNEL_TYPE_GPE_NSH) )
            {
                if (tunnelRule->actParam.nshLength > (FM_TUNNEL_NGE_DATA_SIZE -
                                                      FM_TUNNEL_GPE_HDR_SIZE) )
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }

                fevPtr = &teData[teDataPos].blockVal.flowEncapVal;
                fevPtr->encapConfig |= FM10000_TE_FLOW_ENCAP_NGE;

                fevPtr->ngeMask |= FM10000_NGE_MASK_NSH_BASE_HDR;

                /* NSH: Flags=0x0, NextProt=3 */
                fevPtr->ngeData[FM10000_NGE_POS_NSH_BASE_HDR] =
                    ( (tunnelRule->actParam.nshCritical & 1) << 28 )  |
                    ( (tunnelRule->actParam.nshLength & 0x3F) << 16 ) |
                    ( (tunnelRule->actParam.nshMdType & 0xFF) << 8 )  |
                    0x3;
            }

            if ( (tunnelRule->action & FM_TUNNEL_SET_NSH_SERVICE_HDR) &&
                 (encapFlow->param.type == FM_TUNNEL_TYPE_GPE_NSH) )
            {
                fevPtr = &teData[teDataPos].blockVal.flowEncapVal;
                fevPtr->encapConfig |= FM10000_TE_FLOW_ENCAP_NGE;

                fevPtr->ngeMask |= FM10000_NGE_MASK_NSH_SERVICE_HDR;

                fevPtr->ngeData[FM10000_NGE_POS_NSH_SERVICE_HDR] =
                    ( (tunnelRule->actParam.nshSvcPathId & 0xFFFFFF) << 8 ) |
                    (tunnelRule->actParam.nshSvcIndex & 0xFF);
            }

            if ( (tunnelRule->action & FM_TUNNEL_SET_NSH_DATA) &&
                 (encapFlow->param.type == FM_TUNNEL_TYPE_GPE_NSH) )
            {
                fevPtr = &teData[teDataPos].blockVal.flowEncapVal;
                fevPtr->encapConfig |= FM10000_TE_FLOW_ENCAP_NGE;

                fevPtr->ngeMask |= (tunnelRule->actParam.nshDataMask &
                                    FM10000_TE_NSH_DATA_MASK) <<
                                    FM10000_NGE_POS_NSH_DATA;

                FM_MEMCPY_S(&fevPtr->ngeData[FM10000_NGE_POS_NSH_DATA],
                            sizeof(fevPtr->ngeData[0]) *
                            FM_TUNNEL_NSH_DATA_SIZE,
                            tunnelRule->actParam.nshData,
                            sizeof(tunnelRule->actParam.nshData));
            }
        }

        if (tunnelRule->action & FM_TUNNEL_SET_NGE)
        {
            /* Can only set a VNI if the rule refer to an encap flow */
            if ((tunnelRule->action & FM_TUNNEL_ENCAP_FLOW) == 0)
            {
                return FM_ERR_TUNNEL_NO_ENCAP_FLOW;
            }
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_NGE;
            teData[teDataPos].blockVal.flowEncapVal.ngeMask = tunnelRule->actParam.ngeMask;

            FM_MEMCPY_S(teData[teDataPos].blockVal.flowEncapVal.ngeData,
                        sizeof(teData[teDataPos].blockVal.flowEncapVal.ngeData),
                        tunnelRule->actParam.ngeData,
                        sizeof(tunnelRule->actParam.ngeData));
        }

        if (tunnelRule->action & FM_TUNNEL_SET_NGE_TIME)
        {
            /* Can only set a VNI if the rule refer to an encap flow */
            if ((tunnelRule->action & FM_TUNNEL_ENCAP_FLOW) == 0)
            {
                return FM_ERR_TUNNEL_NO_ENCAP_FLOW;
            }
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_NGE_TIME;
        }

        if (tunnelRule->action & FM_TUNNEL_COUNT)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_COUNTER;
            teData[teDataPos].blockVal.flowEncapVal.counterIdx = tunnelRule->counter;
        }

        if (tunnelRule->action & FM_TUNNEL_ENCAP_FLOW)
        {
            teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_TUNNEL;

            /* Shared encap flow used pointer while unshared one follow the
             * tunnel data. */
            err = fmTreeFind(&tunnelGrp->encapFlows,
                             tunnelRule->actParam.encapFlow,
                             &value);
            if (err == FM_ERR_NOT_FOUND)
            {
                /* Specified encap flow not currently specify in that group */
                return FM_ERR_TUNNEL_NO_ENCAP_FLOW;
            }
            else if (err != FM_OK)
            {
                /* Unspecified error */
                return err;
            }

            encapFlow = (fm_fm10000EncapFlow *) value;

            if (encapFlow->param.shared)
            {
                teData[teDataPos].blockVal.flowEncapVal.encapConfig |= FM10000_TE_FLOW_ENCAP_TUNNEL_PTR;
                teData[teDataPos].blockVal.flowEncapVal.tunnelIdx = encapFlow->teLookup;
            }
            else
            {
                teDataPos++;

                /* The bin size limit is virtual and can be increased if needed. */
                if (teDataSize <= teDataPos)
                {
                    err = FM_ERR_TUNNEL_BIN_FULL;
                    return err;
                }

                err = EncapFlowToTeData(encapFlow->field,
                                        &encapFlow->param,
                                        &teData[teDataPos]);
                if (err != FM_OK)
                {
                    return err;
                }
            }
        }

        teDataPos++;

    }
    else
    {
        /* The bin size limit is virtual and can be increased if needed. */
        if (teDataSize <= teDataPos)
        {
            err = FM_ERR_TUNNEL_BIN_FULL;
            return err;
        }

        teData[teDataPos].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP;

        teData[teDataPos].blockVal.flowDecapVal.decapConfig = 0;

        /* Can't keep and move */
        if ( (tunnelRule->action & FM_TUNNEL_DECAP_KEEP_OUTER_HDR) &&
             (tunnelRule->action & FM_TUNNEL_DECAP_MOVE_OUTER_HDR) )
        {
            err = FM_ERR_TUNNEL_CONFLICT;
            return err;
        }
        else if (tunnelRule->action & FM_TUNNEL_DECAP_KEEP_OUTER_HDR)
        {
            teData[teDataPos].blockVal.flowDecapVal.outerHeader = FM_FM10000_TE_OUTER_HEADER_LEAVE_AS_IS;
        }
        else if (tunnelRule->action & FM_TUNNEL_DECAP_MOVE_OUTER_HDR)
        {
            teData[teDataPos].blockVal.flowDecapVal.outerHeader = FM_FM10000_TE_OUTER_HEADER_MOVE_TO_END;
        }
        /* Default behavious */
        else
        {
            teData[teDataPos].blockVal.flowDecapVal.outerHeader = FM_FM10000_TE_OUTER_HEADER_DELETE;
        }

        /* Translate the decap action from tunnel structure to low level
         * representation */
        if (tunnelRule->action & FM_TUNNEL_SET_DGLORT)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_DGLORT;
            teData[teDataPos].blockVal.flowDecapVal.dglort = tunnelRule->actParam.dglort;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_DMAC)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_DMAC;
            teData[teDataPos].blockVal.flowDecapVal.dmac = tunnelRule->actParam.dmac;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_SMAC)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_SMAC;
            teData[teDataPos].blockVal.flowDecapVal.smac = tunnelRule->actParam.smac;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_VLAN)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_VLAN;
            teData[teDataPos].blockVal.flowDecapVal.vlan = tunnelRule->actParam.vlan;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_DIP)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_DIP;
            teData[teDataPos].blockVal.flowDecapVal.dip = tunnelRule->actParam.dip;

            if (tunnelRule->actParam.dip.isIPv6)
            {
                teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_IPV6;
            }
        }

        if (tunnelRule->action & FM_TUNNEL_SET_SIP)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_SIP;
            teData[teDataPos].blockVal.flowDecapVal.sip = tunnelRule->actParam.sip;

            if (tunnelRule->actParam.sip.isIPv6)
            {
                teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_IPV6;
            }
        }

        if (tunnelRule->action & FM_TUNNEL_SET_L4SRC)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_L4SRC;
            teData[teDataPos].blockVal.flowDecapVal.l4Src = tunnelRule->actParam.l4Src;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_L4DST)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_L4DST;
            teData[teDataPos].blockVal.flowDecapVal.l4Dst = tunnelRule->actParam.l4Dst;
        }

        if (tunnelRule->action & FM_TUNNEL_SET_TTL)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_TTL;
            teData[teDataPos].blockVal.flowDecapVal.ttl = tunnelRule->actParam.ttl;
        }

        if (tunnelRule->action & FM_TUNNEL_COUNT)
        {
            teData[teDataPos].blockVal.flowDecapVal.decapConfig |= FM10000_TE_FLOW_DECAP_COUNTER;
            teData[teDataPos].blockVal.flowDecapVal.counterIdx = tunnelRule->counter;
        }

        teDataPos++;

    }

    *teDataOut = teDataPos;

ABORT:
    return err;

}   /* end TunnelRuleToTeData */




/*****************************************************************************/
/** ComputeTunnelHash
 * \ingroup intTunnel
 *
 * \desc            Compute the hash based on the condition mask and value.
 *
 * \param[in]       cond is the condition bitmask
 *
 * \param[in]       condParam refer to the value of each field.
 *
 * \return          The computer hash result
 *
 *****************************************************************************/
static fm_uint16 ComputeTunnelHash(fm_tunnelCondition       cond,
                                   fm_tunnelConditionParam *condParam)
{
    fm_int    i;
    fm_int    pos;
    fm_uint32 ipTmp;
    fm_byte   hashKey[FM10000_TUNNEL_HASH_KEY_BYTES] = {0};
    fm_uint32 crc32;
    fm_uint16 hashKeyConfig;

    /* The packed hash key used to generate lookupIndex.
     * W[0]  = key[7:0]
     * W[1]  = key[15:8]
     * W[14] = key[479:472] */

    pos = 0;

    hashKeyConfig = TunnelToTeHashKey(cond, condParam);

    hashKey[pos++] = hashKeyConfig & 0xFF;
    hashKey[pos++] = (hashKeyConfig >> 8) & 0xFF;

    if (cond & FM_TUNNEL_MATCH_VSI_TEP)
    {
        hashKey[pos++] = condParam->vsiTep & 0xFF;
        hashKey[pos++] = (condParam->vsiTep >> 8) & 0xFF;
    }
    else
    {
        pos += 2;
    }

    if (cond & FM_TUNNEL_MATCH_VNI)
    {
        hashKey[pos++] = condParam->vni & 0xFF;
        hashKey[pos++] = (condParam->vni >> 8) & 0xFF;
        hashKey[pos++] = (condParam->vni >> 16) & 0xFF;
        pos++;
    }
    else
    {
        pos += 4;
    }

    if (cond & FM_TUNNEL_MATCH_DMAC)
    {
        hashKey[pos++] = (condParam->dmac) & 0xFF;
        hashKey[pos++] = (condParam->dmac >> 8) & 0xFF;
        hashKey[pos++] = (condParam->dmac >> 16) & 0xFF;
        hashKey[pos++] = (condParam->dmac >> 24) & 0xFF;
        hashKey[pos++] = (condParam->dmac >> 32) & 0xFF;
        hashKey[pos++] = (condParam->dmac >> 40) & 0xFF;
    }
    else
    {
        pos += 6;
    }

    if (cond & FM_TUNNEL_MATCH_SMAC)
    {
        hashKey[pos++] = (condParam->smac) & 0xFF;
        hashKey[pos++] = (condParam->smac >> 8) & 0xFF;
        hashKey[pos++] = (condParam->smac >> 16) & 0xFF;
        hashKey[pos++] = (condParam->smac >> 24) & 0xFF;
        hashKey[pos++] = (condParam->smac >> 32) & 0xFF;
        hashKey[pos++] = (condParam->smac >> 40) & 0xFF;
    }
    else
    {
        pos += 6;
    }

    if (cond & FM_TUNNEL_MATCH_VLAN)
    {
        hashKey[pos++] = (condParam->vlan) & 0xFF;
        hashKey[pos++] = (condParam->vlan >> 8) & 0x0F;
    }
    else
    {
        pos += 2;
    }

    if (cond & FM_TUNNEL_MATCH_DIP)
    {
        if (condParam->dip.isIPv6)
        {
            for (i = 0; i < 4; i++)
            {
                ipTmp = ntohl(condParam->dip.addr[i]);
                hashKey[pos++] = ipTmp & 0xFF;
                hashKey[pos++] = (ipTmp >> 8) & 0xFF;
                hashKey[pos++] = (ipTmp >> 16) & 0xFF;
                hashKey[pos++] = (ipTmp >> 24) & 0xFF;
            }
        }
        else
        {
            ipTmp = ntohl(condParam->dip.addr[0]);
            hashKey[pos++] = ipTmp & 0xFF;
            hashKey[pos++] = (ipTmp >> 8) & 0xFF;
            hashKey[pos++] = (ipTmp >> 16) & 0xFF;
            hashKey[pos++] = (ipTmp >> 24) & 0xFF;
            pos += 12;
        }
    }
    else
    {
        pos += 16;
    }

    if (cond & FM_TUNNEL_MATCH_SIP)
    {
        if (condParam->sip.isIPv6)
        {
            for (i = 0; i < 4; i++)
            {
                ipTmp = ntohl(condParam->sip.addr[i]);
                hashKey[pos++] = ipTmp & 0xFF;
                hashKey[pos++] = (ipTmp >> 8) & 0xFF;
                hashKey[pos++] = (ipTmp >> 16) & 0xFF;
                hashKey[pos++] = (ipTmp >> 24) & 0xFF;
            }
        }
        else
        {
            ipTmp = ntohl(condParam->sip.addr[0]);
            hashKey[pos++] = ipTmp & 0xFF;
            hashKey[pos++] = (ipTmp >> 8) & 0xFF;
            hashKey[pos++] = (ipTmp >> 16) & 0xFF;
            hashKey[pos++] = (ipTmp >> 24) & 0xFF;
            pos += 12;
        }
    }
    else
    {
        pos += 16;
    }

    if (cond & FM_TUNNEL_MATCH_L4SRC)
    {
        hashKey[pos++] = condParam->l4Src & 0xFF;
        hashKey[pos++] = (condParam->l4Src >> 8) & 0xFF;
    }
    else
    {
        pos += 2;
    }

    if (cond & FM_TUNNEL_MATCH_L4DST)
    {
        hashKey[pos++] = condParam->l4Dst & 0xFF;
        hashKey[pos++] = (condParam->l4Dst >> 8) & 0xFF;
    }
    else
    {
        pos += 2;
    }

    if (cond & FM_TUNNEL_MATCH_PROT)
    {
        hashKey[pos++] = condParam->protocol;
    }
    else
    {
        pos += 2;
    }

    crc32 = fmCrc32Math(hashKey, FM10000_TUNNEL_HASH_KEY_BYTES);
    return (crc32 & 0xFFFF);

}   /* end ComputeTunnelHash */




/*****************************************************************************/
/** MoveTeDataBlock
 * \ingroup intTunnel
 *
 * \desc            Move a TeData Block from one position to the other.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       srcIndex is the position of the source block.
 *
 * \param[in]       length is the size of the moved block.
 *
 * \param[in]       dstIndex is the position of the destination block.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status MoveTeDataBlock(fm_int     sw,
                                 fm_int     te,
                                 fm_uint16  srcIndex,
                                 fm_uint16  length,
                                 fm_uint16  dstIndex)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  blockHandler;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;
    fm_fm10000TunnelGrp *tunnelGrp;
    void *value;
    fm_fm10000TunnelLookupBin *lookupBin;
    fm_treeIterator itRule;
    fm_treeIterator itRuleBin;
    fm_uint64 nextKey;
    fm_uint64 ruleNumber;
    fm_fm10000TunnelRule *tunnelRule;
    fm_fm10000TeLookup newTeLookup;
    fm_registerSGListEntry sgList;
    fm_uint32 teDataReg[FM10000_TE_DATA_LENGTH_SIZE_32];
    fm_bool usedEntry;
    fm_uint i;
    fm_fm10000EncapFlow *encapFlow;
    fm_fm10000TeData teData[FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE];
    fm_int teDataPos;
    fm_int teDataOutSet;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    /* Find the owner of the block */
    blockHandler = teDataCtrl->teDataHandler[srcIndex];
    if (blockHandler == 0)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    teDataBlkCtrl = teDataCtrl->teDataBlkCtrl[blockHandler];

    /* Sanity check */
    if ( (teDataBlkCtrl->index != srcIndex) ||
         (teDataBlkCtrl->length != length) )
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[teDataBlkCtrl->tunnelGrp >> 3]
                                                [teDataBlkCtrl->tunnelGrp & 0x7];

    /* Moving a bin data block is achieved by copying the block to the new
     * location and updating the lookup after. The update of the lookup will
     * validate the register write to be completed prior to return. At that
     * point, the old location is not used anymore. The old block is not
     * cleared but virtually set as free. */
    if (teDataBlkCtrl->tunnelDataType == FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN)
    {
        err = fmTreeFind(&tunnelGrp->lookupBins,
                         teDataBlkCtrl->tunnelEntry,
                         &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin = (fm_fm10000TunnelLookupBin *)value;

        /* Sanity check */
        if ( (lookupBin->teLookup.dataPtr != srcIndex) ||
             (lookupBin->teLookup.dataLength != length) )
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        newTeLookup.dataPtr = dstIndex;
        newTeLookup.dataLength = length;
        newTeLookup.last = TRUE;

        /* Always clear the used bit of the destination index prior to validate
         * that entry. */
        err = fm10000SetTeFlowUsed(sw, te, dstIndex, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Copying teData block using SG */
        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeData,
                                  length,
                                  srcIndex,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDataReg,
                                  FALSE);

        err = fmRegCacheRead(sw, 1, &sgList, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeData,
                                  length,
                                  dstIndex,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDataReg,
                                  FALSE);

        err = fmRegCacheWrite(sw, 1, &sgList, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Now refer to this new block */
        err = fm10000SetTeLookup(sw,
                                 te,
                                 tunnelGrp->teDGlort.baseLookup +
                                     teDataBlkCtrl->tunnelEntry,
                                 &newTeLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin->teLookup = newTeLookup;

        /* Update the teData Position and used bit of the rule */
        if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_DIRECT)
        {
            fmTreeIterInit(&itRule, &lookupBin->rules);
            err = fmTreeIterNext(&itRule, &nextKey, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            err = fmTreeFind(&tunnelGrp->rules, nextKey, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            tunnelRule = (fm_fm10000TunnelRule *)value;

            tunnelRule->dataPos = dstIndex;

            err = fm10000GetTeFlowUsed(sw, te, srcIndex, &usedEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            if (usedEntry)
            {
                err = fm10000SetTeFlowUsed(sw, te, dstIndex, usedEntry);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }
    }
    /* teDataBlkCtrl->tunnelDataType == FM_FM10000_TUNNEL_TE_DATA_TYPE_ENCAPFLOW */
    else
    {
        /* Moving a shared encap flow block needs more processing than a bin
         * since all the rules that refer to that shared encap flow must also
         * be updated to refer to the new position. */
        err = fmTreeFind(&tunnelGrp->encapFlows,
                         teDataBlkCtrl->tunnelEntry,
                         &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        encapFlow = (fm_fm10000EncapFlow *)value;

        /* Sanity check */
        if ( (encapFlow->teLookup.dataPtr != srcIndex) ||
             (encapFlow->teLookup.dataLength != length) )
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        newTeLookup.dataPtr = dstIndex;
        newTeLookup.dataLength = length;
        newTeLookup.last = TRUE;

        /* Copying teData block using SG */
        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeData,
                                  length,
                                  srcIndex,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDataReg,
                                  FALSE);

        err = fmRegCacheRead(sw, 1, &sgList, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeData,
                                  length,
                                  dstIndex,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDataReg,
                                  FALSE);

        err = fmRegCacheWrite(sw, 1, &sgList, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        encapFlow->teLookup = newTeLookup;

        /* Go over all the rules that refer to that shared encap flow and
         * update them. The update is done without any movement since the
         * block is rebuilt exactly as the original except for the shared
         * encap flow pointer which is updated atomically. */
        for (fmTreeIterInit(&itRule, &encapFlow->rules) ;
              (err = fmTreeIterNext(&itRule, &nextKey, &value)) == FM_OK ; )
        {
            err = fmTreeFind(&tunnelGrp->rules, nextKey, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            tunnelRule = (fm_fm10000TunnelRule *)value;

            err = fmTreeFind(&tunnelGrp->lookupBins,
                             tunnelRule->lookupBin,
                             &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin = (fm_fm10000TunnelLookupBin *)value;
            teDataPos = 0;

            FM_MEMSET_S( teData,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                         0,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

            /* Rebuild the entire bin in the same sorting order as it was
             * created to match it with the exeption of the new encap flow
             * position. */
            for (fmTreeIterInit(&itRuleBin, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRuleBin, &ruleNumber, &value)) == FM_OK ; )
            {
                err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                err = TunnelRuleToTeData(tunnelGrp,
                                         (fm_fm10000TunnelRule*) value,
                                         &teData[teDataPos],
                                         FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                         &teDataOutSet);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                teDataPos += teDataOutSet;
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* Update the block */
            err = fm10000SetTeData(sw,
                                   te,
                                   lookupBin->teLookup.dataPtr,
                                   teData,
                                   teDataPos,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        err = FM_OK;

        /* Make sure all the register write are completed before the function
         * return. This is to avoid race condition. */
        err = fm10000SyncTeDataLookup(sw, te);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Update teData control structures */
    for (i = 0 ; i < length ; i++)
    {
        teDataCtrl->teDataHandler[srcIndex + i] = 0;
        teDataCtrl->teDataHandler[dstIndex + i] = blockHandler;
    }
    teDataBlkCtrl->index = dstIndex;
    teDataBlkCtrl->length = length;


ABORT:

    return err;

}   /* end MoveTeDataBlock */




/*****************************************************************************/
/** UpdateTeDataSwap
 * \ingroup intTunnel
 *
 * \desc            Update the Swap size and adjust TeData Control variable.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       size is the new swap size.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status UpdateTeDataSwap(fm_int     sw,
                                  fm_int     te,
                                  fm_uint16  size)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  i;
    fm_uint16  upperBound;
    fm_uint16  index;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    /* new size is smaller than previous one, just move the limit */
    if (size <= teDataCtrl->teDataSwapSize)
    {
        /* free entry count does not includes entries reserved for swap */
        teDataCtrl->teDataFreeEntryCount += (teDataCtrl->teDataSwapSize - size);
        teDataCtrl->teDataSwapSize = size;
    }
    /* new size is bigger than previous one */
    else
    {
        /* current swap limit */
        upperBound = FM10000_TE_DATA_ENTRIES_0 - (teDataCtrl->teDataSwapSize + 1);

        /* go over all the entry from the actual limit to the new one and
         * validates that all of them are free. */
        for (i = 0 ; i < (size - teDataCtrl->teDataSwapSize) ; i++)
        {
            /* One of the entry is not free, defrag the table */
            if (teDataCtrl->teDataHandler[upperBound - i] != 0)
            {
                err = DefragTeData(sw,
                                   te,
                                   size - teDataCtrl->teDataSwapSize,
                                   &index);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;
            }
        }
        /* free entry count does not includes entries reserved for swap */
        teDataCtrl->teDataFreeEntryCount -= (size - teDataCtrl->teDataSwapSize);
        teDataCtrl->teDataSwapSize = size;
    }


ABORT:

    return err;

}   /* end UpdateTeDataSwap */




/*****************************************************************************/
/** DefragTeDataWithSwap
 * \ingroup intTunnel
 *
 * \desc            Defrag the teData table entirely starting at the first free
 *                  slot.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status DefragTeDataWithSwap(fm_int     sw,
                                      fm_int     te)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  freeBaseIndex = 0;
    fm_uint16  freeBlockLength = 0;
    fm_uint16  blockLength = 0;
    fm_uint16  baseIndex = 0;
    fm_uint16  lastHandler = 0;
    fm_uint16  i;
    fm_uint16  upperBound;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    blockLength = 0;
    freeBaseIndex = teDataCtrl->teDataHandlerFirstFreeEntry;
    upperBound = (FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize);

    /* Pack the table using the swap. */
    for (i = teDataCtrl->teDataHandlerFirstFreeEntry ; i < (upperBound + 1) ; i++)
    {
        if (teDataCtrl->teDataHandler[i] == 0)
        {
            if (baseIndex)
            {
                /* Move it directly */
                if (freeBlockLength >= blockLength)
                {
                    err = MoveTeDataBlock(sw,
                                          te,
                                          baseIndex,
                                          blockLength,
                                          freeBaseIndex);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                /* Use the swap area */
                else
                {
                    err = MoveTeDataBlock(sw,
                                          te,
                                          baseIndex,
                                          blockLength,
                                          upperBound);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    err = MoveTeDataBlock(sw,
                                          te,
                                          upperBound,
                                          blockLength,
                                          freeBaseIndex);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                freeBaseIndex += blockLength;
                baseIndex = 0;
                blockLength = 0;
            }

            /* free block always increase since that block is now a contiguous
             * free block that includes all the previous individual entry. */
            freeBlockLength++;
        }
        else
        {
            /* new block frontier */
            if (lastHandler != teDataCtrl->teDataHandler[i])
            {
                if (baseIndex)
                {
                    /* Move it directly */
                    if (freeBlockLength >= blockLength)
                    {
                        err = MoveTeDataBlock(sw,
                                              te,
                                              baseIndex,
                                              blockLength,
                                              freeBaseIndex);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }
                    /* Use the swap area */
                    else
                    {
                        err = MoveTeDataBlock(sw,
                                              te,
                                              baseIndex,
                                              blockLength,
                                              upperBound);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                        err = MoveTeDataBlock(sw,
                                              te,
                                              upperBound,
                                              blockLength,
                                              freeBaseIndex);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }
                    freeBaseIndex += blockLength;
                }

                /* start a new block */
                baseIndex = i;
                blockLength = 1;
            }
            else
            {
                /* currently running over entry of the same block as the
                 * previous one */
                blockLength++;
            }
        }

        lastHandler = teDataCtrl->teDataHandler[i];
    }

    /* entries are all packed up to the first free position */
    teDataCtrl->teDataHandlerFirstFreeEntry = freeBaseIndex;


ABORT:

    return err;

}   /* end DefragTeDataWithSwap */




/*****************************************************************************/
/** DefragTeData
 * \ingroup intTunnel
 *
 * \desc            Defrag the teData table to make room for a block size as
 *                  defined with "size" argument and return the index.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       size is the number of consecutive block needed.
 *
 * \param[out]      index refer to the block starting index.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status DefragTeData(fm_int     sw,
                              fm_int     te,
                              fm_uint16  size,
                              fm_uint16 *index)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  firstBlockLength;
    fm_uint16  firstBaseIndex = 0;
    fm_uint16  lastBlockLength;
    fm_uint16  lastBaseIndex = 0;
    fm_uint16  lastHandler;
    fm_uint16  i;
    fm_uint16  j;
    fm_uint16  upperBound;
    fm_uint16  maxBlockSize = FM10000_TUNNEL_TE_DATA_MIN_SWAP_SIZE;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    firstBlockLength = 0;
    upperBound = (FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize);

    /* Find a free teData block and try to fill it with blocks located at the
     * end of the table. This would defrag the begenning of the table while
     * fragementing the last part. */
    for (i = teDataCtrl->teDataHandlerFirstFreeEntry ; i < upperBound ; i++)
    {
        if (teDataCtrl->teDataHandler[i] == 0)
        {
            if (firstBlockLength == 0)
            {
                firstBaseIndex = i;
            }
            firstBlockLength++;
        }
        else
        {
            /* Fill this hole located at firstBaseIndex with size
               firstBlockLength by blocks from the end. */
            lastBlockLength = 0;
            lastHandler = 0;
            for (j = upperBound - 1 ; j > i ; j--)
            {
                /* New block */
                if ( (lastHandler == 0) && teDataCtrl->teDataHandler[j] )
                {
                    lastHandler = teDataCtrl->teDataHandler[j];
                    lastBlockLength = 1;
                }
                /* Cross a block boundary */
                else if ( (lastHandler != 0) &&
                          (lastHandler != teDataCtrl->teDataHandler[j]) )
                {
                    lastBaseIndex = j + 1;

                    /* This block fit into the free slot, move it. */
                    if (lastBlockLength <= firstBlockLength)
                    {
                        err = MoveTeDataBlock(sw,
                                              te,
                                              lastBaseIndex,
                                              lastBlockLength,
                                              firstBaseIndex);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                        firstBlockLength -= lastBlockLength;
                        firstBaseIndex += lastBlockLength;

                        /* Initial block filled entirely */
                        if (firstBlockLength == 0)
                        {
                            break;
                        }
                    }

                    lastHandler = teDataCtrl->teDataHandler[j];
                    lastBlockLength = 1;
                }
                else
                {
                    lastBlockLength++;
                }
            }
            /* Not able to fill this last hole, must proceed to a full defrag
             * for the remaining part. */
            if (j == i)
            {
                err = DefragTeDataWithSwap(sw, te);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                break;
            }
            /* Hole filled, update the first free entry */
            else
            {
                for (; i < upperBound ; i++)
                {
                    if (teDataCtrl->teDataHandler[i] == 0)
                    {
                        teDataCtrl->teDataHandlerFirstFreeEntry = i;
                        i--;
                        break;
                    }
                }

            }
            firstBlockLength = 0;
        }
    }

    /* Scan the whole table for the biggest block */
    lastBlockLength = 0;
    lastHandler = 0;
    for (i = 1 ; i < upperBound ; i++)
    {
        if (teDataCtrl->teDataHandler[i] == 0)
        {
            break;
        }

        if (teDataCtrl->teDataHandler[i] != lastHandler)
        {
            if (lastBlockLength > maxBlockSize)
            {
                maxBlockSize = lastBlockLength;
            }
            lastBlockLength = 1;
            lastHandler = teDataCtrl->teDataHandler[i];
        }
        else
        {
            lastBlockLength++;
        }

    }
    /* Cover the last remaining block just before the upper boundary */
    if (lastBlockLength > maxBlockSize)
    {
        maxBlockSize = lastBlockLength;
    }

    /* Make sure the new block fit into the defrag area */
    if (size > maxBlockSize)
    {
        maxBlockSize = size;
    }

    /* is the new bigger block smaller than swap size? */
    if (maxBlockSize < teDataCtrl->teDataSwapSize)
    {
        err = UpdateTeDataSwap(sw, te, maxBlockSize);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* the table is now completely defrag, the first free entry refer to
     * the first entry of the only free block located at the end of the
     * table. */
    if (size <= teDataCtrl->teDataFreeEntryCount)
    {
        *index = teDataCtrl->teDataHandlerFirstFreeEntry;
    }
    else
    {
        err = FM_ERR_TUNNEL_FLOW_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:

    return err;

}   /* end DefragTeData */




/*****************************************************************************/
/** FindTeDataBlock
 * \ingroup intTunnel
 *
 * \desc            Find a TeData Block with size number of consecutive entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       size is the number of consecutive block needed.
 *
 * \param[out]      index refer to the block starting index found.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status FindTeDataBlock(fm_int     sw,
                                 fm_int     te,
                                 fm_uint16  size,
                                 fm_uint16 *index)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  currentBlockLength;
    fm_uint16  baseIndex = 0;
    fm_uint16  i;
    fm_uint16  upperBound;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    /* Always keep some kind of buffer in the table to avoid continuous defrag
     * of the table at every add/remove rule when the usage is pretty high.
     * The current scheme reserve 1% of the table as buffer. This is only for
     * performance enhancement. */
    if (teDataCtrl->teDataFreeEntryCount < (FM10000_TUNNEL_TE_DATA_MIN_FREE_SIZE + size) )
    {
        err = FM_ERR_TUNNEL_FLOW_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* swap size must always be big enough to cover the worst case */
    if (size > teDataCtrl->teDataSwapSize)
    {
        err = UpdateTeDataSwap(sw, te, size);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    currentBlockLength = 0;
    upperBound = (FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize);

    /* Find a free teData block. The search starts at the first free position
     * and end at the swap boundary. This try to always fill the lowest index
     * first to avoid fragmentation as much as possible. */
    for (i = teDataCtrl->teDataHandlerFirstFreeEntry ; i < upperBound ; i++)
    {
        if (teDataCtrl->teDataHandler[i] == 0)
        {
            if (currentBlockLength == 0)
            {
                baseIndex = i;
            }
            currentBlockLength++;

            /* Found free block large enough */
            if (currentBlockLength >= size)
            {
                break;
            }
        }
        else
        {
            currentBlockLength = 0;
        }
    }

    /* the table don't have free block large enough, defrag */
    if (i == upperBound)
    {
        err = DefragTeData(sw, te, size, &baseIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    *index = baseIndex;


ABORT:

    return err;

}   /* end FindTeDataBlock */




/*****************************************************************************/
/** ReserveTeDataBlock
 * \ingroup intTunnel
 *
 * \desc            Reserve a TeData Block with size number of consecutive entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       teDataBlkCtrl refer to the block configuration to reserve.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status ReserveTeDataBlock(fm_int                           sw,
                                    fm_int                           te,
                                    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  i;
    fm_uint16 teDataHandler;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    /* Start the search after the last inserted position. The control position
     * is not important and we uses that scheme to increase the possibility
     * to find a free position. */
    for (teDataHandler = teDataCtrl->lastTeDataBlkCtrlIndex + 1 ;
         teDataHandler < FM10000_TE_DATA_ENTRIES_0 ;
         teDataHandler++)
    {
        if (teDataCtrl->teDataBlkCtrl[teDataHandler] == NULL)
        {
            break;
        }
    }
    /* Restart the search (wrap on the control block array) */
    if (teDataHandler == FM10000_TE_DATA_ENTRIES_0)
    {
        for (teDataHandler = 1 ;
             teDataHandler < FM10000_TE_DATA_ENTRIES_0 ;
             teDataHandler++)
        {
            if (teDataCtrl->teDataBlkCtrl[teDataHandler] == NULL)
            {
                break;
            }
        }
    }
    /* Should always have at least one slot free */
    if (teDataHandler == FM10000_TE_DATA_ENTRIES_0)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Validate the block */
    for (i = 0 ; i < teDataBlkCtrl->length ; i++)
    {
        if (teDataCtrl->teDataHandler[teDataBlkCtrl->index + i] != 0)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    /* Reserve the block */
    for (i = 0 ; i < teDataBlkCtrl->length ; i++)
    {
        teDataCtrl->teDataHandler[teDataBlkCtrl->index + i] = teDataHandler;
    }
    teDataCtrl->teDataBlkCtrl[teDataHandler] = teDataBlkCtrl;

    /* Adjust the first free block position if the one reserve was refering
     * this entry */
    if (teDataBlkCtrl->index <= teDataCtrl->teDataHandlerFirstFreeEntry)
    {
        for (i = teDataBlkCtrl->index ;
             i < FM10000_TE_DATA_ENTRIES_0 ;
             i++)
        {
            if (teDataCtrl->teDataHandler[i] == 0)
            {
                teDataCtrl->teDataHandlerFirstFreeEntry = i;
                break;
            }
        }
    }

    teDataCtrl->lastTeDataBlkCtrlIndex = teDataHandler;

    /* free count adjustement */
    teDataCtrl->teDataFreeEntryCount -= teDataBlkCtrl->length;

ABORT:

    return err;

}   /* end ReserveTeDataBlock */




/*****************************************************************************/
/** FreeTeDataBlock
 * \ingroup intTunnel
 *
 * \desc            Free a TeData Block with size number of consecutive entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \param[in]       index is the block starting index to free.
 *
 * \param[in]       size is the number of consecutive block to free.
 *
 * \param[out]      teDataBlkCtrl refer to the control block previously assigned
 *                  to that entry.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status FreeTeDataBlock(fm_int                            sw,
                                 fm_int                            te,
                                 fm_uint16                         index,
                                 fm_uint16                         size,
                                 fm_fm10000TunnelTeDataBlockCtrl **teDataBlkCtrl)
{
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status  err = FM_OK;
    fm_uint16  i;
    fm_uint16 tmpTeDataHandler;
    fm_uint16 teDataHandler;

    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

    /* Validate the block by making sure previous element is different. This is
     * also a sanity check that could be removed for performance enhancement. */
    tmpTeDataHandler = teDataCtrl->teDataHandler[index - 1];
    teDataHandler = teDataCtrl->teDataHandler[index];
    if (tmpTeDataHandler == teDataHandler)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    for (i = 1 ; i < size ; i++)
    {
        if (teDataCtrl->teDataHandler[index + i] != teDataHandler)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }
    /* Validate the block by making sure next element is different. This is
     * also a sanity check that could be removed for performance enhancement. */
    if ((index + i) < FM10000_TE_DATA_ENTRIES_0)
    {
        tmpTeDataHandler = teDataCtrl->teDataHandler[index + i];
        if (tmpTeDataHandler == teDataHandler)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    /* Free the block */
    for (i = 0 ; i < size ; i++)
    {
        teDataCtrl->teDataHandler[index + i] = 0;
    }
    *teDataBlkCtrl = teDataCtrl->teDataBlkCtrl[teDataHandler];
    teDataCtrl->teDataBlkCtrl[teDataHandler] = NULL;
    teDataCtrl->lastTeDataBlkCtrlIndex = teDataHandler - 1;

    /* Adjust other variable */
    if (index < teDataCtrl->teDataHandlerFirstFreeEntry)
    {
        teDataCtrl->teDataHandlerFirstFreeEntry = index;
    }

    teDataCtrl->teDataFreeEntryCount += size;

ABORT:

    return err;

}   /* end FreeTeDataBlock */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000TunnelInit
 * \ingroup intTunnel
 *
 * \desc            Initialize the tunnel members of the FM10000 switch
 *                  extension.
 *
 * \note            This function is called from fm10000InitSwitch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000TunnelInit(fm_int sw)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *        switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status               err = FM_OK;
    fm_int                  destBase;
    fm_glortDestEntry *     destEntry[FM10000_TE_DGLORT_MAP_ENTRIES_1];
    fm_uint32               camIndex;
    fm_int                  i;
    fm_logicalPortInfo *    lportInfo;
    fm_portmask             activeDestMask;
    fm_islTagFormat         islTagFormat;
    fm_fm10000TeSGlort      teSGlort;
    fm_fm10000TeTrapCfg     teTrapCfg;
    fm_bool                 learning;
    fm_uint32               parser;
    fm_bool                 isInternal;
    fm_bool                 routable;
    fm_bool                 lbs;
    fm_bitArray             portMask;
    fm_bool                 portMaskAllocated = FALSE;
    fm_int                  cpi;
    fm_int                  logPort;
    fm_int                  fabricPort;
    fm_uint32               routeUpdateFields;
    fm_fm10000TeGlortCfg    teGlortCfg;
    fm_fm10000TeTunnelCfg   tunnelCfg;
    fm_fm10000TeChecksumCfg teChecksumCfg;
    fm_fm10000TeParserCfg   parserCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    if (switchExt->tunnelCfg != NULL)
    {
        FreeTunnelCfgStruct(switchExt->tunnelCfg);
        switchExt->tunnelCfg = NULL;
    }

    switchExt->tunnelCfg = fmAlloc( sizeof(fm_fm10000TunnelCfg) );

    if (switchExt->tunnelCfg == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    FM_CLEAR(*switchExt->tunnelCfg);

    err = InitializeTunnelCfgStruct(switchExt->tunnelCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /* Find the logical port numbers of TE ports */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        /* Get logical port number */
        logPort = GET_LOGICAL_PORT(sw, cpi);

        err = fm10000MapLogicalPortToFabricPort(sw, logPort, &fabricPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        if (fabricPort == FM10000_TE_TO_FABRIC_PORT(0))
        {
            switchExt->tunnelCfg->tunnelPort[0] = logPort;
        }
        else if (fabricPort == FM10000_TE_TO_FABRIC_PORT(1))
        {
            switchExt->tunnelCfg->tunnelPort[1] = logPort;
        }
    }

    if ( (switchExt->tunnelCfg->tunnelPort[0] == -1) &&
         (switchExt->tunnelCfg->tunnelPort[1] == -1))
    {
        /* The TE ports are not included in the scheduler, therefore
         * tunnel API cannot be used. Skip the rest of the configuration */
        goto ABORT;
    }

    /* Allocate 2 DestEntry for the two TE ports */
    err = fmAllocDestEntries(sw,
                             FM10000_TE_DGLORT_MAP_ENTRIES_1,
                             NULL,
                             destEntry,
                             FM_PORT_TYPE_SPECIAL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    islTagFormat = FM_ISL_TAG_F56;
    learning = FALSE;
    parser = FM_PORT_PARSER_STOP_AFTER_L4;
    isInternal = TRUE;
    routable = TRUE;
    lbs = FALSE;
    routeUpdateFields = 0;

    err = fmCreateBitArray(&portMask, switchPtr->numCardinalPorts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    portMaskAllocated = TRUE;

    /* Set specific tunnel port default value */
    for (i = 0 ; i < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; i++)
    {
        /* Skip TE if not present in schedule */
        if ((switchExt->tunnelCfg->tunnelPort[i] == -1))
        {
            continue;
        }

        err = fm10000GetPortAttribute(sw,
                                  switchExt->tunnelCfg->tunnelPort[i],
                                  FM_PORT_ACTIVE_MAC,
                                  FM_PORT_LANE_NA,
                                  FM_PORT_MASK_WIDE,
                                  (void *) &portMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        if ((switchExt->tunnelCfg->tunnelPort[0] != -1))
        {
            err = fmSetPortInBitArray(sw,
                                      &portMask,
                                      switchExt->tunnelCfg->tunnelPort[0],
                                      FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        if ((switchExt->tunnelCfg->tunnelPort[1] != -1))
        {
            err = fmSetPortInBitArray(sw,
                                      &portMask,
                                      switchExt->tunnelCfg->tunnelPort[1],
                                      FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        err = fmAssignPortToPortMask(sw,
                                     &activeDestMask,
                                     switchExt->tunnelCfg->tunnelPort[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetGlortDestMask(sw,
                                      destEntry[i],
                                      &activeDestMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_ISL_TAG_FORMAT,
                                      (void *) &islTagFormat);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_INTERNAL,
                                      (void *) &isInternal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_LEARNING,
                                      (void *) &learning);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_PARSER,
                                      (void *) &parser);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_ROUTABLE,
                                      (void *) &routable);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_ROUTED_FRAME_UPDATE_FIELDS,
                                      (void *) &routeUpdateFields);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_LOOPBACK_SUPPRESSION,
                                      (void *) &lbs);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortAttribute(sw,
                                      switchExt->tunnelCfg->tunnelPort[i],
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_MASK_WIDE,
                                      (void *) &portMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetPortState(sw,
                                  switchExt->tunnelCfg->tunnelPort[i],
                                  FM_PORT_ACTIVE_MAC,
                                  FM_PORT_STATE_UP,
                                  0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    destBase = destEntry[0]->destIndex;

    /* Create an entry to forward 0x8000/0x8000 to the proper TE.
     * 0x8000/0xC000 --> TE0, 0xC000/0xC000 --> TE1 */
    err = fmCreateGlortCamEntry(sw,
                                0x8000,
                                0x8000,
                                FM_GLORT_ENTRY_TYPE_STRICT,
                                destBase,
                                FM10000_TE_DGLORT_MAP_ENTRIES_1,/* hash count */
                                1,                      /* A length */
                                14,                     /* A offset */
                                0,                      /* B length */
                                0,                      /* B offset */
                                FM_GLORT_ENTRY_HASH_A,
                                0,                      /* dglortTag */
                                &camIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    lportInfo = &switchPtr->logicalPortInfo;

    teSGlort.glortValue = 0;
    teSGlort.glortMask = 0;
    teSGlort.vsiStart = 0;
    teSGlort.vsiLength = 0;
    teSGlort.vsiOffset = 0;

    teGlortCfg.decapDglort = 0;
    teGlortCfg.encapDglort = 0;

    FM_CLEAR(tunnelCfg);
    tunnelCfg.l4DstVxLan     = switchExt->vnVxlanUdpPort;
    tunnelCfg.ttl            = switchExt->vnOuterTTL;
    tunnelCfg.tos            = 0;
    tunnelCfg.deriveOuterTOS = TRUE;
    tunnelCfg.ngeMask        = 0;
    tunnelCfg.ngeTime        = FALSE;
    tunnelCfg.dmac           = 0;
    tunnelCfg.smac           = 0;
    tunnelCfg.encapProtocol  = GET_PROPERTY()->vnEncapProtocol;
    tunnelCfg.encapVersion   = GET_PROPERTY()->vnEncapVersion;

    teChecksumCfg.notIp = FM_FM10000_TE_CHECKSUM_COMPUTE;
    teChecksumCfg.notTcpOrUdp = FM_FM10000_TE_CHECKSUM_COMPUTE;
    teChecksumCfg.tcpOrUdp = FM_FM10000_TE_CHECKSUM_HEADER;

    parserCfg.vxLanPort      = switchExt->vnVxlanUdpPort;

    for (i = 0 ; i < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; i++)
    {
        destEntry[i]->owner = &lportInfo->camEntries[camIndex];

        if (switchExt->tunnelCfg->tunnelMode[i] == FM_TUNNEL_API_MODE_VXLAN_NVGRE_NGE)
        {
            tunnelCfg.l4DstNge = switchExt->vnGeneveUdpPort;
            parserCfg.ngePort  = switchExt->vnGeneveUdpPort;
        }
        else
        {
            tunnelCfg.l4DstNge = switchExt->vnGpeUdpPort;
            parserCfg.ngePort  = switchExt->vnGpeUdpPort;
        }

        /* Entry 0 is always match by default */
        err = fm10000SetTeSGlort(sw, i, 0, &teSGlort, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetTeDefaultGlort(sw,
                                       i,
                                       &teGlortCfg,
                                       FM10000_TE_DEFAULT_GLORT_ENCAP_DGLORT |
                                       FM10000_TE_DEFAULT_GLORT_DECAP_DGLORT,
                                       FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetTeDefaultTunnel(sw,
                                        i,
                                        &tunnelCfg,
                                        FM10000_TE_DEFAULT_TUNNEL_ALL |
                                        FM10000_TE_DEFAULT_TUNNEL_NGE_ALL,
                                        FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetTeChecksum(sw,
                                   i,
                                   &teChecksumCfg,
                                   FM10000_TE_CHECKSUM_NOT_IP |
                                   FM10000_TE_CHECKSUM_NOT_TCP_OR_UDP |
                                   FM10000_TE_CHECKSUM_TCP_OR_UDP,
                                   FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000SetTeParser(sw,
                                 i,
                                 &parserCfg,
                                 FM10000_TE_PARSER_VXLAN_PORT |
                                 FM10000_TE_PARSER_NGE_PORT,
                                 FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Initialize trap DGLORT */
    teTrapCfg.trapGlort = (switchPtr->glortInfo.cpuBase & 0xFF00) | FM10000_TUNNEL_0_TRAP_CODE_BASE;
    err = fm10000SetTeTrap(sw, 0, &teTrapCfg, FM10000_TE_TRAP_BASE_DGLORT, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    teTrapCfg.trapGlort = (switchPtr->glortInfo.cpuBase & 0xFF00) | FM10000_TUNNEL_1_TRAP_CODE_BASE;
    err = fm10000SetTeTrap(sw, 1, &teTrapCfg, FM10000_TE_TRAP_BASE_DGLORT, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:

    if (portMaskAllocated)
    {
        fmDeleteBitArray(&portMask);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000TunnelInit */




/*****************************************************************************/
/** fm10000TunnelFree
 * \ingroup intTunnel
 *
 * \desc            Free the memory used by the tunnel members of the
 *                  FM10000 switch extension.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000TunnelFree(fm_int sw)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    if (switchExt->tunnelCfg != NULL)
    {
        FreeTunnelCfgStruct(switchExt->tunnelCfg);
        switchExt->tunnelCfg = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, FM_OK);

}   /* end fm10000TunnelFree */




/*****************************************************************************/
/** fm10000CreateTunnel
 * \ingroup intTunnel
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
fm_status fm10000CreateTunnel(fm_int          sw,
                              fm_int *        group,
                              fm_tunnelParam *tunnelParam)
{
    fm_int               index;
    fm_uint16            baseLookup[FM10000_TE_DGLORT_MAP_ENTRIES_0];
    fm_uint16            sizeLookup[FM10000_TE_DGLORT_MAP_ENTRIES_0];
    fm_uint16            glortValue[FM10000_TE_DGLORT_MAP_ENTRIES_0];
    fm_uint16            glortMask[FM10000_TE_DGLORT_MAP_ENTRIES_0];
    fm_int               entry;
    fm_int               i;
    fm_int               j;
    fm_int               binSize;
    fm_uint16            tmpValue;
    fm_uint16            prevBaseLookup;
    fm_uint16            prevSizeLookup;
    fm_uint16            glortSize;
    fm_uint16            glortSizeMask;
    fm_uint16            prevGlortValue;
    fm_uint16            prevBaseGlortValue;
    fm_uint16            prevGlortMask;
    fm_uint16            prevGlortSize;
    fm_uint16            maxGlortSize;
    fm_int               holeSize;
    fm_int               currentHoleSize;
    fm_fm10000TunnelGrp *insertedGrp;
    fm_switch *          switchPtr;
    fm10000_switch *     switchExt;
    fm_status            err;
    fm_bool              tunnelLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    switchPtr          = GET_SWITCH_PTR(sw);
    switchExt          = (fm10000_switch *) switchPtr->extension;
    err                = FM_OK;
    tunnelLockTaken    = FALSE;

    FM_CLEAR(baseLookup);
    FM_CLEAR(sizeLookup);
    FM_CLEAR(glortValue);
    FM_CLEAR(glortMask);


    /* Validating the input argument */
    if ( (group == NULL) || (tunnelParam == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (tunnelParam->te >= FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (tunnelParam->te < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (switchExt->tunnelCfg->tunnelPort[tunnelParam->te] == -1)
    {
        err = FM_ERR_LOG_PORT_REQUIRED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (tunnelParam->size >= FM10000_TE_LOOKUP_ENTRIES_0) ||
         (tunnelParam->size <= 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( tunnelParam->tepSize &&
         ((tunnelParam->hashKeyConfig & FM_TUNNEL_MATCH_VSI_TEP) == 0) )
    {
        err = FM_ERR_TUNNEL_TEP_SIZE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    /* Find free slot for this group in this te */
    for (index = 0 ; index < FM10000_TE_DGLORT_MAP_ENTRIES_0; index++)
    {
        if (switchExt->tunnelCfg->tunnelGrp[tunnelParam->te][index].active == FALSE)
        {
            break;
        }
    }

    /* no free slot available */
    if (index == FM10000_TE_DGLORT_MAP_ENTRIES_0)
    {
        err = FM_ERR_TUNNEL_NO_FREE_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    insertedGrp = &switchExt->tunnelCfg->tunnelGrp[tunnelParam->te][index];

    /* Initialize the group */
    insertedGrp->tunnelParam = *tunnelParam;
    insertedGrp->teDGlort.encap = tunnelParam->encap;
    insertedGrp->teDGlort.setSGlort = FALSE;
    insertedGrp->teDGlort.setDGlort = FALSE;
    insertedGrp->lookupBinFirstFreeEntry = 0;

    /* hash lookup type */
    if (tunnelParam->hashKeyConfig)
    {
        insertedGrp->teDGlort.lookupType = FM_FM10000_TE_LOOKUP_HASH;
        insertedGrp->teDGlort.lookupData.hashLookup.hashKeyConfig =
            TunnelToTeHashKey(tunnelParam->hashKeyConfig, NULL);

        /* Round up the size to the nearest power of 2 */
        binSize = 1;
        while (binSize < tunnelParam->size)
        {
            binSize <<= 1;
        }
        insertedGrp->teDGlort.lookupData.hashLookup.hashSize = binSize;
        insertedGrp->teDGlort.lookupData.hashLookup.tepStart = 0;
        insertedGrp->teDGlort.lookupData.hashLookup.tepWidth = 0;
    }
    /* direct lookup type */
    else
    {
        insertedGrp->teDGlort.lookupType = FM_FM10000_TE_LOOKUP_DIRECT;
        insertedGrp->teDGlort.lookupData.directLookup.indexStart = 0;
        insertedGrp->teDGlort.lookupData.directLookup.indexWidth = 0;
    }

    entry = 0;
    /* capture all lookup and base/user information from all groups */
    for (i = 0 ; i < FM10000_TE_DGLORT_MAP_ENTRIES_0; i++)
    {
        if (switchExt->tunnelCfg->tunnelGrp[tunnelParam->te][i].active)
        {
            baseLookup[entry] = switchExt->tunnelCfg->tunnelGrp[tunnelParam->te]
                                                               [i].teDGlort.baseLookup;
            sizeLookup[entry] = switchExt->tunnelCfg->tunnelGrp[tunnelParam->te]
                                                               [i].tunnelParam.size;
            glortValue[entry] = switchExt->tunnelCfg->tunnelGrp[tunnelParam->te]
                                                               [i].teDGlort.glortValue;
            glortMask[entry] = switchExt->tunnelCfg->tunnelGrp[tunnelParam->te]
                                                              [i].teDGlort.glortMask;

            entry++;
        }
    }

    /* Sort the lookup reserved range to find first empty block big enough */
    for (i = 0 ; i < (entry - 1) ; i++)
    {
        for (j = 0 ; j < (entry - i - 1) ; j++)
        {
            /* Sort is based on lookup start position */
            if (baseLookup[j] > baseLookup[j + 1])
            {
                tmpValue = baseLookup[j];
                baseLookup[j] = baseLookup[j + 1];
                baseLookup[j + 1] = tmpValue;

                tmpValue = sizeLookup[j];
                sizeLookup[j] = sizeLookup[j + 1];
                sizeLookup[j + 1] = tmpValue;
            }
        }
    }

    prevBaseLookup = 0;
    prevSizeLookup = 0;
    /* Walk over sorted lookup range */
    for (i = 0 ; i < entry ; i++)
    {
        /* Found position large enough for this new block */
        if ((baseLookup[i] - (prevBaseLookup + prevSizeLookup)) >= tunnelParam->size)
        {
            insertedGrp->teDGlort.baseLookup = prevBaseLookup + prevSizeLookup;
            break;
        }
        prevBaseLookup = baseLookup[i];
        prevSizeLookup = sizeLookup[i];
    }

    /* No hole found, see if the block can be inserted at the end */
    if (i == entry)
    {
        if ((FM10000_TE_LOOKUP_ENTRIES_0 - (prevBaseLookup + prevSizeLookup)) >=
             tunnelParam->size)
        {
            insertedGrp->teDGlort.baseLookup = prevBaseLookup + prevSizeLookup;
        }
        else
        {
            err = FM_ERR_TUNNEL_LOOKUP_FULL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    /* Sort the glort range to find an empty range */
    for (i = 0 ; i < (entry - 1) ; i++)
    {
        for (j = 0 ; j < (entry - i - 1) ; j++)
        {
            /* Sort is based on glort value */
            if (glortValue[j] > glortValue[j + 1])
            {
                tmpValue = glortMask[j];
                glortMask[j] = glortMask[j + 1];
                glortMask[j + 1] = tmpValue;

                tmpValue = glortValue[j];
                glortValue[j] = glortValue[j + 1];
                glortValue[j + 1] = tmpValue;
            }
        }
    }

    /* hash lookup type */
    if (insertedGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
    {
        glortSize = tunnelParam->tepSize;
    }
    /* direct lookup type */
    else
    {
        glortSize = tunnelParam->size;
    }

    /* Always uses at least one glort value */
    if (glortSize == 0)
    {
        glortSize = 1;
        glortSizeMask = 0xFFFF;
    }
    else
    {
        /* Find glort next glort size which is exponent of 2 */
        glortSize = (glortSize << 1) - 1;

        for (i = 15 ; i > 0 ; i--)
        {
            if ((1 << i) & glortSize)
            {
                glortSize = (1 << i);
                break;
            }
        }

        /* hash lookup type */
        if (insertedGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
        {
            insertedGrp->teDGlort.lookupData.hashLookup.tepWidth = i;

            /* Uses the top 6 bit of user since the last 2 bits are reserved */
            if (tunnelParam->useUser)
            {
                insertedGrp->teDGlort.lookupData.hashLookup.tepStart = 2;
                glortSize >>= 6;

                /* Always uses at least one glort value */
                if (glortSize == 0)
                {
                    glortSize = 1;
                }
            }
            else
            {
                insertedGrp->teDGlort.lookupData.hashLookup.tepStart = 8;
            }
        }
        /* direct lookup type */
        else
        {
            insertedGrp->teDGlort.lookupData.directLookup.indexWidth = i;

            /* Uses the top 6 bit of user since the last 2 bits are reserved */
            if (tunnelParam->useUser)
            {
                insertedGrp->teDGlort.lookupData.directLookup.indexStart = 2;
                glortSize >>= 6;

                /* Always uses at least one glort value */
                if (glortSize == 0)
                {
                    glortSize = 1;
                }
            }
            else
            {
                insertedGrp->teDGlort.lookupData.directLookup.indexStart = 8;
            }
        }

        glortSizeMask = ~(glortSize - 1);
    }

    /* Tunnel GLORT have bit 15 set to 1. bit 14 is the TE engine. */
    prevGlortValue = (0x8000 | (tunnelParam->te << 14)) - 1;
    prevGlortMask = 0xFFFF;
    currentHoleSize = 0x4000;

    /* Find the smallest hole possible */
    for (i = 0 ; i < entry ; i++)
    {
        prevGlortSize = ((~prevGlortMask) & 0xFFFF) + 1;
        maxGlortSize = (prevGlortSize > glortSize) ? prevGlortSize : glortSize;
        prevBaseGlortValue = prevGlortValue & ~(maxGlortSize - 1);

        holeSize = glortValue[i] - (fm_int)(prevBaseGlortValue + maxGlortSize);
        if ( (holeSize >= glortSize) &&
             (currentHoleSize > holeSize) )
        {
            insertedGrp->teDGlort.glortValue = prevBaseGlortValue + maxGlortSize;
            insertedGrp->teDGlort.glortMask = glortSizeMask;
            currentHoleSize = holeSize;
        }
        prevGlortValue = glortValue[i];
        prevGlortMask = glortMask[i];
    }

    /* First entry */
    if (entry == 0)
    {
        /* If this is the first entry, the total range defines the maximum
         * available. */
        if (glortSize > currentHoleSize)
        {
            err = FM_ERR_TUNNEL_GLORT_FULL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        insertedGrp->teDGlort.glortValue = 0x8000 | (tunnelParam->te << 14);
        insertedGrp->teDGlort.glortMask = glortSizeMask;
    }
    /* Look if the last hole is a better option */
    else
    {
        prevGlortSize = ((~prevGlortMask) & 0xFFFF) + 1;
        maxGlortSize = (prevGlortSize > glortSize) ? prevGlortSize : glortSize;
        prevBaseGlortValue = prevGlortValue & ~(maxGlortSize - 1);

        holeSize = (0x8000 | (tunnelParam->te << 14) | 0x3FFF) -
                   (fm_int)(prevBaseGlortValue + maxGlortSize) + 1;
        if ( (holeSize >= glortSize) &&
             (currentHoleSize > holeSize) )
        {
            insertedGrp->teDGlort.glortValue = prevBaseGlortValue + maxGlortSize;
            insertedGrp->teDGlort.glortMask = glortSizeMask;
            currentHoleSize = holeSize;
        }

        /* No position found */
        else if (currentHoleSize == 0x4000)
        {
            err = FM_ERR_TUNNEL_GLORT_FULL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    /* Configure user as part of the matching if set */
    insertedGrp->teDGlort.userValue = 0;
    if (tunnelParam->useUser)
    {
        insertedGrp->teDGlort.userMask = 0xFC;
    }
    else
    {
        insertedGrp->teDGlort.userMask = 0x0;
    }

    err = fm10000SetTeDGlort(sw,
                             tunnelParam->te,
                             index,
                             &insertedGrp->teDGlort,
                             TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    *group = (tunnelParam->te * FM10000_TE_DGLORT_MAP_ENTRIES_0) + index;
    insertedGrp->active = TRUE;


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000CreateTunnel */




/*****************************************************************************/
/** fm10000DeleteTunnel
 * \ingroup intTunnel
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
fm_status fm10000DeleteTunnel(fm_int sw, fm_int group)
{
    fm_fm10000TunnelGrp *removedGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_treeIterator itEntry;
    fm_uint64 key;
    void *value;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    removedGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (removedGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Iterate through all rules and encap flow and remove them one by one for
     * now. This could be enhanced in the future for better performance. */
    for (fmTreeIterInit(&itEntry, &removedGrp->rules) ;
          (err = fmTreeIterNext(&itEntry, &key, &value)) == FM_OK ; )
    {
        err = fm10000DeleteTunnelRule(sw, group, key);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Reinit the iterator since the tree was modified during the delete. */
        fmTreeIterInit(&itEntry, &removedGrp->rules);
    }
    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    err = FM_OK;

    for (fmTreeIterInit(&itEntry, &removedGrp->encapFlows) ;
          (err = fmTreeIterNext(&itEntry, &key, &value)) == FM_OK ; )
    {
        err = fm10000DeleteTunnelEncapFlow(sw, group, key);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Reinit the iterator since the tree was modified during the delete. */
        fmTreeIterInit(&itEntry, &removedGrp->encapFlows);
    }
    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    err = FM_OK;

    /* lookup bins should have been cleared when removing all rules. */
    if (fmTreeSize(&removedGrp->lookupBins) != 0)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Invalidate the entry */
    removedGrp->teDGlort.glortMask = 0x0;
    removedGrp->teDGlort.glortValue = 0xFFFF;
    err = fm10000SetTeDGlort(sw,
                             group >> 3,
                             group & 0x7,
                             &removedGrp->teDGlort,
                             TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    removedGrp->active = FALSE;


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000DeleteTunnel */




/*****************************************************************************/
/** fm10000GetTunnel
 * \ingroup intTunnel
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
fm_status fm10000GetTunnel(fm_int          sw,
                           fm_int          group,
                           fm_tunnelParam *tunnelParam)
{
    fm_fm10000TunnelGrp *retrievedGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (tunnelParam == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    retrievedGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (retrievedGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    *tunnelParam = retrievedGrp->tunnelParam;


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnel */




/*****************************************************************************/
/** fm10000GetTunnelFirst
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelFirst(fm_int sw, fm_int *firstGroup)
{
    fm_int group;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    if (firstGroup == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    /* Find first group */
    for (group = 0 ;
         group < (FM10000_TE_DGLORT_MAP_ENTRIES_1 * FM10000_TE_DGLORT_MAP_ENTRIES_0) ;
         group++)
    {
        if (switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7].active == TRUE)
        {
            break;
        }
    }

    if (group == (FM10000_TE_DGLORT_MAP_ENTRIES_1 * FM10000_TE_DGLORT_MAP_ENTRIES_0))
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    else
    {
        *firstGroup = group;
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelFirst */




/*****************************************************************************/
/** fm10000GetTunnelNext
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelNext(fm_int  sw,
                               fm_int  currentGroup,
                               fm_int *nextGroup)
{
    fm_int group;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    if (nextGroup == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (currentGroup >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                          FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (currentGroup < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    /* Find next group */
    for (group = currentGroup + 1 ;
         group < (FM10000_TE_DGLORT_MAP_ENTRIES_1 * FM10000_TE_DGLORT_MAP_ENTRIES_0) ;
         group++)
    {
        if (switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7].active == TRUE)
        {
            break;
        }
    }

    if (group == (FM10000_TE_DGLORT_MAP_ENTRIES_1 * FM10000_TE_DGLORT_MAP_ENTRIES_0))
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    else
    {
        *nextGroup = group;
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelNext */




/*****************************************************************************/
/** fm10000AddTunnelEncapFlow
 * \ingroup intTunnel
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
fm_status fm10000AddTunnelEncapFlow(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int                   encapFlow,
                                    fm_tunnelEncapFlow       field,
                                    fm_tunnelEncapFlowParam *param)
{
    fm_fm10000TunnelGrp *            tunnelGrp;
    fm_switch *                      switchPtr;
    fm10000_switch *                 switchExt;
    fm_status                        err;
    fm_bool                          tunnelLockTaken;
    void *                           value;
    fm_int                           blockLength;
    fm_fm10000TeData                 teData;
    fm_uint16                        baseIndex;
    fm_fm10000EncapFlow *            encapFlowEntry;
    fm_int                           i;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d, field = 0x%x\n",
                 sw, group, encapFlow, field);

    tunnelGrp       = NULL;
    switchPtr       = GET_SWITCH_PTR(sw);
    switchExt       = (fm10000_switch *) switchPtr->extension;
    err             = FM_OK;
    tunnelLockTaken = FALSE;

    FM_CLEAR(teData);

    /* Validate input argument */
    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (param == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    /* Verify Tunnel Mode is compatible */
    if (param->type == FM_TUNNEL_TYPE_NGE ||
        param->type == FM_TUNNEL_TYPE_NVGRE)
    {
        if (switchExt->tunnelCfg->tunnelMode[group >> 3] !=
            FM_TUNNEL_API_MODE_VXLAN_NVGRE_NGE)
        {
            err = FM_ERR_TUNNEL_TYPE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }
    else if (param->type == FM_TUNNEL_TYPE_GPE_NSH ||
             param->type == FM_TUNNEL_TYPE_GPE)
    {
        if (switchExt->tunnelCfg->tunnelMode[group >> 3] !=
            FM_TUNNEL_API_MODE_VXLAN_GPE_NSH)
        {
            err = FM_ERR_TUNNEL_TYPE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* encap flow already exist? */
    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) == FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Shared entry must be inserted into the table */
    if (param->shared)
    {
        err = EncapFlowToTeData(field, param, &teData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000GetTeDataBlockLength(&teData, 1, &blockLength);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Find a free counter index if needed */
        if (teData.blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
        {
            err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                      1,
                                      FALSE,
                                      &i);
            if (i < 0)
            {
                err = FM_ERR_TUNNEL_COUNT_FULL;
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            teData.blockVal.tunnelVal.counterIdx = i;
        }

        /* Find a free block large enough */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Clear the counter if needed */
        if (teData.blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
        {
            err = fm10000SetTeFlowCnt(sw,
                                      group >> 3,
                                      teData.blockVal.tunnelVal.counterIdx,
                                      0LL,
                                      0LL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Configure the encap flow into the table. */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               &teData,
                               1,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        encapFlowEntry = fmAlloc(sizeof(fm_fm10000EncapFlow));

        if (encapFlowEntry == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*encapFlowEntry);

        encapFlowEntry->field = field;
        encapFlowEntry->param = *param;
        encapFlowEntry->teLookup.dataPtr = baseIndex;
        encapFlowEntry->teLookup.dataLength = blockLength;
        encapFlowEntry->teLookup.last = TRUE;
        encapFlowEntry->counter = teData.blockVal.tunnelVal.counterIdx;
        fmTreeInit(&encapFlowEntry->rules);

        err = fmTreeInsert(&tunnelGrp->encapFlows, encapFlow, encapFlowEntry);
        if (err != FM_OK)
        {
            fmFreeEncapFlow(encapFlowEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Reserve the counter index if properly configured */
        if (teData.blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
        {
            /* Should never fail since this bit was already reserved */
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   encapFlowEntry->counter,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        teDataBlkCtrl = fmAlloc(sizeof(fm_fm10000TunnelTeDataBlockCtrl));

        if (teDataBlkCtrl == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*teDataBlkCtrl);
        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;
        teDataBlkCtrl->tunnelGrp = group;
        teDataBlkCtrl->tunnelDataType = FM_FM10000_TUNNEL_TE_DATA_TYPE_ENCAPFLOW;
        teDataBlkCtrl->tunnelEntry = encapFlow;

        /* Should never fail since this block was already validated */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    else
    {
        /* Not Supporting count action on unshared encap flow. Use rule count
         * instead. */
        if (field & FM_TUNNEL_ENCAP_FLOW_COUNTER)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        encapFlowEntry = fmAlloc(sizeof(fm_fm10000EncapFlow));

        if (encapFlowEntry == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*encapFlowEntry);

        encapFlowEntry->field = field;
        encapFlowEntry->param = *param;
        encapFlowEntry->teLookup.dataPtr = 0;
        encapFlowEntry->teLookup.dataLength = 0;
        encapFlowEntry->teLookup.last = TRUE;
        encapFlowEntry->counter = 0;
        fmTreeInit(&encapFlowEntry->rules);

        /* unshared entry are inserted into the tree but are not applied to
         * the hardware. The encap flow information will be replicated on
         * every rule that make uses of it. */
        err = fmTreeInsert(&tunnelGrp->encapFlows, encapFlow, encapFlowEntry);
        if (err != FM_OK)
        {
            fmFreeEncapFlow(encapFlowEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000AddTunnelEncapFlow */




/*****************************************************************************/
/** fm10000DeleteTunnelEncapFlow
 * \ingroup intTunnel
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
fm_status fm10000DeleteTunnelEncapFlow(fm_int sw,
                                       fm_int group,
                                       fm_int encapFlow)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000EncapFlow *encapFlowEntry;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d\n",
                 sw, group, encapFlow);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    encapFlowEntry = (fm_fm10000EncapFlow *) value;

    /* Only remove encap flow that are currently not in use */
    if (fmTreeSize(&encapFlowEntry->rules))
    {
        err = FM_ERR_TUNNEL_IN_USE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (encapFlowEntry->param.shared)
    {
        /* Free resource allocation */
        err = FreeTeDataBlock(sw,
                              group >> 3,
                              encapFlowEntry->teLookup.dataPtr,
                              encapFlowEntry->teLookup.dataLength,
                              &teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        fmFree(teDataBlkCtrl);

        if (encapFlowEntry->counter)
        {
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   encapFlowEntry->counter,
                                   FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }

    err = fmTreeRemoveCertain(&tunnelGrp->encapFlows, encapFlow, fmFreeEncapFlow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000DeleteTunnelEncapFlow */




/*****************************************************************************/
/** fm10000UpdateTunnelEncapFlow
 * \ingroup intTunnel
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
fm_status fm10000UpdateTunnelEncapFlow(fm_int                   sw,
                                       fm_int                   group,
                                       fm_int                   encapFlow,
                                       fm_tunnelEncapFlow       field,
                                       fm_tunnelEncapFlowParam *param)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_int blockLength;
    fm_fm10000TeData teData;
    fm_uint16 baseIndex;
    fm_fm10000EncapFlow *encapFlowEntry;
    fm_int i;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;
    fm_uint16 upperBound;
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_fm10000TeLookup oldTeLookup;
    fm_fm10000TeLookup newTeLookup;
    fm_fm10000TunnelLookupBin *lookupBin;
    fm_treeIterator itRule;
    fm_treeIterator itRuleBin;
    fm_uint64 nextKey;
    fm_uint64 ruleNumber;
    fm_fm10000TunnelRule *tunnelRule;
    fm_fm10000TeData teDataArray[FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE];
    fm_int teDataPos;
    fm_int teDataOutSet;
    fm_tunnelEncapFlow originalField;
    fm_tunnelEncapFlowParam originalParam;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d, field = 0x%x\n",
                 sw, group, encapFlow, field);

    /* Validate input argument */
    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (param == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* encap flow already exist? */
    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_NO_ENCAP_FLOW;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    encapFlowEntry = (fm_fm10000EncapFlow *)value;

    /* Can't update the sharing argument of an encap flow */
    if (encapFlowEntry->param.shared != param->shared)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Process shared entry */
    if (param->shared)
    {
        err = EncapFlowToTeData(field, param, &teData);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fm10000GetTeDataBlockLength(&teData, 1, &blockLength);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Find a free counter index if needed */
        if (teData.blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
        {
            /* Only pick a counter index if not already defined */
            if (encapFlowEntry->counter == 0)
            {
                err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                          1,
                                          FALSE,
                                          &i);
                if (i < 0)
                {
                    err = FM_ERR_TUNNEL_COUNT_FULL;
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                teData.blockVal.tunnelVal.counterIdx = i;
            }
            else
            {
                teData.blockVal.tunnelVal.counterIdx = encapFlowEntry->counter;
            }
        }

        /* Find a free teData block */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);

        /* No Free Data block big enough available, using swap */
        if (err == FM_ERR_TUNNEL_FLOW_FULL)
        {
            /* Using swap only works if the block can be moved back to the
             * previous location */
            if (blockLength > encapFlowEntry->teLookup.dataLength)
            {
                err = FM_ERR_TUNNEL_FLOW_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[group >> 3];

            upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

            /* Save the old index since the lookup bin will be updated after
             * the move call. */
            baseIndex = encapFlowEntry->teLookup.dataPtr;

            err = MoveTeDataBlock(sw,
                                  group >> 3,
                                  encapFlowEntry->teLookup.dataPtr,
                                  encapFlowEntry->teLookup.dataLength,
                                  upperBound);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* do we need to adjust the free ptr? */
            if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
            {
                teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
            }
        }
        else if (err != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Write the updated block to a new location if free block found.
         * Otherwise, the position used will be the same as the previous
         * block. In case no free block was found, the swap copy is the one
         * that currently handle that flow. */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               &teData,
                               1,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* oldTeLookup refer to the actual used location */
        oldTeLookup = encapFlowEntry->teLookup;

        /* update the location to refer to the updated one */
        encapFlowEntry->teLookup.dataPtr = baseIndex;
        encapFlowEntry->teLookup.dataLength = blockLength;
        encapFlowEntry->teLookup.last = TRUE;

        /* Go over all the rules that refer to that shared encap flow and
         * update them. The update is done without any movement since the
         * block is rebuilt exactly as the original except for the shared
         * encap flow pointer which is updated atomically. */
        for (fmTreeIterInit(&itRule, &encapFlowEntry->rules) ;
              (err = fmTreeIterNext(&itRule, &nextKey, &value)) == FM_OK ; )
        {
            err = fmTreeFind(&tunnelGrp->rules, nextKey, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            tunnelRule = (fm_fm10000TunnelRule *)value;

            err = fmTreeFind(&tunnelGrp->lookupBins,
                             tunnelRule->lookupBin,
                             &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin = (fm_fm10000TunnelLookupBin *)value;
            teDataPos = 0;

            FM_MEMSET_S( teDataArray,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                         0,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

            /* Rebuild the entire bin in the same sorting order as it was
             * created to match it with the exeption of the new encap flow
             * position. */
            for (fmTreeIterInit(&itRuleBin, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRuleBin, &ruleNumber, &value)) == FM_OK ; )
            {
                err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                err = TunnelRuleToTeData(tunnelGrp,
                                         (fm_fm10000TunnelRule*) value,
                                         &teDataArray[teDataPos],
                                         FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                         &teDataOutSet);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                teDataPos += teDataOutSet;
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* Update the block */
            err = fm10000SetTeData(sw,
                                   group >> 3,
                                   lookupBin->teLookup.dataPtr,
                                   teDataArray,
                                   teDataPos,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        err = FM_OK;

        /* Make sure all the register write are completed before the function
         * return. This is to avoid race condition. */
        err = fm10000SyncTeDataLookup(sw, group >> 3);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


        /* Should never fail since this block was already reserved. This
         * could be either the old position or the swap copy. */
        err = FreeTeDataBlock(sw,
                              group >> 3,
                              oldTeLookup.dataPtr,
                              oldTeLookup.dataLength,
                              &teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Reserve the counter index */
        if (teData.blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
        {
            if (encapFlowEntry->counter == 0)
            {
                /* Should never fail since this bit was already reserved */
                err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                       teData.blockVal.tunnelVal.counterIdx,
                                       TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                err = fm10000SetTeFlowCnt(sw,
                                          group >> 3,
                                          teData.blockVal.tunnelVal.counterIdx,
                                          0LL,
                                          0LL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }
        /* Free the counter index */
        else if (encapFlowEntry->counter)
        {
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   encapFlowEntry->counter,
                                   FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Update the encap flow structure information */
        encapFlowEntry->field = field;
        encapFlowEntry->param = *param;
        encapFlowEntry->counter = teData.blockVal.tunnelVal.counterIdx;

        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;

        /* Should never fail since this block was already validated */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* Process unshared entry */
    else
    {
        /* Not Supporting count action on unshared encap flow. Use rule count
         * instead. */
        if (field & FM_TUNNEL_ENCAP_FLOW_COUNTER)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Save original configuration. This would be restored if the update
         * did not successefully complete. */
        originalField = encapFlowEntry->field;
        originalParam = encapFlowEntry->param;

        /* Update the field and parameter of the encap flow that would be used
         * to update every rules that uses that configuration. */
        encapFlowEntry->field = field;
        encapFlowEntry->param = *param;

        /* Go over all the rules that refer to that unshared encap flow and
         * update them. The update is done by rebuilding updated blocks into
         * a new location if one free position is found. If no room is found
         * and the original block has the same size as the updated version,
         * the update will uses the swap area to complete. */
        for (fmTreeIterInit(&itRule, &encapFlowEntry->rules) ;
              (err = fmTreeIterNext(&itRule, &nextKey, &value)) == FM_OK ; )
        {
            err = fmTreeFind(&tunnelGrp->rules, nextKey, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            tunnelRule = (fm_fm10000TunnelRule *)value;

            err = fmTreeFind(&tunnelGrp->lookupBins,
                             tunnelRule->lookupBin,
                             &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin = (fm_fm10000TunnelLookupBin *)value;
            teDataPos = 0;

            FM_MEMSET_S( teDataArray,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                         0,
                         sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

            /* Rebuild the entire bin using the updated encap flow information. */
            for (fmTreeIterInit(&itRuleBin, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRuleBin, &ruleNumber, &value)) == FM_OK ; )
            {
                err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                err = TunnelRuleToTeData(tunnelGrp,
                                         (fm_fm10000TunnelRule*) value,
                                         &teDataArray[teDataPos],
                                         FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                         &teDataOutSet);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                teDataPos += teDataOutSet;
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            err = fm10000GetTeDataBlockLength(teDataArray, teDataPos, &blockLength);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Find a free teData block */
            err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);

            /* No Free Data block big enough available, using swap */
            if (err == FM_ERR_TUNNEL_FLOW_FULL)
            {
                /* Using swap only works if the block can be moved back to the
                 * previous location */
                if (blockLength > lookupBin->teLookup.dataLength)
                {
                    /* If any of the blocks can't be updated, move back the
                     * state of all blocks that belong to that encap flow to
                     * the original state. */
                    err = fm10000UpdateTunnelEncapFlow(sw,
                                                       group,
                                                       encapFlow,
                                                       originalField,
                                                       &originalParam);
                    if (err != FM_OK)
                    {
                        err = FM_FAIL;
                    }
                    else
                    {
                        err = FM_ERR_TUNNEL_FLOW_FULL;
                    }
                    encapFlowEntry->field = originalField;
                    encapFlowEntry->param = originalParam;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }

                teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[group >> 3];

                upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

                /* Save the old index since the lookup bin will be updated after
                 * the move call. */
                baseIndex = lookupBin->teLookup.dataPtr;

                err = MoveTeDataBlock(sw,
                                      group >> 3,
                                      lookupBin->teLookup.dataPtr,
                                      lookupBin->teLookup.dataLength,
                                      upperBound);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* do we need to adjust the free ptr? */
                if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
                {
                    teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
                }
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Write the updated block to a new location if free block found.
             * Otherwise, the position used will be the same as the previous
             * block. In case no free block was found, the swap copy is the one
             * that currently handle that flow. */
            err = fm10000SetTeData(sw,
                                   group >> 3,
                                   baseIndex,
                                   teDataArray,
                                   teDataPos,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Now use this new block for all the rules defined */
            newTeLookup.dataPtr = baseIndex;
            newTeLookup.dataLength = blockLength;
            newTeLookup.last = TRUE;
            err = fm10000SetTeLookup(sw,
                                     group >> 3,
                                     tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                     &newTeLookup,
                                     FM10000_TE_LOOKUP_ALL,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Should never fail since this block was already reserved. This
             * could be either the old position or the swap copy. */
            err = FreeTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  &teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin->teLookup = newTeLookup;

            /* Update block control information */
            teDataBlkCtrl->index = baseIndex;
            teDataBlkCtrl->length = blockLength;

            /* Should never fail since this block was already found to be free */
            err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        err = FM_OK;

    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000UpdateTunnelEncapFlow */




/*****************************************************************************/
/** fm10000GetTunnelEncapFlow
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelEncapFlow(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int                   encapFlow,
                                    fm_tunnelEncapFlow *     field,
                                    fm_tunnelEncapFlowParam *param)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000EncapFlow *encapFlowEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d\n",
                 sw, group, encapFlow);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (field == NULL) || (param == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    encapFlowEntry = (fm_fm10000EncapFlow *) value;

    *field = encapFlowEntry->field;
    *param = encapFlowEntry->param;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelEncapFlow */




/*****************************************************************************/
/** fm10000GetTunnelEncapFlowFirst
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelEncapFlowFirst(fm_int                   sw,
                                         fm_int                   group,
                                         fm_int *                 firstEncapFlow,
                                         fm_tunnelEncapFlow *     field,
                                         fm_tunnelEncapFlowParam *param)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000EncapFlow *encapFlowEntry;
    fm_treeIterator it;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (firstEncapFlow == NULL) || (field == NULL) || (param == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    fmTreeIterInit(&it, &tunnelGrp->encapFlows);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    encapFlowEntry = (fm_fm10000EncapFlow *) nextValue;

    *firstEncapFlow = nextKey;
    *field = encapFlowEntry->field;
    *param = encapFlowEntry->param;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelEncapFlowFirst */




/*****************************************************************************/
/** fm10000GetTunnelEncapFlowNext
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelEncapFlowNext(fm_int                   sw,
                                        fm_int                   group,
                                        fm_int                   currentEncapFlow,
                                        fm_int *                 nextEncapFlow,
                                        fm_tunnelEncapFlow *     field,
                                        fm_tunnelEncapFlowParam *param)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000EncapFlow *encapFlowEntry;
    fm_treeIterator it;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, currentEncapFlow = %d\n",
                 sw, group, currentEncapFlow);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (nextEncapFlow == NULL) || (field == NULL) || (param == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    err = fmTreeIterInitFromSuccessor(&it, &tunnelGrp->encapFlows, currentEncapFlow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    encapFlowEntry = (fm_fm10000EncapFlow *) nextValue;

    *nextEncapFlow = nextKey;
    *field = encapFlowEntry->field;
    *param = encapFlowEntry->param;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelEncapFlowNext */




/*****************************************************************************/
/** fm10000AddTunnelRule
 * \ingroup intTunnel
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
fm_status fm10000AddTunnelRule(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   rule,
                               fm_tunnelCondition       cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction          action,
                               fm_tunnelActionParam *   actParam)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_uint16 hash;
    fm_uint16 hashMask;
    fm_uint16 hashBin;
    fm_fm10000TunnelRule *tunnelRule;
    fm_fm10000TeData teData[FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE];
    fm_int teDataOutSet;
    fm_int teDataPos;
    fm_fm10000TunnelLookupBin *lookupBin;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_int blockLength;
    fm_fm10000TeLookup newTeLookup;
    fm_int prevLookupIndex;
    fm_uint64 lookupIndex;
    fm_int i;
    fm_uint16 baseIndex;
    fm_fm10000EncapFlow *encapFlow;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d, cond = 0x%x, action = 0x%x\n",
                 sw, group, rule, cond, action);

    /* Validating input argument */
    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (actParam == NULL) ||
         (cond && (condParam == NULL)) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Is this rule already part of the group? */
    if (fmTreeFind(&tunnelGrp->rules, rule, &value) == FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    FM_MEMSET_S( teData,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                 0,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

    /* Hash Lookup type needs key block */
    if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
    {
        /* Hash and Search key must match */
        if ( (tunnelGrp->tunnelParam.hashKeyConfig & FM10000_TUNNEL_HASH_SEARCH_KEY) !=
             (cond & FM10000_TUNNEL_HASH_SEARCH_KEY) )
        {
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        tunnelRule = fmAlloc(sizeof(fm_fm10000TunnelRule));

        if (tunnelRule == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*tunnelRule);

        tunnelRule->condition = cond;
        tunnelRule->condParam = *condParam;
        tunnelRule->action = action;
        tunnelRule->actParam = *actParam;

        /* Find a free counter index if needed */
        if (action & FM_TUNNEL_COUNT)
        {
            err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                      1,
                                      FALSE,
                                      &i);
            if (i < 0)
            {
                fmFree(tunnelRule);
                err = FM_ERR_TUNNEL_COUNT_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            else if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            tunnelRule->counter = i;

            /* Clear the counter */
            err = fm10000SetTeFlowCnt(sw,
                                      group >> 3,
                                      tunnelRule->counter,
                                      0LL,
                                      0LL);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }

        hash = ComputeTunnelHash(cond, condParam);
        hashMask = tunnelGrp->teDGlort.lookupData.hashLookup.hashSize - 1;

        /* Hash bin does not include the base offset */
        hashBin = hash & hashMask;
        tunnelRule->lookupBin = hashBin;

        err = fmTreeFind(&tunnelGrp->lookupBins, hashBin, &value);
        /* Add this entry to the current block */
        if (err == FM_OK)
        {
            teDataPos = 0;
            lookupBin = (fm_fm10000TunnelLookupBin *) value;

            /* Insert this new rule in the lookup group to built it in the
             * right order. */
            err = fmTreeInsert(&lookupBin->rules, rule, NULL);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* The block must always be build the same way (with rule in
               ascending order) to be able to update it atomically. */
            for (fmTreeIterInit(&itRule, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRule, &ruleNumber, &value)) == FM_OK ; )
            {
                /* The inserted rule is still not part of the rule tree */
                if ((fm_int)ruleNumber == rule)
                {
                    value = tunnelRule;
                }
                else
                {
                    err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                    if (err != FM_OK)
                    {
                        fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                        fmFree(tunnelRule);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }
                }

                /* Append every teData structure of the block */
                err = TunnelRuleToTeData(tunnelGrp,
                                         (fm_fm10000TunnelRule*) value,
                                         &teData[teDataPos],
                                         FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                         &teDataOutSet);
                if (err != FM_OK)
                {
                    fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                    fmFree(tunnelRule);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                teDataPos += teDataOutSet;
            }
            if (err != FM_ERR_NO_MORE)
            {
                fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* This rule will gets reinserted in one of the next steps */
            err = fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }
        /* Create a new block for this hash bin */
        else if (err == FM_ERR_NOT_FOUND)
        {
            /* Convert the rule into teData structure */
            err = TunnelRuleToTeData(tunnelGrp,
                                     tunnelRule,
                                     teData,
                                     FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                                     &teDataOutSet);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* First teData block is defined by the inserted rule */
            teDataPos = teDataOutSet;

            lookupBin = fmAlloc(sizeof(fm_fm10000TunnelLookupBin));

            if (lookupBin == NULL)
            {
                fmFree(tunnelRule);
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            FM_CLEAR(*lookupBin);

            fmTreeInit(&lookupBin->rules);

            /* This hash bin is now part of the group */
            err = fmTreeInsert(&tunnelGrp->lookupBins, hashBin, lookupBin);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                fmFreeLookupBin(lookupBin);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }
        else
        {
            fmFree(tunnelRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Insert the rule into the group */
        err = fmTreeInsert(&tunnelGrp->rules, rule, tunnelRule);
        if (err != FM_OK)
        {
            fmFree(tunnelRule);
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* teData now contains the inserted rule + the one already part of
         * that hash bin (if some was defined). */
        err = fmTreeInsert(&lookupBin->rules, rule, NULL);
        if (err != FM_OK)
        {
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Get the needed length of the whole block */
        err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Find a block large enough */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Configure the block */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               teData,
                               teDataPos,
                               TRUE);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* If this rule have encap flow action, this rule must be added to the
         * proper encap flow rule tree. */
        if (action & FM_TUNNEL_ENCAP_FLOW)
        {
            err = fmTreeFind(&tunnelGrp->encapFlows,
                             actParam->encapFlow,
                             &value);
            if (err != FM_OK)
            {
                fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                if (fmTreeSize(&lookupBin->rules) == 0)
                {
                    fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
                }
                fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            encapFlow = (fm_fm10000EncapFlow *) value;

            err = fmTreeInsert(&encapFlow->rules, rule, NULL);
            if (err != FM_OK)
            {
                fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                if (fmTreeSize(&lookupBin->rules) == 0)
                {
                    fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
                }
                fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }

        /* Now use this new block for all the rules defined */
        newTeLookup.dataPtr = baseIndex;
        newTeLookup.dataLength = blockLength;
        newTeLookup.last = TRUE;
        err = fm10000SetTeLookup(sw,
                                 group >> 3,
                                 tunnelGrp->teDGlort.baseLookup + hashBin,
                                 &newTeLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            if (fmTreeSize(&lookupBin->rules) == 0)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, hashBin, fmFreeLookupBin);
            }
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Only free the block if this rule was part of an already defined bin */
        if (lookupBin->teLookup.dataPtr)
        {
            err = FreeTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  &teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        /* Create the control block */
        else
        {
            teDataBlkCtrl = fmAlloc(sizeof(fm_fm10000TunnelTeDataBlockCtrl));

            if (teDataBlkCtrl == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            FM_CLEAR(*teDataBlkCtrl);
            teDataBlkCtrl->tunnelGrp = group;
            teDataBlkCtrl->tunnelDataType = FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN;
            teDataBlkCtrl->tunnelEntry = tunnelRule->lookupBin;
        }

        lookupBin->teLookup = newTeLookup;

        /* Update block control information */
        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;

        /* Should never fail since this block was already found to be free */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Reserve counter index if properly configured */
        if (action & FM_TUNNEL_COUNT)
        {
            /* Should never fail since this bit was already reserved */
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   tunnelRule->counter,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

    }
    /* Direct lookup type */
    else
    {
        /* Direct lookup does not supports condition */
        if (cond)
        {
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Find empty lookup slot to insert this rule */
        err = fmTreeIterInitFromSuccessor(&itRule,
                                          &tunnelGrp->lookupBins,
                                          tunnelGrp->lookupBinFirstFreeEntry);
        if (err == FM_ERR_NOT_FOUND)
        {
            /* Found a Free slot */
            lookupIndex = tunnelGrp->lookupBinFirstFreeEntry;
        }
        else if (err == FM_OK)
        {
            /* This position was not free but it is a starting point to find
             * a real empty slot. */
            prevLookupIndex = tunnelGrp->lookupBinFirstFreeEntry;
            while ((err = fmTreeIterNext(&itRule, &lookupIndex, &value)) == FM_OK )
            {
                if ((fm_int)lookupIndex > (prevLookupIndex + 1))
                {
                    break;
                }
                else
                {
                    prevLookupIndex = lookupIndex;
                }
            }
            if (err == FM_OK)
            {
                /* found hole in the lookup table */
                lookupIndex = prevLookupIndex + 1;
            }
            else if (err == FM_ERR_NO_MORE)
            {
                /* can it be added at the end of the lookup table? */
                if (prevLookupIndex + 1 < tunnelGrp->tunnelParam.size)
                {
                    lookupIndex = prevLookupIndex + 1;
                }
                else
                {
                    err = FM_ERR_TUNNEL_GROUP_FULL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
            }
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Move the first free position to the freshly inserted one. This is
         * to speed up the next search. This position will most likely not be
         * free except if the insertion fail. */
        tunnelGrp->lookupBinFirstFreeEntry = lookupIndex;

        tunnelRule = fmAlloc(sizeof(fm_fm10000TunnelRule));

        if (tunnelRule == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*tunnelRule);

        tunnelRule->action = action;
        tunnelRule->actParam = *actParam;

        /* Find a free counter index if needed */
        if (action & FM_TUNNEL_COUNT)
        {
            err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                      1,
                                      FALSE,
                                      &i);
            if (i < 0)
            {
                fmFree(tunnelRule);
                err = FM_ERR_TUNNEL_COUNT_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            else if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            tunnelRule->counter = i;

            /* Clear the counter */
            err = fm10000SetTeFlowCnt(sw,
                                      group >> 3,
                                      tunnelRule->counter,
                                      0LL,
                                      0LL);
            if (err != FM_OK)
            {
                fmFree(tunnelRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }

        /* Must have at least 2 teData entries available for flow action +
         * tunnel */
        err = TunnelRuleToTeData(tunnelGrp,
                                 tunnelRule,
                                 teData,
                                 FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                                 &teDataOutSet);
        if (err != FM_OK)
        {
            fmFree(tunnelRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* First teData block is defined by the inserted rule */
        teDataPos = teDataOutSet;

        /* lookup bin does not include the base offset */
        tunnelRule->lookupBin = lookupIndex;

        /* Create a new block for this lookup bin */
        lookupBin = fmAlloc(sizeof(fm_fm10000TunnelLookupBin));

        if (lookupBin == NULL)
        {
            fmFree(tunnelRule);
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*lookupBin);

        fmTreeInit(&lookupBin->rules);
        /* insert the lookup bin into the group lookup tree */
        err = fmTreeInsert(&tunnelGrp->lookupBins, lookupIndex, lookupBin);
        if (err != FM_OK)
        {
            fmFree(tunnelRule);
            fmFreeLookupBin(lookupBin);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* insert the rule into the group rule tree */
        err = fmTreeInsert(&tunnelGrp->rules, rule, tunnelRule);
        if (err != FM_OK)
        {
            fmFree(tunnelRule);
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* insert the rule into the lookup bin tree */
        err = fmTreeInsert(&lookupBin->rules, rule, NULL);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Retrieve the block size needed for that rule */
        err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Find a free teData block large enough */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Configure the block */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               teData,
                               teDataPos,
                               TRUE);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Clear the Use bit at insertion */
        err = fm10000SetTeFlowUsed(sw, group >> 3, baseIndex, FALSE);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* This index will be used to retrieve the use bit */
        tunnelRule->dataPos = baseIndex;

        /* If this rule have encap flow action, this rule must be added to the
         * proper encap flow rule tree. */
        if (action & FM_TUNNEL_ENCAP_FLOW)
        {
            err = fmTreeFind(&tunnelGrp->encapFlows,
                             actParam->encapFlow,
                             &value);
            if (err != FM_OK)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
                fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            encapFlow = (fm_fm10000EncapFlow *) value;

            err = fmTreeInsert(&encapFlow->rules, rule, NULL);
            if (err != FM_OK)
            {
                fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
                fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }

        /* Now use this new block for the rule defined */
        lookupBin->teLookup.dataPtr = baseIndex;
        lookupBin->teLookup.dataLength = blockLength;
        lookupBin->teLookup.last = TRUE;
        err = fm10000SetTeLookup(sw,
                                 group >> 3,
                                 tunnelGrp->teDGlort.baseLookup + lookupIndex,
                                 &lookupBin->teLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&tunnelGrp->lookupBins, lookupIndex, fmFreeLookupBin);
            fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Reserve counter index if properly configured */
        if (action & FM_TUNNEL_COUNT)
        {
            /* Should never fail since this bit was already reserved */
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   tunnelRule->counter,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        teDataBlkCtrl = fmAlloc(sizeof(fm_fm10000TunnelTeDataBlockCtrl));

        if (teDataBlkCtrl == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        FM_CLEAR(*teDataBlkCtrl);
        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;
        teDataBlkCtrl->tunnelGrp = group;
        teDataBlkCtrl->tunnelDataType = FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN;
        teDataBlkCtrl->tunnelEntry = tunnelRule->lookupBin;

        /* Should never fail since this block was already found to be free */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000AddTunnelRule */




/*****************************************************************************/
/** fm10000DeleteTunnelRule
 * \ingroup intTunnel
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
fm_status fm10000DeleteTunnelRule(fm_int sw, fm_int group, fm_int rule)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;
    fm_fm10000TeData teData[FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE];
    fm_int teDataOutSet;
    fm_int teDataPos;
    fm_fm10000TunnelLookupBin *lookupBin;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_int blockLength;
    fm_fm10000TeLookup newTeLookup;
    fm_uint16 baseIndex;
    fm_fm10000EncapFlow *encapFlow;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;
    fm_uint16 upperBound;
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Rules not found */
    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule*) value;

    FM_MEMSET_S( teData,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                 0,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

    /* Hash Lookup type */
    if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
    {
        err = fmTreeFind(&tunnelGrp->lookupBins, tunnelRule->lookupBin, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin = (fm_fm10000TunnelLookupBin *) value;

        /* This rule should be part of that bin */
        err = fmTreeFind(&lookupBin->rules, rule, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Only one rule part of that bin, remove the entire bin */
        if (fmTreeSize(&lookupBin->rules) == 1)
        {
            /* Unactivated bin refer to the teData index 0 */
            newTeLookup.dataPtr = 0;
            newTeLookup.dataLength = 0;
            newTeLookup.last = TRUE;
            err = fm10000SetTeLookup(sw,
                                     group >> 3,
                                     tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                     &newTeLookup,
                                     FM10000_TE_LOOKUP_ALL,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Free teData resource */
            err = FreeTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  &teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            fmFree(teDataBlkCtrl);

            /* Free counter index if one reserved */
            if (tunnelRule->counter)
            {
                err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                       tunnelRule->counter,
                                       FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Remove the rule from proper encap flow rule tree if needed */
            if (tunnelRule->action & FM_TUNNEL_ENCAP_FLOW)
            {
                err = fmTreeFind(&tunnelGrp->encapFlows,
                                 tunnelRule->actParam.encapFlow,
                                 &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                encapFlow = (fm_fm10000EncapFlow *) value;

                err = fmTreeRemove(&encapFlow->rules, rule, NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Remove the rule from control structure */
            err = fmTreeRemoveCertain(&tunnelGrp->lookupBins,
                                      tunnelRule->lookupBin,
                                      fmFreeLookupBin);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            err = fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        /* Need to rebuild a new block that does not includes the removed rule */
        else
        {
            teDataPos = 0;

            /* Rebuilding the entire bin block in ascending rule order */
            for (fmTreeIterInit(&itRule, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRule, &ruleNumber, &value)) == FM_OK ; )
            {
                if ((fm_int)ruleNumber != rule)
                {
                    err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    err = TunnelRuleToTeData(tunnelGrp,
                                             (fm_fm10000TunnelRule*) value,
                                             &teData[teDataPos],
                                             FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                             &teDataOutSet);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    teDataPos += teDataOutSet;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* Find the new block size, this can't be bigger than the previous
             * one. */
            err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Find a free teData block */
            err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);

            /* No Free Data block big enough available, using swap */
            if (err == FM_ERR_TUNNEL_FLOW_FULL)
            {
                teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[group >> 3];

                upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

                /* Save the old index since the lookup bin will be updated after
                 * the move call. */
                baseIndex = lookupBin->teLookup.dataPtr;

                err = MoveTeDataBlock(sw,
                                      group >> 3,
                                      lookupBin->teLookup.dataPtr,
                                      lookupBin->teLookup.dataLength,
                                      upperBound);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* do we need to adjust the free ptr? */
                if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
                {
                    teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
                }
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Write the updated block to a new location if free block found.
             * Otherwise, the position used will be the same as the previous
             * block. In case no free block was found, the swap copy is the one
             * that currently handle that flow. */
            err = fm10000SetTeData(sw,
                                   group >> 3,
                                   baseIndex,
                                   teData,
                                   teDataPos,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Now use this new block for all the rules defined */
            newTeLookup.dataPtr = baseIndex;
            newTeLookup.dataLength = blockLength;
            newTeLookup.last = TRUE;
            err = fm10000SetTeLookup(sw,
                                     group >> 3,
                                     tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                     &newTeLookup,
                                     FM10000_TE_LOOKUP_ALL,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Should never fail since this block was already reserved. This
             * could be either the old position or the swap copy. */
            err = FreeTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  &teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin->teLookup = newTeLookup;

            /* Free counter index if one reserved */
            if (tunnelRule->counter)
            {
                err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                       tunnelRule->counter,
                                       FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Update block control information */
            teDataBlkCtrl->index = baseIndex;
            teDataBlkCtrl->length = blockLength;

            /* Should never fail since this block was already found to be free */
            err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            if (tunnelRule->action & FM_TUNNEL_ENCAP_FLOW)
            {
                err = fmTreeFind(&tunnelGrp->encapFlows,
                                 tunnelRule->actParam.encapFlow,
                                 &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                encapFlow = (fm_fm10000EncapFlow *) value;

                err = fmTreeRemove(&encapFlow->rules, rule, NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Remove the rule from control structure */
            err = fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            err = fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }
    /* Direct lookup type */
    else
    {
        err = fmTreeFind(&tunnelGrp->lookupBins, tunnelRule->lookupBin, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin = (fm_fm10000TunnelLookupBin *) value;

        /* This rule should be part of that bin */
        err = fmTreeFind(&lookupBin->rules, rule, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Only one rule part of that bin, remove the entire bin */
        newTeLookup.dataPtr = 0;
        newTeLookup.dataLength = 0;
        newTeLookup.last = TRUE;
        err = fm10000SetTeLookup(sw,
                                 group >> 3,
                                 tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                 &newTeLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Free counter index if one reserved */
        if (tunnelRule->counter)
        {
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   tunnelRule->counter,
                                   FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Free teData resource */
        err = FreeTeDataBlock(sw,
                              group >> 3,
                              lookupBin->teLookup.dataPtr,
                              lookupBin->teLookup.dataLength,
                              &teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        fmFree(teDataBlkCtrl);

        if (tunnelRule->action & FM_TUNNEL_ENCAP_FLOW)
        {
            err = fmTreeFind(&tunnelGrp->encapFlows,
                             tunnelRule->actParam.encapFlow,
                             &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            encapFlow = (fm_fm10000EncapFlow *) value;

            err = fmTreeRemove(&encapFlow->rules, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Remove the rule from control structure */
        err = fmTreeRemoveCertain(&tunnelGrp->lookupBins,
                                  tunnelRule->lookupBin,
                                  fmFreeLookupBin);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Update the first free index if the removed entry was located prior
         * to the actual free position. */
        if (tunnelRule->lookupBin < tunnelGrp->lookupBinFirstFreeEntry)
        {
            tunnelGrp->lookupBinFirstFreeEntry = tunnelRule->lookupBin;
        }

        err = fmTreeRemoveCertain(&tunnelGrp->rules, rule, fmFree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000DeleteTunnelRule */




/*****************************************************************************/
/** fm10000UpdateTunnelRule
 * \ingroup intTunnel
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
fm_status fm10000UpdateTunnelRule(fm_int                   sw,
                                  fm_int                   group,
                                  fm_int                   rule,
                                  fm_tunnelCondition       cond,
                                  fm_tunnelConditionParam *condParam,
                                  fm_tunnelAction          action,
                                  fm_tunnelActionParam *   actParam)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_uint16 hash;
    fm_uint16 hashMask;
    fm_uint16 hashBin;
    fm_fm10000TunnelRule *tunnelRule;
    fm_fm10000TeData teData[FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE];
    fm_int teDataOutSet;
    fm_int teDataPos;
    fm_fm10000TunnelLookupBin *lookupBin;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_int blockLength;
    fm_fm10000TeLookup newTeLookup;
    fm_int i;
    fm_uint16 baseIndex;
    fm_fm10000EncapFlow *encapFlow;
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl;
    fm_fm10000TunnelRule updatedTunnelRule;
    fm_uint16 upperBound;
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_bool usedEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d, cond = 0x%x, action = 0x%x\n",
                 sw, group, rule, cond, action);

    /* Validating input argument */
    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (actParam == NULL) ||
         (cond && (condParam == NULL)) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];
    teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[group >> 3];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Is this rule already part of the group? */
    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    FM_MEMSET_S( teData,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                 0,
                 sizeof(fm_fm10000TeData) * FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE );

    /* Hash Lookup type needs key block */
    if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_HASH)
    {
        /* Hash and Search key must match */
        if ( (tunnelGrp->tunnelParam.hashKeyConfig & FM10000_TUNNEL_HASH_SEARCH_KEY) !=
             (cond & FM10000_TUNNEL_HASH_SEARCH_KEY) )
        {
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        updatedTunnelRule.condition = cond;
        updatedTunnelRule.condParam = *condParam;
        updatedTunnelRule.action = action;
        updatedTunnelRule.actParam = *actParam;
        updatedTunnelRule.dataPos = tunnelRule->dataPos;
        updatedTunnelRule.counter = 0;

        /* Find a free counter index if needed */
        if (action & FM_TUNNEL_COUNT)
        {
            if (tunnelRule->counter == 0)
            {
                err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                          1,
                                          FALSE,
                                          &i);
                if (i < 0)
                {
                    err = FM_ERR_TUNNEL_COUNT_FULL;
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                updatedTunnelRule.counter = i;

                /* Clear the counter */
                err = fm10000SetTeFlowCnt(sw,
                                          group >> 3,
                                          updatedTunnelRule.counter,
                                          0LL,
                                          0LL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            else
            {
                updatedTunnelRule.counter = tunnelRule->counter;
            }
        }

        hash = ComputeTunnelHash(cond, condParam);
        hashMask = tunnelGrp->teDGlort.lookupData.hashLookup.hashSize - 1;

        /* Hash bin does not include the base offset */
        hashBin = hash & hashMask;
        updatedTunnelRule.lookupBin = hashBin;

        err = fmTreeFind(&tunnelGrp->lookupBins, hashBin, &value);
        /* Add this entry to the current block */
        if (err == FM_OK)
        {
            teDataPos = 0;
            lookupBin = (fm_fm10000TunnelLookupBin *) value;

            /* Insert this new rule in the lookup group to built it in the
             * right order if the new hash bin is not the same as the previous
             * one. */
            if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
            {
                err = fmTreeInsert(&lookupBin->rules, rule, NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* The block must always be build the same way (with rule in
               ascending order) to be able to update it atomically. */
            for (fmTreeIterInit(&itRule, &lookupBin->rules) ;
                  (err = fmTreeIterNext(&itRule, &ruleNumber, &value)) == FM_OK ; )
            {
                /* The updated rule is still not part of the rule tree */
                if ((fm_int)ruleNumber == rule)
                {
                    value = &updatedTunnelRule;
                }
                else
                {
                    err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                    if (err != FM_OK)
                    {
                        if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
                        {
                            fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                        }
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }
                }

                /* Append every teData structure of the block */
                err = TunnelRuleToTeData(tunnelGrp,
                                         (fm_fm10000TunnelRule*) value,
                                         &teData[teDataPos],
                                         FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                         &teDataOutSet);
                if (err != FM_OK)
                {
                    if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
                    {
                        fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                    }
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                teDataPos += teDataOutSet;
            }
            if (err != FM_ERR_NO_MORE)
            {
                if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
                {
                    fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* This rule will gets reinserted in one of the next steps */
            if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
            {
                err = fmTreeRemoveCertain(&lookupBin->rules, rule, NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
        }
        /* Create a new block for this hash bin */
        else if (err == FM_ERR_NOT_FOUND)
        {
            /* Convert the rule into teData structure */
            err = TunnelRuleToTeData(tunnelGrp,
                                     &updatedTunnelRule,
                                     teData,
                                     FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                                     &teDataOutSet);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* First teData block is defined by the inserted rule */
            teDataPos = teDataOutSet;

            /* Do not update an already active bin */
            lookupBin = NULL;
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Get the needed length of the whole block */
        err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Find a block large enough */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);

        /* No Free Data block big enough available, using swap */
        if (err == FM_ERR_TUNNEL_FLOW_FULL)
        {
            /* Using swap only works if the block can be moved back to the
             * previous location */
            if ( (tunnelRule->lookupBin != updatedTunnelRule.lookupBin) ||
                 (lookupBin == NULL) ||
                 (blockLength > lookupBin->teLookup.dataLength) )
            {
                err = FM_ERR_TUNNEL_FLOW_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

            /* Save the old index since the lookup bin will be updated after
             * the move call. */
            baseIndex = lookupBin->teLookup.dataPtr;

            err = MoveTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  upperBound);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* do we need to adjust the free ptr? */
            if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
            {
                teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
            }
        }
        else if (err != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Write the updated block to a new location if free block found.
         * Otherwise, the position used will be the same as the previous
         * block. In case no free block was found, the swap copy is the one
         * that currently handle that flow. */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               teData,
                               teDataPos,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Now use this new block for all the rules defined */
        newTeLookup.dataPtr = baseIndex;
        newTeLookup.dataLength = blockLength;
        newTeLookup.last = TRUE;
        err = fm10000SetTeLookup(sw,
                                 group >> 3,
                                 tunnelGrp->teDGlort.baseLookup + hashBin,
                                 &newTeLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        err = fmTreeFind(&tunnelGrp->lookupBins, hashBin, &value);
        /* Added/Updated rule in a previously allocated hash bin */
        if (err == FM_OK)
        {
            lookupBin = (fm_fm10000TunnelLookupBin *) value;

            /* Should never fail since this block was already reserved. This
             * could be either the old position or the swap copy. */
            err = FreeTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  &teDataBlkCtrl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        /* This Updated rule now refer to a new bin */
        else
        {
            lookupBin = fmAlloc(sizeof(fm_fm10000TunnelLookupBin));

            if (lookupBin == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            FM_CLEAR(*lookupBin);

            fmTreeInit(&lookupBin->rules);

            /* This hash bin is now part of the group */
            err = fmTreeInsert(&tunnelGrp->lookupBins, hashBin, lookupBin);
            if (err != FM_OK)
            {
                fmFreeLookupBin(lookupBin);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            teDataBlkCtrl = fmAlloc(sizeof(fm_fm10000TunnelTeDataBlockCtrl));

            if (teDataBlkCtrl == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            FM_CLEAR(*teDataBlkCtrl);
            teDataBlkCtrl->tunnelGrp = group;
            teDataBlkCtrl->tunnelDataType = FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN;
            teDataBlkCtrl->tunnelEntry = hashBin;
        }

        lookupBin->teLookup = newTeLookup;

        /* Update block control information */
        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;

        /* Should never fail since this block was already found to be free */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Now remove the rule from the old block */
        if (tunnelRule->lookupBin != updatedTunnelRule.lookupBin)
        {
            err = fmTreeFind(&tunnelGrp->lookupBins, tunnelRule->lookupBin, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin = (fm_fm10000TunnelLookupBin *) value;

            /* This rule should be part of that bin */
            err = fmTreeFind(&lookupBin->rules, rule, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Only one rule part of that bin, remove the entire bin */
            if (fmTreeSize(&lookupBin->rules) == 1)
            {
                /* Free teData resource */
                err = FreeTeDataBlock(sw,
                                      group >> 3,
                                      lookupBin->teLookup.dataPtr,
                                      lookupBin->teLookup.dataLength,
                                      &teDataBlkCtrl);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                fmFree(teDataBlkCtrl);

                /* Remove the bin from control structure */
                err = fmTreeRemoveCertain(&tunnelGrp->lookupBins,
                                          tunnelRule->lookupBin,
                                          fmFreeLookupBin);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            /* Need to rebuild a new block that does not includes the removed rule */
            else
            {
                teDataPos = 0;

                /* Rebuilding the entire bin block in ascending rule order */
                for (fmTreeIterInit(&itRule, &lookupBin->rules) ;
                      (err = fmTreeIterNext(&itRule, &ruleNumber, &value)) == FM_OK ; )
                {
                    if ((fm_int)ruleNumber != rule)
                    {
                        err = fmTreeFind(&tunnelGrp->rules, ruleNumber, &value);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                        err = TunnelRuleToTeData(tunnelGrp,
                                                 (fm_fm10000TunnelRule*) value,
                                                 &teData[teDataPos],
                                                 FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE - teDataPos,
                                                 &teDataOutSet);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                        teDataPos += teDataOutSet;
                    }
                }
                if (err != FM_ERR_NO_MORE)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                err = FM_OK;

                /* Find the new block size, this can't be bigger than the previous
                 * one. */
                err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* Sanity check */
                if ( (blockLength > lookupBin->teLookup.dataLength) ||
                     (lookupBin->teLookup.dataLength > teDataCtrl->teDataSwapSize) )
                {
                    err = FM_FAIL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }

                /* Always uses the swap area to update the old block */
                upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

                /* Save the old index since the lookup bin will be updated after
                 * the move call. */
                baseIndex = lookupBin->teLookup.dataPtr;

                err = MoveTeDataBlock(sw,
                                      group >> 3,
                                      lookupBin->teLookup.dataPtr,
                                      lookupBin->teLookup.dataLength,
                                      upperBound);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* do we need to adjust the free ptr? */
                if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
                {
                    teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
                }

                /* Write the updated block to the previous location since the
                 * size must be lower or equal than the created hole. */
                err = fm10000SetTeData(sw,
                                       group >> 3,
                                       baseIndex,
                                       teData,
                                       teDataPos,
                                       TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* Now use this new block for all the rules defined */
                newTeLookup.dataPtr = baseIndex;
                newTeLookup.dataLength = blockLength;
                newTeLookup.last = TRUE;
                err = fm10000SetTeLookup(sw,
                                         group >> 3,
                                         tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                         &newTeLookup,
                                         FM10000_TE_LOOKUP_ALL,
                                         TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* Freeing the swap space block. */
                err = FreeTeDataBlock(sw,
                                      group >> 3,
                                      lookupBin->teLookup.dataPtr,
                                      lookupBin->teLookup.dataLength,
                                      &teDataBlkCtrl);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                lookupBin->teLookup = newTeLookup;

                /* Update block control information */
                teDataBlkCtrl->index = baseIndex;
                teDataBlkCtrl->length = blockLength;

                /* Should never fail since this block was already found to be free */
                err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                /* The old block is rebuilt without the updated rule. */
                err = fmTreeRemove(&lookupBin->rules, rule, NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Now insert the updated rule into the new bin that carry it */
            err = fmTreeFind(&tunnelGrp->lookupBins, hashBin, &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            lookupBin = (fm_fm10000TunnelLookupBin *) value;

            err = fmTreeInsert(&lookupBin->rules, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

    }
    /* Direct lookup type */
    else
    {
        /* Direct lookup does not supports condition */
        if (cond)
        {
            err = FM_ERR_TUNNEL_CONFLICT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        err = fmTreeFind(&tunnelGrp->lookupBins, tunnelRule->lookupBin, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin = (fm_fm10000TunnelLookupBin *) value;

        /* This rule should be part of that bin */
        err = fmTreeFind(&lookupBin->rules, rule, &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        updatedTunnelRule.condition = cond;
        updatedTunnelRule.condParam = *condParam;
        updatedTunnelRule.action = action;
        updatedTunnelRule.actParam = *actParam;
        updatedTunnelRule.dataPos = tunnelRule->dataPos;
        updatedTunnelRule.counter = 0;
        updatedTunnelRule.lookupBin = tunnelRule->lookupBin;

        /* Find a free counter index if needed */
        if (action & FM_TUNNEL_COUNT)
        {
            if (tunnelRule->counter == 0)
            {
                err = fmFindBitInBitArray(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                          1,
                                          FALSE,
                                          &i);
                if (i < 0)
                {
                    err = FM_ERR_TUNNEL_COUNT_FULL;
                }
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                updatedTunnelRule.counter = i;

                /* Clear the counter */
                err = fm10000SetTeFlowCnt(sw,
                                          group >> 3,
                                          updatedTunnelRule.counter,
                                          0LL,
                                          0LL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            else
            {
                updatedTunnelRule.counter = tunnelRule->counter;
            }
        }

        /* Translate the updated rule to teData */
        err = TunnelRuleToTeData(tunnelGrp,
                                 &updatedTunnelRule,
                                 teData,
                                 FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE,
                                 &teDataOutSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* First teData block is defined by the updated rule */
        teDataPos = teDataOutSet;

        /* Retrieve the block size needed for that updated rule */
        err = fm10000GetTeDataBlockLength(teData, teDataPos, &blockLength);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Find a free teData block large enough */
        err = FindTeDataBlock(sw, group >> 3, blockLength, &baseIndex);

        /* No Free Data block big enough available, using swap */
        if (err == FM_ERR_TUNNEL_FLOW_FULL)
        {
            /* Using swap only works if the block can be moved back to the
             * previous location */
            if (blockLength > lookupBin->teLookup.dataLength)
            {
                err = FM_ERR_TUNNEL_FLOW_FULL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            upperBound = FM10000_TE_DATA_ENTRIES_0 - teDataCtrl->teDataSwapSize;

            /* Save the old index since the lookup bin will be updated after
             * the move call. */
            baseIndex = lookupBin->teLookup.dataPtr;

            err = MoveTeDataBlock(sw,
                                  group >> 3,
                                  lookupBin->teLookup.dataPtr,
                                  lookupBin->teLookup.dataLength,
                                  upperBound);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* do we need to adjust the free ptr? */
            if (baseIndex < teDataCtrl->teDataHandlerFirstFreeEntry)
            {
                teDataCtrl->teDataHandlerFirstFreeEntry = baseIndex;
            }
        }
        else if (err != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Clear the Use bit at insertion */
        err = fm10000SetTeFlowUsed(sw, group >> 3, baseIndex, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Write the updated block to a new location if free block found.
         * Otherwise, the position used will be the same as the previous
         * block. In case no free block was found, the swap copy is the one
         * that currently handle that flow. */
        err = fm10000SetTeData(sw,
                               group >> 3,
                               baseIndex,
                               teData,
                               teDataPos,
                               TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Now use this new block for the updated rule */
        newTeLookup.dataPtr = baseIndex;
        newTeLookup.dataLength = blockLength;
        newTeLookup.last = TRUE;
        err = fm10000SetTeLookup(sw,
                                 group >> 3,
                                 tunnelGrp->teDGlort.baseLookup + tunnelRule->lookupBin,
                                 &newTeLookup,
                                 FM10000_TE_LOOKUP_ALL,
                                 TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Update the Used flag to the new position */
        err = fm10000GetTeFlowUsed(sw, group >> 3, tunnelRule->dataPos, &usedEntry);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        if (usedEntry)
        {
            err = fm10000SetTeFlowUsed(sw, group >> 3, baseIndex, usedEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        /* Should never fail since this block was already reserved. This
         * could be either the old position or the swap copy. */
        err = FreeTeDataBlock(sw,
                              group >> 3,
                              lookupBin->teLookup.dataPtr,
                              lookupBin->teLookup.dataLength,
                              &teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        lookupBin->teLookup = newTeLookup;

        /* Update block control information */
        teDataBlkCtrl->index = baseIndex;
        teDataBlkCtrl->length = blockLength;

        /* Should never fail since this block was already found to be free */
        err = ReserveTeDataBlock(sw, group >> 3, teDataBlkCtrl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        updatedTunnelRule.dataPos = baseIndex;

    }

    /* Update the encap flow configuration by removing old encap flow
     * reference and adding the new one. */
    if (tunnelRule->action & FM_TUNNEL_ENCAP_FLOW)
    {
        err = fmTreeFind(&tunnelGrp->encapFlows,
                         tunnelRule->actParam.encapFlow,
                         &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        encapFlow = (fm_fm10000EncapFlow *) value;

        err = fmTreeRemove(&encapFlow->rules, rule, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* The encap flow was already validated to be part of the group when
     * building the updated teData. */
    if (action & FM_TUNNEL_ENCAP_FLOW)
    {
        err = fmTreeFind(&tunnelGrp->encapFlows,
                         actParam->encapFlow,
                         &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        encapFlow = (fm_fm10000EncapFlow *) value;

        err = fmTreeInsert(&encapFlow->rules, rule, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Reserve counter index if properly configured */
    if (action & FM_TUNNEL_COUNT)
    {
        if (tunnelRule->counter == 0)
        {
            /* Should never fail since this bit was already reserved */
            err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                   updatedTunnelRule.counter,
                                   TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
    }
    /* Free the counter index */
    else if (tunnelRule->counter)
    {
        err = fmSetBitArrayBit(&switchExt->tunnelCfg->cntInUse[group >> 3],
                               tunnelRule->counter,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    *tunnelRule = updatedTunnelRule;


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000UpdateTunnelRule */




/*****************************************************************************/
/** fm10000GetTunnelRule
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelRule(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   rule,
                               fm_tunnelCondition *     cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction *        action,
                               fm_tunnelActionParam *   actParam)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (cond == NULL) || (condParam == NULL) ||
         (action == NULL) || (actParam == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    *cond = tunnelRule->condition;
    *condParam = tunnelRule->condParam;
    *action = tunnelRule->action;
    *actParam = tunnelRule->actParam;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelRule */




/*****************************************************************************/
/** fm10000GetTunnelRuleFirst
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelRuleFirst(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int *                 firstRule,
                                    fm_tunnelCondition *     cond,
                                    fm_tunnelConditionParam *condParam,
                                    fm_tunnelAction *        action,
                                    fm_tunnelActionParam *   actParam)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000TunnelRule *tunnelRule;
    fm_treeIterator it;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d, group = %d\n", sw, group);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (firstRule == NULL) || (cond == NULL) || (condParam == NULL) ||
         (action == NULL) || (actParam == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    fmTreeIterInit(&it, &tunnelGrp->rules);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    tunnelRule = (fm_fm10000TunnelRule *) nextValue;

    *firstRule = nextKey;
    *cond = tunnelRule->condition;
    *condParam = tunnelRule->condParam;
    *action = tunnelRule->action;
    *actParam = tunnelRule->actParam;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelRuleFirst */




/*****************************************************************************/
/** fm10000GetTunnelRuleNext
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelRuleNext(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   currentRule,
                                   fm_int *                 nextRule,
                                   fm_tunnelCondition *     cond,
                                   fm_tunnelConditionParam *condParam,
                                   fm_tunnelAction *        action,
                                   fm_tunnelActionParam *   actParam)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_uint64 nextKey;
    void *nextValue;
    fm_fm10000TunnelRule *tunnelRule;
    fm_treeIterator it;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, currentRule = %d\n",
                 sw, group, currentRule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if ( (nextRule == NULL) || (cond == NULL) || (condParam == NULL) ||
         (action == NULL) || (actParam == NULL) )
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    err = fmTreeIterInitFromSuccessor(&it, &tunnelGrp->rules, currentRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    tunnelRule = (fm_fm10000TunnelRule *) nextValue;

    *nextRule = nextKey;
    *cond = tunnelRule->condition;
    *condParam = tunnelRule->condParam;
    *action = tunnelRule->action;
    *actParam = tunnelRule->actParam;

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelRuleNext */




/*****************************************************************************/
/** fm10000GetTunnelRuleCount
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelRuleCount(fm_int             sw,
                                    fm_int             group,
                                    fm_int             rule,
                                    fm_tunnelCounters *counters)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (counters == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    if (tunnelRule->counter)
    {
        err = fm10000GetTeFlowCnt(sw,
                                  group >> 3,
                                  tunnelRule->counter,
                                  &counters->cntPkts,
                                  &counters->cntOctets);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* No Count Action defined */
    else
    {
        err = FM_ERR_TUNNEL_NO_COUNT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelRuleCount */




/*****************************************************************************/
/** fm10000GetTunnelEncapFlowCount
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelEncapFlowCount(fm_int             sw,
                                         fm_int             group,
                                         fm_int             encapFlow,
                                         fm_tunnelCounters *counters)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000EncapFlow *encapFlowEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d\n",
                 sw, group, encapFlow);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (counters == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    encapFlowEntry = (fm_fm10000EncapFlow *) value;

    if (encapFlowEntry->counter)
    {
        err = fm10000GetTeFlowCnt(sw,
                                  group >> 3,
                                  encapFlowEntry->counter,
                                  &counters->cntPkts,
                                  &counters->cntOctets);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* No Count Action defined */
    else
    {
        err = FM_ERR_TUNNEL_NO_COUNT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelEncapFlowCount */




/*****************************************************************************/
/** fm10000GetTunnelRuleUsed
 * \ingroup intTunnel
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
fm_status fm10000GetTunnelRuleUsed(fm_int  sw,
                                   fm_int  group,
                                   fm_int  rule,
                                   fm_bool *used)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (used == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* The used bit is only supported on direct lookup group. */
    if (tunnelGrp->teDGlort.lookupType != FM_FM10000_TE_LOOKUP_DIRECT)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    err = fm10000GetTeFlowUsed(sw,
                               group >> 3,
                               tunnelRule->dataPos,
                               used);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelRuleUsed */




/*****************************************************************************/
/** fm10000ResetTunnelRuleCount
 * \ingroup intTunnel
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
fm_status fm10000ResetTunnelRuleCount(fm_int sw, fm_int group, fm_int rule)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    if (tunnelRule->counter)
    {
        err = fm10000SetTeFlowCnt(sw,
                                  group >> 3,
                                  tunnelRule->counter,
                                  0LL,
                                  0LL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* No Count Action defined */
    else
    {
        err = FM_ERR_TUNNEL_NO_COUNT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000ResetTunnelRuleCount */




/*****************************************************************************/
/** fm10000ResetTunnelEncapFlowCount
 * \ingroup intTunnel
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
fm_status fm10000ResetTunnelEncapFlowCount(fm_int sw,
                                           fm_int group,
                                           fm_int encapFlow)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000EncapFlow *encapFlowEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, encapFlow = %d\n",
                 sw, group, encapFlow);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->encapFlows, encapFlow, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    encapFlowEntry = (fm_fm10000EncapFlow *) value;

    if (encapFlowEntry->counter)
    {
        err = fm10000SetTeFlowCnt(sw,
                                  group >> 3,
                                  encapFlowEntry->counter,
                                  0LL,
                                  0LL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* No Count Action defined */
    else
    {
        err = FM_ERR_TUNNEL_NO_COUNT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000ResetTunnelEncapFlowCount */




/*****************************************************************************/
/** fm10000ResetTunnelRuleUsed
 * \ingroup intTunnel
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
fm_status fm10000ResetTunnelRuleUsed(fm_int sw, fm_int group, fm_int rule)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    void *value;
    fm_fm10000TunnelRule *tunnelRule;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d\n",
                 sw, group, rule);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* The used bit is only supported on direct lookup group. */
    if (tunnelGrp->teDGlort.lookupType != FM_FM10000_TE_LOOKUP_DIRECT)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fmTreeFind(&tunnelGrp->rules, rule, &value) != FM_OK)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    tunnelRule = (fm_fm10000TunnelRule *) value;

    err = fm10000SetTeFlowUsed(sw,
                               group >> 3,
                               tunnelRule->dataPos,
                               FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000ResetTunnelRuleUsed */




/*****************************************************************************/
/** fm10000SetTunnelAttribute
 * \ingroup intTunnel
 *
 * \desc            Set a Tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 *
 * \param[in]       rule is the id on which to operate.
 *
 * \param[in]       attr is the Tunnel attribute (see ''Tunnel Attributes'')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetTunnelAttribute(fm_int sw,
                                    fm_int group,
                                    fm_int rule,
                                    fm_int attr,
                                    void * value)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d, attr = %d\n",
                 sw, group, rule, attr);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (value == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switch (attr)
    {
        case FM_TUNNEL_SET_DEFAULT_SGLORT:
            tunnelGrp->teDGlort.setSGlort = *( (fm_bool *) value );
            err = fm10000SetTeDGlort(sw,
                                     group >> 3,
                                     group & 0x7,
                                     &tunnelGrp->teDGlort,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;

        case FM_TUNNEL_SET_DEFAULT_DGLORT:
            tunnelGrp->teDGlort.setDGlort = *( (fm_bool *) value );
            err = fm10000SetTeDGlort(sw,
                                     group >> 3,
                                     group & 0x7,
                                     &tunnelGrp->teDGlort,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;
    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTunnelAttribute */




/*****************************************************************************/
/** fm10000GetTunnelAttribute
 * \ingroup intTunnel
 *
 * \desc            Get a Tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group is the group handler.
 *
 * \param[in]       rule is the id on which to operate.
 *
 * \param[in]       attr is the Tunnel attribute (see ''Tunnel Attributes'')
 *                  to get.
 *
 * \param[out]      value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_TUNNEL_INVALID_ENTRY if group or rule is invalid.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetTunnelAttribute(fm_int sw,
                                    fm_int group,
                                    fm_int rule,
                                    fm_int attr,
                                    void * value)
{
    fm_fm10000TunnelGrp *tunnelGrp;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status err = FM_OK;
    fm_bool tunnelLockTaken = FALSE;
    fm_tunnelUsage *tunnelUsage;
    fm_int usedLookupEntries;
    fm_int i;
    fm_fm10000TunnelGrp *tunnelGrpTmp;
    fm_int teDataUsed;
    fm_fm10000TunnelLookupBin *lookupBin;
    void *nextValue;
    fm_uint64 index;
    fm_treeIterator itTeData;
    fm_fm10000EncapFlow *encapFlow;
    fm_int cntUsed;
    fm_treeIterator itRule;
    fm_fm10000TunnelRule *tunnelRule;
    fm_tunnelGlortUser *glortUser;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, group = %d, rule = %d, attr = %d\n",
                 sw, group, rule, attr);

    if ( (group >= FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                   FM10000_TE_DGLORT_MAP_ENTRIES_1) ||
         (group < 0) )
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (value == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    tunnelGrp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][group & 0x7];

    if (tunnelGrp->active == FALSE)
    {
        err = FM_ERR_TUNNEL_INVALID_ENTRY;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switch (attr)
    {
        case FM_TUNNEL_SET_DEFAULT_SGLORT:
            *( (fm_bool *) value ) = tunnelGrp->teDGlort.setSGlort;
            break;

        case FM_TUNNEL_SET_DEFAULT_DGLORT:
            *( (fm_bool *) value ) = tunnelGrp->teDGlort.setDGlort;
            break;

        case FM_TUNNEL_USAGE:
            tunnelUsage = (fm_tunnelUsage*) value;

            /* Reserved lookup entry */
            tunnelUsage->lookupReserved = tunnelGrp->tunnelParam.size;

            /* Used lookup entry */
            tunnelUsage->lookupUsed = fmTreeSize(&tunnelGrp->lookupBins);

            usedLookupEntries = 0;
            for (i = 0 ; i < FM10000_TE_DGLORT_MAP_ENTRIES_0; i++)
            {
                tunnelGrpTmp = &switchExt->tunnelCfg->tunnelGrp[group >> 3][i];
                if (tunnelGrpTmp->active)
                {
                    usedLookupEntries += tunnelGrpTmp->tunnelParam.size;
                }
            }

            /* Remaining lookup entry */
            tunnelUsage->lookupAvailable = FM10000_TE_LOOKUP_ENTRIES_0 - usedLookupEntries;

            /* Total lookup entry */
            tunnelUsage->lookupTotal = FM10000_TE_LOOKUP_ENTRIES_0;

            teDataUsed = 0;

            /* teData usage includes rule and shared encap flow */
            for (fmTreeIterInit(&itTeData, &tunnelGrp->lookupBins) ;
                 (err = fmTreeIterNext(&itTeData, &index, &nextValue)) == FM_OK ; )
            {
                lookupBin = (fm_fm10000TunnelLookupBin *)nextValue;
                teDataUsed += lookupBin->teLookup.dataLength;
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            for (fmTreeIterInit(&itTeData, &tunnelGrp->encapFlows) ;
                 (err = fmTreeIterNext(&itTeData, &index, &nextValue)) == FM_OK ; )
            {
                encapFlow = (fm_fm10000EncapFlow *)nextValue;
                if (encapFlow->param.shared)
                {
                    teDataUsed += encapFlow->teLookup.dataLength;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* Used flow entry */
            tunnelUsage->flowUsed = teDataUsed;

            /* Available flow entry. This value got adjusted to always keep a
             * 1% buffer for performance enhancement. */
            tunnelUsage->flowAvailable = switchExt->tunnelCfg->teDataCtrl[group >>3].teDataFreeEntryCount -
                 FM10000_TUNNEL_TE_DATA_MIN_FREE_SIZE;

            /* It is possible to cross the virtual limit under some situation */
            if (tunnelUsage->flowAvailable < 0)
            {
                tunnelUsage->flowAvailable = 0;
            }

            /* Total flow entry. Swap is deducted from the real total */
            tunnelUsage->flowTotal = FM10000_TE_DATA_ENTRIES_0 -
                 (switchExt->tunnelCfg->teDataCtrl[group >>3].teDataSwapSize + 1);

            cntUsed = 0;

            /* Counter resource are used by rule and shared encap flow. */
            for (fmTreeIterInit(&itRule, &tunnelGrp->rules) ;
                 (err = fmTreeIterNext(&itRule, &index, &nextValue)) == FM_OK ; )
            {
                tunnelRule = (fm_fm10000TunnelRule *)nextValue;
                if (tunnelRule->counter)
                {
                    cntUsed++;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            for (fmTreeIterInit(&itRule, &tunnelGrp->encapFlows) ;
                 (err = fmTreeIterNext(&itRule, &index, &nextValue)) == FM_OK ; )
            {
                encapFlow = (fm_fm10000EncapFlow *)nextValue;
                if (encapFlow->counter)
                {
                    cntUsed++;
                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }
            err = FM_OK;

            /* Used counter resource */
            tunnelUsage->countUsed = cntUsed;

            err = fmGetBitArrayNonZeroBitCount(&switchExt->tunnelCfg->cntInUse[group >> 3],
                                               &cntUsed);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            /* Available counter resource. Index 0 is not usable */
            tunnelUsage->countAvailable = FM10000_TE_STATS_ENTRIES_0 - cntUsed - 1;

            /* Total counter resource. Index 0 is not usable */
            tunnelUsage->countTotal = FM10000_TE_STATS_ENTRIES_0 - 1;

            break;

        case FM_TUNNEL_GLORT_USER:
            glortUser = (fm_tunnelGlortUser*) value;

            /* Group GLORT/USER value with mask */
            if (rule < 0)
            {
                glortUser->glort = tunnelGrp->teDGlort.glortValue;
                glortUser->glortMask = tunnelGrp->teDGlort.glortMask;
                glortUser->user = tunnelGrp->teDGlort.userValue;
                glortUser->userMask = tunnelGrp->teDGlort.userMask;
            }
            /* Single rule/tep GLORT/USER value with mask */
            else
            {
                if (tunnelGrp->teDGlort.lookupType == FM_FM10000_TE_LOOKUP_DIRECT)
                {
                    if (fmTreeFind(&tunnelGrp->rules, rule, &nextValue) != FM_OK)
                    {
                        err = FM_ERR_TUNNEL_INVALID_ENTRY;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }

                    tunnelRule = (fm_fm10000TunnelRule *) nextValue;

                    if (tunnelGrp->tunnelParam.useUser)
                    {
                        glortUser->glort = tunnelGrp->teDGlort.glortValue +
                                           (tunnelRule->lookupBin >> 6);
                        glortUser->glortMask = 0xFFFF;
                        glortUser->user = (tunnelRule->lookupBin << 2) & 0xFC;
                        glortUser->userMask = 0xFC;
                    }
                    else
                    {
                        glortUser->glort = tunnelGrp->teDGlort.glortValue +
                                           tunnelRule->lookupBin;
                        glortUser->glortMask = 0xFFFF;
                        glortUser->user = 0;
                        glortUser->userMask = 0;
                    }
                }
                else
                {
                    /* Make sure the tep retrieved is in the proper range */
                    if ( (rule != 0) &&
                         (rule >= tunnelGrp->tunnelParam.tepSize) )
                    {
                        err = FM_ERR_TUNNEL_TEP_SIZE;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                    }

                    if (tunnelGrp->tunnelParam.useUser)
                    {
                        glortUser->glort = tunnelGrp->teDGlort.glortValue +
                                           (rule >> 6);
                        glortUser->glortMask = 0xFFFF;
                        glortUser->user = (rule << 2) & 0xFC;
                        glortUser->userMask = 0xFC;
                    }
                    else
                    {
                        glortUser->glort = tunnelGrp->teDGlort.glortValue +
                                           rule;
                        glortUser->glortMask = 0xFFFF;
                        glortUser->user = 0;
                        glortUser->userMask = 0;
                    }
                }
            }

            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;
    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelAttribute */




/*****************************************************************************/
/** fm10000SetTunnelApiAttribute
 * \ingroup intTunnel
 *
 * \desc            Set a Tunnel Api attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the Tunnel API Attribute (see ''fm_tunnelApiAttr'')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetTunnelApiAttribute(fm_int sw,
                                       fm_int attr,
                                       void * value)
{
    fm_status             err = FM_OK;
    fm_bool               tunnelLockTaken = FALSE;
    fm_switch *           switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *      switchExt = (fm10000_switch *) switchPtr->extension;
    fm_int                i;
    fm_tunnelModeAttr *   modeAttr;
    fm_fm10000TeTunnelCfg tunDefCfg;
    fm_fm10000TeParserCfg parserCfg;
    fm_uint32             defSelectMask;
    fm_uint32             parserSelectMask;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, attr = %d\n",
                 sw, attr);

    if (value == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    switch (attr)
    {
        case FM_TUNNEL_API_MODE:
            modeAttr = (fm_tunnelModeAttr *) value;

            if (modeAttr->te >= FM10000_TE_TUN_HEADER_CFG_ENTRIES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            if (modeAttr->mode >= FM_TUNNEL_API_MODE_MAX)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            for (i = 0; i < FM10000_TE_DGLORT_MAP_ENTRIES_0; i++)
            {
                if (switchExt->tunnelCfg->tunnelGrp[modeAttr->te][i].active == TRUE)
                {
                    err = FM_ERR_TUNNEL_IN_USE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
            }

            defSelectMask    = FM10000_TE_DEFAULT_TUNNEL_MODE;
            parserSelectMask = 0;

            tunDefCfg.mode = modeAttr->mode;

            /* Also initialize some of the TE defaults to match the mode */
            if (modeAttr->mode == FM_TUNNEL_API_MODE_VXLAN_GPE_NSH)
            {
                defSelectMask |= FM10000_TE_DEFAULT_TUNNEL_PROTOCOL |
                                 FM10000_TE_DEFAULT_GPE_NSH_CLEAR |
                                 FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE;
                tunDefCfg.encapProtocol = 0x0403;
                tunDefCfg.l4DstNge = switchExt->vnGpeUdpPort;

                parserSelectMask  = FM10000_TE_PARSER_NGE_PORT;
                parserCfg.ngePort = switchExt->vnGpeUdpPort;
            }
            else if (modeAttr->mode == FM_TUNNEL_API_MODE_VXLAN_NVGRE_NGE)
            {
                defSelectMask |= FM10000_TE_DEFAULT_TUNNEL_PROTOCOL |
                                 FM10000_TE_DEFAULT_GPE_NSH_CLEAR |
                                 FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE;
                tunDefCfg.encapProtocol = GET_PROPERTY()->vnEncapProtocol;
                tunDefCfg.l4DstNge = switchExt->vnGeneveUdpPort;

                parserSelectMask  = FM10000_TE_PARSER_NGE_PORT;
                parserCfg.ngePort = switchExt->vnGeneveUdpPort;
            }

            err = fm10000SetTeDefaultTunnel(sw,
                                            modeAttr->te,
                                            &tunDefCfg,
                                            defSelectMask,
                                            TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            err = fm10000SetTeParser(sw,
                                     modeAttr->te,
                                     &parserCfg,
                                     parserSelectMask,
                                     TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

            switchExt->tunnelCfg->tunnelMode[modeAttr->te] = modeAttr->mode;
            break; 

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;
    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTunnelApiAttribute */




/*****************************************************************************/
/** fm10000GetTunnelApiAttribute
 * \ingroup intTunnel
 *
 * \desc            Get a Tunnel Api attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the Tunnel API Attribute (see ''fm_tunnelApiAttr'')
 *                  to set.
 *
 * \param[out]      value points to the attribute value to get.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on NULL pointer or out of range value.
 * \return          FM_ERR_UNSUPPORTED if attribute is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetTunnelApiAttribute(fm_int sw,
                                       fm_int attr,
                                       void * value)
{
    fm_status          err = FM_OK;
    fm_bool            tunnelLockTaken = FALSE;
    fm_switch *        switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *   switchExt = (fm10000_switch *) switchPtr->extension;
    fm_tunnelModeAttr *modeAttr;

    FM_LOG_ENTRY(FM_LOG_CAT_TE,
                 "sw = %d, attr = %d\n",
                 sw, attr);

    if (value == NULL)
    {
       err = FM_ERR_INVALID_ARGUMENT;
       FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    TAKE_TUNNEL_LOCK(sw);
    tunnelLockTaken = TRUE;

    switch (attr)
    {
        case FM_TUNNEL_API_MODE:
            modeAttr = (fm_tunnelModeAttr *) value;

            /* Sanity Check on input */
            if (modeAttr->te >= FM10000_TE_TUN_HEADER_CFG_ENTRIES)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            }

            /* Return the mode */
            modeAttr->mode = switchExt->tunnelCfg->tunnelMode[modeAttr->te];
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
            break;
    }

ABORT:
    if (tunnelLockTaken)
    {
        DROP_TUNNEL_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTunnelApiAttribute */




/*****************************************************************************/
/** fm10000DbgDumpTunnel
 * \ingroup intDiagTunnel
 *
 * \desc            Display the tunnel engine status of a switch
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpTunnel(fm_int sw)
{
    fm_status err = FM_OK;
    fm_fm10000TunnelTeDataCtrl *teDataCtrl;
    fm_int te;
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_int i;
    fm_int start;
    char   oldLine[BUFFER_SIZE];
    char   newLine[BUFFER_SIZE];

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    fm10000DbgDumpTe(sw, 0);
    fm10000DbgDumpTe(sw, 1);

    for (te = 0 ; te < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; te++)
    {
        FM_LOG_PRINT("===============================================================================\n");
        FM_LOG_PRINT("Tunneling API Structure for TE %d\n", te);
        FM_LOG_PRINT("===============================================================================\n\n");

        teDataCtrl = &switchExt->tunnelCfg->teDataCtrl[te];

        FM_LOG_PRINT("teDataFreeEntryCount:       %d\n", teDataCtrl->teDataFreeEntryCount);
        FM_LOG_PRINT("teDataHandlerFirstFreeEntry:%d\n", teDataCtrl->teDataHandlerFirstFreeEntry);
        FM_LOG_PRINT("teDataSwapSize:             %d\n", teDataCtrl->teDataSwapSize);
        FM_LOG_PRINT("lastTeDataBlkCtrlIndex:     %d\n\n", teDataCtrl->lastTeDataBlkCtrlIndex);

        start = 0;
        oldLine[0] = 0;
        FM_LOG_PRINT("  TeData     Handler Index Length TunnelGrp TunnelType TunnelEntry\n"
                     "-----------   -----  -----  ----      --     ---------  --------- \n");
        for (i = 0 ; i < FM10000_TE_DATA_ENTRIES_0; i++)
        {
            if (teDataCtrl->teDataHandler[i])
            {
                FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                              " %5d  %5d  %4d      %2d     %s  %9d",
                              teDataCtrl->teDataHandler[i],
                              teDataCtrl->teDataBlkCtrl[teDataCtrl->teDataHandler[i]]->index,
                              teDataCtrl->teDataBlkCtrl[teDataCtrl->teDataHandler[i]]->length,
                              teDataCtrl->teDataBlkCtrl[teDataCtrl->teDataHandler[i]]->tunnelGrp,
                              (teDataCtrl->teDataBlkCtrl[teDataCtrl->teDataHandler[i]]->tunnelDataType ==
                              FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN) ? "      Bin" : "EncapFlow",
                              teDataCtrl->teDataBlkCtrl[teDataCtrl->teDataHandler[i]]->tunnelEntry);
            }
            else
            {
                FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                              " %5d  -----  ----      --     ---------  ---------",
                              0);
            }

            if (strcmp(oldLine, newLine) != 0)
            {
                if (oldLine[0])
                {
                    PrintLine(start, i - 1, oldLine);
                }

                FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
                start = i;
            }
        }
        PrintLine(start, i - 1, oldLine);

    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000DbgDumpTunnel */
