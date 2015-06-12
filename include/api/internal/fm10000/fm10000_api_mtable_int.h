/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mtable_int.h
 * Creation Date:   Feb 26, 2014
 * Description:     Contains functions dealing with the raw management
 *                  of the IP multicast table.
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

#ifndef __FM_FM10000_API_MTABLE_INT_H
#define __FM_FM10000_API_MTABLE_INT_H

/***************************************************
 * Maintains state information about the multicast
 * table.
 **************************************************/
typedef struct _fm10000_mtableInfo
{
    /* a boolean indicating if the MTABLE was initialized */
    fm_bool       isInitialized;

    /* maps dest table index to group logical port */
    fm_tree       groups;

    /* maps replicationGroup to dest table index */
    fm_tree       mtableDestIndex;

    /* A bit array representing the usage status of the MCAST_DEST_TABLE */
    fm_bitArray   destTableUsage;

    /* entry count */
    fm_int        destTableCount;

    /* A bit array representing the usage status of the MCAST_LEN_TABLE */
    fm_bitArray   lenTableUsage;

    /* entry count */
    fm_int        lenTableCount;

    /* A bit array representing the quarantined entries in MCAST_LEN_TABLE */
    fm_bitArray   clonedLenEntriesBitArray;

    /* A bit array representing the usage status of the MCAST_VLAN_TABLE */
    fm_bitArray   vlanTableUsage;

    /* entry count */
    fm_int        vlanTableCount;

    /* A bit array representing the quarantined entries in MCAST_VLAN_TABLE */
    fm_bitArray   clonedEntriesBitArray;

    /* entry count */
    fm_int        clonedEntriesCount;

    /* current watermark for cloned entries */
    fm_int        clonedEntriesWatermark;

    /* watermark expressed as a percentage of the available space in MCAST_VLAN_TABLE */
    fm_int        watermarkPercentage;

    /* A tree (key (vlan, port) pair) holding a tree (key dglort)
     * that holds a list of entry instances. */
    fm_tree entryList;

    /* A tree holding the number of listeners per (group, port) pair */
    fm_tree listenersCount;

    /* A tree holding a list of entry instances for each valid lenTableIndex. The same object
     * in entryList is used here. */
    fm_tree entryListPerLenIndex;      

    /* current epoch value */
    fm_byte       epoch;

    /* whether or not MTable cleanup is ongoing */
    fm_bool       cleanupOnGoing;

    /* how many time we try to verify if MTable cleanup is over */
    fm_int        cleanupRetries;

    /* how many cleanup cycles of increasing length we tried */
    fm_byte       cleanupCycles;

    /* the cleanup period in terms of fast maintenance time units */
    fm_uint64     cleanupPeriod;

    /* Whether or not to attempt automatic cleanup */
    fm_bool       autoCleanup;

} fm10000_mtableInfo;


/* Wraps the MTable entry list mapping (port, vlan) -> list < entries > */
typedef struct
{
    /* Multicast Group ID */
    fm_int    mcastGroup;

    /* Replication Group ID */
    fm_int    repliGroup;

    /* entry index in the MCAST_LEN_TABLE */
    fm_int    lenTableIndex;

    /* entry index in the MCAST_VLAN_TABLE */
    fm_int    vlanEntryIndex;

} fm10000_entryListWrapper;

/* Internal structure which should passed as argument for adding and deleting listener. */
typedef struct
{
    /* The VLAN to which the multicast should be sent. */
    fm_uint16 vlan;

    /* The logical port to which the multicast should be sent. */
    fm_int    port;

    /* The DGLORT to which the multicast should be sent when dglortUpdate
     *  is enabled. */
    fm_uint16 dglort;

    /* Flag to determine whether VLAN to be updated. */
    fm_bool   vlanUpdate;

    /* Flag to determine whether DGLORT to be updated. */
    fm_bool   dglortUpdate;

} fm10000_mtableEntry;

fm_status fm10000AllocateMTableDataStructures(fm_switch *switchPtr);

fm_status fm10000FreeMTableDataStructures(fm_switch *switchPtr);

fm_status fm10000MTableInitialize(fm_int sw);

fm_status fm10000FreeMTableResources(fm_int sw);

fm_status fm10000MTableEnableGroup(fm_int            sw,
                                   fm_int             mcastGroup,
                                   fm_mtableGroupType mcastGroupType,
                                   fm_uint16          vlanID,
                                   fm_int *           repliGroup,
                                   fm_int *           mcastDestIndex);

fm_status fm10000MTableDisableGroup(fm_int  sw,
                                    fm_int  mcastGroup,
                                    fm_int  repliGroup,
                                    fm_bool privateGroup);

fm_status fm10000MTableAddListener(fm_int               sw,
                                   fm_int               mcastGroup,
                                   fm_int               repliGroup,
                                   fm10000_mtableEntry  listener);

fm_status fm10000MTableDeleteListener(fm_int              sw,
                                      fm_int               mcastGroup,
                                      fm_int               repliGroup,
                                      fm10000_mtableEntry  listener,
                                      fm_bool              updateDestMask);

fm_status fm10000MTableUpdateListenerState(fm_int sw,
                                           fm_int vlan,
                                           fm_int port,
                                           fm_int state);

fm_status fm10000GetAvailableMulticastListenerCount(fm_int  sw,
                                                    fm_int *count);

fm_status fm10000MTablePeriodicMaintenance(fm_int sw);


fm_status fm10000DbgDumpMulticastVlanTable(fm_int sw);

fm_status fm10000MTableReserveEntry(fm_int sw, 
                                    fm_int group,
                                    fm_int mcastLogicalPort,
                                    fm_int *mcastDestIndex);

fm_status fm10000MTableFreeMcastGroupVlanEntries(fm_int sw, 
                                                 fm_int mcastGroup, 
                                                 fm_int mtableDestIndex);

fm_status fm10000MTableFreeDestTableEntry(fm_int sw, fm_int repliGroup);

#endif /* __FM_FM10000_API_MTABLE_INT_H */
