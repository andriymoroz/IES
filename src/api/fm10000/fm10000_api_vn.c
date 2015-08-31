/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_vn.c
 * Creation Date:   Mar 4, 2014
 * Description:     Virtual Network Services
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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
#define ENCAP_TUNNEL                    0
#define DECAP_TUNNEL                    1
#define BASE_ENCAP_ACL_RULE             1
#define BASE_FLOODSET_ENCAP_ACL_RULE    200000
#define BASE_DECAP_ACL_RULE             1
#define ACL_RULE_TYPE_NORMAL            0
#define ACL_RULE_TYPE_ADDR_MASK         1
#define ACL_RULE_TYPE_FLOODSET          2
#define STATUS_TEXT_LEN                 1024

#define IP_CHKSUM_ADD(cksum,addend)                                 \
    cksum += addend;                                                \
    if ( (cksum & 0xffff0000) != 0 )                                \
    {                                                               \
        cksum = ( (cksum >> 16) & 0xffff ) + (cksum & 0xffff);      \
    }

#define GET_ENCAP_FLOW_TYPE(vn)                                     \
    ( (vn->descriptor.mode == FM_VN_MODE_TRANSPARENT)               \
        ? FM_VN_ENCAP_FLOW_TRANSPARENT                              \
        : FM_VN_ENCAP_FLOW_VSWITCH )

#define GET_ENCAP_TUNNEL_GROUP(addrRec)                             \
        ( (addrRec->addrMask == NULL)                               \
         ? FM_VN_ENCAP_GROUP_DIRECT                                 \
         : ( (addrRec->addrMask->tunnel == NULL)                    \
            ? addrRec->hashEncapGroup : -1 ) )

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static fm_status GetTunnelGroupParams(fm_int          sw,
                                      fm_int          tunnelGroup,
                                      fm_tunnelParam *groupParams);
static fm_status WriteDecapAclRule(fm_int                  sw,
                                   fm10000_vnDecapAclRule *decapAclRule);
static fm_status CreateDecapAclRule(fm_int                   sw,
                                    fm_virtualNetwork *      vn,
                                    fm_vnTunnel *            tunnel,
                                    fm_int                   decapTunnelGroup,
                                    fm10000_vnDecapAclRule **decapAclRulePtr);
static fm_status GetDecapAclRule(fm_int                   sw,
                                 fm_virtualNetwork *      vn,
                                 fm_vnTunnel *            tunnel,
                                 fm10000_vnDecapAclRule **decapAclRulePtr);
static fm_int CompareDecapAclRules(const void *first, const void *second);
static fm_status WriteEncapTunnelRule(fm_int                   sw,
                                      fm10000_vnRemoteAddress *addrRec);
static fm_status BuildTunnelRule(fm_int                   sw,
                                 fm_virtualNetwork *      vn,
                                 fm_vnTunnel *            tunnel,
                                 fm_int                   tunnelGroup,
                                 fm_int                   encapFlowId,
                                 fm_vnAddress *           addr,
                                 fm_int                   vsi,
                                 fm_tunnelCondition *     ruleCond,
                                 fm_tunnelConditionParam *ruleCondParam,
                                 fm_tunnelAction *        ruleAction,
                                 fm_tunnelActionParam *   ruleActionParam);




/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** FreeCustomTreeRecord
 * \ingroup intVN
 *
 * \desc            Frees a custom tree record, called from fmCustomTreeDestroy.
 *
 * \param[in]       key points to the record key.
 *
 * \param[in]       record points to the data record.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FreeCustomTreeRecord(void *key, void *record)
{
    if ( (key != NULL) && (key != record) )
    {
        fmFree(key);
    }

    if (record != NULL)
    {
        fmFree(record);
    }

}   /* end FreeCustomTreeRecord */




/*****************************************************************************/
/** FreeRemoteAddressRecord
 * \ingroup intVN
 *
 * \desc            Frees a remote address record, called from fmCustomTreeDestroy.
 *
 * \param[in]       key points to the record key.
 *
 * \param[in]       record points to the data record.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FreeRemoteAddressRecord(void *key, void *record)
{
    fm10000_vnRemoteAddress *remoteAddr;

    if ( (key != record) && (key != NULL) )
    {
        fmFree(key);
    }

    remoteAddr = (fm10000_vnRemoteAddress *) record;

    if (remoteAddr != NULL)
    {
        fmFree(remoteAddr);
    }

}   /* end FreeRemoteAddressRecord */




/*****************************************************************************/
/** TranslateChecksumAction
 * \ingroup intVN
 *
 * \desc            Translates a VN checksum action into a TE checksum action.
 *
 * \param[in]       vnChksumAction is the VN checksum action.
 *
 * \return          The TE checksum action.
 *
 *****************************************************************************/
static fm_fm10000TeChecksumAction TranslateChecksumAction(fm_vnChecksumAction vnChksumAction)
{
    fm_fm10000TeChecksumAction teChksumAction;

    switch (vnChksumAction)
    {
        case FM_VN_CHECKSUM_TRAP:
            teChksumAction = FM_FM10000_TE_CHECKSUM_TRAP;
            break;

        case FM_VN_CHECKSUM_ZERO:
            teChksumAction = FM_FM10000_TE_CHECKSUM_ZERO;
            break;

        case FM_VN_CHECKSUM_COMPUTE:
            teChksumAction = FM_FM10000_TE_CHECKSUM_COMPUTE;
            break;

        case FM_VN_CHECKSUM_HEADER:
            teChksumAction = FM_FM10000_TE_CHECKSUM_HEADER;
            break;

        default:
            teChksumAction = FM_FM10000_TE_CHECKSUM_MAX;
            break;
    }

    return teChksumAction;

}   /* end TranslateChecksumAction */




/*****************************************************************************/
/** TranslateTeChecksumAction
 * \ingroup intVN
 *
 * \desc            Translates a TE checksum action into a VN checksum action.
 *
 * \param[in]       teChksumAction is the TE checksum action.
 *
 * \return          The VN checksum action.
 *
 *****************************************************************************/
static fm_vnChecksumAction TranslateTeChecksumAction(fm_fm10000TeChecksumAction teChksumAction)
{
    fm_vnChecksumAction vnChksumAction;

    switch (teChksumAction)
    {
        case FM_FM10000_TE_CHECKSUM_TRAP:
            vnChksumAction = FM_VN_CHECKSUM_TRAP;
            break;

        case FM_FM10000_TE_CHECKSUM_ZERO:
            vnChksumAction = FM_VN_CHECKSUM_ZERO;
            break;

        case FM_FM10000_TE_CHECKSUM_COMPUTE:
            vnChksumAction = FM_VN_CHECKSUM_COMPUTE;
            break;

        case FM_FM10000_TE_CHECKSUM_HEADER:
            vnChksumAction = FM_VN_CHECKSUM_HEADER;
            break;

        default:
            vnChksumAction = FM_VN_CHECKSUM_MAX;
            break;
    }

    return vnChksumAction;

}   /* end TranslateTeChecksumAction */




/*****************************************************************************/
/** WriteVsi
 * \ingroup intVN
 *
 * \desc            Writes VSI information to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record. If NULL, the
 *                  hardware registers for this VSI will be cleared.
 *
 * \param[in]       vsi is the VSI number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteVsi(fm_int             sw,
                          fm_virtualNetwork *vn,
                          fm_int             vsi)
{
    fm_status          status;
    fm_fm10000TeTepCfg teTepCfg;
    fm_uint32          teFieldSelectMask;
    fm_vsiData         vsiData;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, vsi = %d\n",
                 sw,
                 (void *) vn,
                 vsi);

    FM_CLEAR(teTepCfg);

    teFieldSelectMask = FM10000_TE_DEFAULT_TEP_VNI;

    FM_CLEAR(vsiData);

    vsiData.te       = ENCAP_TUNNEL;
    vsiData.vsi      = vsi;
    vsiData.dataMask = FM_VSI_DATA_ENCAP_VNI;

    if (vn != NULL)
    {
        teTepCfg.vni     = vn->vsId;
        vsiData.encapVni = vn->vsId;

        if ( !fmIsIPAddressEmpty(&vn->descriptor.sip) )
        {
            vsiData.dataMask |= FM_VSI_DATA_ENCAP_SIP;
            vsiData.encapSip  = vn->descriptor.sip;
        }
    }
    else
    {
        vsiData.dataMask |= FM_VSI_DATA_ENCAP_SIP;
    }

    status = fm10000SetTeDefaultTep(sw,
                                    ENCAP_TUNNEL,
                                    vsi,
                                    &teTepCfg,
                                    teFieldSelectMask,
                                    TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmSetSwitchAttribute(sw,
                                  FM_SWITCH_VSI_DATA,
                                  &vsiData);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end WriteVsi */




/*****************************************************************************/
/** GetTunnelGroups
 * \ingroup intVN
 *
 * \desc            Returns the encap and decap tunnel groups for a
 *                  remote address type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrType is the remote address type.
 *
 * \param[out]      encapTunnelGroup points to caller-provided storage into
 *                  which the encap tunnel group will be written.
 *
 * \param[out]      decapTunnelGroup points to caller-provided storage into
 *                  which the decap tunnel group will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTunnelGroups(fm_int           sw,
                                 fm_vnAddressType addrType,
                                 fm_int *         encapTunnelGroup,
                                 fm_int *         decapTunnelGroup)
{
    fm_int encap;
    fm_int decap;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrType = %d, encapTunnelGroup = %p, decapTunnelGroup = %p\n",
                 sw,
                 addrType,
                 (void *) encapTunnelGroup,
                 (void *) decapTunnelGroup);

    switch (addrType)
    {
        case FM_VN_ADDR_TYPE_IP:
            encap = FM_VN_ENCAP_GROUP_DIP_VID;
            decap = FM_VN_DECAP_GROUP_DIP_VID;
            break;

        case FM_VN_ADDR_TYPE_MAC:
            encap = FM_VN_ENCAP_GROUP_DMAC_VID;
            decap = FM_VN_DECAP_GROUP_DMAC_VID;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_UNSUPPORTED);
    }

    if (encapTunnelGroup != NULL)
    {
        *encapTunnelGroup = encap;
    }

    if (decapTunnelGroup != NULL)
    {
        *decapTunnelGroup = decap;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end GetTunnelGroups */




/*****************************************************************************/
/** ConvertTunnelGroupToString
 * \ingroup intVN
 *
 * \desc            Converts a tunnel group number into a string containing
 *                  the group's name.
 *
 * \param[in]       tunnelGroup specifies the VN tunnel group number.
 *
 * \param[in]       textBufSize is the size of textOut buffer.
 *
 * \param[out]      textOut points to caller-allocated storage where
 *                  the string will be stored.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void ConvertTunnelGroupToString(fm_int tunnelGroup,
                                       fm_int textBufSize,
                                       fm_char *textOut)
{
    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
            FM_STRCPY_S(textOut, textBufSize, "ENCAP_GROUP_DMAC_VID");
            break;
        case FM_VN_ENCAP_GROUP_DIP_VID:
            FM_STRCPY_S(textOut, textBufSize, "ENCAP_GROUP_DIP_VID");
            break;
        case FM_VN_ENCAP_GROUP_DIRECT:
            FM_STRCPY_S(textOut, textBufSize, "ENCAP_GROUP_DIRECT");
            break;
        case FM_VN_DECAP_GROUP_DMAC_VID:
            FM_STRCPY_S(textOut, textBufSize, "DECAP_GROUP_DMAC_VID");
            break;
        case FM_VN_DECAP_GROUP_DIP_VID:
            FM_STRCPY_S(textOut, textBufSize, "DECAP_GROUP_DIP_VID");
            break;
        case FM_VN_DECAP_GROUP_DIRECT:
            FM_STRCPY_S(textOut, textBufSize, "DECAP_GROUP_DIRECT");
            break;
        case -1:
            FM_STRCPY_S(textOut, textBufSize, "-1");
            break;
        default:
            FM_STRCPY_S(textOut, textBufSize, "unknown");
    }
}




/*****************************************************************************/
/** ConvertTunnelTypeToString
 * \ingroup intVN
 *
 * \desc            Converts a tunnel type number into a string containing
 *                  the tunnel type's name.
 *
 * \param[in]       tunnelType specifies the protocol used by a tunnel.
 *
 * \param[in]       textBufSize is the size of textOut buffer.
 *
 * \param[out]      textOut points to caller-allocated storage where
 *                  the string will be stored.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void ConvertTunnelTypeToString(fm_vnTunnelType tunnelType,
                                      fm_int textBufSize,
                                      fm_char *textOut)
{
    switch (tunnelType)
    {
        case FM_VN_TUNNEL_TYPE_VXLAN_IPV4:
            FM_STRCPY_S(textOut, textBufSize, "VXLAN_IPV4");
            break;
        case FM_VN_TUNNEL_TYPE_VXLAN_IPV6:
            FM_STRCPY_S(textOut, textBufSize, "VXLAN_IPV6");
            break;
        case FM_VN_TUNNEL_TYPE_NVGRE:
            FM_STRCPY_S(textOut, textBufSize, "NVGRE");
            break;
        case FM_VN_TUNNEL_TYPE_GENEVE:
            FM_STRCPY_S(textOut, textBufSize, "GENEVE");
            break;
        default:
            FM_STRCPY_S(textOut, textBufSize, "unknown");
    }
}




/*****************************************************************************/
/** PrintBitArray
 * \ingroup intVN
 *
 * \desc            Dump a bit array for debugging purposes.
 *
 * \param[in]       bitArray points to the bit array.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void PrintBitArray(fm_bitArray *bitArray)
{
    FM_LOG_PRINT("bit array:");
    if (bitArray->bitCount > 0)
    {
        FM_LOG_PRINT("\n----------");
        fmDbgDumpBitArray(bitArray, bitArray->bitCount);
    }
    else
    {
        FM_LOG_PRINT(" [zero size]\n\n");
    }
}




/*****************************************************************************/
/** CreateTunnelGroup
 * \ingroup intVN
 *
 * \desc            Creates a tunnel group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup specifies the VN tunnel group number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateTunnelGroup(fm_int  sw,
                                   fm_int  tunnelGroup)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_tunnelParam          tunnelParam;
    fm_bool                 setDglort;
    fm_tunnelCondition      ruleCond;
    fm_tunnelConditionParam ruleCondParam;
    fm_tunnelAction         ruleAction;
    fm_tunnelActionParam    ruleActionParam;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d, tunnelGroup = %d\n", sw, tunnelGroup);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->vnTunnelGroups[tunnelGroup] < 0)
    {
        status = GetTunnelGroupParams(sw, tunnelGroup, &tunnelParam);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fm10000CreateTunnel(sw,
                                     &switchExt->vnTunnelGroups[tunnelGroup],
                                     &tunnelParam);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "Tunnel Group %d created, group ID = %d,\n",
                     tunnelGroup,
                     switchExt->vnTunnelGroups[tunnelGroup]);

        setDglort = TRUE;

        status = fm10000SetTunnelAttribute(sw,
                                           switchExt->vnTunnelGroups[tunnelGroup],
                                           0,
                                           FM_TUNNEL_SET_DEFAULT_DGLORT,
                                           &setDglort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        if (tunnelGroup == FM_VN_DECAP_GROUP_DIRECT)
        {
            status = BuildTunnelRule(sw,
                                     NULL,
                                     NULL,
                                     FM_VN_DECAP_GROUP_DIRECT,
                                     0,
                                     NULL,
                                     0,
                                     &ruleCond,
                                     &ruleCondParam,
                                     &ruleAction,
                                     &ruleActionParam);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            status = fm10000AddTunnelRule(sw,
                                          switchExt->vnTunnelGroups[FM_VN_DECAP_GROUP_DIRECT],
                                          0,
                                          ruleCond,
                                          &ruleCondParam,
                                          ruleAction,
                                          &ruleActionParam);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end CreateTunnelGroup */




/*****************************************************************************/
/** AllocateTunnelRuleNum
 * \ingroup intVN
 *
 * \desc            Returns the next available tunnel group rule number.
 *                  The returned rule number will be marked in-use.
 *                  If the specified tunnel group has not yet been created,
 *                  it will be created and configured.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup specifies the VN tunnel group number
 *
 * \param[out]      ruleNum points to caller-provided storage into which
 *                  the tunnel rule number will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if all default rule numbers are in use.
 *
 *****************************************************************************/
static fm_status AllocateTunnelRuleNum(fm_int  sw,
                                       fm_int  tunnelGroup,
                                       fm_int *ruleNum)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_int          bitNum;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelGroup = %d, ruleNum = %p\n",
                 sw,
                 tunnelGroup,
                 (void *) ruleNum);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->vnTunnelGroups[tunnelGroup] < 0)
    {
        status = CreateTunnelGroup(sw, tunnelGroup);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmFindBitInBitArray(&switchExt->vnTunnelRuleIds[tunnelGroup],
                                 0,
                                 0,
                                 &bitNum);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (bitNum < 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MORE);
    }

    status = fmSetBitArrayBit(&switchExt->vnTunnelRuleIds[tunnelGroup],
                              bitNum,
                              1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    *ruleNum = bitNum;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end AllocateTunnelRuleNum */




/*****************************************************************************/
/** FreeTunnelRuleNum
 * \ingroup intVN
 *
 * \desc            Frees a tunnel group rule number.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup is the tunnel group number
 *
 * \param[in]       ruleNum is the rule number to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeTunnelRuleNum(fm_int sw, fm_int tunnelGroup, fm_int ruleNum)
{
    fm_status       status;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelGroup = %d, ruleNum = %d\n",
                 sw,
                 tunnelGroup,
                 ruleNum);

    switchExt = GET_SWITCH_EXT(sw);

    status = fmSetBitArrayBit(&switchExt->vnTunnelRuleIds[tunnelGroup],
                              ruleNum,
                              0);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end FreeTunnelRuleNum */




/*****************************************************************************/
/** AllocateTunnelAclRuleNum
 * \ingroup intVN
 *
 * \desc            Returns the next available tunnel ACL rule number.
 *                  The returned rule number will be marked in-use.
 *                  If the specified ACL has not yet been created,
 *                  it will be created.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup specifies the VN tunnel group number
 *
 * \param[in]       ruleType specifies the type of ACL rule needed:
 *                  ACL_RULE_TYPE_NORMAL for a normal remote address,
 *                  ACL_RULE_TYPE_ADDR_MASK for a remote address mask entry,
 *                  ACL_RULE_TYPE_FLOODSET for a floodset rule.
 *
 * \param[out]      ruleNum points to caller-provided storage into which
 *                  the tunnel rule number will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if all default rule numbers are in use.
 *
 *****************************************************************************/
static fm_status AllocateTunnelAclRuleNum(fm_int  sw,
                                          fm_int  tunnelGroup,
                                          fm_int  ruleType,
                                          fm_int *ruleNum)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_int          bitNum;
    fm_bitArray *   bitArray;
    fm_int          baseRule;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelGroup = %d, ruleType = %d, ruleNum = %p\n",
                 sw,
                 tunnelGroup,
                 ruleType,
                 (void *) ruleNum);

    switchExt = GET_SWITCH_EXT(sw);

    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
        case FM_VN_ENCAP_GROUP_DIP_VID:
        case FM_VN_ENCAP_GROUP_DIRECT:
            switch (ruleType)
            {
                case ACL_RULE_TYPE_NORMAL:
                case ACL_RULE_TYPE_ADDR_MASK:
                    bitArray = &switchExt->vnEncapAclRuleNumbers;
                    baseRule = BASE_ENCAP_ACL_RULE;
                    break;

                case ACL_RULE_TYPE_FLOODSET:
                    bitArray = &switchExt->vnEncapAclFloodsetRuleNumbers;
                    baseRule = BASE_FLOODSET_ENCAP_ACL_RULE;
                    break;

                default:
                    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
            }
            break;

        case FM_VN_DECAP_GROUP_DMAC_VID:
        case FM_VN_DECAP_GROUP_DIP_VID:
        case FM_VN_DECAP_GROUP_DIRECT:
            bitArray = &switchExt->vnDecapAclRuleNumbers;
            baseRule = BASE_DECAP_ACL_RULE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    status = fmFindBitInBitArray(bitArray, 0, 0, &bitNum);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (bitNum < 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MORE);
    }

    status = fmSetBitArrayBit(bitArray, bitNum, 1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    *ruleNum = bitNum + baseRule;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end AllocateTunnelAclRuleNum */




/*****************************************************************************/
/** FreeTunnelAclRuleNum
 * \ingroup intVN
 *
 * \desc            Frees a tunnel ACL rule number.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup is the tunnel group number
 *
 * \param[in]       ruleType specifies the type of ACL rule needed:
 *                  ACL_RULE_TYPE_NORMAL for a normal remote address,
 *                  ACL_RULE_TYPE_ADDR_MASK for a remote address mask entry,
 *                  ACL_RULE_TYPE_FLOODSET for a floodset rule.
 *
 * \param[in]       ruleNum is the rule number to be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FreeTunnelAclRuleNum(fm_int  sw,
                                      fm_int  tunnelGroup,
                                      fm_int  ruleType,
                                      fm_int  ruleNum)
{
    fm_status       status;
    fm_bitArray *   bitArray;
    fm10000_switch *switchExt;
    fm_int          baseRule;
    fm_int          bitNum;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelGroup = %d, ruleNum = %d\n",
                 sw,
                 tunnelGroup,
                 ruleNum);

    switchExt = GET_SWITCH_EXT(sw);

    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
        case FM_VN_ENCAP_GROUP_DIP_VID:
        case FM_VN_ENCAP_GROUP_DIRECT:
            switch (ruleType)
            {
                case ACL_RULE_TYPE_NORMAL:
                case ACL_RULE_TYPE_ADDR_MASK:
                    bitArray = &switchExt->vnEncapAclRuleNumbers;
                    baseRule = BASE_ENCAP_ACL_RULE;
                    break;

                case ACL_RULE_TYPE_FLOODSET:
                    bitArray = &switchExt->vnEncapAclFloodsetRuleNumbers;
                    baseRule = BASE_FLOODSET_ENCAP_ACL_RULE;
                    break;

                default:
                    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
            }
            break;

        case FM_VN_DECAP_GROUP_DMAC_VID:
        case FM_VN_DECAP_GROUP_DIP_VID:
        case FM_VN_DECAP_GROUP_DIRECT:
            bitArray = &switchExt->vnDecapAclRuleNumbers;
            baseRule = BASE_DECAP_ACL_RULE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    bitNum = ruleNum - baseRule;

    status = fmSetBitArrayBit(bitArray, bitNum, 0);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end FreeTunnelAclRuleNum */




/*****************************************************************************/
/** GetTunnelGroupParams
 * \ingroup intVN
 *
 * \desc            Returns the encapsulation or decapsulation parameters
 *                  for a tunnel group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnelGroup is the tunnel group number
 *
 * \param[out]      groupParams points to caller-provided storage into which
 *                  the group's encapsulation or decapsulation parameters
 *                  will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTunnelGroupParams(fm_int          sw,
                                      fm_int          tunnelGroup,
                                      fm_tunnelParam *groupParams)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelGroup = %d, groupParams = %p\n",
                 sw,
                 tunnelGroup,
                 (void *) groupParams);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    FM_CLEAR(*groupParams);

    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
            groupParams->te            = ENCAP_TUNNEL;
            groupParams->encap         = TRUE;
            groupParams->hashKeyConfig = FM_TUNNEL_MATCH_DMAC | FM_TUNNEL_MATCH_VSI_TEP;
            groupParams->size          = switchExt->vnTunnelGroupHashSize;
            break;

        case FM_VN_ENCAP_GROUP_DIP_VID:
            groupParams->te            = ENCAP_TUNNEL;
            groupParams->encap         = TRUE;
            groupParams->hashKeyConfig = FM_TUNNEL_MATCH_DIP | FM_TUNNEL_MATCH_VSI_TEP;
            groupParams->size          = switchExt->vnTunnelGroupHashSize;
            break;

        case FM_VN_ENCAP_GROUP_DIRECT:
            groupParams->te            = ENCAP_TUNNEL;
            groupParams->encap         = TRUE;
            groupParams->hashKeyConfig = 0;
            groupParams->size          = switchExt->vnTunnelGroupHashSize;
            break;

        case FM_VN_DECAP_GROUP_DMAC_VID:
            groupParams->te            = DECAP_TUNNEL;
            groupParams->encap         = FALSE;
            groupParams->hashKeyConfig = FM_TUNNEL_MATCH_SMAC | FM_TUNNEL_MATCH_VNI;
            groupParams->size          = switchExt->vnTunnelGroupHashSize;
            break;

        case FM_VN_DECAP_GROUP_DIP_VID:
            groupParams->te            = DECAP_TUNNEL;
            groupParams->encap         = FALSE;
            groupParams->hashKeyConfig = FM_TUNNEL_MATCH_SIP | FM_TUNNEL_MATCH_VNI;
            groupParams->size          = switchExt->vnTunnelGroupHashSize;
            break;

        case FM_VN_DECAP_GROUP_DIRECT:
            groupParams->te            = DECAP_TUNNEL;
            groupParams->encap         = FALSE;
            groupParams->hashKeyConfig = 0;
            groupParams->size          = 1;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end GetTunnelGroupParams */




/*****************************************************************************/
/** InitializeVNSubsystem
 * \ingroup intVN
 *
 * \desc            Initializes the Virtual Networking Subsystem. Called when
 *                  creating the first virtual network or the first tunnel,
 *                  whichever comes first.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitializeVNSubsystem(fm_int sw)
{
    fm_status       status;
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_int          i;
    fm_bool         routable;
    fm_aclArguments aclArgs;
    fm_bool         useDefaultVlan;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    routable  = FM_ENABLED;

    if ( (switchExt->numVirtualNetworks != 0)
         || (switchExt->numVNTunnels != 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /************************************************************************
     * Clean up previous state.
     ************************************************************************/
    /* Delete any existing tunnel groups. */
    for (i = 0 ; i < FM_VN_NUM_TUNNEL_GROUPS ; i++)
    {
        if (switchExt->vnTunnelGroups[i] >= 0)
        {
            status = fm10000DeleteTunnel(sw, switchExt->vnTunnelGroups[i]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }

        status = fmDeleteBitArray(&switchExt->vnTunnelRuleIds[i]);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        switchExt->vnTunnelGroups[i] = -1;
    }

    /* Delete ACL Rule number bit arrays. */
    status = fmDeleteBitArray(&switchExt->vnEncapAclRuleNumbers);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmDeleteBitArray(&switchExt->vnEncapAclFloodsetRuleNumbers);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmDeleteBitArray(&switchExt->vnDecapAclRuleNumbers);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Delete each encapsulation flow id bit array. */
    status = fmDeleteBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DMAC_VID]);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmDeleteBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DIP_VID]);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmDeleteBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DIRECT]);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Delete pre-existing encap and decap ACLs. */
    if ( (switchExt->vnEncapAcl != 0) && ( fmGetACL(sw, switchExt->vnEncapAcl, &aclArgs) == FM_OK ) )
    {
        status = fmDeleteACLInt(sw, switchExt->vnEncapAcl, TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (switchExt->vnDecapAcl != 0) && ( fmGetACL(sw, switchExt->vnDecapAcl, &aclArgs) == FM_OK ) )
    {
        status = fmDeleteACLInt(sw, switchExt->vnDecapAcl, TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    /* Destroy the decap ACL rule tree. */
    if ( fmCustomTreeIsInitialized(&switchExt->vnDecapAclRuleTree) )
    {
        fmCustomTreeDestroy(&switchExt->vnDecapAclRuleTree, FreeCustomTreeRecord);
    }

    /************************************************************************
     * Retrieve new API attribute values.
     ************************************************************************/
    switchExt->useSharedEncapFlows  = fmGetBoolApiProperty(FM_AAK_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS,
                                                           FM_AAD_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS);
    switchExt->maxVNRemoteAddresses = fmGetIntApiProperty(FM_AAK_API_FM10000_VN_MAX_TUNNEL_RULES,
                                                          FM_AAD_API_FM10000_VN_MAX_TUNNEL_RULES);
    switchExt->vnTunnelGroupHashSize = fmGetIntApiProperty(FM_AAK_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE,
                                                           FM_AAD_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE);
    switchExt->vnTeVid               = fmGetIntApiProperty(FM_AAK_API_FM10000_VN_TE_VID,
                                                           FM_AAD_API_FM10000_VN_TE_VID);
    switchExt->vnEncapProtocol       = fmGetIntApiProperty(FM_AAK_API_VN_ENCAP_PROTOCOL,
                                                           FM_AAD_API_VN_ENCAP_PROTOCOL);
    switchExt->vnEncapVersion        = fmGetIntApiProperty(FM_AAK_API_VN_ENCAP_VERSION,
                                                           FM_AAD_API_VN_ENCAP_VERSION);
    switchExt->vnEncapAcl            = fmGetIntApiProperty(FM_AAK_API_FM10000_VN_ENCAP_ACL_NUM,
                                                           FM_AAD_API_FM10000_VN_ENCAP_ACL_NUM);
    switchExt->vnDecapAcl            = fmGetIntApiProperty(FM_AAK_API_FM10000_VN_DECAP_ACL_NUM,
                                                           FM_AAD_API_FM10000_VN_DECAP_ACL_NUM);

    /************************************************************************
     * Initialize new state.
     ************************************************************************/

    if (switchExt->vnTeVid > 0)
    {
        /* Create the teVID if it doesn't already exist. */
        status = fmCreateVlanInt(sw, switchExt->vnTeVid);
        if (status == FM_ERR_VLAN_ALREADY_EXISTS)
        {
            status = FM_OK;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        /* Make the TE VID routable. */
        status = fmSetVlanAttribute(sw, switchExt->vnTeVid, FM_VLAN_ROUTABLE, &routable);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        /* Set the encap TE PVID to the teVid. */
        status = fmSetPortAttributeInternal(sw,
                                            switchExt->tunnelCfg->tunnelPort[ENCAP_TUNNEL],
                                            FM_PORT_DEF_VLAN,
                                            &switchExt->vnTeVid);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        /* Use the default VLAN for encapsulated frames being returned from
         * the ENCAP TE port. */
        useDefaultVlan = FM_ENABLED;
        status         = fmSetPortAttributeInternal(sw,
                                                    switchExt->tunnelCfg->tunnelPort[ENCAP_TUNNEL],
                                                    FM_PORT_REPLACE_VLAN_FIELDS,
                                                    &useDefaultVlan);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Create the Encapsulation flow id bit arrays. */
    status = fmCreateBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DMAC_VID],
                              switchPtr->maxVNTunnels);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCreateBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DIP_VID],
                              switchPtr->maxVNTunnels);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCreateBitArray(&switchExt->vnTunnelActiveEncapFlowIds[FM_VN_ENCAP_GROUP_DIRECT],
                              switchPtr->maxVNTunnels);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Create tunnel rule ID bit arrays */
    for (i = 0 ; i < FM_VN_NUM_TUNNEL_GROUPS ; i++)
    {
        status = fmCreateBitArray(&switchExt->vnTunnelRuleIds[i],
                                  switchExt->maxVNRemoteAddresses);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Create Encap and Decap ACL Rule Number bit arrays. */
    status = fmCreateBitArray(&switchExt->vnEncapAclRuleNumbers,
                              switchExt->maxVNRemoteAddresses);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCreateBitArray(&switchExt->vnEncapAclFloodsetRuleNumbers,
                              switchExt->maxVNRemoteAddresses);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCreateBitArray(&switchExt->vnDecapAclRuleNumbers,
                              switchExt->maxVNRemoteAddresses);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Create the Encapsulation and Decapsulation ACLs. */
    status = fmCreateACLInt(sw,
                            switchExt->vnEncapAcl,
                            FM_ACL_SCENARIO_ANY_FRAME_TYPE
                            | FM_ACL_SCENARIO_ANY_ROUTING_TYPE
                            | FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE,
                            FM_ACL_DEFAULT_PRECEDENCE,
                            TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCreateACLInt(sw,
                            switchExt->vnDecapAcl,
                            FM_ACL_SCENARIO_IPv4
                            | FM_ACL_SCENARIO_IPv6
                            | FM_ACL_SCENARIO_UNICAST_ROUTED
                            | FM_ACL_SCENARIO_UCAST_ROUTED_GLORT,
                            FM_ACL_DEFAULT_PRECEDENCE,
                            TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Create Decap ACL Rule Tree. */
    fmCustomTreeInit(&switchExt->vnDecapAclRuleTree, CompareDecapAclRules);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end InitializeVNSubsystem */




/*****************************************************************************/
/** BuildTunnelRule
 * \ingroup intVN
 *
 * \desc            Builds a tunnel rule for encapsulation or decapsulation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \param[in]       tunnelGroup is the encap or decap tunnel group.
 *
 * \param[in]       encapFlowId is the encap flow ID to use with this rule, if
 *                  this is an encapsulation rule.
 *
 * \param[in]       addr points to the address record for the rule. If NULL,
 *                  the tunnel group must be either FM_VN_ENCAP_GROUP_DIRECT
 *                  or FM_VN_DECAP_GROUP_DIRECT.
 *
 * \param[in]       vsi is the VSI number for this tunnel rule, if the
 *                  tunnel group is a hash encap tunnel group.
 *
 * \param[out]      ruleCond points to caller-provided storage into which the
 *                  rule condition field will be written.
 *
 * \param[out]      ruleCondParam points to caller-provided storage into which
 *                  the rule condition parameters will be written.
 *
 * \param[out]      ruleAction points to caller-provided storage into which the
 *                  rule action field will be written.
 *
 * \param[out]      ruleActionParam points to caller-provided storage into
 *                  which the rule action parameters will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status BuildTunnelRule(fm_int                   sw,
                                 fm_virtualNetwork *      vn,
                                 fm_vnTunnel *            tunnel,
                                 fm_int                   tunnelGroup,
                                 fm_int                   encapFlowId,
                                 fm_vnAddress *           addr,
                                 fm_int                   vsi,
                                 fm_tunnelCondition *     ruleCond,
                                 fm_tunnelConditionParam *ruleCondParam,
                                 fm_tunnelAction *        ruleAction,
                                 fm_tunnelActionParam *   ruleActionParam)
{
    fm10000_vnTunnel *      tunnelExt;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, tunnelGroup = %d, "
                 "encapFlowId = %d, addr = %p, vsi = %d, "
                 "ruleCond = %p, ruleCondParam = %p, "
                 "ruleAction = %p, ruleActionParam = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 tunnelGroup,
                 encapFlowId,
                 (void *) addr,
                 vsi,
                 (void *) ruleCond,
                 (void *) ruleCondParam,
                 (void *) ruleAction,
                 (void *) ruleActionParam);

    switchExt = GET_SWITCH_EXT(sw);

    if (tunnel != NULL)
    {
        tunnelExt = tunnel->extension;
    }
    else
    {
        tunnelExt = NULL;
    }

    if (vn != NULL)
    {
        vnExt = vn->extension;
    }
    else if (tunnelGroup != FM_VN_DECAP_GROUP_DIRECT)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }
    else
    {
        vnExt = NULL;
    }

    if ( (addr == NULL)
         && (tunnelGroup != FM_VN_ENCAP_GROUP_DIRECT)
         && (tunnelGroup != FM_VN_DECAP_GROUP_DIRECT) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    FM_CLEAR(*ruleCondParam);
    FM_CLEAR(*ruleActionParam);

    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
            *ruleCond             = FM_TUNNEL_MATCH_DMAC | FM_TUNNEL_MATCH_VSI_TEP;
            ruleCondParam->dmac   = addr->macAddress;
            ruleCondParam->vsiTep = vsi;
            break;

        case FM_VN_ENCAP_GROUP_DIP_VID:
            *ruleCond             = FM_TUNNEL_MATCH_DIP | FM_TUNNEL_MATCH_VSI_TEP;
            ruleCondParam->dip    = addr->ipAddress;
            ruleCondParam->vsiTep = vsi;
            break;

        case FM_VN_ENCAP_GROUP_DIRECT:
        case FM_VN_DECAP_GROUP_DIRECT:
            *ruleCond   = 0;
            break;

        case FM_VN_DECAP_GROUP_DMAC_VID:
            *ruleCond           = FM_TUNNEL_MATCH_SMAC | FM_TUNNEL_MATCH_VNI;
            ruleCondParam->smac = addr->macAddress;
            ruleCondParam->vni  = vn->vsId;
            break;

        case FM_VN_DECAP_GROUP_DIP_VID:
            *ruleCond          = FM_TUNNEL_MATCH_SIP | FM_TUNNEL_MATCH_VNI;
            ruleCondParam->sip = addr->ipAddress;
            ruleCondParam->vni = vn->vsId;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    switch (tunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
        case FM_VN_ENCAP_GROUP_DIP_VID:
            *ruleAction                = FM_TUNNEL_ENCAP_FLOW;
            ruleActionParam->encapFlow = encapFlowId;
            break;

        case FM_VN_ENCAP_GROUP_DIRECT:
            *ruleAction                = FM_TUNNEL_ENCAP_FLOW
                                            | FM_TUNNEL_SET_VNI;
            ruleActionParam->encapFlow = encapFlowId;
            ruleActionParam->vni       = vn->vsId;
            break;

        case FM_VN_DECAP_GROUP_DMAC_VID:
        case FM_VN_DECAP_GROUP_DIP_VID:
        case FM_VN_DECAP_GROUP_DIRECT:
            *ruleAction             = FM_TUNNEL_SET_DGLORT;
            ruleActionParam->dglort = 0;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end BuildTunnelRule */




/*****************************************************************************/
/** BuildDecapAclRule
 * \ingroup intVN
 *
 * \desc            Builds a decap ACL rule for a tunnel and virtual network tuple.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       decapAclRule points to the decap ACL rule record.
 *
 * \param[out]      aclCond points to caller-provided storage into which the
 *                  ACL condition mask will be written.
 *
 * \param[out]      aclCondData points to caller-provided storage into which the
 *                  ACL condition data will be written.
 *
 * \param[out]      aclAction points to caller-provided storage into which the
 *                  ACL action mask will be written.
 *
 * \param[out]      aclActionData points to caller-provided storage into which
 *                  the ACL action data will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status BuildDecapAclRule(fm_int                  sw,
                                   fm10000_vnDecapAclRule *decapAclRule,
                                   fm_aclCondition *       aclCond,
                                   fm_aclValue *           aclCondData,
                                   fm_aclActionExt *       aclAction,
                                   fm_aclParamExt *        aclActionData)
{
    fm_status          status;
    fm10000_switch *   switchExt;
    fm_int             decapTunnelGroup;
    fm_virtualNetwork *vn;
    fm_vnTunnel *      tunnel;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, decapAclRule = %p, aclCond = %p, "
                 "aclCondData = %p, aclAction = %p, aclActionData = %p\n",
                 sw,
                 (void *) decapAclRule,
                 (void *) aclCond,
                 (void *) aclCondData,
                 (void *) aclAction,
                 (void *) aclActionData);

    switchExt = GET_SWITCH_EXT(sw);
    vn        = decapAclRule->vn;
    tunnel    = decapAclRule->tunnel;

    status = GetTunnelGroups(sw,
                             vn->descriptor.addressType,
                             NULL,
                             &decapTunnelGroup);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_CLEAR(*aclCondData);
    *aclCond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP;

    FM_MEMCPY_S(&aclCondData->srcIp,
                sizeof(fm_ipAddr),
                &tunnel->remoteIp,
                sizeof(fm_ipAddr) );

    FM_MEMSET_S(&aclCondData->srcIpMask,
                sizeof(fm_ipAddr),
                -1,
                sizeof(fm_ipAddr) );

    aclCondData->srcIpMask.isIPv6 = tunnel->remoteIp.isIPv6;

    FM_MEMCPY_S(&aclCondData->dstIp,
                sizeof(fm_ipAddr),
                &tunnel->localIp,
                sizeof(fm_ipAddr) );

    FM_MEMSET_S(&aclCondData->dstIpMask,
                sizeof(fm_ipAddr),
                -1,
                sizeof(fm_ipAddr) );

    aclCondData->dstIpMask.isIPv6 = tunnel->localIp.isIPv6;

    switch (tunnel->tunnelType)
    {
        case FM_VN_TUNNEL_TYPE_VXLAN_IPV4:
        case FM_VN_TUNNEL_TYPE_VXLAN_IPV6:
            *aclCond |= FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT
                | FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
            aclCondData->L4DstStart                 = switchExt->vnVxlanUdpPort;
            aclCondData->L4DstMask                  = 0xFFFF;
            aclCondData->L4DeepInspectionExt[4]     = (vn->vsId & 0xFF0000) >> 16;
            aclCondData->L4DeepInspectionExt[5]     = (vn->vsId & 0x00FF00) >> 8;
            aclCondData->L4DeepInspectionExt[6]     = vn->vsId & 0x0000FF;
            aclCondData->L4DeepInspectionExtMask[4] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[5] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[6] = 0xFF;
            break;

        case FM_VN_TUNNEL_TYPE_NVGRE:
            *aclCond |= FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK
                        | FM_ACL_MATCH_L4_DST_PORT_WITH_MASK
                        | FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT;
            aclCondData->L4SrcStart                 = 0x2000;   /* NVGRE header */
            aclCondData->L4SrcMask                  = 0xBFF8;
            aclCondData->L4DstStart                 = switchExt->vnEncapProtocol;
            aclCondData->L4DstMask                  = 0xFFFF;
            aclCondData->L4DeepInspectionExt[0]     = (vn->vsId & 0xFF0000) >> 16;
            aclCondData->L4DeepInspectionExt[1]     = (vn->vsId & 0x00FF00) >> 8;
            aclCondData->L4DeepInspectionExt[2]     = vn->vsId & 0x0000FF;
            aclCondData->L4DeepInspectionExtMask[0] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[1] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[2] = 0xFF;
            break;

        case FM_VN_TUNNEL_TYPE_GENEVE:
            *aclCond |= FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT
                | FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
            aclCondData->L4DstStart                 = switchExt->vnGeneveUdpPort;
            aclCondData->L4DstMask                  = 0xFFFF;
            aclCondData->L4DeepInspectionExt[2]     = (switchExt->vnEncapProtocol & 0xFF00) >> 8;
            aclCondData->L4DeepInspectionExt[3]     = switchExt->vnEncapProtocol & 0x00FF;
            aclCondData->L4DeepInspectionExt[4]     = (vn->vsId & 0xFF0000) >> 16;
            aclCondData->L4DeepInspectionExt[5]     = (vn->vsId & 0x00FF00) >> 8;
            aclCondData->L4DeepInspectionExt[6]     = vn->vsId & 0x0000FF;
            aclCondData->L4DeepInspectionExtMask[2] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[3] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[4] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[5] = 0xFF;
            aclCondData->L4DeepInspectionExtMask[6] = 0xFF;
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_CLEAR(*aclActionData);
    *aclAction                 = FM_ACL_ACTIONEXT_REDIRECT_TUNNEL
                                 | FM_ACL_ACTIONEXT_SET_VLAN;
    aclActionData->tunnelGroup = switchExt->vnTunnelGroups[decapTunnelGroup];
    aclActionData->tunnelRule  = 0;
    aclActionData->vlan        = vn->descriptor.vlan;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end BuildDecapAclRule */




/*****************************************************************************/
/** WriteEncapFlow
 * \ingroup intVN
 *
 * \desc            Writes an Encap Flow Record to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \param[in]       vn points to the virtual network record. If NULL,
 *                  existing encap flow records for the tunnel will be
 *                  updated.
 *
 * \param[in]       tunnelGroup is the tunnel group identifier.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TABLE_FULL if there is no room in the tunneling
 *                  hardware for the encap flow.
 *
 *****************************************************************************/
static fm_status WriteEncapFlow(fm_int             sw,
                                fm_vnTunnel *      tunnel,
                                fm_virtualNetwork *vn,
                                fm_int             tunnelGroup)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_vnTunnel *      tunnelExt;
    fm_tunnelEncapFlow      flowFields;
    fm_tunnelEncapFlowParam encapParams;
    fm_int                  flowId;
    fm_int                  firstFlowType;
    fm_int                  lastFlowType;
    fm_int                  flowType;
    fm_bool                 updateOnly;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p (tunnelId=%d, tunnelType=%d), "
                  "vn = %p, tunnelGroup = %d\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId,
                  tunnel->tunnelType,
                  (void *) vn,
                  tunnelGroup );

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tunnel->extension;

    /* We cannot proceed unless we have the minimum amount of information
     * required for tunneling. If we don't, just exit and we will try again
     * once we get more information. */
    if (!tunnelExt->haveLocalIp || !tunnelExt->haveRemoteIp)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_VN_TUNNEL_NOT_CONFIGURED);
    }

    if (switchExt->vnTunnelGroups[tunnelGroup] < 0)
    {
        status = CreateTunnelGroup(sw, tunnelGroup);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (vn == NULL)
    {
        firstFlowType = FM_VN_ENCAP_FLOW_TRANSPARENT;
        lastFlowType  = FM_VN_NUM_ENCAP_FLOW_TYPES;
        updateOnly    = TRUE;
    }
    else
    {
        firstFlowType = GET_ENCAP_FLOW_TYPE(vn);
        lastFlowType  = firstFlowType;
        updateOnly    = FALSE;
    }

    flowType = firstFlowType;

    do
    {
        flowId = tunnelExt->encapFlowIds[tunnelGroup][flowType];

        if ( (flowId < 0) && updateOnly )
        {
            continue;
        }

        /* Create or update tunnel encapsulation flow for this tunnel. */
        flowFields = 0;

        FM_CLEAR(encapParams);

        encapParams.dip = tunnel->remoteIp;

        if (flowType == FM_VN_ENCAP_FLOW_TRANSPARENT)
        {
            flowFields      |= FM_TUNNEL_ENCAP_FLOW_SIP;
            encapParams.sip  = tunnel->localIp;
        }

        if (switchExt->useSharedEncapFlows)
        {
            encapParams.shared = TRUE;
        }

        switch (tunnel->tunnelType)
        {
            case  FM_VN_TUNNEL_TYPE_VXLAN_IPV4:
            case FM_VN_TUNNEL_TYPE_VXLAN_IPV6:
                encapParams.type = FM_TUNNEL_TYPE_VXLAN;

                if (tunnel->encapTTL != (fm_uint) ~0)
                {
                    encapParams.ttl  = tunnel->encapTTL;
                    flowFields      |= FM_TUNNEL_ENCAP_FLOW_TTL;
                }

                break;

            case FM_VN_TUNNEL_TYPE_NVGRE:
                encapParams.type = FM_TUNNEL_TYPE_NVGRE;
                break;

            case FM_VN_TUNNEL_TYPE_GENEVE:
                encapParams.type = FM_TUNNEL_TYPE_NGE;

                if (tunnel->encapTTL != (fm_uint) ~0)
                {
                    encapParams.ttl  = tunnel->encapTTL;
                    flowFields      |= FM_TUNNEL_ENCAP_FLOW_TTL;
                }

                break;

            default:
                FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
        }

        if (flowId < 0)
        {
            /* Create tunnel encapsulation flow for this tunnel. */
            status = fmFindBitInBitArray(&switchExt->vnTunnelActiveEncapFlowIds[tunnelGroup],
                                         0,
                                         0,
                                         &flowId);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            tunnelExt->encapFlowIds[tunnelGroup][flowType] = flowId;

            if (flowId < 0)
            {
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, FM_ERR_TABLE_FULL);
            }

            status = fm10000AddTunnelEncapFlow(sw,
                                               switchExt->vnTunnelGroups[tunnelGroup],
                                               flowId,
                                               flowFields,
                                               &encapParams);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            status = fmSetBitArrayBit(&switchExt->vnTunnelActiveEncapFlowIds[tunnelGroup],
                                      flowId,
                                      1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
        else
        {
            /* Update the default encapsulation flow. */
            status = fm10000UpdateTunnelEncapFlow(sw,
                                                  switchExt->vnTunnelGroups[tunnelGroup],
                                                  flowId,
                                                  flowFields,
                                                  &encapParams);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }
    while (++flowType < lastFlowType);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end WriteEncapFlow */




/*****************************************************************************/
/** DeleteEncapFlow
 * \ingroup intVN
 *
 * \desc            Deletes an Encap Flow Record from the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \param[in]       tunnelGroup is the tunnel group identifier.
 *
 * \param[in]       flowType is the flow type used for this VN.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEncapFlow(fm_int       sw,
                                 fm_vnTunnel *tunnel,
                                 fm_int       tunnelGroup,
                                 fm_int       flowType)
{
    fm_status         status;
    fm10000_switch *  switchExt;
    fm10000_vnTunnel *tunnelExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnel = %p, tunnelId=%d, tunnelType=%d, "
                 "tunnelGroup=%d, flowType=%d\n",
                 sw,
                 (void *) tunnel,
                 tunnel->tunnelId,
                 tunnel->tunnelType,
                 tunnelGroup,
                 flowType);

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tunnel->extension;

    if ( (tunnelExt->ruleCounts[tunnelGroup][flowType] == 0)
         && (tunnelExt->encapFlowIds[tunnelGroup][flowType] >= 0) )
    {
        status = fm10000DeleteTunnelEncapFlow(sw,
                                              switchExt->vnTunnelGroups[tunnelGroup],
                                              tunnelExt->encapFlowIds[tunnelGroup][flowType]);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmSetBitArrayBit(&switchExt->vnTunnelActiveEncapFlowIds[tunnelGroup],
                                  tunnelExt->encapFlowIds[tunnelGroup][flowType],
                                  0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        tunnelExt->encapFlowIds[tunnelGroup][flowType] = -1;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end DeleteEncapFlow */




/*****************************************************************************/
/** UpdateVNTunnel
 * \ingroup intVN
 *
 * \desc            Updates VN Tunnel rules after a tunnel configuration change.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateVNTunnel(fm_int sw, fm_vnTunnel *tunnel)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_vnTunnel *      tunnelExt;
    fm_int                  i;
    fm_customTreeIterator   iter;
    fm10000_vnDecapAclRule *aclRuleKey;
    fm10000_vnDecapAclRule *aclRule;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p, tunnelId=%d, tunnelType=%d\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId,
                  tunnel->tunnelType );

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tunnel->extension;

    /* We cannot proceed unless we have the minimum amount of information
     * required for tunneling. If we don't, just exit and we will try again
     * once we get more information. */
    if (!tunnelExt->haveLocalIp || !tunnelExt->haveRemoteIp)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /* Update all encap flows for this tunnel. */
    for (i = 0 ; i < FM_VN_NUM_ENCAP_TUNNEL_GROUPS ; i++)
    {
        if (switchExt->vnTunnelGroups[i] >= 0)
        {
            status = WriteEncapFlow(sw,
                                    tunnel,
                                    NULL,
                                    switchExt->vnTunnelGroups[i]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }

    /* Update all decap ACL rules for this tunnel. */
    fmCustomTreeIterInit(&iter, &switchExt->vnDecapAclRuleTree);

    while ( ( status = fmCustomTreeIterNext(&iter,
                                            (void **) &aclRuleKey,
                                            (void **) &aclRule) )
            == FM_OK )
    {
        if (aclRule->tunnel == tunnel)
        {
            status = WriteDecapAclRule(sw, aclRule);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }

    if (status == FM_ERR_NO_MORE)
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end UpdateVNTunnel */




/*****************************************************************************/
/** CompareRemoteAddresses
 * \ingroup intVN
 *
 * \desc            Compare remote address records for sort-order storage
 *                  in a custom tree.
 *
 * \param[in]       first points to the first remote address record.
 *
 * \param[in]       second points to the second remote address record.
 *
 * \return          -1 if the first record sorts before the second.
 * \return           0 if the records are identical.
 * \return           1 if the first record sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareRemoteAddresses(const void *first, const void *second)
{
    fm10000_vnRemoteAddress *firstAddr;
    fm10000_vnRemoteAddress *secondAddr;
    fm_int                   comparison;

    firstAddr  = (fm10000_vnRemoteAddress *) first;
    secondAddr = (fm10000_vnRemoteAddress *) second;

    /* Compare Virtual Network VNI */
    if (firstAddr->vn->vsId < secondAddr->vn->vsId)
    {
        return -1;
    }
    else if (firstAddr->vn->vsId > secondAddr->vn->vsId)
    {
        return 1;
    }

    /* Compare Address Types */
    if (firstAddr->vn->descriptor.addressType < secondAddr->vn->descriptor.addressType)
    {
        return -1;
    }
    else if (firstAddr->vn->descriptor.addressType > secondAddr->vn->descriptor.addressType)
    {
        return 1;
    }

    /* Compare Addresses */
    switch (firstAddr->vn->descriptor.addressType)
    {
        case FM_VN_ADDR_TYPE_MAC:
            if (firstAddr->remoteAddress.macAddress < secondAddr->remoteAddress.macAddress)
            {
                return -1;
            }
            else if (firstAddr->remoteAddress.macAddress > secondAddr->remoteAddress.macAddress)
            {
                return 1;
            }

            comparison = 0;
            break;

        case FM_VN_ADDR_TYPE_IP:
            comparison = fmCompareIPAddresses(&firstAddr->remoteAddress.ipAddress,
                                              &secondAddr->remoteAddress.ipAddress);
            break;

        default:
            return -1;
    }

    return comparison;

}   /* end CompareRemoteAddresses */




/*****************************************************************************/
/** CompareTepRules
 * \ingroup intVN
 *
 * \desc            Compare encap TEP records for sort-order storage
 *                  in a custom tree.
 *
 * \param[in]       first points to the first encap TEP record.
 *
 * \param[in]       second points to the second encap TEP record.
 *
 * \return          -1 if the first record sorts before the second.
 * \return           0 if the records are identical.
 * \return           1 if the first record sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareTepRules(const void *first, const void *second)
{
    fm10000_vnEncapTep *firstRule;
    fm10000_vnEncapTep *secondRule;

    firstRule  = (fm10000_vnEncapTep *) first;
    secondRule = (fm10000_vnEncapTep *) second;

    /* Compare Virtual Network VNI */
    if (firstRule->vn->vsId < secondRule->vn->vsId)
    {
        return -1;
    }
    else if (firstRule->vn->vsId > secondRule->vn->vsId)
    {
        return 1;
    }

    /* Compare tunnel IDs */
    if (firstRule->tunnel->tunnelId < secondRule->tunnel->tunnelId)
    {
        return -1;
    }
    else if (firstRule->tunnel->tunnelId > secondRule->tunnel->tunnelId)
    {
        return 1;
    }

    return 0;

}   /* end CompareTepRules */




/*****************************************************************************/
/** CompareDecapAclRules
 * \ingroup intVN
 *
 * \desc            Compare decap ACL rules for sort-order storage in a custom tree.
 *
 * \param[in]       first points to the first rule.
 *
 * \param[in]       second points to the second rule.
 *
 * \return          -1 if the first rule sorts before the second.
 * \return           0 if the rules are identical.
 * \return           1 if the first rule sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareDecapAclRules(const void *first, const void *second)
{
    fm10000_vnDecapAclRule *firstRule;
    fm10000_vnDecapAclRule *secondRule;

    firstRule  = (fm10000_vnDecapAclRule *) first;
    secondRule = (fm10000_vnDecapAclRule *) second;

    /* Compare tunnel IDs */
    if (firstRule->tunnel->tunnelId < secondRule->tunnel->tunnelId)
    {
        return -1;
    }
    else if (firstRule->tunnel->tunnelId > secondRule->tunnel->tunnelId)
    {
        return 1;
    }

    /* Compare Virtual Network VNI */
    if (firstRule->vn->vsId < secondRule->vn->vsId)
    {
        return -1;
    }
    else if (firstRule->vn->vsId > secondRule->vn->vsId)
    {
        return 1;
    }

    return 0;

}   /* end CompareDecapAclRules */




/*****************************************************************************/
/** CompareAddressMasks
 * \ingroup intVN
 *
 * \desc            Compare address masks for sort-order storage in a custom tree.
 *
 * \param[in]       first points to the first address mask.
 *
 * \param[in]       second points to the second address mask.
 *
 * \return          -1 if the first address mask sorts before the second.
 * \return           0 if the address masks are identical.
 * \return           1 if the first address mask sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareAddressMasks(const void *first, const void *second)
{
    fm10000_vnRemoteAddressMask *firstAddr;
    fm10000_vnRemoteAddressMask *secondAddr;
    fm_int comparison;

    firstAddr  = (fm10000_vnRemoteAddressMask *) first;
    secondAddr = (fm10000_vnRemoteAddressMask *) second;

    /* Compare Address Types */
    if (firstAddr->vn->descriptor.addressType < secondAddr->vn->descriptor.addressType)
    {
        return -1;
    }
    else if (firstAddr->vn->descriptor.addressType > secondAddr->vn->descriptor.addressType)
    {
        return 1;
    }

    /* Compare Addresses */
    switch (firstAddr->vn->descriptor.addressType)
    {
        case FM_VN_ADDR_TYPE_MAC:
            if (firstAddr->remoteAddress.macAddress < secondAddr->remoteAddress.macAddress)
            {
                return -1;
            }
            else if (firstAddr->remoteAddress.macAddress > secondAddr->remoteAddress.macAddress)
            {
                return 1;
            }

            comparison = 0;
            break;

        case FM_VN_ADDR_TYPE_IP:
            comparison = fmCompareIPAddresses(&firstAddr->remoteAddress.ipAddress,
                                              &secondAddr->remoteAddress.ipAddress);
            break;

        default:
            return -1;
    }

    if (comparison != 0)
    {
        return comparison;
    }

    return 0;

}   /* end CompareAddressMasks */




/*****************************************************************************/
/** CheckAddrMasksForRemoteAddr
 * \ingroup intVN
 *
 * \desc            Checks the address masks for a virtual network to see if a
 *                  remote address mask was created for the target remote
 *                  address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       addr points to the address to be checked.
 *
 * \param[out]      addrMaskPtr points to caller-provided storage where the
 *                  address mask record will be written, if addrMaskPtr is not
 *                  NULL.
 *
 * \return          TRUE if an address match is found, FALSE if not.
 *
 *****************************************************************************/
static fm_bool CheckAddrMasksForRemoteAddr(fm_int                        sw,
                                           fm_virtualNetwork *           vn,
                                           fm_vnAddress *                addr,
                                           fm10000_vnRemoteAddressMask **addrMaskPtr)
{
    fm_status                    status;
    fm10000_virtualNetwork *     vnExt;
    fm_customTreeIterator        iter;
    fm10000_vnRemoteAddressMask *addrMaskKey;
    fm10000_vnRemoteAddressMask *addrMaskRule;
    fm_bool                      foundMatch;
    fm_macaddr                   macAddrMask;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, addr = %p\n",
                 sw,
                 (void *) vn,
                 (void *) addr);

    FM_NOT_USED(sw);

    vnExt      = vn->extension;
    foundMatch = FALSE;

    fmCustomTreeIterInit(&iter, &vnExt->addressMasks);

    while ( (status = fmCustomTreeIterNext(&iter,
                                           (void **) &addrMaskKey,
                                           (void **) &addrMaskRule) )
            == FM_OK)
    {
        if (addrMaskRule->vn == vn)
        {
            switch (vn->descriptor.addressType)
            {
                case FM_VN_ADDR_TYPE_MAC:
                    macAddrMask = addrMaskRule->addrMask.macAddress;
                    if ( (addrMaskRule->remoteAddress.macAddress & macAddrMask)
                         == (addr->macAddress & macAddrMask) )
                    {
                        foundMatch = TRUE;
                    }
                    break;

                case FM_VN_ADDR_TYPE_IP:
                    if ( fmIsIPAddressInRangeMask(&addrMaskRule->remoteAddress.ipAddress,
                                                  &addrMaskRule->addrMask.ipAddress,
                                                  &addr->ipAddress) )
                    {
                        foundMatch = TRUE;
                    }
                    break;

                default:
                    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VN, FALSE, "unsupported address type.\n");
                    break;
            }

            if (foundMatch)
            {
                if (addrMaskPtr != NULL)
                {
                    *addrMaskPtr = addrMaskRule;
                }

                FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VN, TRUE, "address match found.\n");
            }
        }
    }

    if (addrMaskPtr != NULL)
    {
        *addrMaskPtr = NULL;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_VN, FALSE, "address match not found.\n");

}   /* end CheckAddrMasksForRemoteAddr */




/*****************************************************************************/
/** WriteVsiEncapTunnelRule
 * \ingroup intVN
 *
 * \desc            Writes an Encapsulation Tunnel Rule for a specified VSI.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec points to the tunnel rule record.
 *
 * \param[in]       vsi is the vsi value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteVsiEncapTunnelRule(fm_int                   sw,
                                         fm10000_vnRemoteAddress *addrRec,
                                         fm_int                   vsi)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_vnTunnel *      tunnelExt;
    fm_int                  encapRule;
    fm_tunnelCondition      ruleCond;
    fm_tunnelConditionParam ruleCondParam;
    fm_tunnelAction         ruleAction;
    fm_tunnelActionParam    ruleActionParam;
    fm_int                  flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p, vsi = %d\n",
                 sw,
                 (void *) addrRec,
                 vsi);

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = addrRec->tunnel->extension;
    flowType  = GET_ENCAP_FLOW_TYPE(addrRec->vn);
    encapRule = -1;

    if (addrRec->encapTunnelRules[vsi] < 0)
    {
        /* Get a tunneling engine encapsulation rule number. */
        status = AllocateTunnelRuleNum(sw,
                                       addrRec->encapTunnelGroup,
                                       &encapRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        addrRec->encapTunnelRules[vsi] = encapRule;
    }

    /* Build the tunneling engine encapsulation rule. */
    status = BuildTunnelRule(sw,
                             addrRec->vn,
                             addrRec->tunnel,
                             addrRec->encapTunnelGroup,
                             tunnelExt->encapFlowIds[addrRec->encapTunnelGroup][flowType],
                             &addrRec->remoteAddress,
                             vsi,
                             &ruleCond,
                             &ruleCondParam,
                             &ruleAction,
                             &ruleActionParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (encapRule >= 0)
    {
        /* Write the encapsulation rule. */
        status = fm10000AddTunnelRule(sw,
                                      switchExt->vnTunnelGroups[addrRec->encapTunnelGroup],
                                      addrRec->encapTunnelRules[vsi],
                                      ruleCond,
                                      &ruleCondParam,
                                      ruleAction,
                                      &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        /* Update the encapsulation rule. */
        status = fm10000UpdateTunnelRule(sw,
                                         switchExt->vnTunnelGroups[addrRec->encapTunnelGroup],
                                         addrRec->encapTunnelRules[vsi],
                                         ruleCond,
                                         &ruleCondParam,
                                         ruleAction,
                                         &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (encapRule >= 0)
        {
            FreeTunnelRuleNum(sw,
                              addrRec->encapTunnelGroup,
                              addrRec->encapTunnelRules[vsi]);

            addrRec->encapTunnelRules[vsi] = -1;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteVsiEncapTunnelRule */




/*****************************************************************************/
/** WriteEncapTepRule
 * \ingroup intVN
 *
 * \desc            Writes an Encapsulation TEP Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tepRule points to the tunnel end-point rule record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteEncapTepRule(fm_int              sw,
                                   fm10000_vnEncapTep *tepRule)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_vnTunnel *      tunnelExt;
    fm_int                  encapRule;
    fm_tunnelCondition      ruleCond;
    fm_tunnelConditionParam ruleCondParam;
    fm_tunnelAction         ruleAction;
    fm_tunnelActionParam    ruleActionParam;
    fm_int                  flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tepRule = %p\n",
                 sw,
                 (void *) tepRule);

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tepRule->tunnel->extension;
    flowType  = GET_ENCAP_FLOW_TYPE(tepRule->vn);
    encapRule = -1;

    if (tepRule->encapTunnelRule < 0)
    {
        /* Get a tunneling engine encapsulation rule number. */
        status = AllocateTunnelRuleNum(sw,
                                       FM_VN_ENCAP_GROUP_DIRECT,
                                       &encapRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        tepRule->encapTunnelRule = encapRule;
    }

    /* Build the tunneling engine encapsulation rule. */
    status = BuildTunnelRule(sw,
                             tepRule->vn,
                             tepRule->tunnel,
                             FM_VN_ENCAP_GROUP_DIRECT,
                             tunnelExt->encapFlowIds[FM_VN_ENCAP_GROUP_DIRECT][flowType],
                             NULL,
                             0,
                             &ruleCond,
                             &ruleCondParam,
                             &ruleAction,
                             &ruleActionParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (encapRule >= 0)
    {
        /* Write the encapsulation rule. */
        status = fm10000AddTunnelRule(sw,
                                      switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                      tepRule->encapTunnelRule,
                                      ruleCond,
                                      &ruleCondParam,
                                      ruleAction,
                                      &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        /* Update the encapsulation rule. */
        status = fm10000UpdateTunnelRule(sw,
                                         switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                         tepRule->encapTunnelRule,
                                         ruleCond,
                                         &ruleCondParam,
                                         ruleAction,
                                         &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (encapRule >= 0)
        {
            FreeTunnelRuleNum(sw,
                              FM_VN_ENCAP_GROUP_DIRECT,
                              tepRule->encapTunnelRule);

            tepRule->encapTunnelRule = -1;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteEncapTepRule */




/*****************************************************************************/
/** WriteEncapTunnelRule
 * \ingroup intVN
 *
 * \desc            Writes an Encapsulation Tunnel Rule for a remote address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec points to the remote address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteEncapTunnelRule(fm_int                   sw,
                                      fm10000_vnRemoteAddress *addrRec)
{
    fm_status         status;
    fm10000_switch *  switchExt;
    fm10000_vnTunnel *tunnelExt;
    fm_int            vsi;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = addrRec->tunnel->extension;
    status    = FM_OK;

    if ( (addrRec->addrMask == NULL)
         || (addrRec->addrMask->tunnel != NULL) )
    {
        status = FM_FAIL;
        FM_LOG_EXIT(FM_LOG_CAT_VN, status);
    }

    if (addrRec->vn->descriptor.mode == FM_VN_MODE_TRANSPARENT)
    {
        status = WriteVsiEncapTunnelRule(sw, addrRec, 0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    else
    {
        for (vsi = 0; vsi < FM10000_TE_VNI_ENTRIES_0; vsi++)
        {
            if (switchExt->vnVsi[vsi] == addrRec->vn)
            {
                status = WriteVsiEncapTunnelRule(sw, addrRec, vsi);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteEncapTunnelRule */




/*****************************************************************************/
/** DeleteEncapTunnelRule
 * \ingroup intVN
 *
 * \desc            Deletes an Encapsulation Tunnel Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec points to the address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEncapTunnelRule(fm_int                   sw,
                                       fm10000_vnRemoteAddress *addrRec)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_int          vsi;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    switchExt = GET_SWITCH_EXT(sw);

    if ( (addrRec->addrMask == NULL)
         || (addrRec->addrMask->tunnel != NULL) )
    {
        status = FM_FAIL;
        FM_LOG_EXIT(FM_LOG_CAT_VN, status);
    }

    for (vsi = 0; vsi < FM10000_TE_VNI_ENTRIES_0; vsi++)
    {
        if (addrRec->encapTunnelRules[vsi] >= 0)
        {
            /* Delete the encapsulation rule. */
            status = fm10000DeleteTunnelRule(sw,
                                              switchExt->vnTunnelGroups[addrRec->encapTunnelGroup],
                                              addrRec->encapTunnelRules[vsi]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            FreeTunnelRuleNum(sw,
                              addrRec->encapTunnelGroup,
                              addrRec->encapTunnelRules[vsi]);

            addrRec->encapTunnelRules[vsi] = -1;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end DeleteEncapTunnelRule */




/*****************************************************************************/
/** WriteDecapTunnelRule
 * \ingroup intVN
 *
 * \desc            Writes a Decapsulation Tunnel Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec points to the remote address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteDecapTunnelRule(fm_int                   sw,
                                      fm10000_vnRemoteAddress *addrRec)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_int                  decapRule;
    fm_tunnelCondition      ruleCond;
    fm_tunnelConditionParam ruleCondParam;
    fm_tunnelAction         ruleAction;
    fm_tunnelActionParam    ruleActionParam;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    decapRule = -1;
    switchExt = GET_SWITCH_EXT(sw);

    if (addrRec->decapTunnelRule < 0)
    {
        /* Get a tunneling engine encapsulation rule number. */
        status = AllocateTunnelRuleNum(sw,
                                       addrRec->decapTunnelGroup,
                                       &decapRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        addrRec->decapTunnelRule = decapRule;
    }

    /* Build the tunneling engine decapsulation rule. */
    status = BuildTunnelRule(sw,
                             addrRec->vn,
                             addrRec->tunnel,
                             addrRec->decapTunnelGroup,
                             0,
                             &addrRec->remoteAddress,
                             0,
                             &ruleCond,
                             &ruleCondParam,
                             &ruleAction,
                             &ruleActionParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (decapRule >= 0)
    {
        /* Write the decapsulation rule. */
        status = fm10000AddTunnelRule(sw,
                                      switchExt->vnTunnelGroups[addrRec->decapTunnelGroup],
                                      addrRec->decapTunnelRule,
                                      ruleCond,
                                      &ruleCondParam,
                                      ruleAction,
                                      &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        /* Update the decapsulation rule. */
        status = fm10000UpdateTunnelRule(sw,
                                         switchExt->vnTunnelGroups[addrRec->decapTunnelGroup],
                                         addrRec->decapTunnelRule,
                                         ruleCond,
                                         &ruleCondParam,
                                         ruleAction,
                                         &ruleActionParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (decapRule >= 0)
        {
            FreeTunnelRuleNum(sw,
                              addrRec->decapTunnelGroup,
                              addrRec->decapTunnelRule);

            addrRec->decapTunnelRule = -1;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteDecapTunnelRule */




/*****************************************************************************/
/** DeleteDecapTunnelRule
 * \ingroup intVN
 *
 * \desc            Deletes a Decapsulation Tunnel Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec points to the remote address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteDecapTunnelRule(fm_int                   sw,
                                       fm10000_vnRemoteAddress *addrRec)
{
    fm_status       status;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    switchExt = GET_SWITCH_EXT(sw);

    if (addrRec->decapTunnelRule < 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /* Delete the decapsulation rule. */
    status = fm10000DeleteTunnelRule(sw,
                                      switchExt->vnTunnelGroups[addrRec->decapTunnelGroup],
                                      addrRec->decapTunnelRule);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FreeTunnelRuleNum(sw,
                      addrRec->decapTunnelGroup,
                      addrRec->decapTunnelRule);

    addrRec->decapTunnelRule = -1;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end DeleteDecapTunnelRule */




/*****************************************************************************/
/** WriteEncapAclRule
 * \ingroup intVN
 *
 * \desc            Writes an encapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec is the remote address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteEncapAclRule(fm_int                   sw,
                                   fm10000_vnRemoteAddress *addrRec)
{
    fm_status          status;
    fm10000_switch *   switchExt;
    fm_int             aclRule;
    fm_aclCondition    aclCond;
    fm_aclValue        aclValue;
    fm_aclActionExt    aclAction;
    fm_aclParamExt     aclParam;
    fm_bool            encapAclRuleAdded;
    fm_vnAddressType   addrType;
    fm_char            statusText[STATUS_TEXT_LEN];

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    switchExt         = GET_SWITCH_EXT(sw);
    addrType          = addrRec->vn->descriptor.addressType;
    encapAclRuleAdded = FALSE;
    aclRule           = -1;
    status            = FM_OK;

    FM_CLEAR(aclValue);
    FM_CLEAR(aclParam);

    aclCond = 0;

    if (addrRec->vn->descriptor.vlan != 0)
    {
        aclCond            |= FM_ACL_MATCH_VLAN;
        aclValue.vlanId     = addrRec->vn->descriptor.vlan;
        aclValue.vlanIdMask = 0x0fff;
    }

    aclAction            = FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
    aclParam.tunnelGroup = switchExt->vnTunnelGroups[addrRec->encapTunnelGroup];

    switch (addrRec->encapTunnelGroup)
    {
        case FM_VN_ENCAP_GROUP_DMAC_VID:
        case FM_VN_ENCAP_GROUP_DIP_VID:
            aclParam.tunnelRule = 0;
            break;

        case FM_VN_ENCAP_GROUP_DIRECT:
            aclParam.tunnelRule  = addrRec->encapTep->encapTunnelRule;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    if (addrType == FM_VN_ADDR_TYPE_MAC)
    {
        /* Fill in the MAC-based ACL rule. */
        aclCond          |= FM_ACL_MATCH_DST_MAC;
        aclValue.dst      = addrRec->remoteAddress.macAddress;
        aclValue.dstMask  = 0xFFFFFFFFFFFFLL;
    }
    else if (addrType == FM_VN_ADDR_TYPE_IP)
    {
        /* Fill in the IP-based ACL rule. */
        aclCond |= FM_ACL_MATCH_DST_IP;
        FM_MEMCPY_S(&aclValue.dstIp,
                    sizeof(fm_ipAddr),
                    &addrRec->remoteAddress.ipAddress,
                    sizeof(fm_ipAddr) );
        FM_MEMSET_S(&aclValue.dstIpMask,
                    sizeof(fm_ipAddr),
                    -1,
                    sizeof(fm_ipAddr) );
        aclValue.dstIpMask.isIPv6 = addrRec->remoteAddress.ipAddress.isIPv6;
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addrRec->encapAclRule < 0)
    {
        /* This IP address isn't covered, get an ACL rule. */
        status = AllocateTunnelAclRuleNum(sw,
                                          addrRec->encapTunnelGroup,
                                          ACL_RULE_TYPE_NORMAL,
                                          &aclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        addrRec->encapAclRule = aclRule;
    }

    if (aclRule >= 0)
    {
        status = fmAddACLRuleExt(sw,
                              switchExt->vnEncapAcl,
                              addrRec->encapAclRule,
                              aclCond,
                              &aclValue,
                              aclAction,
                              &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        encapAclRuleAdded = TRUE;

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                                 | FM_ACL_COMPILE_FLAG_INTERNAL,
                                 &switchExt->vnEncapAcl);
        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                               | FM_ACL_APPLY_FLAG_INTERNAL,
                               &switchExt->vnEncapAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        status = fmUpdateACLRule(sw,
                                 switchExt->vnEncapAcl,
                                 addrRec->encapAclRule,
                                 aclCond,
                                 &aclValue,
                                 aclAction,
                                 &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (encapAclRuleAdded)
        {
            fmDeleteACLRule(sw, switchExt->vnEncapAcl, addrRec->encapAclRule);
        }

        if (aclRule >= 0)
        {
            FreeTunnelAclRuleNum(sw,
                                 addrRec->encapTunnelGroup,
                                 FALSE,
                                 addrRec->encapAclRule);

            addrRec->encapAclRule = -1;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteEncapAclRule */




/*****************************************************************************/
/** DeleteEncapAclRule
 * \ingroup intVN
 *
 * \desc            Deletes an encapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addrRec is the remote address record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEncapAclRule(fm_int                   sw,
                                    fm10000_vnRemoteAddress *addrRec)
{
    fm_status          status;
    fm10000_switch *   switchExt;
    fm_char            statusText[STATUS_TEXT_LEN];

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, addrRec = %p\n",
                 sw,
                 (void *) addrRec);

    switchExt = GET_SWITCH_EXT(sw);
    status    = FM_OK;

    if (addrRec->encapAclRule >= 0)
    {
        status = fmDeleteACLRule(sw,
                                 switchExt->vnEncapAcl,
                                 addrRec->encapAclRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                                 | FM_ACL_COMPILE_FLAG_INTERNAL,
                                 &switchExt->vnEncapAcl);
        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                               | FM_ACL_APPLY_FLAG_INTERNAL,
                               &switchExt->vnEncapAcl);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = FreeTunnelAclRuleNum(sw,
                                      addrRec->encapTunnelGroup,
                                      ACL_RULE_TYPE_NORMAL,
                                      addrRec->encapAclRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        addrRec->encapAclRule = -1;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end DeleteEncapAclRule */




/*****************************************************************************/
/** WriteDecapAclRule
 * \ingroup intVN
 *
 * \desc            Writes a decapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       decapAclRule points to the decap ACL rule record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteDecapAclRule(fm_int                  sw,
                                   fm10000_vnDecapAclRule *decapAclRule)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_aclCondition aclCond;
    fm_aclValue     aclValue;
    fm_aclActionExt aclAction;
    fm_aclParamExt  aclParam;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, decapAclRule = %p\n",
                 sw,
                 (void *) decapAclRule);

    switchExt = GET_SWITCH_EXT(sw);

    status = BuildDecapAclRule(sw,
                               decapAclRule,
                               &aclCond,
                               &aclValue,
                               &aclAction,
                               &aclParam);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmUpdateACLRule(sw,
                             switchExt->vnDecapAcl,
                             decapAclRule->aclRule,
                             aclCond,
                             &aclValue,
                             aclAction,
                             &aclParam);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end WriteDecapAclRule */




/*****************************************************************************/
/** CreateDecapAclRule
 * \ingroup intVN
 *
 * \desc            Creates a decapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \param[in]       decapTunnelGroup is the decap tunnel group ID.
 *
 * \param[out]      decapAclRulePtr points to caller-provided storage into
 *                  which the decap ACL Rule record pointer will be written,
 *                  if decapAclRulePtr is not NULL.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateDecapAclRule(fm_int                   sw,
                                    fm_virtualNetwork *      vn,
                                    fm_vnTunnel *            tunnel,
                                    fm_int                   decapTunnelGroup,
                                    fm10000_vnDecapAclRule **decapAclRulePtr)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_int                  aclRule;
    fm_aclCondition         aclCond;
    fm_aclValue             aclValue;
    fm_aclActionExt         aclAction;
    fm_aclParamExt          aclParam;
    fm10000_vnDecapAclRule *decapAclRule;
    fm_bool                 decapAclRuleAdded;
    fm_bool                 decapAclRuleAllocated;
    fm_bool                 decapAclRuleInserted;
    fm_char                 statusText[STATUS_TEXT_LEN];

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, decapTunnelGroup = %d, "
                 "decapAclRulePtr = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 decapTunnelGroup,
                 (void *) decapAclRulePtr);

    switchExt             = GET_SWITCH_EXT(sw);
    decapAclRuleAdded     = FALSE;
    decapAclRuleAllocated = FALSE;
    decapAclRuleInserted  = FALSE;
    aclRule               = -1;

    if (switchExt->vnTunnelGroups[decapTunnelGroup] < 0)
    {
        status = CreateTunnelGroup(sw, decapTunnelGroup);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = GetDecapAclRule(sw, vn, tunnel, &decapAclRule);

    if (status == FM_ERR_NOT_FOUND)
    {
        decapAclRule = fmAlloc(sizeof(fm10000_vnDecapAclRule));

        if (decapAclRule == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
        }

        decapAclRuleAllocated = TRUE;

        FM_CLEAR(*decapAclRule);

        decapAclRule->vn               = vn;
        decapAclRule->tunnel           = tunnel;
        decapAclRule->decapTunnelGroup = decapTunnelGroup;
        decapAclRule->aclRule          = -1;
        decapAclRule->useCount         = 0;

        status = AllocateTunnelAclRuleNum(sw,
                                          decapTunnelGroup,
                                          ACL_RULE_TYPE_NORMAL,
                                          &aclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        decapAclRule->aclRule = aclRule;

        status = fmCustomTreeInsert(&switchExt->vnDecapAclRuleTree,
                                  decapAclRule,
                                  decapAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        decapAclRuleInserted = TRUE;

        status = BuildDecapAclRule(sw,
                                   decapAclRule,
                                   &aclCond,
                                   &aclValue,
                                   &aclAction,
                                   &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmAddACLRuleExt(sw,
                                 switchExt->vnDecapAcl,
                                 decapAclRule->aclRule,
                                 aclCond,
                                 &aclValue,
                                 aclAction,
                                 &aclParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        decapAclRuleAdded = TRUE;

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                                 | FM_ACL_COMPILE_FLAG_INTERNAL,
                                 &switchExt->vnDecapAcl);
        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                               | FM_ACL_APPLY_FLAG_INTERNAL,
                               &switchExt->vnDecapAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (decapAclRule == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }

    ++decapAclRule->useCount;

    if (decapAclRulePtr != NULL)
    {
        *decapAclRulePtr = decapAclRule;
    }

ABORT:

    if (status != FM_OK)
    {
        if (decapAclRuleInserted)
        {
            fmCustomTreeRemove(&switchExt->vnDecapAclRuleTree, decapAclRule, NULL);
        }

        if (decapAclRuleAdded)
        {
            fmDeleteACLRule(sw, switchExt->vnDecapAcl, decapAclRule->aclRule);
        }

        if (aclRule >= 0)
        {
            FreeTunnelAclRuleNum(sw,
                                 decapTunnelGroup,
                                 ACL_RULE_TYPE_NORMAL,
                                 decapAclRule->aclRule);
        }

        if (decapAclRuleAllocated)
        {
            fmFree(decapAclRule);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end CreateDecapAclRule */




/*****************************************************************************/
/** WriteEncapFloodsetAclRule
 * \ingroup intVN
 *
 * \desc            Writes an encapsulation ACL Floodset Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteEncapFloodsetAclRule(fm_int             sw,
                                           fm_virtualNetwork *vn)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;
    fm_int                  aclRule;
    fm_aclCondition         aclCond;
    fm_aclValue             aclValue;
    fm_aclActionExt         aclAction;
    fm_aclParamExt          aclParam;
    fm_bool                 encapAclRuleAdded;
    fm_char                 statusText[STATUS_TEXT_LEN];
    fm_intMulticastGroup *  mcastGroup;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p\n",
                 sw,
                 (void *) vn);

    switchExt         = GET_SWITCH_EXT(sw);
    vnExt             = vn->extension;
    encapAclRuleAdded = FALSE;
    aclRule           = 0;
    status            = FM_OK;

    if (vnExt->floodsetEncapAclRule > 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    mcastGroup = fmFindMcastGroup(sw, vnExt->floodsetMcastGroup);

    if (mcastGroup == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    FM_CLEAR(aclValue);
    FM_CLEAR(aclParam);

    aclCond              = FM_ACL_MATCH_VLAN;
    aclValue.vlanId      = vn->descriptor.vlan;
    aclValue.vlanIdMask  = 0x0fff;
    aclAction            = FM_ACL_ACTIONEXT_SET_FLOOD_DEST
                            | FM_ACL_ACTIONEXT_SET_PRECEDENCE;
    aclParam.logicalPort = mcastGroup->logicalPort;
    aclParam.precedence  = 0;

    status = AllocateTunnelAclRuleNum(sw,
                                      FM_VN_ENCAP_GROUP_DIRECT,
                                      ACL_RULE_TYPE_FLOODSET,
                                      &vnExt->floodsetEncapAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmAddACLRuleExt(sw,
                          switchExt->vnEncapAcl,
                          vnExt->floodsetEncapAclRule,
                          aclCond,
                          &aclValue,
                          aclAction,
                          &aclParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    encapAclRuleAdded = TRUE;

    status = fmCompileACLExt(sw,
                             statusText,
                             sizeof(statusText),
                             FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                             | FM_ACL_COMPILE_FLAG_INTERNAL,
                             &switchExt->vnEncapAcl);
    FM_LOG_DEBUG(FM_LOG_CAT_VN,
                 "ACL compiled, status=%d, statusText=%s\n",
                 status,
                 statusText);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmApplyACLExt(sw,
                           FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                           | FM_ACL_APPLY_FLAG_INTERNAL,
                           &switchExt->vnEncapAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (status != FM_OK)
    {
        if (encapAclRuleAdded)
        {
            fmDeleteACLRule(sw, switchExt->vnEncapAcl, vnExt->floodsetEncapAclRule);
        }

        if (vnExt->floodsetEncapAclRule > 0)
        {
            FreeTunnelAclRuleNum(sw,
                                 FM_VN_ENCAP_GROUP_DIRECT,
                                 ACL_RULE_TYPE_FLOODSET,
                                 vnExt->floodsetEncapAclRule);

            vnExt->floodsetEncapAclRule = 0;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end WriteEncapFloodsetAclRule */




/*****************************************************************************/
/** DeleteEncapFloodsetAclRule
 * \ingroup intVN
 *
 * \desc            Deletes an encapsulation floodset ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEncapFloodsetAclRule(fm_int             sw,
                                            fm_virtualNetwork *vn)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm10000_switch *        switchExt;
    fm_char                 statusText[STATUS_TEXT_LEN];

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p\n",
                 sw,
                 (void *) vn);

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;
    status    = FM_OK;

    if (vnExt->floodsetEncapAclRule > 0)
    {
        status = fmDeleteACLRule(sw,
                                 switchExt->vnEncapAcl,
                                 vnExt->floodsetEncapAclRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                                 | FM_ACL_COMPILE_FLAG_INTERNAL,
                                 &switchExt->vnEncapAcl);
        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                               | FM_ACL_APPLY_FLAG_INTERNAL,
                               &switchExt->vnEncapAcl);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = FreeTunnelAclRuleNum(sw,
                                      FM_VN_ENCAP_GROUP_DIRECT,
                                      ACL_RULE_TYPE_FLOODSET,
                                      vnExt->floodsetEncapAclRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        vnExt->floodsetEncapAclRule = 0;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end DeleteEncapFloodsetAclRule */




/*****************************************************************************/
/** GetDecapAclRule
 * \ingroup intVN
 *
 * \desc            Gets a decapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \param[in]       tunnel points to the tunnel record.
 *
 * \param[out]      decapAclRulePtr points to caller-provided storage
 *                  into which the decap acl rule record pointer will be
 *                  written if decapAclRulePtr is not NULL. Use NULL if the
 *                  function is called only to determine if the rule exists.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetDecapAclRule(fm_int                   sw,
                                 fm_virtualNetwork *      vn,
                                 fm_vnTunnel *            tunnel,
                                 fm10000_vnDecapAclRule **decapAclRulePtr)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_vnDecapAclRule *decapAclRule;
    fm10000_vnDecapAclRule  decapAclKey;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, decapAclRulePtr = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 (void *) decapAclRulePtr);

    switchExt = GET_SWITCH_EXT(sw);

    FM_CLEAR(decapAclKey);

    decapAclKey.vn     = vn;
    decapAclKey.tunnel = tunnel;

    status = fmCustomTreeFind(&switchExt->vnDecapAclRuleTree,
                              (void *) &decapAclKey,
                              (void **) &decapAclRule);

    if (decapAclRulePtr != NULL)
    {
        if (status == FM_OK)
        {
            *decapAclRulePtr = decapAclRule;
        }
        else
        {
            *decapAclRulePtr = NULL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end GetDecapAclRule */




/*****************************************************************************/
/** DeleteDecapAclRule
 * \ingroup intVN
 *
 * \desc            Deletes a decapsulation ACL Rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       decapAclRule points to the decap ACL rule which should be
 *                  deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteDecapAclRule(fm_int                  sw,
                                    fm10000_vnDecapAclRule *decapAclRule)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_char         statusText[STATUS_TEXT_LEN];

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, decapAclRule = %p\n",
                 sw,
                 (void *) decapAclRule);

    if (--decapAclRule->useCount > 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    switchExt = GET_SWITCH_EXT(sw);

    status = fmDeleteACLRule(sw,
                             switchExt->vnDecapAcl,
                             decapAclRule->aclRule);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCompileACLExt(sw,
                             statusText,
                             sizeof(statusText),
                             FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                             | FM_ACL_COMPILE_FLAG_INTERNAL,
                             &switchExt->vnDecapAcl);
    FM_LOG_DEBUG(FM_LOG_CAT_VN,
                 "ACL compiled, status=%d, statusText=%s\n",
                 status,
                 statusText);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmApplyACLExt(sw,
                           FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                           | FM_ACL_APPLY_FLAG_INTERNAL,
                           &switchExt->vnDecapAcl);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = FreeTunnelAclRuleNum(sw,
                                  decapAclRule->decapTunnelGroup,
                                  ACL_RULE_TYPE_NORMAL,
                                  decapAclRule->aclRule);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    decapAclRule->aclRule = -1;

    status = fmCustomTreeRemove(&switchExt->vnDecapAclRuleTree, decapAclRule, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    fmFree(decapAclRule);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end DeleteDecapAclRule */




/*****************************************************************************/
/** AddTunnelRuleToVN
 * \ingroup intVN
 *
 * \desc            Adds a remote address tunnel rule to a virtual network. This
 *                  creates a tunnel rule use count record if needed and sets the
 *                  use count to 1. Otherwise, it increments the use count.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       tunnel is the tunnel record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddTunnelRuleToVN(fm_int             sw,
                                   fm_virtualNetwork *vn,
                                   fm_vnTunnel *      tunnel)
{
    fm_status                 status;
    fm10000_switch *          switchExt;
    fm10000_virtualNetwork *  vnExt;
    fm_mcastGroupListener     listener;
    fm10000_vnTunnelUseCount *tunnelUseCount;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel);

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    status = fmTreeFind(&vnExt->tunnels,
                        (fm_uint64) tunnel->tunnelId,
                        (void **) &tunnelUseCount);

    if (status == FM_ERR_NOT_FOUND)
    {
        FM_CLEAR(listener);

        listener.listenerType             = FM_MCAST_GROUP_LISTENER_VN_TUNNEL;
        listener.info.vnListener.tunnelId = tunnel->tunnelId;
        listener.info.vnListener.vni      = vn->vsId;

        status = fmAddMcastGroupListenerInternal(sw,
                                                 vnExt->floodsetMcastGroup,
                                                 &listener);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        tunnelUseCount = fmAlloc( sizeof(fm10000_vnTunnelUseCount) );

        if (tunnelUseCount == NULL)
        {
            fmDeleteMcastGroupListenerInternal(sw,
                                               vnExt->floodsetMcastGroup,
                                               &listener);
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MEM);
        }

        tunnelUseCount->tunnel   = tunnel;
        tunnelUseCount->useCount = 1;

        status = fmTreeInsert(&vnExt->tunnels,
                              tunnel->tunnelId,
                              tunnelUseCount);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else if (status == FM_OK)
    {
        ++tunnelUseCount->useCount;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end AddTunnelRuleToVN */




/*****************************************************************************/
/** DeleteTunnelRuleFromVN
 * \ingroup intVN
 *
 * \desc            Deletes a remote address tunnel rule from a virtual network.
 *                  This decrements the tunnel rule use count. If the use count
 *                  goes to zero, the tunnel rule is deleted from the hardware
 *                  and the tunnel rule use count record is deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       tunnel is the tunnel record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteTunnelRuleFromVN(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_vnTunnel *      tunnel)
{
    fm_status                 status;
    fm10000_switch *          switchExt;
    fm10000_virtualNetwork *  vnExt;
    fm_mcastGroupListener     listener;
    fm10000_vnTunnelUseCount *tunnelUseCount;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel);

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    status = fmTreeFind(&vnExt->tunnels,
                        (fm_uint64) tunnel->tunnelId,
                        (void **) &tunnelUseCount);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (--tunnelUseCount->useCount > 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    FM_CLEAR(listener);

    listener.listenerType             = FM_MCAST_GROUP_LISTENER_VN_TUNNEL;
    listener.info.vnListener.tunnelId = tunnel->tunnelId;
    listener.info.vnListener.vni      = vn->vsId;

    status = fmDeleteMcastGroupListenerInternal(sw,
                                                vnExt->floodsetMcastGroup,
                                                &listener);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmTreeRemove(&vnExt->tunnels, tunnel->tunnelId, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    fmFree(tunnelUseCount);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end DeleteTunnelRuleFromVN */




/*****************************************************************************/
/** GetEncapTepRule
 * \ingroup intVN
 *
 * \desc            Searches for a encapsulation tunnel end-point rule record.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the Virtual Network record.
 *
 * \param[in]       tunnel points to the VN Tunnel record.
 *
 * \param[in]       create is TRUE if the TEP should be created if it doesn't
 *                  exist.
 *
 * \param[out]      tepRulePtr points to caller-provided storage where
 *                  the TEP rule pointer will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetEncapTepRule(fm_int               sw,
                                 fm_virtualNetwork *  vn,
                                 fm_vnTunnel *        tunnel,
                                 fm_bool              create,
                                 fm10000_vnEncapTep **tepRulePtr)
{
    fm_status           status;
    fm10000_switch *    switchExt;
    fm10000_vnTunnel *  tunnelExt;
    fm10000_vnEncapTep  tepRuleKey;
    fm10000_vnEncapTep *tepRule;
    fm_bool             incrRuleCounts;
    fm_bool             encapRuleWritten;
    fm_int              flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p (%d), tunnel = %p (%d), create = %d, tepRulePtr = %p\n",
                 sw,
                 (void *) vn,
                 vn->vsId,
                 (void *) tunnel,
                 tunnel->tunnelId,
                 create,
                 (void *) tepRulePtr);

    switchExt        = GET_SWITCH_EXT(sw);
    tunnelExt        = tunnel->extension;
    flowType         = GET_ENCAP_FLOW_TYPE(vn);
    tepRule          = NULL;
    incrRuleCounts   = FALSE;
    encapRuleWritten = FALSE;

    /* Try to find an existing encap TEP record for this tunnel/vni */
    FM_CLEAR(tepRuleKey);
    tepRuleKey.tunnel           = tunnel;
    tepRuleKey.vn               = vn;

    status = fmCustomTreeFind(&tunnelExt->tepRules,
                              &tepRuleKey,
                              (void **) &tepRule);

    if ( (status == FM_ERR_NOT_FOUND) && create )
    {
        /* Create the tunnel group. */
        if (switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT] < 0)
        {
            status = CreateTunnelGroup(sw, FM_VN_ENCAP_GROUP_DIRECT);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }

        /* allocate a new encap TEP record */
        tepRule = fmAlloc( sizeof(fm10000_vnEncapTep) );

        if (tepRule == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_EXIT(FM_LOG_CAT_VN, status);
        }

        FM_CLEAR(*tepRule);

        tepRule->vn              = vn;
        tepRule->tunnel          = tunnel;
        tepRule->encapTunnelRule = -1;

        /* Create the encap flow record if needed. */
        if (tunnelExt->encapFlowIds[FM_VN_ENCAP_GROUP_DIRECT][flowType] < 0)
        {
            status = WriteEncapFlow(sw,
                                    tunnel,
                                    vn,
                                    FM_VN_ENCAP_GROUP_DIRECT);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
        }

        ++tunnelExt->ruleCounts[FM_VN_ENCAP_GROUP_DIRECT][flowType];

        incrRuleCounts = TRUE;

        /* Add the encapsulation TEP tunnel rule. */
        status = WriteEncapTepRule(sw, tepRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        encapRuleWritten = TRUE;

        /* Add the TEP rule to the tree of TEP rules associated with this tunnel. */
        status = fmCustomTreeInsert(&tunnelExt->tepRules, tepRule, tepRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else if (status != FM_OK)
    {
        tepRule = NULL;
        FM_LOG_ABORT(FM_LOG_CAT_VN, status);
    }

    if ( (tepRule != NULL) && create )
    {
        ++tepRule->useCount;
    }

ABORT:

    if (status != FM_OK)
    {
        if (tepRule != NULL)
        {
            if (encapRuleWritten)
            {
                fm10000DeleteTunnelRule(sw,
                                        switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                        tepRule->encapTunnelRule);

                FreeTunnelRuleNum(sw,
                                  FM_VN_ENCAP_GROUP_DIRECT,
                                  tepRule->encapTunnelRule);

                tepRule->encapTunnelRule = -1;
            }

            if (incrRuleCounts)
            {
                if (--tunnelExt->ruleCounts[FM_VN_ENCAP_GROUP_DIRECT][flowType] <= 0)
                {
                    DeleteEncapFlow(sw,
                                    tepRule->tunnel,
                                    FM_VN_ENCAP_GROUP_DIRECT,
                                    flowType);
                }
            }

            fmFree(tepRule);

            tepRule = NULL;
        }
    }

    *tepRulePtr = tepRule;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end GetEncapTepRule */




/*****************************************************************************/
/** DeleteEncapTepRule
 * \ingroup intVN
 *
 * \desc            Deletes an encapsulation tunnel end point rule if it is
 *                  no longer in use.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tepRule points to the encap TEP rule record
 *                  that is to be deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEncapTepRule(fm_int              sw,
                                    fm10000_vnEncapTep *tepRule)
{
    fm_status          status;
    fm10000_switch *   switchExt;
    fm10000_vnTunnel * tunnelExt;
    fm_int             flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tepRule = %p\n",
                 sw,
                 (void *) tepRule);

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tepRule->tunnel->extension;
    flowType  = GET_ENCAP_FLOW_TYPE(tepRule->vn);

    if (--tepRule->useCount < 0)
    {
        status = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_VN, status);
    }

    if (tepRule->useCount > 0)
    {
        status = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_VN, status);
    }

    /* Delete the TEP rule from the tree of TEP rules associated with this tunnel. */
    status = fmCustomTreeRemove(&tunnelExt->tepRules, tepRule, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Delete the encapsulation tunnel rule. */
    status = fm10000DeleteTunnelRule(sw,
                                     switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                     tepRule->encapTunnelRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);


    /* Delete the encap flow record if needed. */
    if (--tunnelExt->ruleCounts[FM_VN_ENCAP_GROUP_DIRECT][flowType] == 0)
    {
        status = DeleteEncapFlow(sw,
                                 tepRule->tunnel,
                                 FM_VN_ENCAP_GROUP_DIRECT,
                                 flowType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Free the TEP record */
    fmFree(tepRule);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end DeleteEncapTepRule */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000CreateVirtualNetwork
 * \ingroup intVN
 *
 * \desc            Creates a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network information structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CreateVirtualNetwork(fm_int             sw,
                                      fm_virtualNetwork *vn)
{
    fm_status               status;
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;
    fm_bool                 mcastGroupActivated;
    fm_bool                 bTrue;
    fm_multicastAddress     bcastAddr;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, vsid = %u, internalId = %u\n",
                  sw,
                  (void *) vn,
                  vn->vsId,
                  vn->descriptor.internalId );

    switchPtr           = GET_SWITCH_PTR(sw);
    switchExt           = GET_SWITCH_EXT(sw);
    mcastGroupActivated = FALSE;
    bTrue               = TRUE;

    status = InitializeVNSubsystem(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    vnExt = fmAlloc( sizeof(fm10000_virtualNetwork) );
    if (vnExt == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MEM);
    }

    vn->extension = vnExt;

    FM_CLEAR(*vnExt);

    vnExt->vn                 = vn;
    vnExt->floodsetMcastGroup = -1;
    vnExt->primaryVsi         = vn->descriptor.internalId;

    fmCustomTreeInit(&vnExt->addressMasks, CompareAddressMasks);
    fmCustomTreeInit(&vnExt->remoteAddresses, CompareRemoteAddresses);
    fmTreeInit(&vnExt->tunnels);

    status = fmCreateMcastGroupInt(sw, &vnExt->floodsetMcastGroup, FALSE, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_CLEAR(bcastAddr);

    bcastAddr.addressType             = FM_MCAST_ADDR_TYPE_L2MAC_VLAN;
    bcastAddr.mcastGroup              = vnExt->floodsetMcastGroup;
    bcastAddr.info.mac.destMacAddress = 0xFFFFFFFFFFFFLL;
    bcastAddr.info.mac.vlan           = vn->descriptor.vlan;

    status = fmAddMcastGroupAddressInt(sw,
                                       vnExt->floodsetMcastGroup,
                                       &bcastAddr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmSetMcastGroupAttributeInt(sw,
                                         vnExt->floodsetMcastGroup,
                                         FM_MCASTGROUP_L3_FLOOD_SET,
                                         &bTrue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmSetMcastGroupAttributeInt(sw,
                                         vnExt->floodsetMcastGroup,
                                         FM_MCASTGROUP_L3_SWITCHING_ONLY,
                                         &bTrue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (vn->descriptor.bcastFlooding)
    {
        status = fmActivateMcastGroupInt(sw, vnExt->floodsetMcastGroup);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        mcastGroupActivated = TRUE;

        status = WriteEncapFloodsetAclRule(sw, vn);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (vn->descriptor.mode == FM_VN_MODE_VSWITCH_OFFLOAD)
         && (vnExt->primaryVsi != -1) )
    {
        switchExt->vnVsi[vnExt->primaryVsi] = vn;

        status = WriteVsi(sw, vn, vnExt->primaryVsi);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    ++switchExt->numVirtualNetworks;

ABORT:

    if (status != FM_OK)
    {
        if (vnExt != NULL)
        {
            if (vnExt->floodsetMcastGroup >= 0)
            {
                if (mcastGroupActivated)
                {
                    fmDeactivateMcastGroupInt(sw, vnExt->floodsetMcastGroup);
                }

                fmDeleteMcastGroupInt(sw, vnExt->floodsetMcastGroup, TRUE);
            }

            fmFree(vnExt);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000CreateVirtualNetwork */




/*****************************************************************************/
/** fm10000DeleteVirtualNetwork
 * \ingroup intVN
 *
 * \desc            Deletes a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network information structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_VIRTUAL_NETWORK_IN_USE if the virtual network is in use.
 *
 *****************************************************************************/
fm_status fm10000DeleteVirtualNetwork(fm_int             sw,
                                      fm_virtualNetwork *vn)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, vsid = %u\n",
                  sw,
                  (void *) vn,
                  vn->vsId );

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    /* If the virtual network is in use, reject the deletion request. */
    if ( fmCustomTreeSize(&vnExt->remoteAddresses) != 0 )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_VIRTUAL_NETWORK_IN_USE);
    }

    if ( fmCustomTreeSize(&vnExt->addressMasks) != 0 )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_VIRTUAL_NETWORK_IN_USE);
    }

    if (vnExt->floodsetEncapAclRule > 0)
    {
        status = DeleteEncapFloodsetAclRule(sw, vn);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmDeactivateMcastGroup(sw, vnExt->floodsetMcastGroup);

    if (status == FM_ERR_MCAST_GROUP_NOT_ACTIVE)
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmDeleteMcastGroupInt(sw, vnExt->floodsetMcastGroup, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if ( (vn->descriptor.mode == FM_VN_MODE_VSWITCH_OFFLOAD)
         && (vnExt->primaryVsi != -1) )
    {
        status = WriteVsi(sw, NULL, vnExt->primaryVsi);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    fmFree(vnExt);
    vn->extension = NULL;

    --switchExt->numVirtualNetworks;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000DeleteVirtualNetwork */




/*****************************************************************************/
/** fm10000UpdateVirtualNetwork
 * \ingroup intVN
 *
 * \desc            Changes virtual network hardware configuration to match
 *                  values in an updated descriptor record.
 *
 * \note            Function assumes that the descriptor record in the
 *                  virtual network record has already been updated and
 *                  that the original descriptor record is pointed to by
 *                  oldDescriptor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn points to the virtual network information structure.
 *
 * \param[in]       oldDescriptor points to the original descriptor values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateVirtualNetwork(fm_int             sw,
                                      fm_virtualNetwork *vn,
                                      fm_vnDescriptor *  oldDescriptor)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, vsid = %u, internalId = %u\n",
                  sw,
                  (void *) vn,
                  vn->vsId,
                  vn->descriptor.internalId );

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    if ( (oldDescriptor->mode == vn->descriptor.mode)
         && (oldDescriptor->internalId == vn->descriptor.internalId) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /* First configure the new VSI */
    if ( (vn->descriptor.mode == FM_VN_MODE_VSWITCH_OFFLOAD)
         && ( vn->descriptor.internalId != (fm_uint) ~0 ) )
    {
        status = WriteVsi(sw, vn, vn->descriptor.internalId);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Now unconfigure the old VSI if different. */
    if ( (oldDescriptor->mode == FM_VN_MODE_VSWITCH_OFFLOAD)
         && (oldDescriptor->internalId != (fm_uint) ~0) )
    {
        status = WriteVsi(sw, NULL, oldDescriptor->internalId);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    vnExt->primaryVsi = vn->descriptor.internalId;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000UpdateVirtualNetwork */




/*****************************************************************************/
/** fm10000CreateVNTunnel
 * \ingroup intVN
 *
 * \desc            Creates a virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel information structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the tunnel ID is greater than
 *                  the maximum value supported by the hardware, since tunnel
 *                  ID is used as an index into the appropriate hardware tables.
 *
 *****************************************************************************/
fm_status fm10000CreateVNTunnel(fm_int       sw,
                                fm_vnTunnel *tunnel)
{
    fm_status         status;
    fm10000_switch *  switchExt;
    fm10000_vnTunnel *tunnelExt;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p, tunnelId=%d, tunnelType=%d\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId,
                  tunnel->tunnelType );

    status = InitializeVNSubsystem(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    switchExt = GET_SWITCH_EXT(sw);

    tunnelExt = fmAlloc( sizeof(fm10000_vnTunnel) );
    if (tunnelExt == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MEM);
    }

    FM_CLEAR(*tunnelExt);

    tunnelExt->tunnel = tunnel;
    tunnel->extension = tunnelExt;
    fmCustomTreeInit(&tunnelExt->tepRules, CompareTepRules);

    FM_MEMSET_S( tunnelExt->encapFlowIds,
                 sizeof(tunnelExt->encapFlowIds),
                 -1,
                 sizeof(tunnelExt->encapFlowIds) );

    ++switchExt->numVNTunnels;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000CreateVNTunnel */




/*****************************************************************************/
/** fm10000DeleteVNTunnel
 * \ingroup intVN
 *
 * \desc            Deletes a virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel information structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_IN_USE if the tunnel is being used.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNTunnel(fm_int       sw,
                               fm_vnTunnel *tunnel)
{
    fm10000_switch *  switchExt;
    fm10000_vnTunnel *tunnelExt;
    fm_int            i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p, tunnelId=%d, tunnelType=%d\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId,
                  tunnel->tunnelType );

    switchExt = GET_SWITCH_EXT(sw);
    tunnelExt = tunnel->extension;

    if (tunnelExt == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /* Make sure the tunnel is not in use. If it is, reject the deletion request. */
    for (i = FM_VN_ENCAP_GROUP_DMAC_VID ; i <= FM_VN_ENCAP_GROUP_DIRECT ; i++)
    {
        if ( (tunnelExt->ruleCounts[i][FM_VN_ENCAP_FLOW_TRANSPARENT] != 0)
             || (tunnelExt->ruleCounts[i][FM_VN_ENCAP_FLOW_VSWITCH] != 0) )
        {
            FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_TUNNEL_IN_USE);
        }
    }

    fmFree(tunnelExt);

    tunnel->extension = NULL;

    --switchExt->numVNTunnels;

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000DeleteVNTunnel */




/*****************************************************************************/
/** fm10000SetVNTunnelAttribute
 * \ingroup intVN
 *
 * \desc            Handles changes to a virtual network tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel information structure.
 *
 * \param[in]       attr is the attribute number.
 *
 * \param[in]       value is the new value for the attribute. This function
 *                  assumes that the value has already been written into
 *                  the tunnel information structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TABLE_FULL if there is no room in the tunneling
 *                  hardware for the default tunnel rule for this tunnel.
 *
 *****************************************************************************/
fm_status fm10000SetVNTunnelAttribute(fm_int              sw,
                                     fm_vnTunnel *       tunnel,
                                     fm_vnTunnelAttrType attr,
                                     void *              value)
{
    fm_status        status;
    fm10000_vnTunnel *tunnelExt;
    fm_int           trafficIdentifier;
    fm_bool          updateNextHops;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p, tunnelId = %d, attr = %d, value=%p\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId,
                  attr,
                  value );

    FM_NOT_USED(value);

    updateNextHops = FALSE;
    tunnelExt      = tunnel->extension;

    switch (attr)
    {
        case FM_VNTUNNEL_ATTR_LOCAL_IP:
            tunnelExt->haveLocalIp = TRUE;
            status = UpdateVNTunnel(sw, tunnel);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            updateNextHops = TRUE;
            break;

        case FM_VNTUNNEL_ATTR_REMOTE_IP:
            tunnelExt->haveRemoteIp = TRUE;
            status = UpdateVNTunnel(sw, tunnel);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            updateNextHops = TRUE;
            break;

        case FM_VNTUNNEL_ATTR_VRID:
            /* Nothing to do until remote IP attribute is set. */
            status = FM_OK;
            break;

        case FM_VNTUNNEL_ATTR_MCAST_GROUP:
            updateNextHops = TRUE;
            status = FM_OK;
            break;

        case FM_VNTUNNEL_ATTR_MCAST_DMAC:
            updateNextHops = TRUE;
            status = FM_OK;
            break;

        case FM_VNTUNNEL_ATTR_TRAFFIC_IDENTIFIER:
            trafficIdentifier = *( (fm_int *) value );
            tunnel->trafficIdentifier = trafficIdentifier;
            updateNextHops = TRUE;
            status = FM_OK;
            break;

        case FM_VNTUNNEL_ATTR_ENCAP_TTL:
            status = UpdateVNTunnel(sw, tunnel);
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000SetVNTunnelAttribute */




/*****************************************************************************/
/** fm10000GetVNTunnelGroupAndRule
 * \ingroup intVN
 *
 * \desc            Gets the Tunneling Engine API group, rule, and glort
 *                  for a VN Tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel ID number.
 *
 * \param[in]       encap is TRUE to retrieve encap rule information, FALSE
 *                  to retrieve decap rule information.
 *
 * \param[in]       vni is the VNI number for the desired rule.
 *
 * \param[out]      group points to caller-provided storage into which the
 *                  tunneling engine group will be written, if this
 *                  pointer is not NULL.
 *
 * \param[out]      rule points to caller-provided storage into which
 *                  the tunnel rule number will be written, if this
 *                  pointer is not NULL.
 *
 * \param[out]      glort points to caller-provided storage into which the
 *                  tunneling engine glort will be written, if this pointer
 *                  is not NULL.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetVNTunnelGroupAndRule(fm_int              sw,
                                         fm_int              tunnelId,
                                         fm_bool             encap,
                                         fm_uint32           vni,
                                         fm_int *            group,
                                         fm_int *            rule,
                                         fm_tunnelGlortUser *glort)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_vnTunnel *           tunnel;
    fm_virtualNetwork *     vn;
    fm10000_virtualNetwork *vnExt;
    fm_int                  encapTunnelGroup;
    fm_int                  decapTunnelGroup;
    fm_int                  tunnelGroup;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelId = %d, group = %p, rule = %p, glort = %p\n",
                 sw,
                 tunnelId,
                 (void *) group,
                 (void *) rule,
                 (void *) glort );

    switchExt = GET_SWITCH_EXT(sw);

    /* Get the VN record. */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
       FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get the Tunnel record. */
    tunnel = fmGetVNTunnel(sw, tunnelId);
    if (tunnel == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    vnExt = vn->extension;

    status = GetTunnelGroups(sw,
                             vn->descriptor.addressType,
                             &encapTunnelGroup,
                             &decapTunnelGroup);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    tunnelGroup = (encap) ? encapTunnelGroup : decapTunnelGroup;

    if ( encap && (encapTunnelGroup == switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT]) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_UNSUPPORTED);
    }

    if (glort != NULL)
    {
        status = fm10000GetTunnelAttribute(sw,
                                           tunnelGroup,
                                           -1,
                                           FM_TUNNEL_GLORT_USER,
                                           glort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (group != NULL)
    {
        if (encap)
        {
            *group = encapTunnelGroup;
        }
        else
        {
            *group = decapTunnelGroup;
        }
    }

    if (rule != NULL)
    {
        *rule = -1;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000GetVNTunnelGroupAndRule */




/*****************************************************************************/
/** fm10000AddVNRemoteAddress
 * \ingroup intVN
 *
 * \desc            Adds a remote address to a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       tunnel is the tunnel record.
 *
 * \param[in]       addr points to the address to be added.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddVNRemoteAddress(fm_int             sw,
                                    fm_virtualNetwork *vn,
                                    fm_vnTunnel *      tunnel,
                                    fm_vnAddress *     addr)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;
    fm10000_vnTunnel *      tunnelExt;
    fm10000_vnRemoteAddress *addrRec;
    fm_bool                 encapAclRuleAdded;
    fm_bool                 tunnelRuleAdded;
    fm_bool                 remoteAddrAdded;
    fm10000_vnDecapAclRule *decapAclRule;
    fm_int                  flowType;
    fm_int                  encapTunnelGroup;
    fm_bool                 wroteEncapFlow;
    fm_bool                 wroteEncapTunnelRule;
    fm_bool                 wroteEncapAclRule;
    fm_bool                 addedTunnelRuleToVN;
    fm10000_vnEncapTep *    tepRule;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, addr = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 (void *) addr);

    switchExt            = GET_SWITCH_EXT(sw);
    vnExt                = vn->extension;
    tunnelExt            = tunnel->extension;
    decapAclRule         = NULL;
    encapAclRuleAdded    = FALSE;
    tunnelRuleAdded      = FALSE;
    remoteAddrAdded      = FALSE;
    flowType             = GET_ENCAP_FLOW_TYPE(vn);
    encapTunnelGroup     = -1;
    wroteEncapAclRule    = FALSE;
    wroteEncapFlow       = FALSE;
    wroteEncapTunnelRule = FALSE;
    addedTunnelRuleToVN  = FALSE;

    /* Allocate and populate a tunnel rule record. */
    addrRec = fmAlloc( sizeof(fm10000_vnRemoteAddress) );
    if (addrRec == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_CLEAR(*addrRec);

    addrRec->tunnel           = tunnel;
    addrRec->vn               = vn;
    addrRec->decapTunnelRule  = -1;
    addrRec->encapAclRule     = -1;
    addrRec->encapTunnelGroup = -1;

    FM_MEMSET_S( addrRec->encapTunnelRules,
                 sizeof(addrRec->encapTunnelRules),
                 -1,
                 sizeof(addrRec->encapTunnelRules) );

    status = GetTunnelGroups(sw,
                             vn->descriptor.addressType,
                             &addrRec->hashEncapGroup,
                             &addrRec->decapTunnelGroup);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_MEMCPY_S( &addrRec->remoteAddress,
                 sizeof(fm_vnAddress),
                 addr,
                 sizeof(fm_vnAddress) );

    CheckAddrMasksForRemoteAddr(sw, vn, addr, &addrRec->addrMask);

    encapTunnelGroup = GET_ENCAP_TUNNEL_GROUP(addrRec);

    addrRec->encapTunnelGroup = encapTunnelGroup;

    if (encapTunnelGroup >= 0)
    {
        /* Create the encap flow record if needed. */
        if (tunnelExt->encapFlowIds[encapTunnelGroup][flowType] < 0)
        {
            status = WriteEncapFlow(sw,
                                    tunnel,
                                    vn,
                                    encapTunnelGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            wroteEncapFlow = TRUE;
        }

        ++tunnelExt->ruleCounts[encapTunnelGroup][flowType];
    }

    /* Update the tunnel use count and broadcast/flooding listener list. */
    status = AddTunnelRuleToVN(sw, vn, tunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    addedTunnelRuleToVN = TRUE;

    if (encapTunnelGroup == FM_VN_ENCAP_GROUP_DIRECT)
    {
        /* Get the encap tunnel end-point for this tunnel/vni */
        status = GetEncapTepRule(sw, vn, tunnel, TRUE, &tepRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        addrRec->encapTep = tepRule;
    }
    else if (encapTunnelGroup == addrRec->hashEncapGroup)
    {
        /* Add the encapsulation tunnel rule. */
        status = WriteEncapTunnelRule(sw, addrRec);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        wroteEncapTunnelRule = TRUE;
    }

    /* Add the decapsulation tunnel rule. */
    status = WriteDecapTunnelRule(sw, addrRec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (encapTunnelGroup == FM_VN_ENCAP_GROUP_DIRECT)
    {
        /* Write the Encapsulation ACL Rule. */
        status = WriteEncapAclRule(sw, addrRec);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        wroteEncapAclRule = TRUE;
    }

    /* Add the address record to the tree of remote addresses associated with
     * this virtual network. */
    status = fmCustomTreeInsert(&vnExt->remoteAddresses, addrRec, addrRec);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    remoteAddrAdded = TRUE;

    status = CreateDecapAclRule(sw,
                                addrRec->vn,
                                addrRec->tunnel,
                                addrRec->decapTunnelGroup,
                                &decapAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if ( (status != FM_OK) && (addrRec != NULL) )
    {
        if (decapAclRule != NULL)
        {
            DeleteDecapAclRule(sw, decapAclRule);
        }

        DeleteDecapTunnelRule(sw, addrRec);

        if (wroteEncapAclRule)
        {
            DeleteEncapAclRule(sw, addrRec);
        }

        if (addrRec->encapTep != NULL)
        {
            DeleteEncapTepRule(sw, addrRec->encapTep);
        }

        if (wroteEncapTunnelRule)
        {
            DeleteEncapTunnelRule(sw, addrRec);
        }

        if (addedTunnelRuleToVN)
        {
            DeleteTunnelRuleFromVN(sw, vn, tunnel);
        }

        if (wroteEncapFlow)
        {
            --tunnelExt->ruleCounts[encapTunnelGroup][flowType];
            DeleteEncapFlow(sw, tunnel, encapTunnelGroup, flowType);
        }

        if (remoteAddrAdded)
        {
            fmCustomTreeRemove(&vnExt->remoteAddresses, addrRec, NULL);
        }

        fmFree(addrRec);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000AddVNRemoteAddress */




/*****************************************************************************/
/** fm10000DeleteVNRemoteAddress
 * \ingroup intVN
 *
 * \desc            Deletes a remote address from a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       tunnel is the tunnel record.
 *
 * \param[in]       addr points to the address to be deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNRemoteAddress(fm_int             sw,
                                       fm_virtualNetwork *vn,
                                       fm_vnTunnel *      tunnel,
                                       fm_vnAddress *     addr)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm10000_virtualNetwork *vnExt;
    fm10000_vnTunnel *      tunnelExt;
    fm10000_vnRemoteAddress *addrRec;
    fm10000_vnRemoteAddress  addrKey;
    fm10000_vnDecapAclRule *decapAclRule;
    fm_int                  flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, addr = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 (void *) addr);

    switchExt        = GET_SWITCH_EXT(sw);
    vnExt            = vn->extension;
    tunnelExt        = tunnel->extension;
    flowType         = GET_ENCAP_FLOW_TYPE(vn);

    /* Try to find the remote address record. */
    FM_CLEAR(addrKey);
    addrKey.tunnel        = tunnel;
    addrKey.vn            = vn;
    addrKey.remoteAddress = *addr;

    status = fmCustomTreeFind(&vnExt->remoteAddresses,
                              &addrKey,
                              (void **) &addrRec);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCustomTreeRemove(&vnExt->remoteAddresses, addrRec, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (addrRec->encapAclRule >= 0)
    {
        status = DeleteEncapAclRule(sw, addrRec);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addrRec->encapTep != NULL)
    {
        status = DeleteEncapTepRule(sw, addrRec->encapTep);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addrRec->encapTunnelGroup == addrRec->hashEncapGroup)
    {
        /* Delete the encapsulation tunnel rule(s). */
        status = DeleteEncapTunnelRule(sw, addrRec);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addrRec->encapTunnelGroup >= 0)
    {
        if (--tunnelExt->ruleCounts[addrRec->encapTunnelGroup][flowType] == 0)
        {
            status = DeleteEncapFlow(sw,
                                     tunnel,
                                     addrRec->encapTunnelGroup,
                                     flowType);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }

    status = GetDecapAclRule(sw, vn, tunnel, &decapAclRule);

    if (status == FM_OK)
    {
        status = DeleteDecapAclRule(sw, decapAclRule);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    else
    {
        if (status == FM_ERR_NOT_FOUND)
        {
            status = FM_OK;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addrRec->decapTunnelRule >= 0)
    {
        status = DeleteDecapTunnelRule(sw, addrRec);
    }

    /* Update the tunnel use count and broadcast/flooding listener list. */
    status = DeleteTunnelRuleFromVN(sw, vn, tunnel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    fmFree(addrRec);

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000DeleteVNRemoteAddress */




/*****************************************************************************/
/** fm10000AddVNRemoteAddressMask
 * \ingroup intVN
 *
 * \desc            Adds a remote address mask to a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       tunnel is the tunnel record, or NULL if this mask
 *                  is not associated with a single tunnel.
 *
 * \param[in]       addr points to the address to be added.
 *
 * \param[in]       addrMask points to the address bit-mask to be used
 *                  with the address.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_IN_VN_TRANSPARENT_MODE if this function is
 *                  called for a virtual network that is in transparent mode
 *                  and the switch is not operating in "decap all traffic"
 *                  mode. The problem is that when we aren't in "decap all traffic"
 *                  mode, decap ACLs have to point to hash tunneling engine
 *                  groups. The tunneling engine can't match on masks, though,
 *                  only on exact matches, so we can't match on an address mask in
 *                  a hash group. When we are in "decap all traffic", we don't
 *                  need to have every remote address in a decap TE hash group
 *                  because we aren't trying to only decap traffic from known
 *                  remote addresses. We decap in a single direct rule that
 *                  handles all decap traffic, and we can thus support
 *                  transparent mode.
 *
 *****************************************************************************/
fm_status fm10000AddVNRemoteAddressMask(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_vnTunnel *      tunnel,
                                        fm_vnAddress *     addr,
                                        fm_vnAddress *     addrMask)
{
    fm_status                    status;
    fm10000_switch *             switchExt;
    fm10000_virtualNetwork *     vnExt;
    fm10000_vnTunnel *           tunnelExt;
    fm10000_vnTunnel *           addrTunnelExt;
    fm10000_vnRemoteAddressMask *addressMask;
    fm_vnAddressType             addrType;
    fm_int                       encapTunnelGroup;
    fm_int                       decapTunnelGroup;
    fm_int                       encapTunnelRule;
    fm_int                       aclRule;
    fm_aclCondition              aclCond;
    fm_aclValue                  aclValue;
    fm_aclActionExt              aclAction;
    fm_aclParamExt               aclParam;
    fm_bool                      aclRuleAdded;
    fm_bool                      addedToTree;
    fm_char                      statusText[STATUS_TEXT_LEN];
    fm10000_vnDecapAclRule *     decapAclRule;
    fm_int                       encapFlowType;
    fm10000_vnEncapTep *         tepRule;
    fm10000_vnRemoteAddress **   remAddrList;
    fm10000_vnRemoteAddress **   newRemAddrList;
    fm_int                       remAddrListSize;
    fm_int                       numRemAddrs;
    fm_customTreeIterator        iter;
    fm10000_vnRemoteAddress *    addrKey;
    fm10000_vnRemoteAddress *    addrRec;
    fm_bool                      foundMatch;
    fm_int                       i;
    fm_int                       addrsRemoved;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, tunnel = %p, addr = %p, addrMask = %p\n",
                 sw,
                 (void *) vn,
                 (void *) tunnel,
                 (void *) addr,
                 (void *) addrMask);

    if ( (tunnel == NULL) && (vn->descriptor.mode == FM_VN_MODE_TRANSPARENT) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_IN_VN_TRANSPARENT_MODE);
    }

    switchExt       = GET_SWITCH_EXT(sw);
    vnExt           = vn->extension;
    addrType        = vn->descriptor.addressType;
    aclRuleAdded    = FALSE;
    addedToTree     = FALSE;
    decapAclRule    = NULL;
    encapFlowType   = GET_ENCAP_FLOW_TYPE(vn);
    remAddrList     = NULL;
    remAddrListSize = 0;
    numRemAddrs     = 0;
    addrsRemoved    = 0;

    status = GetTunnelGroups(sw, addrType, &encapTunnelGroup, &decapTunnelGroup);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (tunnel != NULL)
    {
        encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
    }

    /* Allocate and populate a remote address mask record. */
    addressMask = fmAlloc( sizeof(fm10000_vnRemoteAddressMask) );
    if (addressMask == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_CLEAR(*addressMask);

    addressMask->vn              = vn;
    addressMask->tunnel          = tunnel;
    addressMask->encapAclRule    = -1;
    addressMask->encapTunnelRule = -1;
    FM_MEMCPY_S( &addressMask->remoteAddress,
                 sizeof(fm_vnAddress),
                 addr,
                 sizeof(fm_vnAddress) );
    FM_MEMCPY_S( &addressMask->addrMask,
                 sizeof(fm_vnAddress),
                 addrMask,
                 sizeof(fm_vnAddress) );

    switch (addrType)
    {
        case FM_VN_ADDR_TYPE_MAC:
        case FM_VN_ADDR_TYPE_IP:
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (switchExt->vnTunnelGroups[encapTunnelGroup] < 0)
    {
        status = CreateTunnelGroup(sw, encapTunnelGroup);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (tunnel == NULL)
    {
        /* Add all remote addresses included within this address mask to
         * the encap hash group. */
        fmCustomTreeIterInit(&iter, &vnExt->remoteAddresses);

        while (1)
        {
            status = fmCustomTreeIterNext(&iter,
                                          (void **) &addrKey,
                                          (void **) &addrRec);

            if (status == FM_ERR_NO_MORE)
            {
                status = FM_OK;
                break;
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            if (addrRec->vn == vn)
            {
                foundMatch = FALSE;

                switch (vn->descriptor.addressType)
                {
                    case FM_VN_ADDR_TYPE_MAC:
                        if ( (addr->macAddress & addrMask->macAddress)
                             == (addrRec->remoteAddress.macAddress & addrMask->macAddress) )
                        {
                            foundMatch = TRUE;
                        }
                        break;

                    case FM_VN_ADDR_TYPE_IP:
                        if ( fmIsIPAddressInRangeMask(&addr->ipAddress,
                                                      &addrMask->ipAddress,
                                                      &addrRec->remoteAddress.ipAddress) )
                        {
                            foundMatch = TRUE;
                        }
                        break;

                    default:
                        FM_LOG_ABORT(FM_LOG_CAT_VN, FM_FAIL);
                        break;
                }

                if (foundMatch)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_VN,
                                 "Found matching addrRec %p\n",
                                 (void *) addrRec);

                    if (numRemAddrs == remAddrListSize)
                    {
                        /* This array is only used locally to this thread
                         * during this function's execution. It does not need
                         * to be allocated from shared memory. */
                        newRemAddrList = malloc( sizeof(fm10000_vnRemoteAddress *)
                                                 * (remAddrListSize + 10000) );
                        if (newRemAddrList == NULL)
                        {
                            status = FM_ERR_NO_MEM;
                            FM_LOG_ABORT(FM_LOG_CAT_VN, status);
                        }

                        if (remAddrList != NULL)
                        {
                            FM_MEMCPY_S(newRemAddrList,
                                        sizeof(fm10000_vnRemoteAddress *) * remAddrListSize,
                                        remAddrList,
                                        sizeof(fm10000_vnRemoteAddress *) * remAddrListSize);
                            free(remAddrList);
                        }

                        remAddrList = newRemAddrList;
                        remAddrListSize += 10000;

                        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                                     "Allocated temporary address list at %p with %d entries\n",
                                     (void *) remAddrList,
                                     remAddrListSize);
                    }

                    remAddrList[numRemAddrs++] = addrRec;

                    /* Remove the remote address from the encap direct group. */

                    if (addrRec->encapTep != NULL)
                    {
                        status = DeleteEncapTepRule(sw, addrRec->encapTep);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

                        addrRec->encapTep = NULL;
                    }

                    addrTunnelExt = addrRec->tunnel->extension;

                    if (addrRec->encapTunnelGroup >= 0)
                    {
                        if (--addrTunnelExt->ruleCounts[addrRec->encapTunnelGroup][encapFlowType] == 0)
                        {
                            status = DeleteEncapFlow(sw,
                                                     addrRec->tunnel,
                                                     addrRec->encapTunnelGroup,
                                                     encapFlowType);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                        }
                    }

                    addrRec->encapTunnelGroup = addrRec->hashEncapGroup;
                    addrRec->addrMask         = addressMask;

                    /* Create the encap flow record if needed. */
                    if (addrTunnelExt->encapFlowIds[addrRec->encapTunnelGroup][encapFlowType] < 0)
                    {
                        status = WriteEncapFlow(sw,
                                                addrRec->tunnel,
                                                vn,
                                                addrRec->encapTunnelGroup);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                    }

                    ++addrTunnelExt->ruleCounts[addrRec->encapTunnelGroup][encapFlowType];

                    status = WriteEncapTunnelRule(sw, addrRec);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                }
            }
        }
    }

    if (tunnel != NULL)
    {
        /* Update all remote addresses included within this address mask */
        fmCustomTreeIterInit(&iter, &vnExt->remoteAddresses);

        while (1)
        {
            status = fmCustomTreeIterNext(&iter,
                                          (void **) &addrKey,
                                          (void **) &addrRec);

            if (status == FM_ERR_NO_MORE)
            {
                status = FM_OK;
                break;
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            if (addrRec->vn == vn)
            {
                foundMatch = FALSE;

                switch (vn->descriptor.addressType)
                {
                    case FM_VN_ADDR_TYPE_MAC:
                        if ( (addr->macAddress & addrMask->macAddress)
                             == (addrRec->remoteAddress.macAddress & addrMask->macAddress) )
                        {
                            foundMatch = TRUE;
                        }
                        break;

                    case FM_VN_ADDR_TYPE_IP:
                        if ( fmIsIPAddressInRangeMask(&addr->ipAddress,
                                                      &addrMask->ipAddress,
                                                      &addrRec->remoteAddress.ipAddress) )
                        {
                            foundMatch = TRUE;
                        }
                        break;

                    default:
                        FM_LOG_ABORT(FM_LOG_CAT_VN, FM_FAIL);
                        break;
                }

                if (foundMatch)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_VN,
                                 "Found matching addrRec %p\n",
                                 (void *) addrRec);

                    if (numRemAddrs == remAddrListSize)
                    {
                        /* This array is only used locally to this thread
                         * during this function's execution. It does not need
                         * to be allocated from shared memory. */
                        newRemAddrList = malloc( sizeof(fm10000_vnRemoteAddress *)
                                                 * (remAddrListSize + 10000) );
                        if (newRemAddrList == NULL)
                        {
                            status = FM_ERR_NO_MEM;
                            FM_LOG_ABORT(FM_LOG_CAT_VN, status);
                        }

                        if (remAddrList != NULL)
                        {
                            FM_MEMCPY_S(newRemAddrList,
                                        sizeof(fm10000_vnRemoteAddress *) * remAddrListSize,
                                        remAddrList,
                                        sizeof(fm10000_vnRemoteAddress *) * remAddrListSize);
                            free(remAddrList);
                        }

                        remAddrList = newRemAddrList;
                        remAddrListSize += 10000;

                        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                                     "Allocated temporary address list at %p with %d entries\n",
                                     (void *) remAddrList,
                                     remAddrListSize);
                    }

                    remAddrList[numRemAddrs++] = addrRec;

                    /* Remove the remote address from the encap direct group. */

                    if (addrRec->encapTep != NULL)
                    {
                        status = DeleteEncapTepRule(sw, addrRec->encapTep);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

                        addrRec->encapTep = NULL;
                    }

                    addrTunnelExt = addrRec->tunnel->extension;
                    if (addrRec->encapTunnelGroup >= 0)
                    {
                        if (--addrTunnelExt->ruleCounts[addrRec->encapTunnelGroup][encapFlowType] == 0)
                        {
                            status = DeleteEncapFlow(sw,
                                                     addrRec->tunnel,
                                                     addrRec->encapTunnelGroup,
                                                     encapFlowType);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                        }
                    }

                    addrRec->encapTunnelGroup = -1;
                    addrRec->addrMask         = addressMask;

                }
            }
        }

        tunnelExt = tunnel->extension;

        /* Update the tunnel use count and broadcast/flooding listener list. */
        status = AddTunnelRuleToVN(sw, vn, tunnel);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = GetEncapTepRule(sw, vn, tunnel, TRUE, &tepRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (tepRule == NULL)
        {
            status = FM_FAIL;
            FM_LOG_ABORT(FM_LOG_CAT_VN, status);
        }

        addressMask->encapTep = tepRule;

        encapTunnelRule = tepRule->encapTunnelRule;
    }
    else
    {
        encapTunnelRule = 0;
    }

    /* Get an encap ACL rule number. */
    status = AllocateTunnelAclRuleNum(sw,
                                      encapTunnelGroup,
                                      ACL_RULE_TYPE_ADDR_MASK,
                                      &aclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    addressMask->encapAclRule = aclRule;

    FM_CLEAR(aclValue);
    FM_CLEAR(aclParam);
    aclCond              = FM_ACL_MATCH_VLAN;
    aclValue.vlanId      = vn->descriptor.vlan;
    aclValue.vlanIdMask  = 0x0FFF;
    aclAction            = FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
    aclParam.tunnelGroup = switchExt->vnTunnelGroups[encapTunnelGroup];
    aclParam.tunnelRule  = encapTunnelRule;

    if (addrType == FM_VN_ADDR_TYPE_MAC)
    {
        /* Fill in the MAC-based ACL rule. */
        aclCond          |= FM_ACL_MATCH_DST_MAC;
        aclValue.dst      = addr->macAddress;
        aclValue.dstMask  = addrMask->macAddress;
    }
    else if (addrType == FM_VN_ADDR_TYPE_IP)
    {
        /* Fill in the IP-based ACL rule. */
        aclCond |= FM_ACL_MATCH_DST_IP;
        FM_MEMCPY_S(&aclValue.dstIp,
                    sizeof(fm_ipAddr),
                    addr,
                    sizeof(fm_ipAddr) );
        FM_MEMCPY_S(&aclValue.dstIpMask,
                    sizeof(fm_ipAddr),
                    addrMask,
                    sizeof(fm_ipAddr) );
    }

    status = fmAddACLRuleExt(sw,
                             switchExt->vnEncapAcl,
                             aclRule,
                             aclCond,
                             &aclValue,
                             aclAction,
                             &aclParam);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    aclRuleAdded = TRUE;

    status = fmCompileACLExt(sw,
                             statusText,
                             sizeof(statusText),
                             FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                             | FM_ACL_COMPILE_FLAG_INTERNAL,
                             &switchExt->vnEncapAcl);
    FM_LOG_DEBUG(FM_LOG_CAT_VN,
                 "ACL compiled, status=%d, statusText=%s\n",
                 status,
                 statusText);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmApplyACLExt(sw,
                           FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                           | FM_ACL_APPLY_FLAG_INTERNAL,
                           &switchExt->vnEncapAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Remove affected remote address encap ACL rules. */
    for (addrsRemoved = 0 ; addrsRemoved < numRemAddrs ; addrsRemoved++)
    {
        addrRec = remAddrList[addrsRemoved];

        if (addrRec->encapAclRule >= 0)
        {
            addrRec->encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
            addrRec->addrMask         = NULL;

            status = DeleteEncapAclRule(sw, addrRec);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            if (tunnel != NULL)
            {
                addrRec->encapTunnelGroup = -1;
                addrRec->addrMask         = addressMask;
            }
            else
            {
                addrRec->encapTunnelGroup = addrRec->hashEncapGroup;
                addrRec->addrMask         = addressMask;
            }
        }
    }

    status = fmCustomTreeInsert(&vnExt->addressMasks, addressMask, addressMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    addedToTree = TRUE;

    if (tunnel != NULL)
    {
        status = CreateDecapAclRule(sw,
                                    vn,
                                    tunnel,
                                    decapTunnelGroup,
                                    &decapAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if ( (status != FM_OK) && (addressMask != NULL) )
    {
        if (addedToTree)
        {
            fmCustomTreeRemove(&vnExt->addressMasks, addressMask, NULL);
        }

        if (addrsRemoved > 0)
        {
            for (i = 0 ; i < addrsRemoved ; i++)
            {
                addrRec                   = remAddrList[i];
                addrRec->encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
                addrRec->addrMask         = NULL;
                WriteEncapAclRule(sw, remAddrList[i]);
            }
        }

        if (aclRuleAdded)
        {
            fmDeleteACLRule(sw, switchExt->vnEncapAcl, aclRule);
            fmCompileACLExt(sw,
                            statusText,
                            sizeof(statusText),
                            FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                            | FM_ACL_COMPILE_FLAG_INTERNAL,
                            &switchExt->vnEncapAcl);
            fmApplyACLExt(sw,
                          FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                          | FM_ACL_APPLY_FLAG_INTERNAL,
                          &switchExt->vnEncapAcl);
        }

        if (addressMask->encapAclRule >= 0)
        {
            FreeTunnelAclRuleNum(sw,
                                 encapTunnelGroup,
                                 ACL_RULE_TYPE_ADDR_MASK,
                                 addressMask->encapAclRule);
        }

        for (i = 0 ; i < numRemAddrs ; i++)
        {
            addrRec = remAddrList[i];

            addrRec->encapTunnelGroup = addrRec->hashEncapGroup;
            DeleteEncapTunnelRule(sw, addrRec);
            addrRec->encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
        }

        if (addressMask->encapTunnelRule >= 0)
        {
            fm10000DeleteTunnelRule(sw,
                                    encapTunnelGroup,
                                    addressMask->encapTunnelRule);
            FreeTunnelRuleNum(sw,
                              encapTunnelGroup,
                              addressMask->encapTunnelRule);
        }

        if (decapAclRule != NULL)
        {
            DeleteDecapAclRule(sw, decapAclRule);
        }

        fmFree(addressMask);
    }

    if (remAddrList != NULL)
    {
        free(remAddrList);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000AddVNRemoteAddressMask */




/*****************************************************************************/
/** fm10000DeleteVNRemoteAddressMask
 * \ingroup intVN
 *
 * \desc            Deletes a remote address mask from a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       addr points to the address mask to be deleted.
 *
 * \param[in]       addrMask points to the address bit mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNRemoteAddressMask(fm_int             sw,
                                           fm_virtualNetwork *vn,
                                           fm_vnAddress *     addr,
                                           fm_vnAddress *     addrMask)
{
    fm_status                    status;
    fm10000_switch *             switchExt;
    fm10000_virtualNetwork *     vnExt;
    fm10000_vnTunnel *           addrTunnelExt;
    fm10000_vnRemoteAddressMask  ruleKey;
    fm10000_vnRemoteAddressMask *addressMask;
    fm_vnAddressType             addrType;
    fm_int                       encapTunnelGroup;
    fm_customTreeIterator        iter;
    fm_char                      statusText[STATUS_TEXT_LEN];
    fm10000_vnDecapAclRule *     decapAclRule;
    fm_int                       encapFlowType;
    fm10000_vnRemoteAddress **   remAddrList;
    fm10000_vnRemoteAddress **   newRemAddrList;
    fm_int                       remAddrListSize;
    fm_int                       numRemAddrs;
    fm10000_vnRemoteAddress *    addrKey;
    fm10000_vnRemoteAddress *    addrRec;
    fm_bool                      foundMatch;
    fm_int                       i;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, addr = %p, addrMask = %p\n",
                 sw,
                 (void *) vn,
                 (void *) addr,
                 (void *) addrMask);

    switchExt       = GET_SWITCH_EXT(sw);
    vnExt           = vn->extension;
    addrType        = vn->descriptor.addressType;
    encapFlowType   = GET_ENCAP_FLOW_TYPE(vn);
    remAddrList     = NULL;
    remAddrListSize = 0;
    numRemAddrs     = 0;

    status = GetTunnelGroups(sw, addrType, &encapTunnelGroup, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Try to find the address mask record. */
    FM_CLEAR(ruleKey);
    ruleKey.vn            = vn;
    ruleKey.remoteAddress = *addr;
    ruleKey.addrMask      = *addrMask;

    status = fmCustomTreeFind(&vnExt->addressMasks,
                              &ruleKey,
                              (void **) &addressMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmCustomTreeRemove(&vnExt->addressMasks, addressMask, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* If any tunnel rules now need individual encap acl rules, add them. */
    fmCustomTreeIterInit(&iter, &vnExt->remoteAddresses);

    while ( ( status = fmCustomTreeIterNext(&iter,
                                            (void **) &addrKey,
                                            (void **) &addrRec) ) == FM_OK )
    {
        if (addrRec->vn == vn)
        {
            foundMatch = FALSE;

            switch (vn->descriptor.addressType)
            {
                case FM_VN_ADDR_TYPE_MAC:
                    if ( (addr->macAddress & addrMask->macAddress)
                         == (addrRec->remoteAddress.macAddress & addrMask->macAddress) )
                    {
                        foundMatch = TRUE;
                    }
                    break;

                case FM_VN_ADDR_TYPE_IP:
                    if ( fmIsIPAddressInRangeMask(&addr->ipAddress,
                                                  &addrMask->ipAddress,
                                                  &addrRec->remoteAddress.ipAddress) )
                    {
                        foundMatch = TRUE;
                    }
                    break;

                default:
                    FM_LOG_ABORT(FM_LOG_CAT_VN, FM_FAIL);
                    break;
            }

            if (foundMatch)
            {
                if (numRemAddrs == remAddrListSize)
                {
                    /* This array is only used locally to this thread
                     * during this function's execution. It does not need
                     * to be allocated from shared memory. */
                    newRemAddrList = malloc( sizeof(fm10000_vnRemoteAddress *)
                                             * (remAddrListSize + 10000) );
                    if (newRemAddrList == NULL)
                    {
                        status = FM_ERR_NO_MEM;
                        FM_LOG_ABORT(FM_LOG_CAT_VN, status);
                    }

                    if (remAddrList != NULL)
                    {
                        FM_MEMCPY_S(newRemAddrList,
                                    sizeof(fm10000_vnRemoteAddress *) * remAddrListSize,
                                    remAddrList,
                                    sizeof(fm10000_vnRemoteAddress *) * remAddrListSize);
                        free(remAddrList);
                    }

                    remAddrList = newRemAddrList;
                    remAddrListSize += 10000;
                }

                remAddrList[numRemAddrs++] = addrRec;

                if (addressMask->tunnel == NULL)
                {
                    /* Remove the remote address from the encap hash group
                     * and add it to the encap direct group. */

                    addrTunnelExt = addrRec->tunnel->extension;
                    if (addrRec->encapTunnelGroup >= 0)
                    {
                        if (--addrTunnelExt->ruleCounts[addrRec->encapTunnelGroup][encapFlowType] == 0)
                        {
                            status = DeleteEncapFlow(sw,
                                                     addrRec->tunnel,
                                                     addrRec->encapTunnelGroup,
                                                     encapFlowType);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                        }
                    }

                    if (addrTunnelExt->encapFlowIds[FM_VN_ENCAP_GROUP_DIRECT][encapFlowType] < 0)
                    {
                        status = WriteEncapFlow(sw,
                                                addrRec->tunnel,
                                                vn,
                                                FM_VN_ENCAP_GROUP_DIRECT);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                    }

                    ++addrTunnelExt->ruleCounts[FM_VN_ENCAP_GROUP_DIRECT][encapFlowType];

                    /* Get the encap tunnel end-point for this tunnel/vni */
                    status = GetEncapTepRule(sw, vn, addrRec->tunnel, TRUE, &addrRec->encapTep);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                }
                else
                {
                    /* Add the remote address to the encap direct group. */

                    /* Create the encap flow record. */
                    addrTunnelExt = addrRec->tunnel->extension;
                    if (addrTunnelExt->encapFlowIds[FM_VN_ENCAP_GROUP_DIRECT][encapFlowType] < 0)
                    {
                        status = WriteEncapFlow(sw,
                                                addrRec->tunnel,
                                                vn,
                                                FM_VN_ENCAP_GROUP_DIRECT);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                    }

                    ++addrTunnelExt->ruleCounts[FM_VN_ENCAP_GROUP_DIRECT][encapFlowType];

                    /* Get the encap tunnel end-point for this tunnel/vni */
                    status = GetEncapTepRule(sw, vn, addrRec->tunnel, TRUE, &addrRec->encapTep);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                }

                addrRec->encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
                addrRec->addrMask         = NULL;

                status = WriteEncapAclRule(sw, addrRec);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }
        }
    }

    if (status == FM_ERR_NO_MORE)
    {
        status = FM_OK;
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Now remove the address mask's encap ACL rule. */
    if (addressMask->encapAclRule >= 0)
    {
        status = fmDeleteACLRule(sw,
                                 switchExt->vnEncapAcl,
                                 addressMask->encapAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE
                                 | FM_ACL_COMPILE_FLAG_INTERNAL,
                                 &switchExt->vnEncapAcl);
        FM_LOG_DEBUG(FM_LOG_CAT_VN,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE
                               | FM_ACL_APPLY_FLAG_INTERNAL,
                               &switchExt->vnEncapAcl);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        status = FreeTunnelAclRuleNum(sw,
                                      encapTunnelGroup,
                                      ACL_RULE_TYPE_ADDR_MASK,
                                      addressMask->encapAclRule);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Delete the encap tunnel rule. */
    if (addressMask->encapTep != NULL)
    {
        status = DeleteEncapTepRule(sw, addressMask->encapTep);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Delete the decap acl rule. */
    if (addressMask->tunnel != NULL)
    {
        status = GetDecapAclRule(sw,
                                 addressMask->vn,
                                 addressMask->tunnel,
                                 &decapAclRule);
        if (status == FM_OK)
        {
            status = DeleteDecapAclRule(sw, decapAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
        }
        else if (status == FM_ERR_NOT_FOUND)
        {
            status = FM_OK;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    }

    /* Clean out any hash encap tunnel rules. */
    for (i = 0 ; i < numRemAddrs ; i++)
    {
        addrRec = remAddrList[i];

        if (addrRec->encapTunnelRule >= 0)
        {
            addrRec->encapTunnelGroup = addrRec->hashEncapGroup;
            addrRec->addrMask         = addressMask;

            status = DeleteEncapTunnelRule(sw, addrRec);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            addrRec->encapTunnelGroup = FM_VN_ENCAP_GROUP_DIRECT;
            addrRec->addrMask         = NULL;
        }
    }

    fmFree(addressMask);

ABORT:

    if (remAddrList != NULL)
    {
        free(remAddrList);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000DeleteVNRemoteAddressMask */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressList
 * \ingroup intVN
 *
 * \desc            Returns a list of remote addresses of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       maxAddresses is the size of addrList and tunnelIdList,
 *                  being the maximum number of remote addresses that can be
 *                  contained inside these arrays.
 *
 * \param[out]      numAddresses points to caller-provided storage into which
 *                  will be stored the number of remote addresses stored in
 *                  addrList and tunnelIdList.
 *
 * \param[out]      addrList is an array, maxAddresses elements in length,
 *                  that this function will fill with remote addresses.
 *
 * \param[out]      tunnelIdList is an array, maxAddresses elements in length,
 *                  that this function will fill with tunnel IDs for each
 *                  remote address returned in addrList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BUFFER_FULL if maxAddresses was too small
 *                  to accommodate the entire list of remote addresses.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressList(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_int             maxAddresses,
                                        fm_int *           numAddresses,
                                        fm_vnAddress *     addrList,
                                        fm_int *           tunnelIdList)
{
    fm_status                status;
    fm10000_virtualNetwork * vnExt;
    fm_customTreeIterator    iter;
    fm10000_vnRemoteAddress *addrKey;
    fm10000_vnRemoteAddress *addrRec;
	fm_int                   i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, maxAddresses = %d, numAddresses = %p, "
                  "addrList = %p, tunnelIdList = %p\n",
                  sw,
                  (void *) vn,
                  maxAddresses,
                  (void *) numAddresses,
                  (void *) addrList,
                  (void *) tunnelIdList );

    vnExt = vn->extension;
    i = 0;

    fmCustomTreeIterInit(&iter, &vnExt->remoteAddresses);

    while (1)
    {
        status = fmCustomTreeIterNext(&iter,
                                      (void **) &addrKey,
                                      (void **) &addrRec);

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (i >= maxAddresses)
        {
            status = FM_ERR_BUFFER_FULL;
            break;
        }

        tunnelIdList[i] = addrRec->tunnel->tunnelId;
        addrList[i]     = addrRec->remoteAddress;

        i++;
    }

ABORT:

    *numAddresses = i;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressList */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressFirst
 * \ingroup intVN
 *
 * \desc            Gets the first remote address of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to
 *                  ''fm10000GetVNRemoteAddressNext''.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the first remote address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID of the remote address will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no remote addresses.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressFirst(fm_int             sw,
                                         fm_virtualNetwork *vn,
                                         fm_voidptr *       searchToken,
                                         fm_vnAddress *     addr,
                                         fm_int *           tunnelId)
{
    fm_status                status;
    fm10000_virtualNetwork * vnExt;
    fm_customTreeIterator    iter;
    fm10000_vnRemoteAddress *addrKey;
    fm10000_vnRemoteAddress *addrRec;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, addr = %p, "
                  "tunnelId = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) addr,
                  (void *) tunnelId );

    vnExt = vn->extension;

    fmCustomTreeIterInit(&iter, &vnExt->remoteAddresses);

    status = fmCustomTreeIterNext(&iter,
                                  (void **) &addrKey,
                                  (void **) &addrRec);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    *tunnelId = addrRec->tunnel->tunnelId;
    *addr     = addrRec->remoteAddress;

    *searchToken = (fm_voidptr) addrKey;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressFirst */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressNext
 * \ingroup intVN
 *
 * \desc            Gets the next remote address of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fm10000GetVNRemoteAddressFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the next remote address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID of the remote address will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more remote addresses.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressNext(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_voidptr *       searchToken,
                                        fm_vnAddress *     addr,
                                        fm_int *           tunnelId)
{
    fm_status                status;
    fm10000_virtualNetwork * vnExt;
    fm_customTreeIterator    iter;
    fm10000_vnRemoteAddress *addrKey;
    fm10000_vnRemoteAddress *addrRec;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, addr = %p, "
                  "tunnelId = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) addr,
                  (void *) tunnelId );

    vnExt = vn->extension;

    addrKey = (fm10000_vnRemoteAddress *) *searchToken;

    status = fmCustomTreeIterInitFromKey(&iter,
                                         &vnExt->remoteAddresses,
                                         (void *) addrKey);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the previous record */
    status = fmCustomTreeIterNext(&iter,
                                  (void **) &addrKey,
                                  (void **) &addrRec);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the next record */
    status = fmCustomTreeIterNext(&iter,
                                  (void **) &addrKey,
                                  (void **) &addrRec);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    *tunnelId = addrRec->tunnel->tunnelId;
    *addr     = addrRec->remoteAddress;

    *searchToken = (fm_voidptr) addrKey;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressNext */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressMaskList
 * \ingroup intVN
 *
 * \desc            Returns a list of remote address masks of a virtual
 *                  network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       maxAddrMasks is the size of addrList, addrMaskList
 *                  and tunnelIdList, being the maximum number of remote
 *                  address masks that can be contained inside these arrays.
 *
 * \param[out]      numAddrMasks points to caller-provided storage into which
 *                  will be stored the number of remote address masks stored
 *                  in baseAddrList, addrMaskList and tunnelIdList.
 *
 * \param[out]      addrList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with addresses.
 *
 * \param[out]      addrMaskList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with bit-masks used with each address.
 *
 * \param[out]      tunnelIdList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with tunnel IDs for each
 *                  address mask returned in addrList and addrMaskList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BUFFER_FULL if maxAddrMasks was too small
 *                  to accommodate the entire list of remote address masks.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressMaskList(fm_int             sw,
                                            fm_virtualNetwork *vn,
                                            fm_int             maxAddrMasks,
                                            fm_int *           numAddrMasks,
                                            fm_vnAddress *     addrList,
                                            fm_vnAddress *     addrMaskList,
                                            fm_int *           tunnelIdList)
{
    fm_status                    status;
    fm10000_virtualNetwork *     vnExt;
    fm_customTreeIterator        iter;
    fm10000_vnRemoteAddressMask *ruleKey;
    fm10000_vnRemoteAddressMask *addressMask;
	fm_int                       i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, maxAddrMasks = %d, numAddrMasks = %p, "
                  "addrList = %p, addrMaskList = %p, tunnelIdList = %p\n",
                  sw,
                  (void *) vn,
                  maxAddrMasks,
                  (void *) numAddrMasks,
                  (void *) addrList,
                  (void *) addrMaskList,
                  (void *) tunnelIdList );

    vnExt = vn->extension;
    i = 0;

    fmCustomTreeIterInit(&iter, &vnExt->addressMasks);

    while (1)
    {
        status = fmCustomTreeIterNext(&iter,
                                      (void **) &ruleKey,
                                      (void **) &addressMask);

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (i >= maxAddrMasks)
        {
            status = FM_ERR_BUFFER_FULL;
            break;
        }

        if (addressMask->tunnel == NULL)
        {
            tunnelIdList[i] = -1;
        }
        else
        {
            tunnelIdList[i] = addressMask->tunnel->tunnelId;
        }
        addrList[i]     = addressMask->remoteAddress;
        addrMaskList[i] = addressMask->addrMask;

        i++;
    }

ABORT:

    *numAddrMasks = i;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressMaskList */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressMaskFirst
 * \ingroup intVN
 *
 * \desc            Gets the first remote address mask of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to
 *                  ''fm10000GetVNRemoteAddressMaskNext''.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the first address.
 *
 * \param[out]      addrMask points to caller-provided storage into which the
 *                  function will write the bit-mask used with the address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID for the address mask will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no remote address masks.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressMaskFirst(fm_int             sw,
                                             fm_virtualNetwork *vn,
                                             fm_voidptr *       searchToken,
                                             fm_vnAddress *     addr,
                                             fm_vnAddress *     addrMask,
                                             fm_int *           tunnelId)
{
    fm_status                    status;
    fm10000_virtualNetwork *     vnExt;
    fm_customTreeIterator        iter;
    fm10000_vnRemoteAddressMask *ruleKey;
    fm10000_vnRemoteAddressMask *addressMask;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, addr = %p, "
                  "addrMask = %p, tunnelId = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) addr,
                  (void *) addrMask,
                  (void *) tunnelId );

    vnExt = vn->extension;

    fmCustomTreeIterInit(&iter, &vnExt->addressMasks);

        status = fmCustomTreeIterNext(&iter,
                                      (void **) &ruleKey,
                                      (void **) &addressMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (addressMask->tunnel == NULL)
    {
        *tunnelId = -1;
    }
    else
    {
        *tunnelId = addressMask->tunnel->tunnelId;
    }
    *addr     = addressMask->remoteAddress;
    *addrMask = addressMask->addrMask;

    *searchToken = (fm_voidptr) ruleKey;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressMaskFirst */




/*****************************************************************************/
/** fm10000GetVNRemoteAddressMaskNext
 * \ingroup intVN
 *
 * \desc            Gets the next remote address mask of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fm10000GetVNRemoteAddressMaskFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the next base address.
 *
 * \param[out]      addrMask points to caller-provided storage into which the
 *                  function will write the bit-mask used with the address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID for the address mask will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more remote address masks.
 *
 *****************************************************************************/
fm_status fm10000GetVNRemoteAddressMaskNext(fm_int             sw,
                                            fm_virtualNetwork *vn,
                                            fm_voidptr *       searchToken,
                                            fm_vnAddress *     addr,
                                            fm_vnAddress *     addrMask,
                                            fm_int *           tunnelId)
{
    fm_status                    status;
    fm10000_virtualNetwork *     vnExt;
    fm_customTreeIterator        iter;
    fm10000_vnRemoteAddressMask *ruleKey;
    fm10000_vnRemoteAddressMask *addressMask;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, addr = %p, "
                  "addrMask = %p, tunnelId = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) addr,
                  (void *) addrMask,
                  (void *) tunnelId );

    vnExt = vn->extension;

    ruleKey = (fm10000_vnRemoteAddressMask *) *searchToken;

    status = fmCustomTreeIterInitFromKey(&iter,
                                         &vnExt->addressMasks,
                                         (void *) ruleKey);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the previous mask */
    status = fmCustomTreeIterNext(&iter,
                                  (void **) &ruleKey,
                                  (void **) &addressMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the next mask */
    status = fmCustomTreeIterNext(&iter,
                                  (void **) &ruleKey,
                                  (void **) &addressMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (addressMask->tunnel == NULL)
    {
        *tunnelId = -1;
    }
    else
    {
        *tunnelId = addressMask->tunnel->tunnelId;
    }
    *addr     = addressMask->remoteAddress;
    *addrMask = addressMask->addrMask;

    *searchToken = (fm_voidptr) ruleKey;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNRemoteAddressMaskNext */




/*****************************************************************************/
/** fm10000ConfigureVN
 * \ingroup intVN
 *
 * \desc            Configures the Virtual Networking API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       config points to the configuration record to be used.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConfigureVN(fm_int sw, fm_vnConfiguration *config)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_parserDiCfg          parserDiCfg;
    fm_fm10000TeChecksumCfg chksumCfg;
    fm_fm10000TeTunnelCfg   tunnelCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d, config = %p\n", sw, (void *) config);

    switchExt = GET_SWITCH_EXT(sw);
    status    = FM_OK;

    FM_CLEAR(parserDiCfg);

    if (config->deepInspectionCfgIndex >= 0)
    {
        parserDiCfg.index                             = config->deepInspectionCfgIndex;
        parserDiCfg.parserDiCfgFields.protocol        = 0;
        parserDiCfg.parserDiCfgFields.l4Port          = 0;
        parserDiCfg.parserDiCfgFields.l4Compare       = FALSE;
        parserDiCfg.parserDiCfgFields.captureTcpFlags = FALSE;
        parserDiCfg.parserDiCfgFields.enable          = TRUE;
        parserDiCfg.parserDiCfgFields.wordOffset      = 0x76543210;

        status = fm10000SetSwitchAttribute(sw,
                                           FM_SWITCH_PARSER_DI_CFG,
                                           &parserDiCfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }
    switchExt->vnDeepInspectionCfgIndex = config->deepInspectionCfgIndex;

    FM_CLEAR(chksumCfg);

    chksumCfg.notIp       = TranslateChecksumAction(config->nonIP);
    chksumCfg.notTcpOrUdp = TranslateChecksumAction(config->nonTcpUdp);
    chksumCfg.tcpOrUdp    = TranslateChecksumAction(config->tcpOrUdp);

    status = fm10000SetTeChecksum(sw,
                                  ENCAP_TUNNEL,
                                  &chksumCfg,
                                  FM10000_TE_CHECKSUM_NOT_IP
                                  | FM10000_TE_CHECKSUM_NOT_TCP_OR_UDP
                                  | FM10000_TE_CHECKSUM_TCP_OR_UDP,
                                  TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_CLEAR(chksumCfg);

    chksumCfg.verifDecapChecksum = config->outerChecksumValidation;

    status = fm10000SetTeChecksum(sw,
                                  ENCAP_TUNNEL,
                                  &chksumCfg,
                                  FM10000_TE_CHECKSUM_DECAP_VALID,
                                  TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_CLEAR(tunnelCfg);

    switchExt->vnOuterTTL = config->outerTTL;
    tunnelCfg.ttl         = config->outerTTL;

    status = fm10000SetTeDefaultTunnel(sw,
                                       ENCAP_TUNNEL,
                                       &tunnelCfg,
                                       FM10000_TE_DEFAULT_TUNNEL_TTL,
                                       TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000ConfigureVN */




/*****************************************************************************/
/** fm10000GetVNConfiguration
 * \ingroup intVN
 *
 * \desc            Retrieves the Virtual Networking API configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      config points to caller-provided storage into which
 *                  the Virtual Networking API configuration will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetVNConfiguration(fm_int sw, fm_vnConfiguration *config)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_fm10000TeChecksumCfg chksumCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d, config = %p\n", sw, (void *) config);

    switchExt = GET_SWITCH_EXT(sw);
	config->outerTTL = switchExt->vnOuterTTL;
    config->deepInspectionCfgIndex = switchExt->vnDeepInspectionCfgIndex;

    FM_CLEAR(chksumCfg);

    status = fm10000GetTeChecksum(sw, ENCAP_TUNNEL, &chksumCfg, FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    config->nonIP     = TranslateTeChecksumAction(chksumCfg.notIp);
    config->nonTcpUdp = TranslateTeChecksumAction(chksumCfg.notTcpOrUdp);
    config->tcpOrUdp  = TranslateTeChecksumAction(chksumCfg.tcpOrUdp);

    config->outerChecksumValidation = chksumCfg.verifDecapChecksum;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNConfiguration */




/*****************************************************************************/
/** fm10000AddVNDirectTunnelRule
 * \ingroup intVN
 *
 * \desc            Adds a direct encapsulation tunnel rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel ID.
 *
 * \param[in]       vni is the virtual network VNI.
 *
 * \param[out]      portNum points to caller-provided storage into which
 *                  the tunnel engine port number will be written.
 *
 * \param[out]      dglort points to caller-provided storage into which
 *                  the destination glort for the direct tunnel rule will
 *                  be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddVNDirectTunnelRule(fm_int     sw,
                                       fm_int     tunnelId,
                                       fm_uint32  vni,
                                       fm_int *   portNum,
                                       fm_uint32 *dglort)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_virtualNetwork *     vn;
    fm_vnTunnel *           tunnel;
    fm10000_virtualNetwork *vnExt;
    fm10000_vnTunnel *      tunnelExt;
    fm10000_vnEncapTep *    tepRule;
    fm_tunnelGlortUser      glortUser;
    fm_int                  flowType;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelId = %d, vni = %u, portNum = %p, dglort = %p\n",
                 sw,
                 tunnelId,
                 vni,
                 (void *) portNum,
                 (void *) dglort);

    switchExt = GET_SWITCH_EXT(sw);

    /* Get the tunnel record */
    tunnel = fmGetVNTunnel(sw, tunnelId);
    if (tunnel == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get the virtual network record */
    vn = fmGetVN(sw, vni);
    if (vn == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    vnExt     = vn->extension;
    tunnelExt = tunnel->extension;
    flowType  = GET_ENCAP_FLOW_TYPE(vn);

    /* Get the encap TEP for this tunnel/vni */
    status = GetEncapTepRule(sw, vn, tunnel, TRUE, &tepRule);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (tepRule == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    if (status == FM_OK)
    {
        if (portNum != NULL)
        {
            *portNum = switchExt->tunnelCfg->tunnelPort[ENCAP_TUNNEL];
        }

        if (dglort != NULL)
        {
            status = fm10000GetTunnelAttribute(sw,
                                               switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                               tepRule->encapTunnelRule,
                                               FM_TUNNEL_GLORT_USER,
                                               &glortUser);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            *dglort = glortUser.glort;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000AddVNDirectTunnelRule */




/*****************************************************************************/
/** fm10000DeleteVNDirectTunnelRule
 * \ingroup intVN
 *
 * \desc            Deletes a direct encapsulation tunnel rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel ID.
 *
 * \param[in]       vni is the virtual network VNI.
 *
 * \param[out]      portNum points to caller-provided storage into which
 *                  the tunnel engine port number will be written.
 *
 * \param[out]      dglort points to caller-provided storage into which
 *                  the destination glort for the direct tunnel rule will
 *                  be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNDirectTunnelRule(fm_int     sw,
                                          fm_int     tunnelId,
                                          fm_uint32  vni,
                                          fm_int *   portNum,
                                          fm_uint32 *dglort)
{
    fm_status           status;
    fm10000_switch *    switchExt;
    fm_virtualNetwork * vn;
    fm_vnTunnel *       tunnel;
    fm10000_vnEncapTep *tepRule;
    fm_tunnelGlortUser  glortUser;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, tunnelId = %d, vni = %u, portNum = %p, dglort = %p\n",
                 sw,
                 tunnelId,
                 vni,
                 (void *) portNum,
                 (void *) dglort);

    switchExt = GET_SWITCH_EXT(sw);

    /* Get the tunnel record */
    tunnel = fmGetVNTunnel(sw, tunnelId);
    if (tunnel == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get the virtual network record */
    vn = fmGetVN(sw, vni);
    if (vn == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Get the encap TEP for this tunnel/vni */
    status = GetEncapTepRule(sw, vn, tunnel, FALSE, &tepRule);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (tepRule == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_FAIL);
    }

    if (portNum != NULL)
    {
        *portNum = switchExt->tunnelCfg->tunnelPort[ENCAP_TUNNEL];
    }

    if (dglort != NULL)
    {
        status = fm10000GetTunnelAttribute(sw,
                                           switchExt->vnTunnelGroups[FM_VN_ENCAP_GROUP_DIRECT],
                                           tepRule->encapTunnelRule,
                                           FM_TUNNEL_GLORT_USER,
                                           &glortUser);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        *dglort = glortUser.glort;
    }

    status = DeleteEncapTepRule(sw, tepRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DeleteVNDirectTunnelRule */




/*****************************************************************************/
/** fm10000AddVNLocalPort
 * \ingroup intVN
 *
 * \desc            Adds a local port listener to a virtual network's
 *                  broadcast/flooding floodset listener list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       port is the local port number for the ethernet port or
 *                  VM.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddVNLocalPort(fm_int             sw,
                                fm_virtualNetwork *vn,
                                fm_int             port)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm_mcastGroupListener   listener;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, port = %d\n",
                 sw,
                 (void *) vn,
                 port);

    vnExt = vn->extension;

    FM_CLEAR(listener);

    listener.listenerType               = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
    listener.info.portVlanListener.port = port;
    listener.info.portVlanListener.vlan = vn->descriptor.vlan;

    status = fmAddMcastGroupListenerInternal(sw,
                                             vnExt->floodsetMcastGroup,
                                             &listener);
    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000AddVNLocalPort */




/*****************************************************************************/
/** fm10000DeleteVNLocalPort
 * \ingroup intVN
 *
 * \desc            Deletes a local port listener from a virtual network's
 *                  broadcast/flooding floodset listener list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       port is the local port number for the ethernet port or
 *                  VM.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNLocalPort(fm_int             sw,
                                   fm_virtualNetwork *vn,
                                   fm_int             port)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm_mcastGroupListener   listener;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p, port = %d\n",
                 sw,
                 (void *) vn,
                 port);

    vnExt = vn->extension;

    FM_CLEAR(listener);

    listener.listenerType               = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
    listener.info.portVlanListener.port = port;
    listener.info.portVlanListener.vlan = vn->descriptor.vlan;

    status = fmDeleteMcastGroupListenerInternal(sw,
                                                vnExt->floodsetMcastGroup,
                                                &listener);
    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DeleteVNLocalPort */




/*****************************************************************************/
/** fm10000GetVNLocalPortList
 * \ingroup intVN
 *
 * \desc            Returns a list of local port listeners in a virtual
 *                  network's broadcast/flooding floodset listener list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       maxPorts is the size of portList, being the maximum number
 *                  of local ports that can be contained inside the array.
 *
 * \param[out]      numPorts points to caller-provided storage into which
 *                  will be stored the number of local ports stored
 *                  in portList.
 *
 * \param[out]      portList is an array, maxPorts elements in length,
 *                  that this function will fill with logical port numbers
 *                  for each local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BUFFER_FULL if maxPorts was too small to accommodate
 *                  the entire list of local ports.
 *
 *****************************************************************************/
fm_status fm10000GetVNLocalPortList(fm_int             sw,
                                    fm_virtualNetwork *vn,
                                    fm_int             maxPorts,
                                    fm_int *           numPorts,
                                    fm_int *           portList)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm_mcastGroupListener   listener;
    fm_mcastGroupListener   prevListener;
	fm_int                  i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, maxPorts = %d, numPorts = %p, "
                  "portList = %p\n",
                  sw,
                  (void *) vn,
                  maxPorts,
                  (void *) numPorts,
                  (void *) portList );

    vnExt = vn->extension;
    i = 0;

    status = fmGetMcastGroupListenerFirstV2(sw,
                                            vnExt->floodsetMcastGroup,
                                            &listener);
    while (status == FM_OK)
    {
        if (listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
			if (i >= maxPorts)
			{
                status = FM_ERR_BUFFER_FULL;
                break;
			}

            portList[i++] = listener.info.portVlanListener.port;
        }

        prevListener = listener;
        status = fmGetMcastGroupListenerNextV2(sw,
                                               vnExt->floodsetMcastGroup,
                                               &prevListener,
                                               &listener);
    }

    *numPorts = i;

    if (status == FM_ERR_NO_MORE)
    {
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNLocalPortList */




/*****************************************************************************/
/** fm10000GetVNLocalPortFirst
 * \ingroup intVN
 *
 * \desc            Gets the first local port listener in a virtual network's
 *                  broadcast/flooding floodset listener list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_mcastGroupListener, where this function will store
 *                  a token to be used in a subsequent call to
 *                  ''fm10000GetVNLocalPortNext''.
 *
 * \param[out]      port points to caller-provided storage into which the
 *                  function will write the logical port number of the first
 *                  local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no local ports.
 *
 *****************************************************************************/
fm_status fm10000GetVNLocalPortFirst(fm_int                 sw,
                                     fm_virtualNetwork *    vn,
                                     fm_mcastGroupListener *searchToken,
                                     fm_int *               port)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm_mcastGroupListener   listener;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, port = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) port );

    vnExt = vn->extension;

    status = fmGetMcastGroupListenerFirstV2(sw,
                                            vnExt->floodsetMcastGroup,
                                            &listener);
    while (status == FM_OK)
    {
        *searchToken = listener;

        if (listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
            *port = listener.info.portVlanListener.port;
			break;
        }

        status = fmGetMcastGroupListenerNextV2(sw,
                                               vnExt->floodsetMcastGroup,
                                               searchToken,
                                               &listener);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNLocalPortFirst */




/*****************************************************************************/
/** fm10000GetVNLocalPortNext
 * \ingroup intVN
 *
 * \desc            Gets the next local port listener in a virtual network's
 *                  broadcast/flooding floodset listener list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_mcastGroupListener that has been filled in by a prior
 *                  call to this function or to ''fm10000GetVNLocalPortFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      port points to caller-provided storage into which the
 *                  function will write the logical port number of the next
 *                  local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more local ports.
 *
 *****************************************************************************/
fm_status fm10000GetVNLocalPortNext(fm_int                 sw,
                                    fm_virtualNetwork *    vn,
                                    fm_mcastGroupListener *searchToken,
                                    fm_int *               port)
{
    fm_status               status;
    fm10000_virtualNetwork *vnExt;
    fm_mcastGroupListener   listener;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vn = %p, searchToken = %p, port = %p\n",
                  sw,
                  (void *) vn,
                  (void *) searchToken,
                  (void *) port );

    vnExt = vn->extension;

    while (1)
    {
        status = fmGetMcastGroupListenerNextV2(sw,
                                               vnExt->floodsetMcastGroup,
                                               searchToken,
                                               &listener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        *searchToken = listener;

        if (listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
            *port = listener.info.portVlanListener.port;
			break;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNLocalPortNext */




/*****************************************************************************/
/** fm10000AddVNVsi
 * \ingroup intVN
 *
 * \desc            Adds a VSI to a Virtual Network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       vsi is the VSI value that should be added to the virtual network.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ALREADY_EXISTS if the VSI is already in use.
 * \return          FM_ERR_UNSUPPORTED if the VN is not in vswitch-offload mode.
 *
 *****************************************************************************/
fm_status fm10000AddVNVsi(fm_int             sw,
                          fm_virtualNetwork *vn,
                          fm_int             vsi)
{
    fm_status                status;
    fm10000_switch *         switchExt;
    fm10000_virtualNetwork * vnExt;
    fm10000_vnRemoteAddress *curAddrKey;
    fm10000_vnRemoteAddress *curAddrRec;
    fm_customTreeIterator    addrIter;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p (%d), vsi = %d\n",
                 sw,
                 (void *) vn,
                 vn->vsId,
                 vsi);

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    if ( (vn->descriptor.mode != FM_VN_MODE_VSWITCH_OFFLOAD)
         && (vnExt->primaryVsi >= 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_UNSUPPORTED);
    }

    if ( (vsi < 0) || (vsi >= FM10000_TE_VNI_ENTRIES_0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Make sure VSI is not already in use. */
    if ( (switchExt->vnVsi[vsi] != NULL) && (switchExt->vnVsi[vsi] != vn) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_ALREADY_EXISTS);
    }

    /* Assign VSI to this VN. */
    switchExt->vnVsi[vsi] = vn;

    if (vnExt->primaryVsi < 0)
    {
        vnExt->primaryVsi = vsi;
    }

    /* Configure the VSI registers. */
    status = WriteVsi(sw, vn, vsi);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Add encap tunnel rules for this VSI. */
    fmCustomTreeIterInit(&addrIter, &vnExt->remoteAddresses);
    status = fmCustomTreeIterNext(&addrIter,
                                  (void **) &curAddrKey,
                                  (void **) &curAddrRec);
    if (status == FM_ERR_NO_MORE)
    {
        curAddrRec = NULL;
        status     = FM_OK;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    while (curAddrRec != NULL)
    {
        status = WriteVsiEncapTunnelRule(sw, curAddrRec, vsi);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = fmCustomTreeIterNext(&addrIter,
                                      (void **) &curAddrKey,
                                      (void **) &curAddrRec);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000AddVNVsi */




/*****************************************************************************/
/** fm10000DeleteVNVsi
 * \ingroup intVN
 *
 * \desc            Deletes a VSI from a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vn is the Virtual Network record.
 *
 * \param[in]       vsi is the VSI value that should be deleted from the
 *                  virtual network.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if the VSI was not assigned to this VN.
 *
 *****************************************************************************/
fm_status fm10000DeleteVNVsi(fm_int             sw,
                             fm_virtualNetwork *vn,
                             fm_int             vsi)
{
    fm_status                status;
    fm10000_switch *         switchExt;
    fm10000_virtualNetwork * vnExt;
    fm10000_vnRemoteAddress *curAddrKey;
    fm10000_vnRemoteAddress *curAddr;
    fm_customTreeIterator    addrIter;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,
                 "sw = %d, vn = %p (%d), vsi = %d\n",
                 sw,
                 (void *) vn,
                 vn->vsId,
                 vsi);

    switchExt = GET_SWITCH_EXT(sw);
    vnExt     = vn->extension;

    if ( (vn->descriptor.mode != FM_VN_MODE_VSWITCH_OFFLOAD)
         && (vnExt->primaryVsi != vsi) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_UNSUPPORTED);
    }

    if ( (vsi < 0) || (vsi >= FM10000_TE_VNI_ENTRIES_0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    /* Make sure VSI is attached to this VN. */
    if ( (switchExt->vnVsi[vsi] == NULL) || (switchExt->vnVsi[vsi] != vn) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NOT_FOUND);
    }

    /* Delete encap tunnel rules for this VSI. */
    fmCustomTreeIterInit(&addrIter, &vnExt->remoteAddresses);
    status = fmCustomTreeIterNext(&addrIter,
                                  (void **) &curAddrKey,
                                  (void **) &curAddr);
    if (status == FM_ERR_NO_MORE)
    {
        curAddr = NULL;
        status     = FM_OK;
    }

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    while (curAddr != NULL)
    {
        if (curAddr->encapTunnelRules[vsi] >= 0)
        {
            status = fm10000DeleteTunnelRule(sw,
                                             switchExt->vnTunnelGroups[curAddr->encapTunnelGroup],
                                             curAddr->encapTunnelRules[vsi]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            status = FreeTunnelRuleNum(sw,
                                       switchExt->vnTunnelGroups[curAddr->encapTunnelGroup],
                                       curAddr->encapTunnelRules[vsi]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

            curAddr->encapTunnelRules[vsi] = -1;
        }

        status = fmCustomTreeIterNext(&addrIter,
                                      (void **) &curAddrKey,
                                      (void **) &curAddr);
        if (status == FM_ERR_NO_MORE)
        {
            status     = FM_OK;
            curAddr = NULL;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Remove VSI from this VN. */
    switchExt->vnVsi[vsi] = NULL;

    if (vnExt->primaryVsi == vsi)
    {
        vnExt->primaryVsi = -1;
    }

    /* Unconfigure the VSI registers. */
    status = WriteVsi(sw, NULL, vsi);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DeleteVNVsi */




/*****************************************************************************/
/** fm10000GetVNVsiList
 * \ingroup intVN
 *
 * \desc            Returns a list of VSIs in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       maxVsis is the size of vsiList, being the maximum number
 *                  of VSI numbers that can be contained inside the array.
 *
 * \param[out]      numVsis points to caller-provided storage into which
 *                  will be stored the number of VSIs stored in vsiList.
 *
 * \param[out]      vsiList is an array, maxVsis elements in length,
 *                  that this function will fill with VSI numbers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BUFFER_FULL if maxVsis was too small to accommodate
 *                  the entire list of VSIs.
 *
 *****************************************************************************/
fm_status fm10000GetVNVsiList(fm_int    sw,
                              fm_uint32 vni,
                              fm_int    maxVsis,
                              fm_int *  numVsis,
                              fm_int *  vsiList)
{
    fm_status       status;
    fm10000_switch *switchExt;
	fm_int          vsi;
    fm_int          i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vni = %u, maxVsis = %d, numVsis = %p, vsiList = %p\n",
                  sw, vni, maxVsis, (void *) numVsis, (void *) vsiList );

    status = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);
	i = 0;

    for (vsi = 0; vsi < FM10000_TE_VNI_ENTRIES_0; vsi++)
    {
        if ( (switchExt->vnVsi[vsi] != NULL)
             && (switchExt->vnVsi[vsi]->vsId == vni) )
        {
			if (i >= maxVsis)
			{
                status = FM_ERR_BUFFER_FULL;
                break;
			}

            vsiList[i++] = vsi;
        }
    }

    *numVsis = i;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNVsiList */




/*****************************************************************************/
/** fm10000GetVNVsiFirst
 * \ingroup intVN
 *
 * \desc            Gets the first VSI in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[out]      searchToken points to caller-allocated storage where this
 *                  function will store a token to be used in a subsequent
 *                  call to ''fm10000GetVNVsiNext''.
 *
 * \param[out]      vsi points to caller-provided storage into which the
 *                  function will write the first VSI number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no VSIs.
 *
 *****************************************************************************/
fm_status fm10000GetVNVsiFirst(fm_int    sw,
                               fm_uint32 vni,
                               fm_int *  searchToken,
                               fm_int *  vsi)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_int          i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vni = %u, searchToken = %p, vsi = %p\n",
                  sw, vni, (void *) searchToken, (void *) vsi );

    switchExt = GET_SWITCH_EXT(sw);

    status = FM_ERR_NO_MORE;
    for (i = 0; i < FM10000_TE_VNI_ENTRIES_0; i++)
    {
        if ( (switchExt->vnVsi[i] != NULL)
             && (switchExt->vnVsi[i]->vsId == vni) )
        {
            *vsi = i;
            status = FM_OK;
			break;
        }
    }

    *searchToken = i;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNVsiFirst */




/*****************************************************************************/
/** fm10000GetVNVsiNext
 * \ingroup intVN
 *
 * \desc            Gets the next VSI in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in,out]   searchToken points to caller-allocated storage that has
 *                  been filled in by a prior call to this function or to
 *                  ''fm10000GetVNVsiFirst''. It will be updated by this
 *                  function with a new value to be used in a subsequent call
 *                  to this function.
 *
 * \param[out]      vsi points to caller-provided storage into which the
 *                  function will write the next VSI number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more VSIs.
 *
 *****************************************************************************/
fm_status fm10000GetVNVsiNext(fm_int    sw,
                              fm_uint32 vni,
                              fm_int *  searchToken,
                              fm_int *  vsi)
{
    fm_status       status;
    fm10000_switch *switchExt;
    fm_int          i;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, vni = %u, searchToken = %p, vsi = %p\n",
                  sw, vni, (void *) searchToken, (void *) vsi );

    switchExt = GET_SWITCH_EXT(sw);

    if ((*searchToken < 0) || (*searchToken >= FM10000_TE_VNI_ENTRIES_0))
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NOT_FOUND);
    }

    status = FM_ERR_NO_MORE;

	if (*searchToken == (FM10000_TE_VNI_ENTRIES_0 - 1))
	{
        FM_LOG_EXIT(FM_LOG_CAT_VN, status);
	}

    for (i = (*searchToken + 1); i < FM10000_TE_VNI_ENTRIES_0; i++)
    {
        if ( (switchExt->vnVsi[i] != NULL)
             && (switchExt->vnVsi[i]->vsId == vni) )
        {
            *vsi = i;
            status = FM_OK;
			break;
        }
    }

    *searchToken = i;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000GetVNVsiNext */




/*****************************************************************************/
/** fm10000IsVNTunnelInUseByACLs
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
fm_status fm10000IsVNTunnelInUseByACLs(fm_int   sw,
                                       fm_int   tunnelId,
                                       fm_bool *inUse)
{
    fm_vnTunnel *     tunnel;
    fm10000_vnTunnel *tunnelExt;
    fm_int            i;
    fm_int            j;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnelId = %d, inUse = %p\n",
                  sw,
                  tunnelId,
                  (void *) inUse );

    tunnel = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_INVALID_ARGUMENT);
    }

    tunnelExt = tunnel->extension;

    for (i = 0 ; i < FM_VN_NUM_ENCAP_TUNNEL_GROUPS ; i++)
    {
        for (j = 0 ; j < FM_VN_NUM_ENCAP_FLOW_TYPES ; j++)
        {
            if (tunnelExt->ruleCounts[i][j] != 0)
            {
                *inUse = TRUE;
                FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
            }
        }
    }

    *inUse    = FALSE;
    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000IsVNTunnelInUseByACLs */




/*****************************************************************************/
/** fm10000FreeVirtualNetwork
 * \ingroup intVN
 *
 * \desc            Releases resources assigned to a single VN.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vn points to the virtual network record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeVirtualNetwork(fm_int sw, fm_virtualNetwork *vn)
{
    fm10000_virtualNetwork *vnExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d, vn = %p\n", sw, (void *) vn);

    FM_NOT_USED(sw);

    vnExt = vn->extension;

    if (vnExt != NULL)
    {
        if ( fmCustomTreeIsInitialized(&vnExt->remoteAddresses) )
        {
            fmCustomTreeDestroy(&vnExt->remoteAddresses, FreeRemoteAddressRecord);
        }

        if ( fmCustomTreeIsInitialized(&vnExt->addressMasks) )
        {
            fmCustomTreeDestroy(&vnExt->addressMasks, FreeCustomTreeRecord);
        }

        if ( fmTreeIsInitialized(&vnExt->tunnels) )
        {
            fmTreeDestroy(&vnExt->tunnels, NULL);
        }

        fmFree(vnExt);
        vn->extension = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000FreeVirtualNetwork */




/*****************************************************************************/
/** fm10000FreeVNTunnel
 * \ingroup intVN
 *
 * \desc            Releases resources assigned to a single VN Tunnel.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tunnel points to the VN Tunnel record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeVNTunnel(fm_int sw, fm_vnTunnel *tunnel)
{
    fm10000_vnTunnel *tunnelExt;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d, tunnel = %p\n", sw, (void *) tunnel);

    FM_NOT_USED(sw);

    tunnelExt = tunnel->extension;

    if (tunnelExt != NULL)
    {
        fmFree(tunnelExt);
        tunnel->extension = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000FreeVNTunnel */




/*****************************************************************************/
/** fm10000FreeVNResources
 * \ingroup intVN
 *
 * \desc            Releases all VN resources. Called when shutting down the
 *                  switch. Assumes that no hardware resources need be released.
 *                  Only software resources will be cleaned up.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeVNResources(fm_int sw)
{
    fm10000_switch *switchExt;
    fm_int          i;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    /* Delete per-tunnel-group bit arrays. */
    for (i = 0 ; i < FM_VN_NUM_TUNNEL_GROUPS ; i++)
    {
        fmDeleteBitArray(&switchExt->vnTunnelRuleIds[i]);
        fmDeleteBitArray(&switchExt->vnTunnelActiveEncapFlowIds[i]);
    }

    /* Delete remaining bit arrays. */
    fmDeleteBitArray(&switchExt->vnEncapAclRuleNumbers);
    fmDeleteBitArray(&switchExt->vnEncapAclFloodsetRuleNumbers);
    fmDeleteBitArray(&switchExt->vnDecapAclRuleNumbers);

    /* Destroy the decap ACL rule tree. */
    if ( fmCustomTreeIsInitialized(&switchExt->vnDecapAclRuleTree) )
    {
        fmCustomTreeDestroy(&switchExt->vnDecapAclRuleTree, FreeCustomTreeRecord);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fm10000FreeVNResources */




/*****************************************************************************/
/** fm10000DbgDumpVN
 * \ingroup intDebug
 *
 * \desc            Display the virtual networks status of a switch
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpVN(fm_int sw)
{
    fm_status               status;
    fm10000_switch *        switchExt;
    fm_customTreeIterator   iter;
    fm10000_vnDecapAclRule *aclRuleKey;
    fm10000_vnDecapAclRule *aclRule;
    fm_char                 tempString[100];
    fm_int                  cnt;
    fm_int                  i;

    switchExt = GET_SWITCH_EXT(sw);

    FM_LOG_PRINT("vnVxlanUdpPort:        %u\n", switchExt->vnVxlanUdpPort);
    FM_LOG_PRINT("vnGeneveUdpPort:       %u\n", switchExt->vnGeneveUdpPort);
    FM_LOG_PRINT("useSharedEncapFlows:   %s\n",
                 (switchExt->useSharedEncapFlows == TRUE) ? "TRUE" : "FALSE");
    FM_LOG_PRINT("maxVNRemoteAddresses:  %d\n",
                 switchExt->maxVNRemoteAddresses);
    FM_LOG_PRINT("vnTunnelGroupHashSize: %d\n",
                 switchExt->vnTunnelGroupHashSize);
    FM_LOG_PRINT("vnTeVid:               %d\n", switchExt->vnTeVid);
    FM_LOG_PRINT("vnEncapProtocol:       0x%x\n", switchExt->vnEncapProtocol);
    FM_LOG_PRINT("vnEncapVersion:        %d\n", switchExt->vnEncapVersion);
    FM_LOG_PRINT("vnEncapAcl:            %d\n", switchExt->vnEncapAcl);
    FM_LOG_PRINT("vnDecapAcl:            %d\n", switchExt->vnDecapAcl);
    FM_LOG_PRINT("numVirtualNetworks:    %d\n", switchExt->numVirtualNetworks);
    FM_LOG_PRINT("numVNTunnels:          %d\n", switchExt->numVNTunnels);
    FM_LOG_PRINT("vnOuterTTL:            %d\n", switchExt->vnOuterTTL);

    FM_LOG_PRINT("\nvnTunnelGroups:\n");
    for (i = 0; i < FM_VN_NUM_TUNNEL_GROUPS; i++)
    {
        ConvertTunnelGroupToString(i, sizeof(tempString), tempString);
        FM_LOG_PRINT("    tunnel group %s:\tID=%d\n",
                     tempString, switchExt->vnTunnelGroups[i]);
    }

    FM_LOG_PRINT("\nvnTunnelRuleIds:\n\n");
    for (i = 0; i < FM_VN_NUM_TUNNEL_GROUPS; i++)
    {
        ConvertTunnelGroupToString(i, sizeof(tempString), tempString);
        FM_LOG_PRINT("    tunnel group %s:\n\n", tempString);
        PrintBitArray(&switchExt->vnTunnelRuleIds[i]);
    }

    FM_LOG_PRINT("\nvnTunnelActiveEncapFlowIds:\n\n");
    for (i = 0; i < FM_VN_NUM_TUNNEL_GROUPS; i++)
    {
        ConvertTunnelGroupToString(i, sizeof(tempString), tempString);
        FM_LOG_PRINT("    tunnel group %s:\n\n", tempString);
        PrintBitArray(&switchExt->vnTunnelActiveEncapFlowIds[i]);
    }

    FM_LOG_PRINT("\nvnEncapAclRuleNumbers:\n\n");
    PrintBitArray(&switchExt->vnEncapAclRuleNumbers);

    FM_LOG_PRINT("\nEncapAclFloodsetRuleNumbers:\n\n");
    PrintBitArray(&switchExt->vnEncapAclFloodsetRuleNumbers);

    FM_LOG_PRINT("\nDecapAclRuleNumbers:\n\n");
    PrintBitArray(&switchExt->vnDecapAclRuleNumbers);

    FM_LOG_PRINT("\nDecap ACL Rules:\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n\n");

    fmCustomTreeIterInit(&iter, &switchExt->vnDecapAclRuleTree);
    cnt = 0;

    while (1)
    {
        status = fmCustomTreeIterNext(&iter,
                                      (void **) &aclRuleKey,
                                      (void **) &aclRule);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        FM_LOG_PRINT("    aclRule: %d, useCount: %d\n",
                     aclRule->aclRule, aclRule->useCount);
        ConvertTunnelGroupToString(aclRule->decapTunnelGroup,
                                   sizeof(tempString),
                                   tempString);
        FM_LOG_PRINT("        decapTunnelGroup: %s\n", tempString);
        FM_LOG_PRINT("        vn: %p (vsId: %d)\n",
                     (void *) aclRule->vn, aclRule->vn->vsId);
        FM_LOG_PRINT("        tunnel: %p (tunnelId: %d)\n",
                     (void *) aclRule->tunnel, aclRule->tunnel->tunnelId);
		cnt++;
    }
    if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    FM_LOG_PRINT("\nVSI numbers in use:\n");
    for (i = 0, cnt = 0; i < FM10000_TE_VNI_ENTRIES_0; i++)
    {
        if (switchExt->vnVsi[i] != NULL)
        {
            FM_LOG_PRINT("\n    VSI %d: vn %p (vsId %d)\n",
                         i, (void *) switchExt->vnVsi[i],
                         switchExt->vnVsi[i]->vsId);
			cnt++;
        }
    }
	if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DbgDumpVN */




/*****************************************************************************/
/** fm10000DbgDumpVirtualNetwork
 * \ingroup intDebug
 *
 * \desc            Display a virtual network status
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the virtual network VNI.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if a VN for a given vni
 *                  does not exist.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpVirtualNetwork(fm_int sw, fm_uint32 vni)
{
    fm_status                    status;
    fm_virtualNetwork *          vn;
    fm10000_virtualNetwork *     vnExt;
    fm_treeIterator              iter;
    fm_customTreeIterator        customIter;
    fm10000_vnRemoteAddress *    addrKey;
    fm10000_vnRemoteAddress *    addrRec;
    fm10000_vnRemoteAddressMask *addrMaskKey;
    fm10000_vnRemoteAddressMask *addrMaskRule;
    fm_int                       tunnelId;
    fm10000_vnTunnelUseCount *   tunnelUseCount;
    fm_char                      tempString1[100];
    fm_char                      tempString2[100];
    fm_char                      tempString3[100];
    fm_int                       cnt;
    fm_int                       cnt2;
    fm_int                       i;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);
    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_PRINT("\n===============================================================================\n");
    FM_LOG_PRINT("Virtual network VS ID %u\n", vn->vsId);
    FM_LOG_PRINT("(vn record pointer: %p)\n", (void *) vn);
    FM_LOG_PRINT("===============================================================================\n\n");

    FM_LOG_PRINT("Internal ID:        %u\n", vn->descriptor.internalId);
    FM_LOG_PRINT("VLAN:               %u\n", vn->descriptor.vlan);
    FM_LOG_PRINT("Mode:               %s\n",
                 (vn->descriptor.mode == FM_VN_MODE_VSWITCH_OFFLOAD) ?
                 "VSwitch-Offload" : "Transparent");
    FM_LOG_PRINT("Address type:       %s\n",
                 (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC) ?
                 "MAC" : "IP");
    if (vn->descriptor.mode == FM_VN_MODE_VSWITCH_OFFLOAD)
    {
        fmDbgConvertIPAddressToString(&vn->descriptor.sip, tempString1);
        FM_LOG_PRINT("Source IP address:  %s\n", tempString1);
    }
    FM_LOG_PRINT("Broadcast/Flooding: %s\n",
                 (vn->descriptor.bcastFlooding == TRUE) ?
                 "Enabled" : "Disabled");

    vnExt = vn->extension;

    FM_LOG_PRINT("\n*** FM10000-specific data: ***\n\n");

    FM_LOG_PRINT("primaryVsi:           %d\n", vnExt->primaryVsi);
    FM_LOG_PRINT("floodsetMcastGroup:   %d\n", vnExt->floodsetMcastGroup);
    FM_LOG_PRINT("floodsetEncapAclRule: %d\n", vnExt->floodsetEncapAclRule);

    /* Tree of remote addresses associated with this virtual network. */

    FM_LOG_PRINT("\nRemote Addresses:\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n\n");

    fmCustomTreeIterInit(&customIter, &vnExt->remoteAddresses);
    cnt = 0;

    while (1)
    {
        status = fmCustomTreeIterNext(&customIter,
                                      (void **) &addrKey,
                                      (void **) &addrRec);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
        {
            fmDbgConvertMacAddressToString(addrRec->remoteAddress.macAddress,
                                           tempString1);
            if (addrRec->addrMask != NULL)
            {
                fmDbgConvertMacAddressToString(addrRec->addrMask->remoteAddress.macAddress,
                                               tempString2);
                fmDbgConvertMacAddressToString(addrRec->addrMask->addrMask.macAddress,
                                               tempString3);
            }
        }
        else
        {
            fmDbgConvertIPAddressToString(&addrRec->remoteAddress.ipAddress,
                                          tempString1);
            if (addrRec->addrMask != NULL)
            {
                fmDbgConvertIPAddressToString(&addrRec->addrMask->remoteAddress.ipAddress,
                                              tempString2);
                fmDbgConvertIPAddressToString(&addrRec->addrMask->addrMask.ipAddress,
                                              tempString3);
            }
        }

        FM_LOG_PRINT("    Address: %s, Address Mask: ", tempString1);
		if (addrRec->addrMask == NULL)
		{
            FM_LOG_PRINT("NULL\n");
		}
		else
		{
            FM_LOG_PRINT("%s / %s\n", tempString2, tempString3);
		}

        FM_LOG_PRINT("        tunnelId: %d (tunnel: %p)\n",
                     addrRec->tunnel->tunnelId, (void *) addrRec->tunnel);

        ConvertTunnelGroupToString(addrRec->encapTunnelGroup,
                                   sizeof(tempString1), tempString1);
        ConvertTunnelGroupToString(addrRec->hashEncapGroup,
                                   sizeof(tempString2), tempString2);
        ConvertTunnelGroupToString(addrRec->decapTunnelGroup,
                                   sizeof(tempString3), tempString3);
        FM_LOG_PRINT("        encapTunnelGroup: %s\n", tempString1);
        FM_LOG_PRINT("        hashEncapGroup:   %s\n", tempString2);
        FM_LOG_PRINT("        decapTunnelGroup: %s\n", tempString3);

        FM_LOG_PRINT("        encapAclRule: %d, encapTunnelRule: %d, decapTunnelRule: %d\n",
                     addrRec->encapAclRule,
                     addrRec->encapTunnelRule,
                     addrRec->decapTunnelRule);

        FM_LOG_PRINT("        encapTep: %p", (void *) addrRec->encapTep);
        if (addrRec->encapTep == NULL)
        {
            FM_LOG_PRINT("\n");
        }
        else
        {
            FM_LOG_PRINT(" (useCount: %d, encapTunnelRule: %d)\n",
                         addrRec->encapTep->useCount,
                         addrRec->encapTep->encapTunnelRule);
        }

        if (addrRec->encapTunnelGroup == addrRec->hashEncapGroup)
        {
            FM_LOG_PRINT("        encapTunnelRules:\n");
            for (i = 0, cnt2 = 0; i < FM10000_TE_VNI_ENTRIES_0; i++)
            {
                if (addrRec->encapTunnelRules[i] >= 0)
                {
                    FM_LOG_PRINT("            vsi %d: %d\n",
                                 i, addrRec->encapTunnelRules[i]);
					cnt2++;
                }
            }
            if (cnt2 == 0)
            {
               FM_LOG_PRINT("            [none]\n");
            }
        }

		cnt++;
    }
    if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    /* Tree of remote address masks. */

    FM_LOG_PRINT("\nRemote Address Masks:\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n\n");

    fmCustomTreeIterInit(&customIter, &vnExt->addressMasks);
    cnt = 0;

    while (1)
    {
        status = fmCustomTreeIterNext(&customIter,
                                      (void **) &addrMaskKey,
                                      (void **) &addrMaskRule);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
        {
            fmDbgConvertMacAddressToString(addrMaskRule->remoteAddress.macAddress,
                                           tempString1);
            fmDbgConvertMacAddressToString(addrMaskRule->addrMask.macAddress,
                                           tempString2);
        }
        else
        {
            fmDbgConvertIPAddressToString(&addrMaskRule->remoteAddress.ipAddress,
                                          tempString1);
            fmDbgConvertIPAddressToString(&addrMaskRule->addrMask.ipAddress,
                                          tempString2);
        }

        FM_LOG_PRINT("    Address Mask: %s / %s\n", tempString1, tempString2);

        FM_LOG_PRINT("        tunnel: %p", (void *) addrMaskRule->tunnel);
        if (addrMaskRule->tunnel == NULL)
        {
            FM_LOG_PRINT("\n");
        }
        else
        {
            FM_LOG_PRINT(" (tunnelId: %d)\n", addrMaskRule->tunnel->tunnelId);
        }

        FM_LOG_PRINT("        encapAclRule: %d, encapTunnelRule: %d\n",
                     addrMaskRule->encapAclRule, addrMaskRule->encapTunnelRule);

        FM_LOG_PRINT("        encapTep: %p", (void *) addrMaskRule->encapTep);
        if (addrMaskRule->encapTep == NULL)
        {
            FM_LOG_PRINT("\n");
        }
        else
        {
            FM_LOG_PRINT(" (useCount: %d, encapTunnelRule: %d)\n",
                         addrMaskRule->encapTep->useCount,
                         addrMaskRule->encapTep->encapTunnelRule);
        }

		cnt++;
    }
    if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    /* Tree of tunnels that service this virtual network. */

    FM_LOG_PRINT("\nTunnels:\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n\n");

    fmTreeIterInit(&iter, &vnExt->tunnels);
    cnt = 0;

    while (1)
    {

        status = fmTreeIterNext(&iter,
                                (fm_uint64 *) &tunnelId,
                                (void **) &tunnelUseCount);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        FM_LOG_PRINT("    tunnelId: %d (tunnel: %p), useCount: %d\n",
                     tunnelId,
                     (void *) tunnelUseCount->tunnel,
                     tunnelUseCount->useCount);

		cnt++;
    }
    if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DbgDumpVirtualNetwork */




/*****************************************************************************/
/** fm10000DbgDumpVNTunnel
 * \ingroup intDebug
 *
 * \desc            Display a virtual network tunnel status
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel identifier.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if a tunnel for a given tunnel ID
 *                  does not exist.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpVNTunnel(fm_int sw, fm_int tunnelId)
{
    fm_status             status;
    fm_vnTunnel *         tunnel;
    fm10000_vnTunnel *    tunnelExt;
    fm_customTreeIterator customIter;
    fm10000_vnEncapTep    tepRuleKey;
    fm10000_vnEncapTep *  tepRule;
    fm_char               tempString[100];
    fm_char               routeDesc[1000];
    fm_int                cnt;
    fm_int                i;

    /* Get the Tunnel Record */
    tunnel = fmGetVNTunnel(sw, tunnelId);
    if (tunnel == NULL)
    {
        /* Tunnel doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_PRINT("\n===============================================================================\n");
    FM_LOG_PRINT("Tunnel ID %d\n", tunnel->tunnelId);
    FM_LOG_PRINT("(tunnel record pointer: %p)\n", (void *) tunnel);
    FM_LOG_PRINT("===============================================================================\n\n");

    ConvertTunnelTypeToString(tunnel->tunnelType,
                              sizeof(tempString),
                              tempString);
    FM_LOG_PRINT("Tunnel Type:                 %s\n", tempString);

    FM_LOG_PRINT("Traffic Identifier:          %d\n",
                 tunnel->trafficIdentifier);

    fmDbgConvertIPAddressToString(&tunnel->localIp, tempString);
    FM_LOG_PRINT("Local IP Address:            %s\n", tempString);

    fmDbgConvertIPAddressToString(&tunnel->remoteIp, tempString);
    FM_LOG_PRINT("Remote IP Address:           %s\n", tempString);

    FM_LOG_PRINT("Virtual Router ID:           %d\n", tunnel->vrid);
    FM_LOG_PRINT("Remote IP Virtual Router ID: %d\n", tunnel->remoteIpVrid);
    FM_LOG_PRINT("Multicast Group Number:      %d\n", tunnel->mcastGroup);

    fmDbgConvertMacAddressToString(tunnel->mcastDmac, tempString);
    FM_LOG_PRINT("Multicast DMAC:              %s\n", tempString);

    FM_LOG_PRINT("Encapsulation TTL Value:     %d\n", tunnel->encapTTL);

    FM_LOG_PRINT("route: %p\n", (void *) tunnel->route);
    if (tunnel->route != NULL)
    {
        fmDbgBuildRouteDescription(&tunnel->route->route, routeDesc);
        FM_LOG_PRINT("    %s\n", routeDesc);
    }

    tunnelExt = tunnel->extension;

    FM_LOG_PRINT("\n*** FM10000-specific data: ***\n\n");

    FM_LOG_PRINT("haveLocalIp:  %s\n",
                 (tunnelExt->haveLocalIp == TRUE) ? "TRUE" : "FALSE");
    FM_LOG_PRINT("haveRemoteIp: %s\n",
                 (tunnelExt->haveRemoteIp == TRUE) ? "TRUE" : "FALSE");
    FM_LOG_PRINT("decapAclRule: %d\n", tunnelExt->decapAclRule);

    FM_LOG_PRINT("encapFlowIds:\n");
    for (i = 0; i < FM_VN_NUM_TUNNEL_GROUPS; i++)
    {
        ConvertTunnelGroupToString(i, sizeof(tempString), tempString);
        FM_LOG_PRINT("    tunnel group %s, ENCAP_FLOW_TRANSPARENT:\t%d\n",
                     tempString,
                     tunnelExt->encapFlowIds[i][FM_VN_ENCAP_FLOW_TRANSPARENT]);
        FM_LOG_PRINT("    tunnel group %s, ENCAP_FLOW_VSWITCH:\t%d\n",
                     tempString,
                     tunnelExt->encapFlowIds[i][FM_VN_ENCAP_FLOW_VSWITCH]);
    }

    FM_LOG_PRINT("ruleCounts:\n");
    for (i = 0; i < FM_VN_NUM_TUNNEL_GROUPS; i++)
    {
        ConvertTunnelGroupToString(i, sizeof(tempString), tempString);
        FM_LOG_PRINT("    tunnel group %s, ENCAP_FLOW_TRANSPARENT:\t%d\n",
                     tempString,
                     tunnelExt->ruleCounts[i][FM_VN_ENCAP_FLOW_TRANSPARENT]);
        FM_LOG_PRINT("    tunnel group %s, ENCAP_FLOW_VSWITCH:\t%d\n",
                     tempString,
                     tunnelExt->ruleCounts[i][FM_VN_ENCAP_FLOW_VSWITCH]);
    }

    /* Tree of TEP encap tunnel rules. */

    FM_LOG_PRINT("\nTEP Rules:\n");
    FM_LOG_PRINT("-------------------------------------------------------------------------------\n\n");

    fmCustomTreeIterInit(&customIter, &tunnelExt->tepRules);
    cnt = 0;

    while (1)
    {
        status = fmCustomTreeIterNext(&customIter,
                                      (void **) &tepRuleKey,
                                      (void **) &tepRule);
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        FM_LOG_PRINT("    encapTunnelRule: %d, useCount: %d, vn: %p (vsId: %d)\n",
                     tepRule->encapTunnelRule, tepRule->useCount,
                     (void *) tepRule->vn, tepRule->vn->vsId);

		cnt++;
    }
    if (cnt == 0)
    {
        FM_LOG_PRINT("    [none]\n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fm10000DbgDumpVNTunnel */

