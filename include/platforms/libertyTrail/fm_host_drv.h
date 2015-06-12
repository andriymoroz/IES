/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_host_drv.h
 * Creation Date:   August 2014
 * Description:     Functions to interface to Host kernel driver.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#ifndef __FM_HOST_DRV_H
#define __FM_HOST_DRV_H

#define FM_UIO_MAX_NAME_SIZE       64

typedef struct
{
    fm_int    uioNum;
    fm_char   name[FM_UIO_MAX_NAME_SIZE];
    fm_char   version[FM_UIO_MAX_NAME_SIZE];
    fm_uint32 addr;
    fm_int    size;
    fm_int    offset;

} fm_uioDriverInfo;

fm_status fmPlatformHostDrvOpen(fm_int  sw,
                                fm_int  mgmtPep, 
                                fm_text netDevName, 
                                fm_text uioDevName, 
                                void *  desiredMemmapAddr);

fm_status fmPlatformHostDrvClose(fm_int sw);

fm_status fmPlatformHostDrvEnableInterrupt(fm_int sw, fm_uint intrTypes);
fm_status fmPlatformHostDrvDisableInterrupt(fm_int sw, fm_uint intrTypes);
fm_status fmPlatformHostDrvWaitForInterrupt(fm_int   sw,
                                            fm_int   timeout,
                                            fm_uint *intrStatus);

fm_status fmPlatformMmapUioDevice(fm_text devName, 
                                  fm_int *fd, 
                                  void ** memmapAddr,
                                  fm_int *size);
fm_status fmPlatformUnmapUioDevice(fm_int fd, void *memmapAddr, fm_int size);

#endif /* end __FM_HOST_DRV_H */
