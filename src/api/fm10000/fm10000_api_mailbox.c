/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mailbox.c
 * Creation Date:   October 16, 2013
 * Description:     Function for operating mailbox.
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

/* The delay to wait in nsec before retrying if we detect PEP is in reset*/
#define PEP_RESET_RECOVERY_DELAY                (1000 * 1000)

/* The number of retries if we we detect the PEP is in reset */
#define PEP_RESET_RECOVERY_RETRIES              3

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

#define GET_FLOW_TABLE_KEY(flowId, flowTable) \
    ( ( ( (fm_uint32) flowId ) & 0xFFFFFFFFl) | ( (fm_uint64) flowTable << 32 ) )

#define GET_FLOW_FROM_FLOW_TABLE_KEY(key) \
    ( (fm_uint32) (key & 0xFFFFFFFFl) )

#define GET_TABLE_FROM_FLOW_TABLE_KEY(key) \
    ( (fm_uint32) ( (key >> 32) & 0xFFFFFFFFl) )

#define GET_FLOW_GLORT_KEY(flowId, glort) \
    ( ( ( (fm_uint32) flowId ) & 0xFFFFFFFFl) | ( (fm_uint64) glort << 32 ) )

#define GET_GLORT_FROM_FLOW_GLORT_KEY(key) \
    ( (fm_uint32) ( (key >> 32) & 0xFFFFFFFFl) )

#define GET_FLOW_FROM_FLOW_GLORT_KEY(key) \
    ( (fm_uint32) (key & 0xFFFFFFFFl) )

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

    /* Update head value in SM header */
    FM_API_CALL_FAMILY(status,
                       switchPtr->UpdateMailboxSmHdr,
                       sw,
                       pepNb,
                       ctrlHdr,
                       FM_UPDATE_CTRL_HDR_REQUEST_HEAD);

    if (status != FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "UpdateMailboxSmHdr: error while updating "
                     "SM header (err = %d)\n",
                     status);
    }

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

        case FM_MAILBOX_MSG_CREATE_TABLE_ID:
            return "CREATE_TABLE";

        case FM_MAILBOX_MSG_DESTROY_TABLE_ID:
            return "DESTROY_TABLE";

        case FM_MAILBOX_MSG_GET_TABLES_ID:
            return "GET_TABLES";

        case FM_MAILBOX_MSG_SET_RULES_ID:
            return "SET_RULES";

        case FM_MAILBOX_MSG_GET_RULES_ID:
            return "GET_RULES";

        case FM_MAILBOX_MSG_DEL_RULES_ID:
            return "DEL_RULES";

        case FM_MAILBOX_MSG_SET_NO_OF_VFS_ID:
            return "SET_NO_OF_VFS";

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
    emptyBytes = emptyRows * FM_MBX_ENTRY_BYTE_LENGTH;

    /* Add response message header length */
    responseMsgLength += FM_MBX_ENTRY_BYTE_LENGTH;

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
/** WriteMultToResponseQueue
 * \ingroup intMailbox
 *
 * \desc            Write multiple 32-bit values to response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       count is the number of 32-bit words to be written.
 *
 * \param[in]       value points to an array of 32-bit values to be written
 *                  to consecutive queue elements.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteMultToResponseQueue(fm_int                   sw,
                                          fm_int                   pepNb,
                                          fm_int                   count,
                                          fm_uint32 *              value,
                                          fm_mailboxControlHeader *ctrlHdr)
{
    fm_int    index;
    fm_int    i;
    fm_uint64 regAddr;
    fm_status status;

    status = FM_OK;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MAILBOX,
                         "sw = %d, pepNb = %d, count = %d, value = %p, ctrlHdr = %p\n",
                         sw,
                         pepNb,
                         count,
                         (void *) value,
                         (void *) ctrlHdr);

    for (i = 0 ; i < count ; i++)
    {
        index   = CALCULATE_SM_OFFSET_FROM_QUEUE_INDEX(ctrlHdr->respTail);
        regAddr = FM10000_PCIE_MBMEM(index);

        status = fm10000WritePep(sw,
                                 regAddr,
                                 pepNb,
                                 value[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        INCREMENT_QUEUE_INDEX(ctrlHdr->respTail);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteMultToResponseQueue */




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
 * \param[in]       flags is the flag bitmask for message header.
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
                                            fm_byte                  flags,
                                            fm_uint16                msgLength)
{
    fm_status  status;
    fm_uint32  rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, msgTypeId=%d, flags=0x%x, msgLength=%d\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 msgTypeId,
                 flags,
                 msgLength);

    status = FM_OK;
    rv     = 0;

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_TYPE,
                 msgTypeId);

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

    FM_SET_FIELD(rv,
                 FM_MAILBOX_MESSAGE_HEADER,
                 MESSAGE_FLAGS,
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
/** WriteDataToQueue
 * \ingroup intMailbox
 *
 * \desc            Write a burst of response data to mailbox response queue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       msgTypeId is the response message type id.
 *
 * \param[in]       argType is the response argument type.
 *
 * \param[in]       entriesToWrite is the number of data entries to be written.
 *
 * \param[in]       flags is the flag bitmask for message header.
 *
 * \param[in]       dataToWrite is the data entries to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteDataToQueue(
                      fm_int                        sw,
                      fm_int                        pepNb,
                      fm_mailboxControlHeader *     ctrlHdr,
                      fm_mailboxMessageId           msgTypeId,
                      fm_mailboxMessageArgumentType argType,
                      fm_int                        entriesToWrite,
                      fm_byte                       flags,
                      fm_uint32 *                   dataToWrite)
{
    fm_switch *switchPtr;
    fm_status  status;

    switchPtr = GET_SWITCH_PTR(sw);

    status = WriteResponseMessageHeader(
                  sw,
                  pepNb,
                  ctrlHdr,
                  msgTypeId,
                  flags,
                  (entriesToWrite  + 1) * FM_MBX_ENTRY_BYTE_LENGTH);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = WriteResponseArgumentHeader(
                  sw,
                  pepNb,
                  ctrlHdr,
                  argType,
                  entriesToWrite * FM_MBX_ENTRY_BYTE_LENGTH);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    if (dataToWrite != NULL)
    {
        status = WriteMultToResponseQueue(sw,
                                          pepNb,
                                          entriesToWrite,
                                          dataToWrite,
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

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteDataToQueue */




/*****************************************************************************/
/** WriteResponseData
 * \ingroup intMailbox
 *
 * \desc            Writes response data.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[in]       msgTypeId is the response message type id.
 *
 * \param[in]       argType is the response argument type.
 *
 * \param[in]       dataEntries is the number of data entries to be written.
 *
 * \param[in]       dataToWrite is the data entries to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteResponseData(
                      fm_int                        sw,
                      fm_int                        pepNb,
                      fm_mailboxControlHeader *     ctrlHdr,
                      fm_mailboxMessageId           msgTypeId,
                      fm_mailboxMessageArgumentType argType,
                      fm_int                        dataEntries,
                      fm_uint32 *                   dataToWrite)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     emptyQueueData;
    fm_int     entriesSent;
    fm_int     entriesToSend;
    fm_int     emptyQueueElements;
    fm_int     retries;
    fm_byte    messageFlags;
    fm_bool    messageRead;
    fm_uint32  rv;
    fm_uint64  regAddr;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, msgTypeId=%d, argType=%d, dataEntries=%d\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 msgTypeId,
                 argType,
                 dataEntries);

    switchPtr          = GET_SWITCH_PTR(sw);
    status             = FM_OK;
    emptyQueueData     = 0;
    entriesSent        = 0;
    entriesToSend      = 0;
    emptyQueueElements = 0;
    messageFlags       = 0;
    regAddr            = 0;
    messageRead        = FALSE;

    if (dataEntries == 0)
    {
        /* In case we don't have any data entries, just write headers and
         * exit with FM_OK.
         */
        messageFlags = FM_MAILBOX_HEADER_TRANSACTION_FLAG |
                       FM_MAILBOX_HEADER_START_OF_MESSAGE |
                       FM_MAILBOX_HEADER_END_OF_MESSAGE;
        status = WriteDataToQueue(sw,
                                  pepNb,
                                  ctrlHdr,
                                  msgTypeId,
                                  argType,
                                  entriesToSend,
                                  messageFlags,
                                  dataToWrite);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);
    }

    /* Write data to response queue. */
     while (entriesSent < dataEntries)
     {
         emptyQueueElements = CALCULATE_EMPTY_QUEUE_ELEMENTS(ctrlHdr->respHead,
                                                             ctrlHdr->respTail);

         /* Setting transaction flags */
         messageFlags = FM_MAILBOX_HEADER_TRANSACTION_FLAG;

         if (entriesSent == 0)
         {
             messageFlags |= FM_MAILBOX_HEADER_START_OF_MESSAGE;
         }

         if ((dataEntries - entriesSent) <= emptyQueueElements)
         {
             messageFlags |= FM_MAILBOX_HEADER_END_OF_MESSAGE;
         }

         /* Need two entries for transaction and argument headers. */
         if (((dataEntries - entriesSent) + 2) > emptyQueueElements)
         {
             entriesToSend = emptyQueueElements - 2;
         }
         else
         {
             entriesToSend = (dataEntries - entriesSent);
         }

         status = WriteDataToQueue(sw,
                                   pepNb,
                                   ctrlHdr,
                                   msgTypeId,
                                   argType,
                                   entriesToSend,
                                   messageFlags,
                                   &dataToWrite[entriesSent]);
         FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

         entriesSent += entriesToSend;

         /* For messages that need to be fragmented. */
         if (entriesSent < dataEntries)
         {
             retries = 0;

             while (retries < FM10000_MAILBOX_FRAGMENTATION_RETIRES)
             {
                 rv = 0;

                 /* We need to wait for GlobalAckInterrupt bit in PCIE_GMBX
                  * register to know that previous part of message has been
                  * read by PF.
                  */
                 regAddr = FM10000_PCIE_GMBX();

                 status = fm10000ReadPep(sw,
                                         regAddr,
                                         pepNb,
                                         &rv);
                 FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                 messageRead =  FM_GET_BIT(rv,
                                           FM10000_PCIE_GMBX,
                                           GlobalAckInterrupt);

                 if (messageRead)
                 {
                     /* Clear GlobalAckInterrupt bit. */
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

                     break;
                 }

                 fmDelayBy(0, FM10000_MAILBOX_FRAGMENTATION_DELAY);
                 retries++;

             }

             /* If timeout occurs, just stop sending message. */
             if (retries >= FM10000_MAILBOX_FRAGMENTATION_RETIRES)
             {
                 status = FM_OK;
                 goto ABORT;
             }
         }
     }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end WriteResponseData */




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
    fm_mailboxControlHeader controlHeader;
    fm_mailboxMessageHeader pfTransactionHeader;
    fm_uint64               regAddr;
    fm_uint32               rv;
    fm_int                  index;
    fm_int                  swToExecute;
    fm_int                  pepNbToExecute;
    fm_bool                 versionSet;
    fm_bool                 useLoopback;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    switchPtr         = GET_SWITCH_PTR(sw);
    status            = FM_OK;
    rv                = 0;
    useLoopback       = FALSE;
    versionSet        = TRUE;
    index             = 0;
    regAddr           = 0;

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

    useLoopback = GET_FM10000_PROPERTY()->useHniServicesLoopback;

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

            /* Try to ingore headers with no transaction flag set. */
            if ( (pfTransactionHeader.flags &
                  FM_MAILBOX_HEADER_TRANSACTION_FLAG) == 0)
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

                case FM_MAILBOX_MSG_CREATE_TABLE_ID:
                    status = fmCreateTableProcess(swToExecute,
                                                  pepNbToExecute,
                                                  &controlHeader,
                                                  &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_DESTROY_TABLE_ID:
                    status = fmDestroyTableProcess(swToExecute,
                                                   pepNbToExecute,
                                                   &controlHeader,
                                                   &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_GET_TABLES_ID:
                    status = fmGetTablesProcess(swToExecute,
                                                pepNbToExecute,
                                                &controlHeader,
                                                &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_SET_RULES_ID:
                    status = fmSetRulesProcess(swToExecute,
                                               pepNbToExecute,
                                               &controlHeader,
                                               &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_GET_RULES_ID:
                    status = fmGetRulesProcess(swToExecute,
                                               pepNbToExecute,
                                               &controlHeader,
                                               &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_DEL_RULES_ID:
                    status = fmDelRulesProcess(swToExecute,
                                               pepNbToExecute,
                                               &controlHeader,
                                               &pfTransactionHeader);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
                    break;

                case FM_MAILBOX_MSG_SET_NO_OF_VFS_ID:
                    status = fmSetNoOfVfsProcess(swToExecute,
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
    fm_uint64 regAddr;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status  = FM_OK;

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
    destEntry   = NULL;

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
        dividedValue     = currentMinGlort / mailboxInfo->glortsPerPep;

        /* Calculate, how many glorts can be programmed in one CAM entry. */
        while (calculationDone == FALSE)
        {
            /* If this condition is met, double number of glorts to be allocated. */
            if ( ( (dividedValue % 2) == 0) 
                && ( (glortsToAllocate + glortsAllocated)  < numberOfGlorts) 
                && ( (currentMinGlort + (glortsToAllocate * 2) ) <= FM10000_MAILBOX_GLORT_MAX) )
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
                                           0);

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

        if (destEntry != NULL)
        {
            fmFree(destEntry);
            destEntry = NULL;
        }

        glortsAllocated += glortsToAllocate;
        currentMinGlort += glortsToAllocate;
    }

    status = FM_OK;

ABORT:

    if (destEntry != NULL)
    {
        fmFree(destEntry);
    }

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
                 (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
            {
                status = fmDisablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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

    doSearching = TRUE;
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
                 (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
            {
                status = fmDisablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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

    doSearching = TRUE;
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
                 (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
            {
                status = fmDisablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
/** CleanupMacResources
 * \ingroup intMailbox
 *
 * \desc            Cleanup mac table when deleting virtual port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       portNumber is the number of logical port being deleted.
 *
 * \param[in]       resources tracks resources created on Host Interface
 *                  requests via mailbox for a given virtual port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanupMacResources(fm_int               sw,
                                     fm_int               pepNb,
                                     fm_int               portNumber,
                                     fm_mailboxResources *resources)
{
    fm_switch *     switchPtr;
    fm_mailboxInfo *info;
    fm_status       status;
    fm_int          mcastGroup;
    fm_uint16       vlan;
    fm_uint64       key;
    fm_macaddr      macAddr;
    void *          nextValue;
    fm_bool         routingLockTaken;
    fm_treeIterator treeIterMailboxRes;
    fm_multicastAddress  mcastAddr;
    fm_macAddressEntry   macEntry;
    fm_multicastListener listener;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    switchPtr        = GET_SWITCH_PTR(sw);
    info             = GET_MAILBOX_INFO(sw);
    status           = FM_OK;
    routingLockTaken = FALSE;

    /* Delete MAC entries associated with logical port. */
    fmTreeIterInit(&treeIterMailboxRes,
                   &resources->mailboxMacResource);

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
            else
            {
                info->macEntriesAdded[pepNb]--;
            }
        }
    }

    status = FM_OK;

    fmTreeDestroy(&resources->mailboxMacResource, NULL);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupMacResources */




/*****************************************************************************/
/** CleanupFlowResources
 * \ingroup intMailbox
 *
 * \desc            Cleanup flow resources when deleting virtual port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       portNumber is the number of logical port being deleted.
 *
 * \param[in]       resources tracks resources created on Host Interface
 *                  requests via mailbox for a given virtual port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanupFlowResources(fm_int               sw,
                                      fm_int               pepNb,
                                      fm_int               portNumber,
                                      fm_mailboxResources *resources)
{
    fm_mailboxInfo *     info;
    fm_status            status;
    fm_int               flowId;
    fm_int               matchFlowId;
    fm_int               tableIndex;
    fm_int               logicalPort;
    fm_int               pep;
    fm_int               index;
    fm_pciePortType      type;
    fm_uint32            glort;
    fm_uint64            key;
    fm_uint64 *          flowGlortKey;
    fm_treeIterator      treeIterMailboxRes;
    fm_treeIterator      treeFlowIter;
    fm_mailboxFlowTable *mailboxFlowTable;
    fm_mailboxResources *mailboxPfResources;
    fm_mailboxResources *mailboxVfResources;
    fm_flowTableType     flowTableType;
    fm_uintptr           treeValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    info   = GET_MAILBOX_INFO(sw);
    status = FM_OK;

    fmTreeIterInit(&treeIterMailboxRes,
                   &resources->mailboxFlowTableResource);

    /* Cleanup flow table tree. This tree should be used only for
     * PF port, otherwise it should be empty. */
    while ( fmTreeIterNext(&treeIterMailboxRes,
                           &key,
                           (void **) &mailboxFlowTable) != FM_ERR_NO_MORE )
    {
        fmTreeIterInit(&treeFlowIter,
                       &mailboxFlowTable->matchFlowIdMap);

        while ( fmTreeIterNext(&treeFlowIter,
                               &key,
                               (void **) &flowGlortKey) != FM_ERR_NO_MORE )
        {
            /* For PFs we need to cleanup VF flow table resources to be
             * in-sync. */
            flowId = GET_FLOW_FROM_FLOW_GLORT_KEY(*flowGlortKey);
            glort  = GET_GLORT_FROM_FLOW_GLORT_KEY(*flowGlortKey);

            status = fmGetGlortLogicalPort(sw,
                                           glort,
                                           &logicalPort);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Check glort type */
            status = fmGetLogicalPortPcie(sw,
                                          logicalPort,
                                          &pep,
                                          &type,
                                          &index);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            if (type == FM_PCIE_PORT_VF)
            {
                status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                                    logicalPort,
                                    (void **) &mailboxVfResources);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                key = GET_FLOW_TABLE_KEY(flowId, mailboxFlowTable->tableIndex);
                status = fmTreeRemove(&mailboxVfResources->mailboxFlowMap,
                                      key,
                                      NULL);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }

        fmTreeDestroy(&mailboxFlowTable->matchFlowIdMap, fmFree);
        fmTreeDestroy(&mailboxFlowTable->internalFlowIdMap, NULL);

        status = fmGetFlowTableType(sw,
                                    mailboxFlowTable->tableIndex,
                                    &flowTableType);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        switch (flowTableType)
        {
            case FM_FLOW_TCAM_TABLE:
                status = fmDeleteFlowTCAMTable(sw,
                                               mailboxFlowTable->tableIndex);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                break;

            case FM_FLOW_TE_TABLE:
                status = fmDeleteFlowTETable(sw,
                                             mailboxFlowTable->tableIndex);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                break;

            default:
                status = FM_ERR_UNSUPPORTED;
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                break;
        }
    }

    fmTreeDestroy(&resources->mailboxFlowTableResource, fmFree);
    mailboxFlowTable = NULL;
    status = FM_OK;

    /* Cleanup flow tree. This tree should be used only for VF ports,
     * otherwise it should be empty. */
    fmTreeIterInit(&treeFlowIter,
                   &resources->mailboxFlowMap);
    while ( fmTreeIterNext(&treeFlowIter,
                           &key,
                           (void **) &treeValue) != FM_ERR_NO_MORE )
    {
        flowId = GET_FLOW_FROM_FLOW_TABLE_KEY(key);
        tableIndex = GET_TABLE_FROM_FLOW_TABLE_KEY(key);

        status = fmDeleteFlow(sw,
                              tableIndex,
                              flowId);
        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Flow ID (0x%x) for flow table (0x%x) "
                         "is not present and cannot be deleted.\n",
                         flowId,
                         tableIndex);
        }

        /* For VFs we need to cleanup PF flow table resources to be in-sync. */
        status = fmGetPcieLogicalPort(sw,
                                      pepNb,
                                      FM_PCIE_PORT_PF,
                                      0,
                                      &logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxPfResources);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Check if match table index is valid. */
        status = fmTreeFind(&mailboxPfResources->mailboxFlowTableResource,
                            treeValue,
                            (void **) &mailboxFlowTable);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Get match flow ID. */
        status = fmTreeFind(&mailboxFlowTable->internalFlowIdMap,
                            flowId,
                            (void **) &matchFlowId);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmTreeRemove(&mailboxFlowTable->internalFlowIdMap,
                              flowId,
                              NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        status = fmTreeRemove(&mailboxFlowTable->matchFlowIdMap,
                              matchFlowId,
                              fmFree);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    fmTreeDestroy(&resources->mailboxFlowMap, NULL);
    status = FM_OK;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupFlowResources */




/*****************************************************************************/
/** CleanupMacFilteringResources
 * \ingroup intMailbox
 *
 * \desc            Cleanup resources allocated for inner/outer mac filtering
 *                  for given virtual port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       portNumber is the number of logical port being deleted.
 *
 * \param[in]       resources tracks resources created on Host Interface
 *                  requests via mailbox for a given virtual port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CleanupMacFilteringResources(fm_int               sw,
                                              fm_int               pepNb,
                                              fm_int               portNumber,
                                              fm_mailboxResources *resources)
{
    fm_status              status;
    fm_mailboxInfo *       info;
    fm_bool                ruleDeleted;
    fm_int                 bitNum;
    fm_int                 firstRule;
    fm_customTreeIterator  filterIterator;
    fm_hostSrvInnOutMac *  macFilterKey;
    fm_hostSrvInnOutMac *  macFilterVal;
    fm_mailboxMcastMacVni  macVniKey;
    fm_mailboxMcastMacVni *macVniVal;
    fm_multicastListener   listener;
    fm_aclCondition        cond;
    fm_aclValue            value;
    fm_aclActionExt        action;
    fm_aclParamExt         param;
    fm_char                statusText[1024];

    info        = GET_MAILBOX_INFO(sw);
    status      = FM_OK;
    ruleDeleted = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    fmCustomTreeIterInit(&filterIterator,
                         &resources->innOutMacResource);
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
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            ruleDeleted = TRUE;
        }
        else if (status == FM_ERR_NO_MORE)
        {
            break;
        }
        else
        {
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        /* The rule id scheme is:
         * ( (0b000000) |(High 12 bit) | (Low 14 bit)
         * where only 14 lower bits from rule id are taken from bitarray,
         * so we need to mask id before releasing it. */
        bitNum = macFilterVal->aclRule & 0x3FFF;

        status = fmSetBitArrayBit(&info->innOutMacRuleInUse,
                                  bitNum,
                                  0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Decrementing counters */
        resources->innerOuterMacEntriesAdded--;
        info->innerOuterMacEntriesAdded[pepNb]--;

        if (fmIsMulticastMacAddress(macFilterVal->innerMacAddr))
        {
            /* If inner MAC is a mcast one, ACL rule should redirect frame
             * to a multicast group. We need to remove listener
             * from such group. */

            macVniKey.macAddr = macFilterVal->innerMacAddr;
            macVniKey.vni     = macFilterVal->vni;

            status = fmCustomTreeFind(&info->mcastMacVni,
                                      &macVniKey,
                                      (void **) &macVniVal);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_CLEAR(listener);

            listener.port = portNumber;
            listener.vlan = 0;

            status = fmDeleteMcastGroupListener(sw,
                                                macVniVal->mcastGroup,
                                                &listener);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* If there is no other listeners, delete mcast group */
            status = fmGetMcastGroupListenerFirst(sw,
                                                  macVniVal->mcastGroup,
                                                  &listener);
            if (status == FM_ERR_NO_MORE)
            {
                status = fmDeactivateMcastGroup(sw,
                                                macVniVal->mcastGroup);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = fmDeleteMcastGroup(sw,
                                            macVniVal->mcastGroup);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                status = fmCustomTreeRemoveCertain(&info->mcastMacVni,
                                                   &macVniKey,
                                                   fmFreeMcastMacVni);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
            else if (status != FM_OK)
            {
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            }
        }
    }

    fmCustomTreeDestroy(&resources->innOutMacResource,
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
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
        else if (status != FM_OK)
        {
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }

        status = fmCompileACLExt(sw,
                                 statusText,
                                 sizeof(statusText),
                                 FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE |
                                 FM_ACL_COMPILE_FLAG_INTERNAL,
                                 (void*) &info->aclIdForMacFiltering);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                     "ACL compiled, status=%d, statusText=%s\n",
                     status,
                     statusText);

        status = fmApplyACLExt(sw,
                               FM_ACL_APPLY_FLAG_NON_DISRUPTIVE |
                               FM_ACL_APPLY_FLAG_INTERNAL,
                               (void*) &info->aclIdForMacFiltering);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
    }

    status = FM_OK;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupMacFilteringResources */




/*****************************************************************************/
/** CleanupResourcesWhenDeletingLport
 * \ingroup intMailbox
 *
 * \desc            Cleanup switch resources when deleting virtual port.
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
    fm_status            status;
    fm_mailboxInfo *     info;
    fm_mailboxResources *mailboxResourcesUsed;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, portNumber=%d\n",
                 sw,
                 pepNb,
                 portNumber);

    info                 = GET_MAILBOX_INFO(sw);
    mailboxResourcesUsed = NULL;

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        portNumber,
                        (void **) &mailboxResourcesUsed);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Delete MAC entries associated with logical port. */
    status = CleanupMacResources(sw,
                                 pepNb,
                                 portNumber,
                                 mailboxResourcesUsed);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Delete Flow tables and rules associated with logical port. */
    status = CleanupFlowResources(sw,
                                  pepNb,
                                  portNumber,
                                  mailboxResourcesUsed);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Delete MAC filtering entries associated with logical port. */
    status = CleanupMacFilteringResources(sw,
                                          pepNb,
                                          portNumber,
                                          mailboxResourcesUsed);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end CleanupResourcesWhenDeletingLport */




/*****************************************************************************/
/** ReadCreateDelLportArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for LPORT_CREATE/DELETE request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvPort points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadCreateDelLportArg(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_hostSrvPort *         srvPort,
                                       fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvPort=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvPort);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvPort->firstGlort = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_PORT,
                                       FIRST_GLORT);

    srvPort->glortCount = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_PORT,
                                       GLORT_COUNT);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadCreateDelLportArg */




/*****************************************************************************/
/** ReadUpdateMacArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for UPDATE_MAC_FWD_RULE request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvMacUpdate points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadUpdateMacArg(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvMACUpdate *    srvMacUpdate,
                                  fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvMacUpdate=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvMacUpdate);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvMacUpdate->macAddressLower = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_MAC_UPDATE,
                                                 MAC_ADDRESS_LOWER);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvMacUpdate->macAddressUpper = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_MAC_UPDATE,
                                                 MAC_ADDRESS_UPPER);

    srvMacUpdate->vlan = FM_GET_FIELD(rv,
                                      FM_MAILBOX_SRV_MAC_UPDATE,
                                      VLAN);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvMacUpdate->glort = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_MAC_UPDATE,
                                       GLORT);

    srvMacUpdate->flags = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_MAC_UPDATE,
                                       FLAGS);

    srvMacUpdate->action = FM_GET_FIELD(rv,
                                        FM_MAILBOX_SRV_MAC_UPDATE,
                                        ACTION);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadUpdateMacArg */




/*****************************************************************************/
/** ReadXcastModeArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for XCAST_MODES request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvXcastMode points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadXcastModeArg(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvXcastMode *    srvXcastMode,
                                  fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvXcastMode=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvXcastMode);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvXcastMode->glort = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_XCAST_MODE,
                                       GLORT);

    srvXcastMode->mode = FM_GET_FIELD(rv,
                                      FM_MAILBOX_SRV_XCAST_MODE,
                                      MODE);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadXcastModeArg */




/*****************************************************************************/
/** ReadConfigArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for Config request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvConfig points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadConfigArg(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_hostSrvConfig *       srvConfig,
                               fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvConfig=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvConfig);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvConfig->configurationAttributeConstant =
               FM_GET_FIELD(rv,
                            FM_MAILBOX_SRV_CONFIG,
                            CONFIGURATION_ATTRIBUTE_CONSTANT);

    srvConfig->value = FM_GET_FIELD(rv,
                                    FM_MAILBOX_SRV_CONFIG,
                                    VALUE);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadConfigArg */



/*****************************************************************************/
/** ReadMasterClkOffsetArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for MASTER_CLK_OFFSET request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      clkOffset points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadMasterClkOffsetArg(fm_int                     sw,
                                        fm_int                     pepNb,
                                        fm_mailboxControlHeader *  ctrlHdr,
                                        fm_hostSrvMasterClkOffset *clkOffset,
                                        fm_uint32 *                argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, clkOffset=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) clkOffset);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    clkOffset->offsetValueLower =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                           OFFSET_LOW);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    clkOffset->offsetValueUpper =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                           OFFSET_UPP);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadMasterClkOffsetArg */




/*****************************************************************************/
/** ReadInnOutMacArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for FILTER_INNER_OUTER_MAC request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      macFilter points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadInnOutMacArg(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvInnOutMac *    macFilter,
                                  fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, macFilter=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) macFilter);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->outerMacAddr =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           OUT_MAC_LOW);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->outerMacAddr |= (fm_macaddr)
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           OUT_MAC_UPP) << 32;

    macFilter->outerMacAddrMask =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           OUT_MAC_MASK_LOW);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->outerMacAddrMask |= (fm_macaddr)
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           OUT_MAC_MASK_UPP) << 16;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->innerMacAddr =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           INN_MAC_LOW);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->innerMacAddr |= (fm_macaddr)
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           INN_MAC_UPP) << 32;

    macFilter->innerMacAddrMask =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           INN_MAC_MASK_LOW);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->innerMacAddrMask |= (fm_macaddr)
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           INN_MAC_MASK_UPP) << 16;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->vni =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           VNI);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    macFilter->glort =
              FM_GET_FIELD(rv,
                           FM_MAILBOX_SRV_INN_OUT_MAC,
                           GLORT);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadInnOutMacArg */




/*****************************************************************************/
/** ReadCreateTableArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for CREATE_TABLE request and
 *                  fill structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvCreateTable points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadCreateTableArg(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_hostSrvCreateTable *  srvCreateTable,
                                    fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvCreateTable=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvCreateTable);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->matchTableIndex =
                    FM_GET_FIELD(rv,
                                 FM_MAILBOX_SRV_CREATE_TABLE,
                                 TABLE_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->tableType = FM_GET_FIELD(rv,
                                             FM_MAILBOX_SRV_CREATE_TABLE,
                                             TABLE_TYPE);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->numOfAct = FM_GET_FIELD(rv,
                                            FM_MAILBOX_SRV_CREATE_TABLE,
                                            NUM_OF_ACT);

    srvCreateTable->flags = FM_GET_FIELD(rv,
                                         FM_MAILBOX_SRV_CREATE_TABLE,
                                         FLAGS);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->numOfEntr = FM_GET_FIELD(rv,
                                             FM_MAILBOX_SRV_CREATE_TABLE,
                                             NUM_OF_ENTR);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Condition lower word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->condition = FM_GET_FIELD(rv,
                                             FM_MAILBOX_SRV_CREATE_TABLE,
                                             COND_BITMASK);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Condition upper word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->condition |= (fm_flowCondition)
                    (FM_GET_FIELD(rv,
                                  FM_MAILBOX_SRV_CREATE_TABLE,
                                  COND_BITMASK)) << FM_MBX_ENTRY_BIT_LENGTH;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Action lower word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->action = FM_GET_FIELD(rv,
                                          FM_MAILBOX_SRV_CREATE_TABLE,
                                          COND_BITMASK);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Action upper word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvCreateTable->action |= (fm_flowAction)
                    (FM_GET_FIELD(rv,
                                  FM_MAILBOX_SRV_CREATE_TABLE,
                                  COND_BITMASK)) << FM_MBX_ENTRY_BIT_LENGTH;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadCreateTableArg */




/*****************************************************************************/
/** ReadTableArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for DESTROY_TABLE request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvTable points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadTableArg(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_hostSrvTable *        srvTable,
                              fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvTable=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvTable);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvTable->matchTableIndex = FM_GET_FIELD(rv,
                                             FM_MAILBOX_SRV_TABLE,
                                             TABLE_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadTableArg */




/*****************************************************************************/
/** ReadTableRangeArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for GET_TABLES request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvTableRange points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadTableRangeArg(fm_int                    sw,
                                   fm_int                    pepNb,
                                   fm_mailboxControlHeader * ctrlHdr,
                                   fm_hostSrvFlowTableRange *srvTableRange,
                                   fm_uint32 *               argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvTable=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvTableRange);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvTableRange->firstFlowTableIndex =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_TABLE_RANGE,
                               FIRST_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvTableRange->lastFlowTableIndex =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_TABLE_RANGE,
                               LAST_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadTableRangeArg */




/*****************************************************************************/
/** ReadConditionParams
 * \ingroup intMailbox
 *
 * \desc            Read condition params for SET_RULES request and fill
 *                  structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowEntry points to caller-allocated storage where this
 *                  function should place read condition params.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadConditionParams(fm_int                   sw,
                                     fm_int                   pepNb,
                                     fm_mailboxControlHeader *ctrlHdr,
                                     fm_hostSrvFlowEntry *    srvFlowEntry,
                                     fm_uint32 *              argBytesRead)
{
    fm_flowValue *flowVal;
    fm_status     status;
    fm_uint32     rv;
    fm_uint32     glort;
    fm_int        i;

    status    = FM_OK;
    flowVal   = &srvFlowEntry->flowVal;
    glort     = 0;
    rv        = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowEntry=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowEntry);

    /* Read params according to condition set. */
    if (srvFlowEntry->condition & FM_FLOW_MATCH_SRC_MAC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->src = FM_GET_FIELD(rv,
                                    FM_MAILBOX_BITS,
                                    0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->src |= (fm_macaddr)
                        (FM_GET_FIELD(rv,
                                      FM_MAILBOX_BITS,
                                      0_15)) << FM_MBX_ENTRY_BIT_LENGTH;

        flowVal->srcMask = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->srcMask |= (fm_macaddr)
                            (FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          0_31)) << 16;

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_DST_MAC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dst = FM_GET_FIELD(rv,
                                    FM_MAILBOX_BITS,
                                    0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dst |= (fm_macaddr)
                        (FM_GET_FIELD(rv,
                                      FM_MAILBOX_BITS,
                                      0_15)) << FM_MBX_ENTRY_BIT_LENGTH;

        flowVal->dstMask = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dstMask |= (fm_macaddr)
                            (FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          0_31)) << 16;

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_ETHERTYPE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->ethType = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        flowVal->ethTypeMask = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VLAN)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vlanId = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_15);

        flowVal->vlanIdMask = FM_GET_FIELD(rv,
                                           FM_MAILBOX_BITS,
                                           16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VLAN_PRIORITY)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vlanPri = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        flowVal->vlanPriMask = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_SRC_IP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->srcIp.isIPv6 = FALSE;

        if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
        {
            flowVal->srcIp.isIPv6 = TRUE;
        }

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->srcIp.addr[0] = FM_GET_FIELD(rv,
                                              FM_MAILBOX_BITS,
                                              0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowVal->srcIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowVal->srcIp.addr[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);

                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }

        flowVal->srcIpMask.isIPv6 = flowVal->srcIp.isIPv6;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->srcIpMask.addr[0] = FM_GET_FIELD(rv,
                                                  FM_MAILBOX_BITS,
                                                  0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowVal->srcIpMask.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowVal->srcIpMask.addr[i] = FM_GET_FIELD(rv,
                                                          FM_MAILBOX_BITS,
                                                          0_31);

                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_DST_IP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dstIp.isIPv6 = FALSE;

        if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
        {
            flowVal->dstIp.isIPv6 = TRUE;
        }

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dstIp.addr[0] = FM_GET_FIELD(rv,
                                              FM_MAILBOX_BITS,
                                              0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowVal->dstIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowVal->dstIp.addr[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);
                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }

        flowVal->dstIpMask.isIPv6 = flowVal->dstIp.isIPv6;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->dstIpMask.addr[0] = FM_GET_FIELD(rv,
                                                  FM_MAILBOX_BITS,
                                                  0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowVal->dstIpMask.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowVal->dstIpMask.addr[i] = FM_GET_FIELD(rv,
                                                          FM_MAILBOX_BITS,
                                                          0_31);

                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_PROTOCOL)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->protocol = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15);

        flowVal->protocolMask = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_L4_SRC_PORT)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->L4SrcStart = FM_GET_FIELD(rv,
                                           FM_MAILBOX_BITS,
                                           0_15);

        flowVal->L4SrcMask = FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_L4_DST_PORT)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->L4DstStart = FM_GET_FIELD(rv,
                                           FM_MAILBOX_BITS,
                                           0_15);

        flowVal->L4DstMask = FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_INGRESS_PORT_SET)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->portSet = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_TOS)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->tos = FM_GET_FIELD(rv,
                                    FM_MAILBOX_BITS,
                                    0_15);

        flowVal->tosMask = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_FRAME_TYPE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->frameType = FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_SRC_PORT)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        glort = FM_GET_FIELD(rv,
                             FM_MAILBOX_BITS,
                             0_15);

        status = fmGetGlortLogicalPort(sw,
                                       glort,
                                       &flowVal->logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_TCP_FLAGS)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->tcpFlags = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15);

        flowVal->tcpFlagsMask = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_L4_DEEP_INSPECTION)
    {
        /* FM10000 devices, L4 DI supports up to 32 bytes.*/
        for (i = 0 ; i < 8 ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_MEMCPY_S(
                &flowVal->L4DeepInspection[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &rv,
                FM_MBX_ENTRY_BYTE_LENGTH);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }

        for (i = 0 ; i < 8 ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_MEMCPY_S(
                &flowVal->L4DeepInspectionMask[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &rv,
                FM_MBX_ENTRY_BYTE_LENGTH);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_L2_DEEP_INSPECTION)
    {
        /* FM10000 devices, L2 DI supports up to 32 bytes.*/
        for (i = 0 ; i < 8 ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_MEMCPY_S(
                &flowVal->L2DeepInspection[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &rv,
                FM_MBX_ENTRY_BYTE_LENGTH);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }

        for (i = 0 ; i < 8 ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_MEMCPY_S(
                &flowVal->L2DeepInspectionMask[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &rv,
                FM_MBX_ENTRY_BYTE_LENGTH);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_SWITCH_PRIORITY)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->switchPri = FM_GET_FIELD(rv,
                                          FM_MAILBOX_BITS,
                                          0_15);

        flowVal->switchPriMask = FM_GET_FIELD(rv,
                                              FM_MAILBOX_BITS,
                                              16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VLAN_TAG_TYPE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vlanTag = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VLAN2)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vlanId2 = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        flowVal->vlanId2Mask = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_PRIORITY2)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vlanPri2 = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15);

        flowVal->vlanPri2Mask = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             16_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_FRAG)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->fragType = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VNI)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vni = FM_GET_FIELD(rv,
                                    FM_MAILBOX_BITS,
                                    0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_VSI_TEP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowVal->vsiTep = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->condition & FM_FLOW_MATCH_LOGICAL_PORT)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        glort = FM_GET_FIELD(rv,
                             FM_MAILBOX_BITS,
                             0_15);

        status = fmGetGlortLogicalPort(sw,
                                       glort,
                                       &flowVal->logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadConditionParams */




/*****************************************************************************/
/** ReadCommonEncapParams
 * \ingroup intMailbox
 *
 * \desc            Read common encap action params for SET_RULES request and
 *                  fill structure fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowEntry points to caller-allocated storage where this
 *                  function should place read condition params.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadCommonEncapParams(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_hostSrvFlowEntry *    srvFlowEntry,
                                       fm_uint32 *              argBytesRead)
{
    fm_flowParam *flowParam;
    fm_status     status;
    fm_uint32     rv;
    fm_int        i;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowEntry=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowEntry);

    rv        = 0;
    flowParam = &srvFlowEntry->flowParam;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    flowParam->outerDip.isIPv6 = FALSE;

    if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
    {
        flowParam->outerDip.isIPv6 = TRUE;
    }

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    flowParam->outerDip.addr[0] = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_31);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    if (flowParam->outerDip.isIPv6)
    {
        for (i = 1 ; i <= 3 ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            flowParam->outerDip.addr[i] = FM_GET_FIELD(rv,
                                                       FM_MAILBOX_BITS,
                                                       0_31);
            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    flowParam->tunnelType = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;



    if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerNshLength = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_BITS,
                                                 0_7);

        flowParam->outerNshCritical = FALSE;

        if (FM_GET_FIELD(rv, FM_MAILBOX_BITS, 8_15))
        {
            flowParam->outerNshCritical = TRUE;
        }

        flowParam->outerNshMdType = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_BITS,
                                                 16_23);

        flowParam->outerNshSvcIndex = FM_GET_FIELD(rv,
                                                   FM_MAILBOX_BITS,
                                                   24_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerNshSvcPathId = FM_GET_FIELD(rv,
                                                    FM_MAILBOX_BITS,
                                                    0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        /* NGE DATA */
        for (i = 0 ; i < FM_TUNNEL_NSH_DATA_SIZE ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            flowParam->outerNshData[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerNshDataMask = FM_GET_FIELD(rv,
                                                   FM_MAILBOX_BITS,
                                                   0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadCommonEncapParams */




/*****************************************************************************/
/** ReadActionParams
 * \ingroup intMailbox
 *
 * \desc            Read action params for SET_RULES request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowEntry points to caller-allocated storage where this
 *                  function should place read condition params.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadActionParams(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvFlowEntry *    srvFlowEntry,
                                  fm_uint32 *              argBytesRead)
{
    fm_mailboxInfo *info;
    fm_flowParam * flowParam;
    fm_status      status;
    fm_uint16      glort;
    fm_uint32      rv;
    fm_uint64 *    flowGlortKey;
    fm_int         i;
    fm_int         logicalPort;
    fm_bool        encapParamsSet;
    fm_tree *      flowIdMapTree;
    fm_uintptr     internalTableIndex;
    fm_uintptr     internalFlowId;

    fm_mailboxResources *mailboxResourcesUsed;
    fm_mailboxFlowTable *mailboxFlowTable;

    info           = GET_MAILBOX_INFO(sw);
    status         = FM_OK;
    flowParam      = &srvFlowEntry->flowParam;
    encapParamsSet = FALSE;
    flowIdMapTree  = NULL;
    rv             = 0;
    glort          = 0;
    internalFlowId = 0;
    logicalPort    = -1;
    internalTableIndex   = 0;
    flowGlortKey         = NULL;
    mailboxResourcesUsed = NULL;
    mailboxFlowTable     = NULL;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowEntry=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowEntry);

    /* Read params according to action set. */
    if (srvFlowEntry->action & FM_FLOW_ACTION_FORWARD)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        glort = FM_GET_FIELD(rv,
                             FM_MAILBOX_BITS,
                             0_15);

        status = fmGetGlortLogicalPort(sw,
                                       glort,
                                       &flowParam->logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_PUSH_VLAN)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->vlan = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_VLAN)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->vlan = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_DMAC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->dmac = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->dmac |= (fm_macaddr)
                           (FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15)) << FM_MBX_ENTRY_BIT_LENGTH;

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_SMAC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->smac = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->smac |= (fm_macaddr)
                           (FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         0_15)) << FM_MBX_ENTRY_BIT_LENGTH;

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_DIP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->dip.isIPv6 = FALSE;

        if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
        {
            flowParam->dip.isIPv6 = TRUE;
        }

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->dip.addr[0] = FM_GET_FIELD(rv,
                                              FM_MAILBOX_BITS,
                                              0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowParam->dip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowParam->dip.addr[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);
                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_SIP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->sip.isIPv6 = FALSE;

        if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
        {
            flowParam->sip.isIPv6 = TRUE;
        }

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->sip.addr[0] = FM_GET_FIELD(rv,
                                              FM_MAILBOX_BITS,
                                              0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowParam->sip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowParam->sip.addr[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);
                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_L4DST)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->l4Dst = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_L4SRC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->l4Src = FM_GET_FIELD(rv,
                                        FM_MAILBOX_BITS,
                                        0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_TTL)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->ttl = FM_GET_FIELD(rv,
                                      FM_MAILBOX_BITS,
                                      0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_VNI)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerVni = FM_GET_FIELD(rv,
                                           FM_MAILBOX_BITS,
                                           0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadCommonEncapParams(sw,
                                       pepNb,
                                       ctrlHdr,
                                       srvFlowEntry,
                                       argBytesRead);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        encapParamsSet = TRUE;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_SIP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerSip.isIPv6 = FALSE;

        if ( FM_GET_FIELD(rv, FM_MAILBOX_BITS, 0_7) )
        {
            flowParam->outerSip.isIPv6 = TRUE;
        }

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerSip.addr[0] = FM_GET_FIELD(rv,
                                                   FM_MAILBOX_BITS,
                                                   0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (flowParam->outerSip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                status = ReadFromRequestQueue(sw,
                                              pepNb,
                                              &rv,
                                              ctrlHdr);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                flowParam->outerSip.addr[i] = FM_GET_FIELD(rv,
                                                           FM_MAILBOX_BITS,
                                                           0_31);
                *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }
        }

        if (!encapParamsSet)
        {
            status = ReadCommonEncapParams(sw,
                                           pepNb,
                                           ctrlHdr,
                                           srvFlowEntry,
                                           argBytesRead);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            encapParamsSet = TRUE;
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_TTL)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerTtl = FM_GET_FIELD(rv,
                                           FM_MAILBOX_BITS,
                                           0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            status = ReadCommonEncapParams(sw,
                                           pepNb,
                                           ctrlHdr,
                                           srvFlowEntry,
                                           argBytesRead);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            encapParamsSet = TRUE;
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_L4DST)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerL4Dst = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            status = ReadCommonEncapParams(sw,
                                           pepNb,
                                           ctrlHdr,
                                           srvFlowEntry,
                                           argBytesRead);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            encapParamsSet = TRUE;
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_L4SRC)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerL4Src = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            status = ReadCommonEncapParams(sw,
                                           pepNb,
                                           ctrlHdr,
                                           srvFlowEntry,
                                           argBytesRead);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            encapParamsSet = TRUE;
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ENCAP_NGE)
    {
        for (i = 0 ; i < FM_TUNNEL_NGE_DATA_SIZE ; i++)
        {
            status = ReadFromRequestQueue(sw,
                                          pepNb,
                                          &rv,
                                          ctrlHdr);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            flowParam->outerNgeData[i] = FM_GET_FIELD(rv,
                                                      FM_MAILBOX_BITS,
                                                      0_31);

            *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
        }

        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->outerNgeMask = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            status = ReadCommonEncapParams(sw,
                                           pepNb,
                                           ctrlHdr,
                                           srvFlowEntry,
                                           argBytesRead);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            encapParamsSet = TRUE;
        }
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->tableIndex = FM_GET_FIELD(rv,
                                             FM_MAILBOX_BITS,
                                             0_15);

        flowParam->flowId = FM_GET_FIELD(rv,
                                         FM_MAILBOX_BITS,
                                         16_31);

        /* Flow table resources are tracked per logical port
           associated with PF glort. */
        status = fmGetPcieLogicalPort(sw,
                                      pepNb,
                                      FM_PCIE_PORT_PF,
                                      0,
                                      &logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status)

        /* Table index and flow ID are match values, we need to remap
         * them to internal values.
         */
        status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                            logicalPort,
                            (void **) &mailboxResourcesUsed);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Check if match table index is valid. */
        status = fmTreeFind(&mailboxResourcesUsed->mailboxFlowTableResource,
                            flowParam->tableIndex,
                            (void **) &mailboxFlowTable);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        /* Check if match flow ID is valid. */
        status = fmTreeFind(&mailboxFlowTable->matchFlowIdMap,
                            flowParam->flowId,
                            (void **) &flowGlortKey);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->tableIndex = mailboxFlowTable->tableIndex;
        flowParam->flowId     = GET_FLOW_FROM_FLOW_GLORT_KEY(*flowGlortKey);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_BALANCE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->balanceGroup = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_ROUTE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->ecmpGroup = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_VLAN_PRIORITY)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->vlanPriority = FM_GET_FIELD(rv,
                                               FM_MAILBOX_BITS,
                                               0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_SWITCH_PRIORITY)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->switchPriority = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_BITS,
                                                 0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_DSCP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->dscp = FM_GET_FIELD(rv,
                                       FM_MAILBOX_BITS,
                                       0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_LOAD_BALANCE)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->lbgNumber = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            0_31);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_SET_FLOOD_DEST)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        glort = FM_GET_FIELD(rv,
                             FM_MAILBOX_BITS,
                             0_15);

        status = fmGetGlortLogicalPort(sw,
                                       glort,
                                       &flowParam->logicalPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (srvFlowEntry->action & FM_FLOW_ACTION_MIRROR_GRP)
    {
        status = ReadFromRequestQueue(sw,
                                      pepNb,
                                      &rv,
                                      ctrlHdr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        flowParam->mirrorGrp = FM_GET_FIELD(rv,
                                            FM_MAILBOX_BITS,
                                            0_15);

        *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadActionParams */




/*****************************************************************************/
/** ReadFlowEntryArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for SET_RULES request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowEntry points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadFlowEntryArg(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvFlowEntry *    srvFlowEntry,
                                  fm_uint32 *              argBytesRead)
{
    fm_status        status;
    fm_uint32        rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowEntry=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowEntry);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->matchTableIndex =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_ENTRY,
                               TABLE_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->matchFlowId =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_ENTRY,
                               FLOW_ID);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->priority =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_ENTRY,
                               PRIORITY);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->glort =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_ENTRY,
                               GLORT);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Lower condition word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->condition = FM_GET_FIELD(rv,
                                           FM_MAILBOX_SRV_FLOW_ENTRY,
                                           COND_BITMASK);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Upper condition word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->condition |= (fm_flowCondition)
                 (FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_ENTRY,
                               COND_BITMASK)) << FM_MBX_ENTRY_BIT_LENGTH;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Lower action word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->action = FM_GET_FIELD(rv,
                                        FM_MAILBOX_SRV_FLOW_ENTRY,
                                        ACT_BITMASK);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Upper action word. */
    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowEntry->action |= (fm_flowAction)
                  (FM_GET_FIELD(rv,
                                FM_MAILBOX_SRV_FLOW_ENTRY,
                                ACT_BITMASK)) << FM_MBX_ENTRY_BIT_LENGTH;

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    /* Read condition params. */
    status = ReadConditionParams(sw,
                                 pepNb,
                                 ctrlHdr,
                                 srvFlowEntry,
                                 argBytesRead);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Read action params. */
    status = ReadActionParams(sw,
                              pepNb,
                              ctrlHdr,
                              srvFlowEntry,
                              argBytesRead);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadFlowEntryArg */




/*****************************************************************************/
/** ReadFlowHandleArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for DEL_RULES request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowHandle points to caller-supplied storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadFlowHandleArg(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_hostSrvFlowHandle *   srvFlowHandle,
                                   fm_uint32 *              argBytesRead)
{
    fm_status        status;
    fm_uint32        rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowHandle=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowHandle);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowHandle->matchTableIndex =
                   FM_GET_FIELD(rv,
                                FM_MAILBOX_SRV_FLOW_HANDLE,
                                TABLE_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowHandle->matchFlowId =
                  FM_GET_FIELD(rv,
                               FM_MAILBOX_SRV_FLOW_HANDLE,
                               FLOW_ID);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadFlowHandleArg */




/*****************************************************************************/
/** ReadFlowRangeArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for GET_RULES request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvFlowRange points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadFlowRangeArg(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_hostSrvFlowRange *    srvFlowRange,
                                  fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvFlowRange=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvFlowRange);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowRange->matchTableIndex = FM_GET_FIELD(rv,
                                                 FM_MAILBOX_SRV_FLOW_RANGE,
                                                 TABLE_INDEX);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowRange->firstFlowId = FM_GET_FIELD(rv,
                                             FM_MAILBOX_SRV_FLOW_RANGE,
                                             FIRST_FLOW_ID);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvFlowRange->lastFlowId = FM_GET_FIELD(rv,
                                            FM_MAILBOX_SRV_FLOW_RANGE,
                                            LAST_FLOW_ID);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadFlowRangeArg */




/*****************************************************************************/
/** ReadNoOfVfsArg
 * \ingroup intMailbox
 *
 * \desc            Read argument for SET_NO_OF_VFS request and fill structure
 *                  fields with proper values.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       ctrlHdr points to mailbox control header structure.
 *
 * \param[out]      srvNoOfVfs points to caller-allocated storage where this
 *                  function should place read values.
 *
 * \param[in,out]   argBytesRead points to number of read bytes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VALUE if there are not enough queue
 *                  elements used to store proper message arguments.
 *
 *****************************************************************************/
static fm_status ReadNoOfVfsArg(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *ctrlHdr,
                                fm_hostSrvNoOfVfs *      srvNoOfVfs,
                                fm_uint32 *              argBytesRead)
{
    fm_status status;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, srvNoOfVfs=%p\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 (void *) srvNoOfVfs);

    rv = 0;

    status = ReadFromRequestQueue(sw,
                                  pepNb,
                                  &rv,
                                  ctrlHdr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    srvNoOfVfs->glort = FM_GET_FIELD(rv,
                                     FM_MAILBOX_SRV_NO_OF_VFS,
                                     GLORT);

    srvNoOfVfs->noOfVfs = FM_GET_FIELD(rv,
                                       FM_MAILBOX_SRV_NO_OF_VFS,
                                       NO_OF_VFS);

    *argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;


    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end ReadNoOfVfsArg */




/*****************************************************************************/
/** CalculateCondActSize
 * \ingroup intMailbox
 *
 * \desc            Returns number of bytes needed to write flow in the mailbox
 *                  register.
 *
 * \param[in]       condition is a bit mask of matching conditions.
 *
 * \param[in]       action is a bit mask of actions.
 *
 * \param[in]       condVal points to a structure containing the values and
 *                  masks to match.
 *
 * \param[in]       flowParam points to a structure containing values used
 *                  for some actions.
 *
 * \return          Number of bytes.
 *
 *****************************************************************************/
static fm_uint32 CalculateCondActSize(fm_flowCondition condition,
                                      fm_flowAction    action,
                                      fm_flowValue *   condVal,
                                      fm_flowParam *   flowParam)
{
    fm_uint32 argBytesRead;
    fm_bool encapParamsSet;

    argBytesRead   = 0;
    encapParamsSet = FALSE;

    if (condition & FM_FLOW_MATCH_SRC_MAC)
    {
        argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_DST_MAC)
    {
        argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_ETHERTYPE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VLAN)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VLAN_PRIORITY)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_SRC_IP)
    {
        if (condVal->srcIp.isIPv6)
        {
            argBytesRead += 9 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (condition & FM_FLOW_MATCH_DST_IP)
    {
        if (condVal->dstIp.isIPv6)
        {
            argBytesRead += 9 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (condition & FM_FLOW_MATCH_PROTOCOL)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_L4_SRC_PORT)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_L4_DST_PORT)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_INGRESS_PORT_SET)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_TOS)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_FRAME_TYPE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_SRC_PORT)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_TCP_FLAGS)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_L4_DEEP_INSPECTION)
    {
        argBytesRead += 16 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_L2_DEEP_INSPECTION)
    {
        argBytesRead += 16 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_SWITCH_PRIORITY)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VLAN_TAG_TYPE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VLAN2)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_PRIORITY2)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_FRAG)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VNI)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_VSI_TEP)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (condition & FM_FLOW_MATCH_LOGICAL_PORT)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_FORWARD)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_COUNT)
    {
        argBytesRead += 4 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_PUSH_VLAN)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_VLAN)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_DMAC)
    {
        argBytesRead += 2 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_SMAC)
    {
        argBytesRead += 2 * FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_DIP)
    {
        if (flowParam->dip.isIPv6)
        {
            argBytesRead += 5 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 2 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (action & FM_FLOW_ACTION_SET_SIP)
    {
        if (flowParam->sip.isIPv6)
        {
            argBytesRead += 5 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 2 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
    }

    if (action & FM_FLOW_ACTION_SET_L4DST)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_L4SRC)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_TTL)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_ENCAP_VNI)
    {
        argBytesRead += 1 * FM_MBX_ENTRY_BYTE_LENGTH;
        if (flowParam->outerDip.isIPv6)
        {
            argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
        }

        if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
        {
            argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
        }

        encapParamsSet = TRUE;
    }

    if (action & FM_FLOW_ACTION_ENCAP_SIP)
    {
        if (flowParam->outerSip.isIPv6)
        {
            argBytesRead += 5 * FM_MBX_ENTRY_BYTE_LENGTH;
        }
        else
        {
            argBytesRead += 2 * FM_MBX_ENTRY_BYTE_LENGTH;
        }

        if (!encapParamsSet)
        {
            if (flowParam->outerDip.isIPv6)
            {
                argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
            }
            else
            {
                argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
            {
                argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_TTL)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            if (flowParam->outerDip.isIPv6)
            {
                argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
            }
            else
            {
                argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
            {
                argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_L4DST)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            if (flowParam->outerDip.isIPv6)
            {
                argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
            }
            else
            {
                argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
            {
                argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_L4SRC)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            if (flowParam->outerDip.isIPv6)
            {
                argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
            }
            else
            {
                argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
            {
                argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_NGE)
    {
        argBytesRead += 17 * FM_MBX_ENTRY_BYTE_LENGTH;

        if (!encapParamsSet)
        {
            if (flowParam->outerDip.isIPv6)
            {
                argBytesRead += 6 * FM_MBX_ENTRY_BYTE_LENGTH;
            }
            else
            {
                argBytesRead += 3 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
            {
                argBytesRead += 15 * FM_MBX_ENTRY_BYTE_LENGTH;
            }

            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_BALANCE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_ROUTE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_VLAN_PRIORITY)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_SWITCH_PRIORITY)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_DSCP)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_LOAD_BALANCE)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_SET_FLOOD_DEST)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    if (action & FM_FLOW_ACTION_MIRROR_GRP)
    {
        argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
    }

    return argBytesRead;

}   /* End CalculateCondActSize */




/*****************************************************************************/
/** FillFlowConditionData
 * \ingroup intMailbox
 *
 * \desc            Fills mailbox response data with flow condition data.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       condition is a bit mask of matching conditions.
 *
 * \param[in]       flowVal points to a structure containing the values and
 *                  masks to match.
 *
 * \param[in,out]   dataIndex is a current data array index.
 *
 * \param[out]      data points to a user allocated storage where flow params
 *                  should be written.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FillFlowConditionData(fm_int           sw,
                                  fm_flowCondition condition,
                                  fm_flowValue *   flowVal,
                                  fm_int *         dataIndex,
                                  fm_uint32 *      data)
{
    fm_status status;
    fm_int    i;
    fm_uint32 glort;

    status = FM_OK;
    glort  = 0;

    /* Read params according to condition set. */
    if (condition & FM_FLOW_MATCH_SRC_MAC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->src);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     (flowVal->src >> FM_MBX_ENTRY_BIT_LENGTH));

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->srcMask);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     (flowVal->srcMask >> 16));

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_DST_MAC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->dst);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     (flowVal->dst >> FM_MBX_ENTRY_BIT_LENGTH));

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->dstMask);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     (flowVal->dstMask >> 16));

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_ETHERTYPE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->ethType);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->ethTypeMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VLAN)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vlanId);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->vlanIdMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VLAN_PRIORITY)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vlanPri);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->vlanPriMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_SRC_IP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowVal->srcIp.isIPv6);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->srcIp.addr[0]);

        (*dataIndex)++;

        if (flowVal->srcIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowVal->srcIp.addr[i]);

                (*dataIndex)++;
            }
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->srcIpMask.addr[0]);

        (*dataIndex)++;

        if (flowVal->srcIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowVal->srcIpMask.addr[i]);

                (*dataIndex)++;
            }
        }
    }

    if (condition & FM_FLOW_MATCH_DST_IP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowVal->dstIp.isIPv6);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->dstIp.addr[0]);

        (*dataIndex)++;

        if (flowVal->dstIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowVal->dstIp.addr[i]);

                (*dataIndex)++;
            }
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->dstIpMask.addr[0]);

        (*dataIndex)++;

        if (flowVal->dstIp.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowVal->dstIpMask.addr[i]);

                (*dataIndex)++;
            }
        }
    }

    if (condition & FM_FLOW_MATCH_PROTOCOL)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->protocol);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->protocolMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_L4_SRC_PORT)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->L4SrcStart);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->L4SrcMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_L4_DST_PORT)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->L4DstStart);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->L4DstMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_INGRESS_PORT_SET)
    {

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->portSet);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_TOS)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->tos);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->tosMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_FRAME_TYPE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->frameType);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_SRC_PORT)
    {
        status = fmGetLogicalPortGlort(sw,
                                       flowVal->logicalPort,
                                       &glort);
        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Getting glort for logical port %d failed.\n",
                         flowVal->logicalPort);
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     glort);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_TCP_FLAGS)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->tcpFlags);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->tcpFlagsMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_L4_DEEP_INSPECTION)
    {
        /* FM10000 devices, L4 DI supports up to 32 bytes.*/
        for (i = 0 ; i < 8 ; i++)
        {
            FM_MEMCPY_S(
                &data[*dataIndex],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &flowVal->L4DeepInspection[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH);
            (*dataIndex)++;
        }

        for (i = 0 ; i < 8 ; i++)
        {
            FM_MEMCPY_S(
                &data[*dataIndex],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &flowVal->L4DeepInspectionMask[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH);
            (*dataIndex)++;
        }
    }

    if (condition & FM_FLOW_MATCH_L2_DEEP_INSPECTION)
    {
        /* FM10000 devices, L2 DI supports up to 32 bytes.*/
        for (i = 0 ; i < 8 ; i++)
        {
            FM_MEMCPY_S(
                &data[*dataIndex],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &flowVal->L2DeepInspection[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH);
            (*dataIndex)++;
        }

        for (i = 0 ; i < 8 ; i++)
        {
            FM_MEMCPY_S(
                &data[*dataIndex],
                FM_MBX_ENTRY_BYTE_LENGTH,
                &flowVal->L2DeepInspectionMask[i * FM_MBX_ENTRY_BYTE_LENGTH],
                FM_MBX_ENTRY_BYTE_LENGTH);
            (*dataIndex)++;
        }
    }

    if (condition & FM_FLOW_MATCH_SWITCH_PRIORITY)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->switchPri);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->switchPriMask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VLAN_TAG_TYPE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vlanTag);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VLAN2)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vlanId2);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->vlanId2Mask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_PRIORITY2)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vlanPri2);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowVal->vlanPri2Mask);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_FRAG)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->fragType);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VNI)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowVal->vni);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_VSI_TEP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowVal->vsiTep);

        (*dataIndex)++;
    }

    if (condition & FM_FLOW_MATCH_LOGICAL_PORT)
    {
        status = fmGetLogicalPortGlort(sw,
                                       flowVal->logicalPort,
                                       &glort);
        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Getting glort for logical port %d failed.\n",
                         flowVal->logicalPort);
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     glort);

        (*dataIndex)++;
    }

}   /* end FillFlowConditionData */




/*****************************************************************************/
/** FillFlowCommnonEncapData
 * \ingroup intMailbox
 *
 * \desc            Fills mailbox response data with flow common encap action
 *                  params.
 *
 * \param[in]       flowParam points to a structure containing values used
 *                  for some actions.
 *
 * \param[in,out]   dataIndex is a current data array index.
 *
 * \param[out]      data points to a user allocated storage where flow params
 *                  should be written.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FillFlowCommnonEncapData(fm_flowParam *flowParam,
                                     fm_int *      dataIndex,
                                     fm_uint32 *   data)
{
    fm_int i;

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_BITS,
                 0_7,
                 flowParam->outerDip.isIPv6);

    (*dataIndex)++;

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_BITS,
                 0_31,
                 flowParam->outerDip.addr[0]);

    (*dataIndex)++;

    if (flowParam->outerDip.isIPv6)
    {
        for (i = 1 ; i <= 3 ; i++)
        {
            FM_SET_FIELD(data[*dataIndex],
                         FM_MAILBOX_BITS,
                         0_31,
                         flowParam->outerDip.addr[i]);
            (*dataIndex)++;
        }
    }

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_BITS,
                 0_15,
                 flowParam->tunnelType);

    (*dataIndex)++;

    if (flowParam->tunnelType == FM_TUNNEL_TYPE_GPE_NSH)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowParam->outerNshLength);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     8_15,
                     flowParam->outerNshCritical);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_23,
                     flowParam->outerNshMdType);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     24_31,
                     flowParam->outerNshSvcIndex);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->outerNshSvcPathId);

        (*dataIndex)++;

        /* NGE DATA */
        for (i = 0 ; i < FM_TUNNEL_NSH_DATA_SIZE ; i++)
        {
            FM_SET_FIELD(data[*dataIndex],
                         FM_MAILBOX_BITS,
                         0_31,
                         flowParam->outerNshData[i]);

            (*dataIndex)++;
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->outerNshDataMask);

        (*dataIndex)++;
    }

}   /* end FillFlowCommnonEncapData */




/*****************************************************************************/
/** FillFlowActionData
 * \ingroup intMailbox
 *
 * \desc            Fills mailbox response data with flow action data.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       tableIndex is the table instance to which the flow belongs.
 *
 * \param[in]       flowId is the flow instance.
 *
 * \param[in]       action is a bit mask of actions.
 *
 * \param[in]       flowParam points to a structure containing values used
 *                  for some actions.
 *
 * \param[in,out]   dataIndex is a current data array index.
 *
 * \param[out]      data points to a user allocated storage where flow params
 *                  should be written.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FillFlowActionData(fm_int        sw,
                               fm_int        pepNb,
                               fm_int        tableIndex,
                               fm_int        flowId,
                               fm_flowAction action,
                               fm_flowParam *flowParam,
                               fm_int *      dataIndex,
                               fm_uint32 *   data)
{
    fm_mailboxInfo *info;
    fm_status       status;
    fm_int          i;
    fm_int          logicalPort;
    fm_bool         encapParamsSet;
    fm_tree *       flowIdMapTree;
    fm_uintptr      internalFlowId;
    fm_uintptr      treeValue;
    fm_treeIterator treeIter;
    fm_uint32       glort;
    fm_uint64       nextKey;
    fm_flowCounters counter;
    fm_mailboxResources *mailboxResourcesUsed;
    fm_mailboxFlowTable *mailboxFlowTable;


    info           = GET_MAILBOX_INFO(sw);
    status         = FM_OK;
    encapParamsSet = FALSE;
    flowIdMapTree  = NULL;
    internalFlowId = 0;
    glort          = 0;
    logicalPort    = -1;
    mailboxFlowTable     = NULL;
    mailboxResourcesUsed = NULL;

    /* Read params according to action set. */
    if (action & FM_FLOW_ACTION_FORWARD)
    {
         status = fmGetLogicalPortGlort(sw,
                                        flowParam->logicalPort,
                                        &glort);
         if (status != FM_OK)
         {
             FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                          "Getting glort for logical port %d failed.\n",
                          flowParam->logicalPort);
         }

         FM_SET_FIELD(data[*dataIndex],
                      FM_MAILBOX_BITS,
                      0_15,
                      glort);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_COUNT)
    {
        FM_CLEAR(counter);
        status = fmGetFlowCount(sw,
                                tableIndex,
                                flowId,
                                &counter);
        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Failed to read counters for tableIndex=%d flowId=%d.\n",
                          tableIndex,
                          flowId);
        }

        FM_SET_FIELD(data[*dataIndex],
                          FM_MAILBOX_BITS,
                          0_31,
                          counter.cntPkts);
        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                          FM_MAILBOX_BITS,
                          0_31,
                          (counter.cntPkts) >> 32);
        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                          FM_MAILBOX_BITS,
                          0_31,
                          counter.cntOctets);
        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                          FM_MAILBOX_BITS,
                          0_31,
                          (counter.cntOctets) >> 32);
        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_PUSH_VLAN)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->vlan);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_VLAN)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->vlan);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_DMAC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->dmac);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     (flowParam->dmac >> FM_MBX_ENTRY_BIT_LENGTH));

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_SMAC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->smac);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     (flowParam->smac >> FM_MBX_ENTRY_BIT_LENGTH));

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_DIP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowParam->dip.isIPv6);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->dip.addr[0]);

        (*dataIndex)++;

        if (flowParam->dip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowParam->dip.addr[i]);
                (*dataIndex)++;
            }
        }
    }

    if (action & FM_FLOW_ACTION_SET_SIP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowParam->sip.isIPv6);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->sip.addr[0]);

        (*dataIndex)++;

        if (flowParam->sip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowParam->sip.addr[i]);
                (*dataIndex)++;
            }
        }
    }

    if (action & FM_FLOW_ACTION_SET_L4DST)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->l4Dst);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_L4SRC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->l4Src);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_TTL)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->ttl);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_ENCAP_VNI)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->outerVni);

        (*dataIndex)++;

        FillFlowCommnonEncapData(flowParam,
                                 dataIndex,
                                 data);
        encapParamsSet = TRUE;
    }

    if (action & FM_FLOW_ACTION_ENCAP_SIP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_7,
                     flowParam->outerSip.isIPv6);

        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->outerSip.addr[0]);

        (*dataIndex)++;

        if (flowParam->outerSip.isIPv6)
        {
            for (i = 1 ; i <= 3 ; i++)
            {
                FM_SET_FIELD(data[*dataIndex],
                             FM_MAILBOX_BITS,
                             0_31,
                             flowParam->outerSip.addr[i]);
                (*dataIndex)++;
            }
        }

        if (!encapParamsSet)
        {
            FillFlowCommnonEncapData(flowParam,
                                     dataIndex,
                                     data);
            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_TTL)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->outerTtl);

        (*dataIndex)++;

        if (!encapParamsSet)
        {
            FillFlowCommnonEncapData(flowParam,
                                     dataIndex,
                                     data);
            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_L4DST)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->outerL4Dst);

        (*dataIndex)++;

        if (!encapParamsSet)
        {
            FillFlowCommnonEncapData(flowParam,
                                     dataIndex,
                                     data);
            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_L4SRC)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->outerL4Src);

        (*dataIndex)++;

        if (!encapParamsSet)
        {
            FillFlowCommnonEncapData(flowParam,
                                     dataIndex,
                                     data);
            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_ENCAP_NGE)
    {
        for (i = 0 ; i < FM_TUNNEL_NGE_DATA_SIZE ; i++)
        {
            FM_SET_FIELD(data[*dataIndex],
                         FM_MAILBOX_BITS,
                         0_31,
                         flowParam->outerNgeData[i]);
            (*dataIndex)++;
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->outerNgeMask);

        (*dataIndex)++;

        if (!encapParamsSet)
        {
            FillFlowCommnonEncapData(flowParam,
                                     dataIndex,
                                     data);
            encapParamsSet = TRUE;
        }
    }

    if (action & FM_FLOW_ACTION_REDIRECT_TUNNEL)
    {
        /* Flow table resources are tracked per logical port
           associated with PF glort. */
        status = fmGetPcieLogicalPort(sw,
                                      pepNb,
                                      FM_PCIE_PORT_PF,
                                      0,
                                      &logicalPort);
        if (status == FM_OK)
        {
            /* Table index and flow ID are internal values, we need to remap
             * them to match values.
             */

            status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                                logicalPort,
                                (void **) &mailboxResourcesUsed);

            if (status != FM_OK)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Logical port %d is not tracked in mailbox resource tree.\n",
                              logicalPort);
            }
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Getting logical port for PF failed.\n");
        }

        if (status == FM_OK)
        {
            /* Find mailbox flow resource structure for given flow table
             * index. */
            fmTreeIterInit(&treeIter,
                           &mailboxResourcesUsed->mailboxFlowTableResource);
            status = fmTreeIterNext(&treeIter,
                                    &nextKey,
                                    (void **) &mailboxFlowTable);
            while (status == FM_OK)
            {
                if (mailboxFlowTable->tableIndex == flowParam->tableIndex)
                {
                    /* Set match flow table index to be returned. */
                    flowParam->tableIndex = (fm_int) nextKey;
                    break;
                }

                status = fmTreeIterNext(&treeIter,
                                        &nextKey,
                                        (void **) &mailboxFlowTable);
            }

            if (status == FM_OK)
            {
                /* Check if match flow ID is valid. */
                status = fmTreeFind(&mailboxFlowTable->internalFlowIdMap,
                                    flowParam->flowId,
                                    (void **) &treeValue);

                if (status == FM_OK)
                {
                    /* Set match flow ID to be returned. */
                    flowParam->flowId = (fm_int) treeValue;
                }
                else
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                                 "Flow ID %d is not tracked in mailbox resource tree.\n",
                                 flowParam->flowId);
                }
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                             "Flow table index %d is not tracked in mailbox resource tree.\n",
                             flowParam->tableIndex);
            }
        }

        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Mapping internal values to match ones failed, returning default\n");
            flowParam->tableIndex = -1;
            flowParam->flowId     = -1;
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->tableIndex);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     16_31,
                     flowParam->flowId);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_BALANCE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->balanceGroup);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_ROUTE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->ecmpGroup);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_VLAN_PRIORITY)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->vlanPriority);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_SWITCH_PRIORITY)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->switchPriority);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_DSCP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->dscp);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_LOAD_BALANCE)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_31,
                     flowParam->lbgNumber);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_SET_FLOOD_DEST)
    {
        status = fmGetLogicalPortGlort(sw,
                                       flowParam->logicalPort,
                                       &glort);
        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MAILBOX,
                         "Getting glort for logical port %d failed.\n",
                         flowParam->logicalPort);
        }

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     glort);

        (*dataIndex)++;
    }

    if (action & FM_FLOW_ACTION_MIRROR_GRP)
    {
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_BITS,
                     0_15,
                     flowParam->mirrorGrp);

        (*dataIndex)++;
    }

}   /* end FillFlowActionData */




/*****************************************************************************/
/** BuildErrorResp
 * \ingroup intMailbox
 *
 * \desc            Build response message data from fm_hostSrvErr structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildErrorResp(fm_hostSrvErr *response,
                                fm_int *       numberOfWords,
                                fm_uint32 **   data)
{
    fm_status status;
    fm_int    dataElements;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_ERR_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                FM_HOST_SRV_ERR_TYPE_SIZE,
                0,
                FM_HOST_SRV_ERR_TYPE_SIZE);

    dataElements = 0;
    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 STATUS_CODE,
                 response->statusCode);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 MAC_TABLE_ROWS_USED,
                 response->macTableRowsUsed);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 MAC_TABLE_ROWS_AVAILABLE,
                 response->macTableRowsAvailable);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 NEXTHOP_TABLE_ROWS_USED,
                 response->nexthopTableRowsUsed);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 NEXTHOP_TABLE_ROWS_AVAILABLE,
                 response->nexthopTableRowsAvailable);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 FFU_RULES_USED,
                 response->ffuRulesUsed);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_ERR,
                 FFU_RULES_AVAILABLE,
                 response->ffuRulesAvailable);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildErrorResp */




/*****************************************************************************/
/** BuildMapLportResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from fm_hostSrvLportMap
 *                  structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildMapLportResp(fm_hostSrvLportMap *response,
                                   fm_int *            numberOfWords,
                                   fm_uint32 **        data)
{
    fm_status status;
    fm_int    dataElements;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_LPORT_MAP_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                FM_HOST_SRV_LPORT_MAP_TYPE_SIZE,
                0,
                FM_HOST_SRV_LPORT_MAP_TYPE_SIZE);

    dataElements = 0;
    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_LPORT_MAP,
                 GLORT_VALUE,
                 response->glortValue);

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_LPORT_MAP,
                 GLORT_MASK,
                 response->glortMask);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildMapLportResp */




/*****************************************************************************/
/** BuildUpdatePvidResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from fm_hostSrvUpdatePvid
 *                  structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildUpdatePvidResp(fm_hostSrvUpdatePvid *response,
                                     fm_int *              numberOfWords,
                                     fm_uint32 **          data)
{
    fm_status status;
    fm_int    dataElements;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    };

    FM_MEMSET_S(*data,
                FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE,
                0,
                FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE);

    dataElements = 0;
    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_UPDATE_PVID,
                 GLORT_VALUE,
                 response->glort);

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_UPDATE_PVID,
                 PVID_VALUE,
                 response->pvid);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildUpdatePvidResp */




/*****************************************************************************/
/** BuildPacketTimestampResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from fm_hostSrvPacketTimestamp
 *                  structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildPacketTimestampResp(
                                      fm_hostSrvPacketTimestamp *response,
                                      fm_int *                   numberOfWords,
                                      fm_uint32 **               data)
{
    fm_status status;
    fm_int    dataElements;
    fm_uint32 rv;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE,
                0,
                FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE);

    dataElements = 0;
    rv = response->egressTimestamp & 0xFFFFFFFF;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 EGR_TIME_LOW,
                 rv);
    dataElements++;

    rv = response->egressTimestamp >> FM_MBX_ENTRY_BIT_LENGTH;
    rv &= 0xFFFFFFFF;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 EGR_TIME_UP,
                 rv);
    dataElements++;

    rv = response->ingressTimestamp & 0xFFFFFFFF;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 INGR_TIME_LOW,
                 rv);
    dataElements++;

    rv = response->ingressTimestamp >> FM_MBX_ENTRY_BIT_LENGTH;
    rv &= 0xFFFFFFFF;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 INGR_TIME_UP,
                 rv);
    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 SGLORT,
                 response->sglort);

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_PACKET_TIMESTAMP,
                 DGLORT,
                 response->dglort);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildPacketTimestampResp */




/*****************************************************************************/
/** BuildTimestampModeResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from
 *                  fm_hostSrvTimestampModeResp structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildTimestampModeResp(
                                    fm_hostSrvTimestampModeResp *response,
                                    fm_int *                     numberOfWords,
                                    fm_uint32 **                 data)
{
    fm_status status;
    fm_int    dataElements;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE,
                0,
                FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE);

    dataElements = 0;
    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP,
                 GLORT,
                 response->glort);

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP,
                 MODE_ENABLED,
                 response->modeEnabled);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildTimestampModeResp */




/*****************************************************************************/
/** BuildMasterClkOffsetResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from
 *                  fm_hostSrvTimestampModeResp structure.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildMasterClkOffsetResp(
                                    fm_hostSrvMasterClkOffset *response,
                                    fm_int *                   numberOfWords,
                                    fm_uint32 **               data)
{
    fm_status status;
    fm_int    dataElements;

    status = FM_OK;

    *data = fmAlloc(FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE);
    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE,
                0,
                FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE);

    dataElements = 0;
    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                 OFFSET_LOW,
                 response->offsetValueLower);

    dataElements++;

    FM_SET_FIELD((*data)[dataElements],
                 FM_MAILBOX_SRV_MASTER_CLK_OFFSET,
                 OFFSET_UPP,
                 response->offsetValueUpper);
    dataElements++;

    *numberOfWords = dataElements;

    return status;

}   /* end BuildMasterClkOffsetResp */




/*****************************************************************************/
/** FillSourceTablesData
 * \ingroup intMailbox
 *
 * \desc            Fills mailbox response data with source tables.
 *
 * \param[in,out]   dataIndex is a current data array index.
 *
 * \param[out]      data points to caller-supplied storage where source table
 *                  params should be written.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void FillSourceTablesData(fm_int *         dataIndex,
                                 fm_uint32 *      data)
{
    fm_flowCondition condition;
    fm_flowAction    action;
    fm_uint32        rv;
    fm_int           i;

    /* For source tables, table index and table type are the same.
     * Starting with TCAM table. */
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 TABLE_INDEX,
                 FM_MBX_TCAM_TABLE);
    (*dataIndex)++;

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 TABLE_TYPE,
                 FM_MBX_TCAM_TABLE);
    (*dataIndex)++;

    /* Maximum number of actions supported by TCAM table. */
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 NUM_OF_ACT,
                 FM10000_TCAM_TABLE_MAX_ACTIONS);

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 FLAGS,
                 0);
    (*dataIndex)++;

    /* For source tables, we return 0 for max number of entries and for
     * number of entries left in a table. This is done to prevent any
     * misunderstanding of returned values - capacity of source table depends
     *  on the conditions to be used.
     */
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 MAX_NUM_OF_ENTR,
                 0);
    (*dataIndex)++;

    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 NUM_OF_EMPTY_ENTR,
                 0);
    (*dataIndex)++;

    condition = FM10000_FLOW_SUPPORTED_TCAM_CONDITIONS;

    rv = (fm_int)(condition & 0xFFFFFFFF);
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 COND_BITMASK,
                 rv);
    (*dataIndex)++;

    rv = (fm_int)((condition >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 COND_BITMASK,
                 rv);
    (*dataIndex)++;

    action = FM10000_FLOW_SUPPORTED_TCAM_ACTIONS;

    rv = (fm_int)(action & 0xFFFFFFFF);
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 COND_BITMASK,
                 rv);
    (*dataIndex)++;

    rv = (fm_int)((action >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
    FM_SET_FIELD(data[*dataIndex],
                 FM_MAILBOX_SRV_GET_TABLES,
                 COND_BITMASK,
                 rv);
    (*dataIndex)++;

    /* TE tables. */
    for (i = 0 ; i < FM10000_TE_DGLORT_MAP_ENTRIES_1 ; i++)
    {
        if (i == 0)
        {
            rv = FM_MBX_TE_A_TABLE;
        }
        else
        {
            rv = FM_MBX_TE_B_TABLE;
        }

        /* For source tables, table index and table type are the same. */
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     TABLE_INDEX,
                     rv);
        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     TABLE_TYPE,
                     rv);
        (*dataIndex)++;

        /* Only one action is supported by TE tables. */
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     NUM_OF_ACT,
                     FM10000_TE_TABLE_MAX_ACTIONS);

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     FLAGS,
                     0);
        (*dataIndex)++;

        /* For source tables, we return 0 for max number of entries and for
         * number of entries left in a table. This is done to prevent any
         * misunderstanding of returned values - capacity of source table
         * depends on the conditions to be used.
         */
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     MAX_NUM_OF_ENTR,
                     0);
        (*dataIndex)++;

        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     NUM_OF_EMPTY_ENTR,
                     0);
        (*dataIndex)++;

        condition = FM10000_FLOW_SUPPORTED_TE_CONDITIONS;

        rv = (fm_int)(condition & 0xFFFFFFFF);
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     COND_BITMASK,
                     rv);
        (*dataIndex)++;

        rv = (fm_int)((condition >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     COND_BITMASK,
                     rv);
        (*dataIndex)++;

        action = FM10000_FLOW_SUPPORTED_TE_ACTIONS;

        rv = (fm_int)(action & 0xFFFFFFFF);
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     COND_BITMASK,
                     rv);
        (*dataIndex)++;

        rv = (fm_int)((action >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
        FM_SET_FIELD(data[*dataIndex],
                     FM_MAILBOX_SRV_GET_TABLES,
                     COND_BITMASK,
                     rv);
        (*dataIndex)++;
    }

}   /* End FillSourceTablesData */




/*****************************************************************************/
/** BuildGetTablesResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from
 *                  tree containing flow table indexes created on HNI request.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildGetTablesResp(fm_int                    sw,
                                    fm_int                    pepNb,
                                    fm_hostSrvFlowTableRange *response,
                                    fm_int *                  numberOfWords,
                                    fm_uint32 **              data)
{
    fm_switch *          switchPtr;
    fm_mailboxInfo *     info;
    fm_status            status;
    fm_int               dataElements;
    fm_int               tunnelEngine;
    fm_int               managementPep;
    fm_int               logicalPort;
    fm_uint32            rv;
    fm_uint32            allocSize;
    fm_treeIterator      indexIter;
    fm_uint64            nextKey;
    fm_flowCondition     condition;
    fm_flowTableType     flowTableType;
    fm_mailboxTableType  mailboxTableType;
    fm_mailboxFlowTable *mailboxFlowTable;
    fm_mailboxResources *resourcesUsed;

    switchPtr    = GET_SWITCH_PTR(sw);
    info         = GET_MAILBOX_INFO(sw);
    status       = FM_OK;
    condition    = 0;
    dataElements = 0;
    rv           = 0;
    mailboxTableType = FM_MBX_TCAM_TABLE;

    /* Flow table resources are tracked per logical port
       associated with PF glort. */
    status = fmGetPcieLogicalPort(sw,
                                  pepNb,
                                  FM_PCIE_PORT_PF,
                                  0,
                                  &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        logicalPort,
                        (void **) &resourcesUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->MapLogicalPortToPep,
                       sw,
                       switchPtr->cpuPort,
                       &managementPep);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Source tables should be returned only for management PEP. */
    if (pepNb == managementPep)
    {
        /* Alloc size includes source tables. */
        allocSize = FM_HOST_SRV_GET_TABLES_TYPE_SIZE *
                    (FM10000_MAILBOX_SOURCE_TABLES +
                     fmTreeSize(&resourcesUsed->mailboxFlowTableResource));
    }
    else
    {
        allocSize = FM_HOST_SRV_GET_TABLES_TYPE_SIZE *
                    fmTreeSize(&resourcesUsed->mailboxFlowTableResource);

        /* Return error if no tables allocated for non-management PEP. */
        if (allocSize == 0)
        {
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
        }
    }

    if (allocSize > 0)
    {
        *data = fmAlloc(allocSize);
    }
    else
    {
        status = FM_OK;
        goto ABORT;
    }

    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                allocSize,
                0,
                allocSize);

    if (pepNb == managementPep)
    {
        /* Add source tables (TCAM AND TEs). */
        FillSourceTablesData(&dataElements,
                             *data);
    }

    /* If first and last flow table indexes are set to 0 we should return all
     * tables.
     */
    if ( (response->firstFlowTableIndex == 0) &&
         (response->lastFlowTableIndex == 0) )
    {
        response->lastFlowTableIndex = ~0;
    }

    fmTreeIterInit(&indexIter, &resourcesUsed->mailboxFlowTableResource);
    status = fmTreeIterNext(&indexIter,
                            &nextKey,
                            (void **) &mailboxFlowTable);
    while (status == FM_OK)
    {
        if ( (nextKey >= response->firstFlowTableIndex) &&
             (nextKey <= response->lastFlowTableIndex) )
        {
            /* We need to map internal flow table to match one. */
            status = fmGetFlowTableType(sw,
                                        mailboxFlowTable->tableIndex,
                                        &flowTableType);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            if (flowTableType == FM_FLOW_TCAM_TABLE)
            {
                mailboxTableType = FM_MBX_TCAM_TABLE;
            }
            else if (flowTableType == FM_FLOW_TE_TABLE)
            {
                status = fmGetFlowAttribute(sw,
                                            mailboxFlowTable->tableIndex,
                                            FM_FLOW_TABLE_TUNNEL_ENGINE,
                                            &tunnelEngine);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                if (tunnelEngine == 0)
                {
                    mailboxTableType = FM_MBX_TE_A_TABLE;
                }
                else
                {
                    mailboxTableType = FM_MBX_TE_B_TABLE;
                }
            }

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         TABLE_INDEX,
                         nextKey);
            dataElements++;

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         TABLE_TYPE,
                         mailboxTableType);
            dataElements++;

            status = fmGetFlowAttribute(sw,
                                        mailboxFlowTable->tableIndex,
                                        FM_FLOW_TABLE_MAX_ACTIONS,
                                        &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         NUM_OF_ACT,
                         rv);

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         FLAGS,
                         mailboxFlowTable->flags);
            dataElements++;

            status = fmGetFlowAttribute(sw,
                                        mailboxFlowTable->tableIndex,
                                        FM_FLOW_TABLE_MAX_ENTRIES,
                                        &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         MAX_NUM_OF_ENTR,
                         rv);
            dataElements++;

            status = fmGetFlowAttribute(sw,
                                        mailboxFlowTable->tableIndex,
                                        FM_FLOW_TABLE_EMPTY_ENTRIES,
                                        &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         NUM_OF_EMPTY_ENTR,
                         rv);
            dataElements++;

            status = fmGetFlowAttribute(sw,
                                        mailboxFlowTable->tableIndex,
                                        FM_FLOW_TABLE_CONDITION,
                                        &condition);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            rv = (fm_uint32)(condition & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)((condition >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)(mailboxFlowTable->action & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)((mailboxFlowTable->action >> FM_MBX_ENTRY_BIT_LENGTH)
                  & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;
        }

        status = fmTreeIterNext(&indexIter,
                                &nextKey,
                                (void **) &mailboxFlowTable);
    }

    status = FM_OK;

ABORT:

    *numberOfWords = dataElements;

    if ((status != FM_OK) && (*data != NULL))
    {
        fmFree(*data);
        *numberOfWords = 0;
    }

    return status;

}   /* end BuildGetTablesResp */




/*****************************************************************************/
/** BuildGetRulesResp
 * \ingroup intMailbox
 *
 * \desc            Build a response message data from fm_mailboxFlowTable
 *                  structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepNb is the PEP number.
 *
 * \param[in]       response points to a structure sent as a response.
 *
 * \param[out]      numberOfWords points to a number of 32-bit words of
 *                  allocated response data.
 *
 * \param[out]      data is the pointer to the pointer to newly allocated data
 *                  to be returned as a response.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status BuildGetRulesResp(fm_int               sw,
                                   fm_int               pepNb,
                                   fm_hostSrvFlowRange *response,
                                   fm_int *             numberOfWords,
                                   fm_uint32 **         data)
{
    fm_mailboxInfo *     info;
    fm_status            status;
    fm_int               dataElements;
    fm_int               flowId;
    fm_int               priority;
    fm_int               precedence;
    fm_int               logicalPort;
    fm_uint32            rv;
    fm_uint32            glort;
    fm_uint32            allocSize;
    fm_uint64            nextKey;
    fm_uint64 *          flowGlortKey;
    fm_flowCondition     condition;
    fm_flowAction        action;
    fm_flowValue         flowVal;
    fm_flowParam         flowParam;
    fm_treeIterator      treeIter;
    fm_mailboxResources *mailboxResourcesUsed;
    fm_mailboxFlowTable *mailboxFlowTable;

    info         = GET_MAILBOX_INFO(sw);
    status       = FM_OK;
    condition    = 0;
    action       = 0;
    dataElements = 0;
    rv           = 0;
    allocSize    = 0;
    flowGlortKey = NULL;

    /* Flow table resources are tracked per logical port
       associated with PF glort. */
    status = fmGetPcieLogicalPort(sw,
                                  pepNb,
                                  FM_PCIE_PORT_PF,
                                  0,
                                  &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fmTreeFind(&info->mailboxResourcesPerVirtualPort,
                        logicalPort,
                        (void **) &mailboxResourcesUsed);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* Check if match table index is valid. */
    status = fmTreeFind(&mailboxResourcesUsed->mailboxFlowTableResource,
                        response->matchTableIndex,
                        (void **) &mailboxFlowTable);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    /* If first and last flow indexes are set to 0 we should return all flows.*/
    if ( (response->firstFlowId == 0) && (response->lastFlowId == 0) )
    {
        response->lastFlowId = ~0;
    }

    /* Calculate message size. */
    fmTreeIterInit(&treeIter, &mailboxFlowTable->matchFlowIdMap);
    status = fmTreeIterNext(&treeIter, &nextKey, (void **) &flowGlortKey);

    while (status == FM_OK)
    {
        if ( (nextKey >= response->firstFlowId) &&
             (nextKey <= response->lastFlowId) )
        {
            flowId = GET_FLOW_FROM_FLOW_GLORT_KEY(*flowGlortKey);

            status = fmGetFlow(sw,
                               mailboxFlowTable->tableIndex,
                               flowId,
                               &condition,
                               &flowVal,
                               &action,
                               &flowParam,
                               &priority,
                               &precedence);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            /* Size of constant fields. */
            allocSize += FM_HOST_SRV_FLOW_ENTRY_TYPE_SIZE;

            /* Size of all condition & action params. */
            allocSize += CalculateCondActSize(condition,
                                              action,
                                              &flowVal,
                                              &flowParam);
        }

        status = fmTreeIterNext(&treeIter, &nextKey, (void **) &flowGlortKey);
    }

    if (allocSize > 0)
    {
        *data = fmAlloc(allocSize);
    }
    else
    {
        status = FM_OK;
        goto ABORT;
    }

    if (*data == NULL)
    {
        status = FM_ERR_NO_MEM;
        return status;
    }

    FM_MEMSET_S(*data,
                allocSize,
                0,
                allocSize);

    /* Iterate over all flows. */
    fmTreeIterInit(&treeIter, &mailboxFlowTable->matchFlowIdMap);
    status = fmTreeIterNext(&treeIter, &nextKey, (void **) &flowGlortKey);

    while (status == FM_OK)
    {
        if ( (nextKey >= response->firstFlowId) &&
             (nextKey <= response->lastFlowId) )
        {
            flowId = GET_FLOW_FROM_FLOW_GLORT_KEY(*flowGlortKey);

            status = fmGetFlow(sw,
                               mailboxFlowTable->tableIndex,
                               flowId,
                               &condition,
                               &flowVal,
                               &action,
                               &flowParam,
                               &priority,
                               &precedence);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_FLOW_ENTRY,
                         TABLE_INDEX,
                         response->matchTableIndex);
            dataElements++;

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_FLOW_ENTRY,
                         FLOW_ID,
                         (fm_int) nextKey);
            dataElements++;

            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_FLOW_ENTRY,
                         PRIORITY,
                         priority);
            dataElements++;

            glort = GET_GLORT_FROM_FLOW_GLORT_KEY(*flowGlortKey);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_FLOW_ENTRY,
                         GLORT,
                         glort);
            dataElements++;

            rv = (fm_uint32)(condition & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)((condition >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)(action & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            rv = (fm_uint32)((action >> FM_MBX_ENTRY_BIT_LENGTH) & 0xFFFFFFFF);
            FM_SET_FIELD((*data)[dataElements],
                         FM_MAILBOX_SRV_GET_TABLES,
                         COND_BITMASK,
                         rv);
            dataElements++;

            FillFlowConditionData(sw,
                                  condition,
                                  &flowVal,
                                  &dataElements,
                                  *data);

            FillFlowActionData(sw,
                               pepNb,
                               mailboxFlowTable->tableIndex,
                               flowId,
                               action,
                               &flowParam,
                               &dataElements,
                               *data);
        }

        status = fmTreeIterNext(&treeIter, &nextKey, (void **) &flowGlortKey);
    }

    status = FM_OK;

ABORT:

    *numberOfWords = dataElements;

    if ((status != FM_OK) && (*data != NULL))
    {
        fmFree(*data);
        *numberOfWords = 0;
    }

    return status;

}   /* end BuildGetRulesResp */


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
 * \return          FM_ERR_NO_MEM if no memory available.
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
    fm_status  status;
    fm_uint32 *dataToWrite;
    fm_int     dataEntries;

    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d, ctrlHdr=%p, msgTypeId=0x%x, argType=0x%x\n",
                 sw,
                 pepNb,
                 (void *) ctrlHdr,
                 msgTypeId,
                 argType);

    status      = FM_OK;
    dataEntries = 0;
    dataToWrite = NULL;

    /* Prepare response data to be written to queue.
     * No headers are formed here. */
    switch (argType)
    {
        case FM_HOST_SRV_RETURN_ERR_TYPE:

            status = BuildErrorResp((fm_hostSrvErr *) message,
                                    &dataEntries,
                                    &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_MAP_LPORT_TYPE:

            status = BuildMapLportResp((fm_hostSrvLportMap *) message,
                                       &dataEntries,
                                       &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_UPDATE_PVID_TYPE:

            status = BuildUpdatePvidResp((fm_hostSrvUpdatePvid *) message,
                                         &dataEntries,
                                         &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_PACKET_TIMESTAMP_TYPE:

            status = BuildPacketTimestampResp(
                                (fm_hostSrvPacketTimestamp *) message,
                                &dataEntries,
                                &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE:

            status = BuildTimestampModeResp(
                                (fm_hostSrvTimestampModeResp *) message,
                                &dataEntries,
                                &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE:

            status = BuildMasterClkOffsetResp(
                                (fm_hostSrvMasterClkOffset *) message,
                                &dataEntries,
                                &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_TABLE_LIST_TYPE:
            status = BuildGetTablesResp(sw,
                                        pepNb,
                                        (fm_hostSrvFlowTableRange *) message,
                                        &dataEntries,
                                        &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        case FM_HOST_SRV_FLOW_ENTRY_TYPE:
            status = BuildGetRulesResp(sw,
                                       pepNb,
                                       (fm_hostSrvFlowRange *) message,
                                       &dataEntries,
                                       &dataToWrite);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

            break;
    }

    status = WriteResponseData(sw,
                               pepNb,
                               ctrlHdr,
                               msgTypeId,
                               argType,
                               dataEntries,
                               dataToWrite);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:

    if (dataToWrite)
    {
        fmFree(dataToWrite);
    }

    if (status != FM_OK)
    {
        /* In case of error, try to send error response (for all other types).*/
        if (argType != FM_HOST_SRV_RETURN_ERR_TYPE)
        {
            fmSendHostSrvErrResponse(sw,
                                     pepNb,
                                     status,
                                     ctrlHdr,
                                     msgTypeId,
                                     FM_HOST_SRV_RETURN_ERR_TYPE);
        }
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
    nbOfBytesUsed *= FM_MBX_ENTRY_BYTE_LENGTH;

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
 * \param[in]       argumentLength is the minimal request message argument
 *                  legth.
 *
 * \param[out]      message points to caller-allocated storage where this
 *                  function should place read values.
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
    fm_mailboxMessageHeader argHdr;
    fm_bool                 argHdrRead;

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
    argHdrRead       = FALSE;

    /* Read arguments */
    while (bytesRead < pfTrHdr->length)
    {
        status = ReadMessageHeader(sw,
                                   pepNb,
                                   ctrlHdr,
                                   &argHdr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

        bytesRead += FM_MBX_ENTRY_BYTE_LENGTH;

        /* If we meet unexpected argument types, try to ignore them */
        if ( (argHdrRead == FALSE) &&
             (argHdr.type == argumentType) &&
             (argHdr.length >= argumentLength) )
        {
            switch (argumentType)
            {
                case FM_HOST_SRV_CREATE_DELETE_LPORT_TYPE:

                    status = ReadCreateDelLportArg(sw,
                                                   pepNb,
                                                   ctrlHdr,
                                                   (fm_hostSrvPort *) message,
                                                   &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_UPDATE_MAC_TYPE:

                    status = ReadUpdateMacArg(sw,
                                              pepNb,
                                              ctrlHdr,
                                              (fm_hostSrvMACUpdate *) message,
                                              &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_SET_XCAST_MODE_TYPE:

                    status = ReadXcastModeArg(sw,
                                              pepNb,
                                              ctrlHdr,
                                              (fm_hostSrvXcastMode *) message,
                                              &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_REQUEST_CONFIG_TYPE:

                    status = ReadConfigArg(sw,
                                           pepNb,
                                           ctrlHdr,
                                           (fm_hostSrvConfig *) message,
                                           &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE:

                    status = ReadMasterClkOffsetArg(
                                 sw,
                                 pepNb,
                                 ctrlHdr,
                                 (fm_hostSrvMasterClkOffset *) message,
                                 &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_INN_OUT_MAC_TYPE:

                    status = ReadInnOutMacArg(sw,
                                              pepNb,
                                              ctrlHdr,
                                              (fm_hostSrvInnOutMac *) message,
                                              &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_CREATE_TABLE_TYPE:

                    status = ReadCreateTableArg(
                                 sw,
                                 pepNb,
                                 ctrlHdr,
                                 (fm_hostSrvCreateTable *) message,
                                 &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_TABLE_TYPE:

                    status = ReadTableArg(sw,
                                          pepNb,
                                          ctrlHdr,
                                          (fm_hostSrvTable *) message,
                                          &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_FLOW_TABLE_RANGE_TYPE:

                    status = ReadTableRangeArg(
                                 sw,
                                 pepNb,
                                 ctrlHdr,
                                 (fm_hostSrvFlowTableRange *) message,
                                 &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_FLOW_ENTRY_TYPE:

                    status = ReadFlowEntryArg(sw,
                                              pepNb,
                                              ctrlHdr,
                                              (fm_hostSrvFlowEntry *) message,
                                              &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_FLOW_HANDLE_TYPE:

                    status = ReadFlowHandleArg(sw,
                                               pepNb,
                                               ctrlHdr,
                                               (fm_hostSrvFlowHandle *) message,
                                               &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_FLOW_RANGE_TYPE:

                    status = ReadFlowRangeArg(sw,
                                              pepNb,
                                              ctrlHdr,
                                              (fm_hostSrvFlowRange *) message,
                                              &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                case FM_HOST_SRV_NO_OF_VFS_TYPE:

                    status = ReadNoOfVfsArg(sw,
                                            pepNb,
                                            ctrlHdr,
                                            (fm_hostSrvNoOfVfs *) message,
                                            &argBytesRead);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    argHdrRead = TRUE;
                    break;

                default:
                    /* Unknown argument */
                    status = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

                    break;
            }

            /* If there is any more data from the argument - ignore it */
            while (argBytesRead < argHdr.length)
            {
                INCREMENT_QUEUE_INDEX(ctrlHdr->reqHead);
                argBytesRead += FM_MBX_ENTRY_BYTE_LENGTH;
            }

            bytesRead += argBytesRead;
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
 *                  members.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if a pointer is NULL.
 * \return          FM_FAIL otherwise.
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

    *mailboxGlortBase    = FM10000_MAILBOX_GLORT_BASE;
    *mailboxGlortsPerPep = GET_PROPERTY()->hniGlortsPerPep;
    *mailboxGlortCount   = *mailboxGlortsPerPep *
                           FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS();
    *mailboxGlortMask    = 0xFFFF & ~(*mailboxGlortsPerPep - 1);

    /* The number of GloRTs must not exceed the range and the glortsPerPep value
     * must be a power of two */
    if ( ( *mailboxGlortsPerPep & (*mailboxGlortsPerPep - 1) ) ||
         ( numberOfSWAGMembers * (*mailboxGlortCount) >
           (FM10000_MAILBOX_GLORT_MAX - FM10000_MAILBOX_GLORT_BASE + 1) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_MAILBOX,
                      "Invalid attribute value (\"%s\"=%d). Because of the "
                      "number of SWAG switches (%d) and PEP ports (%d), it "
                      "must be less or equal to %d (and a power of two).\n",
                      FM_AAK_API_HNI_GLORTS_PER_PEP,
                      *mailboxGlortsPerPep,
                      numberOfSWAGMembers,
                      FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS(),
                      (FM10000_MAILBOX_GLORT_MAX - FM10000_MAILBOX_GLORT_BASE + 1) /
                      (numberOfSWAGMembers * FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS()) );

        FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, FM_FAIL);
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
    fm_status  status2;
    fm_status  maskStatus;
    fm_switch *switchPtr;
    fm_int     swToExecute;
    fm_uint64  regAddr;
    fm_uint32  rv;
    fm_uint32  retries;
    fm_bool    processRequest;
    fm_bool    processGlobalAck;
    fm_bool    pepResetState;
    
    FM_LOG_ENTRY(FM_LOG_CAT_MAILBOX,
                 "sw=%d, pepNb=%d\n",
                 sw,
                 pepNb);

    status           = FM_OK;
    processRequest   = FALSE;
    processGlobalAck = FALSE;
    swToExecute      = sw;
    pepResetState    = 1; /* Pep in active state*/
    rv               = 0;
    
#if FM_SUPPORT_SWAG
    swToExecute = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);
#endif

    /* Take the Write lock instead of the Read one to protect the Logical Port
     * Structure from parallel access at the application level. At this point,
     * only the Read Switch lock was taken by the interrupt handler 
     * thread. It would be preferable here to have the lock promoted from read 
     * to write lock, but this can cause a deadlock if other locks have been 
     * taken prior to this. */ 
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

    status2 = fm10000GetPepResetState(sw,
                                     pepNb,
                                     &pepResetState);
    FM_LOG_ASSERT(FM_LOG_CAT_MAILBOX, status2==FM_OK, "Unexpected\n");

    if (status2 == FM_OK)
    {
        retries = 0;
        while (pepResetState == 0 && retries < PEP_RESET_RECOVERY_RETRIES)
        {
            /* The PEP could be in DataPathReset, retry every 1ms */
            FM_LOG_DEBUG2(FM_LOG_CAT_MAILBOX,
                          "PEP %d is in reset while trying to re-enable mailbox "
                          "interrupts, retyring in 1ms\n",
                          pepNb);

            fmDelay(0, PEP_RESET_RECOVERY_DELAY);

            status2 = fm10000GetPepResetState(sw,
                                              pepNb,
                                              &pepResetState);
            if (status2 != FM_OK)
            {
                FM_LOG_ASSERT(FM_LOG_CAT_MAILBOX, status2==FM_OK, "Unexpected\n");
                break;
            }
            
            retries++;
        }

        if (status2 == FM_OK)
        {
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
            else
            {
                FM_LOG_DEBUG2(FM_LOG_CAT_MAILBOX,
                              "Failed to re-enable interrupts on PEP %d in %d retries\n",
                              pepNb,
                              retries);
            }
        }
    }

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

    /* Other threads can take the write lock in this gap. */
    PROTECT_SWITCH(sw);

    /* Another thread may have taken the switch lock when we swapped the switch
     * write lock for the read lock. Verify there has not been a switch status
     * change to avoid fatal conditions. */
    if (switchPtr->state != FM_SWITCH_STATE_UP)
    {
        status = FM_ERR_SWITCH_NOT_UP;
    }

    if (status == FM_OK && status2 != FM_OK)
    {
        status = status2;
    }

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
 *                                                                      \lb\lb
 *                  Called via the AllocVirtualLogicalPort function pointer.
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
 *                                                                      \lb\lb
 *                  Called via the FreeVirtualLogicalPort function pointer.
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

    if (GET_PROPERTY()->addPepsToFlooding)
    {
        /* Do not update flooding port masks. */
        status = FM_OK;
        goto ABORT;
    }

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
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

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
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask, GET_PORT_INDEX(sw, pepPort))) == 0)
                {
                    status = fmEnablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&mcastFloodDestMask,
                                          GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &mcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&ucastFloodDestMask,
                                          GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &ucastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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
                     (FM_PORTMASK_GET_BIT(&bcastFloodDestMask,
                                          GET_PORT_INDEX(sw, pepPort))) == 1)
                {
                    status = fmDisablePortInPortMask(sw, &bcastFloodDestMask, pepPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);
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

    info->maxMacEntriesToAddPerPep =
        GET_PROPERTY()->hniMacEntriesPerPep;

    info->maxInnerOuterMacEntriesToAddPerPep =
        GET_PROPERTY()->hniInnOutEntriesPerPep;

    info->maxMacEntriesToAddPerPort =
        GET_PROPERTY()->hniMacEntriesPerPort;

    info->maxInnerOuterMacEntriesToAddPerPort =
        GET_PROPERTY()->hniInnOutEntriesPerPort;

    info->maxFlowEntriesToAddPerVf =
        GET_PROPERTY()->hniFlowEntriesPerVf;



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
    info->maxFlowEntriesToAddPerVf            = -1;

    FM_LOG_EXIT(FM_LOG_CAT_MAILBOX, status);

}   /* end fm10000MailboxUnconfigureCounters */

