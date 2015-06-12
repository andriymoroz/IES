/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_dynamic_load.h
 * Creation Date:   2011
 * Description:     ALOS routines for dynamically loading code libraries.
 *
 * Copyright (c) 2011, Intel Corporation
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

#ifndef __FM_FM_ALOS_DYNAMIC_LOAD_H
#define __FM_FM_ALOS_DYNAMIC_LOAD_H


extern fm_status fmOpenDynamicLoadLibrary(fm_text filePath, fm_int *handle);
extern fm_status fmCloseDynamicLoadLibrary(fm_int handle);
extern fm_status fmGetDynamicLoadSymbol(fm_int  handle,
                                        fm_text symName,
                                        void ** symAddr);
extern fm_status fmLoadDynamicLoadLibrary(fm_int handle);
extern fm_uint64 fmProcessDynLoadLibStatus;

#define FM_CALL_DYN_FUNC(handle, funcPtr, status, ...)      \
    if ( !( fmProcessDynLoadLibStatus & (1 << handle) ) )   \
    {                                                       \
        status = fmLoadDynamicLoadLibrary(handle);          \
    }                                                       \
    else                                                    \
    {                                                       \
        status = FM_OK;                                     \
    }                                                       \
    if (status == FM_OK)                                    \
    {                                                       \
        if (funcPtr != NULL)                                \
        {                                                   \
            status = funcPtr(__VA_ARGS__);                  \
        }                                                   \
        else                                                \
        {                                                   \
            status = FM_ERR_UNSUPPORTED;                    \
        }                                                   \
    }


#endif /* __FM_FM_ALOS_DYNAMIC_LOAD_H */
