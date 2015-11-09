/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mailbox.c
 * Creation Date:   September 4, 2014
 * Description:     Function for operating mailbox.
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

#define FM_GET_OFFSET_FIELD(rvalue, offset)     \
    (offset == 0) ?                             \
        FM_GET_FIELD(rvalue,                    \
                     FM_MAILBOX_BITS,           \
                     0_15) :                    \
        FM_GET_FIELD(rvalue,                    \
                     FM_MAILBOX_BITS,           \
                     16_31)

#define GET_MAILBOX_INFO(sw) \
    &((fm_switch *)(GET_SWITCH_PTR( sw )))->mailboxInfo;

#define GET_LISTENER_KEY(port, vlan) \
    ( ( ( (fm_uint64) port ) << 16 ) | ( (fm_uint64) vlan ) )

#define GET_MACVLAN_RESOURCE_KEY(mac, vlan) \
    ( ( ( (fm_uint64) mac ) & 0xFFFFFFFFFFFFl) | ( (fm_uint64) vlan << 48 ) )

#define GET_MAC_FROM_MACVLAN_RESOURCE_KEY(key) \
    ( (fm_macaddr) (key & 0xFFFFFFFFFFFFl) )

#define GET_VLAN_FROM_MACVLAN_RESOURCE_KEY(key) \
    ( (fm_uint16) ( (key >> 48) & 0xFFFF) )

#define GET_FLOW_RESOURCE_KEY(flowId, flowTable) \
    ( ( ( (fm_uint32) flowId ) & 0xFFFFFFFFl) | ( (fm_uint64) flowTable << 32 ) )

#define GET_FLOW_ID_FROM_FLOW_RESOURCE_KEY(key) \
    ( (fm_uint32) (key & 0xFFFFFFFFl) )

#define GET_FLOW_TABLE_FROM_FLOW_RESOURCE_KEY(key) \
    ( (fm_uint32) ( (key >> 32) & 0xFFFFFFFFl) )

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
/** PrintControlHeader
 * \ingroup intMailbox
 *
 * \desc            Print mailbox control header field values.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          None.
 *
 *****************************************************************************/
static void PrintControlHeader(fm_mailboxControlHeader *ctrlHdr)
{
    FM_LOG_PRINT("smVersion=%d, smErrorFlags=%d reqHead=%d, respTail=%d\n"
                 "pfVersion=%d, pfErrorFlags=%d respHead=%d, reqTail=%d\n",
                 ctrlHdr->smVersion, ctrlHdr->smErrorFlags,
                 ctrlHdr->reqHead, ctrlHdr->respTail,
                 ctrlHdr->pfVersion, ctrlHdr->pfErrorFlags,
                 ctrlHdr->respHead, ctrlHdr->reqTail);

}   /* end PrintControlHeader */




/*****************************************************************************/
/** PrintMessageHeader
 * \ingroup intMailbox
 *
 * \desc            Print mailbox message header field values.
 *
 * \param[in]       msgHdr points to mailbox message header structure.
 *
 * \return          None.
 *
 *****************************************************************************/
static void PrintMessageHeader(fm_mailboxMessageHeader *msgHdr)
{
    FM_LOG_PRINT("type=%d, flags=%d, length=%d\n",
                 msgHdr->type, msgHdr->flags, msgHdr->length);

}   /* end PrintMessageHeader */




/*****************************************************************************/
/** CountBitsInMacMask
 * \ingroup intMailbox
 *
 * \desc            Helper function to count the number of masked bits 
 *                  in MAC address mask.
 *
 * \param[in]       macAddrMask is the MAC address mask whose masked bits
 *                  are to be counted.
 *
 * \return          The number of bits set in the word.
 *
 *****************************************************************************/
static fm_int CountZerosInMacMask(fm_macaddr macAddrMask)
{
    fm_int     i;
    fm_int     counter;
    fm_macaddr maskInvert;

    counter    = 0;
    maskInvert = ~macAddrMask;

    for (i = 0 ; i < FM_MAC_ADDRESS_SIZE ; i++)
    {
        if ( (maskInvert >> i) & 0x1)
        {
            counter++;
        }
    }

    return counter;

}   /* end CountZerosInMacMask */




/*****************************************************************************/
/** AssociateMcastGroupWithFlood
 * \ingroup intMailbox
 *
 * \desc            Associate mcast group with flooding type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       floodPort is the flooding port number.
 *
 * \param[in]       mcastGroup is the mcast group to be associated.
 *
 * \param[in]       associate should be TRUE to associate mcast group,
 *                  FALSE to remove mcast group from flooding type.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AssociateMcastGroupWithFlood(fm_int                sw,
                                              fm_int                pepNb,
                                              fm_int                floodPort,
                                              fm_intMulticastGroup *mcastGroup,
                                              fm_bool               associate)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, floodPort=%d, mcastGroup=%p, associate=%d\n",
                 sw,
                 floodPort,
                 (void *) mcastGroup,
                 associate);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(status,
                       switchPtr->AssociateMcastGroupWithFlood,
                       sw,
                       pepNb,
                       floodPort,
                       mcastGroup,
                       associate);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end AssociateMcastGroupWithFlood */




/*****************************************************************************/
/** GetMaxSupportedTxTimestampMode
 * \ingroup intMailbox
 *
 * \desc            Retrive the maximum supported Tx timestamp mode supported.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      txTimestampMode is where max timestamp mode supported is
 *                  returned.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetMaxSupportedTxTimestampMode(fm_int                   sw, 
                                                fm_portTxTimestampMode * txTimestampMode)
{

    *txTimestampMode = FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_ANY;

    return FM_OK;
    
}   /* end GetMaxSupportedTxTimestampMode */




/*****************************************************************************/
/** AddMacFilterAclRule
 * \ingroup intMailbox
 *
 * \desc            Adds a inner/outer MAC filtering ACL rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       macFilter points to a structure containing fields 
 *                  to be filtered.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddMacFilterAclRule(fm_int               sw,
                                     fm_int               pepNb,
                                     fm_hostSrvInnOutMac *macFilter)
{
    fm_mailboxInfo *       info;
    fm_status              status;
    fm_aclCondition        aclCond;
    fm_aclValue            aclCondData;
    fm_aclActionExt        aclAction;
    fm_aclParamExt         aclActionData;
    fm_aclArguments        aclArgs;
    fm_mailboxResources *  mlbxResUsed;
    fm_mailboxMcastMacVni  macVniKey;
    fm_mailboxMcastMacVni *macVniVal;
    fm_multicastListener   mcastListener;
    fm_int                 logicalPort;
    fm_int                 aclRule;
    fm_int                 bitNum;
    fm_uint32              allScenarios;
    fm_char                statusText[1024];

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw = %d, macFilter = %p\n",
                 sw,
                 (void *) macFilter);

    info         = GET_MAILBOX_INFO(sw);
    status       = FM_OK;
    macVniVal    = NULL;
    allScenarios = (FM_ACL_SCENARIO_ANY_FRAME_TYPE |
                    FM_ACL_SCENARIO_ANY_ROUTING_TYPE |
                    FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE);

    status = fmGetGlortLogicalPort(sw,
                                   macFilter->glort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        logicalPort,
                        (void **) &mlbxResUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Check if we don't exceed MAC filtering entries limits. */

    /* Check PEP limit */
    if ( (info->innerOuterMacEntriesAdded[pepNb]) >=
         (info->maxInnerOuterMacEntriesToAddPerPep) )
    {
        status = FM_ERR_TABLE_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Check virtual port limit */
    if ( (mlbxResUsed->innerOuterMacEntriesAdded) >=
         (info->maxInnerOuterMacEntriesToAddPerPort) )
    {
        status = FM_ERR_TABLE_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Create ACL if needed (with first added rule) */
    status = fmGetACL(sw,
                      info->aclIdForMacFiltering,
                      &aclArgs);

    if (status == FM_ERR_INVALID_ACL)
    {
        status = fmCreateACLInt(sw,
                                info->aclIdForMacFiltering,
                                allScenarios,
                                FM_ACL_DEFAULT_PRECEDENCE,
                                TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else if (status != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Build ACL rule condition */
    FM_CLEAR(aclCondData);

    aclCond = FM_ACL_MATCH_DST_MAC
              | FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT;

    aclCondData.dst     = macFilter->outerMacAddr;
    aclCondData.dstMask = macFilter->outerMacAddrMask;

    switch (macFilter->tunnelType)
    {
        case FM_MAILBOX_TUNNEL_TYPE_VXLAN:

            if (macFilter->outerL4Port != 0)
            {
                aclCond |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
                aclCondData.L4DstStart = macFilter->outerL4Port;
                aclCondData.L4DstMask  = 0xFFFF;
            }
            else
            {
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            if (macFilter->vni != FM_HOST_SRV_INN_OUT_MAC_VNI_NO_MATCH)
            {
                aclCondData.L4DeepInspectionExt[4] =
                            (macFilter->vni & 0xFF0000) >> 16;
                aclCondData.L4DeepInspectionExt[5] =
                            (macFilter->vni & 0x00FF00) >> 8;
                aclCondData.L4DeepInspectionExt[6] =
                            (macFilter->vni & 0x0000FF);

                aclCondData.L4DeepInspectionExtMask[4] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[5] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[6] = 0xFF;
                /* Index 7 for deep inspection should be unused here, as
                   vni is a 24-bit key. */
            }

            aclCondData.L4DeepInspectionExt[8] = 
                        (macFilter->innerMacAddr & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExt[9] =
                        (macFilter->innerMacAddr & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExt[10] =
                        (macFilter->innerMacAddr & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExt[11] =
                        (macFilter->innerMacAddr & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExt[12] =
                        (macFilter->innerMacAddr & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExt[13] =
                        (macFilter->innerMacAddr & 0x0000000000FF);
            
            aclCondData.L4DeepInspectionExtMask[8] = 
                        (macFilter->innerMacAddrMask & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExtMask[9] =
                        (macFilter->innerMacAddrMask & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExtMask[10] =
                        (macFilter->innerMacAddrMask & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExtMask[11] =
                        (macFilter->innerMacAddrMask & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExtMask[12] =
                        (macFilter->innerMacAddrMask & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExtMask[13] =
                        (macFilter->innerMacAddrMask & 0x0000000000FF);
            break;

        case FM_MAILBOX_TUNNEL_TYPE_NVGRE:

            aclCond |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
            aclCondData.L4DstStart = GET_PROPERTY()->vnEncapProtocol;
            aclCondData.L4DstMask  = 0xFFFF;

            if (macFilter->vni != FM_HOST_SRV_INN_OUT_MAC_VNI_NO_MATCH)
            {
                aclCondData.L4DeepInspectionExt[0] =
                            (macFilter->vni & 0xFF0000) >> 16;
                aclCondData.L4DeepInspectionExt[1] =
                            (macFilter->vni & 0x00FF00) >> 8;
                aclCondData.L4DeepInspectionExt[2] =
                            (macFilter->vni & 0x0000FF);

                aclCondData.L4DeepInspectionExtMask[0] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[1] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[2] = 0xFF;
                /* Index 3 for deep inspection should be unused here, as
                   vni is a 24-bit key. */
            }

            aclCondData.L4DeepInspectionExt[4] =
                        (macFilter->innerMacAddr & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExt[5] =
                        (macFilter->innerMacAddr & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExt[6] =
                        (macFilter->innerMacAddr & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExt[7] =
                        (macFilter->innerMacAddr & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExt[8] =
                        (macFilter->innerMacAddr & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExt[9] =
                        (macFilter->innerMacAddr & 0x0000000000FF);

            aclCondData.L4DeepInspectionExtMask[4] =
                        (macFilter->innerMacAddrMask & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExtMask[5] =
                        (macFilter->innerMacAddrMask & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExtMask[6] =
                        (macFilter->innerMacAddrMask & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExtMask[7] =
                        (macFilter->innerMacAddrMask & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExtMask[8] =
                        (macFilter->innerMacAddrMask & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExtMask[9] =
                        (macFilter->innerMacAddrMask & 0x0000000000FF);
            break;

        case FM_MAILBOX_TUNNEL_TYPE_GENEVE:

            if (macFilter->outerL4Port != 0)
            {
                aclCond |= FM_ACL_MATCH_L4_DST_PORT_WITH_MASK;
                aclCondData.L4DstStart = macFilter->outerL4Port;
                aclCondData.L4DstMask  = 0xFFFF;
            }
            else
            {
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            if (macFilter->vni != FM_HOST_SRV_INN_OUT_MAC_VNI_NO_MATCH)
            {
                aclCondData.L4DeepInspectionExt[4] =
                            (macFilter->vni & 0xFF0000) >> 16;
                aclCondData.L4DeepInspectionExt[5] =
                            (macFilter->vni & 0x00FF00) >> 8;
                aclCondData.L4DeepInspectionExt[6] =
                            (macFilter->vni & 0x0000FF);

                aclCondData.L4DeepInspectionExtMask[4] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[5] = 0xFF;
                aclCondData.L4DeepInspectionExtMask[6] = 0xFF;
                /* Index 7 for deep inspection should be unused here, as
                   vni is a 24-bit key. */
            }

            aclCondData.L4DeepInspectionExt[8] =
                        (macFilter->innerMacAddr & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExt[9] =
                        (macFilter->innerMacAddr & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExt[10] =
                        (macFilter->innerMacAddr & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExt[11] =
                        (macFilter->innerMacAddr & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExt[12] =
                        (macFilter->innerMacAddr & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExt[13] =
                        (macFilter->innerMacAddr & 0x0000000000FF);

            aclCondData.L4DeepInspectionExtMask[8] =
                        (macFilter->innerMacAddrMask & 0xFF0000000000) >> 40;
            aclCondData.L4DeepInspectionExtMask[9] =
                        (macFilter->innerMacAddrMask & 0x00FF00000000) >> 32;
            aclCondData.L4DeepInspectionExtMask[10] =
                        (macFilter->innerMacAddrMask & 0x0000FF000000) >> 24;
            aclCondData.L4DeepInspectionExtMask[11] =
                        (macFilter->innerMacAddrMask & 0x000000FF0000) >> 16;
            aclCondData.L4DeepInspectionExtMask[12] =
                        (macFilter->innerMacAddrMask & 0x00000000FF00) >> 8;
            aclCondData.L4DeepInspectionExtMask[13] =
                        (macFilter->innerMacAddrMask & 0x0000000000FF);
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Build ACL rule action */
    FM_CLEAR(aclActionData);

    aclAction = FM_ACL_ACTIONEXT_REDIRECT;

    if (fmIsMulticastMacAddress(macFilter->innerMacAddr))
    {
        /* If inner MAC is a mcast one, ACL rule will redirect frame 
           to a multicast group. First we need to check if such group
           is already created. */
        
        macVniKey.macAddr = macFilter->innerMacAddr;
        macVniKey.vni     = macFilter->vni;

        status = fmCustomTreeFind(&info->mcastMacVni,
                                  &macVniKey,
                                  (void **) &macVniVal);

        /* Use existing mcast group */
        if (status == FM_OK)
        {
            /* Redirect frame to mcast logicalPort */
            status = fmGetMcastGroupPort(sw,
                                         macVniVal->mcastGroup,
                                         &aclActionData.logicalPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_CLEAR(mcastListener);

            mcastListener.port = logicalPort;
            mcastListener.vlan = 0;

            status = fmAddMcastGroupListener(sw,
                                             macVniVal->mcastGroup,
                                             &mcastListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
        /* New entry */
        else if (status == FM_ERR_NOT_FOUND)
        {
            macVniVal = fmAlloc( sizeof(fm_mailboxMcastMacVni) );

            if (macVniVal == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_CLEAR(*macVniVal);

            status = fmCreateMcastGroup(sw,
                                        &macVniVal->mcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = fmActivateMcastGroup(sw,
                                          macVniVal->mcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_CLEAR(mcastListener);

            mcastListener.port = logicalPort;
            mcastListener.vlan = 0;

            status = fmAddMcastGroupListener(sw,
                                             macVniVal->mcastGroup,
                                             &mcastListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Redirect frame to mcast logicalPort */
            status = fmGetMcastGroupPort(sw,
                                         macVniVal->mcastGroup,
                                         &aclActionData.logicalPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            macVniVal->macAddr = macFilter->innerMacAddr;
            macVniVal->vni     = macFilter->vni;

            status = fmCustomTreeInsert(&info->mcastMacVni,
                                        macVniVal,
                                        (void *) macVniVal);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
        /* Unhandled error */
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }
    /* Redirect frame to a single port */
    else
    {
        aclActionData.logicalPort = logicalPort;
    }

    /* Build ACL rule ID. The rule id scheme would be:
       ( (0b000000) |(High 12 bit) | (Low 14 bit)
       Low part is the unique id taken from a pool.
       High part are the numbers of masked bit:
       (outerDmac 6bit) | (innerDmac 6bit)
       This scheme is to make sure the mask drive the precedence. */
    status = fmFindBitInBitArray(&info->innOutMacRuleInUse,
                                 0,
                                 FALSE,
                                 &bitNum);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (bitNum < 0)
    {
        status = FM_ERR_TABLE_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    aclRule = bitNum;

    aclRule |= ( CountZerosInMacMask(macFilter->innerMacAddrMask) << 14)
                & 0xFC000;

    aclRule |= ( CountZerosInMacMask(macFilter->outerMacAddrMask) << 20)
                & 0x3F00000;

    status = fmAddACLRuleExt(sw,
                             info->aclIdForMacFiltering,
                             aclRule,
                             aclCond,
                             &aclCondData,
                             aclAction,
                             &aclActionData);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmCompileACLExt(sw,
                             statusText,
                             sizeof(statusText),
                             FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                             FM_ACL_COMPILE_FLAG_INTERNAL,
                             (void*) &info->aclIdForMacFiltering);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "ACL compiled, status=%d, statusText=%s\n",
                 status,
                 statusText);

    status = fmApplyACLExt(sw,
                           FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                           FM_ACL_APPLY_FLAG_INTERNAL,
                           (void*) &info->aclIdForMacFiltering);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 

    status = fmSetBitArrayBit(&info->innOutMacRuleInUse,
                              bitNum,
                              1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Incrementing pep counter. */
    info->innerOuterMacEntriesAdded[pepNb]++;

    /* Incrementing virtual port counter. */
    mlbxResUsed->innerOuterMacEntriesAdded++;

    macFilter->acl     = info->aclIdForMacFiltering;
    macFilter->aclRule = aclRule;

    status = fmCustomTreeInsert(&mlbxResUsed->innOutMacResource,
                                macFilter,
                                (void *) macFilter);

    if (status == FM_ERR_ALREADY_EXISTS)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Given MAC filter key:\n"
                     "Outer MAC: %012llx mask: %012llx\n"
                     "Inner MAC: %012llx mask: %012llx\n"
                     "VNI: 0x%x, outer L4 port: 0x%x, tunnel type: %d\n"
                     " already tracked with port %d\n",
                     macFilter->outerMacAddr,
                     macFilter->outerMacAddrMask,
                     macFilter->innerMacAddr,
                     macFilter->innerMacAddrMask,
                     macFilter->vni,
                     macFilter->outerL4Port,
                     macFilter->tunnelType,
                     logicalPort);
    }
    else if (status != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (macVniVal != NULL)
        {
            fmFree(macVniVal);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}  /* end AddMacFilterAclRule */




/*****************************************************************************/
/** DeleteMacFilterAclRule
 * \ingroup intMailbox
 *
 * \desc            Deletes a inner/outer MAC filtering ACL rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       macFilter points to a structure containing fields 
 *                  to be filtered.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteMacFilterAclRule(fm_int               sw,
                                        fm_int               pepNb,
                                        fm_hostSrvInnOutMac *macFilter)
{
    fm_mailboxInfo *       info;
    fm_status              status;
    fm_mailboxResources *  mlbxResUsed;
    fm_mailboxMcastMacVni  macVniKey;
    fm_mailboxMcastMacVni *macVniVal;
    fm_multicastListener   mcastListener;
    fm_hostSrvInnOutMac *  macFilterUsed;
    fm_int                 logicalPort;
    fm_int                 aclRule;
    fm_int                 bitNum;
    fm_char                statusText[1024];
    fm_aclCondition        cond;
    fm_aclValue            value;
    fm_aclActionExt        action;
    fm_aclParamExt         param;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw = %d, macFilter = %p\n",
                 sw,
                 (void *) macFilter);

    info   = GET_MAILBOX_INFO(sw);
    status = FM_OK;

    status = fmGetGlortLogicalPort(sw,
                                   macFilter->glort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        logicalPort,
                        (void **) &mlbxResUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmCustomTreeFind(&mlbxResUsed->innOutMacResource,
                              macFilter,
                              (void *) &macFilterUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmDeleteACLRule(sw,
                             macFilterUsed->acl,
                             macFilterUsed->aclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Remove ACL if empty */
    status = fmGetACLRuleFirstExt(sw,
                                  info->aclIdForMacFiltering,
                                  &aclRule,
                                  &cond,
                                  &value,
                                  &action,
                                  &param);

    if (status == FM_ERR_NO_RULES_IN_ACL)
    {
        status = fmDeleteACL(sw,
                             info->aclIdForMacFiltering);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else if (status != FM_OK)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    status = fmCompileACLExt(sw,
                             statusText,
                             sizeof(statusText),
                             FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                             FM_ACL_COMPILE_FLAG_INTERNAL,
                             (void *) &macFilterUsed->acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "ACL compiled, status=%d, statusText=%s\n",
                 status,
                 statusText);

    status = fmApplyACLExt(sw,
                           FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                           FM_ACL_APPLY_FLAG_INTERNAL,
                           (void*) &macFilterUsed->acl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* The rule id scheme is:
       ( (0b000000) |(High 12 bit) | (Low 14 bit)
       where only 14 lower bits from rule id are taken from bitarray,
       so we need to mask id before releasing it. */
    bitNum = macFilterUsed->aclRule & 0x3FFF;

    status = fmSetBitArrayBit(&info->innOutMacRuleInUse,
                              bitNum,
                              0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmIsMulticastMacAddress(macFilter->innerMacAddr))
    {
        /* If inner MAC is a mcast one, ACL rule should redirect frame 
           to a multicast group. We need to remove listener
           from such group. */

        macVniKey.macAddr = macFilter->innerMacAddr;
        macVniKey.vni     = macFilter->vni;

        status = fmCustomTreeFind(&info->mcastMacVni,
                                  &macVniKey,
                                  (void **) &macVniVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        FM_CLEAR(mcastListener);

        mcastListener.port = logicalPort;
        mcastListener.vlan = 0;

        status = fmDeleteMcastGroupListener(sw,
                                            macVniVal->mcastGroup,
                                            &mcastListener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* If there is no other listeners, delete mcast group */
        status = fmGetMcastGroupListenerFirst(sw,
                                              macVniVal->mcastGroup,
                                              &mcastListener);

        if (status == FM_ERR_NO_MORE)
        {
            status = fmDeactivateMcastGroup(sw,
                                            macVniVal->mcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = fmDeleteMcastGroup(sw,
                                        macVniVal->mcastGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = fmCustomTreeRemoveCertain(&info->mcastMacVni,
                                               &macVniKey,
                                               fmFreeMcastMacVni);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
        else if (status != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    fmCustomTreeRemoveCertain(&mlbxResUsed->innOutMacResource,
                              macFilter,
                              fmFreeSrvInnOutMac);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}  /* end DeleteMacFilterAclRule */




/*****************************************************************************/
/** SetMgmtXcastModes
 * \ingroup intMailbox
 *
 * \desc            Set the Management PEP's XCAST modes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       xcastGlort is Xcast flooding glort.
 *
 * \param[in]       srvXcastMode is a structure to the Xcast glort and mode.
 *
 * \param[in]       xcastFloodMode is the Xcast flooding mode.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status SetMgmtXcastModes(fm_int              sw,
                                   fm_int              pepNb,
                                   fm_int              xcastGlort,
                                   fm_hostSrvXcastMode srvXcastMode,
                                   fm_uint16           xcastFloodMode)
{
    fm_switch *              switchPtr;
    fm_status                status;
    fm_int                   logicalPort;
    fm_intMulticastGroup *   mcastGroupForMcastFlood;
    fm_intMulticastGroup *   mcastGroupForUcastFlood;
    fm_intMulticastGroup *   mcastGroupForBcastFlood;
    fm_mailboxInfo *         info;
    fm_uint64                listenerKey;
    fm_intMulticastListener *intListener;
    fm_multicastListener     listener;
    fm_bool                  lockTaken;
    fm_bool                  mcastHNIFlooding;
    fm_int                   swToExecute;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, XcastGlort=0x%x, XcastMode=0x%x\n",
                 sw,
                 pepNb,
                 srvXcastMode.glort,
                 srvXcastMode.mode);

    swToExecute  = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);
    info         = GET_MAILBOX_INFO(swToExecute);
    lockTaken    = FALSE;
    status       = FM_OK;
    listenerKey  = 0;
    logicalPort  = 0;
    mcastGroupForMcastFlood = NULL;
    mcastGroupForUcastFlood = NULL;
    mcastGroupForBcastFlood = NULL;
    intListener             = NULL;

    FM_CLEAR(listener);

    switchPtr = GET_SWITCH_PTR(sw);

    mcastHNIFlooding = GET_PROPERTY()->hniMcastFlooding;

    status = fmGetGlortLogicalPort(sw,
                                   srvXcastMode.glort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    lockTaken = TRUE;    

    /* Using internal functions to avoid iterating over all listeners */
    mcastGroupForMcastFlood = fmFindMcastGroup(sw, 
                                               info->mcastGroupForMcastFlood);
    if (mcastGroupForMcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    mcastGroupForUcastFlood = fmFindMcastGroup(sw,
                                               info->mcastGroupForUcastFlood);
    if (mcastGroupForUcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    mcastGroupForBcastFlood = fmFindMcastGroup(sw,
                                               info->mcastGroupForBcastFlood);
    if (mcastGroupForBcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    listenerKey   = GET_LISTENER_KEY(logicalPort, 
                                     FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS);

    listener.vlan = FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
    listener.port = logicalPort;
    listener.floodListener = TRUE;
    listener.xcastGlort = xcastGlort;

    switch (srvXcastMode.mode)
    {
        case FM_HOST_SRV_XCAST_MODE_PROMISC:

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                    listenerKey,
                                    (void **) &intListener);

                if (status == FM_ERR_NOT_FOUND)
                {
                    status = fmAddMcastGroupListener(sw,
                                                     info->mcastGroupForMcastFlood,
                                                     &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_MCAST,
                                                          mcastGroupForMcastFlood,
                                                          TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    /* Update MCAST HNI flooding groups */
                    if (mcastHNIFlooding)
                    {
                        status = fmUpdateMcastHNIFloodingGroups(sw,
                                                                logicalPort,
                                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    }
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                                    listenerKey,
                                    (void **) &intListener);

                if (status == FM_ERR_NOT_FOUND)
                {
                    status = fmAddMcastGroupListener(sw,
                                                     info->mcastGroupForUcastFlood,
                                                     &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_FLOOD,
                                                          mcastGroupForUcastFlood,
                                                          TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                    listenerKey,
                                    (void **) &intListener);

                if (status == FM_ERR_NOT_FOUND)
                {
                    status = fmAddMcastGroupListener(sw,
                                                     info->mcastGroupForBcastFlood,
                                                     &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_BCAST,
                                                          mcastGroupForBcastFlood,
                                                          TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_HOST_SRV_XCAST_MODE_NONE:

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                    listenerKey,
                                    (void **) &intListener);

                if (status == FM_OK)
                {
                    status = fmDeleteMcastGroupListener(sw,
                                                        info->mcastGroupForMcastFlood,
                                                        &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_MCAST,
                                                          mcastGroupForMcastFlood,
                                                          FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    /* Update MCAST HNI flooding groups */
                    if (mcastHNIFlooding)
                    {
                        status = fmUpdateMcastHNIFloodingGroups(sw,
                                                                logicalPort,
                                                                FALSE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    }
                }
                else if (status != FM_ERR_NOT_FOUND)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                                 listenerKey,
                                 (void **) &intListener);

                if (status == FM_OK)
                {
                    status = fmDeleteMcastGroupListener(sw,
                                                        info->mcastGroupForUcastFlood,
                                                        &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_FLOOD,
                                                          mcastGroupForUcastFlood,
                                                          FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
                else if (status != FM_ERR_NOT_FOUND)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                /* Search the tree of listeners for a matching entry. */
                status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                    listenerKey,
                                    (void **) &intListener);

                if (status == FM_OK)
                {
                    status = fmDeleteMcastGroupListener(sw,
                                                        info->mcastGroupForBcastFlood,
                                                        &listener);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = AssociateMcastGroupWithFlood(sw,
                                                          pepNb,
                                                          FM_PORT_BCAST,
                                                          mcastGroupForBcastFlood,
                                                          FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
                else if (status != FM_ERR_NOT_FOUND)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 

            break;

        default:
            status = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

    status = FM_OK;

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end SetXcastModes */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmFreeMcastMacVni
 * \ingroup intNat
 *
 * \desc            Free a fm_mailboxMcastMacVni structure.
 *
 * \param[in]       key points to the key.
 *
 * \param[in,out]   value points to the structure to free.
 * 
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeMcastMacVni(void *key, void *value)
{
    FM_NOT_USED(key);

    fmFree(value);

}   /* end fmFreeMcastMacVni */




/*****************************************************************************/
/** fmFreeSrvInnOutMac
 * \ingroup intNat
 *
 * \desc            Free a fm_hostSrvInnOutMac structure.
 *
 * \param[in]       key points to the key.
 *
 * \param[in,out]   value points to the structure to free.
 * 
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeSrvInnOutMac(void *key, void *value)
{
    FM_NOT_USED(key);

    fmFree(value);

}   /* end fmFreeSrvInnOutMac */




/*****************************************************************************/
/** fmCompareInnerOuterMacFilters
 * \ingroup intMailbox
 *
 * \desc            Compares two fm_hostSrvInnOutMac structures.
 *
 * \param[in]       key1 points to one of the keys.
 *
 * \param[in]       key2 points to the other key.
 *
 * \return          -1 if key1 comes before key2.
 * \return          0  if key1 is equal to key2.
 * \return          1  if key1 comes after key2.
 *
 *****************************************************************************/
fm_int fmCompareInnerOuterMacFilters(const void *key1, const void *key2)
{
    const fm_hostSrvInnOutMac *filter1;
    const fm_hostSrvInnOutMac *filter2;

    filter1 = (fm_hostSrvInnOutMac *) key1;
    filter2 = (fm_hostSrvInnOutMac *) key2;

    if (filter1->outerMacAddr < filter2->outerMacAddr)
    {
        return -1;
    }
    else if (filter1->outerMacAddr > filter2->outerMacAddr)
    {
        return 1;
    }

    if (filter1->outerMacAddrMask < filter2->outerMacAddrMask)
    {
        return -1;
    }
    else if (filter1->outerMacAddrMask > filter2->outerMacAddrMask)
    {
        return 1;
    }

    if (filter1->innerMacAddr < filter2->innerMacAddr)
    {
        return -1;
    }
    else if (filter1->innerMacAddr > filter2->innerMacAddr)
    {
        return 1;
    }

    if (filter1->innerMacAddrMask < filter2->innerMacAddrMask)
    {
        return -1;
    }
    else if (filter1->innerMacAddrMask > filter2->innerMacAddrMask)
    {
        return 1;
    }

    if (filter1->vni < filter2->vni)
    {
        return -1;
    }
    else if (filter1->vni > filter2->vni)
    {
        return 1;
    }

    if (filter1->outerL4Port < filter2->outerL4Port)
    {
        return -1;
    }
    else if (filter1->outerL4Port > filter2->outerL4Port)
    {
        return 1;
    }

    if (filter1->tunnelType < filter2->tunnelType)
    {
        return -1;
    }
    else if (filter1->tunnelType > filter2->tunnelType)
    {
        return 1;
    }

    if (filter1->glort < filter2->glort)
    {
        return -1;
    }
    else if (filter1->glort > filter2->glort)
    {
        return 1;
    }

    return 0;

}   /* end fmCompareInnerOuterMacFilters */




/*****************************************************************************/
/** fmCompareMcastMacVniKeys
 * \ingroup intMailbox
 *
 * \desc            Compares two fm_mailboxMcastMacVni structures.
 *
 * \param[in]       key1 points to one of the keys.
 *
 * \param[in]       key2 points to the other key.
 *
 * \return          -1 if key1 comes before key2.
 * \return          0  if key1 is equal to key2.
 * \return          1  if key1 comes after key2.
 *
 *****************************************************************************/
fm_int fmCompareMcastMacVniKeys(const void *key1, const void *key2)
{
    const fm_mailboxMcastMacVni *macVniKey1;
    const fm_mailboxMcastMacVni *macVniKey2;

    macVniKey1 = (fm_mailboxMcastMacVni *) key1;
    macVniKey2 = (fm_mailboxMcastMacVni *) key2;

    if (macVniKey1->macAddr < macVniKey2->macAddr)
    {
        return -1;
    }
    else if (macVniKey1->macAddr < macVniKey2->macAddr)
    {
        return 1;
    }

    if (macVniKey1->vni < macVniKey2->vni)
    {
        return -1;
    }
    else if (macVniKey1->vni < macVniKey2->vni)
    {
        return 1;
    }

    return 0;

}   /* end fmCompareMcastMacVniKeys */




/*****************************************************************************/
/** fmSendHostSrvErrResponse
 * \ingroup intMailbox
 *
 * \desc            Send response containing fm_hostSrvErr structure if 
 *                  provided status contains any errors.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       status is the error status from processed message.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 * 
 * \param[in]       msgTypeId is the message type ID.
 *
 * \param[in]       argType is the type of the message to be written.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmSendHostSrvErrResponse(fm_int                        sw,
                              fm_int                        pepNb,
                              fm_status                     status,
                              fm_mailboxControlHeader *     ctrlHdr,
                              fm_mailboxMessageId           msgTypeId,
                              fm_mailboxMessageArgumentType argType)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_hostSrvErr            srvErr;
    fm_uint32                rowsUsed;
    fm_int                   i;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, status=%d, ctrlHdr = %p," 
                 " msgTypeId=%d, argType=%d\n",
                 sw,
                 pepNb,
                 status,
                 (void *) ctrlHdr,
                 msgTypeId,
                 argType);

    switchPtr = GET_SWITCH_PTR(sw); 
    err       = FM_OK;
    rowsUsed  = 0;
    i         = 0;

    FM_CLEAR(srvErr);

    if (status == FM_OK)
    {
        /* Update head value in SM header */
        FM_API_CALL_FAMILY(err,
                           switchPtr->UpdateMailboxSmHdr,
                           sw,
                           pepNb,
                           ctrlHdr,
                           FM_UPDATE_CTRL_HDR_REQUEST_HEAD);

        if (err != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "fmSendHostSrvErrResponse: error while updating "
                         "SM header (err = %d)\n",
                         err);
        }
    }
    else
    {

        FM_API_CALL_FAMILY_VOID(switchPtr->SetHostSrvErrResponse,
                                sw,
                                pepNb,
                                &srvErr);

        srvErr.statusCode = status;

        FM_API_CALL_FAMILY(err,
                           switchPtr->WriteMailboxResponseMessage,
                           sw,
                           pepNb,
                           ctrlHdr,
                           msgTypeId,
                           argType,
                           (void *) &srvErr);
        if (err != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "SendHostSrvErrResponse: error while writing "
                         "response message (err = %d)\n",
                         err);
        }
    }

}   /* end fmSendHostSrvErrResponse */




/*****************************************************************************/
/** fmNotifyPvidUpdate
 * \ingroup intMailbox
 *
 * \desc            Notify host interface about pvid change.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the port on which to operate.
 *
 * \param[in]       pvid is the new pvid value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNotifyPvidUpdate(fm_int sw,
                             fm_int logicalPort,
                             fm_int pvid)
{
    fm_switch *     switchPtr;
    fm_mailboxInfo *info;
    fm_status       status;
    fm_int          pepNb;
    fm_uint32       glort;
    fm_uintptr      cachedPvid;
    fm_mailboxControlHeader controlHeader;
    fm_hostSrvUpdatePvid    updatePvid;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, logicalPort = %d, pvid = %d\n",
                 sw,
                 logicalPort,
                 pvid);

    switchPtr = GET_SWITCH_PTR(sw);
    status = FM_OK;
    glort  = -1;
    pepNb  = -1;

    FM_CLEAR(controlHeader);
    FM_CLEAR(updatePvid);

    PROTECT_SWITCH(sw);

    FM_TAKE_MAILBOX_LOCK(sw);

    status = fmGetLogicalPortGlort(sw,
                                   logicalPort,
                                   &glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    updatePvid.glort = glort;
    updatePvid.pvid  = pvid;

    info = GET_MAILBOX_INFO(sw);

    /* Cache the PVID value */
    if (fmTreeIsInitialized(&info->defaultPvidPerGlort))
    {
        status = fmTreeFind(&info->defaultPvidPerGlort,
                            glort,
                            (void **) &cachedPvid);

        if ( (status == FM_OK) && ( ( (fm_int) cachedPvid ) != pvid) )
        {
            fmTreeRemoveCertain(&info->defaultPvidPerGlort,
                                glort,
                                NULL);

            cachedPvid = pvid;

            status = fmTreeInsert(&info->defaultPvidPerGlort,
                                  glort,
                                  (void *) cachedPvid);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
        else if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "PVID value for glort 0x%x was not cached.\n"
                         "Addig to PVID cache tree.\n",
                         glort);

            cachedPvid = pvid;

            status = fmTreeInsert(&info->defaultPvidPerGlort,
                                  glort,
                                  (void *) cachedPvid);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    /* find pep number */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepNumber,
                       sw,
                       glort,
                       &pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /*************************************************************************
     * Read mailbox control header.
     *************************************************************************/
    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxControlHdr,
                       sw,
                       pepNb,
                       &controlHeader);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->WriteMailboxResponseMessage,
                       sw,
                       pepNb,
                       &controlHeader,
                       FM_MAILBOX_MSG_UPDATE_PVID_ID,
                       FM_HOST_SRV_UPDATE_PVID_TYPE,
                       (void *) &updatePvid);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_DROP_MAILBOX_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmNotifyPvidUpdate */




/*****************************************************************************/
/** fmMapLportProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for LPORT_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmMapLportProcess(fm_int                   sw,
                            fm_int                   pepNb,
                            fm_mailboxControlHeader *ctrlHdr,
                            fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *        switchPtr;
    fm_mailboxInfo *   info;
    fm_status          status;
    fm_uint32          bytesRead;
    fm_int             pepLogicalPort;
    fm_int             schedPortSpeed;
    fm_int             i;
    fm_hostSrvLportMap lportMap;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr = GET_SWITCH_PTR(sw);
    status    = FM_OK;
    bytesRead = 0;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_MAP_LPORT_TYPE,
                       FM_HOST_SRV_LPORT_MAP_TYPE_SIZE,
                       (void *) &lportMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmCleanupResourcesForPep(sw,
                                      pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetSchedPortSpeedForPep,
                       sw,
                       pepNb,
                       &schedPortSpeed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (schedPortSpeed == 0)
    {
        status = FM_ERR_PEP_PORT_NOT_SCHEDULED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Prepare response */
    FM_CLEAR(lportMap);

    FM_API_CALL_FAMILY_VOID(switchPtr->SetMailboxLportGlortRange,
                            sw,
                            pepNb,
                            &lportMap);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepLogicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    info = GET_MAILBOX_INFO(sw);

    /* Cleanup cached PVID values. */
    for (i = lportMap.glortValue ;
         i < (lportMap.glortValue + lportMap.glortsPerPep) ;
         i++)
    {
        /* Try to remove PVID value, it it's not present,
           just  go on with next values. */
        status = fmTreeRemove(&info->defaultPvidPerGlort,
                              i,
                              NULL);

        if (status == FM_OK)
        {
            continue;
        }
        else if (status == FM_ERR_NOT_FOUND)
        {
            status = FM_OK;
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

ABORT:

    if (status == FM_OK)
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->WriteMailboxResponseMessage,
                           sw,
                           pepNb,
                           ctrlHdr,
                           FM_MAILBOX_MSG_MAP_LPORT_ID,
                           FM_HOST_SRV_MAP_LPORT_TYPE,
                           (void *) &lportMap);

        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Writing response message failed with error = %d\n"
                         "(%s)\n",
                         status,
                         fmErrorMsg(status)); 

            FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Values returned from LPORT_MAP request:\n"
                     "Glort value = 0x%x, glort mask = 0x%x\n",
                     lportMap.glortValue,
                     lportMap.glortMask);
    }
    else
    {
        fmSendHostSrvErrResponse(sw,
                                 pepNb,
                                 status,
                                 ctrlHdr,
                                 FM_MAILBOX_MSG_MAP_LPORT_ID,
                                 FM_HOST_SRV_RETURN_ERR_TYPE);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}    /* end fmMapLportProcess */



 
/*****************************************************************************/
/** fmCreateLportProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for LPORT_CREATE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 * \return          FM_ERR_NO_MEM if there is no memory to allocate
 *                  mailbox resources.
 *
 *****************************************************************************/
fm_status fmCreateLportProcess(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_mailboxInfo *        info;
    fm_status               status;
    fm_status               local_status;
    fm_status               abortStatus;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_uint32               rv;
    fm_int                  index;
    fm_int                  firstPort;
    fm_int                  pepPort;
    fm_int                  pvid;
    fm_uintptr              cachedPvid;
    fm_uint32               glort;
    fm_int                  i;
    fm_hostSrvPort          srvPort;
    fm_event *              event;
    fm_eventLogicalPort *   logicalPortEvent;
    fm_mailboxResources *   mailboxResource;
    fm_portAttr *           portAttr;
    fm_bool                 portAttrLockTaken;
    fm_bool                 portsProcessed;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr         = GET_SWITCH_PTR(sw);
    info              = GET_MAILBOX_INFO(sw);
    status            = FM_OK;
    abortStatus       = FM_OK;
    bytesRead         = 0;
    argBytesRead      = 0;
    index             = 0;
    rv                = 0;
    pvid              = 0;
    pepPort           = -1;
    firstPort         = FM_LOGICAL_PORT_ANY;
    event             = NULL;
    logicalPortEvent  = NULL;
    mailboxResource   = NULL;
    portAttr          = NULL;
    portAttrLockTaken = FALSE;
    portsProcessed    = FALSE;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvPort);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_CREATE_DELETE_LPORT_TYPE,
                       FM_HOST_SRV_PORT_TYPE_SIZE,
                       (void *) &srvPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "LPORT_CREATE is trying to create %d PEP logical ports "
                 "starting at glort 0x%x\n",
                 srvPort.glortCount ,
                 srvPort.firstGlort);

    FM_API_CALL_FAMILY(status,
                       switchPtr->AllocVirtualLogicalPort,
                       sw,
                       pepNb,
                       srvPort.glortCount,
                       &firstPort,
                       0,
                       srvPort.firstGlort); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Get PVID for PEP port. */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetPortAttribute(sw,
                                pepPort,
                                FM_PORT_DEF_VLAN,
                                (void *) &pvid);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    {
        portsProcessed = TRUE;

        /* create instances to track resources created on host interface request. */
        for (i = firstPort ; i < (firstPort + srvPort.glortCount) ; i++)
        {
            mailboxResource = fmAlloc(sizeof(fm_mailboxResources));

            if (mailboxResource == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            fmTreeInit(&mailboxResource->mailboxMacResource);
            fmTreeInit(&mailboxResource->mailboxFlowResource);
            fmTreeInit(&mailboxResource->mailboxFlowTableResource);
            fmCustomTreeInit(&mailboxResource->innOutMacResource,
                             fmCompareInnerOuterMacFilters);

            mailboxResource->macEntriesAdded           = 0;
            mailboxResource->innerOuterMacEntriesAdded = 0;

            status = fmTreeInsert(&info->mailboxResourcesPerVirtualPort,
                                  i,
                                  (void *) mailboxResource);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = fmGetLogicalPortGlort(sw, i, &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = fmTreeFind(&info->defaultPvidPerGlort,
                                glort,
                                (void **) &cachedPvid);

            if (status == FM_OK)
            {
                portAttr = GET_PORT_ATTR(sw, i);
                if (portAttr->defVlan != cachedPvid)
                {
                    /* Set the cache port attribute value manually. The current
                       value is invalid because of port initialization. The
                       attribute is not changed but restored so notifying about
                       the change is undesirable. */
                    FM_FLAG_TAKE_PORT_ATTR_LOCK(sw);
                    portAttr->defVlan = cachedPvid;
                    FM_FLAG_DROP_PORT_ATTR_LOCK(sw);
                }
            }
            else if (status == FM_ERR_NOT_FOUND)
            {
                cachedPvid = pvid;

                status = fmTreeInsert(&info->defaultPvidPerGlort,
                                      glort,
                                      (void *) cachedPvid);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
            else
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cannot track used resources per virtual port "
                     "as resource tree is not initialized.\n");
    }

    event = fmAllocateEvent(sw,
                            FM_EVID_HIGH_PORT,
                            FM_EVENT_LOGICAL_PORT,
                            FM_EVENT_PRIORITY_LOW);

    if (event == NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "No event can be reported when creating "
                     "logical ports for PEP usage.\n"
                     "First port = %d, number of created ports = %d\n",
                     firstPort,
                     srvPort.glortCount);
        goto ABORT;
        
    }

    logicalPortEvent = &event->info.fpLogicalPortEvent;
    FM_CLEAR(*logicalPortEvent);

    logicalPortEvent->firstPort      = firstPort;
    logicalPortEvent->numberOfPorts  = srvPort.glortCount;
    logicalPortEvent->pepId          = pepNb;
    logicalPortEvent->pepLogicalPort = pepPort;
    logicalPortEvent->portCreated    = TRUE;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    if (status != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(event);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* As the API will send the PVID value of the first created ports,
     * check if the value is cached, otherwise use PVID of PEP port. */
    status = fmGetLogicalPortGlort(sw,
                                   firstPort,
                                   &glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmTreeFind(&info->defaultPvidPerGlort,
                        glort,
                        (void **) &cachedPvid);

    if (status == FM_OK)
    {
        pvid = cachedPvid;
    }
    else if (status != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_CREATE_LPORT_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    if (status == FM_OK)
    {
        /* Inform HNI about pvid of newly created ports */
        if ( (pvid > 0) && (pvid < FM_MAX_VLAN) )
        {
            status = fmNotifyPvidUpdate(sw,
                                        firstPort,
                                        pvid);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Update control header structure after sending UPDATE_PVID. */
            FM_API_CALL_FAMILY(status,
                               switchPtr->ReadMailboxControlHdr,
                               sw,
                               pepNb,
                               ctrlHdr);
        }
    }
    else
    {
        if ( fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort) &&
             (portsProcessed == TRUE) )
        {
            for (i = firstPort ; i < (firstPort + srvPort.glortCount) ; i++)
            {
                if (fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                               i,
                               (void **) &mailboxResource) == FM_OK)
                {
                    local_status = fmTreeRemove(&info->mailboxResourcesPerVirtualPort,
                                                i,
                                                fmFree);
                    if (local_status != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MAILBOX,
                                     "Failed to free allocated resources. "
                                     "Error=%d %s\n",
                                     local_status,
                                     fmErrorMsg(local_status));
                    }
                }
                else if (mailboxResource != NULL)
                {
                    fmFree(mailboxResource);
                    mailboxResource = NULL;
                }
                else
                {
                    break;
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmCreateLportProcess */




/*****************************************************************************/
/** fmDeleteLportProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for LPORT_DELETE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmDeleteLportProcess(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_status               searchStatus;
    fm_status               err;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_uint32               rv;
    fm_int                  firstPort;
    fm_int                  numAddresses;
    fm_int                  pepPort;
    fm_hostSrvPort          srvPort;
    fm_event *              event;
    fm_eventLogicalPort *   logicalPortEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr        = GET_SWITCH_PTR(sw);
    status           = FM_OK;
    searchStatus     = FM_OK;
    err              = FM_OK;
    bytesRead        = 0;
    argBytesRead     = 0;
    rv               = 0;
    firstPort        = 0;
    pepPort          = -1;
    event            = NULL;
    logicalPortEvent = NULL;
    numAddresses     = 0;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvPort);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_CREATE_DELETE_LPORT_TYPE,
                       FM_HOST_SRV_PORT_TYPE_SIZE,
                       (void *) &srvPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "LPORT_DELETE is trying to delete %d PEP logical ports "
                 "starting at glort 0x%x\n",
                 srvPort.glortCount ,
                 srvPort.firstGlort);

    status = fmGetGlortLogicalPort(sw,
                                   srvPort.firstGlort,
                                   &firstPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->FreeVirtualLogicalPort,
                       sw,
                       pepNb,
                       firstPort,
                       srvPort.glortCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);

    event = fmAllocateEvent(sw,
                            FM_EVID_HIGH_PORT,
                            FM_EVENT_LOGICAL_PORT,
                            FM_EVENT_PRIORITY_LOW);

    if (event == NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "No event can be reported when deleting "
                     "logical ports for PEP usage.\n"
                     "First port = %d, number of deleted ports = %d\n",
                     firstPort,
                     srvPort.glortCount);
        goto ABORT;
        
    }

    logicalPortEvent = &event->info.fpLogicalPortEvent;
    FM_CLEAR(*logicalPortEvent);

    logicalPortEvent->firstPort      = firstPort;
    logicalPortEvent->numberOfPorts  = srvPort.glortCount;
    logicalPortEvent->pepId          = pepNb;
    logicalPortEvent->pepLogicalPort = pepPort;
    logicalPortEvent->portCreated    = FALSE;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    if (status != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(event);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_DELETE_LPORT_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmDeleteLportProcess */




/*****************************************************************************/
/** fmSetXcastModesProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for XCAST_MODES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmSetXcastModesProcess(fm_int                   sw,
                                 fm_int                   pepNb,
                                 fm_mailboxControlHeader *ctrlHdr,
                                 fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *              switchPtr;
    fm_status                status;
    fm_uint32                rv;
    fm_uint32                bytesRead;
    fm_uint32                argBytesRead;
    fm_uint16                xcastFloodMode;
    fm_int                   size;
    fm_int                   logicalPort;
    fm_int                   pepPort;
    fm_hostSrvXcastMode      srvXcastMode;
    fm_intMulticastGroup *   mcastGroupForMcastFlood;
    fm_intMulticastGroup *   mcastGroupForUcastFlood;
    fm_intMulticastGroup *   mcastGroupForBcastFlood;
    fm_mailboxInfo *         info;
    fm_uint64                listenerKey;
    fm_intMulticastListener *intListener;
    fm_multicastListener     listener;
    fm_bool                  lockTaken;
    fm_bool                  mcastHNIFlooding;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    info         = GET_MAILBOX_INFO(sw);
    lockTaken    = FALSE;
    status       = FM_OK;
    bytesRead    = 0;
    rv           = 0;
    argBytesRead = 0;
    size         = 0;
    listenerKey  = 0;
    logicalPort  = 0;
    mcastGroupForMcastFlood = NULL;
    mcastGroupForUcastFlood = NULL;
    mcastGroupForBcastFlood = NULL;
    intListener             = NULL;

    FM_CLEAR(listener);

    FM_CLEAR(srvXcastMode);

    switchPtr = GET_SWITCH_PTR(sw);

    mcastHNIFlooding = GET_PROPERTY()->hniMcastFlooding;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvXcastMode);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_SET_XCAST_MODE_TYPE,
                       FM_HOST_SRV_XCAST_MODE_TYPE_SIZE,
                       (void *) &srvXcastMode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "XCAST_MODES request: "
                 "Glort value = 0x%x, xcast mode = 0x%x\n",
                 srvXcastMode.glort,
                 srvXcastMode.mode);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetGlortLogicalPort(sw,
                                   srvXcastMode.glort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    lockTaken = TRUE;    

    /* Using internal functions to avoid iterating over all listeners */
    mcastGroupForMcastFlood = fmFindMcastGroup(sw, 
                                               info->mcastGroupForMcastFlood);
    if (mcastGroupForMcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    mcastGroupForUcastFlood = fmFindMcastGroup(sw,
                                               info->mcastGroupForUcastFlood);
    if (mcastGroupForUcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    mcastGroupForBcastFlood = fmFindMcastGroup(sw,
                                               info->mcastGroupForBcastFlood);
    if (mcastGroupForBcastFlood == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    listenerKey   = GET_LISTENER_KEY(logicalPort, 
                                     FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS);

    listener.vlan = FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
    listener.port = logicalPort;

    xcastFloodMode = 0;

    switch (srvXcastMode.mode)
    {
        case FM_HOST_SRV_XCAST_MODE_ALLMULTI:

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForMcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_MCAST,
                                                      mcastGroupForMcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Update MCAST HNI flooding groups */
                if (mcastHNIFlooding)
                {
                    status = fmUpdateMcastHNIFloodingGroups(sw,
                                                            logicalPort,
                                                            TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_MCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForUcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_FLOOD,
                                                      mcastGroupForUcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_UCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForBcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_BCAST,
                                                      mcastGroupForBcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_BCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_HOST_SRV_XCAST_MODE_MULTI:

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForMcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_MCAST,
                                                      mcastGroupForMcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Update MCAST HNI flooding groups */
                if (mcastHNIFlooding)
                {
                    status = fmUpdateMcastHNIFloodingGroups(sw,
                                                            logicalPort,
                                                            FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_MCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForUcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_FLOOD,
                                                      mcastGroupForUcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_UCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForBcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_BCAST,
                                                      mcastGroupForBcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_BCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_HOST_SRV_XCAST_MODE_PROMISC:

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForMcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_MCAST,
                                                      mcastGroupForMcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Update MCAST HNI flooding groups */
                if (mcastHNIFlooding)
                {
                    status = fmUpdateMcastHNIFloodingGroups(sw,
                                                            logicalPort,
                                                            TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_MCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForUcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_FLOOD,
                                                      mcastGroupForUcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_UCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_ERR_NOT_FOUND)
            {
                status = fmAddMcastGroupListener(sw,
                                                 info->mcastGroupForBcastFlood,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_BCAST,
                                                      mcastGroupForBcastFlood,
                                                      TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_BCAST;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_HOST_SRV_XCAST_MODE_NONE:

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForMcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForMcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_MCAST,
                                                      mcastGroupForMcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Update MCAST HNI flooding groups */
                if (mcastHNIFlooding)
                {
                    status = fmUpdateMcastHNIFloodingGroups(sw,
                                                            logicalPort,
                                                            FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_MCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForUcastFlood->listenerTree,
                             listenerKey,
                             (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForUcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_FLOOD,
                                                      mcastGroupForUcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_UCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* Search the tree of listeners for a matching entry. */
            status = fmTreeFind(&mcastGroupForBcastFlood->listenerTree,
                                listenerKey,
                                (void **) &intListener);

            if (status == FM_OK)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForBcastFlood,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = AssociateMcastGroupWithFlood(sw,
                                                      pepNb,
                                                      FM_PORT_BCAST,
                                                      mcastGroupForBcastFlood,
                                                      FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                xcastFloodMode |= FM_MAILBOX_XCAST_FLOOD_MODE_BCAST;
            }
            else if (status != FM_ERR_NOT_FOUND)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->SetXcastFlooding,
                               sw,
                               pepNb,
                               srvXcastMode.mode,
                               xcastFloodMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 

            break;

        default:
            status = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

    status = FM_OK;

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_SET_XCAST_MODES_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmSetXcastModesProcess */




/*****************************************************************************/
/** fmUpdateMacFwdRuleProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for UPDATE_MAC_FWD_RULE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmUpdateMacFwdRuleProcess(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_mailboxInfo *        info;
    fm_status               status;
    fm_uint32               rv;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_uint64               key;
    fm_int                  size;
    fm_int                  logicalPort;
    fm_int                  mcastGroup;
    fm_hostSrvMACUpdate     srvMacUpdate;
    fm_macAddressEntry      macAddrEntry;
    fm_bool                 isDeleteAction;
    fm_bool                 isMacSecure;
    fm_bool                 routingLockTaken;
    fm_multicastAddress     mcastAddr;
    fm_multicastListener    listener;
    fm_mailboxResources *   mailboxResourcesUsed;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr      = GET_SWITCH_PTR(sw);
    status         = FM_OK;
    bytesRead      = 0;
    rv             = 0;
    argBytesRead   = 0;
    size           = 0;
    logicalPort    = 0;
    mcastGroup     = -1;
    isDeleteAction = TRUE;
    isMacSecure    = TRUE;
    routingLockTaken = FALSE;
    mailboxResourcesUsed = NULL;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvMacUpdate);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_UPDATE_MAC_TYPE,
                       FM_HOST_SRV_MAC_UPDATE_TYPE_SIZE,
                       (void *) &srvMacUpdate);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Check action type: ADD or DELETE */
    isDeleteAction = FM_GET_BIT(srvMacUpdate.action,
                                FM_HOST_SRV_MAC_UPDATE,
                                ADD_DELETE);

    /* Check if MAC is secure */
    isMacSecure = FM_GET_BIT(srvMacUpdate.flags,
                             FM_HOST_SRV_MAC_UPDATE,
                             MAC_SECURE);

    /* Get logical port number */
    status = fmGetGlortLogicalPort(sw,
                                   srvMacUpdate.glort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(macAddrEntry);
    macAddrEntry.macAddress = ( (fm_macaddr) srvMacUpdate.macAddressUpper << 
                                FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) |
                                srvMacUpdate.macAddressLower;
    macAddrEntry.vlanID     = srvMacUpdate.vlan;
    macAddrEntry.type       = (isMacSecure) ?
                                FM_ADDRESS_SECURE_STATIC : FM_ADDRESS_STATIC;
    macAddrEntry.destMask   = FM_DESTMASK_UNUSED;
    macAddrEntry.port       = logicalPort;

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "Processing MAC %012llx on VLAN %d\n",
                 macAddrEntry.macAddress, macAddrEntry.vlanID);

    info = GET_MAILBOX_INFO(sw);

    if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    {
        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxResourcesUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else
    {
         FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                      "Cannot track used resources per virtual port "
                      "as resource tree is not initialized.\n");
    }

    key = GET_MACVLAN_RESOURCE_KEY(macAddrEntry.macAddress,
                                   macAddrEntry.vlanID);

    if (fmIsMulticastMacAddress(macAddrEntry.macAddress))
    {
        FM_CLEAR(mcastAddr);
        FM_CLEAR(listener);

        mcastAddr.addressType = FM_MCAST_ADDR_TYPE_L2MAC_VLAN;
        mcastAddr.info.mac.destMacAddress = macAddrEntry.macAddress;
        mcastAddr.info.mac.vlan = srvMacUpdate.vlan;

        status = fmFindMcastGroupByAddress(sw,
                                           &mcastAddr,
                                           &mcastGroup);

        if (status == FM_ERR_MCAST_ADDR_NOT_ASSIGNED)
        {
            mcastGroup = -1;
        }
        else if (status != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        if (mcastGroup == -1)
        {
            if (isDeleteAction)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
            else
            {
                status = fmCreateMcastGroup(sw,
                                            &mcastGroup);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                mcastAddr.mcastGroup = mcastGroup;

                status = fmSetMcastGroupAddress(sw,
                                                mcastGroup,
                                                &mcastAddr);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                info->macEntriesAdded[pepNb]++;

                listener.port = macAddrEntry.port;
                listener.vlan = macAddrEntry.vlanID;

                status = fmAddMcastGroupListener(sw,
                                                 mcastGroup,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = fmActivateMcastGroup(sw,
                                              mcastGroup);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                if ( (mailboxResourcesUsed != NULL) &&
                    fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
                {
                    /* Add mcast group to tracked resources list */
                    status = fmTreeInsert(&mailboxResourcesUsed->mailboxMacResource,
                                          key,
                                          NULL);

                    if (status == FM_ERR_ALREADY_EXISTS)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                     "Given MAC key (0x%llx) already tracked with"
                                     " port %d\n",
                                     key,
                                     logicalPort);
                    }
                    else if (status != FM_OK)
                    {
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    }
                }
            }
        }
        else
        {
            listener.port = macAddrEntry.port;
            listener.vlan = macAddrEntry.vlanID;

            if (isDeleteAction)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    mcastGroup,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                if ( (mailboxResourcesUsed != NULL) &&
                    fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
                {
                    /* Delete mcast group from tracked resources list */
                    status = fmTreeRemove(&mailboxResourcesUsed->mailboxMacResource,
                                          key,
                                          NULL);

                    if (status == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                     "Given MAC key (0x%llx) was not tracked with"
                                     " port %d\n",
                                     key,
                                     logicalPort);
                    }
                    else if (status != FM_OK)
                    {
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    }
                }

                /* Take routing lock needed by fmHasMcastGroupNonFloodingListeners */
                status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                routingLockTaken = TRUE;

                if ( !fmHasMcastGroupNonFloodingListeners(sw,
                                                          mcastGroup) )
                {
                    status = fmDeactivateMcastGroup(sw,
                                                    mcastGroup);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = fmDeleteMcastGroup(sw,
                                                mcastGroup);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    info->macEntriesAdded[pepNb]--;
                }

                status = fmReleaseWriteLock(&switchPtr->routingLock);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                routingLockTaken = FALSE;
            }
            else
            {
                status = fmAddMcastGroupListener(sw,
                                                 mcastGroup,
                                                 &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                if ( (mailboxResourcesUsed != NULL) &&
                    fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
                {
                    /* Add mcast group to tracked resources list */
                    status = fmTreeInsert(&mailboxResourcesUsed->mailboxMacResource,
                                          key,
                                          NULL);

                    if (status == FM_ERR_ALREADY_EXISTS)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                     "Given MAC key (0x%llx) already tracked with"
                                     " port %d\n",
                                     key,
                                     logicalPort);
                    }
                    else if (status != FM_OK)
                    {
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    }
                }
            }
        }

    }
    else
    {
        if (isDeleteAction)
        {
            status = fmDeleteAddress(sw, &macAddrEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            info->macEntriesAdded[pepNb]--;

            if ( (mailboxResourcesUsed != NULL) &&
                    fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
            {
                /* Delete MAC address from tracked resources list */
                status = fmTreeRemove(&mailboxResourcesUsed->mailboxMacResource,
                                      key,
                                      NULL);

                if (status == FM_ERR_NOT_FOUND)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Given MAC key (0x%llx) was not tracked with"
                                 " port %d\n",
                                 key,
                                 logicalPort);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }
        }
        else
        {
            status = fmAddAddress(sw, &macAddrEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            info->macEntriesAdded[pepNb]++;

            if ( (mailboxResourcesUsed != NULL) &&
                    fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
            {
                /* Add MAC address to tracked resources list */
                status = fmTreeInsert(&mailboxResourcesUsed->mailboxMacResource,
                                      key,
                                      NULL);

                if (status == FM_ERR_ALREADY_EXISTS)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Given MAC key (0x%llx) already tracked with"
                                 " port %d\n",
                                 key,
                                 logicalPort);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }
        }
    }

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_UPDATE_MAC_FWD_RULE_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmUpdateMacFwdRuleProcess */




/*****************************************************************************/
/** fmConfigReqProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for CONFIG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmConfigReqProcess(fm_int                   sw,
                             fm_int                   pepNb,
                             fm_mailboxControlHeader *ctrlHdr,
                             fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_uint32               rv;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_int                  size;
    fm_int                  pepPort;
    fm_bool                 value;
    fm_hostSrvConfig        srvConfig;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr    = GET_SWITCH_PTR(sw);
    status       = FM_OK;
    bytesRead    = 0;
    rv           = 0;
    argBytesRead = 0;
    size         = 0;
    value        = FM_ENABLED;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvConfig);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_REQUEST_CONFIG_TYPE,
                       FM_HOST_SRV_CONFIG_TYPE_SIZE,
                       (void *) &srvConfig);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    switch (srvConfig.configurationAttributeConstant)
    {
        case FM_MAILBOX_CONFIG_PEP_LEARNING:

            if (srvConfig.value == 0)
            {
                value = FM_DISABLED;
            }
            else
            {
                value = FM_ENABLED;
            }

            status = fmSetPortAttribute(sw,
                                        pepPort,
                                        FM_PORT_LEARNING,
                                        &value);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        default:
            status = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_CONFIG_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmConfigReqProcess */




/*****************************************************************************/
/** fmCreateFlowTableProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for CREATE_FLOW_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmCreateFlowTableProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *               switchPtr;
    fm_mailboxInfo *          info;
    fm_status                 status;
    fm_uint32                 rv;
    fm_uint32                 bytesRead;
    fm_uint32                 argBytesRead;
    fm_uint16                 pfGlort;
    fm_int                    size;
    fm_int                    logicalPort;
    fm_hostSrvLportMap        lportMap; /* Need to get glort range for given PEP*/
    fm_hostSrvCreateFlowTable srvCreateTable;
    fm_flowCondition          condition;
    fm_bool                   tableWithPriority;
    fm_mailboxResources *     mailboxResourcesUsed;
    fm_uintptr                tableType;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr         = GET_SWITCH_PTR(sw);
    info              = GET_MAILBOX_INFO(sw);
    status            = FM_OK;
    bytesRead         = 0;
    rv                = 0;
    argBytesRead      = 0;
    size              = 0;
    condition         = 0;
    pfGlort           = 0;
    logicalPort       = -1;
    tableWithPriority = TRUE;
    mailboxResourcesUsed = NULL;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvCreateTable);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_CREATE_FLOW_TABLE_TYPE,
                       FM_HOST_SRV_CREATE_FLOW_TABLE_TYPE_SIZE,
                       (void *) &srvCreateTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (srvCreateTable.tableType == FM_FLOW_TCAM_TABLE)
    {
        status = fmSetFlowAttribute(sw,
                                    srvCreateTable.tableIndex,
                                    FM_FLOW_TABLE_WITH_PRIORITY,
                                    &tableWithPriority);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    condition = srvCreateTable.flowConditionBitmaskUpper;
    condition = condition << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH;
    condition |= (fm_flowCondition)srvCreateTable.flowConditionBitmaskLower;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ProcessCreateFlowTableRequest,
                       sw,
                       pepNb,
                       srvCreateTable.tableIndex,
                       srvCreateTable.tableType,
                       condition);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(lportMap);

    FM_API_CALL_FAMILY_VOID(switchPtr->SetMailboxLportGlortRange,
                            sw,
                            pepNb,
                            &lportMap);

    pfGlort = CALCULATE_PF_GLORT_VALUE(lportMap.glortValue);

    status = fmGetGlortLogicalPort(sw,
                                   pfGlort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    {
        /* FLOW resources are tracked per logical port
           associated with PF glort. */
        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxResourcesUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        tableType = srvCreateTable.tableType;

        if ( (mailboxResourcesUsed != NULL) &&
            fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowTableResource))
        {
            /* Add flow group to tracked resources list */
            status = fmTreeInsert(&mailboxResourcesUsed->mailboxFlowTableResource,
                                  srvCreateTable.tableIndex,
                                  (void *) tableType);

            if (status == FM_ERR_ALREADY_EXISTS)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Given flow table (index=0x%x, type=%d) already tracked with"
                             " port %d\n",
                             srvCreateTable.tableIndex,
                             srvCreateTable.tableType,
                             logicalPort);
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cannot track used resources per virtual port "
                     "as resource tree is not initialized.\n");
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_CREATE_FLOW_TABLE_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmCreateFlowTableProcess */




/*****************************************************************************/
/** fmDeleteFlowTableProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for DELETE_FLOW_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmDeleteFlowTableProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *               switchPtr;
    fm_mailboxInfo *          info;
    fm_status                 status;
    fm_uint64                 key;
    fm_uint32                 rv;
    fm_uint32                 bytesRead;
    fm_uint32                 argBytesRead;
    fm_uint16                 pfGlort;
    fm_uint16                 flowTableIndex;
    fm_int                    size;
    fm_int                    logicalPort;
    fm_int                    flowId;
    fm_hostSrvLportMap        lportMap; /* Need to get glort range for given PEP*/
    fm_hostSrvDeleteFlowTable srvDeleteTable;
    fm_mailboxResources *     mailboxResourcesUsed;
    fm_treeIterator           treeIterMailboxRes;
    void *                    nextValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr      = GET_SWITCH_PTR(sw);
    info           = GET_MAILBOX_INFO(sw);
    status         = FM_OK;
    bytesRead      = 0;
    rv             = 0;
    argBytesRead   = 0;
    size           = 0;
    pfGlort        = 0;
    logicalPort    = -1;
    flowId         = -1;
    flowTableIndex = 0;
    key            = 0;
    mailboxResourcesUsed = NULL;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvDeleteTable);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_DELETE_FLOW_TABLE_TYPE,
                       FM_HOST_SRV_DELETE_FLOW_TABLE_TYPE_SIZE,
                       (void *) &srvDeleteTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    switch (srvDeleteTable.tableType)
    {
        case FM_FLOW_TCAM_TABLE:
            status = fmDeleteFlowTCAMTable(sw,
                                           srvDeleteTable.tableIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_FLOW_BST_TABLE:
            status = fmDeleteFlowBSTTable(sw,
                                          srvDeleteTable.tableIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_FLOW_TE_TABLE:
            status = fmDeleteFlowTETable(sw,
                                         srvDeleteTable.tableIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

    FM_CLEAR(lportMap);

    FM_API_CALL_FAMILY_VOID(switchPtr->SetMailboxLportGlortRange,
                            sw,
                            pepNb,
                            &lportMap);

    pfGlort = CALCULATE_PF_GLORT_VALUE(lportMap.glortValue);

    status = fmGetGlortLogicalPort(sw,
                                   pfGlort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    {
        /* FLOW resources are tracked per logical port
           associated with PF glort. */
        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxResourcesUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        if ( (mailboxResourcesUsed != NULL) &&
            fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowTableResource) )
        {
            /* Delete flow table from tracked resources list */
            status = fmTreeRemove(&mailboxResourcesUsed->mailboxFlowTableResource,
                                  srvDeleteTable.tableIndex,
                                  NULL);

            if (status == FM_ERR_NOT_FOUND)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Given flow table (index=0x%x, type=%d) was not tracked with"
                             " port %d\n",
                             srvDeleteTable.tableIndex,
                             srvDeleteTable.tableType,
                             logicalPort);
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }

        if ( (mailboxResourcesUsed != NULL) &&
            fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowResource) )
        {
            /* Cleanup Flow entries added to this table. */
            fmTreeIterInit(&treeIterMailboxRes,
                           &mailboxResourcesUsed->mailboxFlowResource);

            while ( fmTreeIterNext(&treeIterMailboxRes,
                                   &key,
                                   &nextValue) != FM_ERR_NO_MORE )
            {
                flowId         = GET_FLOW_ID_FROM_FLOW_RESOURCE_KEY(key);
                flowTableIndex = GET_FLOW_TABLE_FROM_FLOW_RESOURCE_KEY(key);

                if (flowTableIndex == srvDeleteTable.tableIndex)
                {
                    fmTreeRemoveCertain(&mailboxResourcesUsed->mailboxFlowResource,
                                        key,
                                        NULL);

                    /* Re-initialize the iterator because we modified the tree */
                    fmTreeIterInitFromSuccessor(&treeIterMailboxRes, 
                                                &mailboxResourcesUsed->mailboxFlowResource,
                                                0);
                }
            }
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cannot track used resources per virtual port "
                     "as resource tree is not initialized.\n");
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_DELETE_FLOW_TABLE_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmDeleteFlowTableProcess */




/*****************************************************************************/
/** fmUpdateFlowProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for UPDATE_FLOW.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmUpdateFlowProcess(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_mailboxInfo *        info;
    fm_status               status;
    fm_uint16               pfGlort;
    fm_uint32               rv;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_int                  size;
    fm_int                  offset;
    fm_int                  flowId;
    fm_int                  logicalPort;
    fm_hostSrvLportMap      lportMap; /* Need to get glort range for given PEP*/
    fm_hostSrvUpdateFlow    srvUpdateFlow;
    fm_hostSrvFlowHandle    srvFlowHandle;
    fm_flowCondition        condition;
    fm_flowAction           action;
    fm_flowValue            condVal;
    fm_flowParam            flowParam;
    fm_macaddr              macAddr;
    fm_mailboxResources *   mailboxResourcesUsed;
    fm_uint64               key;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr    = GET_SWITCH_PTR(sw);
    info         = GET_MAILBOX_INFO(sw);
    status       = FM_OK;
    bytesRead    = 0;
    rv           = 0;
    argBytesRead = 0;
    offset       = 0;
    size         = 0;
    condition    = 0;
    action       = 0;
    macAddr      = 0;
    key          = 0;
    pfGlort      = 0;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(condVal);
    FM_CLEAR(flowParam);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvUpdateFlow);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_UPDATE_FLOW_TYPE,
                       0, /*message size is variable for UPDATE_FLOW */
                       (void *) &srvUpdateFlow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (srvUpdateFlow.flowID == FM_MAILBOX_FLOW_ID_ADD_NEW)
    {
        status = fmAddFlow(sw,
                           srvUpdateFlow.tableIndex,
                           srvUpdateFlow.priority,
                           0,
                           srvUpdateFlow.condition,
                           &srvUpdateFlow.flowVal,
                           srvUpdateFlow.action,
                           &srvUpdateFlow.flowParam,
                           FM_FLOW_STATE_ENABLED,
                           &flowId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);          

        srvUpdateFlow.flowID = flowId;

        FM_CLEAR(lportMap);

        FM_API_CALL_FAMILY_VOID(switchPtr->SetMailboxLportGlortRange,
                                sw,
                                pepNb,
                                &lportMap);

        pfGlort = CALCULATE_PF_GLORT_VALUE(lportMap.glortValue);

        status = fmGetGlortLogicalPort(sw,
                                       pfGlort,
                                       &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
        {
            /* FLOW resources are tracked per logical port
               associated with PF glort. */
            status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                                logicalPort,
                                (void **) &mailboxResourcesUsed);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            if ( (mailboxResourcesUsed != NULL) &&
                fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowResource) )
            {
                key = GET_FLOW_RESOURCE_KEY(srvUpdateFlow.flowID,
                                            srvUpdateFlow.tableIndex); 

                /* Add flow group to tracked resources list */
                status = fmTreeInsert(&mailboxResourcesUsed->mailboxFlowResource,
                                      key,
                                      NULL);

                if (status == FM_ERR_ALREADY_EXISTS)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Given FLOW key (0x%llx) already tracked with"
                                 " port %d\n",
                                 key,
                                 logicalPort);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Cannot track used resources per virtual port "
                         "as resource tree is not initialized.\n");
        }
    }
    else
    {
        status = fmModifyFlow(sw,
                              srvUpdateFlow.tableIndex,
                              srvUpdateFlow.flowID,
                              srvUpdateFlow.priority,
                              0,
                              srvUpdateFlow.condition,
                              &srvUpdateFlow.flowVal,
                              srvUpdateFlow.action,
                              &srvUpdateFlow.flowParam);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }


ABORT:

    /* Prepare response */
    FM_CLEAR(srvFlowHandle);

    if (status == FM_OK)
    {
        srvFlowHandle.flowID = srvUpdateFlow.flowID;
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->WriteMailboxResponseMessage,
                       sw,
                       pepNb,
                       ctrlHdr,
                       FM_MAILBOX_MSG_UPDATE_FLOW_ID,
                       FM_HOST_SRV_HANDLE_FLOW_TYPE,
                       (void *) &srvFlowHandle);
        
    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmUpdateFlowProcess */




/*****************************************************************************/
/** fmDeleteFlowProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for DELETE_FLOW.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmDeleteFlowProcess(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_mailboxInfo *        info;
    fm_status               status;
    fm_uint16               pfGlort;
    fm_uint32               rv;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_hostSrvDeleteFlow    srvDeleteFlow;
    fm_mailboxResources *   mailboxResourcesUsed;
    fm_uint64               key;
    fm_int                  logicalPort;
    fm_hostSrvLportMap      lportMap; /* Need to get glort range for given PEP*/

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr    = GET_SWITCH_PTR(sw);
    info         = GET_MAILBOX_INFO(sw);
    status       = FM_OK;
    bytesRead    = 0;
    rv           = 0;
    argBytesRead = 0;
    key          = 0;
    pfGlort      = 0;
    logicalPort  = -1;
    mailboxResourcesUsed = NULL;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvDeleteFlow);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_DELETE_FLOW_TYPE,
                       FM_HOST_SRV_DELETE_FLOW_TYPE_SIZE,
                       (void *) &srvDeleteFlow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmDeleteFlow(sw,
                          srvDeleteFlow.tableIndex,
                          srvDeleteFlow.flowID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(lportMap);

    FM_API_CALL_FAMILY_VOID(switchPtr->SetMailboxLportGlortRange,
                            sw,
                            pepNb,
                            &lportMap);

    pfGlort = CALCULATE_PF_GLORT_VALUE(lportMap.glortValue);

    status = fmGetGlortLogicalPort(sw,
                                   pfGlort,
                                   &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    {
        /* FLOW resources are tracked per logical port
           associated with PF glort. */
        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxResourcesUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        if ( (mailboxResourcesUsed != NULL) &&
            fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowResource) )
        {
            key = GET_FLOW_RESOURCE_KEY(srvDeleteFlow.flowID,
                                        srvDeleteFlow.tableIndex);

            /* Delete flow group from tracked resources list */
            status = fmTreeRemove(&mailboxResourcesUsed->mailboxFlowResource,
                                  key,
                                  NULL);

            if (status == FM_ERR_NOT_FOUND)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Given FLOW key (0x%llx) was not tracked with"
                             " port %d\n",
                             key,
                             logicalPort);
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cannot track used resources per virtual port "
                     "as resource tree is not initialized.\n");
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_DELETE_FLOW_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmDeleteFlowProcess */




/*****************************************************************************/
/** fmSetFlowStateProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for SET_FLOW_STATE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmSetFlowStateProcess(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *ctrlHdr,
                                fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_uint32               rv;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_int                  size;
    fm_hostSrvFlowState     srvFlowState;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr       = GET_SWITCH_PTR(sw);
    status          = FM_OK;
    bytesRead       = 0;
    rv              = 0;
    argBytesRead    = 0;
    size            = 0;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(srvFlowState);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_SET_FLOW_STATE_TYPE,
                       FM_HOST_SRV_FLOW_STATE_TYPE_SIZE,
                       (void *) &srvFlowState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmSetFlowState(sw,
                            srvFlowState.tableIndex,
                            srvFlowState.flowID,
                            srvFlowState.flowState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);


ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_SET_FLOW_STATE_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmSetFlowStateProcess */




/*****************************************************************************/
/** fmAnnounceTxTimestampMode
 * \ingroup intMailbox
 *
 * \desc            Process TX_TIMESTAMP_MODE message.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isTxTimestampEnabled specifies whether TX_TIMESTAMP_MODE 
 *                  is enabled/disabled.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAnnounceTxTimestampMode(fm_int  sw,
                                    fm_bool isTxTimestampEnabled)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_bool    switchLockTaken;
    fm_bool    mailboxLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, isTxTimestampEnabled=%d\n",
                 sw,
                 isTxTimestampEnabled);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);
    switchLockTaken = TRUE;

    switchPtr        = GET_SWITCH_PTR(sw);
    status           = FM_OK;
    mailboxLockTaken = FALSE;

    /* Take mailbox lock */
    FM_TAKE_MAILBOX_LOCK(sw);
    mailboxLockTaken = TRUE;

    FM_API_CALL_FAMILY(status,
                       switchPtr->MailboxAnnounceTxTimestampMode,
                       sw,
                       isTxTimestampEnabled);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    if (mailboxLockTaken)
    {
        FM_DROP_MAILBOX_LOCK(sw);
    }

    if (switchLockTaken)
    {
        UNPROTECT_SWITCH(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmAnnounceTxTimestampMode */




/*****************************************************************************/
/** fmMasterClkOffsetProcess
 * \ingroup intMailbox
 *
 * \desc            Process MASTER_CLK_OFFSET message.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMasterClkOffsetProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_status  status;
    fm_int     logicalPort;
    fm_int     timestampOwnerPep;
    fm_uint32  glort;
    fm_hostSrvMasterClkOffset clkOffset;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr        = GET_SWITCH_PTR(sw);
    status           = FM_OK;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE,
                       FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE,
                       (void *) &clkOffset);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetSwitchAttribute(sw,
                                  FM_SWITCH_ETH_TIMESTAMP_OWNER,
                                  &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortGlort(sw,
                                   logicalPort,
                                   &glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    portPtr = GET_PORT_PTR(sw, logicalPort);

    if (portPtr->portType != FM_PORT_TYPE_VIRTUAL)
    {
        status = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* find pep number */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepNumber,
                       sw,
                       glort,
                       &timestampOwnerPep);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Resend the message if timestamp owner pep is the receiving one,
     * otherwise ignore the message */
    if (pepNb == timestampOwnerPep)
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->ProcessMasterClkOffset,
                           sw,
                           pepNb,
                           &clkOffset);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_MASTER_CLK_OFFSET_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmMasterClkOffsetProcess */




/*****************************************************************************/
/** fmFilterInnerOuterMacProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for FILTER_INNER_OUTER_MAC.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmFilterInnerOuterMacProcess(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *          switchPtr;
    fm_status            status;
    fm_hostSrvInnOutMac  macFilter;
    fm_hostSrvInnOutMac *macFilterKey;
    fm_int               filterPep;
    fm_int               pepPort;
    fm_uint32            parseCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr    = GET_SWITCH_PTR(sw);
    status       = FM_OK;
    macFilterKey = NULL;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(macFilter);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_INN_OUT_MAC_TYPE,
                       FM_HOST_SRV_INN_OUT_MAC_TYPE_SIZE,
                       (void *) &macFilter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Validate if glort value fit the receivers pep glort range */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepNumber,
                       sw,
                       macFilter.glort,
                       &filterPep);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (filterPep != pepNb)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Validate if pep port parser is set to L4. If not, just set it. */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepLogicalPort,
                       sw,
                       macFilter.glort,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetPortAttribute(sw,
                                pepPort,
                                FM_PORT_PARSER,
                                &parseCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (parseCfg != FM_PORT_PARSER_STOP_AFTER_L4)
    {
        parseCfg = FM_PORT_PARSER_STOP_AFTER_L4;

        status = fmSetPortAttribute(sw,
                                    pepPort,
                                    FM_PORT_PARSER,
                                    &parseCfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    if (macFilter.action == FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_ADD)
    {
        /* When adding new filtering rule, allocate new structure 
           as it will be stored in the tree. */
        macFilterKey = fmAlloc( sizeof(fm_hostSrvInnOutMac) );

        if (macFilterKey == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT(FM_LOG_CAT_MAILBOX, status);
        }

        macFilterKey->outerMacAddr     = macFilter.outerMacAddr;
        macFilterKey->outerMacAddrMask = macFilter.outerMacAddrMask;
        macFilterKey->innerMacAddr     = macFilter.innerMacAddr;
        macFilterKey->innerMacAddrMask = macFilter.innerMacAddrMask;
        macFilterKey->vni              = macFilter.vni;
        macFilterKey->outerL4Port      = macFilter.outerL4Port;
        macFilterKey->tunnelType       = macFilter.tunnelType;
        macFilterKey->action           = macFilter.action;
        macFilterKey->glort            = macFilter.glort;


        status = AddMacFilterAclRule(sw,
                                     pepNb,
                                     macFilterKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else if (macFilter.action == FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_DEL)
    {
        status = DeleteMacFilterAclRule(sw,
                                        pepNb,
                                        &macFilter);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (macFilterKey != NULL)
        {
            fmFree(macFilterKey);
        }
    }

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_INN_OUT_MAC_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmFilterInnerOuterMacProcess */




/*****************************************************************************/
/** fmSetInnerOuterMacFilter
 * \ingroup intMailbox
 *
 * \desc            This function is created for testing purposes.
 *                  It emulates the FILTER_INNER_OUTER_MAC message.
 *                  User can create/delete inner/outer MAC filters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number on which message is emulated.
 *
 * \param[in]       macFilter points to a structure containing fields 
 *                  to be filtered. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetInnerOuterMacFilter(fm_int               sw,
                                   fm_int               pepNb,
                                   fm_hostSrvInnOutMac *macFilter)
{
    fm_switch *          switchPtr;
    fm_status            status;
    fm_hostSrvInnOutMac *macFilterKey;
    fm_int               filterPep;
    fm_int               pepPort;
    fm_uint32            parseCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, macFilter=%p\n",
                 sw,
                 pepNb,
                 (void *) macFilter);

    switchPtr    = GET_SWITCH_PTR(sw);
    status       = FM_OK;
    macFilterKey = NULL;

    LOCK_SWITCH(sw);
    FM_TAKE_MAILBOX_LOCK(sw);

    /* Validate if glort value fit the receivers pep glort range */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepNumber,
                       sw,
                       macFilter->glort,
                       &filterPep);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (filterPep != pepNb)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Validate if pep port parser is set to L4. If not, just set it. */
    FM_API_CALL_FAMILY(status,
                       switchPtr->MapVirtualGlortToPepLogicalPort,
                       sw,
                       macFilter->glort,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetPortAttribute(sw,
                                pepPort,
                                FM_PORT_PARSER,
                                &parseCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (parseCfg != FM_PORT_PARSER_STOP_AFTER_L4)
    {
        parseCfg = FM_PORT_PARSER_STOP_AFTER_L4;

        status = fmSetPortAttribute(sw,
                                    pepPort,
                                    FM_PORT_PARSER,
                                    &parseCfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    if (macFilter->action == FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_ADD)
    {
        /* When adding new filtering rule, allocate new structure 
           as it will be stored in the tree. */
        macFilterKey = fmAlloc( sizeof(fm_hostSrvInnOutMac) );

        if (macFilterKey == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT(FM_LOG_CAT_MAILBOX, status);
        }

        macFilterKey->outerMacAddr     = macFilter->outerMacAddr;
        macFilterKey->outerMacAddrMask = macFilter->outerMacAddrMask;
        macFilterKey->innerMacAddr     = macFilter->innerMacAddr;
        macFilterKey->innerMacAddrMask = macFilter->innerMacAddrMask;
        macFilterKey->vni              = macFilter->vni;
        macFilterKey->outerL4Port      = macFilter->outerL4Port;
        macFilterKey->tunnelType       = macFilter->tunnelType;
        macFilterKey->action           = macFilter->action;
        macFilterKey->glort            = macFilter->glort;


        status = AddMacFilterAclRule(sw,
                                     pepNb,
                                     macFilterKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else if (macFilter->action == FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_DEL)
    {
        status = DeleteMacFilterAclRule(sw,
                                        pepNb,
                                        macFilter);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    if (status != FM_OK)
    {
        if (macFilterKey != NULL)
        {
            fmFree(macFilterKey);
        }
    }

    FM_DROP_MAILBOX_LOCK(sw);
    UNLOCK_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmSetInnerOuterMacFilter */




/*****************************************************************************/
/** fmUnknownRequestProcess
 * \ingroup intMailbox
 *
 * \desc            Handle unknown request.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUnknownRequestProcess(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_uint32  bytesRead;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr = GET_SWITCH_PTR(sw);
    status    = FM_OK;
    bytesRead = 0;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       -1, /* Unknown message type*/
                       0, /* Unknown size */
                       NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             FM_ERR_INVALID_TRANSACTION_ID,
                             ctrlHdr,
                             -1, /* unknown msg ID */
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmUnknownRequestProcess */




/*****************************************************************************/
/** fmProcessLoopbackRequest
 * \ingroup intMailbox
 *
 * \desc            Process mailbox loopback requests.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       controlHeader points to mailbox control header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmProcessLoopbackRequest(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *controlHeader)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr = GET_SWITCH_PTR(sw);
    status = FM_OK;

    FM_API_CALL_FAMILY(status,
                       switchPtr->ProcessMailboxLoopbackRequest,
                       sw,
                       pepNb,
                       controlHeader);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* fmProcessLoopbackRequest */



/*****************************************************************************/
/** fmMailboxAllocateDataStructures
 * \ingroup intMailbox
 *
 * \desc            Allocates mailbox data structures upon switch insertion.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMailboxAllocateDataStructures(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status    = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->MailboxAllocateDataStructures == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->MailboxAllocateDataStructures,
                       sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmMailboxAllocateDataStructures */




/*****************************************************************************/
/** fmMailboxInit
 * \ingroup intMailbox
 *
 * \desc            This function is called during switch initialization to
 *                  prepare for mailbox support.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMailboxInit(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    switchPtr = GET_SWITCH_PTR(sw);
    status    = FM_OK;
    info      = GET_MAILBOX_INFO(sw);

    /* If mailbox is not supported, exit */
    if (switchPtr->MailboxInit == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);
    }

    /* Perform initialization in single chip configuration
       or for aggregate switch. */
    if ( (GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)) == sw)
    {
        fmTreeInit(&info->mailboxResourcesPerVirtualPort);
        fmTreeInit(&info->defaultPvidPerGlort);
        fmCustomTreeInit(&info->mcastMacVni,
                         fmCompareMcastMacVniKeys);

        info->aclIdForMacFiltering = FM_MAILBOX_MAC_FILTER_ACL;

        FM_API_CALL_FAMILY(status,
                           switchPtr->MailboxConfigureCounters,
                           sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmMailboxInit */




/*****************************************************************************/
/** fmMailboxFreeDataStructures
 * \ingroup intMailbox
 *
 * \desc            Free mailbox data structures upon switch removal.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMailboxFreeDataStructures(fm_int sw)
{
    fm_status  status;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->MailboxFreeDataStructures == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->MailboxFreeDataStructures,
                       sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmMailboxFreeDataStructures */




/*****************************************************************************/
/** fmMailboxFreeResources
 * \ingroup intMailbox
 *
 * \desc            Release mailbox resources held by a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMailboxFreeResources(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    switchPtr = GET_SWITCH_PTR(sw);
    status = FM_OK;
    info = GET_MAILBOX_INFO(sw);

    /* If mailbox is not supported, exit */
    if (switchPtr->MailboxFreeResources == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);
    }

    /* Perform tree cleanup in single chip configuration
       or for aggregate switch. */
    if ( (GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)) == sw)
    {
        fmTreeDestroy(&info->mailboxResourcesPerVirtualPort,
                      fmFree);
        fmTreeDestroy(&info->defaultPvidPerGlort,
                      NULL);
        fmCustomTreeDestroy(&info->mcastMacVni,
                            fmFreeMcastMacVni);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmMailboxFreeResources */




/*****************************************************************************/
/** fmDeliverPacketTimestamp
 * \ingroup intMailbox
 *
 * \desc            Process DELIVER_PACKET_TIMESTAMP message.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sglort is the source glort on which to operate.
 *
 * \param[in]       dglort is the destination glort on which to operate. 
 *
 * \param[in]       egressTimestamp is the egress timestamp value.
 *
 * \param[in]       ingressTimestamp is the ingress timestamp value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDeliverPacketTimestamp(fm_int    sw,
                                   fm_uint16 sglort,
                                   fm_uint16 dglort,
                                   fm_uint64 egressTimestamp,
                                   fm_uint64 ingressTimestamp)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     pepNb;
    fm_int     logicalPort;
    fm_bool    switchLockTaken;
    fm_bool    mailboxLockTaken;
    fm_mailboxControlHeader   controlHeader;
    fm_hostSrvPacketTimestamp packetTimestamp;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, sglort=0x%x, dglort=0x%x "
                 "egressTimestamp=0x%llx, ingressTimestamp=0x%llx\n",
                 sw,
                 sglort,
                 dglort,
                 egressTimestamp,
                 ingressTimestamp);

    switchPtr = GET_SWITCH_PTR(sw);
    status  = FM_OK;
    pepNb   = -1;
    logicalPort = -1;

    switchLockTaken  = FALSE;
    mailboxLockTaken = FALSE;

    FM_CLEAR(controlHeader);
    FM_CLEAR(packetTimestamp);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);
    switchLockTaken = TRUE;

    /* find pep number */
    if (sglort == switchPtr->glortInfo.cpuBase)
    {
        status = fmGetCpuPort(sw,
                              &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        FM_API_CALL_FAMILY(status,
                           switchPtr->MapLogicalPortToPep,
                           sw,
                           logicalPort,
                           &pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 
    }
    else
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->MapVirtualGlortToPepNumber,
                           sw,
                           sglort,
                           &pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    packetTimestamp.sglort           = sglort;
    packetTimestamp.dglort           = dglort;
    packetTimestamp.egressTimestamp  = egressTimestamp;
    packetTimestamp.ingressTimestamp = ingressTimestamp;

    FM_TAKE_MAILBOX_LOCK(sw);
    mailboxLockTaken = TRUE;

    /*************************************************************************
     * Read mailbox control header.
     *************************************************************************/
    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxControlHdr,
                       sw,
                       pepNb,
                       &controlHeader);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->WriteMailboxResponseMessage,
                       sw,
                       pepNb,
                       &controlHeader,
                       FM_MAILBOX_MSG_DELIVER_PACKET_TIMESTAMP_ID,
                       FM_HOST_SRV_PACKET_TIMESTAMP_TYPE,
                       (void *) &packetTimestamp);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    if (mailboxLockTaken)
    {
        FM_DROP_MAILBOX_LOCK(sw);
    }

    if (switchLockTaken)
    {
        UNPROTECT_SWITCH(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmDeliverPacketTimestamp */




/*****************************************************************************/
/** fmGetMailboxGlortRange
 * \ingroup intMailbox
 *
 * \desc            Get mailbox glort range for given PEP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[out]      glortBase points to caller-allocated storage where this
 *                  function should place glort base value.
 *
 * \param[out]      numberOfGlorts points to caller-allocated storage where this
 *                  function should place number of glorts.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetMailboxGlortRange(fm_int     sw,
                                 fm_int     pepNb,
                                 fm_uint32 *glortBase,
                                 fm_int *   numberOfGlorts)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetMailboxGlortRange,
                       sw,
                       pepNb,
                       glortBase,
                       numberOfGlorts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

   FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}  /* end fmGetMailboxGlortRange */




/*****************************************************************************/
/** fmCleanupResourcesForPep
 * \ingroup intMailbox
 *
 * \desc            Cleanup API resources for a PEP port.
 *                  All virtual ports/mac entries/mcast groups/flows added
 *                  on driver demand should be removed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCleanupResourcesForPep(fm_int sw,
                                   fm_int pepNb)
{
    fm_status            status;
    fm_switch *          switchPtr;
    fm_mailboxInfo *     info;
    fm_event *           event;
    fm_eventLogicalPort *logicalPortEvent;
    fm_uint32            glort;
    fm_int               logicalPort;
    fm_int               numberOfGlorts;
    fm_int               firstDeletedPort;
    fm_int               numberOfDeletedPorts;
    fm_int               pepPort;
    fm_int               i;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr = GET_SWITCH_PTR(sw);
    status    = FM_OK;
    info      = GET_MAILBOX_INFO(sw);
    pepPort   = -1;
    firstDeletedPort     = -1;
    numberOfDeletedPorts = 0;

    /* Reset counters */
    info->macEntriesAdded[pepNb]           = 0;
    info->innerOuterMacEntriesAdded[pepNb] = 0; 

    status = fmGetMailboxGlortRange(sw,
                                    pepNb,
                                    &glort,
                                    &numberOfGlorts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    for (i = 0 ; i < numberOfGlorts ; i++)
    {
        status = fmGetGlortLogicalPort(sw,
                                       glort + i,
                                       &logicalPort);

        if (status == FM_OK)
        {
            FM_API_CALL_FAMILY(status,
                               switchPtr->FreeVirtualLogicalPort,
                               sw,
                               pepNb,
                               logicalPort,
                               1 /*Number of ports*/);

            numberOfDeletedPorts++;

            if (firstDeletedPort == -1)
            {
                firstDeletedPort = logicalPort;
            }
        }
    }

    status = FM_OK;

    if (numberOfDeletedPorts > 0)
    {
        event = fmAllocateEvent(sw,
                                FM_EVID_HIGH_PORT,
                                FM_EVENT_LOGICAL_PORT,
                                FM_EVENT_PRIORITY_LOW);

        if (event == NULL)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "No event can be reported when deleting "
                         "logical ports for PEP usage.\n"
                         "First port = %d, number of deleted ports = %d\n",
                         firstDeletedPort,
                         numberOfDeletedPorts);
            goto ABORT;
            
        }

        FM_API_CALL_FAMILY(status,
                           switchPtr->MapPepToLogicalPort,
                           sw,
                           pepNb,
                           &pepPort);

        logicalPortEvent = &event->info.fpLogicalPortEvent;
        FM_CLEAR(*logicalPortEvent);

        logicalPortEvent->firstPort      = firstDeletedPort;
        logicalPortEvent->numberOfPorts  = numberOfDeletedPorts;
        logicalPortEvent->pepId          = pepNb;
        logicalPortEvent->pepLogicalPort = pepPort;
        logicalPortEvent->portCreated    = FALSE;

        status = fmSendThreadEvent(&fmRootApi->eventThread, event);

        if (status != FM_OK)
        {
            /* Free the event since we could not send it to thread */
            fmReleaseEvent(event);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmCleanupResourcesForPep */




/*****************************************************************************/
/** fmFindInternalPortByMailboxGlort
 * \ingroup intStacking
 *
 * \desc            Find the logical port matching a given mailbox glort.
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
fm_status fmFindInternalPortByMailboxGlort(fm_int    sw, 
                                           fm_uint32 glort, 
                                           fm_int *  logicalPort)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, glort=%d, logicalPort=%p\n",
                 sw,
                 glort,
                 (void *) logicalPort);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(status,
                       switchPtr->FindInternalPortByMailboxGlort,
                       sw,
                       glort,
                       logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmFindInternalPortByMailboxGlort */




/*****************************************************************************/
/** fmSetMgmtPepXcastModes
 * \ingroup intMailbox
 *
 * \desc            Set the management PEP XCAST modes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       xCastLogPort is Xcast flooding logical port.
 *
 * \param[in]       addToMcastGroup is a flag to indicate if the CPU 
 *                  port is added to (or removed from) the requested
 *                  flooding mcast group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fmSetMgmtPepXcastModes(fm_int sw, 
                                 fm_int xCastLogPort, 
                                 fm_bool addToMcastGroup)
{
    fm_switch *         switchPtr;
    fm_status           status;
    fm_int              pepNb;
    fm_hostSrvXcastMode srvXcastMode;
    fm_uint16           xcastFloodMode;
    fm_uint32           glort;
    fm_int              xcastGlort;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, xCastLogicalPort=%d, addToMcastGroup=%d\n",
                 sw,
                 xCastLogPort, 
                 addToMcastGroup);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapLogicalPortToPep,
                       sw,
                       switchPtr->cpuPort,
                       &pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortGlort(sw, switchPtr->cpuPort, &glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetGlortForSpecialPort,
                       switchPtr->switchNumber,
                       xCastLogPort,
                       &xcastGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "cpuPort=%d, pepNb=%d, glort=0x%x\n",
                 switchPtr->cpuPort, 
                 pepNb,
                 glort);

    srvXcastMode.glort = glort;

    if (addToMcastGroup)
    {
        /* Add Mgmt PEP to the requested flooding MCAST group */
        srvXcastMode.mode = FM_HOST_SRV_XCAST_MODE_PROMISC;
    }
    else
    {
        /* Remove Mgmt PEP to the requested flooding MCAST group */
        srvXcastMode.mode = FM_HOST_SRV_XCAST_MODE_NONE;
    }

    switch ( xCastLogPort )
    {
        case FM_PORT_BCAST:
            xcastFloodMode = FM_MAILBOX_XCAST_FLOOD_MODE_BCAST;
            break;

        case FM_PORT_MCAST:
            xcastFloodMode = FM_MAILBOX_XCAST_FLOOD_MODE_MCAST;
            break;

        case FM_PORT_FLOOD:
            xcastFloodMode = FM_MAILBOX_XCAST_FLOOD_MODE_UCAST;
            break;

        default:
            status = FM_ERR_INVALID_VALUE;
            goto ABORT;

    }

    status = SetMgmtXcastModes(sw, 
                               pepNb, 
                               xcastGlort, 
                               srvXcastMode, 
                               xcastFloodMode);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmSetMgmtPepXcastModes */
