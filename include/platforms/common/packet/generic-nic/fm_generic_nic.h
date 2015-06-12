/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_generic_nic.h
 * Creation Date:   Sep 1, 2008
 * Description:     Header file for generic NIC packet I/O
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

#ifndef __FM_FM_GENERIC_NIC_H
#define __FM_FM_GENERIC_NIC_H

#define FM_DEV_NAME_LEN             32

typedef struct _fm_nic
{
    /* Net device name used by this NIC. */
    fm_char devName[FM_DEV_NAME_LEN];

    /* Socket File descriptore returned by socket() */
    fm_int sockFd;

    /* Indicate whether the NIC is set (true) in promiscuous mode or not.*/
    fm_bool promiscuous;

    /* Number of switched attached to this NIC */
    fm_int numSwitches;

    /* Semaphore for holding the thread until the socket is part of the
       'select' list */
    fm_semaphore selectSemaphore;

    /* Indicate whether the NIC is part of the 'select' list.*/
    fm_bool inSelectList;

    /* Indicate whether the NIC is part of the 'select' list.*/
    fm_bool isWaitingOnSemaphore;

    /* Contains mac addr of the NIC used by the switch */
    unsigned char          nicMacAddr[6];
} fm_nic;


/* NIC function prototypes */
fm_status fmNicGetDeviceName(fm_int sw, fm_char *devName);
fm_status fmNicSetDeviceName(fm_int sw, fm_char *devName);
fm_status fmNicPacketHandlingInitialize(void);
fm_status fmNicSendPackets(fm_int sw);
fm_status fmNicIsDeviceReady(fm_int sw);

#endif /* __FM_FM_GENERIC_NIC_H */
