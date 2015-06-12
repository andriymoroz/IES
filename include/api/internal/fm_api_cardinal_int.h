/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_cardinal_int.h
 * Creation Date:   October 22, 2011
 * Description:     Cardinal port definitions.
 *
 * Copyright (c) 2011 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_CARDINAL_INT_H
#define __FM_FM_API_CARDINAL_INT_H


/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/

#define DEFINE_CARDINAL_PORT_MACROS 1


/*****************************************************************************
 * Types
 *****************************************************************************/

/**
 * Cardinal port map entry.
 */
typedef struct
{
    /** Logical port number */
    fm_int      logPort;

    /** Physical port number */
    fm_int      physPort;

} fm_cardinalPort;


/**
 * Cardinal port information structure. 
 *  
 * The cardinal ports of a switch are the set of logical ports that map to 
 * the platform's physical ports. 
 *  
 * This structure describes the set of cardinal ports. It maps logical 
 * port numbers to cardinal port indexes, and cardinal port indexes to 
 * (logical port, physical port) tuples. 
 */
typedef struct
{
    /**
     * Cardinal port map.
     *
     * Index is the cardinal port index, in the range 0..numCardinalPorts-1.
     * Value is a (logical port, physical port) tuple. 
     * All entries in the range are guaranteed valid.
     */
    fm_cardinalPort *   portMap;

    /**
     * Cardinal port index table. 
     *  
     * Index is a logical port number in the range 0..maxLogicalPort. 
     * Value is the cardinal port index of the specified logical port, 
     * or -1 if the specified logical is not a cardinal port.
     */
    fm_int *            indexTable;

    /**
     * Maximum logical port number assigned to a cardinal port. 
     * Used to range-check references to indexTable. 
     */ 
    fm_int              maxLogicalPort;

} fm_cardinalPortInfo;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/* Internal functions */

fm_status fmAllocCardinalPortMap(fm_switch * switchPtr);

fm_status fmCreateCardinalPortIndexTable(fm_switch * switchPtr);

fm_status fmDbgDumpCardinalPorts(fm_int sw);

fm_status fmFreeCardinalPortDataStructures(fm_switch * switchPtr);

int fmCompareCardinalPorts(const void * aPtr, const void * bPtr);

/* BitArray functions */

fm_status fmBitArrayToPortList(fm_int        sw, 
                               fm_bitArray * bitArray,
                               fm_int *      numPorts, 
                               fm_int *      portList, 
                               fm_int        maxPorts);

fm_status fmFindPortInBitArray(fm_int        sw,
                               fm_bitArray * bitArray,
                               fm_int        lastPort,
                               fm_int      * nextPort,
                               fm_status     notFound);

fm_status fmGetPortInBitArray(fm_int        sw,
                              fm_bitArray * bitArray,
                              fm_int        port,
                              fm_bool *     state);

fm_status fmSetPortInBitArray(fm_int        sw,
                              fm_bitArray * bitArray,
                              fm_int        port,
                              fm_bool       state);

/* BitMask functions */

fm_status fmFindPortInBitMask(fm_int         sw,
                              fm_uint32      bitMask,
                              fm_int         lastPort,
                              fm_int       * nextPort,
                              fm_status      notFound);

fm_status fmGetPortInBitMask(fm_int      sw,
                             fm_uint32   bitMask,
                             fm_int      port,
                             fm_bool *   state);

fm_status fmSetPortInBitMask(fm_int      sw,
                             fm_uint32 * bitMask,
                             fm_int      port,
                             fm_bool     state);

/* PortMask functions */

fm_status fmFindPortInPortMask(fm_int        sw,
                               fm_portmask * portMask,
                               fm_int        lastPort,
                               fm_int      * nextPort,
                               fm_status     notFound);

fm_status fmEnablePortInPortMask(fm_int        sw,
                                 fm_portmask * maskPtr,
                                 fm_int        port);

fm_status fmDisablePortInPortMask(fm_int        sw,
                                  fm_portmask * maskPtr,
                                  fm_int        port);

fm_status fmSetPortInPortMask(fm_int        sw,
                              fm_portmask * maskPtr,
                              fm_int        port,
                              fm_bool       state);

fm_status fmAssignPortToPortMask(fm_int        sw,
                                 fm_portmask * maskPtr,
                                 fm_int        port);

/* Validation functions */

fm_bool fmIsCardinalPort(fm_int sw, fm_int port);

/* Mapping functions */

fm_status fmMapCardinalPortInternal(fm_switch * switchPtr,
                                    fm_int      cpi,
                                    fm_int *    logPort,
                                    fm_int *    physPort);

#if defined(FM_DEFINE_CARDINAL_PORT_MACROS)

/**
 * Returns the logical port number of a port, given its cardinal port 
 * index. 
 *  
 * A cardinal port is a logical port that has an associated physical port 
 * (as opposed, say, to a LAG port or a remote port).
 *  
 * \note        Assumes that the switch number and port index are valid. 
 *  
 * \param[in]   sw is the switch to operate on. 
 *  
 * \param[in]   cpi is the cardinal port index. 
 *  
 * \return      The logical port number for the port. 
 */
#define GET_LOGICAL_PORT(sw, cpi) \
    GET_SWITCH_PTR(sw)->cardinalPortInfo.portMap[cpi].logPort

/**
 * Returns the cardinal port index for a port, given the logical port number
 *  
 * \note        Assumes that the switch and port numbers are valid. 
 *              It is an error to call this function for a non-cardinal
 *              port.
 *  
 * \param[in]   sw is the switch to operate on. 
 *  
 * \param[in]   port is the logical port number.
 *  
 * \return      The cardinal port index for the port. 
 */
#define GET_PORT_INDEX(sw, port) \
    GET_SWITCH_PTR(sw)->cardinalPortInfo.indexTable[port]

#else

/* Function implementations of macros, for debugging. */
fm_int GET_LOGICAL_PORT(fm_int sw, fm_int cpi);
fm_int GET_PORT_INDEX(fm_int sw, fm_int port);

#endif


#endif /* __FM_FM_API_CARDINAL_INT_H */

