/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_portmask.h
 * Creation Date:   June 22, 2009.
 * Description:     Port mask data type.
 *
 * Copyright (c) 2009 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_PORTMASK_H
#define __FM_FM_API_PORTMASK_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/** The number of 32-bit words in the port mask. */
#define FM_PORTMASK_NUM_WORDS   3

/* The number of bits in the port mask. */
#define FM_PORTMASK_NUM_BITS    (FM_PORTMASK_NUM_WORDS * 32)

/* Legacy function name. */
#define fmPortMaskCounter(portmask, portcount)  \
        fmGetPortMaskCount(portmask, portcount)

#define FM_PORTMASK_ENABLE_PORT(maskPtr, portNo) \
        FM_PORTMASK_ENABLE_BIT(maskPtr, portNo)


/**
 * Represents a mask of all the physical ports on a switch. 
 *  
 * Note the fm_portmask is only supposed to be used for sets of ports 
 * within a PHYSICAL switch. It is intended to be highly efficient, 
 * not requiring dynamic memory allocation. For SWAGs, you are supposed 
 * to use an fm_bitArray or possibly a portset. 
 *  
 * The whole point of fm_portmask is to allow us to use the same data 
 * structure to represent device-level port masks on Bali and Alta, with 
 * as little storage and run-time overhead as possible. 
 */
typedef struct _fm_portmask
{
    fm_uint32   maskWord[FM_PORTMASK_NUM_WORDS];

} fm_portmask;


/**
 * Enables the specified bit in the port mask.
 *
 * \param[in,out]   maskPtr points to the port mask to be modified. 
 * \param[in]       bitNo is the bit number. 
 */
#define FM_PORTMASK_ENABLE_BIT(maskPtr, bitNo) \
    (maskPtr)->maskWord[(bitNo) >> 5] |= (1 << ((bitNo) & 0x1f))


/**
 * Disables the specified bit in the port mask.
 *
 * \param[in,out]   maskPtr points to the port mask to be modified. 
 * \param[in]       bitNo is the bit number. 
 */
#define FM_PORTMASK_DISABLE_BIT(maskPtr, bitNo) \
    (maskPtr)->maskWord[(bitNo) >> 5] &= ~(1 << ((bitNo) & 0x1f))


/**
 * Returns the state of the specified bit in the port mask.
 *
 * \param[in]       maskPtr points to the port mask to be tested.
 * \param[in]       bitNo is the bit number. 
 *  
 * \return          0 if the bit is disabled, 1 if it is enabled. 
 */
#define FM_PORTMASK_GET_BIT(maskPtr, bitNo) \
    ( ((maskPtr)->maskWord[(bitNo) >> 5] >> ((bitNo) & 0x1f)) & 1 )


/**
 * Determines whether the specified bit is enabled in the port mask.
 *
 * \param[in]       maskPtr points to the port mask to be tested.
 * \param[in]       bitNo is the bit number. 
 *  
 * \return          FALSE if the bit is disabled, TRUE if it is enabled. 
 */
#define FM_PORTMASK_IS_BIT_SET(maskPtr, bitNo) \
    ( ((maskPtr)->maskWord[(bitNo) >> 5] & (1 << ((bitNo) & 0x1f))) != 0 )


/**
 * Sets the state of the specified bit in the port mask.
 *
 * \param[in,out]   maskPtr points to the port mask to be modified. 
 * \param[in]       bitNo is the bit number. 
 * \param[in]       state is 0 to disable the bit, 1 to enable it.
 */
#define FM_PORTMASK_SET_BIT(maskPtr, bitNo, state)  \
    FM_PORTMASK_DISABLE_BIT(maskPtr, bitNo);        \
    if (state)                                      \
    {                                               \
        FM_PORTMASK_ENABLE_BIT(maskPtr, bitNo);     \
    }


/**
 * Enables exactly one bit in the port mask.
 *
 * \param[out]      maskPtr points to the port mask to be modified. 
 * \param[in]       bitNo is the bit number. 
 */
#define FM_PORTMASK_ASSIGN_BIT(maskPtr, bitNo)      \
{                                                   \
    FM_PORTMASK_DISABLE_ALL(maskPtr);               \
    FM_PORTMASK_ENABLE_BIT(maskPtr, bitNo);         \
}


/**
 * Enables all the ports in the port mask.
 *
 * \param[out]  maskPtr points to the port mask to be modified. 
 *  
 * \param[in]   numPorts is the number of ports in the mask. 
 */
#define FM_PORTMASK_ENABLE_ALL(maskPtr, numPorts)   \
    fmPortMaskEnableAll(maskPtr, numPorts)


/**
 * Disables all the ports in the port mask. 
 *  
 * Note that this function zeroes the entire port mask.
 *
 * \param[out]  maskPtr points to the port mask to be modified.
 */
#define FM_PORTMASK_DISABLE_ALL(maskPtr) \
    memset( (maskPtr), 0, sizeof(fm_portmask) )


/**
 * Logically ANDs two port masks, producing a third. 
 *  
 * \param[out]  maskPtrX points to the port mask to receive the result. 
 *              May be the same as one of the other operands. 
 * \param[in]   maskPtrA points to the first operand. 
 * \param[in]   maskPtrB points to the second operand. 
 *  
 * \note        Loop may be unrolled for efficiency. 
 *              (The compiler may do this for us.)
 */
#define FM_AND_PORTMASKS(maskPtrX, maskPtrA, maskPtrB)  \
{                                                               \
    fm_int j;                                                   \
    for (j = 0 ; j < FM_PORTMASK_NUM_WORDS ; ++j)               \
    {                                                           \
        (maskPtrX)->maskWord[j] =                               \
            (maskPtrA)->maskWord[j] & (maskPtrB)->maskWord[j];  \
    }                                                           \
}


/**
 * Logically ANDs the first portmask with the invert of the second.
 *  
 * \param[out]  maskPtrX points to the port mask to receive the result. 
 *              May be the same as one of the other operands. 
 * \param[in]   maskPtrA points to the first operand. 
 * \param[in]   maskPtrB points to the second operand. 
 *  
 * \note        Loop may be unrolled for efficiency. 
 *              (The compiler may do this for us.)
 */
#define FM_AND_INV_PORTMASKS(maskPtrX, maskPtrA, maskPtrB)  \
{                                                               \
    fm_int j;                                                   \
    for (j = 0 ; j < FM_PORTMASK_NUM_WORDS ; ++j)               \
    {                                                           \
        (maskPtrX)->maskWord[j] =                               \
            (maskPtrA)->maskWord[j] & ~((maskPtrB)->maskWord[j]); \
    }                                                           \
}


/**
 * Logically ORs two port masks, producing a third. 
 *  
 * \param[out]  maskPtrX points to the port mask to receive the result. 
 *              May be the same as one of the other operands. 
 * \param[in]   maskPtrA points to the first operand. 
 * \param[in]   maskPtrB points to the second operand. 
 *  
 * \note        Loop may be unrolled for efficiency. 
 *              (The compiler may do this for us.)
 */
#define FM_OR_PORTMASKS(maskPtrX, maskPtrA, maskPtrB)           \
{                                                               \
    fm_int j;                                                   \
    for (j = 0 ; j < FM_PORTMASK_NUM_WORDS ; ++j)               \
    {                                                           \
        (maskPtrX)->maskWord[j] =                               \
            (maskPtrA)->maskWord[j] | (maskPtrB)->maskWord[j];  \
    }                                                           \
}


/** 
 * Inverse port mask.
 *  
 * \param[out]  maskPtrX points to the port mask to receive the result. 
 *              May be the same as one of the operand. 
 * \param[in]   maskPtrA points to the operand. 
 *  
 * \note        Loop may be unrolled for efficiency. 
 *              (The compiler may do this for us.)
 */
#define FM_INVERSE_PORTMASKS(maskPtrX, maskPtrA)                \
{                                                               \
    fm_int j;                                                   \
    for (j = 0 ; j < FM_PORTMASK_NUM_WORDS ; ++j)               \
    {                                                           \
        (maskPtrX)->maskWord[j] = ~(maskPtrA)->maskWord[j];     \
    }                                                           \
}


/**
 * Logically ANDs one port mask with the complement of another, removing
 * the ports in the second mask from the first mask. 
 *  
 * In set theory, this is the relative complement, or set-theoretic 
 * difference, of A in B, A - B. 
 *  
 * \param[out]  maskPtrX points to the port mask to receive the result. 
 *              May be the same as one of the other operands. 
 * \param[in]   maskPtrA points to the first operand. 
 * \param[in]   maskPtrB points to the second operand. 
 *  
 * \note        Loop may be unrolled for efficiency. 
 *              (The compiler may do this for us.)
 */
#define FM_PORTMASK_CLEAR_PORTS(maskPtrX, maskPtrA, maskPtrB)   \
{                                                               \
    fm_int j;                                                   \
    for (j = 0 ; j < FM_PORTMASK_NUM_WORDS ; ++j)               \
    {                                                           \
        (maskPtrX)->maskWord[j] =                               \
            (maskPtrA)->maskWord[j] & ~(maskPtrB)->maskWord[j]; \
    }                                                           \
}


/**
 * Determines whether the port mask is zero. 
 *  
 * \param[in]   maskPtr points to the port mask to be tested. 
 *  
 * \note        The loop has been unrolled for efficiency. We also test 
 *              the low-order word first, since this is the only one that
 *              will be set for the FM2000 or FM4000.
 */
#define FM_PORTMASK_IS_ZERO(maskPtr)    \
    ( ((maskPtr)->maskWord[0] == 0) &&      \
      ((maskPtr)->maskWord[1] == 0) &&      \
      ((maskPtr)->maskWord[2] == 0) )


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/* Enables all the ports in the port mask. */
void fmPortMaskEnableAll(fm_portmask* maskPtr, fm_uint numPorts);

/* Converts a port mask to a bit array. */
fm_status fmPortMaskToBitArray(fm_portmask* maskPtr,
                               fm_bitArray* arrayPtr,
                               fm_int       numPorts);

/* Converts a bit array to a port mask. */
fm_status fmBitArrayToPortMask(fm_bitArray* arrayPtr,
                               fm_portmask* maskPtr,
                               fm_int       numPorts);

/* Converts a logical port bit array to a physical port mask. */
fm_status fmBitArrayLogicalToPhysMask(fm_switch *  switchPtr,
                                      fm_bitArray* arrayPtr,
                                      fm_portmask* maskPtr,
                                      fm_int       numPorts);

/* Maps logical port mask to physical port mask. */
fm_status fmPortMaskLogicalToPhysical(fm_switch *   switchPtr,
                                      fm_portmask * logMask,
                                      fm_portmask * physMask);

/* Maps physical port mask to logical port mask. */
fm_status fmPortMaskPhysicalToLogical(fm_switch *   switchPtr,
                                      fm_portmask * physMask,
                                      fm_portmask * logMask);

/* Maps logical port mask to link-up mask. */
fm_status fmPortMaskLogicalToLinkUpMask(fm_switch *   switchPtr,
                                        fm_portmask * logMask,
                                        fm_portmask * upMask);

/* Counts the number of ports in a port mask. */
fm_status fmGetPortMaskCount(fm_portmask * portMask,
                             fm_int *      portCount);

#endif /* __FM_FM_API_PORTMASK_H */
