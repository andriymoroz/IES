/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_com.h
 * Creation Date:   March 8, 2012
 * Description:     Platform functions to access peripheral over USB
 *
 * Copyright (c) 2012 - 2013, Intel Corporation
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

#ifndef __FM_PLATFORM_COM_H
#define __FM_PLATFORM_COM_H

void fmPlatformComSetDebug(fm_int value);

fm_status fmPlatformComInit(fm_text devName, int *fd);
fm_status fmPlatformComSyncCPLD(int fd);

fm_status fmPlatformComExecCmd(int    fd,
                               fm_uint32 cmd,
                               fm_uint32 addr,
                               fm_byte * value,
                               fm_int    length);
fm_status fmPlatformComI2cWriteRead(int   fd,
                                    fm_int   device,
                                    fm_byte *data,
                                    fm_int   wl,
                                    fm_int   rl);

fm_status fmPlatformDumpCPLDRegFields(fm_int regOff, fm_uint32 value);
fm_status fmPlatformDumpFPGARegFields(fm_int regOff, fm_uint32 value);


fm_status fmPlatformComSMBusWriteRead(int   fd,
                                      fm_int   device,
                                      fm_byte *data,
                                      fm_int   wl,
                                      fm_int   rl);
fm_status fmPlatformComSMBusWritePEC(int   fd,
                                     fm_int   device,
                                     fm_byte *data,
                                     fm_int   wl);
fm_status fmPlatformComSMBusReadPEC(int   fd,
                                    fm_int   device,
                                    fm_byte *data,
                                    fm_int   rl);
fm_status fmPlatformComSMBusRead(int   fd,
                                 fm_int   device,
                                 fm_byte *data,
                                 fm_int   rl);

#endif /* __FM_PLATFORM_COM_H */
