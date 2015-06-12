/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_graycode.c
 * Creation Date:   October 03, 2011
 * Description:     File containing helper functions related to the encoding or 
 *                  decoding of numbers using Gray Code (a.k.a. "reflected
 *                  binary code")
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

#include <fm_sdk_int.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmConvertBinaryToGray 
 * 
 * \ingroup intGrayCode
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            This function takes a raw binary number and 
 *                  converts it into Gray Code
 *
 * \param[in]       binary is the binary-encoded number to convert
 * \param[in]       nBits is the number of bits to be converted
 *
 * \return          The Gray-encoded number
 *
 *****************************************************************************/
fm_uint64 fmConvertBinaryToGray( fm_uint64 binary, fm_int nBits )
{

    fm_uint64 gray = 0;
    fm_int    i;

    /* scan all bits */
    for (i = 1 ; i < nBits  ; i++)
    {
        /* if the i-th bit is '1' the (i-1)-the bit is inverted */
        if ( (binary & (FM_LITERAL_U64(1) << i)) != 0 )
        {
            /* inverted */
            gray += ((binary & (FM_LITERAL_U64(1) << (i-1))) ^
                    (FM_LITERAL_U64(1) << (i-1))); 
        }
        else
        {
            /* not inverted */
            gray += (binary & (FM_LITERAL_U64(1) << (i-1)));
        }
    }

    /* Most significant bit is always the same */
    gray += (binary & (FM_LITERAL_U64(1) << (nBits-1)));

    return gray;

} /* end fmConvertBinaryToGray */


/*****************************************************************************/
/** fmConvertGrayToBinary
 * 
 * \ingroup intGrayCode
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            This function takes a Gray Code number and 
 *                  converts it into Binary
 *
 * \param[in]       gray is the gray-encoded number to be converted
 * \param[in]       nBits is the number of bits to be converted
 *
 * \return          The Binary-encoded number
 *
 *****************************************************************************/
fm_uint64 fmConvertGrayToBinary( fm_uint64 gray, fm_int nBits )
{

    fm_uint64 binary = 0;
    fm_int    i;

    /* Most significant bit is always the same */
    binary += (gray & (FM_LITERAL_U64(1) << (nBits-1)));

    for (i = nBits - 1 ; i > 0  ; i--)
    {
        /* if the i-th bit is '1' the (i-1)-the bit is inverted */
        if ( (binary & (FM_LITERAL_U64(1) << i)) != 0 )
        {
            /* inverted */
            binary += ((gray & (FM_LITERAL_U64(1) << (i-1))) ^
                    (FM_LITERAL_U64(1) << (i-1))); 
        }
        else
        {
            /* not inverted */
            binary += (gray & (FM_LITERAL_U64(1) << (i-1)));
        }
    }

    return binary;

} /* end fmConvertGrayToBinary */



 
