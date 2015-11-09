
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_generic_rawsocket.c
 * Creation Date:   May, 2013
 * Description:     Raw socket packet methods. This code assumes
 *                  mutiple switches driven by one single driver.
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

#include <fm_sdk_int.h>
#include <platforms/common/packet/generic-rawsocket/fm_generic_rawsocket.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>  
#include <poll.h>

/* Redefine some type to include ethtool.h */
typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;

#include <linux/sockios.h>
#include <linux/ethtool.h>

#ifdef ENABLE_TIMESTAMP
# include <linux/net_tstamp.h>
#endif

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM_F56_BYTE_LEN             8   /* 8 including user field */
#define FM_F64_BYTE_LEN             8
#define FM_MAC_HDR_BYTE_LEN         12
#define FM_RECV_BUFFER_THRESHOLD    4

/* New linux socket protocol for IES (Intel Ethernet Switch) Frames
 *
 * With this protocol, the ethernet frame (DMAC, SMAC, ETYPE, etc) is
 * preceded with an 8 byte timetag and an 8 byte FTAG.
 * 
 * | Timestamp (8B) | FTAG (8B) | Ethernet Frame (48B - 15KB) |
 * 
 * The new protocol has been placed here until it makes its way through
 * the linux repositories. */ 
#ifndef ETH_P_XDSA
#define ETH_P_XDSA                   0x00F8
#endif

#ifndef ETHTOOL_SPFLAGS
#define ETHTOOL_SPFLAGS              0x00000028 /* Set driver-private flags bitmap */
#endif

#define ETHTOOL_PRV_FLAG_IES        (1 << 0)

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
/** fmRawPacketSocketDestroy
 * \ingroup intPlatformCommon
 *
 * \desc            Destroy the raw packet socket and free up resource.
 *
 * \param[in]       sw is the switch number to destroy.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRawPacketSocketDestroy(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (close(GET_PLAT_STATE(sw)->rawSocket) == -1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, 
                     "couldn't not destroy raw packet socket\n");
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr)
    {
        switchPtr->isRawSocketInitialized = FM_DISABLED;
    }

    err = fmGenericPacketDestroy(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmRawPacketSocketDestroy */




/*****************************************************************************/
/** fmRawPacketSocketHandlingInitialize
 * \ingroup intPlatformCommon
 *
 * \desc            Initializes the raw packet socket transfer module.
 *
 * \param[in]       sw is the switch number to initialize.
 * 
 * \param[in]       hasFcs is TRUE if the packet includes the FCS field.
 * 
 * \param[in]       iface is a string containing the netdev's interface name
 *                  through which the packets should be sent / received.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRawPacketSocketHandlingInitialize(fm_int  sw, 
                                              fm_bool hasFcs, 
                                              fm_text iface)
{
    fm_status                err = FM_OK;
    fm_int                   rawSock = -1;
    struct ifreq             ifr;
    struct sockaddr_ll       sa;
    struct ethtool_value     ethValue;
#ifdef ENABLE_TIMESTAMP
    struct ifreq             hwtstamp;
    struct hwtstamp_config   hwconfig;
    struct hwtstamp_config   hwconfig_requested;
    fm_int                   val;
    socklen_t                len;
    fm_int                   so_timestamping_flags;
    struct cmsghdr *         cmsg;
#endif
    char                     strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                  strErrNum;
    fm_switch               *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d hasFcs=%s\n",
                 sw,
                 FM_BOOLSTRING(hasFcs));

    if (iface == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    err = fmGenericPacketHandlingInitializeV2(sw, hasFcs);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* initialize the raw packet socket */
    rawSock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_XDSA));
    if (rawSock == -1)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                     "couldn't create raw packet socket\n");
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* retrieve ethernet interface index */
    FM_STRNCPY_S(ifr.ifr_name, IF_NAMESIZE, iface, IF_NAMESIZE);
    if (ioctl(rawSock, SIOCGIFINDEX, &ifr) == -1) 
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                     "Failed to retrieve index for interface %s\n",
                     iface);
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* initialize sockaddr_ll */
    FM_CLEAR(sa);
    sa.sll_family   = PF_PACKET;
    sa.sll_protocol = htons(ETH_P_XDSA);
    sa.sll_ifindex  = ifr.ifr_ifindex;
    if (bind(rawSock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                     "Failed to bind to the raw packet socket\n");
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }
    
#ifdef ENABLE_TIMESTAMP
    /*Set timestamp related configurations */
    FM_CLEAR(hwconfig);
    FM_CLEAR(hwconfig_requested);
    FM_CLEAR(hwtstamp);

    strncpy(hwtstamp.ifr_name, iface, IF_NAMESIZE);
    hwtstamp.ifr_data = (void *)&hwconfig;
    hwconfig.rx_filter = HWTSTAMP_FILTER_ALL;
    hwconfig_requested = hwconfig;

    errno = 0;
    if (ioctl(rawSock, SIOCSHWTSTAMP, &hwtstamp) < 0) 
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to configure RX hardware timestamp for all"
                         " received packets. Error: %s\n",
                         strErrBuf);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to configure RX hardware timestamp for all"
                         " received packets. Error: %d\n",
                         errno);
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                 "SIOCSHWTSTAMP: tx_type %d requested, got %d; "
                 " rx_filter %d requested, got %d\n",
                  hwconfig_requested.tx_type, 
                  hwconfig.tx_type,
                  hwconfig_requested.rx_filter, 
                  hwconfig.rx_filter);

    so_timestamping_flags = SOF_TIMESTAMPING_RX_HARDWARE | 
                            SOF_TIMESTAMPING_RAW_HARDWARE;

    if (setsockopt(rawSock,
                   SOL_SOCKET,
                   SO_TIMESTAMPING,
                   &so_timestamping_flags,
                   sizeof(so_timestamping_flags)) < 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Failed to set timestamping on socket\n");
        /* Continue without timestamp */
    }

    len = sizeof(val);
    errno = 0;
    if (getsockopt(rawSock, SOL_SOCKET, SO_TIMESTAMPING, &val, &len) < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to retrieve SO_TIMESTAMPING socket options."
                         " Error %s",
                         strErrBuf);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to retrieve SO_TIMESTAMPING socket options."
                         " Error %d",
                         errno);
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "SO_TIMESTAMPING %d\n", val);
        if (val != so_timestamping_flags)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Expected value %d but retrieved %d\n",
                         so_timestamping_flags,
                         val);
        }
    }
#endif

    /* Set the Driver in ies-tagging mode on management PEP */
    ethValue.cmd = ETHTOOL_SPFLAGS;
    ethValue.data = ETHTOOL_PRV_FLAG_IES;
    ifr.ifr_data = (void*) &ethValue;
    if (ioctl(rawSock, SIOCETHTOOL, &ifr) == -1)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to set ies-tagging: %s\n",
                         strErrBuf);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Failed to set ies-tagging: %d\n",
                         errno);
        }
    }

    if (ioctl(rawSock, SIOCGIFFLAGS, &ifr) == -1)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                         "Failed to get socket flags: %s\n",
                         strErrBuf);
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                         "Failed to get socket flags: %d\n",
                         errno);
        }

        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }

    if ((ifr.ifr_flags & IFF_UP) == 0)
    {
        /* Bring the NIC up */
        ifr.ifr_flags |= IFF_UP;

        if (ioctl(rawSock, SIOCSIFFLAGS, &ifr) == -1)
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                             "Failed to bring up: %s\n",
                             strErrBuf);
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                             "Failed to bring up: %d\n",
                             errno);
            }
            
            err = FM_FAIL;
            FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
        }
    }

    GET_PLAT_STATE(sw)->rawSocket = rawSock;
    FM_STRNCPY_S(GET_PLAT_STATE(sw)->ifaceName, IF_NAMESIZE, iface, IF_NAMESIZE);

    /* Create the receive packet thread */
    err = fmCreateThread("raw_packet_socket receive",
                         FM_EVENT_QUEUE_SIZE_NONE,
                         &fmRawPacketSocketReceivePackets,
                         &(GET_PLAT_STATE(sw)->sw),
                         GET_PLAT_RAW_LISTENER(sw));

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr)
    {
        switchPtr->isRawSocketInitialized = FM_ENABLED;
    }

ABORT:
    if ( (err != FM_OK) &&
         (rawSock != -1) )
    {
        close(rawSock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmRawPacketSocketHandlingInitialize */




/*****************************************************************************/
/** fmRawPacketSocketSendPackets
 * \ingroup intPlatformCommon
 *
 * \desc            When called, iterates through the packet queue and
 *                  continues to send packets until either the queue empties.
 *
 * \param[in]       sw refers to the switch number to send packets to.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRawPacketSocketSendPackets(fm_int sw)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm_packetHandlingState *pktState;
    fm_packetQueue *        txQueue;
    fm_packetEntry *        packet;
    fm_int32                rc;
    fm_buffer               *sendBuf;
    struct msghdr           msg;
    struct iovec            iov[UIO_MAXIOV];
    fm_islTag               islTag;
    fm_uint32               fcs;
    fm_uint64               rawTS;
    char                    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t                 strErrNum;
    struct ifreq            ifr;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    pktState  = GET_PLAT_PKT_STATE(sw);

    if (GET_PLAT_STATE(sw)->rawSocket <= 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_TX, 
                     "Socket is not initialized.\n");
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_UNINITIALIZED);
    }

    /* initialize the message header */
    FM_CLEAR(msg);
    msg.msg_name = NULL; /* Optional field */
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 0;
    msg.msg_flags = 0;

    FM_STRNCPY_S(ifr.ifr_name, IF_NAMESIZE, GET_PLAT_STATE(sw)->ifaceName, IF_NAMESIZE);

    txQueue = &pktState->txQueue;
    fmPacketQueueLock(txQueue);

    if (ioctl(GET_PLAT_STATE(sw)->rawSocket, SIOCGIFFLAGS, &ifr) == -1)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX, 
                         "Failed to get socket %d flags for device %s: %s\n",
                         GET_PLAT_STATE(sw)->rawSocket,
                         ifr.ifr_name,
                         strErrBuf);
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX, 
                         "Failed to get socket %d flags for device %s: %d\n",
                         GET_PLAT_STATE(sw)->rawSocket,
                         ifr.ifr_name,
                         errno);
        }
        switchPtr->transmitterLock = TRUE;
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    if ((ifr.ifr_flags & IFF_RUNNING) == 0)
    {    
        FM_LOG_WARNING(FM_LOG_CAT_EVENT_PKT_TX,
                       "Network device %s resources are not allocated.\n",
                       ifr.ifr_name);
        switchPtr->transmitterLock = TRUE;
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    /* Iterate through the packets in the tx queue */
    for ( ;
          txQueue->pullIndex != txQueue->pushIndex ;
          txQueue->pullIndex = (txQueue->pullIndex + 1) % FM_PACKET_QUEUE_SIZE)
    {
        packet = &txQueue->packetQueueList[txQueue->pullIndex];

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "sending packet in slot %d, length=%d tag=%d fcs=%08x\n",
                     txQueue->pullIndex, packet->length,
                     packet->suppressVlanTag, packet->fcsVal);
        msg.msg_iovlen = 0;

        /* Add the 8 byte timetag iovec. Note that the value is ignored
         * by the driver as it gets overwritten by the PEP. */
        rawTS = 0;
        iov[msg.msg_iovlen].iov_base = &rawTS;
        iov[msg.msg_iovlen].iov_len = sizeof(rawTS);
        msg.msg_iovlen++;

        if (packet->islTagFormat == FM_ISL_TAG_F56)
        {
            /* Add the FTAG (F56) iovec */
            islTag.f56.tag[0] = htonl(packet->islTag.f56.tag[0]);
            islTag.f56.tag[1] = htonl(packet->islTag.f56.tag[1]);
            iov[msg.msg_iovlen].iov_base = &islTag.f56.tag[0];
            iov[msg.msg_iovlen].iov_len = FM_F56_BYTE_LEN;
            msg.msg_iovlen++;
        }
        
        /* iterate through all buffers */
        for ( sendBuf = packet->packet ; sendBuf ; sendBuf = sendBuf->next )
        {
            /* if first buffer ... */
            if (sendBuf == packet->packet)
            {
                /* Cannot modify the send buffer, since the same buffer can be
                 * used multiple times to send to multiple ports */

                /* second iovec is the mac header */
                iov[msg.msg_iovlen].iov_base = sendBuf->data;
                iov[msg.msg_iovlen].iov_len = FM_MAC_HDR_BYTE_LEN;
                msg.msg_iovlen++;

                if (packet->islTagFormat == FM_ISL_TAG_F64)
                {
                    /* Insert the F64 ISL tag */
                    islTag.f64.tag[0] = htonl(packet->islTag.f64.tag[0]);
                    islTag.f64.tag[1] = htonl(packet->islTag.f64.tag[1]);
                    iov[msg.msg_iovlen].iov_base = &islTag.f64.tag[0];
                    iov[msg.msg_iovlen].iov_len = FM_F64_BYTE_LEN;
                    msg.msg_iovlen++;
                }

                /* Third is the data in the first chain */
                if (packet->suppressVlanTag)
                {
                    iov[msg.msg_iovlen].iov_base = &sendBuf->data[4];
                    iov[msg.msg_iovlen].iov_len = sendBuf->len-16;
                    msg.msg_iovlen++;
                }
                else
                {
                    iov[msg.msg_iovlen].iov_base = &sendBuf->data[3];
                    iov[msg.msg_iovlen].iov_len = sendBuf->len-12;
                    msg.msg_iovlen++;
                }
            }
            else
            {
                /* The rest of the chain */
                iov[msg.msg_iovlen].iov_base = sendBuf->data;
                iov[msg.msg_iovlen].iov_len = sendBuf->len;
                msg.msg_iovlen++;
            }

        }   /* end for (...) */

        /* Append user-supplied FCS value to packet. */
        if (pktState->sendUserFcs)
        {
            fcs = htonl(packet->fcsVal);
            iov[msg.msg_iovlen].iov_base = &fcs;
            iov[msg.msg_iovlen].iov_len = sizeof(fcs);
            msg.msg_iovlen++;
        }

        /* now send it to the driver */
        errno = 0;
        rc = sendmsg(GET_PLAT_STATE(sw)->rawSocket, &msg, MSG_DONTWAIT);
        if (rc == -1)
        {
            switchPtr->transmitterLock = TRUE;
            if (errno != EWOULDBLOCK)
            {
                err = FM_FAIL;

                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                             "rawSocket %d\n",
                             GET_PLAT_STATE(sw)->rawSocket);

                strErrNum = FM_STRERROR_S(strErrBuf,
                                          FM_STRERROR_BUF_SIZE,
                                          errno);
                if (strErrNum == 0)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_TX,
                                 "sendmsg failed: %s - errno %d\n",
                                 strErrBuf,
                                 errno);
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_TX, 
                                 "sendmsg failed - errno %d\n", errno);
                }
            }
            if (errno == EMSGSIZE)
            {
                switchPtr->transmitterLock = FALSE;
            }
            else
            {
                goto ABORT;
            }
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX, "%d bytes were sent\n", rc);

            switchPtr->transmitterLock = FALSE;
            fmDbgDiagCountIncr(sw, FM_CTR_TX_PKT_COMPLETE, 1);
        }

        /**************************************************
         * free buffer only when
         * (1) sending to a single port;
         * or (2) this is the last packet of multiple 
         * identical packets
         **************************************************/

        if (packet->freePacketBuffer)
        {
            /* ignore the error code since it's better to continue */
            (void) fmFreeBufferChain(sw, packet->packet);

            fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_TX_BUFFER_FREES, 1);
        }
    }

ABORT:
    fmPacketQueueUnlock(txQueue);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmRawPacketSocketSendPackets */




/*****************************************************************************/
/** fmRawPacketSocketReceivePackets
 * \ingroup intPlatformCommon
 *
 * \desc            Handles reception of packets by raw packet socket.
 *
 * \param[in]       args is a pointer to the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void * fmRawPacketSocketReceivePackets(void *args)
{
    fm_thread *        thread;
    fm_int             sw;
    fm_buffer *        recvChainHead = NULL;
    fm_buffer *        nextBuffer;
    struct pollfd      rfds;
    struct msghdr      msg;
    struct iovec       iov[UIO_MAXIOV];
    struct ifreq       ifr;
    fm_int             retval;
    fm_int             availableBuffers;
    fm_int             len;
    fm_int             iov_offset;
    fm_int             iov_count = 0;
    fm_int             maxMtu = 0;
    fm_int             newMtu;
    fm_status          status;
    char               strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t            strErrNum;
    fm_byte            rawTS[8];
    fm_pktSideBandData sbData;
#ifdef ENABLE_TIMESTAMP
    struct cmsghdr *   cmsg;
    union {
        struct cmsghdr  cm;
        char            control[512];

    } control;
#endif
    

    thread = FM_GET_THREAD_HANDLE(args);
    sw     = *(FM_GET_THREAD_PARAM(fm_int, args));

    FM_NOT_USED(thread);    /* If logging is disabled, thread won't be used */

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "thread = %s, sw = %d\n",
                 thread->name,
                 sw);

    /* initialize the message header */
    FM_CLEAR(msg);
    msg.msg_name = NULL; /* Optional field */
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 0;
    msg.msg_flags = 0;
#ifdef ENABLE_TIMESTAMP
    msg.msg_control = &control;
    msg.msg_controllen = sizeof(control);
#endif

    /* Setup the name of the interface */
    FM_STRNCPY_S(ifr.ifr_name, 
                 sizeof(ifr.ifr_name), 
                 GET_PLAT_STATE(sw)->ifaceName, 
                 sizeof(GET_PLAT_STATE(sw)->ifaceName));

    /* Prepare the pollfd struct */
    rfds.fd = GET_PLAT_STATE(sw)->rawSocket;
    rfds.events = POLLIN;
    rfds.revents = 0;

    /**************************************************
     * Loop forever calling packet receive handler.
     **************************************************/

    while (TRUE)
    {
        errno = 0;
        retval = poll(&rfds, 1, FM_FDS_POLL_TIMEOUT_USEC);

        if (retval == -1)
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                               "ERROR: select failed: %s!\n",
                               strErrBuf);
            }
            else
            {
                FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                               "ERROR: select failed: %d!\n",
                               errno);
            }

            /* Switch was removed, kill the thread */
            if (GET_SWITCH_PTR(sw) == NULL)
            {
                break;
            }

            continue;
        }
        else if (!retval)
        {
            /* Switch was removed, kill the thread */
            if (GET_SWITCH_PTR(sw) == NULL)
            {
                break;
            }

            continue; /* timeout */
        }

        /* get the number of available buffers from the buffer manager*/
        fmPlatformGetAvailableBuffers(&availableBuffers);

        if (availableBuffers <= FM_RECV_BUFFER_THRESHOLD)
        {
            /* wait for buffer to come back, before dequeueing data */
            fmYield();
            continue;
        }

        if (ioctl(GET_PLAT_STATE(sw)->rawSocket, SIOCGIFMTU, &ifr) == -1)
        {
             FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                            "WARNING: failed to read netdev MTU\n");
             continue;
        }
        else
        {
            newMtu = ifr.ifr_mtu;
        }

        /* MTU Size change */
        if (newMtu != maxMtu)
        {
            if (recvChainHead != NULL)
            {
                /* release the existing buffer chain */
                status = fmFreeBufferChain(FM_FIRST_FOCALPOINT, recvChainHead);

                if (status != FM_OK)
                {
                    FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                                 "Unable to release prior buffer chain, "
                                 "status = %d (%s)\n",
                                 status,
                                 fmErrorMsg(status) );
                }

                recvChainHead = NULL;
            }

            /* compute new buffer count */
            iov_count = newMtu / FM_BUFFER_SIZE_BYTES;
            if (newMtu % FM_BUFFER_SIZE_BYTES)
            {
                iov_count++;
            }

            maxMtu = newMtu;
        }

        if (recvChainHead == NULL)
        {
            /* allocate a new buffer chain and initialize iovec array */
            msg.msg_iovlen = 0;
            
            /* 8-Byte Timestamp IOV */
            iov[msg.msg_iovlen].iov_base = rawTS;
            iov[msg.msg_iovlen].iov_len  = sizeof(rawTS);
            msg.msg_iovlen++;

            for (iov_offset = 0 ; iov_offset < iov_count ; iov_offset++)
            {
                do
                {
                    nextBuffer = fmAllocateBuffer(FM_FIRST_FOCALPOINT);

                    if (nextBuffer == NULL)
                    {
                        /* Wait a little while for buffer to return */
                        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_RX_OUT_OF_BUFFERS,
                                                 1);
                        fmYield();
                    }
                }
                while (nextBuffer == NULL);

                if (recvChainHead == NULL)
                {
                    recvChainHead    = nextBuffer;
                    nextBuffer->next = NULL;
                }
                else
                {
                    status = fmAddBuffer(recvChainHead, nextBuffer);

                    if (status != FM_OK)
                    {
                        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                                     "Unable to add buffer %d (%p) to chain %p\n",
                                     iov_offset,
                                     (void *) nextBuffer,
                                     (void *) recvChainHead );
                        break;
                    }
                }

                iov[msg.msg_iovlen].iov_base = nextBuffer->data;
                iov[msg.msg_iovlen].iov_len  = FM_BUFFER_SIZE_BYTES;
                msg.msg_iovlen++;
            }
        }

        /* now receive from the driver */
        len = recvmsg(GET_PLAT_STATE(sw)->rawSocket,
                      &msg,
                      0);

        if (len == -1)
        {
            continue;
        }

#ifdef ENABLE_TIMESTAMP
        for (cmsg = CMSG_FIRSTHDR(&msg);
             cmsg;
             cmsg = CMSG_NXTHDR(&msg, cmsg)) 
        {
            if ( (cmsg->cmsg_level == SOL_SOCKET) &&
                 (cmsg->cmsg_type  == SO_TIMESTAMPING) &&
                 (cmsg->cmsg_len   == CMSG_LEN(sizeof(struct timespec) * 3)) )
            {
                    struct timespec *stamp =
                        (struct timespec *)CMSG_DATA(cmsg);
                    /* cmsg has 3 different timestamps. Timestamp we are interested is 
                     * located in index 2 */
                    sbData.ingressTimestamp.seconds     = ( (fm_int64)(stamp[2].tv_sec) );
                    sbData.ingressTimestamp.nanoseconds = ( (fm_int64)(stamp[2].tv_nsec) );
            }
            else
            {
                    FM_LOG_WARNING(FM_LOG_CAT_PLATFORM, 
                                  "Unknown control message of level %d type %d len %zu  received\n",
                                  cmsg->cmsg_level, 
                                  cmsg->cmsg_type,
                                  cmsg->cmsg_len);
            }
        }
#endif

        /* Remove the timestamp's length to get the length of the actual
         * packet */
        len -= sizeof(rawTS);

        /* The raw socket does not carry the FCS in either tx or rx, however
         * the API expects it to be present. Because the API clears the 
         * FCS value before sending the packet event to the application, don't 
         * bother about setting the correct FCS value and just increment the 
         * length. The FCS value is undefined (whatever is in the fm_buffer at 
         * the FCS position). */
        len += 4;

        /* fill in the used buffer sizes */
        nextBuffer = recvChainHead;

        while (nextBuffer != NULL)
        {
            if (len > FM_BUFFER_SIZE_BYTES)
            {
                nextBuffer->len = FM_BUFFER_SIZE_BYTES;
            }
            else
            {
                nextBuffer->len = len;
            }

            len -= nextBuffer->len;

            if ( (len <= 0) && (nextBuffer->next != NULL) )
            {
                status = fmFreeBufferChain(FM_FIRST_FOCALPOINT,
                                           nextBuffer->next);

                if (status != FM_OK)
                {
                    FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                                 "Unable to release unused buffer chain, "
                                 "status = %d (%s)\n",
                                 status,
                                 fmErrorMsg(status) );
                }

                nextBuffer->next = NULL;
            }

            nextBuffer = nextBuffer->next;
        }

        if (recvChainHead == NULL)
        {
            continue;
        }

        /* Store the raw timestamp in 64b format */
        sbData.rawTimeStamp  = ((fm_uint64) (rawTS[0] & 0xFF)) << 56;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[1] & 0xFF)) << 48;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[2] & 0xFF)) << 40;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[3] & 0xFF)) << 32;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[4] & 0xFF)) << 24;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[5] & 0xFF)) << 16;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[6] & 0xFF)) << 8;
        sbData.rawTimeStamp |= ((fm_uint64) (rawTS[7] & 0xFF));

        /* Don't provide an ISL tag pointer, let the API handle the ISL
         * tag information (included in the fm_buffer chain). */
        status = fmPlatformReceiveProcessV2(sw,
                                            recvChainHead,
                                            NULL,
                                            &sbData);

        if (status != FM_OK)
        {
            FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                         "Returned error status %d "
                         "(%s)\n",
                         status,
                         fmErrorMsg(status) );


        }
        /* Buffer chain has now been consumed */
        recvChainHead = NULL; 

    }   /* end while (TRUE) */

    fmExitThread(thread);

    return NULL;

}   /* end fmRawPacketSocketReceivePackets */




/*****************************************************************************/
/** fmIsRawPacketSocketDeviceOperational
 * \ingroup intPlatformCommon
 *
 * \desc            If raw packet socket was initialized, it verifies that
 *                  the underlying network device is operational.
 *
 * \param[in]       sw refers to the switch number to send packets to.
 *
 * \param[out]      isRawSocket is a pointer to a variable specifying if raw
 *                  socket is used.
 *
 * \param[out]      mtu is a pointer to a variable storing mtu value of
 *                  the raw sockets network device.
 *
 * \return          TRUE if either raw packet socket was not initialized or
 *                  raw packet socket is initialized and the underlying network
 *                  device is operational.
 * \return          FALSE if raw packet socket is initialized and the underlying
 *                  network device is not operational.
 *
 *****************************************************************************/
fm_bool fmIsRawPacketSocketDeviceOperational(fm_int   sw,
                                             fm_bool *isRawSocket,
                                             fm_int * mtu)
{
    fm_switch    *switchPtr;
    char         strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t      strErrNum;
    struct ifreq ifr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (isRawSocket != NULL)
    {
        *isRawSocket = FM_DISABLED;
    }
    
    if (switchPtr->isRawSocketInitialized ==  FM_DISABLED)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, TRUE, "No raw packet socket\n");
    }

    FM_STRNCPY_S(ifr.ifr_name,
                 IF_NAMESIZE,
                 GET_PLAT_STATE(sw)->ifaceName,
                 IF_NAMESIZE);
    
    if (ioctl(GET_PLAT_STATE(sw)->rawSocket, SIOCGIFFLAGS, &ifr) == -1)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                         "Failed to get socket %d flags for device %s: %s\n",
                         GET_PLAT_STATE(sw)->rawSocket,
                         ifr.ifr_name,
                         strErrBuf);
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM, 
                         "Failed to get socket %d flags for device %s: %d\n",
                         GET_PLAT_STATE(sw)->rawSocket,
                         ifr.ifr_name,
                         errno);
        }
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, FALSE, "Not operational\n");
    }

    if ((ifr.ifr_flags & IFF_RUNNING) == 0)
    {    
        FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                       "Network device %s resources are not allocated.\n",
                       ifr.ifr_name);
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, FALSE, "Not operational\n");
    }

    if (ioctl(GET_PLAT_STATE(sw)->rawSocket, SIOCGIFMTU, &ifr) == -1)
    {
        FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                       "WARNING: failed to read netdev MTU\n");
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM,
                           FALSE,
                           "MTU could not be retrieved\n");
    }

    if ( (isRawSocket != NULL) && (mtu != NULL) )
    {
        *isRawSocket = FM_ENABLED;
        *mtu         = ifr.ifr_mtu;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_PLATFORM, TRUE, "Operational\n");

} /* fmIsRawPacketSocketDeviceOperational */

