/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_regs_access_i2c.h
 * Creation Date:   August 2, 2013
 * Description:     Functions to access switch registers using I2C.
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

#ifndef __FM_REGS_ACCESS_I2C_H
#define __FM_REGS_ACCESS_I2C_H

fm_status fmPlatformI2cReadCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformI2cWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);
fm_status fmPlatformI2cReadCSRMult(fm_int     sw,
                                   fm_uint32  addr,
                                   fm_int     n,
                                   fm_uint32 *value);
fm_status fmPlatformI2cWriteCSRMult(fm_int     sw,
                                    fm_uint32  addr,
                                    fm_int     n,
                                    fm_uint32 *value);
fm_status fmPlatformI2cReadCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value);
fm_status fmPlatformI2cWriteCSR64(fm_int sw, fm_uint32 addr, fm_uint64 value);
fm_status fmPlatformI2cReadCSRMult64(fm_int     sw,
                                     fm_uint32  addr,
                                     fm_int     n,
                                     fm_uint64 *value);
fm_status fmPlatformI2cWriteCSRMult64(fm_int     sw,
                                      fm_uint32  addr,
                                      fm_int     n,
                                      fm_uint64 *value);
fm_status fmPlatformI2cMaskCSR(fm_int    sw,
                               fm_uint   reg,
                               fm_uint32 mask,
                               fm_bool   on);

fm_status fmPlatformI2cReadPreBootCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformI2cWritePreBootCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);

fm_status fmPlatformI2cReadPreBootCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value);


#endif  /* __FM_REGS_ACCESS_I2C_H */
