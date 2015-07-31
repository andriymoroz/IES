/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mailbox.c
 * Creation Date:   October 16, 2013
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

#include <fm_sdk_fm10000_int.h>

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
    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                 "smVersion=%d, smErrorFlags=%d reqHead=%d, respTail=%d\n"
                 "pfVersion=%d, pfErrorFlags=%d respHead=%d, reqTail=%d\n",
                 ctrlHdr->smVersion, ctrlHdr->smErrorFlags,
                 ctrlHdr->reqHead, ctrlHdr->respTail,
                 ctrlHdr->pfVersion, ctrlHdr->pfErrorFlags,
                 ctrlHdr->respHead, ctrlHdr->reqTail);
    
}   /* end PrintControlHeader */


/*****************************************************************************/
/** SignalRequestRead
 * \ingroup intMailbox
 *
 * \desc            Inform host interface that request message was read.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SignalRequestRead(fm_int sw,
                                   fm_int pepNb)
{
    fm_status status;
    fm_uint64 regAddr;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status       = FM_OK;

    regAddr = FM10000_PCIE_GMBX();
    rv = 0;

    FM_SET_BIT(rv,
               FM10000_PCIE_GMBX,
               GlobalReqInterrupt,
               1);

    FM_SET_BIT(rv,
               FM10000_PCIE_GMBX,
               PFAck,
               1);

    status = fm10000WritePep(sw,
                             regAddr,
                             pepNb,
                             rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end SignalRequestRead */




/*****************************************************************************/
/** CheckMailboxVersion
 * \ingroup intMailbox
 *
 * \desc            Validates if mailbox version is set. If not, process
 *                  synchronization procedure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          TRUE if proper version is set.
 * \return          FALSE if synchronization is run.
 *
 *****************************************************************************/
static fm_bool CheckMailboxVersion(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_bool    versionSet;
    fm_uint32  updateType;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr);

    switchPtr  = GET_SWITCH_PTR(sw);
    status     = FM_OK;
    versionSet = TRUE;
    updateType = 0;

    if (ctrlHdr->pfVersion == FM_MAILBOX_VERSION_RESET)
    {
        versionSet         = FALSE;
        ctrlHdr->smVersion = FM_MAILBOX_VERSION_RESET;
        updateType         = FM_UPDATE_CTRL_HDR_VERSION;
    }
    else if (ctrlHdr->smVersion == FM_MAILBOX_VERSION_RESET)
    {
        versionSet = FALSE;
        if (ctrlHdr->pfVersion == FM_MAILBOX_VERSION_DEFAULT)
        {
            ctrlHdr->smVersion    = ctrlHdr->pfVersion;
            ctrlHdr->smErrorFlags = FM_MAILBOX_ERR_TYPE_NONE;
            ctrlHdr->reqHead      = FM10000_MAILBOX_QUEUE_MIN_INDEX;
            ctrlHdr->respTail     = FM10000_MAILBOX_QUEUE_MIN_INDEX;
            updateType            = FM_UPDATE_CTRL_HDR_ALL;
        }
        else
        {
            ctrlHdr->smErrorFlags = FM_MAILBOX_ERR_TYPE_INVALID_VERSION;
            updateType            = FM_UPDATE_CTRL_HDR_ERROR;
        }
    }
    else if (ctrlHdr->smVersion != ctrlHdr->pfVersion)
    {
        versionSet            = FALSE;
        ctrlHdr->smErrorFlags = FM_MAILBOX_ERR_TYPE_INVALID_VERSION;
        updateType            = FM_UPDATE_CTRL_HDR_ERROR;
    }

    /* If version is not set, we need to update SM control header */
    if (versionSet == FALSE)
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->UpdateMailboxSmHdr,
                           sw,
                           pepNb,
                           ctrlHdr,
                           updateType);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = SignalRequestRead(sw,
                                   pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_MAILBOX,
                       versionSet,
                       "%d\n",
                       versionSet);

ABORT:

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_MAILBOX,
                       versionSet,
                       "Unable to process synchronization procedure.\n");

}   /* end CheckMailboxVersion */




/*****************************************************************************/
/** CheckCtrlHdrBoundaries
 * \ingroup intMailbox
 *
 * \desc            Validates if request/response queue indexes from control
 *                  header are within the allowed boundaries.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CheckCtrlHdrBoundaries(fm_int                   sw,
                                        fm_int                   pepNb,
                                        fm_mailboxControlHeader *ctrlHdr)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_status  writeStatus;
    fm_int     size;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr);

    switchPtr   = GET_SWITCH_PTR(sw);
    status      = FM_OK;
    writeStatus = FM_OK;
    size        = 0;

    if ( (ctrlHdr->reqHead  < FM10000_MAILBOX_QUEUE_MIN_INDEX) ||
         (ctrlHdr->reqHead  >= FM10000_MAILBOX_QUEUE_SIZE) ||
         (ctrlHdr->respTail < FM10000_MAILBOX_QUEUE_MIN_INDEX) ||
         (ctrlHdr->respTail >= FM10000_MAILBOX_QUEUE_SIZE) ||
         (ctrlHdr->respHead < FM10000_MAILBOX_QUEUE_MIN_INDEX) ||
         (ctrlHdr->respHead >= FM10000_MAILBOX_QUEUE_SIZE) ||
         (ctrlHdr->reqTail  < FM10000_MAILBOX_QUEUE_MIN_INDEX) ||
         (ctrlHdr->reqTail  >= FM10000_MAILBOX_QUEUE_SIZE) )
   {
        ctrlHdr->smVersion = FM_MAILBOX_VERSION_RESET;

        FM_API_CALL_FAMILY(status,
                           switchPtr->UpdateMailboxSmHdr,
                           sw,
                           pepNb,
                           ctrlHdr,
                           FM_UPDATE_CTRL_HDR_VERSION);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }


ABORT:

    /* message id is not important here as we set the queues 
     * in reset state. */
    fmSendHostSrvErrResponse(sw,
                             pepNb,
                             status,
                             ctrlHdr,
                             FM_MAILBOX_MSG_CREATE_LPORT_ID,
                             FM_HOST_SRV_RETURN_ERR_TYPE);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CheckCtrlHdrBoundaries */




/*****************************************************************************/
/** ParseMailboxControlHeader
 * \ingroup intMailbox
 *
 * \desc            Parse values from mailbox control header.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       smHeaderValue is the SM header value.
 *
 * \param[in]       pfHeaderValue is the PF header value.
 *
 * \param[out]      ctrlHdr points to caller-allocated storage where function
 *                  should place mailbox control header fields.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void ParseMailboxControlHeader(fm_int                   sw,
                                      fm_uint32                smHeaderValue,
                                      fm_uint32                pfHeaderValue,  
                                      fm_mailboxControlHeader *ctrlHdr)
{

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw = %d, smHeaderValue = 0x%x, pfHeaderValue = 0x%x, "
                 "ctrlHdr = %p\n",
                 sw,
                 smHeaderValue,
                 pfHeaderValue,
                 (void *) ctrlHdr);

    /* Parse values from SM header value */
    ctrlHdr->smVersion = FM_GET_FIELD(smHeaderValue,
                                      FM_MAILBOX_SM_CONTROL_HEADER,
                                      VERSION);

    ctrlHdr->smErrorFlags = FM_GET_FIELD(smHeaderValue,
                                         FM_MAILBOX_SM_CONTROL_HEADER,
                                         ERROR_FLAGS);
    
    ctrlHdr->reqHead = FM_GET_FIELD(smHeaderValue,
                                    FM_MAILBOX_SM_CONTROL_HEADER,
                                    REQUEST_QUEUE_HEAD);

    ctrlHdr->respTail = FM_GET_FIELD(smHeaderValue,
                                     FM_MAILBOX_SM_CONTROL_HEADER,
                                     RESPONSE_QUEUE_TAIL);

    /* Parse values from PF header value */
    ctrlHdr->pfVersion = FM_GET_FIELD(pfHeaderValue,
                                      FM_MAILBOX_PF_CONTROL_HEADER,
                                      VERSION);

    ctrlHdr->pfErrorFlags = FM_GET_FIELD(pfHeaderValue,
                                         FM_MAILBOX_PF_CONTROL_HEADER,
                                         ERROR_FLAGS);
    
    ctrlHdr->respHead = FM_GET_FIELD(pfHeaderValue,
                                     FM_MAILBOX_PF_CONTROL_HEADER,
                                     RESPONSE_QUEUE_HEAD);

    ctrlHdr->reqTail = FM_GET_FIELD(pfHeaderValue,
                                    FM_MAILBOX_PF_CONTROL_HEADER,
                                    REQUEST_QUEUE_TAIL);

}   /* end ParseMailboxControlHeader */




/*****************************************************************************/
/** ParseMailboxMessageHeader
 * \ingroup intMailbox
 *
 * \desc            Parse values from mailbox message header.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       hdrValue is the transaction header value.
 *
 * \param[out]      msgHdr points to caller-allocated storage where function
 *                  should place mailbox message header fields.
 *
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void ParseMailboxMessageHeader(fm_int                   sw,
                                      fm_uint32                hdrValue,
                                      fm_mailboxMessageHeader *msgHdr)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, hdrValue=%d, msgHdr=%p\n",
                 sw,
                 hdrValue,
                 (void *) msgHdr);

    msgHdr->type = FM_GET_FIELD(hdrValue,
                                FM_MAILBOX_MESSAGE_HEADER,
                                MESSAGE_TYPE);

    msgHdr->flags = FM_GET_FIELD(hdrValue,
                                 FM_MAILBOX_MESSAGE_HEADER,
                                 MESSAGE_FLAGS);

    msgHdr->length = FM_GET_FIELD(hdrValue,
                                  FM_MAILBOX_MESSAGE_HEADER,
                                  MESSAGE_LENGTH);

}   /* end ParseMailboxMessageHeader */




/*****************************************************************************/
/** ReadFromRequestQueue
 * \ingroup intMailbox
 *
 * \desc            Read 32-bit value from request queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       value is the value to be read from the queue
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadFromRequestQueue(fm_int                   sw,
                                      fm_int                   pepNb,
                                      fm_uint32 *              value,
                                      fm_mailboxControlHeader *ctrlHdr)
{
    fm_int    index;
    fm_uint64 regAddr;
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MAILBOX,
                         "sw = %d, pepNb = %d, value = 0x%x, ctrlHdr = %p\n",
                         sw,
                         pepNb,
                         *value,
                         (void *) ctrlHdr);

    if (ctrlHdr->reqHead == ctrlHdr->reqTail)
    {
        status = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status); 
    }

    index   = CALCULATE_PF_OFFSET_FROM_QUEUE_INDEX(ctrlHdr->reqHead);

    regAddr = FM10000_PCIE_MBMEM(index);

    status = fm10000ReadPep(sw,
                            regAddr,
                            pepNb,
                            value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadFromRequestQueue */




/*****************************************************************************/
/** ReadMessageHeader
 * \ingroup intMailbox
 *
 * \desc            Read message header from mailbox.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      msgHdr points to caller-allocated storage where function
 *                  should place message header fields.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReadMessageHeader(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *msgHdr)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, msgHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) msgHdr);

    status = FM_OK;
    rv     = 0;

    /* Read PF transaction header. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Parse result to fm_mailboxMessageHeader structure */
    ParseMailboxMessageHeader(sw,
                              rv,
                              msgHdr);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadMessageHeader */




/*****************************************************************************/
/** MessageIdToText
 * \ingroup intMailbox
 *
 * \desc            Returns the text representation of a message Id.
 *
 * \param[in]       msgId is the message Id.
 *
 * \return          Pointer to a string representing the logical port type.
 *
 *****************************************************************************/
static const char* MessageIdToText(fm_uint16 msgId)
{

    switch (msgId)
    {
        case FM_MAILBOX_MSG_TEST_ID:
            return "TEST_MESSAGE";

        case FM_MAILBOX_MSG_SET_XCAST_MODES_ID:
            return "XCAST_MODES";

        case FM_MAILBOX_MSG_UPDATE_MAC_FWD_RULE_ID:
            return "UPDATE_MAC_FWD_RULE";

        case FM_MAILBOX_MSG_MAP_LPORT_ID:
            return "LPORT_MAP";

        case FM_MAILBOX_MSG_CREATE_LPORT_ID:
            return "LPORT_CREATE";

        case FM_MAILBOX_MSG_DELETE_LPORT_ID:
            return "LPORT_DELETE";

        case FM_MAILBOX_MSG_UPDATE_PVID_ID:
            return "UPDATE_PVID";

        case FM_MAILBOX_MSG_CONFIG_ID:
            return "CONFIG";

        case FM_MAILBOX_MSG_CREATE_FLOW_TABLE_ID:
            return "CREATE_FLOW_TABLE";

        case FM_MAILBOX_MSG_DELETE_FLOW_TABLE_ID:
            return "DELETE_FLOW_TABLE";

        case FM_MAILBOX_MSG_UPDATE_FLOW_ID:
            return "UPDATE_FLOW";

        case FM_MAILBOX_MSG_DELETE_FLOW_ID:
            return "DELETE_FLOW";

        case FM_MAILBOX_MSG_SET_FLOW_STATE_ID:
            return "SET_FLOW_STATE";

        case FM_MAILBOX_MSG_GET_HW_PLATFORM_ID:
            return "GET_HW_PLATFORM";

        case FM_MAILBOX_MSG_DELIVER_PACKET_TIMESTAMP_ID:
            return "DELIVER_PACKET_TIMESTAMP";

        case FM_MAILBOX_MSG_SET_TIMESTAMP_MODE_ID:
            return "TX_TIMESTAMP_MODE";

        case FM_MAILBOX_MSG_MASTER_CLK_OFFSET_ID:
            return "MASTER_CLK_OFFSET";

        case FM_MAILBOX_MSG_INN_OUT_MAC_ID:
            return "FILTER_INNER_OUTER_MAC";

        default:
            return "UNKNOWN";
    }

}   /* end MessageIdToText */




/*****************************************************************************/
/** ValidateResponseLength
 * \ingroup intMailbox
 *
 * \desc            Validates if there are enough free elements in 
 *                  response queue to write response.
 *                  particular requests.
 *
 * \note            Message length should NOT include response header. This
 *                  is done in function body.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       responseMsgLength is response message length.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ValidateResponseLength(fm_int                   sw,
                                        fm_mailboxControlHeader *ctrlHdr,
                                        fm_uint16                responseMsgLength)
{
    fm_status status;
    fm_int    usedRows;
    fm_int    emptyRows;
    fm_int    emptyBytes;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, ctrlHdr=%p, responseMsgSize=%d\n",
                 sw,
                 (void *) ctrlHdr,
                 responseMsgLength);

    status = FM_OK;

    /* Calculate empty rows in response queue 
     * (row 0 is reserved for control header) */
    usedRows   = CALCULATE_USED_QUEUE_ELEMENTS(ctrlHdr->respHead, ctrlHdr->respTail);
    emptyRows  = FM10000_MAILBOX_QUEUE_SIZE - (usedRows + 1);
    emptyBytes = emptyRows * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

    /* Add response message header length */
    responseMsgLength += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

    if (emptyBytes < (responseMsgLength) )
    {
        status = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ValidateResponseLength */




/*****************************************************************************/
/** WriteToResponseQueue
 * \ingroup intMailbox
 *
 * \desc            Write 32-bit value to response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       value is the value to be written to the queue
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteToResponseQueue(fm_int                   sw,
                                      fm_int                   pepNb,
                                      fm_uint32                value,
                                      fm_mailboxControlHeader *ctrlHdr)
{
    fm_int    index;
    fm_uint64 regAddr;
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MAILBOX,
                         "sw = %d, pepNb = %d, value = 0x%x, ctrlHdr = %p\n",
                         sw,
                         pepNb,
                         value,
                         (void *) ctrlHdr);

    index   = CALCULATE_SM_OFFSET_FROM_QUEUE_INDEX(ctrlHdr->respTail);
    regAddr = FM10000_PCIE_MBMEM(index);

    status = fm10000WritePep(sw,
                             regAddr,
                             pepNb,
                             value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    INCREMENT_QUEUE_INDEX(ctrlHdr->respTail);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteToResponseQueue */




/*****************************************************************************/
/** WriteResponseMessageHeader
 * \ingroup intMailbox
 *
 * \desc            Write response message header to mailbox response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       msgTypeId is the response message type id.
 *
 * \param[in]       msgLength is the response message length in bytes.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteResponseMessageHeader(fm_int                   sw,
                                            fm_int                   pepNb,
                                            fm_mailboxControlHeader *ctrlHdr,
                                            fm_mailboxMessageId      msgTypeId,
                                            fm_uint16                msgLength)
{
    fm_status  status;
    fm_uint32  rv;
    fm_int     flags;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, msgTypeId=%d, msgLength=%d\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 msgTypeId,
                 msgLength);

    status = FM_OK;
    rv     = 0;
    flags  = 0;

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_TYPE,
                 msgTypeId);

    FM_SET_BIT(flags,
               FM_MAILBOX_MESSAGE_HEADER,
               START_OF_MESSAGE,
               1);

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_FLAGS,
                 flags);

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_LENGTH,
                 msgLength);

    status = WriteToResponseQueue(sw,
                                  pepNb,
                                  rv,
                                  ctrlHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteResponseMessageHeader */




/*****************************************************************************/
/** WriteResponseArgumentHeader
 * \ingroup intMailbox
 *
 * \desc            Write response argument header to mailbox response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       argType is the response argument type.
 *
 * \param[in]       msgLength is the response message length in bytes.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteResponseArgumentHeader(fm_int                        sw,
                                             fm_int                        pepNb,
                                             fm_mailboxControlHeader *     ctrlHdr,
                                             fm_mailboxMessageArgumentType argType,
                                             fm_uint16                     msgLength)
{
    fm_status  status;
    fm_uint32  rv;
    fm_int     flags;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, argType=%d, msgLength=%d\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 argType,
                 msgLength);

    status = FM_OK;
    rv     = 0;
    flags  = 0;

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_TYPE,
                 argType);

    FM_SET_BIT(flags,
               FM_MAILBOX_MESSAGE_HEADER,
               START_OF_MESSAGE,
               0);

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_FLAGS,
                 flags);

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_LENGTH,
                 msgLength);

    status = WriteToResponseQueue(sw,
                                  pepNb,
                                  rv,
                                  ctrlHdr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteResponseArgumentHeader */




/*****************************************************************************/
/** SignalResponseSent
 * \ingroup intMailbox
 *
 * \desc            Inform host interface that response message was sent.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SignalResponseSent(fm_int sw,
                                    fm_int pepNb)
{
    fm_status status;
    fm_uint64 regAddr;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status = FM_OK;

    regAddr = FM10000_PCIE_GMBX();   

    rv = 0;

    FM_SET_BIT(rv,
               FM10000_PCIE_GMBX,
               PFReq,
               1);

    status = fm10000WritePep(sw,
                             regAddr,
                             pepNb,
                             rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end SignalResponseSent */




/*****************************************************************************/
/** PCIeMailboxProcessRequest
 * \ingroup intMailbox
 *
 * \desc            Process request interrupt from PF.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PCIeMailboxProcessRequest(fm_int sw,
                                           fm_int pepNb)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_status               maskStatus;
    fm_mailboxControlHeader controlHeader;
    fm_mailboxMessageHeader pfTransactionHeader;
    fm_uint64               regAddr;
    fm_uint32               rv;
    fm_int                  index;
    fm_int                  swToExecute;
    fm_int                  pepNbToExecute;
    fm_bool                 versionSet;
    fm_bool                 startOfMessage;
    fm_bool                 useLoopback;
    fm_bool                 pepResetState;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr      = GET_SWITCH_PTR(sw);
    status         = FM_OK;
    rv             = 0;
    startOfMessage = TRUE;
    useLoopback    = FALSE;
    versionSet     = TRUE;
    index          = 0;
    regAddr        = 0;
    pepResetState  = 1; /* Pep in active state*/

    FM_CLEAR(controlHeader);
    FM_CLEAR(pfTransactionHeader);

    /*************************************************************************
     * Read mailbox control header.
     *************************************************************************/
    FM_API_CALL_FAMILY(status,
                       switchPtr->ReadMailboxControlHdr,
                       sw,
                       pepNb,
                       &controlHeader);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    swToExecute    = sw;
    pepNbToExecute = pepNb;

#if FM_SUPPORT_SWAG
    
    swToExecute    = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);
    pepNbToExecute = CALCULATE_SWAG_PEP_ID(sw, pepNb); 

#endif

/*    PrintControlHeader(&controlHeader); */

    useLoopback = fmGetBoolApiProperty(FM_AAK_API_FM10000_HNI_SERVICES_LOOPBACK,
                                       FM_AAD_API_FM10000_HNI_SERVICES_LOOPBACK);

    if (useLoopback == FALSE)
    {
        /* Validate mailbox control header versions */
        versionSet = CheckMailboxVersion(sw,
                                         pepNb,
                                         &controlHeader);
    }

    if (versionSet == TRUE)
    {
        status = CheckCtrlHdrBoundaries(sw,
                                        pepNb,
                                        &controlHeader);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Signal that message from PF to SM has been read by SM */
        status = SignalRequestRead(sw, pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        while (controlHeader.reqTail != controlHeader.reqHead)
        {
            status = ReadMessageHeader(sw,
                                       pepNb,
                                       &controlHeader,
                                       &pfTransactionHeader);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            if (useLoopback == TRUE)
            {
                /* This is first approach: we are processing in loopback mode. */
                status = fmProcessLoopbackRequest(sw,
                                                  pepNb,
                                                  &controlHeader);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                goto ABORT;
            }

            /* Try to ingore transaction headers with no 
             * START_OF_MESSAGE bit set */
            startOfMessage = FM_GET_BIT(pfTransactionHeader.flags,
                                        FM_MAILBOX_MESSAGE_HEADER,
                                        START_OF_MESSAGE);

            if (startOfMessage == FALSE)
            {
                continue;
            }

            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Processing %s request\n",
                         MessageIdToText(pfTransactionHeader.type));

            switch (pfTransactionHeader.type)
            {
                case FM_MAILBOX_MSG_SET_XCAST_MODES_ID:
                    status = fmSetXcastModesProcess(swToExecute,
                                                    pepNbToExecute,
                                                    &controlHeader,
                                                    &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_UPDATE_MAC_FWD_RULE_ID:
                    status = fmUpdateMacFwdRuleProcess(swToExecute,
                                                       pepNbToExecute,
                                                       &controlHeader,
                                                       &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_MAP_LPORT_ID:
                    status = fmMapLportProcess(swToExecute,
                                               pepNbToExecute,
                                               &controlHeader,
                                               &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_CREATE_LPORT_ID:
                    status = fmCreateLportProcess(swToExecute,
                                                  pepNbToExecute,
                                                  &controlHeader,
                                                  &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_DELETE_LPORT_ID:
                    status = fmDeleteLportProcess(swToExecute,
                                                  pepNbToExecute,
                                                  &controlHeader,
                                                  &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_CONFIG_ID:
                    status = fmConfigReqProcess(swToExecute,
                                                pepNbToExecute,
                                                &controlHeader,
                                                &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_CREATE_FLOW_TABLE_ID:
                    status = fmCreateFlowTableProcess(swToExecute,
                                                      pepNbToExecute,
                                                      &controlHeader,
                                                      &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_DELETE_FLOW_TABLE_ID:
                    status = fmDeleteFlowTableProcess(swToExecute,
                                                      pepNbToExecute,
                                                      &controlHeader,
                                                     &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_UPDATE_FLOW_ID:
                    status = fmUpdateFlowProcess(swToExecute,
                                                 pepNbToExecute,
                                                 &controlHeader,
                                                 &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_DELETE_FLOW_ID:
                    status = fmDeleteFlowProcess(swToExecute,
                                                 pepNbToExecute,
                                                 &controlHeader,
                                                 &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_SET_FLOW_STATE_ID:
                    status = fmSetFlowStateProcess(swToExecute,
                                                   pepNbToExecute,
                                                   &controlHeader,
                                                   &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_MASTER_CLK_OFFSET_ID:
                    status = fmMasterClkOffsetProcess(swToExecute,
                                                      pepNbToExecute,
                                                      &controlHeader,
                                                      &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_INN_OUT_MAC_ID:
                    status = fmFilterInnerOuterMacProcess(swToExecute,
                                                          pepNbToExecute,
                                                          &controlHeader,
                                                          &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                default:
                    /* Update head value in SM header */
                    status = fmUnknownRequestProcess(swToExecute,
                                                     pepNbToExecute,
                                                     &controlHeader,
                                                     &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }

ABORT:

    status = fm10000GetPepResetState(sw,
                                     pepNb,
                                     &pepResetState);

    /* If pep is in active state*/
    if (pepResetState)
    {
        /* Re-enable Mailbox Interrupts */
        rv = 0;
        FM_SET_BIT(rv, FM10000_PCIE_IP, Mailbox, 1);
        regAddr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(),
                                       pepNb);

        maskStatus = fmMaskUINT32(sw,
                                  regAddr,
                                  rv,
                                  FALSE);

        if (maskStatus != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "MaskUINT32 error(%d)\n",
                         maskStatus);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    
}   /* end PCIeMailboxProcessRequest  */




/*****************************************************************************/
/** PCIeMailboxProcessGlobalAck
 * \ingroup intMailbox
 *
 * \desc            Process Global ACK interrupt from PF.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PCIeMailboxProcessGlobalAck(fm_int sw,
                                             fm_int pepNb)
{
    fm_status status;
    fm_status maskStatus;
    fm_uint64 regAddr;
    fm_uint32 rv;
    fm_bool   pepResetState;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status  = FM_OK;
    pepResetState  = 1; /* Pep in active state*/

    regAddr = FM10000_PCIE_GMBX();
    rv = 0;
    
    FM_SET_BIT(rv,
               FM10000_PCIE_GMBX,
               GlobalAckInterrupt,
               1);

    status = fm10000WritePep(sw,
                             regAddr,
                             pepNb,
                             rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    status = fm10000GetPepResetState(sw,
                                     pepNb,
                                     &pepResetState);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* If pep is in active state*/
    if (pepResetState)
    {
        /* Re-enable Mailbox Interrupts */
        rv = 0;
        FM_SET_BIT(rv, FM10000_PCIE_IP, Mailbox, 1);
        regAddr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(),
                                       pepNb);

    

       maskStatus = fmMaskUINT32(sw,
                                  regAddr,
                                  rv,
                                  FALSE);

        if (maskStatus != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "MaskUINT32 error(%d)\n",
                         maskStatus);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
 
}   /* end PCIeMailboxProcessGlobalAck */




/*****************************************************************************/
/** AllocateMailboxGlortCams
 * \ingroup intPort
 *
 * \desc            Creates glort cam entries and destination entries for 
 *                  VIRTUAL ports. 
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AllocateMailboxGlortCams(fm_int sw)
{
    fm_status           status;
    fm_mailboxInfo *    mailboxInfo;
    fm_logicalPortInfo *lportInfo;
    fm_glortDestEntry **destEntry;
    fm_int              numberOfPeps;
    fm_uint32           numberOfGlorts;
    fm_uint32           currentMinGlort;
    fm_uint32           currentGlort;
    fm_uint32           currentMask;
    fm_uint32           maxGlort;
    fm_uint32           glortsAllocated;
    fm_uint32           glortsToAllocate;
    fm_uint32           destEntriesUsed;
    fm_uint32           camIndex;
    fm_uint32           dividedValue;
    fm_uint32           i;
    fm_int              destBase;
    fm_int              bitPosition;
    fm_int              numberOfBits;
    fm_int              size;
    fm_int              logicalPort;
    fm_bool             calculationDone;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    mailboxInfo = GET_MAILBOX_INFO(sw);
    lportInfo   = GET_LPORT_INFO(sw);

    numberOfPeps    = FM10000_NUM_PEPS;
    numberOfGlorts  = numberOfPeps * mailboxInfo->glortsPerPep;
    currentMinGlort = mailboxInfo->glortBase;
    maxGlort        = currentMinGlort + numberOfGlorts - 1;
    glortsAllocated = 0;
    destBase        = 0;
    i               = 0;

    bitPosition = fmFindNextBitInMask(sw,
                                      mailboxInfo->glortMask,
                                      0);

    if (bitPosition < 0)
    {
        bitPosition = 0;
    }

    /* We need to optimal allocate proper number of GLORT_CAM/GLORT_RAM entries
     * to program all glorts associated with virtual ports. 
     * The algorithm is based on the assumption, that number of glorts per PEP
     * must be the power of 2 and at least be 128. 
     * Next step is to calculate how many dest entries can be handled
     * per one CAM/RAM entry with a continous mask.
     * Also we need to duplicate dest entries for PEP ports to ensure that 
     * block of dest entries is continuous. We may also need some dest entries
     * with an empty portmask. */
    while (glortsAllocated < numberOfGlorts)
    {
        currentMask      = mailboxInfo->glortMask;
        numberOfBits     = 0;
        destEntriesUsed  = 1;
        calculationDone  = FALSE;
        glortsToAllocate = mailboxInfo->glortsPerPep;
        destEntry        = NULL;
        dividedValue     = currentMinGlort / mailboxInfo->glortsPerPep;

        /* Calculate, how many glorts can be programmed in one CAM entry. */
        while (calculationDone == FALSE)
        {
            /* If this condition is met, double number of glorts to be allocated. */
            if ( ( (dividedValue % 2) == 0) 
                && ( (glortsToAllocate + glortsAllocated)  < numberOfGlorts) 
                && ( (currentMinGlort + glortsToAllocate) <= FM10000_MAILBOX_GLORT_MAX) )
            {
                status = fmClearBitInMask(sw,
                                          &currentMask,
                                          bitPosition + numberOfBits);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                glortsToAllocate <<= 1;
                destEntriesUsed <<= 1;
                numberOfBits++;
                dividedValue = dividedValue / 2;
            }
            else
            {
                calculationDone = TRUE;
            }
        }

        destBase = fmFindUnusedDestEntries(sw,
                                           destEntriesUsed,
                                           destBase);

        if (destBase < 0)
        {
            status = FM_ERR_LPORT_DESTS_UNAVAILABLE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        status = fmCreateGlortCamEntry(sw,
                                       currentMask,
                                       currentMinGlort,
                                       FM_GLORT_ENTRY_TYPE_STRICT,
                                       destBase,
                                       destEntriesUsed,
                                       numberOfBits,
                                       bitPosition,
                                       0,
                                       0,
                                       FM_GLORT_ENTRY_HASH_A,
                                       0,
                                       &camIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        size      = destEntriesUsed * sizeof(fm_glortDestEntry *);
        destEntry = fmAlloc(size);

        if (destEntry == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        status = fmAllocDestEntries(sw,
                                    destEntriesUsed,
                                    &lportInfo->camEntries[camIndex],
                                    destEntry,
                                    FM_PORT_TYPE_VIRTUAL);

        for (i = 0 ; i < destEntriesUsed ; i++)
        {
            currentGlort = currentMinGlort + i * mailboxInfo->glortsPerPep;

            if (currentGlort < maxGlort)
            {
                status = fm10000MapVirtualGlortToLogicalPort(sw,
                                                             currentGlort,
                                                             &logicalPort);

                if (status == FM_OK)
                {
                    status = fmSetPortInPortMask(sw,
                                                 &destEntry[i]->destMask,
                                                 logicalPort,
                                                 TRUE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }
            }

            status = fm10000WriteDestEntry(sw,
                                           destEntry[i]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        glortsAllocated += glortsToAllocate;
        currentMinGlort += glortsToAllocate;
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end AllocateMailboxGlortCams */




/*****************************************************************************/
/** CleanupFloodingListsWhenDeletingLport
 * \ingroup intMailbox
 *
 * \desc            Cleanup mcast groups associated with flooding lists when
 *                  deleting virtual port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       portNumber is the number of logical port being deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanupFloodingListsWhenDeletingLport(fm_int sw,
                                                       fm_int pepNb,
                                                       fm_int portNumber)
{
    fm_switch *          switchPtr;
    fm_status            status;
    fm_mailboxInfo *     info;
    fm_multicastListener currentListener;
    fm_multicastListener nextListener; 
    fm_portmask          ucastFloodDestMask;
    fm_portmask          mcastFloodDestMask;
    fm_portmask          bcastFloodDestMask;
    fm_int               pepPort;
    fm_bool              doSearching;


    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MAILBOX_INFO(sw);
    status = FM_OK;
    doSearching = TRUE;

    /* Check portmask for ucast/mcast/bcast flooding glorts.
     * Add/delete pep port to portmasks if needed. */

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_FLOOD,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &ucastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_MCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &mcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_BCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &bcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);


    /* Cleanup of mcast flood list.*/
    status = fmGetMcastGroupListenerFirst(sw,
                                          info->mcastGroupForMcastFlood,
                                          &currentListener);

    if (status == FM_OK)
    {
        if (currentListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
            currentListener.port == portNumber)
        {
            status = fmDeleteMcastGroupListener(sw,
                                                info->mcastGroupForMcastFlood,
                                                &currentListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Remove pep port from multicast flooding portmask if needed. */
            if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 1) &&
                 (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 1)
            {
                FM_PORTMASK_DISABLE_BIT(&mcastFloodDestMask, pepPort);

                status = fmSetLogicalPortAttribute(sw,
                                                   FM_PORT_MCAST,
                                                   FM_LPORT_DEST_MASK,
                                                   (void *) &mcastFloodDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            info->numberOfVirtualPortsAddedToMcastFlood[pepNb]--;

            /* Listener found, no more searching needed */
            doSearching = FALSE;
        }
    }
    else if (status != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    while ( (status == FM_OK) && (doSearching == TRUE) )
    {
        status = fmGetMcastGroupListenerNext(sw,
                                             info->mcastGroupForMcastFlood,
                                             &currentListener,
                                             &nextListener);

        if (status == FM_OK)
        {
            if (nextListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
                nextListener.port == portNumber)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForMcastFlood,
                                                    &nextListener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Remove pep port from multicast flooding portmask if needed. */
                if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&mcastFloodDestMask, pepPort);

                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_MCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &mcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToMcastFlood[pepNb]--;

                /* Listener found, no more searching needed */
                break;
            }

            currentListener = nextListener;
        }
        else if (status != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    /* Cleanup of ucast flood list.*/
    status = fmGetMcastGroupListenerFirst(sw,
                                          info->mcastGroupForUcastFlood,
                                          &currentListener);

    if (status == FM_OK)
    {
        if (currentListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
            currentListener.port == portNumber)
        {
            status = fmDeleteMcastGroupListener(sw,
                                                info->mcastGroupForUcastFlood,
                                                &currentListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Remove pep port from unicast flooding portmask if needed. */
            if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 1) &&
                 (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 1)
            {
                FM_PORTMASK_DISABLE_BIT(&ucastFloodDestMask, pepPort);

                status = fmSetLogicalPortAttribute(sw,
                                                   FM_PORT_FLOOD,
                                                   FM_LPORT_DEST_MASK,
                                                   (void *) &ucastFloodDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            info->numberOfVirtualPortsAddedToUcastFlood[pepNb]--;

            /* Listener found, no more searching needed */
            doSearching = FALSE;
        }
    }
    else if (status != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    while ( (status == FM_OK) && (doSearching == TRUE) )
    {
        status = fmGetMcastGroupListenerNext(sw,
                                             info->mcastGroupForUcastFlood,
                                             &currentListener,
                                             &nextListener);

        if (status == FM_OK)
        {
            if (nextListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
                nextListener.port == portNumber)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForUcastFlood,
                                                    &nextListener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Remove pep port from unicast flooding portmask if needed. */
                if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&ucastFloodDestMask, pepPort);

                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_FLOOD,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &ucastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToUcastFlood[pepNb]--;

                /* Listener found, no more searching needed */
                break;
            }

            currentListener = nextListener;
        }
        else if (status != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    /* Cleanup of bcast flood list.*/
    status = fmGetMcastGroupListenerFirst(sw,
                                          info->mcastGroupForBcastFlood,
                                          &currentListener);

    if (status == FM_OK)
    {
        if (currentListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
            currentListener.port == portNumber)
        {
            status = fmDeleteMcastGroupListener(sw,
                                                info->mcastGroupForBcastFlood,
                                                &currentListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Remove pep port from broadcast flooding portmask if needed. */
            if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 1) &&
                 (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 1)
            {
                FM_PORTMASK_DISABLE_BIT(&bcastFloodDestMask, pepPort);

                status = fmSetLogicalPortAttribute(sw,
                                                   FM_PORT_BCAST,
                                                   FM_LPORT_DEST_MASK,
                                                   (void *) &bcastFloodDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            info->numberOfVirtualPortsAddedToBcastFlood[pepNb]--;

            /* Listener found, no more searching needed */
            doSearching = FALSE;
        }
    }
    else if (status != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    while ( (status == FM_OK) && (doSearching == TRUE) )
    {
        status = fmGetMcastGroupListenerNext(sw,
                                             info->mcastGroupForBcastFlood,
                                             &currentListener,
                                             &nextListener);

        if (status == FM_OK)
        {
            if (nextListener.vlan == FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS &&
                nextListener.port == portNumber)
            {
                status = fmDeleteMcastGroupListener(sw,
                                                    info->mcastGroupForBcastFlood,
                                                    &nextListener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* Remove pep port from broadcast flooding portmask if needed. */
                if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&bcastFloodDestMask, pepPort);

                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_BCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &bcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToBcastFlood[pepNb]--;

                /* Listener found, no more searching needed */
                break;
            }

            currentListener = nextListener;
        }
        else if (status != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupFloodingListsWhenDeletingLport */




/*****************************************************************************/
/** CleanupResourcesWhenDeletingLport
 * \ingroup intMailbox
 *
 * \desc            Cleanup mac table and ACL resources when
 *                  deleting virtual port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       portNumber is the number of logical port being deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanupResourcesWhenDeletingLport(fm_int sw,
                                                   fm_int pepNb,
                                                   fm_int portNumber)
{
    fm_status       status;
    fm_mailboxInfo *info;
    fm_switch *     switchPtr;
    fm_int          mcastGroup;
    fm_int          flowTableIndex;
    fm_int          flowId;
    fm_int          firstRule;
    fm_int          bitNum;
    fm_uint16       vlan;
    fm_uintptr      tableType;
    fm_uint64       key;
    fm_macaddr      macAddr;
    void *          nextValue;
    fm_multicastListener listener;
    fm_multicastAddress  mcastAddr;
    fm_macAddressEntry macEntry;
    fm_mailboxResources *mailboxResourcesUsed;
    fm_treeIterator treeIterMailboxRes;
    fm_bool         routingLockTaken;
    fm_bool         ruleDeleted;
    fm_hostSrvInnOutMac *macFilterKey;
    fm_hostSrvInnOutMac *macFilterVal;
    fm_mailboxMcastMacVni  macVniKey;
    fm_mailboxMcastMacVni *macVniVal;
    fm_aclCondition cond;
    fm_aclValue     value;
    fm_aclActionExt action;
    fm_aclParamExt  param;
    fm_char         statusText[1024];
    fm_customTreeIterator filterIterator;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    switchPtr = GET_SWITCH_PTR(sw);

    status         = FM_OK;
    macAddr        = 0;
    mcastGroup     = -1;
    vlan           = 0;
    key            = 0;
    flowTableIndex = -1;
    flowId         = -1;
    ruleDeleted    = FALSE;
    routingLockTaken = FALSE;
    mailboxResourcesUsed = NULL;

    info = GET_MAILBOX_INFO(sw);

    if (!fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
    { 
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cannot track used resources per virtual port "
                     "as resource tree is not initialized.\n");
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    }

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        portNumber,
                        (void **) &mailboxResourcesUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (fmTreeIsInitialized(&mailboxResourcesUsed->mailboxMacResource))
    {
        /* Delete MAC entries associated with logical port. */    
        fmTreeIterInit(&treeIterMailboxRes,
                       &mailboxResourcesUsed->mailboxMacResource);

        while ( fmTreeIterNext(&treeIterMailboxRes,
                               &key,
                               &nextValue) != FM_ERR_NO_MORE )
        {
            macAddr = GET_MAC_FROM_MACVLAN_RESOURCE_KEY(key);
            vlan    = GET_VLAN_FROM_MACVLAN_RESOURCE_KEY(key);
 
            if (fmIsMulticastMacAddress(macAddr))
            {
                FM_CLEAR(mcastAddr);
                FM_CLEAR(listener);

                mcastAddr.addressType = FM_MCAST_ADDR_TYPE_L2MAC_VLAN;
                mcastAddr.info.mac.destMacAddress = macAddr;
                mcastAddr.info.mac.vlan = vlan;

                status = fmFindMcastGroupByAddress(sw,
                                                   &mcastAddr,
                                                   &mcastGroup);
                if (status != FM_OK)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Multicast group with given address (0x%llx) "
                                  "and vlan (%d) is not present and "
                                  "cannot be deleted\n",
                                  macAddr,
                                  vlan);
                    continue;
                }

                listener.port = portNumber;
                listener.vlan = vlan;

                status = fmDeleteMcastGroupListener(sw,
                                                    mcastGroup,
                                                    &listener);

                if (status != FM_OK)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Multicast group with given address (0x%llx) "
                                  "does not have listener with (port,vlan) (%d, %d) "
                                  "and cannot be deleted\n",
                                  macAddr,
                                  portNumber,
                                  vlan);
                    continue;
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
                FM_CLEAR(macEntry);
                macEntry.vlanID = vlan;
                macEntry.macAddress = macAddr;

                status = fmDeleteAddress(sw, &macEntry);
                if (status != FM_OK)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "MAC entry with given address (0x%llx) "
                                  "and vlan (%d) is not present and "
                                  "cannot be deleted.\n",
                                  macAddr,
                                  vlan);
                }
            }
        }

        fmTreeDestroy(&mailboxResourcesUsed->mailboxMacResource, NULL);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cleanup for MAC entries for given port (%d) "
                     "will not be processed as resource tree is "
                     "not initialized.\n",
                     portNumber);
    }

    if (fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowResource))
    {
        /* Delete Flow entries associated with logical port. */
        fmTreeIterInit(&treeIterMailboxRes,
                       &mailboxResourcesUsed->mailboxFlowResource);

        while ( fmTreeIterNext(&treeIterMailboxRes,
                               &key,
                               &nextValue) != FM_ERR_NO_MORE )
        {
            flowId         = GET_FLOW_ID_FROM_FLOW_RESOURCE_KEY(key);
            flowTableIndex = GET_FLOW_TABLE_FROM_FLOW_RESOURCE_KEY(key);

            status = fmDeleteFlow(sw,
                                  flowTableIndex,
                                  flowId);
            if (status != FM_OK)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Flow ID (0x%x) for flow table (0x%x) "
                             "is not present and cannot be deleted.\n",
                             flowId,
                             flowTableIndex);
            }
        }

        fmTreeDestroy(&mailboxResourcesUsed->mailboxFlowResource, NULL);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cleanup for FLOW entries for given port (%d) "
                     "will not be processed as resource tree is "
                     "not initialized.\n",
                     portNumber);
    }

    if (fmTreeIsInitialized(&mailboxResourcesUsed->mailboxFlowTableResource))
    {
        /* Try to delete FLOW table as well. */
        fmTreeIterInit(&treeIterMailboxRes,
                       &mailboxResourcesUsed->mailboxFlowTableResource);

        while ( fmTreeIterNext(&treeIterMailboxRes,
                               &key,
                               (void **) &tableType) != FM_ERR_NO_MORE )
        {
            flowTableIndex = (fm_int) key;

            if (tableType == FM_FLOW_TCAM_TABLE)
            {
                status = fmDeleteFlowTCAMTable(sw,
                                               flowTableIndex);

                if (status != FM_OK)
                {
                     FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                  "Flow table (index=0x%x, type=%d) cannot be deleted.\n",
                                  flowTableIndex,
                                  (fm_uint16) tableType);
                }
            }
            else if (tableType == FM_FLOW_BST_TABLE)
            {
                status = fmDeleteFlowBSTTable(sw,
                                              flowTableIndex);
                if (status != FM_OK)
                {
                     FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                  "Flow table (index=0x%x, type=%d) cannot be deleted.\n",
                                  flowTableIndex,
                                  (fm_uint16) tableType);
                }
            }
            else if (tableType == FM_FLOW_TE_TABLE)
            {
                status = fmDeleteFlowTETable(sw,
                                             flowTableIndex);
                if (status != FM_OK)
                {
                     FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                  "Flow table (index=0x%x, type=%d) cannot be deleted.\n",
                                  flowTableIndex,
                                  (fm_uint16) tableType);
                }
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Unknown table type (%d).\n",
                             (fm_uint16) tableType);
            }
        }

        fmTreeDestroy(&mailboxResourcesUsed->mailboxFlowTableResource, NULL);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cleanup for FLOW TABLE entries for given port (%d) "
                     "will not be processed as resource tree is "
                     "not initialized.\n",
                     portNumber);
    }

    if (fmCustomTreeIsInitialized(&mailboxResourcesUsed->innOutMacResource))
    {
        /* Delete MAC filtering entries associated with logical port. */
        fmCustomTreeIterInit(&filterIterator,
                             &mailboxResourcesUsed->innOutMacResource);

        while (status == FM_OK)
        {
            status = fmCustomTreeIterNext(&filterIterator,
                                          (void **) &macFilterKey,
                                          (void **) &macFilterVal);

            if (status == FM_OK)
            {
                status = fmDeleteACLRule(sw,
                                         macFilterVal->acl,
                                         macFilterVal->aclRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                ruleDeleted = TRUE;
            }
            else if (status == FM_ERR_NO_MORE)
            {
                break;
            }
            else
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* The rule id scheme is:
               ( (0b000000) |(High 12 bit) | (Low 14 bit)
               where only 14 lower bits from rule id are taken from bitarray,
               so we need to mask id before releasing it. */
            bitNum = macFilterVal->aclRule & 0x3FFF;

            status = fmSetBitArrayBit(&info->innOutMacRuleInUse,
                                      bitNum,
                                      0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Decrementing counters */
            mailboxResourcesUsed->innerOuterMacEntriesAdded--;
            info->innerOuterMacEntriesAdded[pepNb]--;
            
            if (fmIsMulticastMacAddress(macFilterVal->innerMacAddr))
            {
                /* If inner MAC is a mcast one, ACL rule should redirect frame 
                   to a multicast group. We need to remove listener
                   from such group. */

                macVniKey.macAddr = macFilterVal->innerMacAddr;
                macVniKey.vni     = macFilterVal->vni;

                status = fmCustomTreeFind(&info->mcastMacVni,
                                          &macVniKey,
                                          (void **) &macVniVal);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                FM_CLEAR(listener);

                listener.port = portNumber;
                listener.vlan = 0;

                status = fmDeleteMcastGroupListener(sw,
                                                    macVniVal->mcastGroup,
                                                    &listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                /* If there is no other listeners, delete mcast group */
                status = fmGetMcastGroupListenerFirst(sw,
                                                      macVniVal->mcastGroup,
                                                      &listener);

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
        }

        fmCustomTreeDestroy(&mailboxResourcesUsed->innOutMacResource, 
                            fmFreeSrvInnOutMac);

        /* If any rule was deleted, compile & apply ACL*/
        if (ruleDeleted)
        {
            /* Remove ACL if empty */
            status = fmGetACLRuleFirstExt(sw,
                                          info->aclIdForMacFiltering,
                                          &firstRule,
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
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Cleanup for inner/outer MAC filtering entries "
                     "for given port (%d) will not be processed "
                     "as resource tree is not initialized.\n",
                     portNumber);
    }

    status = FM_OK;

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupResourcesWhenDeletingLport */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000WriteResponseMessage
 * \ingroup intMailbox
 *
 * \desc            Write message to response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 * 
 * \param[in]       msgTypeId is the message type ID.
 *
 * \param[in]       argType is the type of the message to be written.
 *
 * \param[in]       message is the message to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fm10000WriteResponseMessage(fm_int                        sw,
                                      fm_int                        pepNb,
                                      fm_mailboxControlHeader *     ctrlHdr,
                                      fm_mailboxMessageId           msgTypeId,
                                      fm_mailboxMessageArgumentType argType,
                                      fm_voidptr *                  message)
{
    fm_switch *            switchPtr;
    fm_status              status;
    fm_hostSrvErr *        hostSrvErr;
    fm_hostSrvLportMap *   hostSrvLportMap;
    fm_hostSrvFlowHandle * hostSrvFlowHandle;
    fm_uint32 *            dataToWrite;
    fm_uint32              rv;
    fm_uint16              argSize;
    fm_int                 dataElements;
    fm_int                 allocSize;
    fm_int                 i;
    fm_hostSrvUpdatePvid * updatePvid;
    fm_hostSrvPacketTimestamp *packetTimestamp;
    fm_hostSrvTimestampModeResp *timestampMode;
    fm_hostSrvMasterClkOffset *clkOffset;
  
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, argType=%d, message=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 argType,
                 (void *) message);

    switchPtr          = GET_SWITCH_PTR(sw);
    status             = FM_OK;
    hostSrvErr         = NULL;
    hostSrvLportMap    = NULL;
    hostSrvFlowHandle  = NULL;
    dataToWrite        = NULL;
    updatePvid         = NULL;
    timestampMode      = NULL;
    clkOffset          = NULL;
    dataElements       = 0;
    allocSize          = 0;
    rv                 = 0;
    argSize            = 0;
    i                  = 0;

    /* Prepare response data to be written to queue.
     * No headers are formed here. */
    switch (argType)
    {
        case FM_HOST_SRV_RETURN_ERR_TYPE:
            argSize = FM_HOST_SRV_ERR_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            hostSrvErr = (fm_hostSrvErr *) message;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         STATUS_CODE,
                         hostSrvErr->statusCode);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         MAC_TABLE_ROWS_USED,
                         hostSrvErr->macTableRowsUsed);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         MAC_TABLE_ROWS_AVAILABLE,
                         hostSrvErr->macTableRowsAvailable);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         NEXTHOP_TABLE_ROWS_USED,
                         hostSrvErr->nexthopTableRowsUsed);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         NEXTHOP_TABLE_ROWS_AVAILABLE,
                         hostSrvErr->nexthopTableRowsAvailable);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         FFU_RULES_USED,
                         hostSrvErr->ffuRulesUsed);
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_ERR,
                         FFU_RULES_AVAILABLE,
                         hostSrvErr->ffuRulesAvailable);
            break;

        case FM_HOST_SRV_MAP_LPORT_TYPE:
            argSize = FM_HOST_SRV_LPORT_MAP_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            hostSrvLportMap = (fm_hostSrvLportMap *) message;
           
            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_LPORT_MAP,
                         GLORT_VALUE,
                         hostSrvLportMap->glortValue);

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_LPORT_MAP,
                         GLORT_MASK,
                         hostSrvLportMap->glortMask);

            break;

        case FM_HOST_SRV_HANDLE_FLOW_TYPE:
            argSize = FM_HOST_SRV_FLOW_HANDLE_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            hostSrvFlowHandle = (fm_hostSrvFlowHandle *) message;
            
            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_FLOW_HANDLE,
                         FLOW_ID,
                         hostSrvFlowHandle->flowID);

            break;

        case FM_HOST_SRV_UPDATE_PVID_TYPE:

            argSize = FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            updatePvid = (fm_hostSrvUpdatePvid *) message;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_UPDATE_PVID,
                         GLORT_VALUE,
                         updatePvid->glort);

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_UPDATE_PVID,
                         PVID_VALUE,
                         updatePvid->pvid);

            break;

        case FM_HOST_SRV_PACKET_TIMESTAMP_TYPE:

            argSize = FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            packetTimestamp = (fm_hostSrvPacketTimestamp *) message;

            rv = packetTimestamp->egressTimestamp & 0xFFFFFFFF;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         EGR_TIME_LOW,
                         rv);
            dataElements++;

            rv = packetTimestamp->egressTimestamp >> FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH;
            rv &= 0xFFFFFFFF;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         EGR_TIME_UP,
                         rv);
            dataElements++;
           
            rv = packetTimestamp->ingressTimestamp & 0xFFFFFFFF;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         INGR_TIME_LOW,
                         rv);
            dataElements++;

            rv = packetTimestamp->ingressTimestamp >> FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH;
            rv &= 0xFFFFFFFF;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         INGR_TIME_UP,
                         rv); 
            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         SGLORT,
                         packetTimestamp->sglort);

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                         DGLORT,
                         packetTimestamp->dglort);

            break;

        case FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE:

            argSize = FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            timestampMode = (fm_hostSrvTimestampModeResp *) message;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP,
                         GLORT,
                         timestampMode->glort);

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP,
                         MODE_ENABLED,
                         timestampMode->modeEnabled);

            break;

        case FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE:

            argSize = FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE;

            allocSize = sizeof(fm_uint32) *
                        (argSize / FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);

            dataToWrite = fmAlloc(allocSize);

            if (dataToWrite == NULL)
            {
                status = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_MEMSET_S(dataToWrite, allocSize, 0, allocSize);

            clkOffset = (fm_hostSrvMasterClkOffset *) message;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                         OFFSET_LOW,
                         clkOffset->offsetValueLower);

            dataElements++;

            FM_SET_FIELD(dataToWrite[dataElements],
                         FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                         OFFSET_UPP,
                         clkOffset->offsetValueUpper);

            break;

/*        case FM_HOST_SRV_TEST_MESSAGE_TYPE:
            status = ValidateResponseLength(sw,
                                            ctrlHdr,
                                            FM10000_HOST_SRV_TEST_MESSAGE_TYPE_SIZE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            status = WriteResponseMessageHeader(sw,
                                                pepNb,
                                                ctrlHdr,
                                                FM_HOST_SRV_TEST_MESSAGE_TYPE,
                                                FM10000_HOST_SRV_TEST_MESSAGE_TYPE_SIZE);

            hostSrvTestMessage = (fm_hostSrvTestMessage *) message;

            FM_SET_FIELD(rv,
                         FM_MAILBOX_SRV_TEST_MESSAGE,
                         SEND_RESPONSE,
                         hostSrvTestMessage->sendResponse);

            status = WriteToResponseQueue(sw,
                                          pepNb,
                                          rv,
                                          ctrlHdr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
*/
        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

    status = ValidateResponseLength(sw,
                                   ctrlHdr,
                                   argSize);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);


    /* Writing message and argument headers first.
     * Add one queue entry to size to include argument header for message header*/
    status = WriteResponseMessageHeader(sw,
                                        pepNb,
                                        ctrlHdr,
                                        msgTypeId,
                                        argSize + FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = WriteResponseArgumentHeader(sw,
                                         pepNb,
                                         ctrlHdr,
                                         argType,
                                         argSize);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (dataToWrite == NULL)
    {
        status = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    for (i = 0 ; i <= dataElements ; i++)
    {
        status = WriteToResponseQueue(sw,
                                      pepNb,
                                      dataToWrite[i],
                                      ctrlHdr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Update response tail value in SM header */
    FM_API_CALL_FAMILY(status,
                       switchPtr->UpdateMailboxSmHdr,
                       sw,
                       pepNb,
                       ctrlHdr,
                       FM_UPDATE_CTRL_HDR_RESPONSE_TAIL |
                       FM_UPDATE_CTRL_HDR_REQUEST_HEAD);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Signal that response from SM to PF has been sent */
    status = SignalResponseSent(sw,
                                pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    
ABORT:

    if (dataToWrite)
    {
        fmFree(dataToWrite);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000WriteResponseMessage */




/*****************************************************************************/
/** fm10000ReadMailboxControlHdr
 * \ingroup intMailbox
 *
 * \desc            Read mailbox control header.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[out]      ctrlHdr points to caller-allocated storage where function
 *                  should place mailbox control header fields.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReadMailboxControlHdr(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr)
{
    fm_status status;
    fm_uint64 regAddr;
    fm_uint32 smRegValue;
    fm_uint32 pfRegValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr);

    status = FM_OK;

    regAddr = FM10000_MAILBOX_SM_CONTROL_HEADER();

    status = fm10000ReadPep(sw,
                            regAddr,
                            pepNb,
                            &smRegValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    regAddr = FM10000_MAILBOX_PF_CONTROL_HEADER();

    status = fm10000ReadPep(sw,
                            regAddr,
                            pepNb,
                            &pfRegValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    ParseMailboxControlHeader(sw,
                              smRegValue,
                              pfRegValue,
                              ctrlHdr);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000ReadMailboxControlHdr */




/*****************************************************************************/
/** fm10000UpdateSmHeader
 * \ingroup intMailbox
 *
 * \desc            Update switch manager control header with new values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       controlHeader points to mailbox control header structure.
 *
 * \param[in]       updateType is a bitmask of update type flags.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if values in controlHeader structure
 *                  are invalid.
 *
 *****************************************************************************/
fm_status fm10000UpdateSmHeader(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *controlHeader,
                                fm_uint32                updateType)
{
    fm_status status;
    fm_uint32 headerValue;
    fm_uint64 regAddr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, controlHeader=%p, updateHeadValue=%d\n",
                 sw,
                 pepNb,
                 (void *) controlHeader,
                 updateType);

    status      = FM_OK;
    headerValue = 0;

    /*************************************************************************
     * Read mailbox control header.
     *************************************************************************/
    regAddr = FM10000_MAILBOX_SM_CONTROL_HEADER();

    status = fm10000ReadPep(sw,
                            regAddr,
                            pepNb,
                            &headerValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (updateType & FM_UPDATE_CTRL_HDR_RESPONSE_TAIL)
    {
        if (controlHeader->respTail > 0 &&
            controlHeader->respTail < FM10000_MAILBOX_QUEUE_SIZE)
        {
            FM_SET_FIELD(headerValue,
                         FM_MAILBOX_SM_CONTROL_HEADER,
                         RESPONSE_QUEUE_TAIL,
                         controlHeader->respTail);
        }
        else
        {
            status = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    if (updateType & FM_UPDATE_CTRL_HDR_REQUEST_HEAD)
    {
        if (controlHeader->reqHead > 0 &&
            controlHeader->reqHead < FM10000_MAILBOX_QUEUE_SIZE)
        {
            FM_SET_FIELD(headerValue,
                         FM_MAILBOX_SM_CONTROL_HEADER,
                         REQUEST_QUEUE_HEAD,
                         controlHeader->reqHead);
        }
        else
        {
            status = FM_ERR_INVALID_VALUE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    if (updateType & FM_UPDATE_CTRL_HDR_VERSION)
    {
        FM_SET_FIELD(headerValue,
                     FM_MAILBOX_SM_CONTROL_HEADER,
                     VERSION,
                     controlHeader->smVersion);
    }

    if (updateType & FM_UPDATE_CTRL_HDR_ERROR)
    {
        FM_SET_FIELD(headerValue,
                     FM_MAILBOX_SM_CONTROL_HEADER,
                     ERROR_FLAGS,
                     controlHeader->smErrorFlags);
    }

    status = fm10000WritePep(sw,
                             regAddr,
                             pepNb,
                             headerValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
  
ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000UpdateSmHeader */




/*****************************************************************************/
/** fm10000ValidateMessageLength
 * \ingroup intMailbox
 *
 * \desc            First approach of validation. Validates 
 *                  if there are enough elements used in queue to process 
 *                  particular requests.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to transaction header structure
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fm10000ValidateMessageLength(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_mailboxMessageHeader *pfTrHdr)
{
    fm_int    nbOfBytesUsed;
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, ctrlHdr=%p, pfTrHdr=%p\n",
                 sw,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr);

    status        = FM_OK;
    nbOfBytesUsed = CALCULATE_USED_QUEUE_ELEMENTS(ctrlHdr->reqHead,
                                                  ctrlHdr->reqTail);
    nbOfBytesUsed *= FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

    if (pfTrHdr->length > nbOfBytesUsed)
    {
        status = FM_ERR_INVALID_VALUE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:
    
    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000ValidateMessageLength */




/*****************************************************************************/
/** fm10000ReadRequestArguments
 * \ingroup intMailbox
 *
 * \desc            Read request message and fill structure fields with
 *                  proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       pfTrHdr points to PF transaction header structure.
 *
 * \param[in]       argumentType is the request message argument type.
 *
 * \param[in]       argumentLength is the request message argument legth.
 *
 * \param[out]      message points to caller-allocated storage where this
 *                  function should place readed values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue 
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
fm_status fm10000ReadRequestArguments(fm_int                   sw,
                                      fm_int                   pepNb,
                                      fm_mailboxControlHeader *ctrlHdr,
                                      fm_mailboxMessageHeader *pfTrHdr,
                                      fm_uint16                argumentType,
                                      fm_uint16                argumentLength,
                                      fm_voidptr *             message)
{
    fm_status               status;
    fm_uint32               bytesRead;
    fm_uint32               argBytesRead;
    fm_uint32               rv;
    fm_uint64               rv64;
    fm_int                  index;
    fm_int                  offset;
    fm_mailboxMessageHeader argHdr;
    fm_bool                 argHdrRead;
    fm_hostSrvPort *        srvPort;
    fm_hostSrvMACUpdate *   srvMacUpdate;
    fm_hostSrvXcastMode *   srvXcastMode;
    fm_hostSrvConfig *      srvConfig;
    fm_hostSrvCreateFlowTable *srvCreateTable;
    fm_hostSrvDeleteFlowTable *srvDeleteTable;
    fm_hostSrvDeleteFlow *   srvDeleteFlow;
    fm_hostSrvFlowState *    srvFlowState;
    fm_hostSrvUpdateFlow *   srvUpdateFlow;
    fm_hostSrvMasterClkOffset *clkOffset;
    fm_hostSrvInnOutMac *   macFilter;
    fm_flowCondition        condition;
    fm_flowAction           action;
    fm_flowValue *          flowVal;
    fm_flowParam *          flowParam;
    fm_macaddr              macAddr;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, pfTrHdr=%p "
                 "argumentType=%d, argumentLength=%d, message=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) pfTrHdr,
                 argumentType,
                 argumentLength,
                 (void *) message);

    status           = FM_OK;
    bytesRead        = 0;
    argBytesRead     = 0;
    index            = 0;
    offset           = 0;
    rv               = 0;
    rv64             = 0;
    condition        = 0;
    action           = 0;
    macAddr          = 0;
    argHdrRead       = FALSE;

    srvPort        = NULL;
    srvMacUpdate   = NULL;
    srvXcastMode   = NULL;
    srvConfig      = NULL;
    srvCreateTable = NULL;
    srvDeleteTable = NULL;
    srvDeleteFlow  = NULL;
    srvFlowState   = NULL;
    srvUpdateFlow  = NULL;
    clkOffset      = NULL;
    macFilter      = NULL;

    flowVal   = NULL;
    flowParam = NULL;

    /* Read arguments */
    while (bytesRead < pfTrHdr->length)
    {
        status = ReadMessageHeader(sw,
                                   pepNb,
                                   ctrlHdr,
                                   &argHdr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        bytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

        /* If we meet unexpected argument types, try to ignore them */
        if ( (argHdrRead == FALSE) &&
             (argHdr.type == argumentType) &&
             (argHdr.length >= argumentLength) )
        {
            switch (argumentType)
            {
                case FM_HOST_SRV_CREATE_DELETE_LPORT_TYPE:

                    srvPort = (fm_hostSrvPort *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvPort->firstGlort = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_SRV_PORT,
                                                      FIRST_GLORT);

                    srvPort->glortCount = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_SRV_PORT,
                                                      GLORT_COUNT);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_UPDATE_MAC_TYPE:

                    srvMacUpdate = (fm_hostSrvMACUpdate *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvMacUpdate->macAddressLower = FM_GET_FIELD(rv,
                                                                FM_MAILBOX_SRV_MAC_UPDATE,
                                                                MAC_ADDRESS_LOWER);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvMacUpdate->macAddressUpper = FM_GET_FIELD(rv,
                                                                FM_MAILBOX_SRV_MAC_UPDATE,
                                                                MAC_ADDRESS_UPPER);

                    srvMacUpdate->vlan = FM_GET_FIELD(rv,
                                                     FM_MAILBOX_SRV_MAC_UPDATE,
                                                     VLAN);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            
                    srvMacUpdate->glort = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_SRV_MAC_UPDATE,
                                                      GLORT);

                    srvMacUpdate->flags = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_SRV_MAC_UPDATE,
                                                      FLAGS);

                    srvMacUpdate->action = FM_GET_FIELD(rv,
                                                       FM_MAILBOX_SRV_MAC_UPDATE,
                                                       ACTION);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_SET_XCAST_MODE_TYPE:

                    srvXcastMode = (fm_hostSrvXcastMode *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvXcastMode->glort = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_SRV_XCAST_MODE,
                                                      GLORT);

                    srvXcastMode->mode = FM_GET_FIELD(rv,
                                                     FM_MAILBOX_SRV_XCAST_MODE,
                                                     MODE);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_REQUEST_CONFIG_TYPE:

                    srvConfig  = (fm_hostSrvConfig *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvConfig->configurationAttributeConstant =
                               FM_GET_FIELD(rv,
                                            FM_MAILBOX_SRV_CONFIG,
                                            CONFIGURATION_ATTRIBUTE_CONSTANT);

                    srvConfig->value = FM_GET_FIELD(rv,
                                                   FM_MAILBOX_SRV_CONFIG,
                                                   VALUE);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_CREATE_FLOW_TABLE_TYPE:

                    srvCreateTable = (fm_hostSrvCreateFlowTable *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    /* tableType will not be used for now. */
                    srvCreateTable->tableType =
                                    FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_CREATE_FLOW_TABLE,
                                                 TABLE_TYPE);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvCreateTable->tableIndex =
                                    FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_CREATE_FLOW_TABLE,
                                                 TABLE_INDEX);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvCreateTable->flowConditionBitmaskLower =
                                    FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_CREATE_FLOW_TABLE,
                                                 FLOW_COND_BITMASK_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvCreateTable->flowConditionBitmaskUpper =
                                    FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_CREATE_FLOW_TABLE,
                                                 FLOW_COND_BITMASK_UP);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;

                    break;

                case FM_HOST_SRV_DELETE_FLOW_TABLE_TYPE:

                    srvDeleteTable = (fm_hostSrvDeleteFlowTable *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvDeleteTable->tableIndex =
                                   FM_GET_FIELD(rv,
                                                FM_MAILBOX_SRV_DELETE_FLOW_TABLE,
                                                TABLE_INDEX);

                    srvDeleteTable->tableType = 
                                   FM_GET_FIELD(rv,
                                                FM_MAILBOX_SRV_DELETE_FLOW_TABLE,
                                                TABLE_TYPE);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_UPDATE_FLOW_TYPE:

                    srvUpdateFlow = (fm_hostSrvUpdateFlow *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvUpdateFlow->priority =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_UPDATE_FLOW,
                                               PRIORITY);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvUpdateFlow->tableIndex =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_UPDATE_FLOW,
                                               TABLE_INDEX);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvUpdateFlow->flowID =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_UPDATE_FLOW,
                                               FLOW_ID);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvUpdateFlow->condition =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_UPDATE_FLOW,
                                               FLOW_COND_BITMASK_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    rv64 = FM_GET_FIELD(rv,
                                        FM_MAILBOX_SRV_UPDATE_FLOW,
                                        FLOW_COND_BITMASK_UP); 

                    srvUpdateFlow->condition |=
                                   (rv64 << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    condition = srvUpdateFlow->condition;
                    flowVal   = &srvUpdateFlow->flowVal;


                    /* Read values according to condition set. 
                     * 
                     * Take into account below rules:
                     *  - For each bit set in the condition bitmask, the condition
                     * data portion will contain a value field and a mask field in
                     * that order.
                     * - Fields less than 16 bits in length are padded to 16 bits.
                     * - Fields do not straddle word boundaries, when needed pad to
                     * the remainder of the word and continue on the next word.
                     * - Fields greater than 32 bits present their words least
                     * significant to most significant. */
                    if (condition & FM_FLOW_MATCH_SRC_MAC)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        flowVal->src = FM_GET_FIELD(rv,
                                                    FM_MAILBOX_BITS,
                                                    0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowVal->src |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) &
                                         0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        flowVal->srcMask = FM_GET_FIELD(rv,
                                                        FM_MAILBOX_BITS,
                                                        0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = 0;
                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowVal->srcMask |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) &
                                             0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_DST_MAC)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        flowVal->dst = FM_GET_FIELD(rv,
                                                    FM_MAILBOX_BITS,
                                                    0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = 0;
                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowVal->dst |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) & 
                                         0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        flowVal->dstMask = FM_GET_FIELD(rv,
                                                        FM_MAILBOX_BITS,
                                                        0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = 0;
                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowVal->dstMask |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) &
                                             0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_ETHERTYPE)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->ethType = FM_GET_OFFSET_FIELD(rv,
                                                               offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->ethTypeMask = FM_GET_OFFSET_FIELD(rv,
                                                                   offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_VLAN)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->vlanId = FM_GET_OFFSET_FIELD(rv,
                                                              offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->vlanIdMask = FM_GET_OFFSET_FIELD(rv,
                                                                  offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_VLAN_PRIORITY)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->vlanPri = FM_GET_OFFSET_FIELD(rv,
                                                               offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->vlanPriMask = FM_GET_OFFSET_FIELD(rv,
                                                                   offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_SRC_IP)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
          
                        flowVal->srcIp.isIPv6 = FALSE;
                        flowVal->srcIp.addr[0] = htonl( FM_GET_FIELD(rv,
                                                                     FM_MAILBOX_BITS,
                                                                     0_31) );

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowVal->srcIpMask.isIPv6 = FALSE;
                        flowVal->srcIpMask.addr[0] = htonl( FM_GET_FIELD(rv,
                                                                         FM_MAILBOX_BITS,
                                                                         0_31) );
                    }

                    if (condition & FM_FLOW_MATCH_DST_IP)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowVal->dstIp.isIPv6 = FALSE;
                        flowVal->dstIp.addr[0] = htonl( FM_GET_FIELD(rv,
                                                                     FM_MAILBOX_BITS,
                                                                     0_31) );

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowVal->dstIpMask.isIPv6 = FALSE;
                        flowVal->dstIpMask.addr[0] = htonl( FM_GET_FIELD(rv,
                                                                         FM_MAILBOX_BITS,
                                                                         0_31) );
                    }

                    if (condition & FM_FLOW_MATCH_PROTOCOL)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->protocol = FM_GET_OFFSET_FIELD(rv,
                                                                offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->protocolMask = FM_GET_OFFSET_FIELD(rv,
                                                                    offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_L4_SRC_PORT)
                    {
                        if (offset == 0)
                        { 
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->L4SrcStart = FM_GET_OFFSET_FIELD(rv,
                                                                  offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->L4SrcMask = FM_GET_OFFSET_FIELD(rv,
                                                                 offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_L4_DST_PORT)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->L4DstStart = FM_GET_OFFSET_FIELD(rv,
                                                                  offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->L4DstMask = FM_GET_OFFSET_FIELD(rv,
                                                                 offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_INGRESS_PORT_SET)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowVal->portSet = FM_GET_FIELD(rv,
                                                        FM_MAILBOX_BITS,
                                                        0_31);
                    }

                    if (condition & FM_FLOW_MATCH_TOS)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->tos = FM_GET_OFFSET_FIELD(rv,
                                                           offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->tosMask = FM_GET_OFFSET_FIELD(rv,
                                                               offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_FRAME_TYPE)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->frameType = FM_GET_OFFSET_FIELD(rv,
                                                                 offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (condition & FM_FLOW_MATCH_SRC_PORT)
                    {
                        if (offset == 0)
                        {
                            status = ReadFromRequestQueue(sw,
                                                          pepNb,
                                                          &rv,
                                                          ctrlHdr);
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                            argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                        }

                        flowVal->logicalPort = FM_GET_OFFSET_FIELD(rv,
                                                                   offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    /* Read action bitmask */
                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvUpdateFlow->action =
                                   FM_GET_FIELD(rv,
                                                FM_MAILBOX_SRV_UPDATE_FLOW,
                                                FLOW_ACTION_BITMASK_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    rv64 = FM_GET_FIELD(rv,
                                        FM_MAILBOX_SRV_UPDATE_FLOW,
                                        FLOW_ACTION_BITMASK_UP);

                    srvUpdateFlow->action |=
                                   (rv64 << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    action    = srvUpdateFlow->action;
                    flowParam = &srvUpdateFlow->flowParam;

                    /* Read values according to action set.
                     * Take into account same rules as for condition */
                    if (action & FM_FLOW_ACTION_FORWARD)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowParam->logicalPort = FM_GET_FIELD(rv,
                                                              FM_MAILBOX_BITS,
                                                              0_31);

                    }

                    if (action & FM_FLOW_ACTION_SET_VLAN)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        flowParam->vlan = FM_GET_OFFSET_FIELD(rv,
                                                              offset);
                        offset = ((offset + 16) % FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH);
                    }

                    if (action & FM_FLOW_ACTION_SET_DMAC)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        flowParam->dmac = FM_GET_FIELD(rv,
                                                       FM_MAILBOX_BITS,
                                                       0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = 0;
                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowParam->dmac |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) &
                                            0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    if (action & FM_FLOW_ACTION_SET_SMAC)
                    {
                        offset = 0;
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                        
                        flowParam->smac = FM_GET_FIELD(rv,
                                                       FM_MAILBOX_BITS,
                                                       0_31);

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
            
                        status = ReadFromRequestQueue(sw,
                                                      pepNb,
                                                      &rv,
                                                      ctrlHdr);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                        macAddr = 0;
                        macAddr = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

                        flowParam->smac |= (macAddr << FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH) &
                                            0xFFFF00000000LL;

                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_DELETE_FLOW_TYPE:

                    srvDeleteFlow = (fm_hostSrvDeleteFlow *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvDeleteFlow->tableIndex =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_DELETE_FLOW,
                                               TABLE_INDEX);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvDeleteFlow->flowID =
                                  FM_GET_FIELD(rv,
                                               FM_MAILBOX_SRV_DELETE_FLOW,
                                               FLOW_ID);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_SET_FLOW_STATE_TYPE:

                    srvFlowState = (fm_hostSrvFlowState *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvFlowState->tableIndex =
                                 FM_GET_FIELD(rv,
                                              FM_MAILBOX_SRV_FLOW_STATE,
                                              TABLE_INDEX);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvFlowState->flowID =
                                 FM_GET_FIELD(rv,
                                              FM_MAILBOX_SRV_FLOW_STATE,
                                              FLOW_ID);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    srvFlowState->flowState =
                                 FM_GET_FIELD(rv,
                                              FM_MAILBOX_SRV_FLOW_STATE,
                                              FLOW_STATE);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                 case FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE:

                    clkOffset = (fm_hostSrvMasterClkOffset *) message;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    clkOffset->offsetValueLower = 
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                                           OFFSET_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    clkOffset->offsetValueUpper = 
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                                           OFFSET_UPP);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                case FM_HOST_SRV_INN_OUT_MAC_TYPE:

                    macFilter = (fm_hostSrvInnOutMac *) message;

                    /* See fm_hostSrvInnOutMac for MAC field details */
                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->outerMacAddr = 
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           OUT_MAC_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->outerMacAddr |= (fm_macaddr)
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           OUT_MAC_UPP) << 32; 

                    macFilter->outerMacAddrMask =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           OUT_MAC_MASK_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->outerMacAddrMask |= (fm_macaddr)
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           OUT_MAC_MASK_UPP) << 16;

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->innerMacAddr =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           INN_MAC_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->innerMacAddr |= (fm_macaddr)
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           INN_MAC_UPP) << 32;

                    macFilter->innerMacAddrMask =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           INN_MAC_MASK_LOW);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->innerMacAddrMask |= (fm_macaddr)
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           INN_MAC_MASK_UPP) << 16;

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->vni =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           VNI);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;


                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->outerL4Port =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           OUTER_L4_PORT);

                    macFilter->tunnelType =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           TUNNEL_TYPE);

                    macFilter->action =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           ACTION);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    status = ReadFromRequestQueue(sw,
                                                  pepNb,
                                                  &rv,
                                                  ctrlHdr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    macFilter->glort =
                              FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_INN_OUT_MAC,
                                           GLORT);

                    argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;

                    argHdrRead = TRUE;

                    /* If there is any more data from the argument - ignore it */
                    while (argBytesRead < argHdr.length)
                    {
                        INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                        argBytesRead += FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH;
                    }

                    bytesRead += argBytesRead;
                    break;

                default:

                    /* Unknown argument */
                    status = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    break;
            }
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000ReadRequestArguments */




/*****************************************************************************/
/** fm10000ProcessLoopbackRequest
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
fm_status fm10000ProcessLoopbackRequest(fm_int                   sw,
                                        fm_int                   pepNb,
                                        fm_mailboxControlHeader *controlHeader)
{
    fm_switch *             switchPtr;
    fm_status               status;
    fm_mailboxMessageHeader transactionHeader;
    fm_uint32 *             values;
    fm_int                  nbOfEntries;
    fm_int                  element;
    fm_int                  i;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr = GET_SWITCH_PTR(sw);
    status = FM_OK;
    FM_CLEAR(transactionHeader);

    /* 
     * STAGE 1: READING REQUEST MESSAGE
     */
    nbOfEntries = CALCULATE_USED_QUEUE_ELEMENTS(controlHeader->reqHead,
                                                controlHeader->reqTail);

    /* Add one more entry for transaction header */
    nbOfEntries++;

    values  = fmAlloc(sizeof(fm_uint32) * nbOfEntries);

    if (values == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Decrement request head value - we want to copy 
     * transaction header as well. */ 
    DECREMENT_QUEUE_INDEX(controlHeader->reqHead);
    element = 0;

    while (controlHeader->reqHead != controlHeader->reqTail)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &values[element],
                                      controlHeader);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        element++;
    }

    /* Update head value in SM header */
    FM_API_CALL_FAMILY(status,
                       switchPtr->UpdateMailboxSmHdr,
                       sw,
                       pepNb,
                       controlHeader,
                       FM_UPDATE_CTRL_HDR_REQUEST_HEAD);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* 
     * STAGE 2: SENDING RESPONSE MESSAGE
     */
    element = 0;

    for (i = 0 ; i < nbOfEntries ; i++)
    {
        status = WriteToResponseQueue(sw,
                                      pepNb,
                                      values[element],
                                      controlHeader);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        element++;
    }

    /* Update head value in SM header */
    FM_API_CALL_FAMILY(status,
                       switchPtr->UpdateMailboxSmHdr,
                       sw,
                       pepNb,
                       controlHeader,
                       FM_UPDATE_CTRL_HDR_RESPONSE_TAIL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Signal that response from SM to PF has been sent */
    status = SignalResponseSent(sw,
                                pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    if (values)
    {
        fmFree(values);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000ProcessLoopbackRequest */




/*****************************************************************************/
/** fm10000ProcessCreateFlowTableRequest
 * \ingroup intMailbox
 *
 * \desc            Process mailbox requests for CREATE_FLOW_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       tableIndex is the flow table index.
 *
 * \param[in]       tableType is the flow table type.
 *
 * \param[in]       condition is the bit mask of matching conditions.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ProcessCreateFlowTableRequest(fm_int           sw,
                                               fm_int           pepNb,
                                               fm_int           tableIndex,
                                               fm_uint16        tableType,
                                               fm_flowCondition condition)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, tableIndex=%d, condition=0x%llx\n",
                 sw,
                 tableIndex,
                 condition);

    switch (tableType)
    {
        case FM_FLOW_TCAM_TABLE:
            status = fmCreateFlowTCAMTable(sw,
                                           tableIndex,
                                           condition, 
                                           FM10000_MAX_RULE_PER_FLOW_TABLE,
                                           FM_MAILBOX_FLOW_TABLE_MAX_ACTION_SUPPORTED);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_FLOW_BST_TABLE:
            status = fmCreateFlowBSTTable(sw,
                                          tableIndex,
                                          condition,
                                          FM10000_MAX_RULE_PER_FLOW_TABLE,
                                          FM_MAILBOX_FLOW_TABLE_MAX_ACTION_SUPPORTED);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        case FM_FLOW_TE_TABLE:
            status = fmCreateFlowTETable(sw,
                                          tableIndex,
                                          condition,
                                          FM10000_MAX_RULE_PER_FLOW_TABLE,
                                          FM_MAILBOX_FLOW_TABLE_MAX_ACTION_SUPPORTED);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000ProcessCreateFlowTableRequest */




/*****************************************************************************/
/** fm10000GetHardwareMailboxGlortRange
 * \ingroup intMailbox
 *
 * \desc            Get the glort range usable for mailbox.
 *
 * \param[out]      mailboxGlortBase points to caller-provided storage into
 *                  which the default starting glort for mailbox
 *                  will be written.
 *
 * \param[out]      mailboxGlortCount points to caller-provided storage into
 *                  which the number of glorts which may be used for 
 *                  mailbox will be written.
 *
 * \param[out]      mailboxGlortMask points to caller-provided storage into
 *                  which the range of glorts per pep which may be used for 
 *                  mailbox will be written.
 *
 * \param[out]      mailboxGlortsPerPep  points to caller-provided storage into
 *                  which number of glorts available per pep will be written.
 *
 * \param[in]       numberOfSWAGMembers is the number of switch aggregate 
 *                  members. If number of switches in topology exceeds 
 *                  defined value we need to decrease number of available
 *                  glorts.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetHardwareMailboxGlortRange(fm_uint16 *mailboxGlortBase,
                                              fm_uint16 *mailboxGlortCount,
                                              fm_uint16 *mailboxGlortMask,
                                              fm_uint16 *mailboxGlortsPerPep,
                                              fm_int     numberOfSWAGMembers)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "mailboxGlortBase=%p, mailboxGlortCount=%p, mailboxGlortMask=%p, "
                 "mailboxGlortsPerPep=%p, numberOfSWAGMembers=%d\n",
                 (void *) mailboxGlortBase,
                 (void *) mailboxGlortCount,
                 (void *) mailboxGlortMask,
                 (void *) mailboxGlortsPerPep,
                 numberOfSWAGMembers);

    if ( (mailboxGlortBase == NULL) || (mailboxGlortCount == NULL)
        || (mailboxGlortMask == NULL) || (mailboxGlortsPerPep == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_ERR_INVALID_ARGUMENT);
    }

    *mailboxGlortBase = FM10000_MAILBOX_GLORT_BASE;

    if (numberOfSWAGMembers <= 
        FM10000_MAILBOX_MAX_SWITCHES_WITH_FULL_GLORT_RANGE)
    {
        *mailboxGlortCount  = FM10000_MAILBOX_GLORT_COUNT;
        *mailboxGlortMask   = FM10000_MAILBOX_GLORT_MASK;
        *mailboxGlortsPerPep = FM10000_MAILBOX_GLORTS_PER_PEP;
    }
    else
    {
        *mailboxGlortCount  = FM10000_MAILBOX_GLORT_COUNT_CUT;
        *mailboxGlortMask   = FM10000_MAILBOX_GLORT_MASK_CUT;
        *mailboxGlortsPerPep = FM10000_MAILBOX_GLORTS_PER_PEP_CUT;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);

}   /* end fm10000GetHardwareMailboxGlortRange */




/*****************************************************************************/
/** fm10000GetHardwareNumberOfPeps
 * \ingroup intMailbox
 *
 * \desc            Get the number of PEP ports.
 *
 * \return          Number of PEP ports.
 *
 *****************************************************************************/
fm_uint16 fm10000GetHardwareNumberOfPeps(void)
{
    return FM10000_NUM_PEPS;

}   /* end fm10000GetHardwareNumberOfPeps */




/*****************************************************************************/
/** fm10000GetHardwareMaxMailboxGlort
 * \ingroup intMailbox
 *
 * \desc            Get the max mailbox glort value.
 *
 * \param[out]      glort points to caller-provided storage into
 *                  which the max mailbox glort value will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetHardwareMaxMailboxGlort(fm_uint32 *glort)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "glort=%p\n",
                 (void *) glort);

    if ( glort == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_ERR_INVALID_ARGUMENT);
    }

    *glort = FM10000_MAILBOX_GLORT_MAX;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);

}   /* end fm10000GetHardwareMaxMailboxGlort */




/*****************************************************************************/
/** fm10000SetLportGlortRange
 * \ingroup intMailbox
 *
 * \desc            Set glort range for lports within given PEP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[out]      lportMap points to caller-allocated storage where this
 *                  function should place glort range.
 *
 * \return          none.
 *
 *****************************************************************************/
void fm10000SetLportGlortRange(fm_int              sw,
                               fm_int              pepNb,
                               fm_hostSrvLportMap *lportMap)
{
    fm_mailboxInfo *info;

    info = GET_MAILBOX_INFO(sw);

    lportMap->glortValue = info->glortBase +
                           (pepNb * info->glortsPerPep);

    lportMap->glortMask = info->glortMask;
    lportMap->glortsPerPep = info->glortsPerPep;

}   /* end fm10000SetLportGlortRange */




/*****************************************************************************/
/** fm10000SetHostSrvErrResponse
 * \ingroup intMailbox
 *
 * \desc            Sets appropriate fields for response message.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[out]      srvErr points to caller-allocated storage where this
 *                  function should place response values.
 *
 * \return          none.
 *
 *****************************************************************************/
void fm10000SetHostSrvErrResponse(fm_int         sw,
                                  fm_int         pepNb,
                                  fm_hostSrvErr *srvErr)
{
    fm_switch *switchPtr;
    fm10000_switch *ext;
    fm_status status;
    fm_internalMacAddrEntry *cacheEntry;
    fm_int rowsUsed;
    fm_int i;
    fm_int ffuRulesUsed;
    fm_int ffuRulesAvailable;

    rowsUsed  = 0;
    switchPtr = GET_SWITCH_PTR(sw);
    ext       = GET_SWITCH_EXT(sw);
    status    = FM_OK;
    ffuRulesUsed = 0;
    ffuRulesAvailable = 0;

    for (i = 0 ; i < switchPtr->macTableSize ; i++)
    {
        cacheEntry = &switchPtr->maTable[i];
        if (cacheEntry->state != FM_MAC_ENTRY_STATE_INVALID)
        {
            rowsUsed++;
        }
    }

    srvErr->macTableRowsUsed      = rowsUsed;
    srvErr->macTableRowsAvailable = switchPtr->macTableSize - rowsUsed;

    rowsUsed = FM10000_ARP_TABLE_ENTRIES - ext->pNextHopSysCtrl->arpTabFreeEntryCount;
    srvErr->nexthopTableRowsUsed      = rowsUsed;
    srvErr->nexthopTableRowsAvailable = ext->pNextHopSysCtrl->arpTabFreeEntryCount;

    status = fmGetAclFfuRuleUsage(sw,
                                  &ffuRulesUsed,
                                  &ffuRulesAvailable);

    if (status == FM_OK)
    {
        srvErr->ffuRulesUsed = ffuRulesUsed;
        srvErr->ffuRulesAvailable = ffuRulesAvailable;
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "Error in fmGetAclFfuRuleUsage (err=%d):\n"
                     "%s\n"
                     "Setting response values to default.\n",
                     status,
                     fmErrorMsg(status));

        srvErr->ffuRulesUsed      = 0;
        srvErr->ffuRulesAvailable = FM10000_FFU_SLICE_VALID_ENTRIES * FM10000_FFU_SLICE_TCAM_ENTRIES_0;
    }

}   /* end fm10000SetHostSrvErrResponse */




/*****************************************************************************/
/** fm10000PCIeMailboxInterruptHandler
 * \ingroup intMailbox
 *
 * \desc            Handle mailbox interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000PCIeMailboxInterruptHandler(fm_int sw,
                                             fm_int pepNb)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     swToExecute;
    fm_uint64  regAddr;
    fm_uint32  rv;
    fm_bool    processRequest;
    fm_bool    processGlobalAck;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status           = FM_OK;
    processRequest   = FALSE;
    processGlobalAck = FALSE;
    swToExecute      = sw;

#if FM_SUPPORT_SWAG
    swToExecute = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);
#endif

    /* Take the Write lock instead of the Read one to protect the Logical Port
     * Structure from parallel access at the application level. At this point,
     * only the Read Switch lock must be taken by the interrupt handler 
     * thread. */
    UNPROTECT_SWITCH(sw);

    if (sw != swToExecute)
    {
        /* Take lock for SWAG switch. */
        LOCK_SWITCH(swToExecute);
    }

    LOCK_SWITCH(sw);

    if (sw != swToExecute)
    {
        /* Take mailbox lock for SWAG switch. */
        FM_TAKE_MAILBOX_LOCK(swToExecute);
    }

    FM_TAKE_MAILBOX_LOCK(sw);

    /* Ensure that the switch is UP, otherwise just ignore the interrupt. 
       This is to avoid race condition. */
    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr->state != FM_SWITCH_STATE_UP)
    {
        status = FM_ERR_SWITCH_NOT_UP;
        goto ABORT;
    }

    regAddr = FM10000_PCIE_GMBX();

    status = fm10000ReadPep(sw,
                            regAddr,
                            pepNb,
                            &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    processRequest = FM_GET_BIT(rv,
                                FM10000_PCIE_GMBX,
                                GlobalReqInterrupt);

    processGlobalAck = FM_GET_BIT(rv,
                                  FM10000_PCIE_GMBX,
                                  GlobalAckInterrupt);

    if (processRequest)
    {
        status = PCIeMailboxProcessRequest(sw, pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }
    else if (processGlobalAck)
    {
        status = PCIeMailboxProcessGlobalAck(sw, pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    FM_DROP_MAILBOX_LOCK(sw);

    if (sw != swToExecute)
    {
        FM_DROP_MAILBOX_LOCK(swToExecute);
    }

    UNLOCK_SWITCH(sw);

    if (sw != swToExecute)
    {
        UNLOCK_SWITCH(swToExecute);
    }

    PROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000PCIeMailboxInterruptHandler */




/*****************************************************************************/
/** fm10000MailboxAllocateDataStructures
 * \ingroup intMailbox
 *
 * \desc            Allocates mailbox data structures upon switch insertion.
 *
 * \param[in]       sw is the switch on whiich to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MailboxAllocateDataStructures(fm_int sw)
{
    fm_status       status;
    fm_uint32       nbytes;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status    = FM_OK;
    info      = GET_MAILBOX_INFO(sw);

    nbytes = sizeof(fm_int) * FM10000_NUM_PEPS;

    info->numberOfVirtualPortsAddedToUcastFlood = fmAlloc(nbytes);

    if (info->numberOfVirtualPortsAddedToUcastFlood == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    info->numberOfVirtualPortsAddedToMcastFlood = fmAlloc(nbytes);

    if (info->numberOfVirtualPortsAddedToMcastFlood == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    info->numberOfVirtualPortsAddedToBcastFlood = fmAlloc(nbytes);

    if (info->numberOfVirtualPortsAddedToBcastFlood == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxAllocateDataStructures */




/*****************************************************************************/
/** fm10000MailboxInit
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
fm_status fm10000MailboxInit(fm_int sw)
{
    fm_status       status;
    fm_mailboxInfo *info;
    fm_int          logicalPort;
    fm_bool         bypassStp;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status    = FM_OK;
    info      = GET_MAILBOX_INFO(sw);
    bypassStp = FM_ENABLED;

    /* Perform initialization in single chip configuration */
    if ( (GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)) == sw)
    {
        status = AllocateMailboxGlortCams(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Create default mcast groups for handling XCAST_MODES request */
        status = fmCreateMcastGroupInt(sw,
                                       &info->mcastGroupForUcastFlood,
                                       FALSE,
                                       TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmSetMcastGroupAttribute(sw,
                                          info->mcastGroupForUcastFlood,
                                          FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK,
                                          &bypassStp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        logicalPort = FM_PORT_FLOOD;
        status = fmSetMcastGroupAttributeInt(sw,
                                             info->mcastGroupForUcastFlood,
                                             FM_MCASTGROUP_LOGICAL_PORT,
                                             &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmActivateMcastGroup(sw, info->mcastGroupForUcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmCreateMcastGroupInt(sw,
                                       &info->mcastGroupForMcastFlood,
                                       FALSE,
                                       TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmSetMcastGroupAttribute(sw,
                                          info->mcastGroupForMcastFlood,
                                          FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK,
                                          &bypassStp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        logicalPort = FM_PORT_MCAST;
        status = fmSetMcastGroupAttributeInt(sw,
                                             info->mcastGroupForMcastFlood,
                                             FM_MCASTGROUP_LOGICAL_PORT,
                                             &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmActivateMcastGroup(sw, info->mcastGroupForMcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmCreateMcastGroupInt(sw,
                                       &info->mcastGroupForBcastFlood,
                                       FALSE,
                                       TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmSetMcastGroupAttribute(sw,
                                          info->mcastGroupForBcastFlood,
                                          FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK,
                                          &bypassStp);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        logicalPort = FM_PORT_BCAST;
        status = fmSetMcastGroupAttributeInt(sw,
                                             info->mcastGroupForBcastFlood,
                                             FM_MCASTGROUP_LOGICAL_PORT,
                                             &logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmActivateMcastGroup(sw, info->mcastGroupForBcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmCreateBitArray(&info->innOutMacRuleInUse,
                                  FM10000_MAILBOX_MAX_INN_OUT_MAC_RULES);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxInit */




/*****************************************************************************/
/** fm10000MailboxFreeDataStructures
 * \ingroup intMailbox
 *
 * \desc            Free mailbox data structures upon switch removal.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MailboxFreeDataStructures(fm_int sw)
{
    fm_status       status;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status = FM_OK;
    info   = GET_MAILBOX_INFO(sw);

    if (info->numberOfVirtualPortsAddedToUcastFlood != NULL)
    {
        fmFree(info->numberOfVirtualPortsAddedToUcastFlood);
    }

    if (info->numberOfVirtualPortsAddedToMcastFlood != NULL)
    {
        fmFree(info->numberOfVirtualPortsAddedToMcastFlood);
    }

    if (info->numberOfVirtualPortsAddedToBcastFlood != NULL)
    {
        fmFree(info->numberOfVirtualPortsAddedToBcastFlood);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxFreeDataStructures */




/*****************************************************************************/
/** fm10000MailboxFreeResources
 * \ingroup intMailbox
 *
 * \desc            Release mailbox resources held by a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MailboxFreeResources(fm_int sw)
{
    fm_status       status;
    fm_mailboxInfo *info;
    fm_int          pepNb;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status = FM_OK;

    /* Perform cleanup in single chip configuration */
    if ( (GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)) != sw)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    }

    info = GET_MAILBOX_INFO(sw);

    /* Delete mcast groups created for handling XCAST_MODES request */
    if (info->mcastGroupForUcastFlood >= 0)
    {
        status = fmDeactivateMcastGroup(sw, info->mcastGroupForUcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmDeleteMcastGroupInt(sw, info->mcastGroupForUcastFlood, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        info->mcastGroupForUcastFlood = -1;
    }

    if (info->mcastGroupForMcastFlood >= 0)
    {
        status = fmDeactivateMcastGroup(sw, info->mcastGroupForMcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmDeleteMcastGroupInt(sw, info->mcastGroupForMcastFlood, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        info->mcastGroupForMcastFlood = -1;
    }

    if (info->mcastGroupForBcastFlood >= 0)
    {
        status = fmDeactivateMcastGroup(sw, info->mcastGroupForBcastFlood);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmDeleteMcastGroupInt(sw, info->mcastGroupForBcastFlood, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        info->mcastGroupForBcastFlood = -1;
    }

    for (pepNb = 0; pepNb < FM10000_NUM_PEPS; pepNb++)
    {
        status = fm10000SetMailboxGlobalInterrupts(sw, 
                                                   pepNb,
                                                   FM_DISABLED);

        /* Ignore the error if the PEP is simply in reset */
        if (status == FM_ERR_INVALID_STATE)
        {
            status = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    status = fmDeleteBitArray(&info->innOutMacRuleInUse);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxFreeResources */




/*****************************************************************************/
/** fm10000SetMailboxGlobalInterrupts
 * \ingroup intMailbox
 *
 * \desc            Enable/Disable mailbox interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       enable is TRUE to enable interrupts, FALSE to disable them.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetMailboxGlobalInterrupts(fm_int  sw,
                                            fm_int  pepNb,
                                            fm_bool enable)
{
    fm_status status;
    fm_uint32 rv;
    fm_uint32 regAddr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d pepNb=%d enable=%d\n",
                 sw,
                 pepNb,
                 enable);

    status = FM_OK;

    /* Set GlobalInterruptEnable field in PCIE_GMBX reg */
    regAddr = FM10000_PCIE_GMBX();
    
    rv = 0;
    if (enable)
    {
        FM_SET_FIELD(rv, FM10000_PCIE_GMBX, GlobalInterruptEnable, 1);
    }
    else
    {
        FM_SET_FIELD(rv, FM10000_PCIE_GMBX, GlobalInterruptEnable, 2);
    }

    status = fm10000WritePep( sw, regAddr, pepNb, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_OK);

}   /* end fm10000SetMailboxGlobalInterrupts */




/*****************************************************************************/
/** fm10000ResetPepMailboxVersion
 * \ingroup intPort
 *
 * \desc            Reset the mailbox version for a PEP that just came out
 *                  of reset
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       pepNb is the PEP number.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ResetPepMailboxVersion(fm_int sw, fm_int pepNb)
{
    fm_switch              *switchPtr;
    fm_status               status;
    fm_mailboxControlHeader ctrlHdr;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_CLEAR(ctrlHdr);

    ctrlHdr.smVersion = FM_MAILBOX_VERSION_RESET;
        
    FM_API_CALL_FAMILY(status,
                       switchPtr->UpdateMailboxSmHdr,
                       sw,
                       pepNb,
                       &ctrlHdr,
                       FM_UPDATE_CTRL_HDR_VERSION);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = SignalRequestRead(sw, pepNb);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:
    return status;

}   /* end fm10000ResetPepMailboxVersion() */



/*****************************************************************************/
/** fm10000AllocVirtualLogicalPort
 * \ingroup intMailbox
 *
 * \desc            Allocate a continuous block of virtual logical port 
 *                  entries for given PEP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       numberOfPorts is the number of virtual logical ports
 *                  to allocate in the block.
 *
 * \param[in,out]   firstPort is the allocated logical port number for
 *                  the block.  On entry, firstPort contains a hint
 *                  value or FM_LOGICAL_PORT_ANY if no hint is to be provided.
 *                  On exit, firstPortNumber will contain the actual allocated
 *                  value, which may be different from the hint value.
 *
 * \param[in]       useHandle specifies a previously allocated block of
 *                  resources, or zero if no resources have been allocated.
 *                  Used mainly in stacking configurations. 
 *
 * \param[in]       firstGlort is the number of first glort to use.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AllocVirtualLogicalPort(fm_int  sw,
                                         fm_int  pepNb,
                                         fm_int  numberOfPorts,
                                         fm_int *firstPort,
                                         fm_int  useHandle,
                                         fm_int  firstGlort)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, numberOfPorts=%d, firstPort=%p"
                 " useHandle=%d, firstGlort=0x%x\n",
                 sw,
                 pepNb,
                 numberOfPorts,
                 (void *) firstPort,
                 useHandle,
                 firstGlort);

    status = fmCommonAllocVirtualLogicalPort(sw,
                                             numberOfPorts,
                                             firstPort,
                                             useHandle,
                                             firstGlort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* fm10000AllocVirtualLogicalPort */




/*****************************************************************************/
/** fm10000FreeVirtualLogicalPort
 * \ingroup intMailbox
 *
 * \desc            Deallocate a block of virtual logical ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       firstPort is is the first virtual logical port 
 *                  to deallocate.
 *
 * \param[in]       numberOfPorts is the number of virtual logical ports
 *                  to deallocate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeVirtualLogicalPort(fm_int  sw,
                                        fm_int  pepNb,
                                        fm_int  firstPort,
                                        fm_int  numberOfPorts)
{
    fm_status       status;
    fm_mailboxInfo *info;
    fm_int          i;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, firstPort=%d, numberOfPorts=%d\n",
                 sw,
                 pepNb,
                 firstPort,
                 numberOfPorts);

    info   = GET_MAILBOX_INFO(sw);
    status = FM_OK;

    for (i = 0 ; i < numberOfPorts ; i++)
    {
        status = CleanupFloodingListsWhenDeletingLport(sw,
                                                       pepNb,
                                                       firstPort + i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = CleanupResourcesWhenDeletingLport(sw,
                                                   pepNb,
                                                   firstPort + i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmFreeLogicalPort(sw, firstPort + i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        if (fmTreeIsInitialized(&info->mailboxResourcesPerVirtualPort))
        {
            status = fmTreeRemove(&info->mailboxResourcesPerVirtualPort,
                                  firstPort + i,
                                  fmFree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000FreeVirtualLogicalPort */




/*****************************************************************************/
/** fm10000FindInternalPortByMailboxGlort
 * \ingroup intMailbox
 *
 * \desc            Find the logical port matching a given mailbox glort.
 *                  This function is used when the switch must be able 
 *                  to address a glort that exists on a remote switch.
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
fm_status fm10000FindInternalPortByMailboxGlort(fm_int    sw,
                                                fm_uint32 glort,
                                                fm_int *  logicalPort)
{
    fm_status status;
    fm_bool   portFound;

#if FM_SUPPORT_SWAG
    fm_int          swagSw;
    fm_switch *     swagSwitchPtr;
    fmSWAG_switch *aggregatePtr;
    fm_swagMember * member;
    fm_mailboxInfo *info;
    fm_uint32       firstGlort;
    fm_uint32       lastGlort;
    fm_uint16       numberOfPeps;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, glort=0x%x, logicalPort=%p\n",
                 sw,
                 glort,
                 (void *) logicalPort);

    portFound = FALSE;
    status    = FM_OK;

    if ( (glort < FM10000_MAILBOX_GLORT_BASE)
        || (glort > FM10000_MAILBOX_GLORT_MAX) )
    {
        status = FM_ERR_NOT_FOUND; 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

#if FM_SUPPORT_SWAG
    
    swagSw = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);

    if (swagSw != sw)
    {
        swagSwitchPtr = GET_SWITCH_PTR(swagSw);
        aggregatePtr  = swagSwitchPtr->extension;
        member        = fmGetFirstSwitchInSWAG(aggregatePtr);
        numberOfPeps  = FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS();

        while (member != NULL)
        {
            if (member->swId != sw)
            {
                info = GET_MAILBOX_INFO(member->swId);

                firstGlort = info->glortBase;
                lastGlort = info->glortBase + (numberOfPeps * info->glortsPerPep);

                if ( (glort >= firstGlort) && (glort < lastGlort) )
                {
                    *logicalPort = member->switchTrunks[sw]->trunk.trunkPort;
                    portFound  = TRUE;

                    break;
                }
            }

            member = fmGetNextSwitchInSWAG(member);
        }
    }
#endif

    if (!portFound)
    {
        status = FM_ERR_NOT_FOUND;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}




/*****************************************************************************/
/** fm10000SetXcastFlooding
 * \ingroup intMailbox
 *
 * \desc            Check portmask for ucast/mcast/bcast flooding glorts.
 *                  Add/delete pep port to portmask if needed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       mode is the xcast mode to be set.
 *
 * \param[in]       xcastFloodMode is a flag controlling 
 *                  which flooding lists should be updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetXcastFlooding(fm_int       sw,
                                  fm_int       pepNb,
                                  fm_xcastMode mode,
                                  fm_uint16    xcastFloodMode)
{
    fm_switch *     switchPtr;
    fm_mailboxInfo *info;
    fm_status       status;
    fm_portmask     ucastFloodDestMask;
    fm_portmask     mcastFloodDestMask;
    fm_portmask     bcastFloodDestMask;
    fm_int          pepPort;

     FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                  "sw=%d, pepNb=%d, mode=%d\n",
                  sw,
                  pepNb,
                  mode);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MAILBOX_INFO(sw);

    FM_CLEAR(ucastFloodDestMask);
    FM_CLEAR(mcastFloodDestMask);
    FM_CLEAR(bcastFloodDestMask);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_FLOOD,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &ucastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_MCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &mcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_BCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &bcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapPepToLogicalPort,
                       sw,
                       pepNb,
                       &pepPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    switch (mode)
    {
        case FM_HOST_SRV_XCAST_MODE_ALLMULTI:

            /* Add pep port to multicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&mcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_MCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &mcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToMcastFlood[pepNb]++;
            }

            /* Remove pep port from unicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&ucastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_FLOOD,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &ucastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToUcastFlood[pepNb]--;
            }

            /* Add pep port to broadcast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&bcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_BCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &bcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToBcastFlood[pepNb]++;
            }

            break;

        case FM_HOST_SRV_XCAST_MODE_MULTI:

            /* Remove pep port from multicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&mcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_MCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &mcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToMcastFlood[pepNb]--;
            }

            /* Remove pep port from unicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&ucastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_FLOOD,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &ucastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToUcastFlood[pepNb]--;
            }

            /* Add pep port to broadcast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&bcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_BCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &bcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToBcastFlood[pepNb]++;
            }

            break;

        case FM_HOST_SRV_XCAST_MODE_PROMISC:

            /* Add pep port to multicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&mcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_MCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &mcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToMcastFlood[pepNb]++;
            }

            /* Add pep port to unicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&ucastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_FLOOD,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &ucastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToUcastFlood[pepNb]++;
            }

            /* Add pep port to broadcast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 0) &&
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 0)
                {
                    FM_PORTMASK_ENABLE_BIT(&bcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_BCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &bcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToBcastFlood[pepNb]++;
            }

            break;

        case FM_HOST_SRV_XCAST_MODE_NONE:

            /* Remove pep port from multicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_MCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToMcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&mcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_MCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &mcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToMcastFlood[pepNb]--;
            }

            /* Remove pep port from unicast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_UCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToUcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&ucastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_FLOOD,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &ucastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToUcastFlood[pepNb]--;
            }

            /* Remove pep port from broadcast flooding portmask if needed. */
            if (xcastFloodMode & FM_MAILBOX_XCAST_FLOOD_MODE_BCAST)
            {
                if ( (info->numberOfVirtualPortsAddedToBcastFlood[pepNb] == 1) &&
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, pepPort)) == 1)
                {
                    FM_PORTMASK_DISABLE_BIT(&bcastFloodDestMask, pepPort);
                    status = fmSetLogicalPortAttribute(sw,
                                                       FM_PORT_BCAST,
                                                       FM_LPORT_DEST_MASK,
                                                       (void *) &bcastFloodDestMask);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                }

                info->numberOfVirtualPortsAddedToBcastFlood[pepNb]--;
            }

            break;

        default:
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, FM_ERR_INVALID_VALUE);

            break;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000SetXcastFlooding */




/*****************************************************************************/
/** fm10000AssociateMcastGroupWithFlood
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
fm_status fm10000AssociateMcastGroupWithFlood(fm_int                sw,
                                              fm_int                pepNb,
                                              fm_int                floodPort,
                                              fm_intMulticastGroup *mcastGroup,
                                              fm_bool               associate)
{
    fm_status               status;
    fm_uint32               mcastIdx;
    fm_intReplicationGroup *repliGroup;
    fm_multicastListener    listener;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, floodPort=%d, mcastGroup=%p, associate=%d\n",
                 sw,
                 pepNb,
                 floodPort,
                 (void *) mcastGroup,
                 associate);

    status     = FM_OK;
    mcastIdx   = 0;
    repliGroup = NULL;

    status = fmGetLogicalPortAttribute(sw,
                                       floodPort,
                                       FM_LPORT_MULTICAST_INDEX,
                                       &mcastIdx);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (associate)
    {
        if (mcastIdx == 0)
        {
            repliGroup = findReplicationGroup(sw, mcastGroup->repliGroup);
            if (repliGroup == NULL)
            {
                status = FM_ERR_INVALID_MULTICAST_GROUP;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            status = fmSetLogicalPortAttribute(sw,
                                               floodPort,
                                               FM_LPORT_MULTICAST_INDEX,
                                               (void *)&repliGroup->hwDestIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }
    else
    {
        if (mcastIdx != 0)
        {
            repliGroup = findReplicationGroup(sw, mcastGroup->repliGroup);
            if (repliGroup == NULL)
            {
                status = FM_ERR_INVALID_MULTICAST_GROUP;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            /* If there is no other listener, clear the mcastIndex */
            status = fmGetMcastGroupListenerFirst(sw,
                                                  mcastGroup->handle,
                                                  &listener);
            if (status == FM_ERR_NO_MORE)
            {
                mcastIdx = 0;
                status = fmSetLogicalPortAttribute(sw,
                                                   floodPort,
                                                   FM_LPORT_MULTICAST_INDEX,
                                                   (void *)&mcastIdx);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status); 

}   /* end fm10000AssociateMcastGroupWithFlood */




/*****************************************************************************/
/** fm10000GetMailboxGlortRange
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
fm_status fm10000GetMailboxGlortRange(fm_int     sw,
                                      fm_int     pepNb,
                                      fm_uint32 *glortBase,
                                      fm_int *   numberOfGlorts)
{
    fm_status       status;
    fm_mailboxInfo *info;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status = FM_OK;
    info   = GET_MAILBOX_INFO(sw);

    *glortBase      = info->glortBase + (pepNb * info->glortsPerPep);
    *numberOfGlorts = info->glortsPerPep;

   FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}  /* end fm10000GetMailboxGlortRange */




/*****************************************************************************/
/** fm10000AnnounceTxTimestampMode
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
fm_status fm10000AnnounceTxTimestampMode(fm_int  sw,
                                         fm_bool isTxTimestampEnabled)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_status  status;
    fm_int     pepNb;
    fm_int     logicalPort;
    fm_int     i;
    fm_uint32  glort;
    fm_mailboxControlHeader controlHeader;
    fm_hostSrvTimestampModeResp timestampMode;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, isTxTimestampEnabled=%d\n",
                 sw,
                 isTxTimestampEnabled);

    switchPtr = GET_SWITCH_PTR(sw);
    status  = FM_OK;
    glort   = 0;
    pepNb   = -1;

    status = fmGetSwitchAttribute(sw,
                                  FM_SWITCH_ETH_TIMESTAMP_OWNER,
                                  &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* If no Ethernet Timestamp Owner is configured, send txTimestampMode
     * as FM_DISABLED to all PEPs. */
    if (logicalPort != -1)
    {
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
                           &pepNb);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    /* Send message to all PEPs */
    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        FM_CLEAR(controlHeader);
        FM_CLEAR(timestampMode);

        FM_API_CALL_FAMILY(status,
                           switchPtr->ReadMailboxControlHdr,
                           sw,
                           i,
                           &controlHeader);

        /* Ignore peps in reset or disabled */
        if (status == FM_ERR_INVALID_STATE)
        {
            continue;
        }
        else if (status != FM_OK)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        if (i == pepNb)
        {
            timestampMode.modeEnabled = isTxTimestampEnabled;
            timestampMode.glort = glort;
        }
        else
        {
            timestampMode.modeEnabled = FM_DISABLED;
            timestampMode.glort = 0;
        }

        FM_API_CALL_FAMILY(status,
                           switchPtr->WriteMailboxResponseMessage,
                           sw,
                           i,
                           &controlHeader,
                           FM_MAILBOX_MSG_SET_TIMESTAMP_MODE_ID,
                           FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE,
                           (void *) &timestampMode);

        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Cannot send TX_TIMESTAMP_MODE to PEP %d (err=%d)\n",
                         i,
                         status);
        }
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000AnnounceTxTimestampMode */




/*****************************************************************************/
/** fm10000MasterClkOffsetProcess
 * \ingroup intMailbox
 *
 * \desc            Process MASTER_CLK_OFFSET message.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the number of PEP receiving message.
 *
 * \param[in]       clkOffset points to a structure containing 
 *                  offset value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MasterClkOffsetProcess(fm_int                     sw,
                                        fm_int                     pepNb,
                                        fm_hostSrvMasterClkOffset *clkOffset)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     i;
    fm_mailboxControlHeader controlHeader;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, offsetValueLower=0x%x, "
                 "offsetValueUpper=0x%x\n",
                 sw,
                 pepNb,
                 clkOffset->offsetValueLower,
                 clkOffset->offsetValueUpper);

    switchPtr = GET_SWITCH_PTR(sw);
    status  = FM_OK;

    /* Send message to all PEPs excluding receiving one. */
    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        if (i != pepNb)
        {
            FM_CLEAR(controlHeader);

            FM_API_CALL_FAMILY(status,
                               switchPtr->ReadMailboxControlHdr,
                               sw,
                               i,
                               &controlHeader);

            /* Ignore peps in reset or disabled */
            if (status == FM_ERR_INVALID_STATE)
            {
                continue;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }

            FM_API_CALL_FAMILY(status,
                               switchPtr->WriteMailboxResponseMessage,
                               sw,
                               i,
                               &controlHeader,
                               FM_MAILBOX_MSG_MASTER_CLK_OFFSET_ID,
                               FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE,
                               (void *) clkOffset);

            if (status != FM_OK)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Cannot send MASTER_CLK_OFFSET to PEP %d (err=%d)\n",
                             i,
                             status);
            }
        }
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MasterClkOffsetProcess */




/*****************************************************************************/
/** fm10000MailboxConfigureCounters
 * \ingroup intMailbox
 *
 * \desc            Configures mailbox counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MailboxConfigureCounters(fm_int sw)
{
    fm_mailboxInfo *info;
    fm_status       status;
    fm_uint16       nbytes;
    fm_int          value;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status = FM_OK;
    info   = GET_MAILBOX_INFO(sw);

    nbytes = sizeof(fm_int) * FM10000_NUM_PEPS;

    info->macEntriesAdded = fmAlloc(nbytes);

    if (info->macEntriesAdded == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    FM_CLEAR(*info->macEntriesAdded);

    info->innerOuterMacEntriesAdded = fmAlloc(nbytes);

    if (info->innerOuterMacEntriesAdded == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    FM_CLEAR(*info->innerOuterMacEntriesAdded);

    value = fmGetIntApiProperty(FM_AAK_API_HNI_MAC_ENTRIES_PER_PEP,
                                FM_AAD_API_HNI_MAC_ENTRIES_PER_PEP);

    info->maxMacEntriesToAddPerPep = value;

    value = fmGetIntApiProperty(FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PEP,
                                FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PEP);

    info->maxInnerOuterMacEntriesToAddPerPep = value;

    value = fmGetIntApiProperty(FM_AAK_API_HNI_MAC_ENTRIES_PER_PORT,
                                FM_AAD_API_HNI_MAC_ENTRIES_PER_PORT);

    info->maxMacEntriesToAddPerPort = value;

    value = fmGetIntApiProperty(FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PORT,
                                FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PORT);

    info->maxInnerOuterMacEntriesToAddPerPort = value;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxConfigureCounters */




/*****************************************************************************/
/** fm10000MailboxUnconfigureCounters
 * \ingroup intMailbox
 *
 * \desc            Unconfigures mailbox counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MailboxUnconfigureCounters(fm_int sw)
{
    fm_mailboxInfo *info;
    fm_status       status;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d\n",
                 sw);

    status = FM_OK;
    info   = GET_MAILBOX_INFO(sw);

    fmFree(info->macEntriesAdded);

    fmFree(info->innerOuterMacEntriesAdded);

    info->maxMacEntriesToAddPerPep            = -1;
    info->maxInnerOuterMacEntriesToAddPerPep  = -1;
    info->maxMacEntriesToAddPerPort           = -1;
    info->maxInnerOuterMacEntriesToAddPerPort = -1;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxUnconfigureCounters */

