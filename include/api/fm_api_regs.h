/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_regs.h
 * Creation Date:   April 20, 2005
 * Description:     Basic register access functions
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_API_REGS_H
#define __FM_FM_API_REGS_H

/**************************************************/
/** \ingroup typeStruct
 * fm_scatterGatherListEntry defines a single
 * contiguous range of register space to read or
 * write.  An array of these structures can be
 * passed to the platform-defined functions pointed
 * to by the ''ReadScatterGather'' or 
 * ''WriteScatterGather'' function pointers to read 
 * or write multiple non-contiguous regions
 * in a single function call.  Use of these functions
 * can greatly improve the performance of access to 
 * FIBM-controlled remote switches.
 **************************************************/
typedef struct _fm_scatterGatherListEntry
{
    /** Starting register address to read/write. */
    fm_uint32  addr;

    /** Number of words to read/write. */
    fm_uint32  count;

    /** Array of count words, that the read data is written into
     *  or the write data is taken from. The array must be
     *  at least count words long. */
    fm_uint32 *data;

} fm_scatterGatherListEntry;



/* reads a single 32-bit integer */
fm_status fmReadUINT32(fm_int sw, fm_uint reg, fm_uint32 *ptr);


/* reads a single un-cached 32-bit integer */
fm_status fmReadUncachedUINT32(fm_int sw, fm_uint reg, fm_uint32 *ptr);


/* reads multiple consecutive 32-bit integers */
fm_status fmReadUINT32Mult(fm_int sw, fm_uint reg, fm_int wordCount, fm_uint32 *ptr);


/* reads multiple un-cached consecutive 32 bit integers */
fm_status fmReadUncachedUINT32Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint32 *ptr);


/* reads a single 64-bit integer */
fm_status fmReadUINT64(fm_int sw, fm_uint reg, fm_uint64 *ptr);


/* reads a un-cached 64-bit integer */
fm_status fmReadUncachedUINT64(fm_int sw, fm_uint reg, fm_uint64 *ptr);


/* reads multiple consecutive 64-bit integers */
fm_status fmReadUINT64Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint64 *ptr);


/* reads multiple consecutive un-cached 64-bit integers */
fm_status fmReadUncachedUINT64Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint64 *ptr);


/* read multiple discontiguous regions */
fm_status fmReadScatterGather(fm_int                           sw,
                              fm_int                           nEntries,
                              fm_scatterGatherListEntry *      sgList);


/* writes a single 32-bit integer */
fm_status fmWriteUINT32(fm_int sw, fm_uint reg, fm_uint32 value);


/* writes a field in a single 32-bit register */
fm_status fmWriteUINT32Field(fm_int sw,
                             fm_int addr,
                             fm_int startingBit,
                             fm_int length,
                             fm_int value);


/* writes multiple consecutive 32-bit integers */
fm_status fmWriteUINT32Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint32 *ptr);


/* Sets or clears a bit mask in a 32-bit integer. */
fm_status fmMaskUINT32(fm_int sw, fm_uint reg, fm_uint32 mask, fm_bool on);


/* writes a single 64-bit integer */
fm_status fmWriteUINT64(fm_int sw, fm_uint reg, fm_uint64 value);


/* writes multiple consecutive 64-bit integers */
fm_status fmWriteUINT64Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint64 *ptr);


/* write multiple discontiguous regions */
fm_status fmWriteScatterGather(fm_int                           sw,
                               fm_int                           nEntries,
                               fm_scatterGatherListEntry *      sgList);


/* writes a single 32-bit integer through I2C */
fm_status fmI2cWriteUINT32(fm_int sw, fm_uint reg, fm_uint32 value);


/* reads a single 32-bit integer through I2C */
fm_status fmI2cReadUINT32(fm_int sw, fm_uint reg, fm_uint32 *value);


/* writes multiple consecutive 32-bit integers through I2C */
fm_status fmI2cWriteUINT32Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint32 *ptr);


/* reads multiple consecutive 32-bit integers through I2C */
fm_status fmI2cReadUINT32Mult(fm_int sw, fm_uint reg, fm_int n, fm_uint32 *value);


/* writes a sequence of UINT32 register without taking the platform lock */
fm_status fmEmulateWriteRawUINT32Seq(fm_int sw, fm_uint32 *addr, fm_uint32 *value, fm_int n);

/* Use switch I2C interface to access other I2C devices */
fm_status fmI2cWriteRead(fm_int     sw,
                         fm_uint    device,
                         fm_byte   *data,
                         fm_uint    wl,
                         fm_uint    rl);

#endif /* __FM_FM_API_REGS_H */
