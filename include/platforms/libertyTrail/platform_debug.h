/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_debug.h
 * Creation Date:   June 2, 2014
 * Description:     Platform debug functions.
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

#ifndef __FM_PLATFORM_DEBUG_H
#define __FM_PLATFORM_DEBUG_H

void fmPlatformHexDump(fm_int addr, fm_byte *buf, fm_int nbytes);

fm_status fmPlatformDoDebug(fm_int  sw,
                            fm_int  port,
                            fm_text action,
                            fm_text args);

fm_status fmPlatformDumpXcvrEeprom(fm_int sw, fm_int port);

fm_status fmPlatformDumpXcvrState(fm_int sw, fm_int port);

fm_status fmPlatformRetimerDumpInfo(fm_int  sw, 
                                    fm_int  idx, 
                                    fm_text cmd, 
                                    fm_text arg);

fm_status fmPlatformRetimerDumpPortMap(fm_int sw, fm_int port);

fm_status fmPlatformRetimerSetAppMode(fm_int sw, fm_int port, fm_int mode);

fm_status fmPlatformRetimerSetLaneTxEq(fm_int sw,
                                       fm_int port,
                                       fm_int internal,
                                       fm_int polarity,
                                       fm_int preTap,
                                       fm_int attenuation,
                                       fm_int postTap);

fm_status fmPlatformRetimerRegisterRead(fm_int     sw, 
                                        fm_int     phyIdx,
                                        fm_int     reg, 
                                        fm_uint32 *value);

fm_status fmPlatformRetimerRegisterWrite(fm_int     sw, 
                                         fm_int     phyIdx,
                                         fm_int     reg, 
                                         fm_uint32  value);

#endif /* __FM_PLATFORM_DEBUG_H */
