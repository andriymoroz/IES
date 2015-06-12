/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_bitfield.h
 * Creation Date:  March 19, 2007
 * Description:    Macros to manipulate bitfields using the bit
 *                 position constants in fmxxxx_api_regs_int.h
 *
 * Copyright (c) 2007 - 2014, Intel Corporation
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

#ifndef __FM_FM_BITFIELD_H
#define __FM_FM_BITFIELD_H

/** \ingroup macroBitfield
 * @{ */

/** Get the width of a named field. */
#define FM_FIELD_WIDTH(regname, fieldname) \
    (1 + regname ## _h_ ## fieldname - regname ## _l_ ## fieldname)

/** Get the maximum unsigned value a named field (< 33 bits) can hold. */
#define FM_FIELD_UNSIGNED_MAX(regname, fieldname) \
    ( ( 2 << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 )

/** Get the maximum signed value a named field (< 33 bits) can hold. */
#define FM_FIELD_SIGNED_MAX(regname, fieldname) \
    ( ( 1 << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 )

/** Get the minimum signed value a named field (< 33 bits) can hold. */
#define FM_FIELD_SIGNED_MIN(regname, fieldname) \
    ( -( 1 << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) )

/** Get the maximum unsigned value a named field (> 32 bits) can hold. */
#define FM_FIELD_UNSIGNED_MAX64(regname, fieldname) \
    ( ( FM_LITERAL_U64(2) <<                        \
       (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 )

/** Get the maximum signed value a named field (> 32 bits) can hold. */
#define FM_FIELD_SIGNED_MAX64(regname, fieldname) \
    ( ( FM_LITERAL_64(1) <<                       \
       (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 )

/** Get the minimum signed value a named field (> 32 bits) can hold. */
#define FM_FIELD_SIGNED_MIN64(regname, fieldname) \
    ( -( FM_LITERAL_64(1) <<                      \
        (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) )

/** Define the mask for a 1-bit named field in a 32-bit value. */
#define FM_MASKOF_BIT(regname, bitname) \
    (1 << regname ## _b_ ## bitname)

/** Define the mask for a 1-bit named field in a 64-bit value. */
#define FM_MASKOF_BIT64(regname, bitname) \
    (FM_LITERAL_U64(1) << regname ## _b_ ## bitname)

/** Define the mask for a 2..31 bit named field in a 32-bit value. */
#define FM_MASKOF_FIELD(regname, fieldname) \
    (FM_FIELD_UNSIGNED_MAX(regname, fieldname) << regname ## _l_ ## fieldname)

/** Define the mask for a 2..63 bit named field in a 64-bit value. */
#define FM_MASKOF_FIELD64(regname, fieldname) \
    (FM_FIELD_UNSIGNED_MAX64(regname, fieldname) << regname ## _l_ ## fieldname)

/** Get a named field of 2-32 bits within a 32-bit value. */
#define FM_GET_FIELD(rvalue, regname, fieldname) \
    ( (rvalue >> regname ## _l_ ## fieldname) &  \
     ( ( 2 << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 ) )

/** Set a named field of 2-32 bits within a 32-bit value. */
#define FM_SET_FIELD(lvalue, regname, fieldname, fieldvalue)                                   \
    (lvalue ^= ( ( (lvalue >> regname ## _l_ ## fieldname) ^ fieldvalue ) &                    \
                ( ( 2 << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - 1 ) ) \
               << regname ## _l_ ## fieldname)

/** Get a named field of 1 bit within a 32-bit value. */
#define FM_GET_BIT(rvalue, regname, bitname) \
    ( (rvalue >> regname ## _b_ ## bitname) & 1 )

/** Set a named field of 1 bit within a 32-bit value. */
#define FM_SET_BIT(lvalue, regname, bitname, bitvalue)          \
    ( lvalue = ( lvalue & ~(1 << regname ## _b_ ## bitname) ) | \
               ( (bitvalue & 1) << regname ## _b_ ## bitname ) )

/** Get a named field of 2-64 bits within a 64-bit value. */
#define FM_GET_FIELD64(rvalue, regname, fieldname)                                            \
    ( (rvalue >> regname ## _l_ ## fieldname) &                                               \
     ( ( FM_LITERAL_U64(2) << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - \
      FM_LITERAL_U64(1) ) )

/** Set a named field of 2-64 bits within a 64-bit value. */
#define FM_SET_FIELD64(lvalue, regname, fieldname, fieldvalue)                                           \
    (lvalue ^= ( ( (lvalue >> regname ## _l_ ## fieldname) ^ fieldvalue ) &                              \
                ( ( FM_LITERAL_U64(2) << (regname ## _h_ ## fieldname - regname ## _l_ ## fieldname) ) - \
                 FM_LITERAL_U64(1) ) ) << regname ## _l_ ## fieldname)

/** Get a named field of 1 bit within a 64-bit value. */
#define FM_GET_BIT64(rvalue, regname, bitname) \
    ( (rvalue >> regname ## _b_ ## bitname) & 1 )

/** Set a named field of 1 bit within a 64-bit value. */
#define FM_SET_BIT64(lvalue, regname, bitname, bitvalue)                      \
    ( lvalue = ( lvalue & ~(FM_LITERAL_U64(1) << regname ## _b_ ## bitname) ) \
               | ( ( bitvalue & FM_LITERAL_U64(1) ) << regname ## _b_ ## bitname ) )

/** Get a named field of 2-32 bits within a >64-bit value. */
#define FM_ARRAY_GET_FIELD(array, regname, fieldname)            \
    fmMultiWordBitfieldGet32(array, regname ## _h_ ## fieldname, \
                             regname ## _l_ ## fieldname)

/** Set a named field of 2-32 bits within a >64-bit value. */
#define FM_ARRAY_SET_FIELD(array, regname, fieldname, fieldvalue) \
    fmMultiWordBitfieldSet32(array, regname ## _h_ ## fieldname,  \
                             regname ## _l_ ## fieldname, fieldvalue)

/** Get a named field of 1 bit within a >64-bit value. */
#define FM_ARRAY_GET_BIT(array, regname, bitname)              \
    fmMultiWordBitfieldGet32(array, regname ## _b_ ## bitname, \
                             regname ## _b_ ## bitname)

/** Set a named field of 1 bit within a >64-bit value. */
#define FM_ARRAY_SET_BIT(array, regname, bitname, bitvalue)    \
    fmMultiWordBitfieldSet32(array, regname ## _b_ ## bitname, \
                             regname ## _b_ ## bitname, bitvalue)

/** Get a named field of 33-64 bits within a >64-bit value. */
#define FM_ARRAY_GET_FIELD64(array, regname, fieldname)          \
    fmMultiWordBitfieldGet64(array, regname ## _h_ ## fieldname, \
                             regname ## _l_ ## fieldname)

/** Get a named portmask field within a >64-bit value. */
#define FM_ARRAY_GET_PORTMASK(array, regname, fieldname, portmaskPtr)   \
    fmMultiWordBitfieldGetPortmask(array,                               \
                                   regname ## _h_ ## fieldname,         \
                                   regname ## _l_ ## fieldname,         \
                                   portmaskPtr)

/** Set a named portmask field within a >64-bit value. */
#define FM_ARRAY_SET_PORTMASK(array, regname, fieldname, portmaskPtr)   \
    fmMultiWordBitfieldSetPortmask(array,                               \
                                   regname ## _h_ ## fieldname,         \
                                   regname ## _l_ ## fieldname,         \
                                   portmaskPtr)

/** Set a named field of 33-64 bits within a >64-bit value. */
#define FM_ARRAY_SET_FIELD64(array, regname, fieldname, fieldvalue) \
    fmMultiWordBitfieldSet64(array, regname ## _h_ ## fieldname,    \
                             regname ## _l_ ## fieldname, fieldvalue)

/** Get a named field of 2-32 bits within an array of 64-bit values. */
#define FM_ARRAY64_GET_FIELD(array, regname, fieldname)            \
    fmMulti64BitWordBitfieldGet32(array, regname ## _h_ ## fieldname, \
                                  regname ## _l_ ## fieldname)

/** Set a named field of 2-32 bits within an array of 64-bit values. */
#define FM_ARRAY64_SET_FIELD(array, regname, fieldname, fieldvalue) \
    fmMulti64BitWordBitfieldSet32(array, regname ## _h_ ## fieldname,  \
                                  regname ## _l_ ## fieldname, fieldvalue)

/** Get a named field of 1 bit within an array of 64-bit values. */
#define FM_ARRAY64_GET_BIT(array, regname, bitname)              \
    fmMulti64BitWordBitfieldGet32(array, regname ## _b_ ## bitname, \
                                  regname ## _b_ ## bitname)

/** Set a named field of 1 bit within an array of 64-bit values. */
#define FM_ARRAY64_SET_BIT(array, regname, bitname, bitvalue)    \
    fmMulti64BitWordBitfieldSet32(array, regname ## _b_ ## bitname, \
                                  regname ## _b_ ## bitname, bitvalue)

/** Get a named field of 33-64 bits within an array of 64-bit values. */
#define FM_ARRAY64_GET_FIELD64(array, regname, fieldname)          \
    fmMulti64BitWordBitfieldGet64(array, regname ## _h_ ## fieldname, \
                                  regname ## _l_ ## fieldname)

/** Set a named field of 33-64 bits within an array of 64-bit values. */
#define FM_ARRAY64_SET_FIELD64(array, regname, fieldname, fieldvalue) \
    fmMulti64BitWordBitfieldSet64(array, regname ## _h_ ## fieldname,    \
                                  regname ## _l_ ## fieldname, fieldvalue)

/** Set a named field of 1-32 bits within a 32-bit field array value. */
#define FM_SET_FIELD_ARRAY(lvalue, regname, fieldname, fieldindex, fieldvalue) \
    (lvalue ^= ( ( (lvalue >> (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex))) ^ fieldvalue ) & \
                ( ( 1 << regname ## _s_ ## fieldname ) - 1 ) ) \
               << (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex)))
    
/** Get a named field of 1-32 bits within a 32-bit field array value. */
#define FM_GET_FIELD_ARRAY(rvalue, regname, fieldname, fieldindex) \
    ( (rvalue >> (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex))) &  \
     ( ( 1 << regname ## _s_ ## fieldname ) - 1 ) )

/** Set a named field of 1-64 bits within a 64-bit field array value. */
#define FM_SET_FIELD64_ARRAY(lvalue, regname, fieldname, fieldindex, fieldvalue) \
    (lvalue ^= ( ( (lvalue >> (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex))) ^ fieldvalue ) & \
                ( ( FM_LITERAL_U64(1) << regname ## _s_ ## fieldname ) - \
                 FM_LITERAL_U64(1) ) ) << (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex)))

/** Get a named field of 1-64 bits within a 64-bit field array value. */
#define FM_GET_FIELD64_ARRAY(rvalue, regname, fieldname, fieldindex) \
    ( (rvalue >> (regname ## _l_ ## fieldname + (regname ## _s_ ## fieldname * fieldindex))) & \
     ( ( FM_LITERAL_U64(1) << regname ## _s_ ## fieldname ) - \
      FM_LITERAL_U64(1) ) )

/** Extract a field of 32 or fewer bits from an unnamed 32-bit value. */
#define FM_GET_UNNAMED_FIELD(lvalue, start, len) \
    ((lvalue >> (start)) & ((1 << (len)) - 1))

/** Extract a field of 64 or fewer bits from an unnamed 64-bit value. */
#define FM_GET_UNNAMED_FIELD64(lvalue, start, len) \
    ((lvalue >> (start)) & ((FM_LITERAL_U64(1) << (len)) - FM_LITERAL_U64(1))) 

/** Set a field of 32 or fewer bits for an unnamed 32-bit value. */
#define FM_SET_UNNAMED_FIELD(lvalue, start, len, value) \
    lvalue &= ~(((1 << (len)) - 1) << (start)); \
    lvalue |= ((value) & ((1 << (len)) - 1)) << (start); 

/** Set a field of 64 or fewer bits for an unnamed 64-bit value. */
#define FM_SET_UNNAMED_FIELD64(lvalue, start, len, value) \
    lvalue &= ~(((FM_LITERAL_U64(1) << (len)) - FM_LITERAL_U64(1)) << (start)); \
    lvalue |= ((value) & ((FM_LITERAL_U64(1) << (len)) - FM_LITERAL_U64(1))) << (start); 

/** Extract an unnamed field of 32 or fewer bits from a >64-bit value. */
#define FM_ARRAY_GET_UNNAMED_FIELD(array, start, len) \
    fmMultiWordBitfieldGet32((array), (start) + (len) - 1, (start))

/** Set an unnamed field of 32 or fewer bits within a >64-bit value. */
#define FM_ARRAY_SET_UNNAMED_FIELD(array, start, len, value) \
    fmMultiWordBitfieldSet32((array), (start) + (len) - 1, (start), (value))

/** Extract an unnamed bit from a >64-bit value. */
#define FM_ARRAY_GET_UNNAMED_BIT(array, bit) \
    fmMultiWordBitfieldGet32((array), (bit), (bit))

/** Set an unnamed field of 32 or fewer bits within a >64-bit value. */
#define FM_ARRAY_SET_UNNAMED_BIT(array, bit, value) \
    fmMultiWordBitfieldSet32((array), (bit), (bit), (value))

/** Extract a bit from an array of width-bit words. */
#define FM_UNNAMED_ARRAY_GET_BIT(lvalue, bit, width) \
    ((lvalue)[(bit)/(width)] >> ((bit) % (width)))

/** Set a bit in an array of width-bit words. */
#define FM_UNNAMED_ARRAY_SET_BIT(lvalue, bit, width, value) \
    ((lvalue)[(bit)/(width)]) &= ~(1 << ((bit) % (width)));  \
    ((lvalue)[(bit)/(width)]) |=  (((value & 1)) << ((bit) % (width)));


/** @} (end of Doxygen group) */


/* Prototypes for functions used by the FM_ARRAY_* macros. */
fm_uint32 fmMultiWordBitfieldGet32(const fm_uint32 *array,
                                   fm_int           hiBit,
                                   fm_int           loBit);
void fmMultiWordBitfieldSet32(fm_uint32 *array,
                              fm_int     hiBit,
                              fm_int     loBit,
                              fm_uint32  value);
fm_uint64 fmMultiWordBitfieldGet64(const fm_uint32 *array,
                                   fm_int           hiBit,
                                   fm_int           loBit);
void fmMultiWordBitfieldSet64(fm_uint32 *array,
                              fm_int     hiBit,
                              fm_int     loBit,
                              fm_uint64  value);
void fmMultiWordBitfieldGetPortmask(fm_uint32 *array,
                                    fm_int     hiBit,
                                    fm_int     loBit,
                                    void *     portmaskVoidPtr);
void fmMultiWordBitfieldSetPortmask(fm_uint32 *array,
                                    fm_int     hiBit,
                                    fm_int     loBit,
                                    void *     portmaskVoidPtr);
fm_uint32 fmMulti64BitWordBitfieldGet32(const fm_uint64 *array,
                                        fm_int           hiBit,
                                        fm_int           loBit);
void fmMulti64BitWordBitfieldSet32(fm_uint64 *array,
                                   fm_int     hiBit,
                                   fm_int     loBit,
                                   fm_uint32  value);
fm_uint64 fmMulti64BitWordBitfieldGet64(const fm_uint64 *array,
                                        fm_int           hiBit,
                                        fm_int           loBit);
void fmMulti64BitWordBitfieldSet64(fm_uint64 *array,
                                   fm_int     hiBit,
                                   fm_int     loBit,
                                   fm_uint64  value);


/* Prototypes for sign-extension functions. */
fm_int fmSignExtendInt(fm_int value, fm_uint signifigantBits);
fm_int32 fmSignExtendInt32(fm_int32 value, fm_uint signifigantBits);
fm_int64 fmSignExtendInt64(fm_int64 value, fm_uint signifigantBits);

/* Key/KeyInvert Generation on FM6000 switch family */
void fmGenerateCAMKey(fm_uint32 *value,
                      fm_uint32 *mask,
                      fm_uint32 *key,
                      fm_uint32 *keyInvert,
                      fm_int     size);

void fmGenerateCAMValueMask(fm_uint32 *key,
                            fm_uint32 *keyInvert,
                            fm_uint32 *value,
                            fm_uint32 *mask,
                            fm_int     size);

fm_bool IsCamKeyValid(fm_uint32 *key,
                      fm_uint32 *keyInvert,
                      fm_int     size);

/* Key/KeyInvert Generation on FM10000 switch family */

void fmGenerateCAMKey2(fm_uint32 *value,
                       fm_uint32 *mask,
                       fm_uint32 *key,
                       fm_uint32 *keyInvert,
                       fm_int     size);

void fmGenerateCAMValueMask2(fm_uint32 *key,
                             fm_uint32 *keyInvert,
                             fm_uint32 *value,
                             fm_uint32 *mask,
                             fm_int     size);

fm_bool IsCamKeyValid2(fm_uint32 *key,
                       fm_uint32 *keyInvert,
                       fm_int     size);


#endif /* __FM_FM_BITFIELD_H */
