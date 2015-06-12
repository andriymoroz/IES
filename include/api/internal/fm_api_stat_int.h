/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stat_int.h
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with counters
 *                  (statistics)
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

#ifndef __FM_FM_API_STAT_INT_H
#define __FM_FM_API_STAT_INT_H

/* Generic enable bits for initialization of counter modules.
 * These constants should be or'ed to produce a 3 bit value
 * that indicates which counter modules (port/vlan/vlanAssigment) 
 * should be initialized by generic code. */

/* When this bit is set, the generic API will allocate and initialize
 * memory for each cardinal port */
#define FM_STAT_PORT_INIT_GENERIC               1 << 0   

/* When this bit is set, the generic API will allocate and initialize 
 * memory for each vlan counter */
#define FM_STAT_VLAN_INIT_GENERIC               1 << 1

/* When this bit is set, the generic API will allocate and initialize 
 * memory for vlan assigment */
#define FM_STAT_VLAN_ASSIGNMENT_INIT_GENERIC    1 << 2



#define FM_UNALLOCATED_VLAN_COUNTER  -1

/** Reads the specified VLAN counter register and assigns the value to var
 * in the 'counters' structure.
 *
 * Depends on the existence of the variables:
 * sw - switch number.
 * vcid - counter set ID
 * ci - fm_counterInfo pointer.
 */
#define FM_ASSIGN_VLAN_COUNTER_REG(reg, var)                                     \
    if ( ( err = switchPtr->ReadUINT64(sw,                                       \
                                       reg(vcid, 0),                             \
                                       &ci->lastReadVlan[vcid].var) ) != FM_OK ) \
    {                                                                            \
     goto ABORT;                                                                 \
    }                                                                            \
    counters->var = ci->lastReadVlan[vcid].var - ci->subtractVlan[vcid].var;


/** Add the read to the scatter gather list
 * As 64 bit is more common than 32 bit this lacks a bit size suffix.
 *
 * Depends on the existence of the variables:
 * lastReadPort [in] - fm_portCounters 
 * ci - fm_counterInfo pointer.
 * sgList [out] - scatter gather array to hold the reads
 * sgListCnt [out] - number of entry in the sgList
 */
#define FM_GET_PORT_STAT(reg, var)  \
    sgList[sgListCnt].addr = reg; \
    sgList[sgListCnt].data = (fm_uint32*)&lastReadPort.var; \
    sgList[sgListCnt].count = 2; \
    sgListCnt++;

/** Add the read to the scatter gather list for 32-bit read
 *
 * Depends on the existence of the variables:
 * lastReadPort [in] - fm_portCounters 
 * ci - fm_counterInfo pointer.
 * sgList [out] - scatter gather array to hold the reads
 * sgListCnt [out] - number of entry in the sgList
 */
#define FM_GET_PORT_STAT32(reg, var)  \
    sgList[sgListCnt].addr = reg; \
    sgList[sgListCnt].data = (fm_uint32*)&lastReadPort.var; \
    sgList[sgListCnt].count = 1; \
    sgListCnt++;


/** Swap the result, if enabled, and assigns the value to var in
 * the 'counters' structure.
 * As 64 bit is more common than 32 bit this lacks a bit size suffix.
 *
 * Depends on the existence of the variables:
 * lastReadPort [in] - fm_portCounters 
 * ci - fm_counterInfo pointer.
 * cpi [in] - cardinal port index.
 * swap64 [in] - if swapping is needed
 */
#define FM_UPDATE_PORT_STAT(reg, var)                                       \
    if (swap64)                                                             \
    {                                                                       \
        fm_uint32 temp32;                                                   \
        temp32 = *(fm_uint32*)&lastReadPort.var;                            \
        ci->lastReadPort[cpi].var = *(((fm_uint32*)&lastReadPort.var)+1);   \
        ci->lastReadPort[cpi].var <<= 32;                                   \
        ci->lastReadPort[cpi].var |= (fm_uint64)temp32;                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        ci->lastReadPort[cpi].var = lastReadPort.var;                       \
    }                                                                       \
    FM_SET_PORT_STAT(var)

/** Assigns the value to var in the 'counters' structure.
 *
 * Depends on the existence of the variables:
 * lastReadPort [in] - fm_portCounters 
 * ci [out] - fm_counterInfo pointer. 
 * cpi [in] - cardinal port index.
 */
#define FM_UPDATE_PORT_STAT32(reg, var)     \
    ci->lastReadPort[cpi].var = *(fm_uint32*)&lastReadPort.var;    \
    FM_SET_PORT_STAT(var)

/**
 * Assigns the specified value to a 64-bit stat counter.
 *
 * @param var   Stats field to be updated.
 * @param val   Value to be assigned to the stats field.
 *
 * The 'lastReadPort' structure contains the current values of the stats
 * registers. The 'subtractPort' structure contains the values of the stat 
 * registers the last time fmResetPortCounters was called. We must 
 * maintain this relationship without changing 'subtractPort'. 
 *  
 * To achieve this result:
 *      lastReadPort - subtractPort = C
 *  
 * We must establish this precondition:
 *      lastReadPort = C + subtractPort 
 *
 * Depends on the existence of the variables:
 *  counters [out] - fm_portCounters pointer.
 *  ci [out] - fm_counterInfo pointer.
 *  cpi [in] - cardinal port index.
 */
#define FM_FORCE_PORT_STAT(var, val)    \
    ci->lastReadPort[cpi].var = (val) + ci->subtractPort[cpi].var; \
    FM_SET_PORT_STAT(var)

/**
 * Sets the specified field in the counters structure to the value of the
 * underlying stat register.
 *
 * @param var   Stats field to be updated.
 *
 * The 'lastReadPort' structure contains the current values of the stats
 * registers.
 *
 * The 'subtractPort' structure contains the values of the stat registers
 * the last time fmResetPortCounters was called.
 *
 * The difference between corresponding fields in the two structures is
 * the number of counts since the last reset.
 *
 * This macro depends on the existence of the variables:
 *  counters [out] - fm_portCounters pointer.
 *  ci [in] - fm_counterInfo pointer.
 *  cpi [in] - cardinal port index.
 */
#define FM_SET_PORT_STAT(var) \
    counters->var = ci->lastReadPort[cpi].var - ci->subtractPort[cpi].var

/* holds shared state about counters */
typedef struct
{
    /* Number of port counters - 1. */
    fm_int            portCount;
    fm_int            vlanCount;

    /* The state of the counters when they were last read.
     * Referenced by logical port number (FM2000) or cardinal port index. */
    fm_portCounters * lastReadPort;

    /* The state of the counters when they were last reset.
     * Referenced by logical port number (FM2000) or cardinal port index. */
    fm_portCounters * subtractPort;

    fm_switchCounters lastReadSwitch;
    fm_switchCounters subtractSwitch;

    fm_vlanCounters * lastReadVlan;
    fm_vlanCounters * subtractVlan;

    fm_int *          vlanAssignedToCounter;

} fm_counterInfo;


fm_status fmAllocateCounterDataStructures(fm_switch *swState);
fm_status fmInitCounters(fm_int sw);
fm_status fmFreeCounterDataStructures(fm_switch *swState);


#endif /* __FM_FM_API_STAT_INT_H */
