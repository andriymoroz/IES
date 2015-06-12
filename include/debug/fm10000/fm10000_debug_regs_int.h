/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_regs_int.h
 * Creation Date:   April 27, 2013
 * Description:     Header file for auto-generated debug register data.
 *
 * Copyright (c) 2009 - 2013, Intel Corporation
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

#ifndef __FM_FM10000_DEBUG_REGS_INT_H
#define __FM_FM10000_DEBUG_REGS_INT_H


/**************************************************
 * Register table entry
 **************************************************/

typedef struct
{
    fm_text     regname;
    fm_int      accessMethod;
    fm_uint32   regAddr;
    fm_byte     flags;
    fm_byte     statGroup;
    fm_int      wordcount;
    fm_int      indexMin0;
    fm_int      indexMin1;
    fm_int      indexMin2;
    fm_int      indexMax0;
    fm_int      indexMax1;
    fm_int      indexMax2;
    fm_int      indexStep0;
    fm_int      indexStep1;
    fm_int      indexStep2;

} fm10000DbgFulcrumRegister;


/**************************************************
 * Register access methods.
 **************************************************/

enum
{
    /* Scalar register */
    SCALAR = 0,

    /* Multi-word register. */
    MULTIWRD,

    /* Indexed register. */
    INDEXED,

    /* Multi-word indexed register. */
    MWINDEX,

    /* Doubly indexed register. */
    DBLINDEX,

    /* Multi-word doubly indexed register. */
    MWDBLIDX,

    /* Triply indexed register. */
    TPLINDEX,

    /* Multi-word triply indexed register. */
    MWTPLIDX,

    /* Composite: all registers for port. */
    ALL4PORT,

    /* Group Registers */
    GROUPREG,

    /* All configuration registers */
    ALLCONFG,

    /* Special debug pseudo-registers (not taken from the design) */
    SPECIAL

};


/**************************************************
 * Flag bit masks.
 **************************************************/
 
#define REG_FLAG_INDEX0_PER_PORT        0x01
#define REG_FLAG_INDEX1_PER_PORT        0x02
#define REG_FLAG_INDEX0_PER_EPL         0x04
#define REG_FLAG_INDEX0_CHAN_1_EPL      0x08
#define REG_FLAG_PER_PORT               (REG_FLAG_INDEX0_PER_PORT | \
                                         REG_FLAG_INDEX1_PER_PORT | \
                                         REG_FLAG_INDEX0_CHAN_1_EPL)
#define REG_FLAG_STATISTIC              0x10
#define REG_FLAG_END_OF_REGS            0x80


/**********************************************************************
 * Register table
 **********************************************************************/

extern const fm10000DbgFulcrumRegister fm10000RegisterTable[];

extern const fm_int fm10000RegisterTableSize;


/**************************************************
 * Register field entry.
 **************************************************/

typedef struct
{
    fm_text     name;  /* field name */
    fm_int      start; /* position in bits starting from 0-indexed LSB */
    fm_int      size;  /* length in bits */
} fmRegisterField;


/**********************************************************************
 * Register field lookup
 **********************************************************************/

const fmRegisterField * fm10000DbgGetRegisterFields(fm_text registerName);


#endif  /* __FM_FM10000_DEBUG_REGS_INT_H */

