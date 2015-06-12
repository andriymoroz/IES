/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_netlink.h
 * Creation Date:   November 26, 2013
 * Description:     Netlink definitions that allow for reading packets directly
 *                  from netlink socket.
 *
 * Copyright (c) 2013, Intel Corporation
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

#ifndef __FM_FM_API_NETLINK_H
#define __FM_FM_API_NETLINK_H


/*****************************************************************************
 * These definitions MUST be synchronized with the corresponding definitions 
 * in fm_generic_netlink_kernel.h.
 *****************************************************************************/

/**************************************************/
/** \ingroup typeStruct
 * Netlink packet message extended header. 
 *
 * This structure is visible to both the application
 * and the kernel driver. It is used for received
 * packets that can be read without API processing.
 * To enable this mode, set 
 * ''FM_PACKET_RX_DRV_DEST_NETLINK_APP'' in API
 * property ''api.packet.rxDriverDestinations''.
 **************************************************/
typedef struct _fm_nlExtHdr
{
    /** The ISL tag. */
    u_int   fm64[FM_MAX_ISL_TAG_SIZE];

    /** The number of the switch from which the packet was received. */
    u_short sw;

    /** The VLAN from which the packet was received. */
    u_short vlan;

    /** The internal switch priority associated with this packet. */
    u_short priority;

    /** The vlan priority associated with this packet. */
    u_short vlanPriority;

    /** Code indicating the reason that the frame was delivered by the
     *  switch to the CPU. */
    u_short trapAction;

    /** The vlan EtherType associated with this packet. */
    u_short vlanEtherType;

    /** The logical port number on which the packet was received. */
    int     srcPort;

} fm_nlExtHdr;


/** Netlink socket. Uses the same number as NETLINK_USERSOCK.
 *  Specified when creating netlink socket.
 *  \ingroup constSystem */
#define FM_NL_SOCK              2

/* Netlink listening group types when reading from the netlink interface. */
#define FM_NL_GROUP_L2          1
#define FM_NL_GROUP_IP          2
#define FM_NL_GROUP_ARP         4
#define FM_NL_GROUP_RAW_CB      8


/** Netlink socket listening to all groups. Used when binding netlink socket.
 *  \ingroup constSystem */
#define FM_NL_GROUP_ALL                         \
        (FM_NL_GROUP_L2  | FM_NL_GROUP_IP |     \
         FM_NL_GROUP_ARP | FM_NL_GROUP_RAW_CB)


#endif /* __FM_FM_API_NETLINK_H */
