/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_generic_tx.c
 * Creation Date:   April 30th, 2013
 * Description:     Generic packet sending code.
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

/* Some platforms might not define this */
#ifndef FM_PACKET_OFFSET_ETHERTYPE
#define FM_PACKET_OFFSET_ETHERTYPE 3
#endif

#define MAX_VLAN_ETHER_TYPES        FM10000_PARSER_VLAN_TAG_ENTRIES /* 4 */
#define VLAN_TAG_0_BIT              (1 << 0)
#define VLAN_TAG_1_BIT              (1 << 1)
#define VLAN_TAG_2_BIT              (1 << 2)
#define VLAN_TAG_3_BIT              (1 << 3)

#define VLAN1_TAG                   0
#define VLAN2_TAG                   1


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
/** fm10000GetTrapGlort
 * \ingroup intPlatformCommon
 *
 * \desc            Return the stag type
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      trapGlort is the pointer to hold trap glort.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status fm10000GetTrapGlort(fm_int     sw,
                                     fm_uint32 *trapGlort)
{
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* This must read from cached for fibm enabled switches */
    return switchPtr->ReadUINT32(sw, FM10000_TRAP_GLORT(), trapGlort);

}   /* end fm10000GetTrapGlort */




/*****************************************************************************/
/** fm10000GetVlanTypes
 * \ingroup intPlatformCommon
 *
 * \desc            Return the stag type
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       port is the physical port to operate on.
 * 
 * \param[in]       vlanType should be set to VLAN1_TAG to get the VLAN1 types
 *                  or to VLAN2_TAG to get the VLAN2 types.
 *
 * \param[out]      etherTypes is a pointer to an array of supported
 *                  etherTypes where they should be stored. The array must
 *                  be of size MAX_VLAN_ETHER_TYPES.
 *
 * \param[out]      numEtherTypes is the pointer to caller allocated storage
 *                  where the number of valid ether types set in etherTypes
 *                  should be stored. (Maxmimum MAX_VLAN_ETHER_TYPES).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status fm10000GetVlanTypes(fm_int     sw,
                                     fm_int     port,
                                     fm_bool    vlanType,
                                     fm_uint32 *etherTypes,
                                     fm_uint32 *numEtherTypes)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  rv32;
    fm_uint64  rv64;
    fm_uint32  enabledTags;
    fm_uint32  index;

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT64(sw, 
                                FM10000_PARSER_PORT_CFG_1(port, 0), 
                                &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    if (vlanType == VLAN1_TAG)
    {
        enabledTags = FM_GET_FIELD64(rv64, FM10000_PARSER_PORT_CFG_1, Vlan1Tag);
    }
    else
    {
        enabledTags = FM_GET_FIELD64(rv64, FM10000_PARSER_PORT_CFG_1, Vlan2Tag);
    }

    index = 0;

    if (enabledTags & VLAN_TAG_0_BIT )
    {
        err = switchPtr->ReadUINT32(sw, FM10000_PARSER_VLAN_TAG(0), &rv32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        etherTypes[index++] = rv32;
    }

    if (enabledTags & VLAN_TAG_1_BIT )
    {
        err = switchPtr->ReadUINT32(sw, FM10000_PARSER_VLAN_TAG(1), &rv32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        etherTypes[index++] = rv32;
    }

    if (enabledTags & VLAN_TAG_2_BIT )
    {
        err = switchPtr->ReadUINT32(sw, FM10000_PARSER_VLAN_TAG(2), &rv32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        etherTypes[index++] = rv32;
    }

    if (enabledTags & VLAN_TAG_3_BIT )
    {
        err = switchPtr->ReadUINT32(sw, FM10000_PARSER_VLAN_TAG(3), &rv32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        etherTypes[index++] = rv32;
    }

    *numEtherTypes = index;

ABORT:
    if (err != FM_OK)
    {
        *numEtherTypes = 0;
    }

    return err;

} /* end fm10000GetVlanTypes */








/*****************************************************************************/
/** fm10000ParseVlanTags
 * \ingroup intPlatformCommon
 *
 * \desc            Parse the frame's vlan tags (if any) and return their
 *                  contents.
 * 
 * \note            This function only parses VLAN1 at the outer vlan tag
 *                  position. See bugzilla #21942 for details. 
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port used for parsing the vlan
 *                  ethertypes. 
 *
 * \param[in]       packet is the packet buffer.
 *
 * \param[in]       vlanTag1 is the caller allocated storage where the vlanTag1
 *                  should be stored (will be -1 if not vlan1 tagged). Includes
 *                  the vlan priority.
 *                   
 * \param[out]      vlanTag2 is the caller allocated storage where the vlanTag2
 *                  should be stored (will be -1 if not vlan2 tagged). Includes
 *                  the vlan priority.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if sw or port are invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_SWITCH if switch is invalid for SWAG.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status fm10000ParseVlanTags(fm_int     sw,
                                      fm_int     port, 
                                      fm_buffer *packet,
                                      fm_int *   vlanTag1,
                                      fm_int *   vlanTag2)
{
    fm_status     err;
    fm_switch *   switchPtr;
    fm_int        switchNum;
    fm_int        physPort;
    fm_uint32     outerHeader;
    fm_uint16     outerEtherType;
    fm_uint32     i;
    fm_uint32     vlan1EtherTypes[MAX_VLAN_ETHER_TYPES];
    fm_uint32     numVlan1EtherTypes;
    fm_bool       outerMatchVlan1;
    fm_bool       regLockTaken;

    regLockTaken = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    outerMatchVlan1    = FALSE;
    numVlan1EtherTypes = 0;
    
    err = fmPlatformMapLogicalPortToPhysical(sw,
                                             port,
                                             &switchNum,
                                             &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Get Vlan1 EtherTypes */
    err = fm10000GetVlanTypes(sw, 
                              physPort, 
                              VLAN1_TAG, 
                              vlan1EtherTypes, 
                              &numVlan1EtherTypes);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    FM_FLAG_DROP_REG_LOCK(sw);

    /* Look for outer vlan */
    outerHeader     = ntohl(packet->data[FM_PACKET_OFFSET_ETHERTYPE]);
    outerEtherType  = (outerHeader >> 16) & 0xffff;

    for (i = 0; i < numVlan1EtherTypes; i++)
    {
        if (outerEtherType == vlan1EtherTypes[i])
        {
            outerMatchVlan1 = TRUE;
            break;
        }
    }

    /* Frame contains vlan1 header */
    if (outerMatchVlan1)
    {
        *vlanTag1 = outerHeader & 0xFFFF;
        *vlanTag2 = -1;
    }
    else 
    {
        *vlanTag1 = -1;
        *vlanTag2 = -1;
    }

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    return err;

} /* fm10000ParseVlanTags */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000GenericSendPacketDirected
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       portList points to an array of logical port numbers the
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
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
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GenericSendPacketDirected(fm_int           sw,
                                           fm_int *         portList,
                                           fm_int           numPorts,
                                           fm_buffer *      packet,
                                           fm_packetInfoV2 *info)
{
    fm_status   err;
    fm_int      cpuPort;
    fm_uint32   fcsValue;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "portList = %p, "
                 "numPorts = %d, "
                 "packet->index = 0x%x\n",
                 sw,
                 (void *) portList,
                 numPorts,
                 packet->index);

    switch (info->fcsMode)
    {
        case FM_FCS_MODE_DEFAULT:
        case FM_FCS_MODE_VALUE:
            fcsValue = info->fcsValue;
            break;

        case FM_FCS_MODE_ZERO:
            fcsValue = 0;
            break;

        case FM_FCS_MODE_TIMESTAMP:
            fcsValue = 0x00000080;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmGetCpuPortInt(sw, &cpuPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    /* Call the shared code */
    err = fmGenericSendPacketDirected(sw, portList, numPorts,
                                      packet, fcsValue,
                                      cpuPort, info->switchPriority);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fm10000GenericSendPacketDirected */




/*****************************************************************************/
/** fm10000GenericSendPacketSwitched
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if packet is not valid packet buffer
 * \return          FM_ERR_INVALID_PORT if port is not valid
 * \return          FM_ERR_INVALID_SWITCH if sw is not valid
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GenericSendPacketSwitched(fm_int sw, fm_buffer *packet)
{
    fm_status               err = FM_OK;
    fm_int                  cpuPort;
    
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "packet->index = 0x%x\n",
                 sw,
                 packet->index);

    err = fmGetCpuPortInt(sw, &cpuPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    /* Call the shared code */
    err = fmGenericSendPacketSwitched(sw, 
                                      packet, 
                                      cpuPort, 
                                      FM_USE_VLAN_PRIORITY);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fm10000GenericSendPacketSwitched */




/*****************************************************************************/
/** fm10000GenericSendPacketISL
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add the given packet to the internal packet
 *                  queue in the lookup packet send mode.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       islTag points to the ISL tag contents
 *
 * \param[in]       islTagFormat is the ISL tag format value.
 *
 * \param[in]       buffer points to a chain of fm_buffer structures
 *                  containing the payload.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GenericSendPacketISL(fm_int          sw,
                                      fm_uint32 *     islTag,
                                      fm_islTagFormat islTagFormat,
                                      fm_buffer *     buffer)
{
    fm_status       err;
    fm_islTag       tag;
    
    FM_LOG_ENTRY( FM_LOG_CAT_EVENT_PKT_TX,
                  "sw=%d islTag = %p islTagFormat = %d buffer=%p\n",
                  sw,
                  (void *) islTag,
                  islTagFormat,
                  (void *) buffer );

    /* Validate that the ISL tag format is F56 */
    if (islTagFormat != FM_ISL_TAG_F56)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    FM_CLEAR(tag);

    tag.f56.tag[0] = islTag[0];
    tag.f56.tag[1] = islTag[1];
        
    err = fmGenericSendPacketISL(sw, &tag, islTagFormat, 1, buffer);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000GenericSendPacketISL */




/*****************************************************************************/
/** fm10000GeneratePacketISL
 * \ingroup intPlatformCommon
 *
 * \desc            Generate packet ISL words and suppressVlanTag flag.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       buffer is the packet buffer.
 *
 * \param[in]       info is the packet info data.
 *
 * \param[in]       cpuPort contains the logical port number for the CPU port. 
 *
 * \param[in]       switchPriority is the switch priority.
 * 
 * \param[out]      islTagFormat is a pointer to the caller allocated storage
 *                  where the ISL tag type should be stored
 * 
 * \param[out]      islTag is a pointer to the caller allocated storage
 *                  where the ISL tag data should be stored.
 * 
 * \param[out]      suppressVlanTag is a pointer to the caller allocated
 *                  storage the suppressVlanTag flag should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_SWITCH if switch is invalid for SWAG.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GeneratePacketISL(fm_int           sw,
                                   fm_buffer       *buffer,
                                   fm_packetInfo   *info,
                                   fm_int           cpuPort,
                                   fm_uint32        switchPriority,
                                   fm_islTagFormat *islTagFormat,
                                   fm_islTag       *islTag,
                                   fm_bool         *suppressVlanTag)
{
    fm_status     err;
    fm_int        vlanTag1;
    fm_int        vlanTag2;
    
    /* ISL Tag fields */
    fm_byte       ftype;
    fm_byte       vtype;
    fm_byte       swpri;
    fm_byte       user;
    fm_uint16     vpriVlan;
    fm_uint32     sglort;
    fm_uint32     dglort;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "info->logicalPort = %d, "
                 "info->sourcePort = %d, "
                 "info->vlanId = %d, "
                 "info->vlanPriority = %d, "
                 "info->switchPriority = %d, "
                 "info->useEgressRules = %s, "
                 "info->directSendToCpu = %s\n",
                 sw,
                 info->logicalPort,
                 info->sourcePort,
                 info->vlanId,
                 info->vlanPriority,
                 info->switchPriority,
                 FM_BOOLSTRING(info->useEgressRules),
                 FM_BOOLSTRING(info->directSendToCpu));

    *suppressVlanTag = 0;

    /*****************************************
     * 1. Define the ftype and dglort fields
     *****************************************/

    if ( (info->logicalPort == FM_LOG_PORT_USE_FTYPE_NORMAL) &&
         (!info->directSendToCpu) )
    {
        /* The frame will be switched */
        ftype  = FM_FTYPE_NORMAL;
        dglort = 0;
    }
    else
    {
        /* The frame will be directed */
        ftype = FM_FTYPE_SPECIAL_DELIVERY;

        err = fmGetLogicalPortGlort(sw,
                                    info->logicalPort,
                                    &dglort);            
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    /*****************************************
     * 2. Parse the vlan-tags supported
     *****************************************/

    vlanTag1 = -1;
    vlanTag2 = -1;
    err = fm10000ParseVlanTags(sw, cpuPort, buffer, &vlanTag1, &vlanTag2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    
    /*****************************************
     * 3. Keep this comment to have the same 
     *    sequence as other families 
     *****************************************/


    /*****************************************
     * 4. Check if we have a valid vlan tag.
     *****************************************/

    vpriVlan = 0;

    if (vlanTag1 != -1)
    {
        vpriVlan = vlanTag1;

        /* For FM10000, the vlan from the frame needs to be stripped,
         * if the frame is of type NORMAL. */
        if (ftype == FM_FTYPE_NORMAL)
        {
            /***********************************************
             * We need to skip sending the vlan tag in the
             * payload since the switch will add a VLAN tag
             * using the info from the ISL tag.
             ***********************************************/
            *suppressVlanTag = 1;
        }
    }

    /*****************************************
     * 5. Check if we should use egress rules.
     *****************************************/

    /* For tagged frame, use the vlan info carried
     * in the frame. Do not use packet->info.vlanId and
     * packet->info.vlanPriority. */
    if (info->useEgressRules && (vlanTag1 != -1))
    {
        if (info->vlanId == 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX,
                         "the user application needs to provide a "
                         "vlanId in packet info structure\n");
        }
        else
        {
            vpriVlan = info->vlanId;
            vpriVlan |= (info->vlanPriority << 13);
        }
    }
    else if ((vlanTag1 == -1))
    {
        /* For an untagged frame use the pvid of port 0 */
        err = fmGetPortDefVlanInt(sw, cpuPort, &vpriVlan);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    /*****************************************
     * 6. Define the swpri fields
     *****************************************/
    if (switchPriority != FM_USE_VLAN_PRIORITY)
    {
        swpri = switchPriority & 0xf;
    }
    else if (info->switchPriority != FM_USE_VLAN_PRIORITY)
    {
        swpri = (info->switchPriority & 0xf);
    }
    else
    {
        swpri = ( (vpriVlan >> 12) & 0xf );
    }

    /*****************************************
     * 7. Set the vtype for FM10000
     *****************************************/
    vtype = 0;
    
    /*****************************************
     * 8. Force user field to 0
     *****************************************/
    user = 0;

    /*****************************************
     * 9. Define the sglort
     *****************************************/

    /* If requested, force the source glort to zero */
    if (info->zeroSourceGlort)
    {
        sglort = 0;
    }
    /* else use the trap glort (cpu glort) as the source glort
     *  for send? */
    else if (info->sourcePort == 0)
    {
        /* User does not provide the source glort. Use default.*/
        err = fm10000GetTrapGlort(sw, &sglort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }
    else
    {
        err = fmGetLogicalPortGlort(sw, info->sourcePort, &sglort);
        if (err != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                         "fmGeneratePacketIsl: Invalid sourcePort %d\n",
                         info->sourcePort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
        }
    }

    /*****************************************
     * 10. Finally, build the ISL header
     *****************************************/

    /* FM10000 only supports F56 */
    *islTagFormat = FM_ISL_TAG_F56;

    /* For frames going through PCIe, the user field is supported */
    islTag->f56.tag[0] = ( (user     & FM_F56_USER_MASK)    << FM_F56_USER_POS)  |
                         ( (ftype    & FM_F56_FTYPE_MASK)   << FM_F56_FTYPE_POS) |
                         ( (vtype    & FM_F56_VTYPE_MASK)   << FM_F56_VTYPE_POS) |
                         ( (swpri    & FM_F56_SWPRI_MASK)   << FM_F56_SWPRI_POS) |
                         ( (vpriVlan & FM_F56_VPRIVLAN_MASK << FM_F56_VPRIVLAN_POS));

    islTag->f56.tag[1] = ( (sglort   & FM_F56_SGLORT_MASK)  << FM_F56_SGLORT_POS) |
                         ( (dglort   & FM_F56_DGLORT_MASK)  << FM_F56_DGLORT_POS);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                 "ISL Tag: [0] = %X, [1] = %X\n",
                 islTag->f56.tag[0],
                 islTag->f56.tag[1]);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
    
}   /* end fm10000GeneratePacketISL */

