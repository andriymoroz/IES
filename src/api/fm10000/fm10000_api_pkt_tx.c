
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_pkt_tx.c
 * Creation Date:   2013
 * Description:     Functions and threads related to packet sending
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

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000SendPacket
 * \ingroup intPkt
 *
 * \desc            Sends a packet over the CPU interface.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       info points to a table of information describing
 *                  information relevant to the packet and the processing to
 *                  be done.  The logical destMask field is not used.
 *
 * \param[in]       pkt points to the packet buffer to be sent.
 *
 * \return          FM_ERR_UNSUPPORTED.
 * 
 *****************************************************************************/
fm_status fm10000SendPacket(fm_int         sw,
                           fm_packetInfo *info,
                           fm_buffer *    pkt)
{
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw = %d, " "info = %p,"
                                                      "pkt = %p\n",
                 sw, (void *) info, (void *) pkt);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_UNSUPPORTED);

}   /* end fm10000SendPacket */



/*****************************************************************************/
/** fm10000SetPacketInfo
 * \ingroup intPkt
 *
 * \desc            Set the fmPacketInfo struct to be passed to fmSentPacketExt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      info points to the user allocated fmPacketInfo struct.
 *
 * \param[in]       destMask contains the destination logical port mask.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_DSTMASK if destMask contains more than
 *                  one port.
 *
 *****************************************************************************/
fm_status fm10000SetPacketInfo(fm_int         sw,
                              fm_packetInfo *info,
                              fm_uint32      destMask)
{
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, " "info = %p, "
                 "destMask = 0x%x\n",
                 sw, (void *) info, destMask);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_UNSUPPORTED);

}   /* end fm10000SetPacketInfo */



/*****************************************************************************/
/** fm10000SendPacketDirected
 * \ingroup intPkt
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portList points to an array of logical port numbers the 
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 * 
 * \param[in]       info is a pointer to the packet information structure which
 *                  contains some relevant information describing the packet.
 *                  See 'fm_packetInfoV2' for more information.
 *                  Note: the structure pointed to by info must have all fields
 *                  initialized. Any unused field should be set to zero.
 *                  Failure to initialize all fields can result in the packet
 *                  being mishandled.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_STATE if switch is down, or if the 
 *                  ports contained in the portList are not in 
 *                  the proper port state.
 * \return          FM_ERR_INVALID_PORT if portList contains invalid port 
 *                  number.
 * \return          FM_ERR_INVALID_ARGUMENT if pkt is not a valid packet buffer.
 * \return          FM_ERR_BAD_IOCTL if the transmit packet queue is full.
 *
 *****************************************************************************/
fm_status fm10000SendPacketDirected(fm_int           sw, 
                                   fm_int *         portList, 
                                   fm_int           numPorts, 
                                   fm_buffer *      pkt,
                                   fm_packetInfoV2 *info)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw=%d portList=%p numPort=%d " 
                    "pkt=%p\n", sw, (void *)portList, numPorts, (void *) pkt);

    err = FM_FM10000_SEND_PACKET_DIRECTED(sw, portList, numPorts, pkt, info);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fm10000SendPacketDirected */



/*****************************************************************************/
/** fm10000SendPacketSwitched
 * \ingroup intPkt
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_STATE if switch is down.
 * \return          FM_ERR_INVALID_ARGUMENT if pkt is not a valid packet buffer.
 * \return          FM_ERR_INVALID_PORT if port is not valid.
 * \return          FM_ERR_INVALID_SWITCH if sw is not valid.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SendPacketSwitched(fm_int sw, fm_buffer *pkt)
{
    fm_status     err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw=%d pkt=%p\n", sw, (void *) pkt);

    err = FM_FM10000_SEND_PACKET_SWITCHED(sw, pkt);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fm10000SendPacketSwitched */




/*****************************************************************************/
/** fm10000SendPacketISL
 * \ingroup intPkt
 *
 * \desc            Send a message packet to the switch with custom ISL
 *                  tags.
 *                                                                      \lb\lb
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       islTag points to an array of words that contain the ISL tag
 *
 * \param[in]       islTagFormat is the format of the ISL tag.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue
 *                  is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 *
 *****************************************************************************/
fm_status fm10000SendPacketISL(fm_int          sw,
                              fm_uint32 *     islTag,
                              fm_islTagFormat islTagFormat,
                              fm_buffer *     pkt)
{
    fm_status err;

    FM_LOG_ENTRY( FM_LOG_CAT_EVENT_PKT_TX,
                  "sw=%d islTag = %p islTagFormat = %d pkt=%p\n",
                  sw,
                  (void *) islTag,
                  islTagFormat,
                  (void *) pkt );

    switch (islTagFormat)
    {
        case FM_ISL_TAG_F64:
        case FM_ISL_TAG_F56:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    err = FM_FM10000_SEND_PACKET_ISL(sw, islTag, islTagFormat, pkt);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fm10000SendPacketISL */

