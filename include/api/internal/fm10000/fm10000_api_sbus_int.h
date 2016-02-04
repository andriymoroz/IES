/* vim:et:sw=4:ts=4:tw=79:
 */
/*****************************************************************************
 * File:            include/api/internal/fm10000/fm10000_api_sbus_int.h
 * Creation Date:   November 6, 2013
 * Description:     SBus low-level API
 *
 * Copyright (c) 2013, Intel Corporation
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

#ifndef __FM_FM10000_API_SBUS_INT_H
#define __FM_FM10000_API_SBUS_INT_H


/* sbus divider for the EPL ring SBus: set to 78,125 Mhz (= SerDes RefClk/2) */
#define FM10000_SBUS_EPL_RING_DIVIDER   0x01


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

void fm10000SbusSetDebug(fm_int debug);
fm_status fm10000SbusInit(fm_int sw, fm_bool eplRing);
fm_status fm10000SbusRead(fm_int sw,
                          fm_bool eplRing,
                          fm_uint sbusAddr,
                          fm_uint sbusReg,
                          fm_uint32 *data);
fm_status fm10000SbusWrite(fm_int sw,
                           fm_bool eplRing,
                           fm_uint sbusAddr,
                           fm_uint sbusReg,
                           fm_uint32 data);
fm_status fm10000SbusReceiverReset(fm_int    sw,
                                   fm_int    serDes);
fm_status fm10000SbusReset(fm_int    sw,
                           fm_bool   eplRing);
fm_status fm10000SbusSbmReset(fm_int    sw,
                              fm_bool   eplRing);
#endif  /* __FM_FM10000_API_SBUS_INT_H */

