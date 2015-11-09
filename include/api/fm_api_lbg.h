/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lbg.h
 * Creation Date:   March 31, 2008
 * Description:     Prototypes for managing load balancing groups.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_LBG_H
#define __FM_FM_API_LBG_H

/** The number of bins to use in a load balancing group distribution map on
 *  FM3000/FM4000 devices.
 *  \ingroup constSystem */
#define FM_LBG_MAX_BINS_PER_DISTRIBUTION  1000

/** For FM6000 devices, the default number of bins to use in 
 *  mapped and redirect modes.
 *  \ingroup constSystem */
#define FM_FM6000_LBG_DEFAULT_BINS_PER_DISTRIBUTION 512

/** For FM10000 devices, the default number of bins to use in 
 *  mapped and redirect modes.
 *  \ingroup constSystem */
#define FM_FM10000_LBG_DEFAULT_BINS_PER_DISTRIBUTION 128


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmCreateLBGExt''
 *  and ''fmCreateStackLBGExt''. Provides 
 *  parameters for the load balancing group to be 
 *  created.
 **************************************************/
typedef struct _fm_LBGParams
{
    /** The number of bins to be used.
     *                                                                  \lb\lb
     *  For FM3000/FM4000 devices, this number is used for
     *  ''FM_LBG_MODE_MAPPED'' to determine the number of internal
     *  bins used for the LBG. If the value is set to 0, the number
     *  of bins will be selected automatically to the optimal value
     *  based on the number of ports in the LBG. If the number is
     *  between 1 and 16, only Glort hashing will be used to balance the 
     *  traffic. If a value greater then 16 is specified, hashing will be 
     *  done at the ARP (ECMP) level and Glort level. 
     *                                                                  \lb\lb
     *  For FM6000 devices, this number is used for both
     *  ''FM_LBG_MODE_REDIRECT'' and ''FM_LBG_MODE_MAPPED''. In
     *  both cases it determines the number of internal bins used,
     *  although in the former case, the bins are not directly
     *  configured by the user. The supported number of bins is both 
     *  limited and sparse.  Valid sizes can be encoded via the expression
     *                                                                  \lb\lb
     *  ((2 * select) + 1) << shift
     *                                                                  \lb\lb
     *  where select is a 3 bit quantity, and shift is 4 bit quantity.  An 
     *  unsupported value will result in using the next smallest supported 
     *  value.  The attribute ''FM_LBG_DISTRIBUTION_MAP_SIZE'' can be used 
     *  to retrieve the chosen value.
     *  
     *  For FM10000 devices, this number is used for modes
     *  ''FM_LBG_MODE_REDIRECT'' , ''FM_LBG_MODE_MAPPED'', and
     *  ''FM_LBG_MODE_MAPPED_L234HASH''.
     *                                                                  \lb\lb
     *  For ''FM_LBG_MODE_REDIRECT'' and ''FM_LBG_MODE_MAPPED'', if the
     *  value is set to 0, the number of bins will be equal to
     *  ''FM_FM10000_LBG_DEFAULT_BINS_PER_DISTRIBUTION''. Valid sizes are any
     *  integer between 1 and 16 bins. Above 16, the number of bins must be a
     *  power of 2 and must not exceed 4096.
     *                                                                  \lb\lb
     *  For ''FM_LBG_MODE_MAPPED_L234HASH'', valid sizes are any integer
     *  between 1 and 16 bins. */
    fm_int     numberOfBins;

    /** Indicates which load balancing group mode to use. This value overrides
     *  the mode selected using the switch attribute ''FM_LBG_MODE''. */
    fm_LBGMode mode;

} fm_LBGParams;

/**************************************************/
/** \ingroup typeStruct
 *  This type is used for the ''FM_LBG_DISTRIBUTION_MAP''
 *  load balancing group attribute..
 **************************************************/
typedef struct _fm_LBGDistributionMap
{
    /** An array of length ''FM_LBG_MAX_BINS_PER_DISTRIBUTION'', each 
     *  element represents a bin and each bin contains a port number. The 
     *  load balanced traffic will be conceptually distributed among the 
     *  bins and forwarded out the port associated with each bin. The 
     *  number of bins is generally larger than the number of ports to 
     *  provide a finer granularity for specifying the distribution; ports 
     *  with more bins assigned to them will carry a higher percentage of 
     *  the flows than ports with fewer bins assigned to them.
     *                                                                  \lb\lb
     *  Port numbers must be members of the LBG. More than one bin may be 
     *  assigned to a particular port. Not all ports in the LBG have to 
     *  be assigned to bins. Unassigned ports are essentially held in 
     *  standby for failover. Traffic hashed to a particular port can 
     *  be redistributed to other ports by reassigning that port's bins 
     *  to the other ports.
     *                                                                  \lb\lb
     *  Bins are mapped to hashed entities in the hardware. The mapping 
     *  may not be one-to-one, so that the distribution represented by the 
     *  distribution map may be only approximated in the hardware. */
    fm_int bin[FM_LBG_MAX_BINS_PER_DISTRIBUTION];

} fm_LBGDistributionMap;

/**************************************************/
/** \ingroup typeStruct
 *  This type is used for the 
 *  ''FM_LBG_DISTRIBUTION_MAP_RANGE'' load balancing 
 *  group attribute..
 **************************************************/
typedef struct _fm_LBGDistributionMapRange
{
    /** The first bin in the range to set. */
    fm_int firstBin;

    /** The number of bins in the range to set. */
    fm_int numberOfBins;

    /** Pointer to a caller-allocated array of size numberOfBins containing 
     *  logical port numbers, such that the i'th entry contains the logical 
     *  port number to assign to bin (firstBin + i). */
    fm_int *ports;

} fm_LBGDistributionMapRange;


/**************************************************/
/** \ingroup typeEnum
 *
 *  LBG member type used in ''fm_LBGDistributionMapRangeV2''
 *  for setting the LBG attribute
 *  ''FM_LBG_DISTRIBUTION_MAP_RANGE_V2''.
 **************************************************/
typedef enum
{
    /** Member type is Logical Port. */
    FM_LBG_MEMBER_TYPE_PORT = 0,
 
    /** Member type is NextHop L2 address. */
    FM_LBG_MEMBER_TYPE_MAC_ADDR,

    /** Member type is Multicast group. */
    FM_LBG_MEMBER_TYPE_MCAST_GROUP,
 
    /** Member type is ''FM_LBG_MODE_MAPPED_L234HASH'' load-balancing
     *  group. */
    FM_LBG_MEMBER_TYPE_L234_LBG,

    /** Member type is Tunnel */
    FM_LBG_MEMBER_TYPE_TUNNEL,

    /** UNPUBLISHED: For internal use only. */
    FM_LBG_MEMBER_TYPE_MAX
 
} fm_LBGMemberType;


/**************************************************/
/** \ingroup typeStruct
 *  Represents an LBG member of type Logical Port                     .
 *  or NextHop L2 address.
 *  This type is used in ''fm_LBGDistributionMapRangeV2''.
 **************************************************/
typedef struct _fm_LBGMember
{
    /** Type of the LBG member. */
    fm_LBGMemberType  lbgMemberType;
 
    /** Logical port corresponding to physical port or virtual port.
     *  Valid when lbgMemberType is ''FM_LBG_MEMBER_TYPE_PORT''. */
    fm_int            port;
 
    /** Egress Vlan of Next Hop. Valid when lbgMemberType is
     *  ''FM_LBG_MEMBER_TYPE_MAC_ADDR''. */
    fm_uint16         egressVlan;
 
    /** Destination MAC address of Next Hop. Valid when lbgMemberType is
     *  ''FM_LBG_MEMBER_TYPE_MAC_ADDR''. */
    fm_macaddr        dmac;
    
    /** Virtual Router Id of Next Hop. Valid when lbgMemberType is
     *  ''FM_LBG_MEMBER_TYPE_MAC_ADDR''. May be ''FM_ROUTER_ANY'' or
     *  the router id of any Virtual Router. */
    fm_int            vrid;

    /** Mcast group handle of L2 and L3 multicast group.
     *  Valid when lbgMemberType is ''FM_LBG_MEMBER_TYPE_MCAST_GROUP''.
     *  For L3 multicast group, a packet hitting this bin will be 
     *  treated as a routed packet, and the packet will be modified
     *  based on the router/port configuration. */
    fm_int            mcastGroup;

    /** Handle of ''FM_LBG_MODE_MAPPED_L234HASH'' load balancing group.
     *  Valid when lbgMemberType is ''FM_LBG_MEMBER_TYPE_L234_LBG''. */
    fm_int            l234Lbg;

    /** Tunnel group of Next Hop. Valid when lbgMemberType is 
     *  ''FM_LBG_MEMBER_TYPE_TUNNEL''. */
    fm_int            tunnelGrp;

    /** Tunnel rule of NextHop. Valid when lbgMemberType is 
     *  ''FM_LBG_MEMBER_TYPE_TUNNEL''. */
    fm_int            tunnelRule;

 } fm_LBGMember;


/**************************************************/
/** \ingroup typeStruct
 *  This type is used for the
 *  ''FM_LBG_DISTRIBUTION_MAP_RANGE_V2'' load balancing
 *  group attribute.
 **************************************************/
typedef struct _fm_LBGDistributionMapRangeV2
{
    /** The first bin in the range to set. */
    fm_int firstBin;
 
    /** The number of bins in the range to set. */
    fm_int numberOfBins;
 
    /** Pointer to a caller-allocated array of size numberOfBins containing
     *  fm_LBGMember, such that the i'th entry contains the LBG member
     *  to assign to bin (firstBin + i). */
    fm_LBGMember *lbgMembers;
 
} fm_LBGDistributionMapRangeV2;


/****************************************************************************/
/** \ingroup constLBGAttr
 *
 *  Load Balancing Group Attributes, used as an argument to 
 *  ''fmSetLBGAttribute'' and ''fmGetLBGAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 *                                                                      \lb\lb
 *  Load balancing groups are not supported on FM2000 devices.
 ****************************************************************************/
enum _fm_lbgAttr
{
    /** Type ''fm_LBGMode'':  This attribute is read-only and is used
     *  to retrieve the LBG mode of this load balancing group. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_LBG_GROUP_MODE,

    /** Type ''fm_LBGDistributionMap'':  Indicates the distribution
     *  map used by load balancing groups when the mode is 
     *  ''FM_LBG_MODE_MAPPED''. Not all bins in the map are used, see the 
     *  ''FM_LBG_DISTRIBUTION_MAP_SIZE'' attribute. 
     *                                                                  \lb\lb
     *  If an ACL is actively routing traffic to an LBG and the distribution 
     *  map is changed, flows associated with bins that get changed will be 
     *  distributed to the the new ports in a disruptive fashion. 
     *                                                                  \lb\lb
     *  In stacking configuration, it is the responsibility of the caller to
     *  populate the bins with ports consistent for all switches. A recommended
     *  method is to sort the ports in glort order, before populating the 
     *  bins. 
     *                                                                  \lb\lb
     *  On FM6000 and FM10000 devices, use ''FM_LBG_DISTRIBUTION_MAP_RANGE''.
     *
     *  \chips  FM3000, FM4000 */
    FM_LBG_DISTRIBUTION_MAP,

    /** Type ''fm_LBGDistributionMapRange'':  Indicates the distribution
     *  for a range of bins and is used by load balancing groups when the 
     *  mode is ''FM_LBG_MODE_MAPPED''. 
     *                                                                  \lb\lb
     *  If an ACL is actively routing traffic to an LBG and the distribution 
     *  map is changed, flows associated with the bins that get changed will be 
     *  distributed to the the new ports in a disruptive fashion. 
     *                                                                  \lb\lb
     *  In stacking configuration, it is the responsibility of the caller to
     *  populate the bins with ports consistent for all switches. A recommended
     *  method is to sort the ports in glort order, before populating the 
     *  bins. 
     *                                                                  \lb\lb
     *  On FM3000/FM4000 devices, use ''FM_LBG_DISTRIBUTION_MAP''.
     *
     *  \chips  FM6000, FM10000 */
    FM_LBG_DISTRIBUTION_MAP_RANGE,

    /** Type fm_int:  Indicates the number of used bins in the
     *  ''FM_LBG_DISTRIBUTION_MAP'' attribute or the actual number of 
     *  allocated bins as a result of specifying numberOfBins in 
     *  ''fm_LBGParams'' when ''fmCreateLBGExt'' or ''fmCreateStackLBGExt''
     *  is called.  This attribute is read-only. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_DISTRIBUTION_MAP_SIZE,

    /** Type fm_int: Indicates the state of the LBG.  Valid values are
     *  FM_LBG_STATE_ACTIVE and FM_LBG_STATE_INACTIVE.  
     *                                                                  \lb\lb
     *  Member ports may not be added to or deleted from an LBG while it is
     *  in the active state.
     *                                                                  \lb\lb
     *  Traffic cannot flow through an LBG while it is in the inactive state.
     *                                                                  \lb\lb
     *  The state of an LBG's member ports may be changed while the LBG is in
     *  the active state. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_STATE,

    /** Type fm_int: Indicates the method used to handle port failover when
     *  the LBG is in ''FM_LBG_MODE_REDIRECT'' mode.
     *                                                                  \lb\lb
     *  FM_LBG_REDIRECT_PORT indicates that traffic is redirected to a specific
     *  port set by the member port's ''FM_LBG_PORT_REDIRECT_TARGET'' attribute.
     *                                                                  \lb\lb
     *  FM_LBG_REDIRECT_STANDBY indicates that traffic is hashed across
     *  all standby ports.
     *                                                                  \lb\lb
     *  FM_LBG_REDIRECT_ALL_PORTS indicates that traffic destined to a port
     *  in failover is hashed to all active and standby ports.
     *                                                                  \lb\lb
     *  FM_LBG_REDIRECT_PREFER_STANDBY indicates that traffic destined to a
     *  port in failover is first sent directly to a standby port if it has
     *  not already been used, and once all standby ports are in use, to
     *  hash across all ports.
     *                                                                  \lb\lb
     *  Changing this attribute will cause all flows to potentially rehash. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_REDIRECT_METHOD,

    /** Type ''fm_LBGDistributionMapRangeV2'':  Indicates the distribution
     *  for a range of bins and is used by load balancing groups when the 
     *  mode is ''FM_LBG_MODE_MAPPED''. Each bin contains ''fm_LBGMember''
     *  which can be of certain types of logical port or NextHop DMAC,
     *  Egress Vlan. See ''fm_LBGMember'' for more details.
     *                                                                  \lb\lb
     *  If an ACL is actively routing traffic to an LBG and the distribution 
     *  map is changed, flows associated with the bins that get changed will be 
     *  distributed to the the new ports in a disruptive fashion. 
     *                                                                  \lb\lb
     *  In a stacking configuration, it is the responsibility of the caller
     *  to populate the bins with logical ports consistently for all
     *  switches. A recommended method is to sort the ports in glort order,
     *  before populating the bins.
     *                                                                  \lb\lb
     *  On FM3000/FM4000 devices, use ''FM_LBG_DISTRIBUTION_MAP''.
     *                                                                  \lb\lb
     *  On FM6000 devices, use ''FM_LBG_DISTRIBUTION_MAP_RANGE''.
     *
     *  \chips  FM10000 */
    FM_LBG_DISTRIBUTION_MAP_RANGE_V2,

    /** Type fm_int:  This is a read-only attribute to return the logical
     *  port associated with LBG. This attribute is valid only for  
     *  ''FM_LBG_MODE_MAPPED_L234HASH''. The LBG logical port can be used in
     *  other supported subsystems (Address management, ACL, etc).
     *
     *  \chips  FM10000 */
    FM_LBG_LOGICAL_PORT,

    /** UNPUBLISHED: For internal use only. */
    FM_LBG_ATTRIBUTE_MAX,

};

/****************************************************************************/
/** \ingroup constLBGPortAttr
 *
 *  Load Balancing Group Port Attributes, used as an argument to 
 *  ''fmSetLBGPortAttribute'' and ''fmGetLBGPortAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 *                                                                      \lb\lb
 *  Load balancing groups are not supported on FM2000 devices.
 ****************************************************************************/
enum _fm_lbgPortAttr
{
    /** Type fm_int:  Indicates a port's role when active and inactive.  This 
     *  attribute is only used when ''FM_LBG_MODE'' is set to either 
     *  ''FM_LBG_MODE_NPLUS1'' or ''FM_LBG_MODE_REDIRECT''.  Valid values
     *  are:
     *                                                                  \lb\lb
     *  FM_LBG_PORT_ACTIVE - indicates that traffic is distributed to the port
     *  and the port can serve as a redirect target for another port.
     *                                                                  \lb\lb
     *  FM_LBG_PORT_FAILOVER - indicates that traffic is not distributed to
     *  the port and its intended traffic is redirected according to the
     *  ''FM_LBG_PORT_REDIRECT_TARGET'' attribute.
     *                                                                  \lb\lb
     *  FM_LBG_PORT_STANDBY - indicates that traffic is not distributed to the
     *  port until it serves as a redirect target for another port.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_PORT_MODE,

    /** Type fm_int:  Indicates what happens to this port's traffic when the 
     *  port becomes inactive.  Value is the logical port number of another 
     *  member port of the LBG, indicating the port to which this port's 
     *  traffic will be redirected when the ''FM_LBG_PORT_MODE'' attribute is 
     *  set to FM_LBG_PORT_FAILOVER.  Note that the redirect target may 
     *  itself be active.
     *                                                                  \lb\lb
     *  This attribute is only valid when ''FM_LBG_MODE'' is 
     *  ''FM_LBG_MODE_REDIRECT'' and the ''FM_LBG_REDIRECT_METHOD'' attribute
     *  of the LBG is set to FM_LBG_REDIRECT_PORT. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_LBG_PORT_REDIRECT_TARGET,

    /** UNPUBLISHED: For internal use only. */
    FM_LBG_PORT_ATTRIBUTE_MAX,

};


/***************************************************
 * These attribute constants are used for setting
 * the FM_LBG_STATE attribute.
 **************************************************/
#define FM_LBG_STATE_ACTIVE               0
#define FM_LBG_STATE_INACTIVE             1

/***************************************************
 ** \ingroup constLBGPortAttr
 *
 * These attribute constants are used for setting
 * the FM_LBG_PORT_MODE port attribute.
 **************************************************/
enum _fm_lbgPortMode
{
    /** The port is active and can forward traffic. */
    FM_LBG_PORT_ACTIVE = 0,

    /** The port is down or otherwise unavailable. In certain modes, this
     *  may result in this port's traffic being redirected to other ports.
     */
    FM_LBG_PORT_FAILOVER,

    /** This port normally does not forward traffic, but is available for
     *  use if other ports become unavailable. This is the default port
     *  mode. */
    FM_LBG_PORT_STANDBY,

    /** This port is not available for use. */
    FM_LBG_PORT_INACTIVE,

    /** UNPUBLISHED: For internal use only */
    FM_LBG_PORT_MODE_MAX

};

/***************************************************
 * These attribute constants are used for setting
 * the FM_LBG_REDIRECT_METHOD attribute.
 **************************************************/
#define FM_LBG_REDIRECT_PORT              0
#define FM_LBG_REDIRECT_STANDBY           1
#define FM_LBG_REDIRECT_ALL_PORTS         2
#define FM_LBG_REDIRECT_PREFER_STANDBY    3

/* Create and delete load balancing groups */
fm_status fmCreateLBG(fm_int sw, fm_int *lbgNumber);
fm_status fmCreateLBGExt(fm_int sw, fm_int *lbgNumber, fm_LBGParams *params);
fm_status fmDeleteLBG(fm_int sw, fm_int lbgNumber);


/* Manage members of load balancing groups */
fm_status fmAddLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);
fm_status fmDeleteLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);


/* Manage getting the list of load balancing groups */
fm_status fmGetLBGList(fm_int sw, fm_int *numLBGs, fm_int *lbgList, fm_int max);
fm_status fmGetLBGFirst(fm_int sw, fm_int *firstLbgNumber);
fm_status fmGetLBGNext(fm_int sw, fm_int currentLbgNumber,
                       fm_int *nextLbgNumber);


/* Manage getting the list of load balancing group members */
fm_status fmGetLBGPortList(fm_int sw, fm_int lbgNumber,
                           fm_int *numPorts, fm_int *portList, fm_int max);
fm_status fmGetLBGPortFirst(fm_int sw, fm_int lbgNumber, fm_int *firstLbgPort);
fm_status fmGetLBGPortNext(fm_int sw, fm_int lbgNumber, fm_int currentLbgPort,
                           fm_int *nextLbgPort);


/* Handle setting attributes on load balancing groups */
fm_status fmSetLBGAttribute(fm_int sw, fm_int lbgNumber,
                            fm_int attr, void *value);
fm_status fmGetLBGAttribute(fm_int sw, fm_int lbgNumber,
                            fm_int attr, void *value);


/* Handle setting attributes on load balancing group members */
fm_status fmSetLBGPortAttribute(fm_int sw, fm_int lbgNumber, fm_int port,
                                fm_int attr, void *value);
fm_status fmGetLBGPortAttribute(fm_int sw, fm_int lbgNumber, fm_int port,
                                fm_int attr, void *value);


/* Debug functions */
fm_status fmDbgDumpLBG(fm_int sw, fm_int lbg);


#endif /* __FM_FM_API_LBG_H */
