/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_attr_int.h
 * Creation Date:   May 13th, 2013
 * Description:     Header file for high level attributes of a switch
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

#ifndef __FM_FM10000_API_ATTR_INT_H
#define __FM_FM10000_API_ATTR_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* FM10000_SERDES_DEFAULT_SPICO_FW specifies which SPICO firmware version
 * is the default.
 * Valid values are:
 * 0: use fm10000_serdes_spico_code_prd1 as default.
 * 1: use fm10000_serdes_spico_code_prd2 as default. */
#define FM10000_SERDES_DEFAULT_SPICO_FW         1

/** Deep Inspection profile index used for inner/outer MAC address filtering
 *  (See ''FM_SWITCH_PARSER_DI_CFG'' switch attribute).
 *  \ingroup  constSystem */
#define FM10000_MAILBOX_MAC_FILTER_DI_PROFILE   3

/** Deep Inspection profile index for ''FM_FLOW_MATCH_TCP_FLAGS''.
 *  (See the ''FM_SWITCH_PARSER_DI_CFG'' switch attribute.)
 *  \ingroup  constSystem */
#define FM10000_FLOW_DI_PROFILE                 4

typedef enum
{
    FM_RES_MAC_INDEX_MIN        = 0x00,     /* Minimum reserved MAC index */

    FM_RES_MAC_INDEX_BPDU       = 0x00,     /* 01-80-c2-00-00-00 */
    FM_RES_MAC_INDEX_PAUSE      = 0x01,     /* 01-80-c2-00-00-01 */
    FM_RES_MAC_INDEX_LACP       = 0x02,     /* 01-80-c2-00-00-02 */
    FM_RES_MAC_INDEX_802_1X     = 0x03,     /* 01-80-c2-00-00-03 */

    FM_RES_MAC_INDEX_GARP_MIN   = 0x20,     /* 01-80-c2-00-00-20 */
    FM_RES_MAC_INDEX_GARP_MAX   = 0x21,     /* 01-80-c2-00-00-21 */

    FM_RES_MAC_INDEX_MAX        = 0x3F,      /* Maximum reserved MAC index */

} fm_resMacIndex;


/*****************************************************************************
 * Public Function Prototypes
 *****************************************************************************/

fm_status fm10000GetSwitchAttribute(fm_int sw, fm_int attr, void *value);
fm_status fm10000SetSwitchAttribute(fm_int sw, fm_int attr, void *value);

fm_status fm10000SetBoolSwitchAttribute(fm_int sw, fm_int attr, fm_bool value);
fm_status fm10000SetIntSwitchAttribute(fm_int sw, fm_int attr, fm_int value);

fm_status fm10000SetMplsEtherType(fm_int sw, fm_int index, fm_uint16 etherType);

fm_status fm10000InitSwitchAttributes(fm_int sw);

fm_status fm10000InitHashing(fm_int sw);

fm_status fm10000EnableSwitchMacFiltering(fm_int sw);

#endif /* __FM_FM10000_API_ATTR_INT_H */
