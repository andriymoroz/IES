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




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




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
                       switchPtr->MapVsiGlortToPepNumber,
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
 *                  elements used to store proper message arguments.
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
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvPort          srvPort;
    fm_event *              event;
    fm_eventLogicalPort *   logicalPortEvent;
    fm_bool                 argHdrRead;
    fm_mailboxResources *   mailboxResource;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr        = GET_SWITCH_PTR(sw);
    info             = GET_MAILBOX_INFO(sw);
    status           = FM_OK;
    abortStatus      = FM_OK;
    bytesRead        = 0;
    argBytesRead     = 0;
    index            = 0;
    rv               = 0;
    pvid             = 0;
    pepPort          = -1;
    argHdrRead       = FALSE;
    firstPort        = FM_LOGICAL_PORT_ANY;
    event            = NULL;
    logicalPortEvent = NULL;
    mailboxResource  = NULL;

    FM_CLEAR(argHdr);

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
        /* create instances to track resources created on host interface request. */
        for (i = firstPort ; i < (firstPort + srvPort.glortCount) ; i++)
        {
            mailboxResource = fmAlloc(sizeof(fm_mailboxResources));

            if (mailboxResource == NULL)
            {
                status = FM_ERR_INVALID_MULTICAST_GROUP;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            fmTreeInit(&mailboxResource->mailboxMacResource);
            fmTreeInit(&mailboxResource->mailboxFlowResource);
            fmTreeInit(&mailboxResource->mailboxFlowTableResource);

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
                /* Do nothing. */
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
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvPort          srvPort;
    fm_event *              event;
    fm_eventLogicalPort *   logicalPortEvent;
    fm_bool                 argHdrRead;

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
    argHdrRead       = FALSE;

    FM_CLEAR(argHdr);

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
    fm_mailboxMessageHeader  argHdr;
    fm_hostSrvXcastMode      srvXcastMode;
    fm_intMulticastGroup *   mcastGroupForMcastFlood;
    fm_intMulticastGroup *   mcastGroupForUcastFlood;
    fm_intMulticastGroup *   mcastGroupForBcastFlood;
    fm_mailboxInfo *         info;
    fm_uint64                listenerKey;
    fm_intMulticastListener *intListener;
    fm_multicastListener     listener;
    fm_bool                  argHdrRead;
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
    argHdrRead   = FALSE;
    mcastGroupForMcastFlood = NULL;
    mcastGroupForUcastFlood = NULL;
    mcastGroupForBcastFlood = NULL;
    intListener             = NULL;

    FM_CLEAR(listener);

    FM_CLEAR(argHdr);
    FM_CLEAR(srvXcastMode);

    switchPtr = GET_SWITCH_PTR(sw);

    mcastHNIFlooding = fmGetBoolApiProperty(FM_AAK_API_MULTICAST_HNI_FLOODING,
                                            FM_AAD_API_MULTICAST_HNI_FLOODING);

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

    /* Force the XCAST mode of the management port to NONE */
    if (switchPtr->cpuPort == pepPort)
    {
        srvXcastMode.mode = FM_HOST_SRV_XCAST_MODE_NONE;
    }

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
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvMACUpdate     srvMacUpdate;
    fm_macAddressEntry      macAddrEntry;
    fm_bool                 isDeleteAction;
    fm_bool                 isMacSecure;
    fm_bool                 argHdrRead;
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
    argHdrRead     = FALSE;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(argHdr);

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
                 "Processing MAC %012llx\n",
                 macAddrEntry.macAddress);

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

                status = fmGetMcastGroupListenerFirst(sw,
                                                      mcastGroup,
                                                      &listener);

                if (status  == FM_ERR_NO_MORE)
                {
                    status = fmDeactivateMcastGroup(sw,
                                                    mcastGroup);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    status = fmDeleteMcastGroup(sw,
                                                mcastGroup);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
                else if (status != FM_OK)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
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
    fm_bool                 argHdrRead;
    fm_mailboxMessageHeader argHdr;
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
    argHdrRead   = FALSE;
    value        = FM_ENABLED;

    FM_CLEAR(argHdr);

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
    fm_mailboxMessageHeader   argHdr;
    fm_hostSrvCreateFlowTable srvCreateTable;
    fm_flowCondition          condition;
    fm_bool                   tableWithPriority;
    fm_bool                   argHdrRead;
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
    argHdrRead        = FALSE;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(argHdr);

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
    fm_mailboxMessageHeader   argHdr;
    fm_hostSrvDeleteFlowTable srvDeleteTable;
    fm_bool                   argHdrRead;
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
    argHdrRead     = FALSE;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(argHdr);

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
    fm_int                  flowID;
    fm_int                  logicalPort;
    fm_hostSrvLportMap      lportMap; /* Need to get glort range for given PEP*/
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvUpdateFlow    srvUpdateFlow;
    fm_hostSrvFlowHandle    srvFlowHandle;
    fm_flowCondition        condition;
    fm_flowAction           action;
    fm_flowValue            condVal;
    fm_flowParam            flowParam;
    fm_macaddr              macAddr;
    fm_bool                 argHdrRead;
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
    argHdrRead   = FALSE;
    flowID       = FM_MAILBOX_FLOW_ID_ADD_NEW;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(condVal);
    FM_CLEAR(flowParam);

    FM_CLEAR(argHdr);

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
                           &flowID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);          

        srvUpdateFlow.flowID = flowID;

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

    if (argHdrRead == TRUE)
    {
        srvFlowHandle.flowID = srvUpdateFlow.flowID;
    }
    else
    {
        srvFlowHandle.flowID = flowID;
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
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvDeleteFlow    srvDeleteFlow;
    fm_bool                 argHdrRead;
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
    argHdrRead   = FALSE;
    mailboxResourcesUsed = NULL;

    FM_CLEAR(argHdr);

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
    fm_mailboxMessageHeader argHdr;
    fm_hostSrvFlowState     srvFlowState;
    fm_bool                 argHdrRead;

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
    argHdrRead      = FALSE;

    FM_CLEAR(argHdr);

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
/** fmSetTimestampModeProcess
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for TX_TIMESTAMP_MODE.
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
fm_status fmSetTimestampModeProcess(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_mailboxMessageHeader *pfTrHdr)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_uint32               rv;
    fm_int                  logicalPort;
    fm_mailboxMessageHeader argHdr;
    fm_bool                 argHdrRead;
    fm_hostSrvTimestampModeReq  modeReq;
    fm_hostSrvTimestampModeResp modeResp;
    fm_portTxTimestampMode      maxTimestampMode;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    switchPtr        = GET_SWITCH_PTR(sw);
    status           = FM_OK;
    bytesRead        = 0;
    argBytesRead     = 0;
    rv               = 0;
    logicalPort      = -1;
    argHdrRead       = FALSE;
    maxTimestampMode = FM_PORT_TX_TIMESTAMP_MODE_NONE;

    FM_CLEAR(argHdr);
    FM_CLEAR(modeReq);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ValidateMailboxMessageLength,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_CLEAR(modeReq);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxRequestArguments,
                       sw,
                       pepNb,
                       ctrlHdr,
                       pfTrHdr,
                       FM_HOST_SRV_SET_TIMESTAMP_MODE_REQ_TYPE,
                       FM_HOST_SRV_TMSTAMP_MODE_REQ_TYPE_SIZE,
                       (void *) &modeReq);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Prepare response */
    FM_CLEAR(modeResp);

    status = GetMaxSupportedTxTimestampMode(sw,
                                            &maxTimestampMode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (modeReq.mode <= maxTimestampMode)
    {
        status = fmGetGlortLogicalPort(sw,
                                       modeReq.glort,
                                       &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status)

        status = fmSetLogicalPortAttribute(sw,
                                           logicalPort,
                                           FM_LPORT_TX_TIMESTAMP_MODE,
                                           (void *) &modeReq.mode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else
    {
        status = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    modeResp.glort = modeReq.glort;

    modeResp.maxMode = maxTimestampMode;

    modeResp.status = status;

    FM_API_CALL_FAMILY(status,
                       switchPtr->WriteMailboxResponseMessage,
                       sw,
                       pepNb,
                       ctrlHdr,
                       FM_MAILBOX_MSG_SET_TIMESTAMP_MODE_ID,
                       FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE,
                       (void *) &modeResp);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fmSetTimestampModeProcess */




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
    }

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
                           switchPtr->MapVsiGlortToPepNumber,
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
