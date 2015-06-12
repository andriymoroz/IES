/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_regs_access_memmap.h
 * Creation Date:   August 2, 2013
 * Description:     Functions to access switch registers using memmap
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

#ifndef __FM_REGS_ACCESS_EBI_H
#define __FM_REGS_ACCESS_EBI_H

fm_status fmPlatformEbiInit(fm_int sw, fm_text devName);

fm_status fmPlatformEbiReadCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformEbiWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);
fm_status fmPlatformEbiReadCSRMult(fm_int     sw,
                                   fm_uint32  addr,
                                   fm_int     n,
                                   fm_uint32 *value);
fm_status fmPlatformEbiWriteCSRMult(fm_int     sw,
                                    fm_uint32  addr,
                                    fm_int     n,
                                    fm_uint32 *value);
fm_status fmPlatformEbiReadCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value);
fm_status fmPlatformEbiWriteCSR64(fm_int sw, fm_uint32 addr, fm_uint64 value);
fm_status fmPlatformEbiReadCSRMult64(fm_int     sw,
                                     fm_uint32  addr,
                                     fm_int     n,
                                     fm_uint64 *value);
fm_status fmPlatformEbiWriteCSRMult64(fm_int     sw,
                                      fm_uint32  addr,
                                      fm_int     n,
                                      fm_uint64 *value);
fm_status fmPlatformEbiReadRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformEbiWriteRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);
fm_status fmPlatformEbiWriteRawCSRSeq(fm_int     sw,
                                      fm_uint32 *addr,
                                      fm_uint32 *value,
                                      fm_int     n);
fm_status fmPlatformEbiMaskCSR(fm_int    sw,
                               fm_uint   reg,
                               fm_uint32 mask,
                               fm_bool   on);

#endif  /* __FM_REGS_ACCESS_EBI_H */
