/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_netsock.h
 * Creation Date:   October 4, 2010
 * Description:     Header file for simple unix UDP & TCP wrappers.
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_NETSOCK_H
#define __FM_FM_NETSOCK_H

typedef enum
{
    /* Indicates a socket of type SOCK_STREAM */
    FM_SOCKET_TYPE_TCP,

    /* Indicates a socket of type SOCK_DGRAM */
    FM_SOCKET_TYPE_UDP,

    /* Indicates a closed socket */
    FM_SOCKET_TYPE_CLOSED,

    /** UNPUBLISHED: For internal use only */
    FM_SOCKET_TYPE_MAX

} fm_socketType;

typedef struct _fm_socket
{
    /** socket descriptor, may be UDP or TCP */
    fm_int             sock;

    /** socket stream type */
    fm_socketType      type;

    /** the address structure associated with the socket */
    struct sockaddr_in address;

    /** the port a server socket is bound to */
    fm_int             serverPort;

} fm_socket;

typedef enum
{
    /** Indicates no event */
    FM_NETWORK_EVENT_NONE = 0,

    /** Indicates a new client connected */
    FM_NETWORK_EVENT_NEW_CLIENT,

    /** Indicates data available */
    FM_NETWORK_EVENT_DATA_AVAILABLE,

    /** UNPUBLISHED: For internal use only */
    FM_NETWORK_EVENT_MAX,

} fm_networkEvent;

fm_status fmCreateNetworkServer(fm_socket *   socketInfo,
                                fm_socketType type,
                                fm_int        port,
                                fm_int        backlog);

fm_status fmCreateNetworkClient(fm_socket *   socketInfo,
                                fm_socketType type,
                                fm_text       host,
                                fm_int        port);

/* Assumes numSockets > 1, index 0 is the server */
fm_status fmWaitForNetworkEvent(fm_socket **  sockets,
                                fm_int *      numSockets,
                                fm_int        maxSockets,
                                fm_int *      eventReceived,
                                fm_timestamp *timeout);

fm_status fmCloseNetworkConnection(fm_socket *client);

fm_status fmSendNetworkData(fm_socket *socketInfo,
                            void *     data,
                            fm_int     numBytes);

fm_status fmReceiveNetworkData(fm_socket *socketInfo,
                               void *     data,
                               fm_int     maxBytes,
                               fm_int *   numBytes);

#endif /* __FM_FM_NETSOCK_H */
