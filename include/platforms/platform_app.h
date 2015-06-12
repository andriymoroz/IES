/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_app.h
 * Creation Date:   January 8, 2013
 * Description:     Platform layer application services.
 * 
 * Defines services that are generally needed by all platforms and may be
 * called upon directly by the system application software.
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

#ifndef __FM_PLATFORM_APP_H
#define __FM_PLATFORM_APP_H


/**************************************************
 * Platform attributes
 **************************************************/

fm_status fmPlatformGetAttribute(fm_int    sw,
                                 fm_int    index,
                                 fm_uint32 attr,
                                 void *    value);

fm_status fmPlatformSetAttribute(fm_int    sw,
                                 fm_int    index,
                                 fm_uint32 attr,
                                 void *    value);


/**************************************************
 * I2C services
 **************************************************/

fm_status fmPlatformI2cRead(fm_int     sw,
                            fm_int     bus,
                            fm_int     address,
                            fm_uint32 *value,
                            fm_int     length);

fm_status fmPlatformI2cWrite(fm_int    sw,
                             fm_int    bus,
                             fm_int    address,
                             fm_uint32 value,
                             fm_int    length);

fm_status fmPlatformI2cWriteLong(fm_int    sw,
                                 fm_int    bus,
                                 fm_int    address,
                                 fm_uint64 data,
                                 fm_int    length);

fm_status fmPlatformI2cWriteRead(fm_int     sw,
                                 fm_int     bus,
                                 fm_int     address,
                                 fm_uint32 *data,
                                 fm_int     write_length,
                                 fm_int     read_length);


/**************************************************
 * MDIO services
 **************************************************/

fm_status fmPlatformMdioRead(fm_int     sw,
                             fm_int     bus,
                             fm_int     address,
                             fm_int     device,
                             fm_int     reg,
                             fm_uint16 *value);

fm_status fmPlatformMdioWrite(fm_int    sw,
                              fm_int    bus,
                              fm_int    address,
                              fm_int    device,
                              fm_int    reg,
                              fm_uint16 value);


#endif /* __FM_PLATFORM_APP_H */

