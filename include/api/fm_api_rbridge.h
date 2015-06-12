/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_rbridge.h
 * Creation Date:   August 31, 2012
 * Description:     Constants for attributes and attribute values
 *
 * Copyright (c) 2005 - 2012, Intel Corporation
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

#ifndef __FM_FM_API_RBRIDGE_H
#define __FM_FM_API_RBRIDGE_H

/** Maximum number of distribution tree        \ingroup constSystem */
#define FM_MAX_DISTRIBUTION_TREE    256

/** Maximum number of remote RBridge supported \ingroup constSystem */
#define FM_MAX_REMOTE_RBRIDGE       (4096 - FM_MAX_DISTRIBUTION_TREE)

/* First Distribution Tree index */
#define FM_FIRST_DISTRIBUTION_TREE_INDEX    0

/* First Distribution Tree index */
#define FM_FIRST_RBRIDGE_INDEX              (0 + FM_MAX_DISTRIBUTION_TREE)

/* Max Distribution Tree Hop Count */
#define FM_MAX_HOP_COUNT   0x3f


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a structure containing information that describes a
 * remote RBridge.
 * 
 * Referenced by:
 * ''fmCreateRBridge'',
 * ''fmGetRBridgeEntry'',
 * ''fmGetRBridgeFirst'',
 * ''fmGetRBridgeNext'',
 * ''fmUpdateRBridgeEntry''.
 ****************************************************************************/
typedef struct _fm_remoteRBridge
{
    /** NextHop RBridge MAC address used to reach this RBridge. */
    fm_macaddr    address;
   
    /** Remote RBridge nickname. */
    fm_uint16     egressNick;
   
} fm_remoteRBridge;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a structure containing information that describes a
 * Distribution Tree.
 * 
 * Referenced by:
 * ''fmCreateRBridgeDistTree'',
 * ''fmGetRBridgeDistTree'',
 * ''fmGetRBridgeDistTreeFirst'',
 * ''fmGetRBridgeDistTreeNext'',
 * ''fmUpdateRBridgeDistTree''.
 ****************************************************************************/
typedef struct _fm_distTree
{
    /** Hop Count for this specific distribution tree. */
    fm_uint16   hopCnt;
   
    /** Nickname associated with this distribution tree. */
    fm_uint16   nick;
   
} fm_distTree;


fm_status fmCreateRBridge(fm_int            sw, 
                          fm_remoteRBridge *rbridge, 
                          fm_int *          tunnelId);

fm_status fmDeleteRBridge(fm_int sw, fm_int tunnelId);

fm_status fmUpdateRBridgeEntry(fm_int            sw,
                               fm_int            tunnelId,
                               fm_remoteRBridge *rbridge);

fm_status fmCreateRBridgeDistTree(fm_int       sw, 
                                  fm_distTree *distTree, 
                                  fm_int *     tunnelId);

fm_status fmDeleteRBridgeDistTree(fm_int sw, fm_int tunnelId);

fm_status fmUpdateRBridgeDistTree(fm_int       sw,
                                  fm_int       tunnelId,
                                  fm_distTree *distTree);

fm_status fmGetRBridgeEntry(fm_int            sw,
                            fm_int            tunnelId,
                            fm_remoteRBridge *rbridge);

fm_status fmGetRBridgeFirst(fm_int            sw,
                            fm_int *          tunnelId,
                            fm_remoteRBridge *rbridge);

fm_status fmGetRBridgeNext(fm_int            sw,
                           fm_int            currentTunnelId,
                           fm_int *          nextTunnelId,
                           fm_remoteRBridge *rbridge);

fm_status fmGetRBridgeDistTree(fm_int       sw,
                               fm_int       tunnelId,
                               fm_distTree *distTree);

fm_status fmGetRBridgeDistTreeFirst(fm_int       sw,
                                    fm_int *     tunnelId,
                                    fm_distTree *distTree);

fm_status fmGetRBridgeDistTreeNext(fm_int       sw,
                                   fm_int       currentTunnelId,
                                   fm_int *     nextTunnelId,
                                   fm_distTree *distTree);

fm_status fmSetRBridgePortHopCount(fm_int sw, fm_int port, fm_uint32 value);

fm_status fmGetRBridgePortHopCount(fm_int sw, fm_int port, fm_uint32 *value);

#endif /* __FM_FM_API_RBRIDGE_H */
