/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform.h
 * Creation Date:   2005
 * Description:     Platform definitions.
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

#ifndef __FM_PLATFORM_H
#define __FM_PLATFORM_H

/* This is the platform-specific header file */
#include <platform_types.h>

/* Default macro definitions for older platforms. */
#include <platforms/platform_defaults.h>

/* Platform layer API services. */
#include <platforms/platform_api.h>

/* Platform layer application services. */
#include <platforms/platform_app.h>

/* Platform layer application services. */
#include <platforms/common/stubs/platform_api_stubs.h>

/**************************************************
 * Internal functions
 **************************************************/

/* Interrupt Management */
fm_status fmPlatformTriggerInterrupt(fm_int sw, fm_uint intrTypes);

/* Port and PHY Management */
fm_status fmPlatformSetPortInterruptMask(fm_int switchNum, fm_int port);
fm_status fmPlatformClearPortInterruptMask(fm_int switchNum, fm_int port);
fm_status fmPlatformGetPortInterruptMask(fm_int switchNum, fm_uint32 *portMask);
fm_status fmInitializePhysicalInterfaces(fm_int switchNum, fm_switch *pSwitch);

/* Raw CSR access */
fm_status fmPlatformReadRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformWriteRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);

#endif	/* __FM_PLATFORM_H */
