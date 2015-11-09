/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_generic_rx.c
 * Creation Date:   May 8th, 2013
 * Description:     Generic receive for the FM10000 series
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#define IS_TRAP_GLORT(x)    \
        (((x) & FM10000_GLORT_CPU_MASK) == FM10000_GLORT_CPU_BASE)

#define BPDU_DMAC FM_LITERAL_64(0x0180C2000000)

#define EXTRACT_DMAC(buf) \
    (((fm_uint64) ntohl((buf)->data[0])) << 16) | \
     ((ntohl((buf)->data[1]) >> 16))

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/* To be removed once the correct one is defined. */
void fm10000DbgSelfTestEventHandler(fm_event * event)
{

}   /* end fm10000DbgSelfTestEventHandler */


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** EventFreeHandler
 * \ingroup intPlatformCommon
 *
 * \desc            Handles notification when a event free is available to
 *                  continue processing receiving packets.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          None
 *
 *****************************************************************************/
static void EventFreeHandler(fm_int sw)
{
    fmSignalSemaphore(&GET_PLAT_PKT_STATE(sw)->eventsAvailableSignal);
}




/*****************************************************************************/
/** TimestampsEnabled
 * \ingroup intPlatformCommon
 *
 * \desc            Determines whether timestamps are enabled for the
 *                  specified ingress port.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port number.
 *
 * \return          TRUE if timestamps are enabled, FALSE otherwise.
 *
 *****************************************************************************/
static fm_bool TimestampsEnabled(fm_int sw, fm_int port)
{
    fm_status   err;
    fm_bool     isEnabled;

    err       = FM_OK;
    isEnabled = FALSE;

    err = fm10000GetPortAttribute(sw,
                                  port,
                                  FM_PORT_ACTIVE_MAC,
                                  FM_PORT_LANE_NA,
                                  FM_PORT_TIMESTAMP_GENERATION,
                                  (void *) &isEnabled);

    return (err == FM_OK) && isEnabled;

}   /* end TimestampsEnabled */



/*****************************************************************************/
/** IsDeliverPepToPepTimestamp
 * \ingroup intPlatformCommon
 *
 * \desc            Determines whether ingress timestamp of pep to pep 
 *                  packets are to be delivered to appropriate pep.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mirrorTrapCodeId is the trapcode Id set on the mirror 
 *                  created for timestamp retrieval purpose.
 *
 * \return          TRUE if timestamp to be delivered, FALSE otherwise.
 *
 *****************************************************************************/
static fm_bool IsDeliverPepToPepTimestamp(fm_int sw, 
                                          fm_int logicalPort, 
                                          fm_int mirrorTrapCodeId)
{
    fm_status              err;
    fm_portTxTimestampMode txTimestampMode;
    fm_int                 pepToPepTimestampTrapcodeId;

    err = fm10000GetSwitchAttribute(sw,
                                    FM_SWITCH_PEP_TIMESTAMP_TRAP_ID,
                                    (void *)&pepToPepTimestampTrapcodeId);

    if ( ( err == FM_OK ) && 
         ( pepToPepTimestampTrapcodeId != 0 ) &&
         ( mirrorTrapCodeId == pepToPepTimestampTrapcodeId ) )
    {
        err = fm10000GetLogicalPortAttribute(sw,
                                             logicalPort,
                                             FM_LPORT_TX_TIMESTAMP_MODE,
                                             (void *)&txTimestampMode);
        return (err == FM_OK) && 
               (txTimestampMode >= FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_PEP);
    }
    else
    {
        /* PEP to PEP timestamp is not enabled. */
        return FALSE;
    }
                                         
}   /* end IsDeliverPepToPepTimestamp */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/** fm10000PacketReceiveProcess
 * \ingroup intPlatformCommon
 *
 * \desc            Process the received packet and enqueue to higher layer.
 *
 * \note            To support FIBM, this function cannot make any function
 *                  calls that read from hardware. This will cause a deadlock
 *                  if such an attempt is made.
 *
 * \note            This function handles for all different interfaces: LCI, 
 *                  netlink, nic, etc. Use flags option if different behaviour
 *                  is needed for a specific interface.
 *
 * \note            This function is also responsible for freeing the buffer
 *                  if needed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       buffer points to the receive packet buffer. If ISL tag is 
 *                  present in data buffer, the ISL tags should be in 
 *                  network-byte order.
 *
 * \param[in]       pIslTag points to the ISL tag, if not present in data buffer.
 *                  Set to NULL if ISL tag is present in data buffer. If ISL
 *                  tag is not present in the data buffer, the data buffer must
 *                  have enough space for vlan tag to be inserted. This tag
 *                  should be in host-byte order.
 *
 * \param[in]       sbData is a pointer to the caller allocated storage where
 *                  any sideband data is stored. Can be set to NULL if there
 *                  is no sideband data to carry. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000PacketReceiveProcess(fm_int              sw,
                                      fm_buffer *         buffer,
                                      fm_uint32 *         pIslTag,
                                      fm_pktSideBandData *sbData)
{
    fm_packetHandlingState *    ps;
    fm_status                   err = FM_OK;
    fm_switch *                 switchPtr;
    fm_eventPktRecv *           recvEvent = NULL;
    fm_uint32                   fType;
    fm_uint32                   user;
    fm_event *                  currentRecvEvent = NULL;
    fm_bool                     cleanupFreeBuffer = TRUE;
    fm_uint32                   ISLTag[2];
    fm_uint                     srcGlort;
    fm_uint                     dstGlort;
    fm_int                      trapAction;
    fm_event                    directEnqueueEvent;
    fm_timestamp                ts = { 30, 0 };
    fm_switchEventHandler       switchEventHandler;
    fm_int                      vlan;
    fm_int                      svAction;
    fm_macaddr                  macAddr;
    fm_int                      switchNum;
    fm_int                      srcPort;
    fm_bool                     dropPacketUnknownPort;
#ifndef FM_SUPPORT_SWAG
    fm_uint64                   dmac;
    fm_int                      stpInstance;
    fm_int                      portState;
    fm_int                      trapCodeLogArpRedirect;
    fm_bool                     logArpRedirect;
    fm_bool                     isPciePort;
    fm_int                      portNbr;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_RX, 
                 "sw = %d pIslTag = %p\n", 
                 sw, (void *) pIslTag);

    if ( (sw < 0) || (sw >= fmRootPlatform->cfg.numSwitches) )
    {
        fmFreeBufferChain(sw, buffer);
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, FM_ERR_INVALID_SWITCH);
    }

    if ( !SWITCH_LOCK_EXISTS(sw) )
    {
        fmFreeBufferChain(sw, buffer);
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, FM_ERR_INVALID_SWITCH);
    }

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    ps = GET_PLAT_PKT_STATE(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);

        err = FM_ERR_SWITCH_NOT_UP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);
    }

    if (switchPtr->state < FM_SWITCH_STATE_INIT ||
        switchPtr->state > FM_SWITCH_STATE_GOING_DOWN)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);

        err = FM_ERR_SWITCH_NOT_UP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);
    }

    if (pIslTag)
    {
        /* ISL tag is given separately */
        ISLTag[0] = pIslTag[0];
        ISLTag[1] = pIslTag[1];
    }
    else
    {
        /* This is a F56 tagged frame */
        ISLTag[0] = ntohl(buffer->data[0]);
        ISLTag[1] = ntohl(buffer->data[1]);
    }

    /* This is an F56 tagged frame */
    fType = (ISLTag[0] >> 22 ) & 0x3;
    
    /**************************************************
     * This is a mgmt message, let fibm handle it 
     **************************************************/

    if ( fType == FM_FTYPE_MANAGEMENT )
    {
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_FIBM, 1);

        if (switchPtr->ProcessMgmtPacket)
        {
            err = switchPtr->ProcessMgmtPacket(sw, buffer, pIslTag);

            /* fmFibmProcessPktHandler will free the buffer after use */
            cleanupFreeBuffer = FALSE;
        }
        else
        {
            fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
        }

        goto ABORT;
    }

    fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_COMPLETE, 1);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX, "packet complete\n");
    
    /**************************************************
     * Get ISL header information.
     **************************************************/

    user        = (ISLTag[0] >> 24) & 0xff;
    vlan        = ISLTag[0] & 0xfff;

    srcGlort    = (ISLTag[1] >> 16) & 0xffff;
    dstGlort    = ISLTag[1] & 0xffff;
    trapAction  = IS_TRAP_GLORT(dstGlort) ? (dstGlort & 0xff) : 0;

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                 "ISL: 0x%08x 0x%08x srcGlort=0x%04x destGlort=0x%04x fType=%04x "
                 "user=%d vlan=%d trapAction=%02x\n",
                 ISLTag[0], ISLTag[1], srcGlort, dstGlort, fType, 
                 user, vlan, trapAction);

    if (!pIslTag)
    {
        /******************************************
         * Discard the ISL Tag present in the data.  
         ******************************************/
        buffer->data   += 2;
        buffer->len    -= 8;
    }
    
    /**************************************************
     * Determine source port from glort.
     **************************************************/

    switchNum = sw;
    srcPort   = -1;
    
    fmGetSwitchEventHandler(sw, &switchEventHandler);

    if (switchEventHandler == fm10000DbgSelfTestEventHandler)
    {
        /* The switch is in self-test, all the incoming frames come
         * from the CPU. */
        srcPort = 0;
    }
    else if (fmGetGlortLogicalPort(sw, srcGlort, &srcPort) != FM_OK)
    {
        /* The code will return the physical or remote port if found above. */

        /* Can't associate with a port on local switch,
         * maybe it from fibm without a remote port created
         */
        if (fmFindSlaveSwitchPortByGlort(srcGlort,
                                         &switchNum,
                                         &srcPort) != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                         "Unable to get source port from glort 0x%x\n",
                         srcGlort);

            /* check if the frame should be dropped */
            err = fm10000GetSwitchAttribute(sw,
                                            FM_SWITCH_RX_PKT_DROP_UNKNOWN_PORT,
                                            &dropPacketUnknownPort);

            if (dropPacketUnknownPort)
            {
                /**************************************************
                 * Just drop the packet. A possible reason for
                 * fm10000GetLogicalPort to fail is if a packet
                 * was received on a LAG, then the LAG was deleted
                 * before we processed the packet.
                 **************************************************/
                fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);

                goto ABORT;
            }
            else
            {
                /* set unknown source port */
                srcPort = -1;
            }
        }
    }
    
    /**************************************************
     * If this is a trapped frame, inspect the trap
     * code to see if it's the result of a BPDU trap 
     * or one of the MAC security triggers. 
     **************************************************/

    if (IS_TRAP_GLORT(dstGlort))
    {
#ifndef FM_SUPPORT_SWAG
        /* Drop BPDU Frames if ingress port is in STP discarding state.
         * For swag, it is handled in swag Event handler. */
        if (trapAction == FM10000_MIRROR_IEEE_CODE &&
            !fmIsRemotePort(sw, srcPort))
        {
            dmac = EXTRACT_DMAC(buffer);

            if (dmac == BPDU_DMAC)
            {
                if (fmIsCardinalPort(sw, srcPort) )
                {
                    err = fmFindInstanceForVlan(sw, vlan, &stpInstance);
                    if (err == FM_OK)
                    {
                        err = fmGetSpanningTreePortState(sw, 
                                                         stpInstance, 
                                                         srcPort, 
                                                         &portState);
    
                        if (err == FM_OK)
                        {
                            /* Ingress port is disabled and DMAC == BPDU */
                            if (portState == FM_STP_STATE_DISABLED)
                            {
                                /* Dropping BPDU packet */
                                fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
                                goto ABORT;
                            }
                        }
                        else
                        {
                            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                         "Unable to find STP port state for instance "
                                         "%d, port %d\n",
                                         stpInstance,
                                         srcPort);
                        }
                    }
                    else
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                     "Unable to find STP instance for vlan %d\n",
                                     vlan);
                    }
                } 

                /* Drop BPDU Frames if the ingress port is a PCIe port and is not the 
                 * managment port. */
                 
                if (fmIsVirtualPort(sw, srcPort))
                {
                    if (fm10000MapVirtualGlortToPhysicalPort(sw, srcGlort, &portNbr) != FM_OK)
                    {
                        portNbr = srcPort;
                    }
                }
                else
                {
                    portNbr = srcPort;
                }
 
                err = fmIsPciePort( sw, portNbr, &isPciePort );
                if (err == FM_OK)
                {
                    if (( isPciePort ) && (!fmIsMgmtPort( sw, portNbr)))
                    {
                        /* Dropping BPDU packet */
                        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
                        goto ABORT;
                        
                    }
                }

            }
        }
#endif

        /* Mirrored packets */
        if ( (trapAction & 0xf0) == FM10000_MIRROR_CPU_CODE_BASE )
        {
            if ( ( sbData != NULL ) &&
                 ( IsDeliverPepToPepTimestamp(sw, srcPort, (trapAction & 0xf)) ) )
            {
                err = fmDeliverPacketTimestamp(sw,
                                               srcGlort,
                                               0,
                                               0,
                                               sbData->rawTimeStamp);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err); 
            }
        }

        err = fm10000CheckSecurityViolation(sw, srcPort, trapAction, &svAction);

        if (err == FM_OK && svAction != FM10000_SV_NO_ACTION)
        {
            /* Get the source MAC address on which the violation occurred. */
            macAddr = fmGetPacketSrcAddr(sw, buffer);

            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                         "macAddr=%012llx action=%s\n",
                         macAddr,
                         fmSvActionToText(svAction));

            /* Take the appropriate action. */
            switch (svAction)
            {
                case FM10000_SV_UNKNOWN_SMAC_DROP:
                case FM10000_SV_SECURE_SMAC_DROP:
                case FM10000_SV_NON_SECURE_SMAC_DROP:
                    /* Silently drop the packet. */
                    fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_SECURITY, 1);
                    goto ABORT;

                case FM10000_SV_UNKNOWN_SMAC_EVENT:
                case FM10000_SV_SECURE_SMAC_EVENT:
                case FM10000_SV_NON_SECURE_SMAC_EVENT:
                    /* Generate a security violation event. */
                    err = fm10000ReportSecurityEvent(sw, macAddr, vlan, srcPort);
                    fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_SV_EVENT, 1);
                    goto ABORT;

                default:
                    /* Queue packet to application layer. */
                    break;

            }   /* end switch (svInfo->trapAction) */

        }   /* end if (err == FM_OK && svInfo.trapType != ...) */
#ifndef FM_SUPPORT_SWAG
        /* ARP Redirect will be processed at the event handler level
         * on SWAG enabled platform. */
        else if (err == FM_OK)
        {
            err = fm10000GetSwitchTrapCode(sw,
                                           FM_TRAPCODE_LOG_ARP_REDIRECT,
                                           &trapCodeLogArpRedirect);
            if ((err == FM_OK) && (trapAction == trapCodeLogArpRedirect))
            {
                logArpRedirect = FALSE;
                err = fm10000RoutingProcessArpRedirect(sw,
                                                       &logArpRedirect);
                if ( (err == FM_OK) && !logArpRedirect )
                {
                    goto ABORT;
                }
            }
        }
#endif

    }   /* end if (IS_TRAP_GLORT(dstGlort)) */
    
    /**************************************************
     * Allocate event object.
     **************************************************/

    if ( (ps->rxDirectEnqueueing) && 
         (switchEventHandler != fm10000DbgSelfTestEventHandler) )
    {
        directEnqueueEvent.sw       = sw;
        directEnqueueEvent.eventID  = FM_EVID_HIGH_PKT_RECV;
        directEnqueueEvent.type     = FM_EVENT_PKT_RECV;
        directEnqueueEvent.priority = FM_EVENT_PRIORITY_LOW;
        currentRecvEvent = &directEnqueueEvent;
    }
    else
    {
        for (;;)
        {
            currentRecvEvent = fmAllocateEvent(sw,
                                               FM_EVID_HIGH_PKT_RECV,
                                               FM_EVENT_PKT_RECV,
                                               FM_EVENT_PRIORITY_LOW);

            if (currentRecvEvent)
            {
                break;
            }

            /* Will get notification when a free event is available */
            fmAddEventFreeNotify(sw,
                                 EVENT_FREE_NOTIFY_PKT_INTR,
                                 EventFreeHandler);

            err = fmWaitSemaphore(&ps->eventsAvailableSignal, &ts);

            if (err == FM_ERR_SEM_TIMEOUT)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Unable to acquire event object in 30 seconds!\n");
                /* Count occurrences? */
                /* Abort, discarding packet? */
            }
        }
    }
    
    /**************************************************
     * Set packet properties from header word.
     **************************************************/

    recvEvent = &currentRecvEvent->info.fpPktEvent;

    /* Save the switch number */
    recvEvent->switchNum = switchNum;

    /* This will be set when we allocate the first buffer */
    recvEvent->pkt = buffer;

    /* Store the ISL tag (F56) */
    recvEvent->ISLTag[0] = ISLTag[0];
    recvEvent->ISLTag[1] = ISLTag[1];

    /* Store the source port */
    recvEvent->srcPort = srcPort;

    /* Store the VLAN, priority and trap code */
    recvEvent->vlan = vlan;

    /* Priority in fm_eventPktRecv struct is documented to
     * contain internal switch priority. */
    recvEvent->priority     = (ISLTag[0] >> 24) & 0xf;
    recvEvent->vlanPriority = (ISLTag[0] >> 13) & 0x7;
    recvEvent->trapAction   = trapAction;

    /* Unknown for when CPU tagging state equals untagged, otherwise etherType
     * is available in the frame */
    recvEvent->vlanEtherType = 0;

    /* Set this for event handler to pass up the correct switch.
     * Actually this should be the master switch, but most of
     * the upper code uses this info, instead of switchNum. */
    currentRecvEvent->sw = switchNum;

    if (sbData != NULL && TimestampsEnabled(sw, recvEvent->srcPort))
    {
        recvEvent->rawIngressTimeV2  = sbData->rawTimeStamp;
        recvEvent->ingressTimeV2     = sbData->ingressTimestamp;
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX, 
                     "Raw Timestamp = 0x%" FM_FORMAT_64 "x " 
                     "Wall ingress timestamp = %" FM_FORMAT_64 "d."
                     "%" FM_FORMAT_64 "d\n",
                     sbData->rawTimeStamp,
                     sbData->ingressTimestamp.seconds,
                     sbData->ingressTimestamp.nanoseconds);
    }
    else
    {
        /* Timestamps not supported. */
        FM_CLEAR(recvEvent->ingressTimeV2);
        recvEvent->rawIngressTimeV2 = 0;
    }

    /***************************************************
     * The CRC is incorrect since it includes the ISL 
     * tags we removed. Make it zero so the user knows 
     * that the CRC should not be used.
     **************************************************/
    fmPacketClearCRC(buffer);

    /**************************************************
     * Send the event up to the event handler.
     **************************************************/

    err = fmPacketReceiveEnqueue(sw,
                                 currentRecvEvent,
                                 fm10000DbgSelfTestEventHandler);
    if (err != FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                     "Unable to send event to event handler\n");

        /***************************************************
         * fmPacketReceiveEnqueue already handles
         * freeing the buffer and the event, so we 
         * simply let the code take the normal
         * exit path.
         **************************************************/
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                     "Successfully sent event to event handler\n");
    }

    /* Clean exit */
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);

ABORT:
    /* Free the buffer */
    if (cleanupFreeBuffer)
    {
        fmFreeBufferChain(sw, buffer);
    }

    if (!ps->rxDirectEnqueueing && currentRecvEvent)
    {
        /* Free also the event */
        fmReleaseEvent(currentRecvEvent);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);

}   /* end fm10000PacketReceiveProcess */

