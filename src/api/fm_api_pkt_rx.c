/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_pkt_rx.c
 * Creation Date:   July 2, 2007
 * Description:     Packet Reception Thread.
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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
/** fmReceivePacket
 * \ingroup intApi
 *
 * \desc            Handles reception of packets.
 *
 * \param[in]       sw is the switch on which the packet is being recevied.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmReceivePacket(fm_int sw)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY(err, fmRootApi->fmSwitchStateTable[sw]->ReceivePacket, sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fmReceivePacket */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmReceivePacketTask
 * \ingroup intApi
 *
 * \desc            Handles reception of packets.
 *
 * \param[in]       args contains a pointer to the thread information.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void *fmReceivePacketTask(void *args)
{
    fm_thread *thread;
    fm_status  err = FM_OK;
    fm_int     sw;

    thread = FM_GET_THREAD_HANDLE(args);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "thread = %s\n",
                 thread->name);


    /**************************************************
     * Loop forever, waiting for signals from the
     * interrupt handler.
     **************************************************/

    while (TRUE)
    {
        /**************************************************
         * Wait for a signal from the interrupt handler.
         **************************************************/

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                     "fmLCIReceivePacketTask: waiting for signal..\n");

        err = fmWaitSemaphore(&fmRootApi->packetReceiveSemaphore,
                              FM_WAIT_FOREVER);

        if (err != FM_OK)
        {
            FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                         "%s: %s\n",
                         thread->name,
                         fmErrorMsg(err) );
            continue;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                     "fmReceivePacketTask: signaled!\n");

        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if ( (fmRootApi->fmSwitchStateTable[sw] != NULL) 
                  && (!fmRootApi->isSwitchFibmSlave[sw]) )
            {

                fmReceivePacket(sw);
            }
        }

    } /* end while (TRUE) */

    /**************************************************
     * Should never exit.
     **************************************************/

    FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                 "ERROR: fmReceivePacketTask: exiting inadvertently!\n");

    return NULL;

}   /* end fmReceivePacketTask */




/*****************************************************************************/
/** fmGetPacketDestAddr
 * \ingroup intSwitch
 *
 * \desc            Retrieve the destination MAC address from a packet, taking
 *                  into account the endianness of the platform's host
 *                  processor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pkt points to the packet buffer containing the destination
 *                  address.
 *
 * \return          The destination MAC address in native platform host format.
 *
 *****************************************************************************/
fm_macaddr fmGetPacketDestAddr(fm_int sw, fm_buffer *pkt)
{
    fm_macaddr destMacAddress = FM_LITERAL_64(0);
    fm_uint32  destMacAddressWord1;
    fm_uint32  destMacAddressWord2;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d, pkt=%p\n", sw, (void *) pkt);

    destMacAddressWord1 = ntohl(pkt->data[0]);
    destMacAddressWord2 = ntohl(pkt->data[1]);

    destMacAddress = ( (fm_uint64) destMacAddressWord1 << 32 ) |
                     ( (fm_uint64) (destMacAddressWord2 & 0xffff0000) );

    destMacAddress = destMacAddress >> 16;

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_SWITCH,
                       destMacAddress,
                       "sw=%d, pkt=%p dmac=" FM_FORMAT_ADDR "\n",
                       sw,
                       (void *) pkt,
                       destMacAddress);

}   /* end fmGetPacketDestAddr */




/*****************************************************************************/
/** fmGetPacketSrcAddr
 * \ingroup intSwitch
 *
 * \desc            Retrieve the source MAC address from a packet, taking
 *                  into account the endianness of the platform's host
 *                  processor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pkt points to the packet buffer containing the source
 *                  address.
 *
 * \return          The source MAC address in native platform host format.
 *
 *****************************************************************************/
fm_macaddr fmGetPacketSrcAddr(fm_int sw, fm_buffer *pkt)
{
    fm_macaddr srcMacAddress;
    fm_uint32  srcMacAddressWord1;
    fm_uint32  srcMacAddressWord2;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d, pkt=%p\n", sw, (void *) pkt);

    srcMacAddressWord1 = ntohl(pkt->data[1]);
    srcMacAddressWord2 = ntohl(pkt->data[2]);

    srcMacAddressWord1 &= (0x0000ffff);

    srcMacAddress = ( (fm_uint64) srcMacAddressWord1 << 32 ) |
                    ( (fm_uint64) srcMacAddressWord2 );

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_SWITCH,
                       srcMacAddress,
                       "sw=%d, pkt=%p smac=" FM_FORMAT_ADDR "\n",
                       sw,
                       (void *) pkt,
                       srcMacAddress);

}   /* end fmGetPacketSrcAddr */
