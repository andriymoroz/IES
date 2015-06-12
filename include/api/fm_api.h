/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api.h
 * Creation Date:   April 20, 2005
 * Description:     Wrapper to include all api files
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_H
#define __FM_FM_API_H


#include <api/fm_api_common.h>
#include <api/fm_api_init.h>
#include <common/fm_errno.h>
#include <api/fm_api_regs.h>
#include <api/fm_api_routing.h>
#include <api/fm_api_nexthop.h>
#include <api/fm_api_event_mac_maint.h>
#include <api/fm_api_event_types.h>
#include <api/fm_api_event_mgmt.h>
#include <api/fm_api_buffer.h>
#include <api/fm_api_packet.h>
#include <api/fm_api_attr.h>

/* microcode generated public attributes */
#include <api/fm_api_uc_attr.h>

#include <api/fm_api_vlan.h>
#include <api/fm_api_stp.h>
#include <api/fm_api_addr.h>
#include <api/fm_api_lag.h>
#include <api/fm_api_multicast.h>
#include <api/fm_api_storm.h>
#include <api/fm_api_acl.h>
#include <api/fm_api_mapper.h>
#include <api/fm_api_stats.h>
#include <api/fm_api_parity.h>
#include <api/fm_api_port.h>
#include <api/fm_api_portset.h>
#include <api/fm_api_mirror.h>
#include <api/fm_api_netlink.h>
#include <api/fm_api_qos.h>
#include <api/fm_api_ffu.h>
#include <api/fm_api_pkt.h>
#include <api/fm_api_policer.h>
#include <api/fm_api_lbg.h>
#include <api/fm_api_swag.h>
#include <api/fm_api_stacking.h>
#include <api/fm_api_sflow.h>
#include <api/fm_api_trigger.h>
#include <api/fm_api_replication.h>
#include <api/fm_api_rbridge.h>
#include <api/fm_api_tunnel.h>
#include <api/fm_api_nat.h>
#include <api/fm_api_vn.h>
#include <api/fm_api_flow.h>
#include <api/fm_api_sched.h>


#endif /* __FM_FM_API_H */
