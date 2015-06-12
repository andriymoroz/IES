/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_generic_netlink.h
 * Creation Date:   September, 2011
 * Description:     Header file for generic netlink packet I/O
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

#ifndef __FM_FM_GENERIC_NETLINK_H
#define __FM_FM_GENERIC_NETLINK_H


/*****************************************************************************
 * START OF DEFINITIONS SHARED BETWEEN PLATFORM AND KERNEL CODE. 
 *  
 * These definitions MUST be synchronized with the corresponding definitions 
 * in fm_generic_netlink_kernel.h.
 *****************************************************************************/

/**************************************************
 * Netlink packet message header.
 *
 * This structure is a common structure visible by
 * both the API and the driver.
 **************************************************/
struct fm_nlHdr
{
    u_short     sw;             /* switch number */
    u_short     reserved;
    u_int       fm64[2];        /* F64 tag */
};

/*****************************************************************************
 * END OF DEFINITIONS SHARED BETWEEN PLATFORM AND KERNEL CODE
 *****************************************************************************/

/* Netlink function prototypes */
fm_status fmNetlinkSendPackets(fm_int sw);
void * fmNetlinkReceivePackets(void *args);
fm_status fmNetlinkPacketHandlingInitialize(void);
fm_status fmNetlinkPacketHandlingInitializeV2(fm_int sw, fm_bool hasFcs);
void * fmNetlinkReceivePacketsV2(void *args);
void * fmPlatformPacketScheduler(void *args);

#endif /* __FM_FM_GENERIC_NETLINK_H */
