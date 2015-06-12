/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_sdk_int.h
 * Creation Date:   Feb 21, 2007
 * Description:     Wrapper file to include all needed internal files.
 *                  For SDK internal use only.
 *
 * Copyright (c) 2007 - 2011, Intel Corporation
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

#ifndef __FM_FM_SDK_INT_H
#define __FM_FM_SDK_INT_H

/*
 * Include wrapper to OS & Standard C include files
 *
 * No dependencies upon Fulcrum ControlPoint SDK definitions
 *
 * No known order dependencies
 *
 */
#include <fm_alos_sys_int.h>

/*
 * All external definitions
 *
 * All interdependencies are handled within fm_sdk.h
 *
 */
#include <fm_sdk.h>

/*
 * ALOS
 *
 * Internal code related to the OS abstraction layer.
 *
 */
#include <fm_alos_int.h>

/*
 * Debug
 *
 * No known dependencies other than system and external files above
 */
#include <debug/fm_debug_int.h>

/*
 * all the rest of the SDK
 *
 * the non-API files should really be listed at this level instead of
 * inside fm_api_int.h, but this works for now
 *
 */
#include <api/internal/fm_api_int.h>

/*
 * platform specific code
 *
 */
#include <platform.h>


#endif /* __FM_FM_SDK_INT_H */
