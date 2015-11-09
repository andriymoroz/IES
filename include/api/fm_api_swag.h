/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_swag.h
 * Creation Date:   Feb 20, 2007
 * Description:     Header file for services to manage switch aggregates and
 *                  switch-aggregate topologies.
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

#ifndef __FM_FM_API_SWAG_H
#define __FM_FM_API_SWAG_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/**************************************************
 * Logical port numbers
 **************************************************/

/* Reserved value. */
#define FM_RESERVED_LOGICAL_PORT        0

/* Range of values available for LAGs, multicast groups, etc. */
#define FM_MIN_INTERNAL_LOGICAL_PORT    0x8000
#define FM_MAX_INTERNAL_LOGICAL_PORT    0xFDFF

/* Mask used internally for in-band management of switches. */
#define FM_IN_BAND_MGMT_LOGICAL_PORT    0xFE00

/* Mask used for reporting events to the CPU. */
#define FM_EVENT_TO_CPU_LOGICAL_PORT    0xFF00

/** The minimum logical port number that may be used by the application in
 *  calls to API functions that require them.
 *  \ingroup constSystem */
#define FM_MIN_USER_LOGICAL_PORT        1

/** The maximum logical port number that may be used by the application in
 *  calls to API functions that require them.
 *  \ingroup constSystem */
#define FM_MAX_USER_LOGICAL_PORT        0x7fff


/**************************************************/
/** \ingroup typeEnum
 * Identifies the operation to be performed by
 * a switch aggregate's solver function (see
 * ''fm_swagTopologySolver'').
 **************************************************/
typedef enum
{
    /** Initialize the topology solver (during ''fmCreateSWAG'' processing) */
    FM_SOLVER_CMD_INITIALIZE = 0,

    /** Shut down the topology solver (during ''fmDeleteSWAG'' processing) */
    FM_SOLVER_CMD_SHUTDOWN,

    /** The switch aggregate has been put into the "up" state. */
    FM_SOLVER_CMD_SWAG_UP,

    /** The switch aggregate has been put into the "down" state. */
    FM_SOLVER_CMD_SWAG_DOWN,

    /** A switch has been added to the aggregate. */
    FM_SOLVER_CMD_ADD_SWITCH,

    /** A switch has been removed from the aggregate. */
    FM_SOLVER_CMD_REMOVE_SWITCH,

    /** A link has been added to the aggregate. */
    FM_SOLVER_CMD_ADD_LINK,

    /** A link has been removed from the aggregate. */
    FM_SOLVER_CMD_REMOVE_LINK,

    /** A switch in the switch aggregate has changed states from DOWN to UP. */
    FM_SOLVER_CMD_SWITCH_UP,

    /** A switch in the switch aggregate has changed states from UP to DOWN. */
    FM_SOLVER_CMD_SWITCH_DOWN,

    /** A port in the switch aggregate has changed states from UP to DOWN. */
    FM_SOLVER_CMD_PORT_DOWN,

    /** A port in the switch aggregate has changed states from DOWN to UP. */
    FM_SOLVER_CMD_PORT_UP,

    /** A link has been enabled. */
    FM_SOLVER_CMD_LINK_ENABLED,

    /** A link has been disabled. */
    FM_SOLVER_CMD_LINK_DISABLED,

    /** A port has been added to a VLAN. */
    FM_SOLVER_CMD_VLAN_PORT_ADD,

    /** A port has been deleted from a VLAN. */
    FM_SOLVER_CMD_VLAN_PORT_DELETE,

    /** A port is joining a multicast group. */
    FM_SOLVER_CMD_PORT_JOIN,

    /** A port is leaving a multicast group. */
    FM_SOLVER_CMD_PORT_LEAVE,

    /** An address is being added to the MAC table. */
    FM_SOLVER_CMD_ADD_MAC_ADDRESS,

    /** An address is being removed from the MAC table. */
    FM_SOLVER_CMD_DELETE_MAC_ADDRESS,

    /** An address has been learned on an external port. */
    FM_SOLVER_CMD_LEARNED_MAC_ADDRESS,

    /** An address has been aged on an external port. */
    FM_SOLVER_CMD_AGED_MAC_ADDRESS,

#if 0
    /** The topology of the switch aggregate is being changed. */
    FM_SOLVER_CMD_CHANGE_TOPOLOGY,
#endif

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_SOLVER_CMD_MAX

} fm_topologySolverCmd;


/**************************************************/
/** \ingroup typeEnum
 * Identifies the position that an individual switch
 * device may take in the topology of a switch
 * aggregate.
 **************************************************/
typedef enum
{
    /** The device's role in the topology is not specified (default). */
    FM_SWITCH_ROLE_UNDEFINED = 0,

    /** The device is a leaf switch in a fat tree topology. Some of the
     *  switch's ports will be externally-facing and some will connect to
     *  spine switches. No frames will be switched between spine ports -
     *  all switching will be between leaf and spine ports. */
    FM_SWITCH_ROLE_LEAF = 1,

    /** The device is a spine switch in a fat tree topology. All of the
     *  switch's ports will be internally-facing, connecting to
     *  leaf switches. Frames will be switched between the spine ports. */
    FM_SWITCH_ROLE_SPINE = 2,

    /** The device has both spine and leaf functionality.  Some of the
     *  switch's ports will be internally-facing, connecting to other
     *  spine or leaf switches, and some of the ports will be externally-
     *  facing.  Frames may be switched between spine ports as needed. */
    FM_SWITCH_ROLE_SPINE_LEAF = 3,

    /** The device is one of a set of stacked switches. */
    FM_SWITCH_ROLE_STACK = 4,

    /** The device is the SWAG switch itself. */
    FM_SWITCH_ROLE_SWAG = 5,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_SWITCH_ROLE_MAX

} fm_switchRole;


/**************************************************/
/** \ingroup typeEnum
 * Identifies the position a link (port-to-port
 * connection) takes in the topology of a switch
 * aggregate. See ''fm_swagLink''.
 **************************************************/
typedef enum
{
    /** The link is not defined. Used to indicate an unused element in
     *  an aggregate of switch aggregate links. */
    FM_SWAG_LINK_UNDEFINED = 0,

    /** The link connects the ports of two switch devices together. */
    FM_SWAG_LINK_INTERNAL,

    /** The link identifies a port on the switch aggregate that will connect
     *  externally. */
    FM_SWAG_LINK_EXTERNAL,

    /** The link is to a CPU. There may be more than one CPU links in a
     *  switch aggregate. */
    FM_SWAG_LINK_CPU,

    /** The link identifies a remote port belonging to a stacking switch
     *  outside of the switch aggregate. */
    FM_SWAG_LINK_REMOTE,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_SWAG_LINK_MAX

} fm_swagLinkType;


/**************************************************/
/** \ingroup typeEnum
 * Identifies the topology of a switch aggregate.
 **************************************************/
typedef enum
{
    /** The topology is not specified (default). */
    FM_SWAG_TOPOLOGY_UNDEFINED = 0,

    /** The topology is a ring of switches. Switches have links between them
     *  and external in a non-heirarchical arrangement. */
    FM_SWAG_TOPOLOGY_RING = 1,

    /** The topology is a fat tree. In this heirarchical configuration, some
     *  switches are leaf switches and some are spine switches with links
     *  between leaf and spine switches and external links only on leaf
     *  switches. */
    FM_SWAG_TOPOLOGY_FAT_TREE = 2,

    /** The topology is a mesh.  In this configuration, all switches
     *  are interconnected to all other switches in the aggregate.
     */
    FM_SWAG_TOPOLOGY_MESH = 3,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_SWAG_TOPOLOGY_MAX

} fm_swagTopology;


/****************************************************************************/
/** \ingroup constSwagLinkAttr
 *
 * Switch Aggregate Link Attributes, used as an argument to
 * ''fmSetSWAGLinkAttribute'' and ''fmGetSWAGLinkAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_swagLinkAttr
{
    /** Type fm_bool: The administrative state of a switch aggregate link:
     *  FM_ENABLED or FM_DISABLED (default). */
    FM_LINK_ATTR_ADMIN_STATE = 0,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_LINK_ATTR_MAX

};


/**************************************************/
/** \ingroup typeStruct
 * Describes the role of a link within the topology
 * of a switch aggregate. A link is a connection
 * between ports. For some links, this will be
 * two ports, one on each of two switch devices.
 * For other links, this will be a single port that
 * connects externally or to the CPU. Used as an
 * argument to ''fmAddSWAGLink''.
 *                                                                      \lb\lb
 * The logicalPort member of the structure
 * identifies the link within a switch aggregate's
 * collection of links. The partnerLogicalPort
 * also identifies the same link.
 **************************************************/
typedef struct _fm_swagLink
{
    /** Specifies the position of the link in the topology of the switch
     *  aggregate.  See 'fm_swagLinkType'. */
    fm_swagLinkType type;

    /** The logical port number of the port from which the link originates.
     *  This value identifies the originating physical switch/port uniquely
     *  within the set of switch/port tuples in the switch aggregate. The
     *  logical port number also uniquely identifies the link with the set of
     *  links in the switch aggregate. */
    fm_int          logicalPort;

    /** The switch ID of the underlying switch device from which the link
     *  originates. */
    fm_int          swId;

    /** The underlying switch device logical port number of the port from
     *  which the link originates. */
    fm_int          swPort;

    /** The logical port number of the port on which a 
     *  ''FM_SWAG_LINK_INTERNAL'' link terminates. This value identifies 
     *  the terminating physical switch/port uniquely within the set of 
     *  switch/port tuples in the switch aggregate.  Like the logicalPort member, 
     *  this member also uniquely identifies the link with the set of links 
     *  in the switch aggregate. */
    fm_int          partnerLogicalPort;

    /** The switch ID of the switch device on which a
     *  ''FM_SWAG_LINK_INTERNAL'' link terminates. */
    fm_int          partnerSwitch;

    /** The underlying switch device logical port number of the port
     *  on which a ''FM_SWAG_LINK_INTERNAL'' link terminates. */
    fm_int          partnerPort;

} fm_swagLink;


/**************************************************/
/** \ingroup intTypeStruct
 * Describes a trunk between switches.  A trunk may
 * be a single link or a link aggregation group
 * of links.
 **************************************************/
typedef struct _fm_swagTrunk
{
    /** Whether the trunk is enabled. */
    fm_bool         isEnabled;

    /** Whether the trunk is a Link-Agg Group */
    fm_bool         isLag;

    /** The switch-aggregate LAG ID number if the trunk is in a LAG. */
    fm_int          lagId;

    /** Logical port number associated with this trunk in the aggregate, either
     *  single port or LAG logical port.
     */
    fm_int          swagPort;

    /** Underlying switch number for this trunk. */
    fm_int          trunkSw;

    /** Underlying switch logical port number for this trunk. */
    fm_int          trunkPort;

    /** Underlying switch number for the trunk's destination switch. */
    fm_int          destSw;

    /** Specifies the type of the trunk in the topology of the switch
     *  aggregate.  See 'fm_swagLinkType'. */
    fm_swagLinkType trunkType;

} fm_swagTrunk;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver switch state change
 * event.
 **************************************************/
typedef struct _fm_topologySolverSwitchEvent
{
    /** The switch number whose state has changed. */
    fm_int sw;

} fm_topologySolverSwitchEvent;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver port state change
 * event.
 **************************************************/
typedef struct _fm_topologySolverPortEvent
{
    /** The port number whose state has changed. */
    fm_int logicalPort;

} fm_topologySolverPortEvent;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver link event.
 **************************************************/
typedef struct _fm_topologySolverLinkEvent
{
    /** The link associated with the event. */
    fm_swagLink link;

} fm_topologySolverLinkEvent;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver vlan port event.
 **************************************************/
typedef struct _fm_topologySolverVlanPortEvent
{
    /** The port number associated with the event. */
    fm_int    logicalPort;
    
    /** The VLAN associated with the event. */
    fm_uint16 vlan;

} fm_topologySolverVlanPortEvent;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver multicast group event.
 **************************************************/
typedef struct _fm_topologySolverMcastGroupEvent
{
    /** The multicast group associated with the event. */
    fm_int mcastGroup;
    
    /** The port number associated with the event. */
    fm_int logicalPort;

} fm_topologySolverMcastGroupEvent;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver MAC Address event.
 **************************************************/
typedef struct _fm_topologySolverMacAddrEvent
{
    /** The MA Table entry associated with the event. */
    fm_macAddressEntry *     userMacEntry;
    
    /** A flag that indicates whether the hardware should be updated with
     *  the new MA Table entry information. */
    fm_bool                  updateHw;

} fm_topologySolverMacAddrEvent;


/**************************************************/
/** \ingroup typeStruct
 * A union used by ''fm_topologySolverEvent''that
 * describes the specific details of the command
 * or event.
 **************************************************/
typedef union _fm_tsEventInfo
{
    /** Switch Event Structure, used for:                                   \lb
     *      FM_SOLVER_CMD_ADD_SWITCH                                        \lb
     *      FM_SOLVER_CMD_REMOVE_SWITCH                                     \lb
     *      FM_SOLVER_CMD_SWITCH_UP                                         \lb
     *      FM_SOLVER_CMD_SWITCH_DOWN. */
    fm_topologySolverSwitchEvent switchEvent;

    /** Port Event Structure, used for:                                     \lb
     *      FM_SOLVER_CMD_PORT_DOWN                                         \lb
     *      FM_SOLVER_CMD_PORT_UP. */
    fm_topologySolverPortEvent   portEvent;

    /** Link Event Structure, used for:                                     \lb
     *      FM_SOLVER_CMD_ADD_LINK                                          \lb
     *      FM_SOLVER_CMD_REMOVE_LINK                                       \lb
     *      FM_SOLVER_CMD_LINK_ENABLED                                      \lb
     *      FM_SOLVER_CMD_LINK_DISABLED. */
    fm_topologySolverLinkEvent   linkEvent;

    /** Vlan Port Event Structure, used for:                                \lb
     *      FM_SOLVER_CMD_VLAN_PORT_ADD                                     \lb
     *      FM_SOLVER_CMD_VLAN_PORT_DELETE. */
    fm_topologySolverVlanPortEvent vlanPortEvent;

    /** Multicast Group Event Structure, used for:                          \lb
     *      FM_SOLVER_CMD_PORT_JOIN                                         \lb
     *      FM_SOLVER_CMD_PORT_LEAVE. */
    fm_topologySolverMcastGroupEvent mcastEvent;

    /** Mac Address Event Structure, used for:                              \lb
     *      FM_SOLVER_CMD_ADD_MAC_ADDRESS                                   \lb
     *      FM_SOLVER_CMD_DELETE_MAC_ADDRESS                                \lb
     *      FM_SOLVER_CMD_LEARNED_MAC_ADDRESS                               \lb
     *      FM_SOLVER_CMD_AGED_MAC_ADDRESS. */
    fm_topologySolverMacAddrEvent macAddrEvent;

} fm_tsEventInfo;


/**************************************************/
/** \ingroup typeStruct
 * Describes a topology solver command or event.
 **************************************************/
typedef struct _fm_topologySolverEvent
{
    /** The command. */
    fm_topologySolverCmd cmd;

    /** Information related to the event/command.
     * Not used for the following topology solver commands:                 \lb
     *      FM_SOLVER_CMD_INITIALIZE                                        \lb
     *      FM_SOLVER_CMD_SHUTDOWN
     */
    fm_tsEventInfo info;

} fm_topologySolverEvent;


/**************************************************/
/** \ingroup typeScalar
 * A switch aggregate topology solver function is provided by
 * the application to manage a user-defined switch aggregate
 * topology. If set to NULL, then the API uses its
 * own solver for known topologies (e.g., stack or
 * fat tree - see 'fm_swagTopology') or does nothing
 * at all for undefined topologies. The solver is
 * called for the following operations:
 *                                                                      \lb\lb
 *      - Adding/Deleting ports in for a VLAN,                              \lb
 *      - Joining/Leaving multicast groups,                                 \lb
 *      - Changing the switch aggregate topology,                           \lb
 *      - Port state changes (UP/DOWN).
 *                                                                      \lb\lb
 * Any operation that requires analyzing the topology is 
 * forwarded to the solver which has the responsibility 
 * of configuring the switch devices in the switch aggregate 
 * and the ports in switch aggregate links as needed for the 
 * required service to work properly.
 *                                                                      \lb\lb
 * The solver function returns fm_status and takes as
 * arguments:
 *                                                                      \lb\lb
 *  fm_int sw - identifies the switch aggregate id number,
 *                                                                      \lb\lb
 *  ''fm_topologySolverEvent'' event - points to a topology solver
 *  event instance.
 *
 **************************************************/
typedef fm_status (*fm_swagTopologySolver)(fm_int                  sw,
                                           fm_topologySolverEvent *event);


#define FM_DEFINE_SWAG_EXTERNAL_LINK(logPort, swId, swPort)         \
    {                                                               \
        FM_SWAG_LINK_EXTERNAL,                                      \
        logPort,                                                    \
        swId,                                                       \
        swPort,                                                     \
        -1,                                                         \
        -1,                                                         \
        -1                                                          \
    }


#define FM_DEFINE_SWAG_INTERNAL_LINK(logPort, swId, swPort,         \
    partnerLogPort, partnerSwId, partnerSwPort)                     \
    {                                                               \
        FM_SWAG_LINK_INTERNAL,                                      \
        logPort,                                                    \
        swId,                                                       \
        swPort,                                                     \
        partnerLogPort,                                             \
        partnerSwId,                                                \
        partnerSwPort                                               \
    }


#define FM_DEFINE_SWAG_CPU_LINK(logPort, swId, swPort)              \
    {                                                               \
        FM_SWAG_LINK_CPU,                                           \
        logPort,                                                    \
        swId,                                                       \
        swPort,                                                     \
        -1,                                                         \
        -1,                                                         \
        -1                                                          \
    }


#define FM_DEFINE_SWAG_UNDEFINED_LINK()                             \
    {                                                               \
        FM_SWAG_LINK_UNDEFINED,                                     \
        -1,                                                         \
        -1,                                                         \
        -1,                                                         \
        -1,                                                         \
        -1,                                                         \
        -1                                                          \
    }

#define FM_DEFINE_STACK_LINK(swId, swPort, partnerSwId, partnerSwPort)  \
    {                                                                   \
        FM_SWAG_LINK_INTERNAL,                                          \
        -1,                                                             \
        swId,                                                           \
        swPort,                                                         \
        -1,                                                             \
        partnerSwId,                                                    \
        partnerSwPort                                                   \
    }


#ifdef FM_SUPPORT_SWAG

fm_status fmCreateSWAG(fm_swagTopology       topology,
                       fm_swagTopologySolver solver,
                       fm_int *              swagId);
fm_status fmDeleteSWAG(fm_int swagId);
fm_status fmAddSWAGSwitch(fm_int        swagId,
                          fm_int        sw,
                          fm_switchRole switchRole);
fm_status fmDeleteSWAGSwitch(fm_int swagId);
fm_status fmSetSWAGSwitchMaster(fm_int  swagId,
                                fm_int  sw,
                                fm_bool isMaster);
fm_status fmGetSWAGSwitchFirst(fm_int      swagId,
                               fm_voidptr *searchToken,
                               fm_int *    firstSwitch);
fm_status fmGetSWAGSwitchNext(fm_int      swagId,
                              fm_voidptr *searchToken,
                              fm_int *    nextSwitch);
fm_status fmAddSWAGLink(fm_int swagId, fm_swagLink *link);
fm_status fmDeleteSWAGLink(fm_int swagId, fm_int logicalPort);
fm_status fmSetSWAGActiveCpuLink(fm_int swagId,
                                 fm_int logicalPort);
fm_status fmSetSWAGLinkAttribute(fm_int swagId,
                                 fm_int logicalPort,
                                 fm_int attr,
                                 void * value);
fm_status fmGetSWAGLinkAttribute(fm_int swagId,
                                 fm_int logicalPort,
                                 fm_int attr,
                                 void * value);
fm_status fmGetSWAGLinkFirst(fm_int swagId, fm_swagLink *link);
fm_status fmGetSWAGLinkNext(fm_int          swagId,
                            fm_swagLink *startLink,
                            fm_swagLink *nextLink);
fm_status fmGetSWAGDefaultTopologySolver(fm_swagTopology        topology,
                                         fm_swagTopologySolver *solver);
fm_status fmSetSWAGTopology(fm_int swagId, fm_swagTopology topology);
fm_status fmGetSWAGTopology(fm_int swagId, fm_swagTopology *topology);
fm_status fmSetSWAGTopologySolver(fm_int            swagId,
                              fm_swagTopologySolver solver);
fm_status fmGetSWAGTopologySolver(fm_int                 sw,
                                  fm_swagTopologySolver *solver);
fm_status fmDbgGetSwitchAndPortForSWAGPort(fm_int  swagId,
                                           fm_int  swagPort,
                                           fm_int *sw,
                                           fm_int *port);


/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmGetSWAGSwitchFirst''. */
#define fmGetFirstSWAGSwitch(swagId, searchToken, firstSwitch) \
        fmGetSWAGSwitchFirst( (swagId), (searchToken), (firstSwitch) )

/** A legacy synonym for ''fmGetSWAGSwitchNext''. */
#define fmGetNextSWAGSwitch(swagId, searchToken, nextSwitch) \
        fmGetSWAGSwitchNext( (swagId), (searchToken), (nextSwitch) )

/** @} (end of Doxygen group) */


/* Functions usable by Topology Solvers */
fm_bool fmIsSWAGLinkEnabled(fm_int sw, fm_int port);
fm_status fmClearAllSWAGTrunkCrossReferences(fm_int sw);
fm_status fmGetFirstTrunk(fm_int sw, fm_voidptr *trunkId);
fm_status fmGetNextTrunk(fm_int sw, fm_voidptr *trunkId);
fm_status fmGetTrunkFromId(fm_int     sw,
                           fm_voidptr trunkId,
                           fm_swagTrunk * trunk);
fm_status fmConnectTrunk(fm_int     sw,
                         fm_int     destSwitch,
                         fm_voidptr trunkId);
fm_switchRole fmGetSWAGSwitchRole(fm_int swagId, fm_int sw);
fm_status fmGetFirstLinkFromTrunk(fm_int       sw,
                                  fm_voidptr   trunkId,
                                  fm_voidptr * linkId,
                                  fm_swagLink *link);
fm_status fmGetNextLinkFromTrunk(fm_int       sw,
                                 fm_voidptr * linkId,
                                 fm_swagLink *link);
fm_status fmGetSwitchAndPortForSWAGPort(fm_int swagId,
                                        fm_int swagPort,
                                        fm_int *sw,
                                        fm_int *port);
fm_status fmFindTrunkForSwitchPair(fm_int      swagId,
                                   fm_int      sourceSwitch,
                                   fm_int      destSwitch,
                                   fm_voidptr *trunkId);
fm_status fmCreateTrunk(fm_int          sw,
                        fm_int          sourcePort,
                        fm_swagLinkType trunkType,
                        fm_voidptr *    trunkId);
fm_int fmGetTrunkLinkCount(fm_int sw, fm_voidptr trunkId);
fm_bool fmIsLagTrunk(fm_int sw, fm_voidptr trunkId);
fm_status fmAttachTrunkToPort(fm_int sw, fm_voidptr trunk, fm_int port);
fm_status fmDetachTrunkFromPort(fm_int sw, fm_int port);
fm_status fmAddLinkToTrunk(fm_int sw, fm_voidptr trunkId, fm_swagLink *link);
fm_status fmRemoveLinkFromTrunk(fm_int       sw,
                                fm_voidptr   trunkId,
                                fm_swagLink *link);
fm_status fmGetTrunkForPort(fm_int sw, fm_int port, fm_voidptr *trunkId);
fm_status fmGetLinkForPort(fm_int sw, fm_int port, fm_voidptr *linkId);
fm_status fmUpdateRemotePorts(fm_int swagId);
fm_status fmActivateTrunkLink(fm_int     sw,
                              fm_voidptr trunkId,
                              fm_voidptr linkId);
fm_status fmDeactivateTrunkLink(fm_int     sw,
                                fm_voidptr trunkId,
                                fm_voidptr linkId);
fm_status fmDeactivateTrunkPort(fm_int sw, fm_int port);
fm_status fmSetTrunkGroup(fm_int sw, fm_voidptr trunkId, fm_int trunkGroup);

#endif /* FM_SUPPORT_SWAG */

#endif /* __FM_FM_API_SWAG_H */

