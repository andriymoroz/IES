/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platforms/common/lib/net/fm_netsock.c
 * Creation Date:   October 20, 2010
 * Description:     Wrapper library for basic sockets.
 *
 * Copyright (c) 2010 - 2015, Intel Corporation
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM_LOG_SYS_EXIT_ON_COND(cat, cond)                  \
    if ((cond)) {                                           \
        strErrNum = FM_STRERROR_S(strErrBuf,                \
                                  FM_STRERROR_BUF_SIZE,     \
                                  errno);                   \
        if (strErrNum == 0)                                 \
        {                                                   \
            FM_LOG_FATAL((cat),                             \
                         "System error %d: %s\n",           \
                         errno, strErrBuf);                 \
        }                                                   \
        else                                                \
        {                                                   \
            FM_LOG_FATAL((cat),                             \
                         "System error %d\n", errno);       \
        }                                                   \
        FM_LOG_EXIT_VERBOSE((cat), FM_FAIL);                \
    }

#if 0
#define VERBOSE
#endif

#define FM_GETHOSTBYNAME_BUF_SIZE 2048

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static const int SOCKET_ADDRLEN = sizeof(struct sockaddr_in);

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
/** fmCreateNetworkServer
 * \ingroup platformUtil
 *
 * \desc            Creates a network server socket.
 *
 * \param[out]      socketInfo points to the information structure for the
 *                  server socket.
 *
 * \param[in]       type is the type of socket to create.
 *
 * \param[in]       port is the port number for the socket.
 *
 * \param[in]       backlog is unused.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCreateNetworkServer(fm_socket *   socketInfo, 
                                fm_socketType type,
                                fm_int        port, 
                                fm_int        backlog)
{
    fm_status          status = FM_OK;
    fm_int             errResult;
    struct sockaddr_in addrInfo;
    socklen_t          addrLen = sizeof(addrInfo);
    char               strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t            strErrNum;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM,
                         "socketInfo=%p, port=%d, backlog=%d\n",
                         (void *) socketInfo, 
                         port, 
                         backlog);

    if (!socketInfo)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*socketInfo);

    if (type == FM_SOCKET_TYPE_TCP)
    {
        socketInfo->sock = socket(AF_INET, SOCK_STREAM, 0);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, socketInfo->sock == -1);

        socketInfo->address.sin_family      = AF_INET;
        socketInfo->address.sin_addr.s_addr = htonl(INADDR_ANY);
        socketInfo->address.sin_port        = htons(port);

        errResult = bind(socketInfo->sock, 
                         (struct sockaddr *) &socketInfo->address,  
                         SOCKET_ADDRLEN);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult == -1);

        errResult = listen(socketInfo->sock, 3);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult == -1);

#ifdef VERBOSE
        FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM,
                      "Socket descriptor %d initialized\n",
                      socketInfo->sock);
#endif
    }
    else if (type == FM_SOCKET_TYPE_UDP)
    {
        socketInfo->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, socketInfo->sock == -1);

        socketInfo->address.sin_addr.s_addr = htonl(INADDR_ANY);
        socketInfo->address.sin_port        = htons(port);

        errResult = bind(socketInfo->sock, 
                         (struct sockaddr *) &socketInfo->address,  
                         SOCKET_ADDRLEN);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult == -1);

    }
    else
    {
        FM_LOG_ASSERT(FM_LOG_CAT_PLATFORM,
                      FALSE,
                      "Unexpected socket type %d\n", type);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    errResult = getsockname(socketInfo->sock,
                            (struct sockaddr *) &addrInfo,
                            &addrLen);
    FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult == -1);

    socketInfo->serverPort = ntohs(addrInfo.sin_port);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, status);

}   /* end fmCreateNetworkServer */




/*****************************************************************************/
/** fmCreateNetworkClient
 * \ingroup platformUtil
 *
 * \desc            Creates a network client socket.
 *
 * \param[out]      socketInfo points to the information structure for the
 *                  client socket.
 *
 * \param[in]       type is the type of socket to create.
 *
 * \param[in]       host points to a string specifying the hostname.
 *
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCreateNetworkClient(fm_socket *   socketInfo, 
                                fm_socketType type,
                                fm_text       host, 
                                fm_int        port)
{
    struct hostent  h;
    struct hostent *hp;
    fm_int          errResult;
    char            strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t         strErrNum;
    char            buf[FM_GETHOSTBYNAME_BUF_SIZE];
    int             herrno;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM,
                         "socketInfo=%p, port=%d\n",
                         (void *) socketInfo, 
                         port);

    if (!socketInfo)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*socketInfo);

    if (type == FM_SOCKET_TYPE_TCP)
    {
        socketInfo->sock = socket(AF_INET, SOCK_STREAM, 0);
    }
    else if (type == FM_SOCKET_TYPE_UDP)
    {
        socketInfo->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    if (socketInfo->sock == -1)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to create socket type %d\n",
                     type);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    FM_CLEAR(socketInfo->address);

    socketInfo->address.sin_family = AF_INET;

    if ((gethostbyname_r(host,
                         &h,
                         buf,
                         FM_GETHOSTBYNAME_BUF_SIZE,
                         &hp,
                         &herrno) != 0)
        || (hp == NULL))
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to resolve hostname: %s\n", 
                     host);
    }

    FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, !hp);

    FM_MEMCPY_S(&socketInfo->address.sin_addr.s_addr,
                sizeof(socketInfo->address.sin_addr.s_addr),
                hp->h_addr,
                hp->h_length);

    socketInfo->address.sin_port = htons(port);

    if (type == FM_SOCKET_TYPE_TCP)
    {
        errResult = connect(socketInfo->sock,
                            (struct sockaddr *) &socketInfo->address,
                            SOCKET_ADDRLEN);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult != 0);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmCreateNetworkClient */




/*****************************************************************************/
/** fmWaitForNetworkEvent
 * \ingroup platformUtil
 *
 * \desc            Waits for network events.
 *
 * \param[out]      sockets points to an array of active and inactive sockets.
 *                  This function can activate inactive sockets when a client
 *                  attempts to make a connection. The array must be maxSockets
 *                  in length.
 *
 * \param[in]       numSockets is the current number of active sockets; it must
 *                  be in the range [1..maxSockets).
 *
 * \param[in]       maxSockets is the maximum number of sockets this function
 *                  can activate.
 *
 * \param[out]      eventReceived points to an array where this function places
 *                  the network event type for each active socket. The array
 *                  must be maxSockets elements in length.
 *
 * \param[in]       timeout is the amount of time to wait for a network event.
 *                  Set to ''FM_WAIT_FOREVER'' to wait indefinitely.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmWaitForNetworkEvent(fm_socket **  sockets, 
                                fm_int *      numSockets,
                                fm_int        maxSockets,
                                fm_int *      eventReceived, 
                                fm_timestamp *timeout)
{
    fm_int         currNumSockets;
    fm_int         errResult;
    fm_int         i;
    fm_int         j;
    fm_int         slot;
    fm_bool        doIncrement;
    struct pollfd  fds[FM_MAX_FDS_NUM] = { {0} };
    fm_int         fdsCnt;
    socklen_t      addrLen = SOCKET_ADDRLEN;
    struct timeval ts;
    char           strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t        strErrNum;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM,
                         "sockets=%p, numSockets=%p(%d), maxSockets=%d, "
                         "eventReceived=%p, timeout=%p\n",
                         (void *) sockets, 
                         (void *) numSockets, 
                         (numSockets ? *numSockets : -1),
                         maxSockets,
                         (void *) eventReceived, 
                         (void *) timeout);

    if ((!sockets || !eventReceived || !numSockets || (*numSockets == 0) || \
        (maxSockets > FM_MAX_FDS_NUM) || (*numSockets > FM_MAX_FDS_NUM)))
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    currNumSockets = *numSockets;

    for ( i = 0, fdsCnt = 0 ; i < currNumSockets ; i++ )
    {
#ifdef VERBOSE
        FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM, "Watching descriptor %d\n", sockets[i]->sock);
#endif

        if (sockets[i]->type != FM_SOCKET_TYPE_CLOSED)
        {
            fds[fdsCnt].fd = sockets[i]->sock;
            fds[fdsCnt].events = POLLIN;
            fds[fdsCnt].revents = 0;
            fdsCnt++;
        }
    }

    if (timeout != FM_WAIT_FOREVER)
    {
        FM_CLEAR(ts);
        ts.tv_sec  = (int) timeout->sec;
        ts.tv_usec = (int) timeout->usec;
    }

    errResult = poll(fds,
                     fdsCnt,
                     ((timeout == FM_WAIT_FOREVER) ? -1 : \
                      ((ts.tv_sec * 1000) + (ts.tv_usec / 1000)) ));

    FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult == -1);

    for ( i = 0, fdsCnt = 0 ; i < currNumSockets ; i++, fdsCnt++)
    {
        /* Default case */
        eventReceived[i] = FM_NETWORK_EVENT_NONE;

        if (sockets[i]->type != FM_SOCKET_TYPE_CLOSED && fds[fdsCnt].revents & POLLIN)
        {
            /* Handle TCP connection on server socket */
            if ((sockets[i]->type == FM_SOCKET_TYPE_TCP) &&
                (sockets[i]->serverPort > 0))
            {
                doIncrement = TRUE;
                slot = *numSockets;

                for ( j = 0 ; j < currNumSockets ; j++ )
                {
                    if (sockets[j]->type == FM_SOCKET_TYPE_CLOSED)
                    {
                        doIncrement = FALSE;
                        slot = j;
                        break;
                    }
                }

                if (slot >= maxSockets)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                 "Not enough socket slots left to accept client\n");
                    break;
                }

                FM_CLEAR(*sockets[slot]);

                sockets[slot]->sock
                    = accept(sockets[i]->sock,
                             (struct sockaddr *) &sockets[slot]->address,
                             &addrLen);
                FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM,
                                        sockets[slot]->sock == -1);

                sockets[slot]->type = FM_SOCKET_TYPE_TCP;

                eventReceived[i] = FM_NETWORK_EVENT_NEW_CLIENT;

#ifdef VERBOSE
                FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM,
                              "Handled new client connection\n");
#endif

                if (doIncrement)
                {
                    (*numSockets)++;
                }

                break;
            }
            else
            {
                eventReceived[i] = FM_NETWORK_EVENT_DATA_AVAILABLE;

#ifdef VERBOSE
                FM_LOG_DEBUG2(FM_LOG_CAT_PLATFORM,
                              "Data available on client #%d\n", i - 1);
#endif
            }
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmWaitForNetworkEvent */




/*****************************************************************************/
/** fmCloseNetworkConnection
 * \ingroup platformUtil
 *
 * \desc            Closes a network socket.
 *
 * \param[in,out]   client points to the socket information structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCloseNetworkConnection(fm_socket *client)
{
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM, "client=%p\n", (void *) client);

    if (!client)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (client->type == FM_SOCKET_TYPE_TCP)
    {
        close(client->sock);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmCloseNetworkConnection */




/*****************************************************************************/
/** fmSendNetworkData
 * \ingroup platformUtil
 *
 * \desc            Transmits data on a network socket.
 *
 * \param[in]       socketInfo points to the socket information structure.
 *
 * \param[out]      data points to the data to be transmitted.
 *
 * \param[in]       numBytes is the number of bytes to be transmitted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSendNetworkData(fm_socket *socketInfo, void *data, fm_int numBytes)
{
    fm_int  errResult;
    fm_int  nb;
    char    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM,
                         "socketInfo=%p, data=%p, numBytes=%d\n",
                         (void *) socketInfo, 
                         (void *) data, 
                         numBytes);

    if (!socketInfo || !data)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (socketInfo->type == FM_SOCKET_TYPE_UDP)
    {
        errResult = sendto(socketInfo->sock,
                           data,
                           numBytes,
                           0,
                           (struct sockaddr *) &socketInfo->address,
                           SOCKET_ADDRLEN);
        FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, errResult != 0);
    }
    else if (socketInfo->type == FM_SOCKET_TYPE_TCP)
    {
        do
        {
            nb = send(socketInfo->sock, data, numBytes, 0);
            FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, nb == -1);

            if (nb > 0)
            {
                data = (((fm_byte *) data) + nb);
                numBytes -= nb;
            }
        } while ((nb > 0) && numBytes);

    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmSendNetworkData */




/*****************************************************************************/
/** fmReceiveNetworkData
 * \ingroup platformUtil
 *
 * \desc            Receives data on a network socket.
 *
 * \param[in]       socketInfo points to the socket information structure.
 *
 * \param[out]      data points to the buffer in which the received data
 *                  should be stored.
 *
 * \param[in]       maxBytes is the maximum number of bytes to receive.
 *
 * \param[out]      numBytes points to a location in which the actual number
 *                  of received bytes will be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmReceiveNetworkData(fm_socket *socketInfo, 
                               void *     data,
                               fm_int     maxBytes, 
                               fm_int *   numBytes)
{
    fm_byte *          ptr;
    struct sockaddr_in addrInfo;
    socklen_t          addrLen;
    fm_int             nb;
    char               strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t            strErrNum;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM,
                         "socketInfo=%p, data=%p, maxBytes=%d, numBytes=%p\n",
                         (void *) socketInfo, 
                         (void *) data, 
                         maxBytes, 
                         (void *) numBytes);

    if (!socketInfo || !data || !numBytes)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (socketInfo->type == FM_SOCKET_TYPE_UDP)
    {
        *numBytes = recvfrom(socketInfo->sock,
                             data,
                             maxBytes,
                             0,
                             (struct sockaddr *) &addrInfo,
                             &addrLen);
    }
    else if (socketInfo->type == FM_SOCKET_TYPE_TCP)
    {
        ptr       = data;
        *numBytes = 0;

        do
        {
            nb = recv(socketInfo->sock, ptr, maxBytes - *numBytes, 0);
            FM_LOG_SYS_EXIT_ON_COND(FM_LOG_CAT_PLATFORM, nb == -1);

            /* Closed connection */
            if (nb == 0)
            {
                FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MORE);
            }

            ptr       += nb;
            *numBytes += nb;

        } while (*numBytes < maxBytes);
    }


    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmReceiveNetworkData */
