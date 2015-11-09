/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_event_mac_maint_int.h
 * Creation Date:   August 23, 2013
 * Description:     Internal MAC maintenance declarations for the FM10000.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_EVENT_MAC_MAINT_INT_H
#define __FM_FM10000_API_EVENT_MAC_MAINT_INT_H


/*****************************************************************************
 * Public function prototypes.
 *****************************************************************************/


/**************************************************
 * fm10000_api_event_fast_maint.c
 **************************************************/

fm_uint64 fm10000GetAgingTimer(void);


/**************************************************
 * fm10000_api_event_mac_maint.c
 **************************************************/

fm_status fm10000HandleMACTableEvents(fm_int sw);
fm_status fm10000TCNInterruptHandler(fm_int sw, fm_uint32 events);


/**************************************************
 * fm6000_api_mac_purge_table.c
 **************************************************/

fm_status fm10000HandlePurgeRequest(fm_int sw);


#endif /* __FM_FM10000_API_EVENT_MAC_MAINT_INT_H */

