/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_lag_int.h
 * Creation Date:   May 1, 2013 (from fm4000_api_lag_int.h)
 * Description:     FM10000 Link Aggregation Group (LAG) support.
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FM10000_API_LAG_INT_H
#define __FM_FM10000_API_LAG_INT_H

/* Size of member per lag in power of 2 */
#define FM10000_LAG_MASK_SIZE                   5

#define FM10000_MAX_NUM_LAG_MEMBERS             16

typedef struct
{
    fm_bool     used;
    fm_uint16   glort;
    fm_uint16   maskSize;
    fm_uint16   fieldSize;
} fm10000CanonCamEntry;

typedef struct
{
    /* Canonical base glort of the LAG */
    fm_uint         lagGlort;

} fm10000_lag;

/* Internal functions used only within the API code */
fm_status fm10000LagGroupInit(fm_int sw);

fm_status fm10000CreateCanonicalCamEntries(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_uint   glortSize,
                                           fm_int    clearSize);

fm_status fm10000DeleteCanonicalCamEntries(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_uint   glortSize);
fm_status fm10000AllocateLAGs(fm_int     sw,
                              fm_uint    startGlort,
                              fm_uint    glortSize,
                              fm_int    *baseLagHandle,
                              fm_int    *numLags,
                              fm_int    *step);
fm_status fm10000FreeStackLAGs(fm_int sw, fm_int baseLagHandle);

fm_status fm10000CreateLagOnSwitch(fm_int sw, fm_int lagIndex);
fm_status fm10000DeleteLagFromSwitch(fm_int sw, fm_int lagIndex);
void fm10000FreeLAG(fm_int sw, fm_int lagIndex);

fm_status fm10000AddPortToLag(fm_int sw, fm_int lagIndex, fm_int port);
fm_status fm10000DeletePortFromLag(fm_int sw, fm_int lagIndex, fm_int port);

fm_status fm10000InformLAGPortUp(fm_int sw, fm_int port);
fm_status fm10000InformLAGPortDown(fm_int sw, fm_int port);

fm_status fm10000SetLagAttribute(fm_int sw,
                                 fm_int attribute,
                                 fm_int index,
                                 void * value);
fm_status fm10000GetLagAttribute(fm_int sw,
                                 fm_int attribute,
                                 fm_int index,
                                 void * value);
fm_status fm10000GetHardwareLagGlortRange(fm_uint32 *lagGlortBase,
                                          fm_uint32 *lagGlortCount);
fm_status fm10000GetCanonicalLagGlort(fm_int     sw,
                                      fm_uint16  lagMemberGlort,
                                      fm_uint16 *lagCanonicalGlort);


#endif  /* __FM_FM10000_API_LAG_INT_H */
