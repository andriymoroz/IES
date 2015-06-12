/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_pkt_tx.c
 * Creation Date:   2007
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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmSendPacket
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            DEPRECATED in favor of ''fmSendPacketDirected'' and
 *                  ''fmSendPacketSwitched''.
 *                                                                      \lb\lb
 *                  Send a message packet to the switch. The packet data is
 *                  passed in an ''fm_buffer'' chain with the data appearing
 *                  in network byte order. See ''fm_buffer'' for more
 *                  information on how packet data appears in the ''fm_buffer''
 *                  chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word and the
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain should not include the CRC.
 *                                                                      \lb\lb
 *                  Untagged frames will be tagged according to the tagging
 *                  rules of the egress port on the CPU default VLAN. If the
 *                  packet is intended to go out with a VLAN tag other than 
 *                  the CPU default VLAN, the tag must be explicitly included
 *                  in the packet data by the caller, and the egress port 
 *                  must be configured to transmit tagged frames on the intended
 *                  VLAN.
 *
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       destMask is a bit mask of physical destination ports to
 *                  send to, or zero if the switch should determine the
 *                  destination. On FM3000 and FM4000 devices, destMask may 
 *                  have only a single bit set to 1. Use ''fmSendPacketExt'' 
 *                  and a multicast group logical port to send to more than one
 *                  port.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if switch is down.
 * \return          FM_ERR_INVALID_PORT_STATE if the
 *                  destination ports contained in the info struct are not in
 *                  the proper port state for direct packet send.
 * \return          FM_ERR_INVALID_DSTMASK if destMask contains more than one
 *                  bit set to a 1 on a FM3000 or FM4000 device.
 * \return          FM_ERR_BAD_IOCTL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 *
 *****************************************************************************/
fm_status fmSendPacket(fm_int sw, fm_int destMask, fm_buffer *pkt)
{
    fm_packetInfo info;
    fm_status     err;
    fm_switch *   switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX,
                     "sw=%d destMask=0x%x pkt=%p\n",
                     sw, destMask, (void *) pkt);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    memset(&info, 0, sizeof(info) );

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetPacketInfo,
                       sw,
                       &info,
                       (fm_uint32) destMask);

    if (err == FM_OK)
    {
        err = fmSendPacketExt(sw, &info, pkt);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacket */




/*****************************************************************************/
/** fmSendPacketExt
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            DEPRECATED in favor of ''fmSendPacketDirected'' and
 *                  ''fmSendPacketSwitched''.
 *                                                                      \lb\lb
 *                  Send a message packet to the switch with an information
 *                  structure describing the packet's special needs, such as
 *                  whether or not egress processing rules should be applied.
 *                  The packet data is passed in an ''fm_buffer'' chain with
 *                  the data appearing in network byte order. See ''fm_buffer''
 *                  for more information on how packet data appears in the
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word and the
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain should not include the CRC.
 *                                                                      \lb\lb
 *                  The packet can be VLAN tagged or not. If it is to be
 *                  VLAN tagged, the tag can be specified in two ways. First,
 *                  the caller can build the packet without a VLAN tag in
 *                  the packet data and specify that the packet be tagged
 *                  according to the tagging rules of the egress port.
 *                  The ''fm_packetInfo'' information structure should have
 *                  its useEgressRules field set to TRUE and its vlanId and
 *                  vlanPriority fields set to the values that should appear
 *                  in the VLAN tag. The VLAN tag will be inserted into the
 *                  packet by the API, but only if the egress port is in the
 *                  VLAN specified by the vlanId field and is configured to
 *                  transmit tagged frames.
 *                                                                      \lb\lb
 *                  The second way is for the caller to explicitly include
 *                  the desired VLAN tag in the packet data and set the
 *                  useEgressRules field to FALSE. If useEgressRules is set
 *                  to TRUE, it will be ignored, but only if the VLAN tag in
 *                  the packet data is 0x8100 (for both FM2000, FM3000 and 
 *                  FM4000) or if it matches the configured value for the
 *                  ''FM_PORT_PARSER_VLAN_TAG_A'' or ''FM_PORT_PARSER_VLAN_TAG_B''
 *                  port attributes (FM3000 and FM4000 only - see 
 *                  ''Port Attributes'').
 *                                                                      \lb\lb
 *                  If the packet is to egress unconditionally untagged, the
 *                  caller should not include a VLAN tag in the packet data
 *                  and should set the useEgressRules field to FALSE.   
 *                                                                      \lb\lb
 *                  If the destMask (FM2000 only) or logicalPort (FM2000, 
 *                  FM3000 and FM4000) member of the info argument (see 
 *                  ''fm_packetInfo'') is non-zero, the packet will be 
 *                  transmitted to the port indicated by the caller. Note
 *                  that it is the responsibility of the application to assess
 *                  the spanning tree state of the destination port/VLAN prior
 *                  to calling this function.
 *
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately (typically by calling
 *                  ''fmFreeBufferChain'').
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       info is a pointer to the packet information structure which
 *                  contains the destination mask and other relevant
 *                  information describing the packet. See 'fm_packetInfo' for
 *                  more information. Note: the structure pointed to by info
 *                  must have all fields initialized. Any unused field should
 *                  be set to zero. Failure to initialize all fields can result
 *                  in the packet being mishandled.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if switch is down.
 * \return          FM_ERR_INVALID_PORT_STATE if the
 *                  destination ports contained in the info struct are not in
 *                  the proper port state for direct packet send.
 * \return          FM_ERR_INVALID_PORT if info->logicalPort is invalid.
 * \return          FM_ERR_BAD_IOCTL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 *
 *****************************************************************************/
fm_status fmSendPacketExt(fm_int sw, fm_packetInfo *info, fm_buffer *pkt)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX,
                     "sw=%d info=%p pkt=%p\n",
                     sw,
                     (void *) info,
                     (void *) pkt);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->SendPacket, sw, info, pkt);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketExt */




/*****************************************************************************/
/** fmSendPacketDirected
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Send a message packet to the switch in directed mode, where
 *                  the caller specifies the destination port(s) for the packet.
 *                                                                      \lb\lb
 *                  The packet data is passed in an ''fm_buffer'' chain with 
 *                  the data appearing in network byte order. See ''fm_buffer'' 
 *                  for more information on how packet data appears in the 
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word; and the 
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain, should not include the CRC. 
 *                                                                      \lb\lb
 *                  The packet will egress the switch with or without a VLAN
 *                  tag, exactly as it is provided by the caller.
 *                                                                      \lb\lb
 *                  This function will transmit the packet regardless of
 *                  spanning tree state.
 *
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portList points to an array of logical port numbers to
 *                  which the switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch number is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is down.
 * \return          FM_ERR_INVALID_PORT_STATE if the ports contained in the 
 *                  portList are not in the proper port state.
 * \return          FM_ERR_INVALID_PORT if portList contains an invalid port 
 *                  number.
 * \return          FM_ERR_INVALID_ARGUMENT if portList is NULL, 
 *                  numPorts is not positive, or pkt is not a valid packet 
 *                  buffer.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 *
 *****************************************************************************/
fm_status fmSendPacketDirected(fm_int       sw, 
                               fm_int *     portList, 
                               fm_int       numPorts,
                               fm_buffer *  pkt)
{

    fm_status       err;
    fm_packetInfoV2 info;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX,
                     "sw=%d portList=%p numPort=%d pkt=%p\n", 
                     sw, (void *)portList, numPorts, (void *) pkt);

    if ( (numPorts <= 0) || (portList == NULL) || (pkt == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(info);

    info.switchPriority = FM_USE_VLAN_PRIORITY;

    err = fmSendPacketDirectedV2(sw, portList, numPorts, pkt, &info);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketDirected */




/*****************************************************************************/
/** fmSendPacketDirectedV2
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Send a message packet to the switch in directed mode, where
 *                  the caller specifies the destination port(s) for the packet.
 *                                                                      \lb\lb
 *                  The packet data is passed in an ''fm_buffer'' chain with 
 *                  the data appearing in network byte order. See ''fm_buffer'' 
 *                  for more information on how packet data appears in the 
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word; and the 
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain, should not include the CRC. 
 *                                                                      \lb\lb
 *                  The packet will egress the switch with or without a VLAN
 *                  tag, exactly as it is provided by the caller.
 *                                                                      \lb\lb
 *                  This function will transmit the packet regardless of
 *                  spanning tree state.
 *
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portList points to an array of logical port numbers to
 *                  which the switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 * 
 * \param[in]       info is a pointer to the packet information structure, which
 *                  contains additional information describing the packet.
 *                  See ''fm_packetInfoV2'' for details.
 *                  Note: all fields in the information structure must be
 *                  initialized. Any unused fields should be set to zero.
 *                  Failure to initialize all fields can result in the packet
 *                  being mishandled.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch number is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is down.
 * \return          FM_ERR_INVALID_PORT_STATE if the ports contained in the 
 *                  portList are not in the proper port state.
 * \return          FM_ERR_INVALID_PORT if portList contains an invalid port 
 *                  number.
 * \return          FM_ERR_INVALID_ARGUMENT if portList is NULL, 
 *                  numPorts is not positive, or pkt is not a valid packet 
 *                  buffer.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 *
 *****************************************************************************/
fm_status fmSendPacketDirectedV2(fm_int           sw, 
                                 fm_int *         portList, 
                                 fm_int           numPorts,
                                 fm_buffer *      pkt,
                                 fm_packetInfoV2 *info)
{
    fm_status   err;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX,
                     "sw=%d portList=%p numPort=%d pkt=%p\n",
                     sw, (void *)portList, numPorts, (void *) pkt);

    if ( (numPorts <= 0) || (portList == NULL) || (pkt == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SendPacketDirected,
                       sw,
                       portList,
                       numPorts,
                       pkt,
                       info);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketDirectedV2 */




/*****************************************************************************/
/** fmSendPacketSwitched
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Send a message packet to the switch in switched mode, where
 *                  the switch determines the destination port(s) for the
 *                  packet based on the packet payload.
 *                                                                      \lb\lb
 *                  The packet data is passed in an ''fm_buffer'' chain with 
 *                  the data appearing in network byte order. See ''fm_buffer'' 
 *                  for more information on how packet data appears in the 
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word and the 
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain should not include the CRC. 
 *                                                                      \lb\lb
 *                  If the packet data does not include a VLAN tag, the packet
 *                  will be transmitted through the switch on the CPU port's
 *                  default VLAN.
 *                                                                      \lb\lb
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BAD_IOCTL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_ERR_INVALID_ARGUMENT if pkt is not a valid packet buffer.
 * \return          FM_ERR_INVALID_PORT_STATE if the port is down.
 * \return          FM_ERR_INVALID_SWITCH if the switch number is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is down.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_FAIL if the network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmSendPacketSwitched(fm_int sw, fm_buffer *pkt)
{
    fm_status   err;
    fm_switch * switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX, "sw=%d pkt=%p\n", sw, (void *) pkt);

    if ( pkt == NULL )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, switchPtr->SendPacketSwitched, sw, pkt);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketSwitched */




/*****************************************************************************/
/** fmSendPacketSwitchedOnward
 * \ingroup pkt
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Send a message packet to the switch in switched mode, but
 *                  on a specified VLAN. This function is used for packets 
 *                  that were trapped to the CPU but must be forwarded back 
 *                  out the switch on a particular VLAN.
 *                                                                      \lb\lb
 *                  The packet data is passed in an ''fm_buffer'' chain with 
 *                  the data appearing in network byte order. See ''fm_buffer'' 
 *                  for more information on how packet data appears in the 
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word and the 
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain should not include the CRC. 
 *                                                                      \lb\lb
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pkt points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \param[in]       sourcePort is the logical port indicating from where the 
 *                  packet originated and need not necessarily be the
 *                  CPU port. 
 *
 * \param[in]       vlan is the VLAN on which to switch the packet.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_STATE if switch is down.
 * \return          FM_ERR_INVALID_ARGUMENT if pkt is not a valid packet buffer.
 * \return          FM_ERR_BAD_IOCTL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 *
 *****************************************************************************/
fm_status fmSendPacketSwitchedOnward(fm_int sw, 
                                     fm_int sourcePort, 
                                     fm_int vlan, 
                                     fm_buffer *pkt)
{
    fm_status       err;
    fm_packetInfo   info;

    FM_LOG_ENTRY_API(FM_LOG_CAT_EVENT_PKT_TX, 
                     "sw=%d sourcePort = %d pkt=%p\n", 
                     sw, 
                     sourcePort, 
                     (void *) pkt);

    if ( pkt == NULL )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    VALIDATE_LOGICAL_PORT(sw, sourcePort, (ALLOW_CPU | ALLOW_LAG));

    memset(&info, 0, sizeof(info) );

    info.sourcePort = sourcePort;
    info.useEgressRules = TRUE;
    info.vlanId = vlan;

    err = fmSendPacketExt(sw, &info, pkt);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketSwitchedOnward */




/*****************************************************************************/
/** fmSendPacketISL
 * \ingroup lowlevPkt
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Send a message packet to the switch with custom ISL
 *                  tags.
 *                                                                      \lb\lb
 *                  The packet data is passed in an ''fm_buffer'' chain with 
 *                  the data appearing in network byte order. See ''fm_buffer'' 
 *                  for more information on how packet data appears in the 
 *                  ''fm_buffer'' chain.
 *                                                                      \lb\lb
 *                  The packet data should not include a CRC word and the 
 *                  length of the packet, as specified in the ''fm_buffer''
 *                  chain should not include the CRC nor the ISL. 
 *                                                                      \lb\lb
 * \note            If this function returns FM_OK, ownership of the
 *                  ''fm_buffer'' chain containing the message packet
 *                  has been successfully transferred to the API. The caller
 *                  should no longer access the message packet upon return
 *                  from this function. However, if this function returns
 *                  any other status code, ownership of the ''fm_buffer''
 *                  chain remains with the caller, who is responsible for
 *                  disposing of the chain appropriately.
 *
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
 * \return          FM_ERR_INVALID_ARGUMENT if pkt is not a valid packet buffer
 *                  or if the islTag is not valid or the islTagFormat invalid.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue
 *                  is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 *
 *****************************************************************************/
fm_status fmSendPacketISL(fm_int          sw,
                          fm_uint32 *     islTag,
                          fm_islTagFormat islTagFormat,
                          fm_buffer *     pkt)
{
    fm_status  err;
    fm_switch *switchPtr;
    
    FM_LOG_ENTRY_API( FM_LOG_CAT_EVENT_PKT_TX,
                      "sw=%d islTag = %p islTagFormat = %d pkt=%p\n",
                      sw,
                      (void *) islTag,
                      islTagFormat,
                      (void *) pkt );

    if ( (pkt == NULL) || (islTag == NULL) || (islTagFormat <= 0) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, 
                       switchPtr->SendPacketISL, 
                       sw, 
                       islTag, 
                       islTagFormat, 
                       pkt);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmSendPacketISL */

