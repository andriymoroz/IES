/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_lport_int.h
 * Creation Date:   April 17, 2013 from fm4000_api_lport_int.h
 * Description:     Contains functions dealing with the raw management
 *                  of the glort table for the FM10000 switch chip.
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

#ifndef __FM_FM10000_API_LPORT_INT_H
#define __FM_FM10000_API_LPORT_INT_H

/**************************************************
 * Aliases
 **************************************************/

#define fm10000AllocLogicalPort             fmCommonAllocLogicalPort

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Number of Glorts and destination entries for physical ports. */
#define FM10000_GLORT_PORT_SIZE             0x40

/* Range A length to use when creating CAM entries for physical ports. */
#define FM10000_PHYSICAL_PORT_ALENGTH       6

/* Glort used by the switch to receive FIBM messages.
 * This is one of the reserved entries in the physical port glort range. */
#define FM10000_GLORT_FIBM(base)            ((base) + 48)

/* Number of Glorts and destination entries for CPU port */
#define FM10000_GLORT_CPU_SIZE              0x1

/* Maximum number of GLORTs per LAG. */
#define FM10000_GLORTS_PER_LAG              (1 << FM10000_LAG_MASK_SIZE)

#if 0
/* Special index reserved for remote CPU to receive FIBM message */
#define FM10000_FIBM_CPU_GLORT_INDEX        0x0FF0
#define FM10000_FIBM_MASTER_GLORT_INDEX     0x0FF1

/* Reserve one with the highest precedence or at least higher than CPU. */
#define FM10000_FIBM_CPU_CAM_INDEX          255
#define FM10000_FIBM_MASTER_CAM_INDEX       254
#endif

/**************************************************
 * Glorts.
 **************************************************/

/* CPU glorts */
#define FM10000_GLORT_CPU_BASE              0x7F00      /* 0x7E00 */
#define FM10000_GLORT_CPU_MASK              0xFF00      /* 0xFE00 */

#define FM10000_GLORT_SPECIAL_BASE          0x7FF8
#define FM10000_GLORT_SPECIAL_MASK          0xFFF8
#define FM10000_GLORT_SPECIAL_SIZE          8
#define FM10000_GLORT_SPECIAL_A_LENGTH      3

/* Glort values for SPECIAL ports. */
#define FM10000_GLORT_FLOOD                 0x7FFF
#define FM10000_GLORT_RPF_FAILURE           0x7FFE
#define FM10000_GLORT_DROP                  0x7FFD
#define FM10000_GLORT_BCAST                 0x7FFB
#define FM10000_GLORT_MCAST                 0x7FFA
#define FM10000_GLORT_NOP_FLOW              0x7FF9

/* Lag glorts */
#define FM10000_GLORT_LAG_BASE              0x2000
#define FM10000_GLORT_LAG_SIZE              0x1000   /* Last Glort = 0x2FFF */

/* Multicast glorts */
#define FM10000_GLORT_MCAST_BASE            0x3000
#define FM10000_GLORT_MCAST_SIZE            0x1000   /* Last Glort = 0x3FFF */

/* Lbg glorts */
#define FM10000_GLORT_LBG_BASE              0x7600
#define FM10000_GLORT_LBG_SIZE              0x7ff    /* Last Glort = 0x7DFF */


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

fm_status fm10000CreateLogicalPort(fm_int sw, fm_int port);

fm_status fm10000DbgDumpGlortDestTable(fm_int sw, fm_bool raw);

fm_status fm10000DbgDumpGlortTable(fm_int sw);

fm_status fm10000FreeDestEntry(fm_int             sw,
                               fm_glortDestEntry *destEntry);

fm_status fm10000GetGlortForSpecialPort(fm_int  sw,
                                        fm_int  port,
                                        fm_int *glort);

fm_status fm10000GetLogicalPortAttribute(fm_int sw,
                                         fm_int port,
                                         fm_int attr,
                                         void * value);

fm_status fm10000GetMaxGlortsPerLag(fm_int sw, fm_int *maxGlorts);

fm_status fm10000ResetPortSettings(fm_int sw, fm_int port);

fm_status fm10000SetGlortDestMask(fm_int             sw,
                                  fm_glortDestEntry *destEntry,
                                  fm_portmask *      destMask);

fm_status fm10000SetLogicalPortAttribute(fm_int sw,
                                         fm_int port,
                                         fm_int attr,
                                         void * value);

fm_status fm10000WriteDestEntry(fm_int             sw,
                                fm_glortDestEntry *destEntry);

fm_status fm10000WriteGlortCamEntry(fm_int            sw,
                                    fm_glortCamEntry *camEntry,
                                    fm_camUpdateMode  mode);

fm_status fm10000InitGlortCam(fm_int sw);

fm_status fm10000FreeLaneResources( fm_int sw );

fm_status fm10000FreeLogicalPort( fm_int sw, fm_int logicalPort );

fm_status fm10000CreateVsiGlortCams(fm_int sw);

fm_status fm10000MapVsiGlortToLogicalPort(fm_int     sw,
                                          fm_uint32  vsiGlort,
                                          fm_int    *port);

fm_status fm10000MapVsiGlortToPhysicalPort(fm_int     sw,
                                           fm_uint32  vsiGlort,
                                           fm_int    *port);

fm_status fm10000MapGlortToPepNumber(fm_int     sw,
                                     fm_uint32  vsiGlort,
                                     fm_int    *pepNb);

fm_status fm10000GetPcieLogicalPort(fm_int sw,
                                    fm_int pep,
                                    fm_pciePortType type,
                                    fm_int index,
                                    fm_int *logicalPort);

fm_status fm10000GetLogicalPortPcie(fm_int sw,
                                    fm_int logicalPort,
                                    fm_int *pep,
                                    fm_pciePortType *type,
                                    fm_int *index);

#endif  /* __FM_FM10000_API_LPORT_INT_H */

