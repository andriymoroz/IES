/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_event_mac_maint.c
 * Creation Date:   September 6, 2013
 * Description:     FM10000-specific MAC Table maintenance code.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

typedef struct _fm_fifoEntry
{
    /* MAC address from the frame. */
    fm_macaddr      macAddress;

    /* Ingress VLAN identifier. */
    fm_uint16       vlanID;

    /* Physical source port from which the MAC address arrived. */
    fm_int          srcPort;

    /* Source GLORT of the frame that caused the notification. If the frame 
     * did not have an FTAG, the srcGlort is redundant information, as the 
     * glort could be derived from the port. If the frame has an FTAG, the 
     * srcGlort is from FTAG.SGLORT (unless 0) and represents the original 
     * source glort associated with the port that received the packet on a 
     * multi-switch system. */
    fm_uint16       srcGlort;

    /* MA_TABLE index for the MAC address. Valid only for port moves. */
    fm_uint16       entryIndex;

    /* Source of the MAC address. Will be FM_MAC_SOURCE_TCN_LEARNED for
     * a NewSource event, or FM_MAC_SOURCE_TCN_MOVED for a MacMoved
     * event. */
    fm_macSource    macSource;

    /* Logical port number of the source port. */
    fm_int          logicalPort;

} fm_fifoEntry;


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
/** DecodeFifoEntry
 * \ingroup intMacMaint
 *
 * \desc            Converts a TCN FIFO entry to an internal MAC address
 *                  entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       tcnEntry points to an array of words read from the
 *                  TCN FIFO entry.
 * 
 * \param[out]      fifoEntry points to a structure in which the decoded
 *                  TCN FIFO entry is to be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFifoEntry(fm_int         sw,
                                 fm_uint32 *    tcnEntry,
                                 fm_fifoEntry * fifoEntry)
{
    fm_status           status;
    fm10000_switch *    switchExt;
    fm_bool             entryType;
    fm_uint16           lagCanonicalGlort;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    FM_CLEAR(*fifoEntry);

    fifoEntry->macAddress = FM_ARRAY_GET_FIELD64(tcnEntry,
                                                 FM10000_MA_TCN_DEQUEUE,
                                                 MACAddress);

    fifoEntry->vlanID = FM_ARRAY_GET_FIELD(tcnEntry,
                                           FM10000_MA_TCN_DEQUEUE,
                                           VID);

    fifoEntry->srcPort = FM_ARRAY_GET_FIELD(tcnEntry,
                                            FM10000_MA_TCN_DEQUEUE,
                                            Port);

    fifoEntry->srcGlort = FM_ARRAY_GET_FIELD(tcnEntry,
                                             FM10000_MA_TCN_DEQUEUE,
                                             srcGlort);

    fifoEntry->entryIndex = FM_ARRAY_GET_FIELD(tcnEntry,
                                               FM10000_MA_TCN_DEQUEUE,
                                               Index);

    entryType = FM_ARRAY_GET_BIT(tcnEntry,
                                 FM10000_MA_TCN_DEQUEUE,
                                 EntryType);

    fifoEntry->macSource =
        (entryType == 0) ? 
        FM_MAC_SOURCE_TCN_LEARNED : 
        FM_MAC_SOURCE_TCN_MOVED;

    /***************************************************
     * Convert source glort to logical port.
     **************************************************/
    if (fmIsInLAGGlortRange(sw, fifoEntry->srcGlort))
    {
        status = fm10000GetCanonicalLagGlort(sw, 
                                             fifoEntry->srcGlort,
                                             &lagCanonicalGlort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);

        status = fmGetGlortLogicalPort(sw,
                                       lagCanonicalGlort,
                                       &fifoEntry->logicalPort);
    }
    else
    {
        status = fmGetGlortLogicalPort(sw,
                                       fifoEntry->srcGlort,
                                       &fifoEntry->logicalPort);

        if (status == FM_ERR_INVALID_PORT)
        {
            /* The address may be associated with a glort on a remote switch
             * that is not yet known to us. See if we can create a remote
             * logical port for the glort and use it if we are successful. */
            if (switchExt->createRemoteLogicalPorts)
            {
                status = fmCreateStackLogicalPort(sw,
                                                  fifoEntry->srcGlort,
                                                  &fifoEntry->logicalPort);
                if (status != FM_OK)
                {
                    status = FM_ERR_INVALID_PORT;
                }
            }
        }
    }

ABORT:
    if (status != FM_OK)
    {
        fifoEntry->logicalPort = -1;
    }

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d mac=%012llx vid=%u physPort=%d srcGlort=%u "
                 "source=%s index=%u logicalPort=%d\n",
                 sw,
                 fifoEntry->macAddress,
                 fifoEntry->vlanID,
                 fifoEntry->srcPort,
                 fifoEntry->srcGlort,
                 fmMacSourceToText(fifoEntry->macSource),
                 fifoEntry->entryIndex,
                 fifoEntry->logicalPort);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_EVENT_MAC_MAINT, status);

}   /* end DecodeFifoEntry */




/*****************************************************************************/
/** EnableFHTailTCN
 * \ingroup intMacMaint
 *
 * \desc            Enables FH_TAIL TCN interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EnableFHTailTCN(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   status;
    fm_uint32   mask;

    switchPtr = GET_SWITCH_PTR(sw);

    mask = 0;
    FM_SET_BIT(mask, FM10000_FH_TAIL_IM, TCN, 1);

    status = switchPtr->MaskUINT32(sw, FM10000_FH_TAIL_IM(), mask, FALSE);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "Error enabling FH_TAIL_IM interrupts: %s\n",
                     fmErrorMsg(status));
    }

    return status;

}   /* end EnableFHTailTCN */




/*****************************************************************************/
/** EnableMaTcnMask
 * \ingroup intMacMaint
 *
 * \desc            Enables MA_TCN interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       mask specifies the interrupts to be enabled.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EnableMaTcnMask(fm_int sw, fm_uint32 mask)
{
    fm_switch * switchPtr;
    fm_status   status;

    switchPtr = GET_SWITCH_PTR(sw);

    status = switchPtr->MaskUINT32(sw, FM10000_MA_TCN_IM(), mask, FALSE);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "Error enabling MA_TCN_IM interrupts: %s\n",
                     fmErrorMsg(status));
    }

    return status;

}   /* end EnableMaTcnMask */




/*****************************************************************************/
/** EnableTcnOverflow
 * \ingroup intMacMaint
 *
 * \desc            Enables TCN_Overflow interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EnableTcnOverflow(fm_int sw)
{
    fm_status   status;
    fm_uint32   mask;

    mask = 0;
    FM_SET_BIT(mask, FM10000_MA_TCN_IM, TCN_Overflow, 1);

    status = EnableMaTcnMask(sw, mask);

    return status;

}   /* end EnableTcnOverflow */




/*****************************************************************************/
/** EnableTcnPendingEvents
 * \ingroup intMacMaint
 *
 * \desc            Enables TCN PendingEvents interrupts.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status EnableTcnPendingEvents(fm_int sw)
{
    fm_status   status;
    fm_uint32   mask;

    mask = 0;
    FM_SET_BIT(mask, FM10000_MA_TCN_IM, PendingEvents, 1);

    status = EnableMaTcnMask(sw, mask);

    return status;

}   /* end EnableTcnPendingEvents */




/*****************************************************************************/
/** GetTcnFifoBacklog
 * \ingroup intMacMaint
 *
 * \desc            Returns the number of pending events in the TCN FIFO.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      backlog points to a location to receive the number of
 *                  pending events in the MA TCN FIFO.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetTcnFifoBacklog(fm_int sw, fm_uint32 * backlog)
{
    fm_switch * switchPtr;
    fm_status   status;
    fm_uint32   tcnHead;
    fm_uint32   tcnTail;
    fm_uint32   head;
    fm_uint32   tail;

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    status = switchPtr->ReadUINT32(sw, FM10000_MA_TCN_PTR_HEAD(), &tcnHead);

    if (status == FM_OK)
    {
        status = switchPtr->ReadUINT32(sw, FM10000_MA_TCN_PTR_TAIL(), &tcnTail);
    }

    DROP_REG_LOCK(sw);

    if (status == FM_OK)
    {
        head = FM_GET_FIELD(tcnHead, FM10000_MA_TCN_PTR_HEAD, Head);
        tail = FM_GET_FIELD(tcnTail, FM10000_MA_TCN_PTR_TAIL, Tail);
        *backlog = (tail - head) & (FM10000_MA_TCN_FIFO_ENTRIES - 1);
    }
    else
    {
        /* TCN FIFO pointer read error. */
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "Error reading MA_TCN_PTR: %s\n",
                     fmErrorMsg(status));
        fmDbgDiagCountIncr(sw, FM_CTR_TCN_PTR_READ_ERR, 1);
        *backlog = 0;
    }

    return status;

}   /* end GetTcnFifoBacklog */




/*****************************************************************************/
/** HandleNewSourceEvent
 * \ingroup intMacMaint
 *
 * \desc            Processes a TCN FIFO NewSource event.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       fifoEntry points to a structure containing information
 *                  about the new SMAC address.
 * 
 * \param[in,out]   numUpdates points to a variable containing the number of
 *                  updates stored in the event buffer.
 * 
 * \param[in,out]   outEvent points to a pointer to the event buffer.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleNewSourceEvent(fm_int         sw,
                                      fm_fifoEntry * fifoEntry,
                                      fm_uint32 *    numUpdates,
                                      fm_event **    outEvent)
{
    fm_macAddressEntry  entry;
    fm_status           status;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d macAddress=%012llx vlanID=%u port=%d\n",
                 sw,
                 fifoEntry->macAddress,
                 fifoEntry->vlanID,
                 fifoEntry->logicalPort);

    FM_CLEAR(entry);

    entry.macAddress = fifoEntry->macAddress;
    entry.vlanID     = fifoEntry->vlanID;
    entry.destMask   = FM_DESTMASK_UNUSED;
    entry.port       = fifoEntry->logicalPort;
    entry.type       = FM_ADDRESS_DYNAMIC;

    /***************************************************
     * Convert VLAN ID to learning FID.
     **************************************************/

    status = fm10000GetLearningFID(sw, fifoEntry->vlanID, &entry.vlanID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);

    /***************************************************
     * Add MAC Table entry.
     **************************************************/

    status = fm10000AddMacTableEntry(sw,
                                     &entry,
                                     fifoEntry->macSource,
                                     FM_DEFAULT_TRIGGER,
                                     numUpdates,
                                     outEvent);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, status);

}   /* end HandleNewSourceEvent */




/*****************************************************************************/
/** HandleMacMovedEvent
 * \ingroup intMacMaint
 *
 * \desc            Processes a TCN FIFO MacMoved event.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       fifoEntry points to a structure containing information
 *                  about the SMAC address that has moved.
 * 
 * \param[in,out]   numUpdates points to a variable containing the number of
 *                  updates stored in the event buffer.
 * 
 * \param[in,out]   outEvent points to a pointer to the event buffer.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status HandleMacMovedEvent(fm_int         sw,
                                     fm_fifoEntry * fifoEntry,
                                     fm_uint32 *    numUpdates,
                                     fm_event **    outEvent)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d macAddress=%012llx vlanID=%u port=%d\n",
                 sw,
                 fifoEntry->macAddress,
                 fifoEntry->vlanID,
                 fifoEntry->logicalPort);

    status = fm10000MoveAddressSecure(sw,
                                      fifoEntry->macAddress,
                                      fifoEntry->vlanID,
                                      fifoEntry->logicalPort,
                                      fifoEntry->entryIndex,
                                      numUpdates,
                                      outEvent);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, status);

}   /* end HandleMacMovedEvent */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000HandleMACTableEvents
 * \ingroup intMacMaint
 *
 * \desc            Services the MA Table Change Notification (TCN) FIFO.
 *                  Called through the HandleMACTableEvents function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000HandleMACTableEvents(fm_int sw)
{
    fm_switch *             switchPtr;
    fm10000_switch *        switchExt;
    fm_int                  numTcnEntries;
    fm_uint32               tcnEntry[FM10000_MA_TCN_DEQUEUE_WIDTH];
    fm_fifoEntry            fifoEntry;
    fm_uint32               backlog;
    fm_uint32               numUpdates;
    fm_event *              outEvent;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n", sw);

    switchPtr       = GET_SWITCH_PTR(sw);
    switchExt       = GET_SWITCH_EXT(sw);
    numTcnEntries   = 0;
    numUpdates      = 0;
    outEvent        = NULL;
    backlog         = 0;
    err             = FM_OK;

    for ( ; ; )
    {
        /***************************************************
         * Limit the number of TCN FIFO entries we will 
         * process in a single invocation. 
         **************************************************/

        if (numTcnEntries >= switchExt->tcnFifoBurstSize)
        {
            GetTcnFifoBacklog(sw, &backlog);

            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_MAC_MAINT,
                         "reached burst limit: sw=%d backlog=%u\n",
                         sw,
                         backlog);
            break;
        }

        /***************************************************
         * Read next entry from MA TCN FIFO.
         **************************************************/

        err = switchPtr->ReadUINT32Mult(sw,
                                        FM10000_MA_TCN_DEQUEUE(0),
                                        FM10000_MA_TCN_DEQUEUE_WIDTH,
                                        tcnEntry);
        if (err != FM_OK)
        {
            /* TCN FIFO read error. */
            fmDbgDiagCountIncr(sw, FM_CTR_TCN_FIFO_READ_ERR, 1);
            break;
        }

        if (!FM_ARRAY_GET_BIT(tcnEntry, FM10000_MA_TCN_DEQUEUE, Valid))
        {
            break;
        }

        ++numTcnEntries;

        if (FM_ARRAY_GET_BIT(tcnEntry, FM10000_MA_TCN_DEQUEUE, U_err))
        {
            /* Uncorrectable error in TCN FIFO entry. */
            fmDbgDiagCountIncr(sw, FM_CTR_TCN_FIFO_PARITY_ERR, 1);
            continue;
        }

        /***************************************************
         * Decode the TCN FIFO entry.
         **************************************************/

        err = DecodeFifoEntry(sw, tcnEntry, &fifoEntry);

        if (err == FM_ERR_INVALID_PORT)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_PORT_ERR, 1);
            continue;
        }
        else if (err != FM_OK)
        {
            /* TCN FIFO entry conversion error. */
            fmDbgDiagCountIncr(sw, FM_CTR_TCN_FIFO_CONV_ERR, 1);
            continue;
        }

        /***************************************************
         * Process the decoded entry.
         **************************************************/

        if (fifoEntry.macSource == FM_MAC_SOURCE_TCN_LEARNED)
        {
            /* Increment number of NewSource events removed from FIFO. */
            fmDbgDiagCountIncr(sw, FM_CTR_TCN_LEARNED_EVENT, 1);

            HandleNewSourceEvent(sw, &fifoEntry, &numUpdates, &outEvent);
        }
        else
        {
            /* Increment number of MacMoved events removed from FIFO. */
            fmDbgDiagCountIncr(sw, FM_CTR_TCN_SEC_VIOL_MOVED_EVENT, 1);

            HandleMacMovedEvent(sw, &fifoEntry, &numUpdates, &outEvent);
        }

    }   /* end for ( ; ; ) */

    /* Send update events. */
    if (numUpdates != 0)
    {
        fmSendMacUpdateEvent(sw,
                             &fmRootApi->eventThread,
                             &numUpdates,
                             &outEvent,
                             FALSE);
    }

    /* Free event buffer if we still have one. */
    if (outEvent != NULL)
    {
        fmReleaseEvent(outEvent);
    }

    /* If the backlog is non-zero, we reached the TCN FIFO burst limit
     * without running out the TCN FIFO. Schedule another pass over the
     * FIFO, to pick up any stragglers. See bug #25096 for discussion. */
    if (backlog != 0)
    {
        fmIssueMacMaintRequest(sw, FM_UPD_SERVICE_MAC_FIFO);
    }

    /* Reenable TCN FIFO PendingEvents interrupts. */
    EnableTcnPendingEvents(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end fm10000HandleMACTableEvents */




/*****************************************************************************/
/** fm10000TCNInterruptHandler
 * \ingroup intMacMaint
 *
 * \desc            Handle MA Table TCN FIFO events.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       events is the TCN event bit mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000TCNInterruptHandler(fm_int sw, fm_uint32 events)
{

    /* FM_LOG_CAT_EVENT_INTR? */
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d events=%08x\n",
                 sw,
                 events);

    fmDbgDiagCountIncr(sw, FM_CTR_TCN_INTERRUPT, 1);

    if (FM_GET_BIT(events, FM10000_MA_TCN_IP, PendingEvents))
    {
        fmDbgDiagCountIncr(sw, FM_CTR_TCN_PENDING_EVENTS, 1);

        fmIssueMacMaintRequest(sw, FM_UPD_SERVICE_MAC_FIFO);
    }

    if (FM_GET_BIT(events, FM10000_MA_TCN_IP, TCN_Overflow))
    {
        /* The FM10000 TCN FIFO reports NewSource and MACMoved events,
         * both of which count as LEARNED events, so a TCN FIFO overflow
         * is a LEARNED overflow. */
        fmDbgDiagCountIncr(sw, FM_CTR_TCN_LEARNED_OVERFLOW, 1);

        /* TCN FIFO overflow events are just counted. The FM10000 does
         * software-based learning, so there are no inconsistencies in the 
         * hardware MA table to be addressed (hence no UPDATE_OVERFLOW 
         * maintenance event), and the FIFO will be drained (creating 
         * room for more entries) in response to a PendingEvents
         * interrupt. */

        EnableTcnOverflow(sw);
    }

    /* Reenable FH_TAIL TCN interrupts. */
    EnableFHTailTCN(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, FM_OK);

}   /* end fm10000TCNInterruptHandler */

