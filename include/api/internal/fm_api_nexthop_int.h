/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_nexthop_int.h
 * Creation Date: August 5, 2013
 * Description: header file for routing services.
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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

#ifndef __FM_FM_API_NEXTHOP_INT_H
#define __FM_FM_API_NEXTHOP_INT_H

/*****************************************************************************
 *
 * Route table entry state.
 * 
 ****************************************************************************/
typedef enum
{
    /** The next hop state is active. */
    FM_NEXT_HOP_STATE_UP = 0,

    /** The next hop satet is inactive. */
    FM_NEXT_HOP_STATE_DOWN,

    /** UNPUBLISHED: For internal use only. */
    FM_NEXT_HOP_STATE_MAX

} fm_nextHopState;


/*****************************************************************************
 *
 * Interface Address Entry
 *
 *****************************************************************************/
typedef struct _fm_intIpInterfaceAddressEntry
{
    /* IP Address associated with this interface. */
    fm_ipAddr addr;

    /* Linked-list next/previous pointers */
    FM_DLL_DEFINE_NODE(_fm_intIpInterfaceAddressEntry, nextAddr, prevAddr);

    /* Pointer to the interface to which this address entry belongs. */
    struct _fm_intIpInterfaceEntry * ifEntry;

    /* Tree of next-hops that are associated with this interface. */
    fm_customTree     nextHopTree;

} fm_intIpInterfaceAddressEntry;


#define FM_NEXTHOP_INDEX_UNSPECIFIED (-1)

/*****************************************************************************
 *
 * Interface Table
 *
 *****************************************************************************/
typedef struct _fm_intIpInterfaceEntry
{
    /* Interface number. */
    fm_int            interfaceNum;

    /* Vlan associated with this interface. */
    fm_uint16         vlan;

    /* State of the interface. */
    fm_interfaceState state;

    /* Pointer to the switch-model-specific extension structure. NULL if the
     * switch model does not require an extension. */
    void *            extension;

    /* Linked-list head/tail pointers for the IP address entries associated
     * with this interface. */
    FM_DLL_DEFINE_LIST(_fm_intIpInterfaceAddressEntry, firstAddr, lastAddr);

} fm_intIpInterfaceEntry;


/*****************************************************************************
 *
 * Internal ARP Entry
 *
 *****************************************************************************/
typedef struct _fm_intArpEntry
{
    /* ARP information. */
    fm_arpEntry             arp;

    /* Pointer to the switch structure. */
    fm_switch *             switchPtr;

    /* Pointer to the interface associated with this ARP. NULL if an
     * interface is not being used. */
    fm_intIpInterfaceEntry *ifEntry;

    /* Virtual Router ID. */
    fm_int                  vrid;

    /* State of the ARP entry. */
    fm_routeState           state;

    /* Tree of next-hops associated with this ARP entry. */
    fm_customTree           nextHopTree;

    /* Linked-list next/previous pointers. */
    FM_DLL_DEFINE_NODE(_fm_intArpEntry, nextArp, prevArp);

} fm_intArpEntry;


/*****************************************************************************
 *
 * Internal Next Hop
 *
 *****************************************************************************/
typedef struct _fm_intNextHop
{
    /* Switch number. */
    fm_int                         sw;

    /* Next-Hop Contents. */
    fm_ecmpNextHop                 nextHop;

    /* Pointer to the ECMP group which contains this next-hop. */
    struct _fm_intEcmpGroup *      ecmpGroup;

#if 0
    /* Index of the next hop in the EcmpGroup */
    fm_int                         hopIndex;
#endif

    /* Pointer to the ARP entry associated with this next-hop. NULL if the
     * next-hop type is not an ARP-type or if there is no ARP record for
     * the next-hop IP address. */
    fm_intArpEntry *               arp;

    /* Vlan number for this next-hop. Only used if ifEntry is NULL. Ignored
     * for raw next-hop types. */
    fm_uint16                      vlan;

    /* Prior vlan number. Used internally when a next-hop's vlan is changing. */
    fm_uint16                      oldVlan;

    /* Pointer to the interface entry associated with the interface IP address
     * in the ARP-style next-hop. NULL if the interface address is zero or for
     * non ARP-style next-hops. */
    fm_intIpInterfaceAddressEntry *interfaceAddressEntry;

    /* State of the route using this next-hop. */
    fm_routeState                  routeState;

    /* State of the next-hop. */
    fm_nextHopState                state;

    /* TRUE if the next-hop is usable. A next-hop is usable if the route state
     * is up and if either the interface is up or the vlan is a valid vlan
     * number. */
    fm_bool                        isUsable;

    /* Pointer to the switch-model-specific extension structure. NULL if the
     * switch model does not require an extension. */
    void *                         extension;

} fm_intNextHop;


/*****************************************************************************
 *
 * Internal ECMP Group
 *
 *****************************************************************************/
typedef struct _fm_intEcmpGroup
{
    /* Switch number */
    fm_int                        sw;

    /* Group ID/Handle */
    fm_int                        groupId;

    /* Maximum number of next-hops supported by this group. If the group is
     * a fixed-size group, this field contains the number of next-hops in
     * the group and will be equal to nextHopCount. */
    fm_int                        maxNextHops;

    /* TRUE if the ECMP group is a fixed-size group. */
    fm_bool                       fixedSize;

    /* TRUE if the group's next-hop width is known. For fixed-size ECMP
     * groups, this will always be TRUE, for normal ECMP groups, this will
     * be TRUE if and only if one or more next-hops have been added to
     * the group. */
    fm_bool                       isGroupWidthKnown;

    /* TRUE if the group's next-hops are wide next-hops, FALSE if they are
     * narrow next-hops. Note that all next-hops in an ECMP group must be
     * the same width. */
    fm_bool                       wideGroup;

    /* TRUE if the ECMP group is an MPLS ECMP group. */
    fm_bool                       mplsGroup;

    /* Number of next-hops in the ECMP group. */
    fm_int                        nextHopCount;

    /* Table of next-hop records. */
    fm_intNextHop **              nextHops;

    /* Tree of routes supported by this ECMP group. */
    fm_customTree                 routeTree;

    /* TRUE if the ECMP group is usable. An ECMP group is usable if one or
     * more of its next-hops are usable. */
    fm_bool                       isUsable;

    /* Pointer to the multicast group supported by this ECMP group. NULL
     * for ECMP groups that are not used with multicast routes. */
    struct _fm_intMulticastGroup *mcastGroup;

    /* Pointer to the switch-model-specific ECMP Group extension.
     * NULL if the switch model does not require an extension structure. */
    void *                        extension;

    /** Specifies the VLAN use for loopback suppression in the
     *  replication table */
    fm_uint16                     lbsVlan;

    /** number of nextHops in fixed ECMP group */
    fm_int                        numFixedEntries;    
} fm_intEcmpGroup;


/*****************************************************************************
 *
 * Internal Function Macros
 *
 *****************************************************************************/
#define fmInitInterfaceEntryLinkedLists(ifEntry) \
    FM_DLL_INIT_LIST(ifEntry, firstAddr, lastAddr)

#define fmGetFirstInterfaceAddress(ifEntry) \
    FM_DLL_GET_FIRST(ifEntry, firstAddr)

#define fmGetLastInterfaceAddress(ifEntry) \
    FM_DLL_GET_LAST(ifEntry, lastAddr)

#define fmGetNextInterfaceAddress(ifAddrEntry) \
    FM_DLL_GET_NEXT(ifAddrEntry, nextAddr)

#define fmGetPreviousInterfaceAddress(ifAddrEntry) \
    FM_DLL_GET_PREVIOUS(ifAddrEntry, prevAddr)

#define fmInsertInterfaceAddressAfter(ifEntry, curAddr, newAddr) \
    FM_DLL_INSERT_AFTER(ifEntry, firstAddr, lastAddr,            \
                        curAddr, nextAddr, prevAddr, newAddr)

#define fmInsertInterfaceAddressBefore(ifEntry, curAddr, newAddr) \
    FM_DLL_INSERT_BEFORE(ifEntry, firstAddr, lastAddr,            \
                         curAddr, nextAddr, prevAddr, newAddr)

#define fmAppendInterfaceAddress(ifEntry, addrPtr)   \
    FM_DLL_INSERT_LAST(ifEntry, firstAddr, lastAddr, \
                       addrPtr, nextAddr, prevAddr)

#define fmRemoveInterfaceAddress(ifEntry, addrPtr)   \
    FM_DLL_REMOVE_NODE(ifEntry, firstAddr, lastAddr, \
                       addrPtr, nextAddr, prevAddr)

/*****************************************************************************
 *
 * Internal Function Prototypes
 *
 *****************************************************************************/
fm_status fmInitializeNextHop(fm_int           sw,
                              fm_intEcmpGroup *group,
                              fm_intNextHop   *intNextHop,
                              fm_ecmpNextHop  *nextHop);
fm_intNextHop* fmFindNextHopInternal(fm_int           sw,
                                     fm_intEcmpGroup *group,
                                     fm_ecmpNextHop  *nextHop,
                                     fm_int          *hopIndexPtr);

fm_int    fmCompareInternalNextHops(const void *first,
                                    const void *second);
fm_int fmBasicTreeCompare(const void *first,
                          const void *second);
fm_status fmDeleteArpNextHopFromTrees(fm_int        sw,
                                      fm_intNextHop *intNextHop);
fm_status fmCreateECMPGroupInternal(fm_int                       sw,
                                    fm_int                       *groupId,
                                    fm_ecmpGroupInfo             *info,
                                    struct _fm_intMulticastGroup *group);
fm_status fmDeleteECMPGroupInternal(fm_int sw,
                                    fm_int groupId);
fm_status fmAddECMPGroupNextHopsInternal(fm_int          sw,
                                         fm_int          groupId,
                                         fm_int          numNextHops,
                                         fm_ecmpNextHop *nextHopList);
fm_status fmDeleteECMPGroupNextHopsInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_int          numNextHops,
                                            fm_ecmpNextHop *nextHopList);
fm_status fmReplaceECMPGroupNextHopInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_ecmpNextHop *oldNextHop,
                                            fm_ecmpNextHop *newNextHop);
fm_status fmUpdateEcmpGroupInternal(fm_int           sw,
                                    fm_intEcmpGroup *ecmpGroup);
fm_status fmSetECMPGroupNextHopsInternal(fm_int          sw,
                                         fm_int          groupId,
                                         fm_int          firstIndex,
                                         fm_int          numNextHops,
                                         fm_ecmpNextHop *nextHopList);

fm_status fmGetECMPGroupNextHopListInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_int         *numNextHops,
                                            fm_ecmpNextHop *nextHopList,
                                            fm_int          max);
fm_status fmGetECMPGroupNextHopUsedInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_ecmpNextHop *nextHop,
                                            fm_bool        *used,
                                            fm_bool         resetFlag);
fm_status fmFindInterface(fm_int                 sw,
                          fm_ipAddr              *interfaceAddr,
                          fm_intIpInterfaceEntry **ifEntry);
fm_status fmGetInterface(fm_int                   sw,
                         fm_int                   interface,
                         fm_intIpInterfaceEntry **ppIfEntry);
fm_uint16 fmGetInterfaceVlan(fm_int     sw,
                             fm_ipAddr  *ifAddr,
                             fm_uint16  alternateVlan);
fm_status fmFindArpEntry(fm_int         sw,
                         fm_ipAddr      *arpAddr,
                         fm_uint16      vlan,
                         fm_intArpEntry **arpEntry);
fm_int fmCompareArps(const void *first,
                     const void *second);
fm_int fmCompareInternalArps(const void *first,
                             const void *second);


fm_status fmGetARPEntryUsedInternal(fm_int       sw,
                                    fm_arpEntry  *arp,
                                    fm_bool      *used,
                                    fm_bool      resetFlag);

#endif  /* __FM_FM_API_NEXTHOP_INT_H */
