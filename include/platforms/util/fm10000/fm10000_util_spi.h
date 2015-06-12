/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_util_spi.h
 * Creation Date:   June 3, 2013
 * Description:     Functions using RRC to access SPI flash.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM10000_UTIL_SPI_H
#define __FM10000_UTIL_SPI_H

fm_status fm10000UtilSpiDoOpCode(fm_uintptr             handle,
                                 fm_utilRegRead32Func   readFunc,
                                 fm_utilRegWrite32Func  writeFunc,
                                 fm_byte               *bytes,
                                 fm_uint                numWrite,
                                 fm_uint                numRead);

fm_status fm10000UtilSpiReadFlash(fm_uintptr             handle,
                                  fm_utilRegRead32Func   readFunc,
                                  fm_utilRegWrite32Func  writeFunc,
                                  fm_uint                address,
                                  fm_byte               *value,
                                  fm_int                 len,
                                  fm_int                 freq);

fm_status fm10000UtilSpiWriteFlash(fm_uintptr             handle,
                                   fm_utilRegRead32Func   readFunc,
                                   fm_utilRegWrite32Func  writeFunc,
                                   fm_uint                address,
                                   fm_byte               *value,
                                   fm_int                 len,
                                   fm_int                 freq);

#endif /* __FM10000_UTIL_SPI_H */
