/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_sched_int.h
 * Creation Date:   March 20th, 2014
 * Description:     Contains constants and functions used to support scheduler
 *                  configuration.
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

#ifndef __FM_FM10000_API_SCHED_INT_H
#define __FM_FM10000_API_SCHED_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_SCHED_RING_RX               (1 << 0)
#define FM10000_SCHED_RING_TX               (1 << 1)
#define FM10000_SCHED_RING_ALL              (FM10000_SCHED_RING_RX |          \
                                             FM10000_SCHED_RING_TX)

#define FM10000_MAX_SCHEDULE_LENGTH         512

/* Redefinitions of because this file is included for low-lev APIs
 *    - FM10000_NUM_PORTS
 *    - FM10000_NUM_FABRIC_PORTS
 *    - FM10000_NUM_QPC */ 
#define FM10000_SCHED_NUM_PORTS             48
#define FM10000_SCHED_NUM_FABRIC_PORTS      64
#define FM10000_SCHED_NUM_QPC               (FM10000_SCHED_NUM_FABRIC_PORTS/4)

#define FM10000_SCHED_NUM_EPL_PORTS         36
#define FM10000_SCHED_NUM_PCIE_PORTS        9
#define FM10000_SCHED_NUM_TE_PORTS          2
#define FM10000_SCHED_NUM_LOOPBACK_PORTS    2
#define FM10000_SCHED_NUM_FIBM_PORTS        1
#define FM10000_SCHED_MAX_NUM_PORTS       (FM10000_SCHED_NUM_EPL_PORTS      +  \
                                           FM10000_SCHED_NUM_PCIE_PORTS     +  \
                                           FM10000_SCHED_NUM_TE_PORTS       +  \
                                           FM10000_SCHED_NUM_LOOPBACK_PORTS +  \
                                           FM10000_SCHED_NUM_FIBM_PORTS)

/* Scheduler Modes, see fm10000_property.h for descriptions */
#define FM10000_SCHED_MODE_STATIC           0
#define FM10000_SCHED_MODE_DYNAMIC          1

/* Enumeration of port speeds for the purpose of bin distribution */
typedef enum 
{
    /* Speeds below used during the bin distribution */
    FM10000_SCHED_SPEED_ANY                         = -1,
    FM10000_SCHED_SPEED_10G_RSVD                    = -2,
    FM10000_SCHED_SPEED_NOT_10G_NOT_60G_NOT_100G    = -3,
    FM10000_SCHED_SPEED_NOT_10G_NOT_100G            = -4,
    FM10000_SCHED_SPEED_IDLE_25G_2500M              = -5,
    FM10000_SCHED_SPEED_IDLE_2500M                  = -6,

    /* Special speed used for using default per port */
    FM10000_SCHED_SPEED_DEFAULT             = -10,


    /* Speeds below are actual speeds */
    FM10000_SCHED_SPEED_IDLE                = 0,
    FM10000_SCHED_SPEED_2500M               = 2500,
    FM10000_SCHED_SPEED_10G                 = 10000,
    FM10000_SCHED_SPEED_25G                 = 25000,
    FM10000_SCHED_SPEED_40G                 = 40000,
    FM10000_SCHED_SPEED_60G                 = 60000,
    FM10000_SCHED_SPEED_100G                = 100000,

} fm10000_schedSpeed;




/* Scheduler speed information */
typedef struct fm10000_schedSpeedInfo
{
    /* The current assigned speed. */
    fm_int assignedSpeed;

    /* The singe lane available speed if the port is to be used as
     * single lane.  */ 
    fm_int singleLaneSpeed;

    /* The multi lane available speed if the port is to be used as
     * multi lane. 
     * Note that this may be 0 if other lanes of the QPC/EPL are in use. */
    fm_int multiLaneSpeed;

    /* The speed currently requested/reserved for this port */
    fm_int reservedSpeed;

    /* The speed preReserved for this port. Only ports configured with AN, 
     * have different values for reservedSpeed and preReservedSpeed. */
    fm_int preReservedSpeed;

    /* Will be set to true if the port is currently configured / reserved as
     * a quad port */
    fm_bool isQuad;

} fm10000_schedSpeedInfo;




/* Scheduler Attributes */
typedef struct fm10000_schedAttr
{
    /* The scheduler's operation mode */
    fm_int                mode;

    /* Keep track of whether the scheduler should be updated (or not) on link
     * change (up/down) */
    fm_bool               updateLnkChange;

} fm10000_schedAttr;




/* Structure that tracks the scheduler statistics */
typedef struct fm10000_schedStat
{
    /* The speed of the stat instance */
    fm10000_schedSpeed speed;

    /* The index of the first element in this stat group */
    fm_int             firstIdx;

    /* The index of the last element in this stat group */
    fm_int             lastIdx;

    /* The minimum spacing between elements */
    fm_int             minDiff;

    /* The maximum spacing between elements */
    fm_int             maxDiff;

    /* The index location of where the minimum spacing was found */
    fm_int             minLoc;

    /* The index location of where the maximum spacing was found */
    fm_int             maxLoc;

    /* The number of elements in this stat group */
    fm_int             cnt;

} fm10000_schedStat;




/* Structure that holds the difficulty level of adding a port to scheduler */
typedef struct _fm10000_schedPortDifficulty
{
    /* The physical port */
    fm_int port;

    /* The difficulty (low = easier, high = harder) */
    fm_int diff;

} fm10000_schedPortDifficulty;




typedef struct _fm10000_schedEntryInfo
{
    /* The lane associated with the entry (0..3). */
    fm_int  lane;

    /* Assigned fabric port, set to -1 if the token is not assigned to any
     * fabric port. */
    fm_int  afp;

    /* Assigned physical port, set to -1 if the token is not assigned to any
     * physical port or if it should be derived from 'afp'. */
    fm_int  app;

    /* Defines whether the entry is quad (1) or not (0) */
    fm_bool quad;

} fm10000_schedEntryInfo;




/* Structure that tracks the scheduler software/HW state */
typedef struct _fm10000_schedInfoInt
{
    /* The portlist used in the scheduler generation */
    fm_schedulerPort      portList[FM10000_SCHED_MAX_NUM_PORTS];

    /* The number of valid entries in portList */
    fm_int                nbPorts;

    /* Bit arrays used for the generation of the token list, indexed by
     * physical port (0..47). Only valid during generation. */
    fm_bitArray           p2500M;
    fm_bitArray           p10G;
    fm_bitArray           p25G;
    fm_bitArray           p40G;
    fm_bitArray           p60G;
    fm_bitArray           p100G;

    /* Table that holds the speed per physical port */
    fm10000_schedSpeed    physPortSpeed[FM10000_SCHED_NUM_PORTS];

    /* Table that holds the speed per fabric port */
    fm10000_schedSpeed    fabricPortSpeed[FM10000_SCHED_NUM_FABRIC_PORTS];

    /* Indicates if the physical port is in quad mode or not */
    fm_bool               isQuad[FM10000_SCHED_NUM_PORTS];

    /* Sorted table of difficulty to insert a port. Entry 0 is the
     * hardest port to fit. */
    fm10000_schedPortDifficulty diffTable[FM10000_SCHED_NUM_PORTS];

    /* Schedule length */
    fm_int                schedLen;

    /* Scheduler Entry List, note that index 0 should be the implicit idle */
    fm_schedulerToken     schedList[FM10000_MAX_SCHEDULE_LENGTH];

    /* Speed of the port for each scheduler entry, note that index 0
     * should be the implicit idle */
    fm10000_schedSpeed    speedList[FM10000_MAX_SCHEDULE_LENGTH];

    /* Spare 25G slots used to handle 25G ports being present in only one QPC */
    fm_int                spare25GSlots;

} fm10000_schedInfoInt;




/* Structure that tracks the scheduler software/HW state */
typedef struct _fm10000_schedInfo
{
    fm10000_schedAttr     attr;

    /* Temporary Scheduler Internal Information, used during generation */
    fm10000_schedInfoInt  tmp;

    /* Scheduler Internal Information, active configuration */
    fm10000_schedInfoInt  active;

    /* Tracks which scheduler page list is active, should be 0 or 1 */
    fm_bool               activeListRx;
    fm_bool               activeListTx;

    /* Tree indexed by speed, each tree element points to a struct of
     * type fm10000_schedStat */
    fm_tree               speedStatsTree;

    /* Tree indexed by quad port channel, each tree element points to a struct of
     * type fm10000_schedStat */
    fm_tree               qpcStatsTree;

    /* Tree indexed by physical port, each tree element points to a struct of
     * type fm10000_schedStat */
    fm_tree               portStatsTree;

    /* Array of Quad Port Channel trees to track scheduler entry usage and
     * location. Each tree contain the position of the entry in the schedList
     * as the key, and the value is a structure (fm10000_schedEntryInfo) that 
     * tracks the membership of the entry to a given lane on the QPC. */ 
    fm_tree               qpcState[FM10000_SCHED_NUM_QPC];

    /* Table that maps physical ports to fabric ports, a value of -1 indicates
     * none. */
    fm_int                physicalToFabricMap[FM10000_SCHED_NUM_PORTS];

    /* Table that maps fabric ports to physical ports, a value of -1 indicates
     * none. Intermediate fabric ports on non-EPL quads will always have
     * a value of -1. */
    fm_int                fabricToPhysicalMap[FM10000_SCHED_NUM_FABRIC_PORTS];

    /* Table that holds the speed reserved per physical port and it's quad bit
     * setting */
    fm10000_schedSpeed    reservedSpeed[FM10000_SCHED_NUM_PORTS];
    fm_bool               reservedQuad[FM10000_SCHED_NUM_PORTS];

    /* Table that holds the speed pre reserved per physical port and it's quad 
     * bit setting. It is mainly used in the case of Dynamic Scheduler and AN73.
     * In Dynamic Scheduler mode, Bandwidth availability is checked based on 
     * this table. */
    fm10000_schedSpeed    preReservedSpeed[FM10000_SCHED_NUM_PORTS];
    fm_bool               preReservedQuad[FM10000_SCHED_NUM_PORTS];

} fm10000_schedInfo;



/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/


/***************************************
 * Port Mapping Functions
 ***************************************/

fm_status fm10000MapPhysicalPortToFabricPort(fm_int  sw, 
                                             fm_int  physPort, 
                                             fm_int *fabricPort);

fm_status fm10000MapPhysicalPortToEplLane(fm_int  sw, 
                                          fm_int  physPort, 
                                          fm_int *epl,
                                          fm_int *lane);

fm_status fm10000MapFabricPortToPhysicalPort(fm_int  sw, 
                                             fm_int  fabricPort, 
                                             fm_int *physPort);

fm_status fm10000MapEplLaneToPhysicalPort(fm_int  sw, 
                                          fm_int  epl,
                                          fm_int  lane,
                                          fm_int *physPort);

fm_status fm10000MapLogicalPortToFabricPort(fm_int  sw, 
                                            fm_int  logPort, 
                                            fm_int *fabricPort);

fm_status fm10000MapLogicalPortToEplLane(fm_int  sw, 
                                         fm_int  logPort, 
                                         fm_int *epl,
                                         fm_int *lane);

fm_status fm10000MapFabricPortToLogicalPort(fm_int  sw, 
                                            fm_int  fabricPort, 
                                            fm_int *logPort);

fm_status fm10000MapEplLaneToLogicalPort(fm_int  sw, 
                                         fm_int  epl,
                                         fm_int  lane,
                                         fm_int *logPort);

/***************************************
 * Scheduler Functions
 ***************************************/

fm_status fm10000InitScheduler(fm_int sw);
fm_status fm10000FreeSchedulerResources(fm_int sw);

fm_status fm10000UpdateSchedPort(fm_int               sw, 
                                 fm_int               physPort,
                                 fm_int               speed,
                                 fm_schedulerPortMode mode);

fm_status fm10000GetSchedRing(fm_int             sw, 
                              fm_uint32          ring,
                              fm_int             stMax,
                              fm_schedulerToken *stList,
                              fm_int            *stCount);

fm_status fm10000SetSchedRing(fm_int             sw, 
                              fm_uint32          mask,
                              fm_schedulerToken *stList,
                              fm_int             stCount);

fm_status fm10000DbgDumpSchedulerConfig(fm_int sw);

fm_status fm10000GenerateSchedule(fm_int sw, 
                                  fm_schedulerPort *portList, 
                                  fm_int nbPorts);

fm_status fm10000GetSchedPortSpeed(fm_int  sw,
                                   fm_int  physPort,
                                   fm10000_schedSpeedInfo *speed);

fm_status fm10000GetSchedPortSpeedForPep(fm_int  sw,
                                         fm_int  pepId,
                                         fm_int *speed);

fm_status fm10000SwapSchedQpcBw(fm_int sw,
                                fm_int oldQpc,
                                fm_int newQpc);

fm_status fm10000ReserveSchedBw(fm_int               sw,
                                fm_int               physPort,
                                fm_int               speed,
                                fm_schedulerPortMode mode);

fm_status fm10000RegenerateSchedule(fm_int sw);

fm_status fm10000GetSchedAttributes(fm_int sw, fm10000_schedAttr *attr);

fm_status fm10000PreReserveSchedBw(fm_int               sw,
                                   fm_int               physPort,
                                   fm_int               speed,
                                   fm_schedulerPortMode mode);

fm_status fm10000ReserveSchedBwForAnPort(fm_int               sw,
                                         fm_int               physPort,
                                         fm_int               speed,
                                         fm_schedulerPortMode mode);

#endif /* __FM_FM10000_API_SCHED_INT_H */

