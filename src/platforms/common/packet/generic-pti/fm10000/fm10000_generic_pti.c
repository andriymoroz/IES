/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_generic_pti.c
 * Creation Date:   July 25, 2014
 * Description:     Generic PTI (Packet Test Interface) send and receive for 
 *                  the FM10000 series
 *
 * Copyright (c) 2014, Intel Corporation
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
#include <fm_sdk_fm10000_int.h>
#include <platforms/common/packet/generic-pti/fm10000/fm10000_generic_pti.h>
#include <common/fm_crc32.h>

/******************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Use longer delays for simulation */
#ifdef FV_CODE
#define DELAY_TIME_SECONDS              20
#else
#define DELAY_TIME_SECONDS              1
#endif

#define PORT_MASK_ALL_PORTS             0xffffffffffff
#define SAF_EOF                         0x3
#define FIBM_MUX_SELECT_PTI             0x1
#define BITS_PER_BYTE                   8
#define MARK_EOF                        0x3
#define INFO_ERR_NONE                   0x0
#define INFO_ERR_FCS                    0x1
#define INFO_ERR_FRAMING                0x2
#define INFO_ERR_INTERNAL               0x3
#define RECV_BUFFER_THRESHOLD           4
#define ABSOLUTE_MAX_MTU_BYTES          16384

/******************************************************************************
 * Local function prototypes
 *****************************************************************************/
static fm_uint32 ReverseBytes(fm_uint32 word);
static fm_status DumpPacket(fm_uint64 cat, fm_byte *data, fm_int length);
static fm_status AppendCRC32(fm_byte* data, fm_int pktSize, fm_bool f56Tagged);



/******************************************************************************
 * Global Variables
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** ReverseBytes
 * \ingroup intPlatformCommon
 *
 * \desc            Reverses bytes in a 32-bit word such that the MSB becomes
 *                  the LSB.  Bits within bytes are not changed.
 *
 * \param[in]       word is the 32-bit word to reverse bytes for
 *
 * \return          The reversed 32-bit word
 *
 *****************************************************************************/
static fm_uint32 ReverseBytes(fm_uint32 word)
{
    fm_uint         i;
    fm_uint32       reversed = 0;
    fm_uint32       mask = 0x000000ff;
    fm_byte         by;

    for ( i = 0 ; i < sizeof(word) ; i++)
    {
        mask        <<= BITS_PER_BYTE;
        by          = (word & mask) >> (i * BITS_PER_BYTE);
        reversed    |= (by << ((sizeof(word) - 1 - i) * BITS_PER_BYTE));
    }

    return reversed;
}   /* end ReverseBytes */




/*****************************************************************************/
/** DumpPacket
 * \ingroup intPlatformCommon
 *
 * \desc            Displays packet content.
 *
 * \param[in]       cat is the logging category bit mask.
 *
 * \param[in]       data points to packet data bytes.
 *
 * \param[in]       length is is the number of bytes in the packet data.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DumpPacket(fm_uint64 cat, fm_byte *data, fm_int length)
{
    fm_int              i;
    fm_char             str[64] = "";
    fm_char             tmp[4];

    /* Display the packet */
    FM_LOG_DEBUG(cat, "   %d byte packet:\n", length);
    for (i = 0 ; i < length ; i++)
    {
        if ((i % 16) == 0)
        {
            FM_STRCAT_S(str, sizeof(str), "   ");
        }

        FM_SPRINTF_S(tmp, sizeof(tmp), "%02x ", data[i]);
        FM_STRCAT_S(str, sizeof(str), tmp);
 
        if ((i % 16) == 15)
        {
            FM_LOG_DEBUG(cat, "%s\n", str);
            FM_CLEAR(str);
        }
    }

    if ((length % 16) != 0)
    {
        FM_LOG_DEBUG(cat, "%s\n", str);
    }

    return FM_OK;
}   /* DumpPacket */




/*****************************************************************************/
/** AppendCRC32
 * \ingroup intPlatformCommon
 *
 * \desc            Compute and append the 32-bit CRC (FCS).
 *
 * \param[in]       data is a pointer to an array of frame data bytes.
 *
 * \param[in]       pktSize is the size of the packet including space to
 *                  place the FCS and any F56 tag.
 *
 * \param[in]       f56Tagged is whether the packet data has an F56 tag
 *                  that will be ommitted from FCS calculation.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AppendCRC32(fm_byte* data, fm_int pktSize, fm_bool f56Tagged)
{
    fm_uint32   fcs;
    fm_byte*    dataStart;
    fm_int      fcsSize     = (pktSize - 4);    /* -4 for FCS */
    fm_int      x;

    dataStart = data;

    if (f56Tagged)
    {
        dataStart += 8;     /* 8B F56 */
        fcsSize   -= 8;     /* 8B F56 */
    }

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "data=%p pktSize=%d f56Tagged=%s "
                 "dataStart=%p fcsSize=%d\n",
                 data, pktSize, 
                 ((f56Tagged == TRUE) ? "TRUE" : "FALSE"),
                 dataStart, fcsSize);

    /* Compute the FCS */
    fcs = fmCrc32(dataStart, fcsSize);

    /* Fill the FCS in the last 4B of data */
    dataStart = data + pktSize - 4;
    for (x = 0 ; x < 4 ; x++)
    {
        *dataStart = (fcs >> (8 * x)) & 0xFF;
        dataStart++;
    }

    return FM_OK;
}   /* end AppendCRC32 */




/******************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000PTIInitialize
 * \ingroup intPlatformCommon
 *
 * \desc            Initialize Packet Test Interface.  
 *
 * \note            Initialization will change the FIBM port functionality
 *                  to be used for PTI.  Additionally, the FIBM port will
 *                  be F56 tagged.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_LOCK_UNINITIALIZED if the switch lock does not
 *                  exist.
 *
 * \return          FM_ERR_INVALID_PORT if the FIBM port is not mapped in the
 *                  scheduler.
 *
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PTIInitialize(fm_int sw)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_int          cpi;
    fm_int          portNumber;
    fm_int          fabricPort;
    fm_int          fibmLogicalPort = -1;
    fm_uint32       rv32;
    static fm_bool  threadCreated = FALSE;
    fm_port *       portPtr;
    fm_uint32       f56Format = FM_ISL_TAG_F56;
    fm_bool         safMode = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d threadCreated=%d\n", sw, threadCreated);

    if ( !SWITCH_LOCK_EXISTS(sw) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Switch lock does not exist\n");
        return FM_ERR_LOCK_UNINITIALIZED;
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    LOCK_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Initialize the generic packet handling interface */
    err = fmGenericPacketHandlingInitializeV2(sw, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
  

    /**************************************************
     * Initialize PTI
     **************************************************/


    /**************************************************
     * Find the FIBM port
     **************************************************/

    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        /* Get the logical port for this cardinal port */
        portNumber = GET_LOGICAL_PORT(sw, cpi);

        err = fm10000MapLogicalPortToFabricPort(sw, portNumber, &fabricPort);
        if (err == FM_ERR_INVALID_PORT)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                         "Skipping unmapped logical port %d (cpi=%d)\n", 
                         portNumber, 
                         cpi);
            continue;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        if (fabricPort == FM10000_FIBM_TO_FABRIC_PORT)
        {
            fibmLogicalPort = portNumber;
            break;
        }
    }

    /* If FIBM port was not found, it's not mapped */
    if (fibmLogicalPort == -1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     "FIBM port was not found in the port map\n");
        err = FM_ERR_INVALID_PORT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }
    
    /**************************************************
     * Configure FIBM port as PTI port type so that it
     * can be added to VLANs/etc.
     **************************************************/
    
    portPtr             = switchPtr->portTable[fibmLogicalPort];
    portPtr->portType   = FM_PORT_TYPE_PTI;
    portPtr->linkUp     = TRUE;

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                 "fibmLogicalPort=%d portType=%d\n",
                 fibmLogicalPort, portPtr->portType);


    /**************************************************
     * Setup Store and Forward from PTI (FIBM) to the
     * rest of the system
     **************************************************/

    err = fmSetPortAttribute(sw, 
                             fibmLogicalPort, 
                             FM_PORT_RX_CUT_THROUGH, 
                             &safMode);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fmSetPortAttribute(sw, 
                             fibmLogicalPort, 
                             FM_PORT_TX_CUT_THROUGH, 
                             &safMode);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);


    /**************************************************
     * Enable PTI
     **************************************************/

    err = switchPtr->ReadUINT32(sw, FM10000_FIBM_CFG(), &rv32);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    FM_SET_BIT(rv32, FM10000_FIBM_CFG, muxSelect, FIBM_MUX_SELECT_PTI);
    
    err = switchPtr->WriteUINT32(sw, FM10000_FIBM_CFG(), rv32);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);


    /**************************************************
     * Don't drain frames from PTI continuously
     **************************************************/

    err = switchPtr->ReadUINT32(sw, FM10000_PTI_RX_CTRL(), &rv32);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv32, FM10000_PTI_RX_CTRL, ContinuousDrain, 0);

    err = switchPtr->WriteUINT32(sw, FM10000_PTI_RX_CTRL(), rv32);


    /**************************************************
     * F56 tag the FIBM port 
     **************************************************/

    err = fmSetPortAttribute(sw, 
                             fibmLogicalPort, 
                             FM_PORT_ISL_TAG_FORMAT, 
                             &f56Format);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);


    /**************************************************
     * Create the receive packet thread 
     **************************************************/

    /* Ensure receive thread creation only happens once. */
    if (threadCreated == FALSE)
    {
        err = fmCreateThread("pti_packet_receive",
                             FM_EVENT_QUEUE_SIZE_NONE,
                             &fm10000PTIReceivePackets,
                             &(GET_PLAT_STATE(sw)->sw),
                             &fmRootPlatform->ptiThread);
        if (err == FM_OK)
        {
            threadCreated = TRUE;
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                     "sw=%d threadCreated=%d: Skipping thread creation\n", 
                     sw, 
                     threadCreated);
    }


ABORT:
    
    UNLOCK_SWITCH(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
}   /* fm10000PTIInitialize */




/*****************************************************************************/
/** fm10000PTISendPackets
 * \ingroup intPlatformCommon
 *
 * \desc            Send packets via the PTI interface.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PTISendPackets(fm_int sw)
{
    fm_status                   err = FM_OK;
    fm_switch *                 switchPtr;
    fm_packetHandlingState *    pktState;
    fm_packetQueue *            txQueue;
    fm_packetEntry *            pkt;
    fm_buffer *                 buffer;
    fm_byte *                   data = NULL;
    fm_int                      curWord;
    fm_int                      curB;
    fm_int                      lenB;
    fm_int                      i;
    fm_int                      pktLength;
    fm_uint                     mask;
    fm_int                      shiftBits;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw=%d\n", sw);

    switchPtr   = GET_SWITCH_PTR(sw);
    pktState    = GET_PLAT_PKT_STATE(sw);
    txQueue     = &pktState->txQueue;

    fmPacketQueueLock(txQueue);

    /**************************************************
     * Iterate through the packets in the tx queue
     **************************************************/

    for ( ; 
         txQueue->pullIndex != txQueue->pushIndex ;
         txQueue->pullIndex = (txQueue->pullIndex + 1) % FM_PACKET_QUEUE_SIZE)
    {
        pkt = &txQueue->packetQueueList[txQueue->pullIndex];

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "Sending packet in slot %d, length=%d, "
                     "suppressVlanTag=%d, fcsVal=0x%08x\n",
                     txQueue->pullIndex,
                     pkt->length,
                     pkt->suppressVlanTag,
                     pkt->fcsVal);

        pktLength = pkt->length + 4;    /* +4 = FCS */
        if (pkt->islTagFormat == FM_ISL_TAG_F56) 
        {
            pktLength += 8;
        }

        if (pkt->suppressVlanTag)
        {
            pktLength -= 4;
        }

        data = (fm_byte*) fmAlloc(pktLength);
        if (data == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
        }

        lenB    = 0;
        curWord = 0;
        curB    = 0;
        if (pkt->islTagFormat == FM_ISL_TAG_F56)
        {
            data[curB++] = (pkt->islTag.f56.tag[0] & 0xFF000000) >> 24;
            data[curB++] = (pkt->islTag.f56.tag[0] & 0x00FF0000) >> 16;
            data[curB++] = (pkt->islTag.f56.tag[0] & 0x0000FF00) >> 8;
            data[curB++] = (pkt->islTag.f56.tag[0] & 0x000000FF);

            data[curB++] = (pkt->islTag.f56.tag[1] & 0xFF000000) >> 24;
            data[curB++] = (pkt->islTag.f56.tag[1] & 0x00FF0000) >> 16;
            data[curB++] = (pkt->islTag.f56.tag[1] & 0x0000FF00) >> 8;
            data[curB++] = (pkt->islTag.f56.tag[1] & 0x000000FF);

            lenB += 8;
        }

        
        /**************************************************
         * Iterate through all buffers in this packet
         **************************************************/

        buffer  = pkt->packet;
        while (buffer != NULL)
        {
            /* Cannot modify the send buffer, since the same buffer can be
             * used multiple times to send to multiple ports */
         
            /* 32-bits at a time */
            fm_int curBufB = 0;
            for (i = 0 ; i < (buffer->len / 4) ; i++)
            {
                data[curB++] = (buffer->data[i] & 0x000000FF);
                data[curB++] = (buffer->data[i] & 0x0000FF00) >> 8;
                data[curB++] = (buffer->data[i] & 0x00FF0000) >> 16;
                data[curB++] = (buffer->data[i] & 0xFF000000) >> 24;
                curBufB += 4;
            }

            /* Remaining Bytes: 1 - 4 */
            mask = 0xFF;
            shiftBits = 0;
            while (curBufB < buffer->len)
            {
                data[curB++] = ((buffer->data[i] & mask) >> shiftBits);
                mask <<= 8;
                shiftBits += 8;
                curBufB++;
            }

            if (pkt->suppressVlanTag)
            {
                /* Suppress the vlan tag */

            }

            lenB += curBufB;
            buffer = buffer->next;
        }

        /* Calculate and append FCS */
        AppendCRC32(data, pktLength, 
                    (pkt->islTagFormat == FM_ISL_TAG_F56));
        lenB += 4;  /* FCS +4B */


        /**************************************************
         * Send the packet
         **************************************************/

        err = fm10000PTISend(sw, data, lenB);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_TX,
                         "Error sending packet: %d (%s)\n",
                         err, 
                         fmErrorMsg(err));
        }
        

        /**************************************************
         * Free packet buffer if this is the last packet
         * using this buffer
         **************************************************/

        if (pkt->freePacketBuffer)
        {
            /* Ignore the error since it's better to continue */
            (void) fmFreeBufferChain(sw, pkt->packet);
            
            fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_TX_BUFFER_FREES, 1);
        }
      
        fmFree(data);
        data = NULL;
    }

ABORT:
    fmPacketQueueUnlock(txQueue);
    if (data != NULL)
    {
        fmFree(data);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
}   /* end fm10000PTISendPackets */




/*****************************************************************************/
/** fm10000PTIReceivePackets
 * \ingroup intPlatformCommon
 *
 * \desc            Receive packets via the PTI interface.
 *
 * \param[in]       args is a pointer to the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void * fm10000PTIReceivePackets(void *args)
{
    fm_status       err = FM_OK;
    fm_thread *     thread;
    fm_int          sw;
    fm_buffer *     recvChainHead = NULL;
    fm_buffer *     nextBuffer = NULL;
    fm_switch *     switchPtr;
    fm_byte         data[ABSOLUTE_MAX_MTU_BYTES];
    fm_int          dataLength;
    fm_int          numBuffersNeeded;
    fm_int          availableBuffers;
    fm_int          i;
    fm_int          curByte;
    fm_int          lastBufferByte;
    fm_int          numBytesLeft;
    fm_int          curBufDataIdx;
    fm_int          byInWordNum;
    fm_int          curBufSize;

    thread  = FM_GET_THREAD_HANDLE(args);
    sw      = *(FM_GET_THREAD_PARAM(fm_int, args));

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_RX, 
                 "thread = %s, sw=%d\n", 
                 thread->name, 
                 sw);
    
    thread = FM_GET_THREAD_HANDLE(args);
   
    while (TRUE)
    {
        /* Ensure we don't hammer and hog the CPU */
        fmYield();

        /**************************************************
         * Ensure the switch is up 
         **************************************************/

        switchPtr = GET_SWITCH_PTR(sw);
        if (switchPtr == NULL)
        {
            fmDelay(DELAY_TIME_SECONDS, 0);
            continue;
        }

        if (switchPtr->state != FM_SWITCH_STATE_UP)
        {
            fmDelay(DELAY_TIME_SECONDS, 0);
            continue;
        }
        

        /**************************************************
         * Get the number of available buffers from the 
         * buffer manager.  
         **************************************************/

        err = fmPlatformGetAvailableBuffers(&availableBuffers);
        if (err != FM_OK)
        {
            FM_LOG_WARNING(FM_LOG_CAT_EVENT_PKT_RX,
                           "Failure to get available buffer count: %d\n",
                           err);

            fmDelay(DELAY_TIME_SECONDS, 0);
            continue;
        }

        if (availableBuffers <= RECV_BUFFER_THRESHOLD)
        {
            fmDelay(DELAY_TIME_SECONDS, 0);
            continue;
        }


        /**************************************************
         * Receive the packet via PTI
         * Blocking function call
         **************************************************/

        FM_CLEAR(data);
        dataLength = 0;
        err = fm10000PTIReceive(sw, data, sizeof(data), &dataLength);
        if (err == FM_ERR_SWITCH_NOT_UP)
        {
            /* Switch may have gone down while waiting for a packet to
             * arrive. */
            fmDelay(DELAY_TIME_SECONDS, 0);
            continue;
        }
        else if (err != FM_OK)
        {
            FM_LOG_WARNING(FM_LOG_CAT_EVENT_PKT_RX,
                           "Failure receiving packet: %d\n", 
                           err);
            continue;
        }

        if (dataLength == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                         "Received packet of size 0B\n");
            continue;
        }


        /**************************************************
         * At this point any continue statement will throw
         * away the received packet data.
         **************************************************/
    

        /**************************************************
         * Allocate a buffer chain for the packet
         **************************************************/
        
        curByte             = 0;
        numBytesLeft        = dataLength;
        recvChainHead       = NULL;
        numBuffersNeeded    = (dataLength / FM_BUFFER_SIZE_BYTES);
        if ((dataLength % FM_BUFFER_SIZE_BYTES) > 0)
        {
            numBuffersNeeded++;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX, 
                     "%d buffers are required for frame data\n", 
                     numBuffersNeeded);

        for (i = 0 ; i < numBuffersNeeded ; i++)
        {
            /* Get a buffer */
            do
            {
                nextBuffer = fmAllocateBuffer(sw);

                if (nextBuffer == NULL)
                {
                    fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_RX_OUT_OF_BUFFERS,
                                             1);

                    /* Wait for a buffer to return */
                    fmYield();
                }
            } while (nextBuffer == NULL);
            
            /* Fill data in network byte order */
            curBufSize      = (numBytesLeft < FM_BUFFER_SIZE_BYTES) ?
                              numBytesLeft : FM_BUFFER_SIZE_BYTES;

            lastBufferByte  = curByte + curBufSize;
            curBufDataIdx   = 0;
            byInWordNum     = 0;    /* range 0 - 3 */
            /* FM_CLEAR(*(nextBuffer->data)); */

            while (curByte < lastBufferByte)
            {
                if (byInWordNum == 0)
                {
                    nextBuffer->data[curBufDataIdx] = 0;
                }

                nextBuffer->data[curBufDataIdx] |= 
                    (data[curByte] << (8 * (3 - byInWordNum)));

                if (byInWordNum == 3)   /* last byte in 32-bit word */
                {
                    /* Convert to network byte order */
                    nextBuffer->data[curBufDataIdx] = 
                        htonl(nextBuffer->data[curBufDataIdx]);

                    curBufDataIdx++;
                }

                byInWordNum = (byInWordNum + 1) % 4;
                numBytesLeft--;
                curByte++;
            }
            nextBuffer->len     = curBufSize;
            nextBuffer->next    = NULL;

            /* Not a full word, convert to network byte order */
            if (byInWordNum != 0)
            {
                nextBuffer->data[curBufDataIdx] = 
                    htonl(nextBuffer->data[curBufDataIdx]);
            }

            /* Link the buffer */
            if (recvChainHead == NULL)
            {
                recvChainHead = nextBuffer;
            }
            else
            {
                err = fmAddBuffer(recvChainHead, nextBuffer);
                if (err != FM_OK)
                {
                    /* Safety.  This shouldn't happen as recvChainHead is
                     * checked for NULL before calling fmAddBuffer().
                     * Checking here protects against code changes. */
                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                                 "Unable to add packet buffer %d to chain: "
                                 "%s\n",
                                 i,
                                 fmErrorMsg(err));
                }
            }
        }   /* end for (i = 0 ; i < numBuffersNeeded ; i++) */

        /**************************************************
         * Pass the packet to the platform  
         * Don't provide an ISL tag pointer, instead 
         * letting the API handle the ISL tag information
         * included in the fm_buffer chain.
         **************************************************/

        if ( (recvChainHead == NULL) || (numBuffersNeeded == 0) )
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                         "Received a packet that needed no buffers\n");
            continue;
        }
        err = fmPlatformReceiveProcessV2(sw, recvChainHead, NULL, NULL);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                         "Returned error status %d (%s)\n",
                         err,
                         fmErrorMsg(err));
        }
        

        /**************************************************
         * Buffer chain has been consumed, reset pointers 
         **************************************************/

        recvChainHead   = NULL;
        nextBuffer      = NULL;
    }   /* end while (TRUE) */

    
    /**************************************************
     * Thread should never exit 
     **************************************************/
    FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                 "ERROR: Receive thread terminated inadvertently!\n");

    return NULL;
}   /* end fm10000PTIReceivePackets */

/*****************************************************************************/
/** fm10000PTISend
 * \ingroup intPlatformCommon
 *
 * \desc            Send a packet via Packet Test Interface
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       data points to an array of bytes to send.
 *
 * \param[in]       length is the number of bytes to send.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000PTISend(fm_int         sw, 
                         fm_byte        *data, 
                         fm_int         length)
{
    fm_status           err = FM_OK;
    fm_switch           *switchPtr;
    fm_int              numWords64b;
    fm_int              i;
    fm_int              curBy;
    fm_uint             rv32 = 0;
    fm_uint             word32;
    fm_int              mark = 0;
    fm_int              info = 0;
    fm_int              n = 8;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw=%d length=%d\n", sw, length);

    switchPtr = GET_SWITCH_PTR(sw);

    DumpPacket(FM_LOG_CAT_EVENT_PKT_TX, data, length);

    numWords64b = ((length % 8) == 0) ? (length / 8) : (length / 8) + 1;

    curBy = 0;
    for (i = 0 ; i < numWords64b ; i++)
    {
        /**************************************************
         * First 32-bits of data
         **************************************************/
        
        word32 = data[curBy] |
                 ((data[curBy + 1]) << 8) |
                 ((data[curBy + 2]) << 16) |
                 ((data[curBy + 3]) << 24);

        curBy += 4;
       
        FM_SET_FIELD(rv32, FM10000_PTI_TX_DATA0, Data, word32);
        err = switchPtr->WriteUINT32(sw, FM10000_PTI_TX_DATA0(), rv32);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);


        /**************************************************
         * Second 32-bits of data
         **************************************************/

        word32 = data[curBy] |
                 ((data[curBy + 1]) << 8) |
                 ((data[curBy + 2]) << 16) |
                 ((data[curBy + 3]) << 24);

        curBy += 4;

        FM_SET_FIELD(rv32, FM10000_PTI_TX_DATA1, Data, word32);
        err = switchPtr->WriteUINT32(sw, FM10000_PTI_TX_DATA1(), rv32);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
       

        /**************************************************
         * Last word to write
         **************************************************/
        
        if (i >= (numWords64b - 1))
        {
            n       = ((length & 0x7) != 0) ? (length & 0x7) : 8;
            mark    = 3;
        }


        /**************************************************
         * Start transmission of this 64-bits of data
         **************************************************/
        
        rv32 = 0;
        /* Could use Info field to generate error packets */
        FM_SET_FIELD(rv32, FM10000_PTI_TX_CTRL, Info, info);
        FM_SET_FIELD(rv32, FM10000_PTI_TX_CTRL, Mark, mark); 
        FM_SET_FIELD(rv32, FM10000_PTI_TX_CTRL, Len, n); 
        FM_SET_BIT(rv32, FM10000_PTI_TX_CTRL, TxValid, 1); 
        err = switchPtr->WriteUINT32(sw, FM10000_PTI_TX_CTRL(), rv32);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
      
        /**************************************************
         * Poll for TxValid == 0
         **************************************************/

        do
        {
            err = switchPtr->ReadUINT32(sw, FM10000_PTI_TX_CTRL(), &rv32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
        } while (FM_GET_BIT(rv32, FM10000_PTI_TX_CTRL, TxValid) == 1);
    }   /* end for (i = 0 ; i < numWords64b ; i++) */

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
}   /* end fm10000PTISend */




/*****************************************************************************/
/** fm10000PTIReceive
 * \ingroup intPlatformCommon
 *
 * \desc            Receive a single packet via Packet Test Interface.
 *
 * \note            This function is blocking.  It will return only when a
 *                  full packet has been received.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   data points to a caller allocated array to place packet 
 *                  data in.  This array should be large enough to store
 *                  an MTU frame.  The packet data is stored 1 byte per
 *                  index, with the F56 tag/DMAC stored in index 0.
 *
 * \param[in]       length is the size of the caller allocated data array.
 *
 * \param[out]      dataLength points to the number of bytes in the packet
 *                  data array.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_FAIL if there was an error handling the frame.  This
 *                  could be an FCS error, framing error, or other internal
 *                  error such as the source EPL/PCIe reset.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if data or dataLength is null.
 *
 * \return          FM_ERR_BUFFER_FULL if data array is too small for frame 
 *                  size.  
 *
 *
 *****************************************************************************/
fm_status fm10000PTIReceive(fm_int      sw, 
                            fm_byte *   data, 
                            fm_int      length,
                            fm_int *    dataLength)
{
    fm_status               err = FM_OK;
    fm_status               savedError = FM_OK;
    fm_switch *             switchPtr;
    fm_bool                 eof = FALSE;
    fm_uint                 ptiError = INFO_ERR_NONE;
    fm_int                  len;
    fm_uint                 rv32;
    fm_uint                 rv32_2;
    fm_int                  curBy = 0;
    fm_int                  i;
    fm_int                  blockingReadCount;
    fm_bool                 frameStarted = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_RX, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Check that pointers are valid
     **************************************************/

    if ( (data == NULL) || (dataLength == NULL) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX, 
                     "data or length argument is null");

        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);
    }

    
    /**************************************************
     * Initialize 
     **************************************************/

    *dataLength = 0;

    while (!eof)
    {
        /* Abort this loop only on catastrophic failures like register access 
         * failures.  Frame errors can be dealt with still flushing the rest
         * of the frame data from the wide port channel. */

        
        /**************************************************
         * Poll for RxValid == 1
         * This is blocking, and may be a separate thread.
         * Release the switch read lock occasionally to
         * allow exclusive write access to other threads
         * in case a frame doesn't arrive any time soon.
         **************************************************/

        blockingReadCount = 0;
        do
        {
            if (blockingReadCount == 0)
            {
                VALIDATE_AND_PROTECT_SWITCH(sw);
            }
            err = switchPtr->ReadUINT32(sw, FM10000_PTI_RX_CTRL(), &rv32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);

            /* Number of iterations before releasing switch lock 
             * If the frame has already started, keep trying to get frame
             * data until the frame has been completed */
            if ( (!frameStarted) && (blockingReadCount >= 20) )
            {
                UNPROTECT_SWITCH(sw);
                blockingReadCount = -1;
                /* Don't hammer the CPU or lock out other threads */
                fmDelay(5 * DELAY_TIME_SECONDS, 0);  
            }
            blockingReadCount++;
        } while (FM_GET_BIT(rv32, FM10000_PTI_RX_CTRL, RxValid) == 0);
        
        frameStarted = TRUE;
        
        VALIDATE_AND_PROTECT_SWITCH(sw);

        /**************************************************
         * Error occured?
         * Even if an error occurs, finish receiving the
         * frame to clear it out of the wide port channel.
         * Only record the first error.
         **************************************************/

        if (ptiError == INFO_ERR_NONE)
        {
            ptiError = (FM_GET_FIELD(rv32, FM10000_PTI_RX_CTRL, Info));
            if (ptiError == INFO_ERR_FCS)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX, 
                             "EPL to Fabric FCS Error");
            }
            else if (ptiError == INFO_ERR_FRAMING)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                             "EPL to Fabric Framing Error");
            }
            else if (ptiError == INFO_ERR_INTERNAL)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                             "EPL/PCIe to Fabric Internal Error or EPL/PCIe "
                             "Reset");
            }
        }

        
        /**************************************************
         * Length 
         **************************************************/

        len = FM_GET_FIELD(rv32, FM10000_PTI_RX_CTRL, Len);

        
        /**************************************************
         * EOF 
         **************************************************/

        eof = (FM_GET_FIELD(rv32, FM10000_PTI_RX_CTRL, Mark) == MARK_EOF) ? 
              1 : 0;

        if (eof) 
        {
            frameStarted = FALSE;
        }
        
        /**************************************************
         * Receive up to 64-bits of data 
         **************************************************/

        if ((curBy + len) <= length)
        {
            
            /**************************************************
             * First 32-bits of data.
             **************************************************/

            err = switchPtr->ReadUINT32(sw, FM10000_PTI_RX_DATA0(), &rv32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);

            
            /**************************************************
             * Second 32-bits of data.
             **************************************************/

            err = switchPtr->ReadUINT32(sw, FM10000_PTI_RX_DATA1(), &rv32_2);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);

            
            /**************************************************
             * Update *data with the received data.
             **************************************************/

            for (i = 0 ; i < len ; i++)
            {
                if (i < 4)
                {
                    data[curBy++] = ((rv32 >> (i * 8)) & 0xFF);
                }
                else
                {
                    data[curBy++] = ((rv32_2 >> ((i - 4) * 8)) & 0xFF);
                }
            }

            /**************************************************
             * Update *dataLength
             * If an error occurs on subsequent iterations, the
             * caller will at least have some data.
             **************************************************/

            *dataLength += len;
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX, 
                         "Data array is not large enough to store packet "
                         "content");

            /* Save the error so that future loop iterations don't overwrite
             * this error with FM_OK or some other status */
            savedError = FM_ERR_BUFFER_FULL;
        }


        /**************************************************
         * Clear the 64-bit word (self clear)
         * RxValid = 0 and RxEnable = 1.
         **************************************************/

        rv32 = 0x0;
        FM_SET_BIT(rv32, FM10000_PTI_RX_CTRL, RxEnable, 1);
        err = switchPtr->WriteUINT32(sw, FM10000_PTI_RX_CTRL(), rv32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);

    }   /* end while (!eof) */

    DumpPacket(FM_LOG_CAT_EVENT_PKT_RX, data, *dataLength);
    
    /**************************************************
     * Restore saved error and report.
     **************************************************/

    if (savedError != FM_OK)
    {
        err = savedError;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);
    }
    

    /**************************************************
     * Handle packet error reporting. 
     **************************************************/

    if (ptiError != INFO_ERR_NONE)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_RX, err);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);
}   /* end fm10000PTIReceive */

