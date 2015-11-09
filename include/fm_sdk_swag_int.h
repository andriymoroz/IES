/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_sdk_swag_int.h
 * Creation Date:   April 15, 2008
 * Description:     Wrapper file to include all needed switch-aggregate
 *                  internal files. 
 *                  For switch-aggregate-specific SDK internal use only.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM_SDK_SWAG_INT_H
#define __FM_FM_SDK_SWAG_INT_H

#include <fm_sdk_int.h>

#include <api/internal/swag/fmSWAG_api_routing_int.h>
#include <api/internal/swag/fmSWAG_api_switch_int.h>
#include <api/internal/swag/fmSWAG_api_port_int.h>
#include <api/internal/swag/fmSWAG_api_mirror_int.h>
#include <api/internal/swag/fmSWAG_api_multicast_int.h>
#include <api/internal/swag/fmSWAG_api_acl_int.h>
#include <api/internal/swag/fmSWAG_api_qos_int.h>
#include <api/internal/swag/fmSWAG_api_replication_int.h>
#include <api/internal/swag/fmSWAG_api_stats_int.h>
#include <api/internal/swag/fmSWAG_api_storm_int.h>
#include <api/internal/swag/fmSWAG_api_triggers_int.h>
#include <api/internal/swag/fmSWAG_api_portset_int.h>
#include <api/internal/swag/fmSWAG_api_nexthop_int.h>
#include <api/internal/swag/fmSWAG_api_tunnel_int.h>
#include <api/internal/swag/fmSWAG_api_nat_int.h>
#include <api/internal/swag/fmSWAG_api_flow_int.h>
#include <api/internal/swag/fmSWAG_api_lbg_int.h>
#include <api/internal/swag/fmSWAG_api_vn_int.h>
#include <api/internal/swag/fmSWAG_api_mailbox_int.h>
#include <api/internal/swag/fmSWAG_api_parity_int.h>


/* fake variable to remove precompiled header warning that file doesn't
 * contain any code.
 */
extern int fake_variable_1;

#endif /* __FM_FM_SDK_SWAG_INT_H */
