/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_int.h
 * Creation Date:   2005
 * Description:     Contains structure definitions for various table entries
 *                  and switch state information
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

#ifndef __FM_FM_API_INT_H
#define __FM_FM_API_INT_H


/* Common definitions */
#include <api/internal/fm_api_common_int.h>
#include <api/internal/fm_api_portmask.h>
#include <api/internal/fm_api_portset_int.h>
#include <api/internal/fm_api_lag_int.h>
#include <api/internal/fm_api_glort_int.h>
#include <api/internal/fm_api_lport_int.h>

/* Internal definitions required by fm_api_switch_int.h */
#include <api/internal/fm_api_addr_int.h>
#include <api/internal/fm_api_cardinal_int.h>
#include <api/internal/fm_api_event_mac_maint_int.h>
#include <api/internal/fm_api_vlan_int.h>
#include <api/internal/fm_api_stp_int.h>
#include <api/internal/fm_api_routing_int.h>
#include <api/internal/fm_api_nat_int.h>
#include <api/internal/fm_api_nexthop_int.h>
#include <api/internal/fm_api_mcast_groups_int.h>
#include <api/internal/fm_api_lbg_int.h>
#include <api/internal/fm_api_events_int.h>
#include <api/internal/fm_api_stacking_int.h>
#include <api/internal/fm_api_fibm_int.h>
#include <api/internal/fm_api_acl_int.h>
#include <api/internal/fm_api_mirror_int.h>
#include <api/internal/fm_api_stat_int.h>
#include <api/internal/fm_api_vn_int.h>
#include <api/internal/fm_api_flow_int.h>
#include <api/internal/fm_api_mailbox_int.h>

/* Switch and port Generic Definitions */
#include <api/internal/fm_api_switch_int.h>
#include <api/internal/fm_api_port_int.h>

/* All other Internal Definitions */
#include <api/internal/fm_api_init_int.h>
#include <api/internal/fm_api_qos_int.h>

#ifdef FM_SUPPORT_SWAG
#include <api/internal/fm_api_swag_int.h>
#endif

#include <api/internal/fm_api_regs_cache_int.h>
#include <api/internal/fm_api_root_int.h>

#endif /* __FM_FM_API_INT_H */
