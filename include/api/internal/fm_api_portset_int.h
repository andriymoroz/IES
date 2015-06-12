/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_portset_int.h
 * Creation Date:   2013
 * Description:     Internal structures and functions for dealing with portsets
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

#ifndef __FM_FM_API_PORTSET_INT_H
#define __FM_FM_API_PORTSET_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* The definition for unused port set. */
#define FM_PORT_SET_UNUSED                0

/* The definition for reserved port set. */
#define FM_PORT_SET_RESERVED              1

/* The number of internal portsets that can exist simultaneously. */
#define FM_NB_INTERNAL_PORTSET      256

/* This portset handle mask is used when manipulating
 * tree entries. More specifically, this mask must be used when
 * reading/writing tree entries. A portset handle is a signed 32-bit value,
 * with this mask, we can ensure that the handle is stored in the lowest 32
 * bits of the tree key. This is especially useful to prevent negative values
 * from being stored in 64-bit representation. */
#define FM_PORTSET_MASK             0xffffffff


/* The API allocates a structure of this type for each port set defined, and 
 * stores the pointer to the object in the 'portSet' tree of the fm_portSetInfo 
 * structure for the switch to which the port set belongs. */ 
typedef struct
{
    /* Keeps track of associated ports in the port set, where each bit position
     * is a cardinal port number. */
    fm_bitArray associatedPorts;

    /* Used by ACLs only. The meaning of this field is chip specific:
     *
     * FM6000: Refers to the bit position of a src port mapper:
     * - Value 0..7 refers to bit 0..7 of MAP_SRC_PORT_ID1
     * - Value 8..15 refers to bit 0..7 of MAP_SRC_PORT_ID2
     * - Value 16..23 refers to bit 0..7 of MAP_SRC_PORT_ID3 */ 
    fm_byte     mappedValue;

} fm_portSet;


/* Structure that tracks the portSets and their usage */
typedef struct _fm_portSetInfo
{
    /* Tree containing all allocated portsets. The entry's key is the portset
     * ID (anded with FM_PORTSET_MASK). The tree entry values are pointers
     * to fm_portSet structures. Internal API's that operate on the tree and/or 
     * its entries should take the state lock (FM_TAKE_STATE_LOCK). */ 
    fm_tree portSetTree;

    /* Bit array to track public portset usage. The size of this bit array
     * is determined by fm_switch.maxPortSets */
    fm_bitArray portSetUsage;

     /* Bit array to track internal portset usage. The size of this bit array
     * is determined by FM_NB_INTERNAL_PORTSET. */
    fm_bitArray portSetUsageInt;

} fm_portSetInfo;


/***************************************************
 * Function prototypes.
 **************************************************/

fm_status fmInitPortSetTable(fm_switch *switchPtr);
fm_status fmDestroyPortSetTable(fm_portSetInfo *psi);
fm_status fmCreatePortSetInt(fm_int sw, fm_int *portSet, fm_bool isInternal);
fm_status fmDeletePortSetInt(fm_int sw, fm_int portSet);
fm_status fmAddPortSetPortInt(fm_int sw, fm_int portSet, fm_int port);
fm_status fmDeletePortSetPortInt(fm_int sw, fm_int portSet, fm_int port);
fm_status fmClearPortSetInt(fm_int sw, fm_int portSet);

fm_status fmGetPortSetPortFirstInt(fm_int   sw,
                                   fm_int   portSet,
                                   fm_int * port);

fm_status fmGetPortSetPortNextInt(fm_int   sw,
                                  fm_int   portSet,
                                  fm_int   currentPort,
                                  fm_int * nextPort);

fm_status fmPortSetToPortList(fm_int    sw, 
                              fm_int    portSet,
                              fm_int *  numPorts, 
                              fm_int *  portList, 
                              fm_int    maxPorts);

fm_status fmPortSetToPortMask(fm_int sw, fm_int portSet, fm_portmask *maskPtr);

fm_status fmPortSetToPhysMask(fm_int sw, fm_int portSet, fm_portmask *maskPtr);

fm_bool   fmIsPortSetEmpty(fm_int sw, fm_int portSet);

fm_int    fmGetPortSetCountInt(fm_int sw, fm_int portSet);

fm_bool   fmIsPortInPortSetInt(fm_int sw, fm_int portSet, fm_int port);

fm_status fmSetPortSetPortInt(fm_int  sw,
                              fm_int  portSet,
                              fm_int  port,
                              fm_bool state);


#endif  /* __FM_FM_API_PORTSET_INT_H */
