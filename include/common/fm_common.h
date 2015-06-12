/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_common.h
 * Creation Date:   2005
 * Description:     Wrapper to include all common header files
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_FM_COMMON_H
#define __FM_FM_COMMON_H


#define FM_DISABLED  FALSE
#define FM_ENABLED   TRUE


/**********************************************************************
 * This file is the primary include file for the externally-exposed
 * "common" portions of the Fulcrum ControlPoint SDK.
 *
 * Internal dependencies inside this subsystem should be documented here.
 * The only external requirements allowed here are the architectural
 * definitions.
 * Any order-dependencies amongst these files must be clearly documented
 * here.
 * As of 04/17/2007, there were no order-dependencies amongst the files
 * referenced here.
 * As of 11/13/2007, fm_dlist.h depends on fmFreeFunc, which is defined
 * in fm_tree.h.
 **********************************************************************/

#include <common/fm_errno.h>
#include <common/fm_tree.h>
#include <common/fm_dlist.h>
#include <common/fm_bitarray.h>
#include <common/fm_bitfield.h>
#include <common/fm_crc32.h>
#include <common/fm_property.h>
#include <common/fm_string.h>
#include <common/fm_md5.h>
#include <common/fm_lock_prec.h>
#include <common/fm_array.h>
#include <common/fm_graycode.h>
#include <common/fm_c11_annex_k.h>
#include <common/fm_state_machine.h>

#if defined(FM_SUPPORT_FM2000)
#include <common/fm2000_property.h>
#endif

#if defined(FM_SUPPORT_FM4000)
#include <common/fm4000_property.h>
#endif

#if defined(FM_SUPPORT_FM6000)
#include <common/fm6000_property.h>
#endif

#if defined(FM_SUPPORT_FM10000)
#include <common/fm10000_property.h>
#endif


/** \ingroup macroSystem
 *  Macro used to suppress compiler warnings when a function parameter is
 *  deliberately not used in a function. */
#define FM_NOT_USED(a) (void) (a)

/** \ingroup macroSystem
 *  Macro used to zero an array or structure. */
#define FM_CLEAR(object) \
    FM_MEMSET_S( &(object), sizeof(object), 0, sizeof(object) )

/* The absolute difference between two values. */
#define FM_DELTA(a, b)  ( (a) > (b) ? (a) - (b) : (b) - (a) )

/* Combines two error codes. */
#define FM_ERR_COMBINE(oldErr, newErr) \
    if ((oldErr) == FM_OK)             \
    {                                  \
        (oldErr) = (newErr);           \
    }

/* The number of entries of a statically allocated array. */
#define FM_NENTRIES(x)  ( sizeof((x)) / sizeof((x)[0]) )

/* A typedef'd union to allow portable conversion of integers to pointers,
 * etc., designed to work in both 32-bit and 64-bit systems. */
typedef union
{
    fm_int    int32[2];
    fm_uint32 uint32[2];
    fm_int64  int64;
    fm_uint64 uint64;
    void *    ptr;

} fm_sizeConverter;

#ifdef __LITTLE_ENDIAN__
#define FM_SIZECONV_PUT_INT32(sc, i) { FM_CLEAR(sc); sc.int32[0] = i; }
#define FM_SIZECONV_GET_INT32(sc) sc.int32[0]

#define FM_SIZECONV_PUT_UINT32(sc, u) { FM_CLEAR(sc); sc.uint32[0] = u; }
#define FM_SIZECONV_GET_UINT32(sc) sc.uint32[0]
#else
#define FM_SIZECONV_PUT_INT32(sc, i) { FM_CLEAR(sc); sc.int32[1] = i; }
#define FM_SIZECONV_GET_INT32(sc) sc.int32[1]

#define FM_SIZECONV_PUT_UINT32(sc, u) { FM_CLEAR(sc); sc.uint32[1] = u; }
#define FM_SIZECONV_GET_UINT32(sc) sc.uint32[1]
#endif

#define FM_SIZECONV_PUT_INT64(sc, i) { FM_CLEAR(sc); sc.int64 = i; }
#define FM_SIZECONV_GET_INT64(sc) sc.int64

#define FM_SIZECONV_PUT_UINT64(sc, u) { FM_CLEAR(sc); sc.uint64 = u; }
#define FM_SIZECONV_GET_UINT64(sc) sc.uint64

#define FM_SIZECONV_PUT_PTR(sc, p) { FM_CLEAR(sc); sc.ptr = p; }
#define FM_SIZECONV_GET_PTR(sc) sc.ptr

#endif /* __FM_FM_COMMON_H */
