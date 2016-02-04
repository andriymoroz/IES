/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_bitarray.c
 * Creation Date:   Nov 10, 2006
 * Description:     Contains functions that implement bit array support
 *
 * Copyright (c) 2006 - 2012, Intel Corporation
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
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * GetBitPosition
 *
 * Description: given a bit number, returns the word pointer and bit mask
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              bitNumber               bit number (0 to bitCount-1)
 *
 *              pWord                   pointer to the word within the array
 *
 *              bitMask                 pointer, filled with bit mask
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
static fm_status GetBitPosition(fm_bitArray *bitArray,
                                fm_int       bitNumber,
                                fm_uint **   ppWord,
                                fm_uint *    bitMask)
{
    fm_int   offset;
    fm_uint *pWord;

    if ( (bitArray == NULL) || (bitNumber < 0) ||
        (bitNumber >= bitArray->bitCount) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    offset   = bitNumber / bitArray->bitsPerWord;
    pWord    = bitArray->bitArrayData + offset;
    *ppWord  = pWord;
    *bitMask = 1 << (bitNumber % bitArray->bitsPerWord);

    return FM_OK;

}   /* end GetBitPosition */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCreateBitArray
 * \ingroup diagBitArray
 *
 * \desc            Initializes the members of a ''fm_bitArray'' structure,
 *                  including allocation of memory for holding the actual
 *                  bit array data. The bit array will be initialized to all 
 *                  zeros.
 *
 * \note            If this function returns FM_OK, ''fmDeleteBitArray'' must 
 *                  be called to deallocate the memory that is allocated by
 *                  this function when the ''fm_bitArray'' object is no
 *                  longer needed.
 *
 * \param[in,out]   bitArray points to a caller-supplied structure of
 *                  type ''fm_bitArray'' to be initialized by this function.
 *
 * \param[in]       bitcount is the number of bits to be held by the array.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory available for holding the bit
 *                  array.
 *
 *****************************************************************************/
fm_status fmCreateBitArray(fm_bitArray *bitArray, fm_int bitcount)
{
    fm_int    bitsPerWord;
    fm_int    wordCount;
    fm_int    arraySize;
    fm_status result;

    bitsPerWord            = sizeof(fm_uint) * 8;
    wordCount              = (bitcount + bitsPerWord - 1) / bitsPerWord;
    arraySize              = wordCount * sizeof(fm_uint);
    bitArray->bitArrayData = (fm_uint *) fmAlloc(arraySize);

    if (bitArray->bitArrayData != NULL)
    {
        memset(bitArray->bitArrayData, 0, arraySize);
        bitArray->bitsPerWord     = bitsPerWord;
        bitArray->wordCount       = wordCount;
        bitArray->bitCount        = bitcount;
        bitArray->nonZeroBitCount = 0;
        result                    = FM_OK;
    }
    else
    {
        result = FM_ERR_NO_MEM;
    }

    return result;

}   /* end fmCreateBitArray */




/*****************************************************************************
 * fmResizeBitArray
 *
 * Description: Resizes and re-initializes an existing bit array
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              newbitcount             new number of bits in the array
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmResizeBitArray(fm_bitArray *bitArray, fm_int newbitcount)
{
    fm_status result;

    result = fmDeleteBitArray(bitArray);

    if (result != FM_OK)
    {
        return result;
    }

    result = fmCreateBitArray(bitArray, newbitcount);

    return result;

}   /* end fmResizeBitArray */




/*****************************************************************************/
/** fmDeleteBitArray
 * \ingroup diagBitArray
 *
 * \desc            Destroys a ''fm_bitArray'' object, including deallocating
 *                  the memory for holding the actual bit array data.
 *
 * \note            All ''fm_bitArray'' objects successfully initialized by
 *                  ''fmCreateBitArray'' must be destroyed by this function
 *                  when the ''fm_bitArray'' object is no longer needed.
 *
 * \param[in,out]   bitArray points to the ''fm_bitArray'' object previously 
 *                  initialized with a call to ''fmCreateBitArray''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL.
 *
 *****************************************************************************/
fm_status fmDeleteBitArray(fm_bitArray *bitArray)
{
    if (bitArray == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (bitArray->bitArrayData != NULL)
    {
        fmFree(bitArray->bitArrayData);
        bitArray->bitArrayData    = NULL;
        bitArray->bitsPerWord     = 0;
        bitArray->wordCount       = 0;
        bitArray->bitCount        = 0;
        bitArray->nonZeroBitCount = 0;
    }

    return FM_OK;

}   /* end fmDeleteBitArray */




/*****************************************************************************/
/** fmSetBitArrayBit
 * \ingroup diagBitArray
 *
 * \desc            Sets the value of a single bit in a bit array.
 *
 * \note            The bit array must have been previously initialized with 
 *                  a call to ''fmCreateBitArray''.
 *
 * \param[in,out]   bitArray points to the ''fm_bitArray'' object.
 *
 * \param[in]       bitNumber is the zero-based bit number of the bit whose
 *                  value is to be set.
 *
 * \param[in]       bitValue is the value to which the bit should be set 
 *                  (0 or 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL or
 *                  bitNumber is out of range for this bit array.
 *
 *****************************************************************************/
fm_status fmSetBitArrayBit(fm_bitArray *bitArray,
                           fm_int       bitNumber,
                           fm_bool      bitValue)
{
    fm_status status;
    fm_uint   bitMask;
    fm_uint * pWord;

    status = GetBitPosition(bitArray, bitNumber, &pWord, &bitMask);

    if (status == FM_OK)
    {
        if (bitValue)
        {
            if ( !(*pWord & bitMask) )
            {
                bitArray->nonZeroBitCount++;
            }

            *pWord |= bitMask;
        }
        else
        {
            if (*pWord & bitMask)
            {
                bitArray->nonZeroBitCount--;

                if (bitArray->nonZeroBitCount < 0)
                {
                    FM_LOG_WARNING(FM_LOG_CAT_GENERAL,
                                   "fmFindBitInBitArray: nonZeroBitCount "
                                   "less than zero!\n");
                }
            }

            *pWord &= ~bitMask;
        }
    }

    return status;

}   /* end fmSetBitArrayBit */




/*****************************************************************************/
/** fmSetBitArrayBlock
 * \ingroup diagBitArray
 *
 * \desc            Sets a block of bits in a bit array to the same value.
 *
 * \note            The bit array must have been previously initialized with 
 *                  a call to ''fmCreateBitArray''.
 *
 * \param[in,out]   bitArray points to the ''fm_bitArray'' object.
 *
 * \param[in]       startBitNumber is the zero-based number of the first bit 
 *                  in the block to be set.
 *
 * \param[in]       numBits is the number of bits in the block to be set.
 *
 * \param[in]       bitValue is the value to which the bits in the block should 
 *                  be set (0 or 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL or
 *                  any of the bits in the range specified by startBitNumber
 *                  and numBits is out of range for this bit array.
 *
 *****************************************************************************/
fm_status fmSetBitArrayBlock(fm_bitArray *bitArray,
                             fm_int       startBitNumber,
                             fm_int       numBits,
                             fm_bool      bitValue)
{
    fm_status status = FM_OK;
    fm_int    i;
    fm_int    j;

    /* This could be optimized by setting whole words at a time,
     * watching out for the boundary words. */

    for (j = 0, i = startBitNumber ; j < numBits ; i++, j++)
    {
        status = fmSetBitArrayBit(bitArray, i, bitValue);

        if (status != FM_OK)
        {
            return status;
        }
    }

    return status;

}   /* end fmSetBitArrayBlock */




/*****************************************************************************/
/** fmGetBitArrayBit
 * \ingroup diagBitArray
 *
 * \desc            Gets the value of a single bit from a bit array.
 *
 * \note            The bit array must have been previously initialized with 
 *                  a call to ''fmCreateBitArray''.
 *
 * \param[in]       bitArray points to the ''fm_bitArray'' object.
 *
 * \param[in]       bitNumber is the zero-based bit number of the bit whose
 *                  value is to be retrieved.
 *
 * \param[out]      bitValue points to caller-supplied storage where this
 *                  function is to place the value of the specified bit.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL or
 *                  bitNumber is out of range for this bit array.
 *
 *****************************************************************************/
fm_status fmGetBitArrayBit(fm_bitArray *bitArray,
                           fm_int       bitNumber,
                           fm_bool *    bitValue)
{
    fm_status status;
    fm_uint   bitMask;
    fm_uint * pWord;

    status = GetBitPosition(bitArray, bitNumber, &pWord, &bitMask);

    if (status == FM_OK)
    {
        if (*pWord & bitMask)
        {
            *bitValue = TRUE;
        }
        else
        {
            *bitValue = FALSE;
        }
    }

    return status;

}   /* end fmGetBitArrayBit */




/*****************************************************************************/
/** fmClearBitArray
 * \ingroup diagBitArray
 *
 * \desc            Clears all bits in a bit array.
 *
 * \note            The bit array must have been previously initialized with 
 *                  a call to ''fmCreateBitArray''.
 *
 * \param[in,out]   bitArray points to the ''fm_bitArray'' object.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL or
 *                  the structure is invalid.
 *
 *****************************************************************************/
fm_status fmClearBitArray(fm_bitArray *bitArray)
{
    fm_int   word;
    fm_uint *pWord;

    if ( (bitArray == NULL) || (bitArray->wordCount == 0) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    pWord = bitArray->bitArrayData;

    for (word = 0 ; word < bitArray->wordCount ; word++, pWord++)
    {
        *pWord = 0;
    }

    bitArray->nonZeroBitCount = 0;

    return FM_OK;

}   /* end fmClearBitArray */




/*****************************************************************************
 * fmFindBitInBitArray
 *
 * Description: Searches a bit array, starting with a specified bit,
 *              looking for the next bit that has the specified value.
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              firstBitNumber          starting bit number (0 to bitCount-1)
 *
 *              bitValue                specified bit value to find
 *                                      (TRUE/FALSE)
 *
 *              foundBit                pointer for returned bit position,
 *                                      will be 0 to bitCount-1 if found,
 *                                      -1 if not found
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmFindBitInBitArray(fm_bitArray *bitArray,
                              fm_int       firstBitNumber,
                              fm_bool      bitValue,
                              fm_int *     foundBit)
{
    fm_uint *pWord;
    fm_int   offset;
    fm_uint  bitMask;
    fm_uint  curWord;
    fm_int   curBit;
    fm_uint  skipPattern;

    if ( (bitArray == NULL) || (firstBitNumber < 0)
        || (firstBitNumber > bitArray->bitCount) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /**************************************************
     * Treat firstBitNumber == bitArray->bitCount as
     * a "done" condition rather than an invalid arg-
     * ument since we can expect users of this function
     * to call with firstBitNumber = *foundBit + 1
     * without making them check to see if
     * firstBitNumber has gone beyond the end of the
     * bit array.
     **************************************************/

    if (firstBitNumber == bitArray->bitCount)
    {
        *foundBit = -1;
        return FM_OK;
    }

    curBit  = firstBitNumber;
    offset  = curBit / bitArray->bitsPerWord;
    bitMask = 1 << (curBit % bitArray->bitsPerWord);

    if (bitValue)
    {
        skipPattern = 0;
    }
    else
    {
        skipPattern = ~0;
    }

    while (curBit < bitArray->bitCount)
    {
        pWord   = bitArray->bitArrayData + offset;
        curWord = *pWord;

        if (curWord == skipPattern)
        {
            /* all bits in this word are not what we are looking for.
             *  Jump to the starting bit of the next word. */
            offset++;
            bitMask = 1;
            curBit  = offset * bitArray->bitsPerWord;
        }
        else
        {
            /* at least one bit in this word is what we are looking for.
             *  Scan the word from the starting bit position. */
            while ( (curBit < bitArray->bitCount) && (bitMask != 0) )
            {
                if ( ( bitValue && (curWord & bitMask) ) ||
                    ( !bitValue && !(curWord & bitMask) ) )
                {
                    *foundBit = curBit;
                    return FM_OK;
                }

                bitMask <<= 1;
                curBit++;
            }

            offset++;
            bitMask = 1;
        }
    }

    /* no more bits found */
    *foundBit = -1;
    return FM_OK;

}   /* end fmFindBitInBitArray */




/*****************************************************************************
 * fmFindLastBitInBitArray
 *
 * Description: Searches a bit array in the reverse direction, starting with a
 *              specified bit, looking backward for the next bit that has the
 *              specified value.
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              firstBitNumber          starting bit number (0 to bitCount-1)
 *
 *              bitValue                specified bit value to find
 *                                      (TRUE/FALSE)
 *
 *              foundBit                pointer for returned bit position,
 *                                      will be 0 to bitCount-1 if found,
 *                                      -1 if not found
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmFindLastBitInBitArray(fm_bitArray *bitArray,
                                  fm_int       firstBitNumber,
                                  fm_bool      bitValue,
                                  fm_int *     foundBit)
{
    fm_uint *pWord;
    fm_int   offset;
    fm_uint  bitMask;
    fm_uint  curWord;
    fm_int   curBit;
    fm_uint  skipPattern;

    /**************************************************
     * Treat firstBitNumber == -1 as
     * a "done" condition rather than an invalid arg-
     * ument since we can expect users of this function
     * to call with firstBitNumber = *foundBit - 1
     * without making them check to see if
     * firstBitNumber has gone beyond the front end of
     * the bit array.
     **************************************************/

    if ( (bitArray == NULL) || (firstBitNumber < -1)
        || (firstBitNumber > bitArray->bitCount) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (firstBitNumber == -1)
    {
        *foundBit = -1;
        return FM_OK;
    }

    curBit  = firstBitNumber;
    offset  = curBit / bitArray->bitsPerWord;
    bitMask = 1 << (curBit % bitArray->bitsPerWord);

    if (bitValue)
    {
        skipPattern = 0;
    }
    else
    {
        skipPattern = ~0;
    }

    while (curBit >= 0)
    {
        pWord   = bitArray->bitArrayData + offset;
        curWord = *pWord;

        if (curWord == skipPattern)
        {
            /* all bits in this word are not what we are looking for.
             *  Jump to the last bit of the previous word. */
            offset--;
            bitMask = 1 << (bitArray->bitsPerWord - 1);
            curBit  = ( (offset + 1) * bitArray->bitsPerWord ) - 1;
        }
        else
        {
            /* at least one bit in this word is what we are looking for.
             *  Scan the word from the starting bit position. */
            while ( (curBit >= 0) && (bitMask != 0) )
            {
                if ( ( bitValue && (curWord & bitMask) ) ||
                    ( !bitValue && !(curWord & bitMask) ) )
                {
                    *foundBit = curBit;
                    return FM_OK;
                }

                bitMask >>= 1;
                curBit--;
            }

            offset--;
            bitMask = 1 << (bitArray->bitsPerWord - 1);
        }
    }

    /* no more bits found */
    *foundBit = -1;
    return FM_OK;

}   /* end fmFindLastBitInBitArray */




/*****************************************************************************
 * fmFindBitBlockInBitArray
 *
 * Description: Searches a bit array in the forward direction, starting with
 *              a specified bit, looking for a block of bits that have the
 *              specified value.
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              firstBitNumber          starting bit number (0 to bitCount-1)
 *
 *              blockSize               Number of bits in block to be found.
 *
 *              bitValue                specified bit value to find
 *                                      (TRUE/FALSE)
 *
 *              foundBit                pointer for returned bit position,
 *                                      will be 0 to bitCount-1 if found,
 *                                      -1 if not found
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmFindBitBlockInBitArray(fm_bitArray *bitArray,
                                   fm_int       firstBitNumber,
                                   fm_int       blockSize,
                                   fm_bool      bitValue,
                                   fm_int *     foundBit)
{
    fm_uint   bitCount;
    fm_int    nextBitNumber;
    fm_int    workingFoundBit;
    fm_status status = FM_OK;

    if ( (bitArray == NULL)
        || (firstBitNumber < 0)
        || (firstBitNumber > bitArray->bitCount)
        || (blockSize < 1)
        || (blockSize > bitArray->bitCount) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /**************************************************
     * Look for the first bit of the requested value.
     **************************************************/

    status = fmFindBitInBitArray(bitArray,
                                 firstBitNumber,
                                 bitValue,
                                 foundBit);

    if ( (status != FM_OK) || (*foundBit == -1) )
    {
        /* Either error or no such bit. */
        *foundBit = -1;
    }
    else
    {
        /**************************************************
         * Check subsequent bits, looking for a contiguous
         * block of the right value.
         **************************************************/

        nextBitNumber = *foundBit + 1;
        bitCount      = blockSize - 1;

        while (bitCount > 0)
        {
            /* Get next bit of the required value. */
            status = fmFindBitInBitArray(bitArray,
                                         nextBitNumber,
                                         bitValue,
                                         &workingFoundBit);

            if ( (status != FM_OK) || (*foundBit == -1) )
            {
                /* An error occurred or no block big enough. */
                *foundBit = -1;
                break;
            }
            else if (workingFoundBit != nextBitNumber)
            {
                /* Block was not the required size. Restart search. */
                *foundBit     = workingFoundBit;
                nextBitNumber = workingFoundBit + 1;
                bitCount      = blockSize - 1;
            }
            else
            {
                /* The current block is still a candidate. */
                nextBitNumber = workingFoundBit + 1;
                --bitCount;
            }

        }   /* end while (bitCount > 0) */

    }       /* end if ( (status != FM_OK) || (*foundBit == -1) ) */

    return status;

}   /* end fmFindBitBlockInBitArray */




/*****************************************************************************
 * fmFindLastBitBlockInBitArray
 *
 * Description: Searches a bit array in the reverse direction, starting with
 *              a specified bit, looking backward for a block of bits that
 *              have the specified value.
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              firstBitNumber          starting bit number (0 to bitCount-1)
 *
 *              blockSize               Number of bits in block to be found.
 *
 *              bitValue                specified bit value to find
 *                                      (TRUE/FALSE)
 *
 *              foundBit                pointer for returned bit position,
 *                                      will be 0 to bitCount-1 if found,
 *                                      -1 if not found
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmFindLastBitBlockInBitArray(fm_bitArray *bitArray,
                                       fm_int       firstBitNumber,
                                       fm_int       blockSize,
                                       fm_bool      bitValue,
                                       fm_int *     foundBit)
{
    fm_uint   bitCount;
    fm_int    nextBitNumber;
    fm_int    workingFoundBit;
    fm_status status = FM_OK;

    if ( (bitArray == NULL)
        || (firstBitNumber < 0)
        || (firstBitNumber > bitArray->bitCount)
        || (blockSize < 1)
        || (blockSize > bitArray->bitCount) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /**************************************************
     * Look for the first bit of the requested value.
     **************************************************/

    status = fmFindLastBitInBitArray(bitArray,
                                     firstBitNumber,
                                     bitValue,
                                     foundBit);

    if ( (status != FM_OK) || (*foundBit == -1) )
    {
        /* Either error or no such bit. */
        *foundBit = -1;
    }
    else
    {
        /**************************************************
         * Check subsequent bits, looking for a contiguous
         * block of the right value.
         **************************************************/

        nextBitNumber = *foundBit - 1;
        bitCount      = blockSize - 1;

        while (bitCount > 0)
        {
            /* Get next bit of the required value. */
            status = fmFindLastBitInBitArray(bitArray,
                                             nextBitNumber,
                                             bitValue,
                                             &workingFoundBit);

            if ( (status != FM_OK) || (*foundBit == -1) )
            {
                /* An error occurred or no block big enough. */
                *foundBit = -1;
                break;
            }
            else if (workingFoundBit != nextBitNumber)
            {
                /* Block was not the required size. Restart search. */
                *foundBit     = workingFoundBit;
                nextBitNumber = workingFoundBit - 1;
                bitCount      = blockSize - 1;
            }
            else
            {
                /* The current block is still a candidate. */
                nextBitNumber = workingFoundBit - 1;
                --bitCount;
            }

        }   /* end while (bitCount > 0) */

    }       /* end if ( (status != FM_OK) || (*foundBit == -1) ) */

    return status;

}   /* end fmFindLastBitBlockInBitArray */




/*****************************************************************************
 * fmGetBitArrayNonZeroBitCount
 *
 * Description: Retrieves the number of non-zero bits in the bit array
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              bitCount                pointer for returned non-zero-bit count
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
fm_status fmGetBitArrayNonZeroBitCount(fm_bitArray *bitArray,
                                       fm_int *     bitCount)
{
    if ( (bitArray == NULL) || (bitArray->wordCount <= 0) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *bitCount = bitArray->nonZeroBitCount;

    return FM_OK;

}   /* end fmGetBitArrayNonZeroBitCount */




/*****************************************************************************
 * fmDbgDumpBitArray
 *
 * Description: Displays the contents of the bit array
 *
 * Arguments:   bitArray                pointer to the bit array
 *
 *              n                       the number of bits to display
 *
 * Returns:     Fulcrum API status code
 *
 *****************************************************************************/
void fmDbgDumpBitArray(fm_bitArray *bitArray, fm_int n)
{
    fm_int  index;
    fm_bool bv;

    FM_LOG_PRINT("\n          ");

    for (index = 0 ; index < 64 ; index++)
    {
        FM_LOG_PRINT("%X", index % 0x10);
    }

    FM_LOG_PRINT("\n");

    for (index = 0 ; index < n ; index++)
    {
        if ( (index % 64) == 0 )
        {
            FM_LOG_PRINT("%08x: ", index);
        }

        fmGetBitArrayBit(bitArray, index, &bv);

        FM_LOG_PRINT("%s", bv ? "x" : ".");

        if ( (index > 0) && ( (index % 64) == 63 ) )
        {
            FM_LOG_PRINT("\n");
        }
    }

    FM_LOG_PRINT("\n");

}   /* end fmDbgDumpBitArray */




/*****************************************************************************
 * fmCompareBitArrays
 *
 * Description: Comapres two bit arrays.
 *
 * Arguments:   array1                  pointer to the first bit array
 *
 *              array2                  pointer to the second bit array
 *
 * Returns:     TRUE if the bit arrays are the same length and all bits
 *              in both arrays are set identically.
 *              FALSE if the bit arrays are of differing lengths or any bit
 *              in the arrays are different.
 *
 *****************************************************************************/
fm_bool fmCompareBitArrays(fm_bitArray *array1, fm_bitArray *array2)
{
    fm_int    curBit;
    fm_int    nextBit1;
    fm_int    nextBit2;
    fm_status err1;
    fm_status err2;

    if (array1->bitCount != array2->bitCount)
    {
        return FALSE;
    }

    curBit = 0;

    while (curBit < array1->bitCount)
    {
        err1 = fmFindBitInBitArray(array1,
                                   curBit,
                                   TRUE,
                                   &nextBit1);

        err2 = fmFindBitInBitArray(array2,
                                   curBit,
                                   TRUE,
                                   &nextBit2);

        if (err1 != err2)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_GENERAL,
                         "fmCompareBitArrays:  curBit=%d, err1=%d, err2=%d\n",
                         curBit,
                         err1,
                         err2);
            return FALSE;
        }

        if (err1 != FM_OK)
        {
            return FALSE;
        }

        if (nextBit1 != nextBit2)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_GENERAL,
                         "fmCompareBitArrays: curBit=%d, nextBit1=%d, "
                         "nextBit2=%d\n",
                         curBit,
                         nextBit1,
                         nextBit2);
            return FALSE;
        }

        if (nextBit1 < 0)
        {
            break;
        }

        curBit = nextBit1 + 1;
    }

    return TRUE;

}   /* end fmCompareBitArrays */


/*****************************************************************************/
/** fmUnionBitArrays
 * \ingroup intBitArray
 *
 * \desc            Computes the union of two bit arrays, and stores the
 *                  result in a third bit array.
 *
 * \param[in]       src1 is a source bit array.
 *
 * \param[in]       src2 is a source bit array.
 *
 * \param[in,out]   dst is the destination bit array.  It must have been
 *                  already allocated with the correct number of bits,
 *                  but any existing data is overwritten.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if src1, src2, and dst are not
 *                  all the same size.
 *
 *****************************************************************************/
fm_status fmUnionBitArrays(const fm_bitArray *src1,
                           const fm_bitArray *src2,
                           fm_bitArray *      dst)
{
    fm_int i;
    fm_uint word;
    fm_int nonZeroBitCount = 0;

    if (src1->bitsPerWord != src2->bitsPerWord ||
        src2->bitsPerWord != dst->bitsPerWord)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (src1->bitCount != src2->bitCount ||
        src2->bitCount != dst->bitCount)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (src1->wordCount != src2->wordCount ||
        src2->wordCount != dst->wordCount)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    for (i = 0 ; i < src1->wordCount ; i++)
    {
        word = (src1->bitArrayData[i] | src2->bitArrayData[i]);
        dst->bitArrayData[i] = word;

        /* "Brian Kernighan Algorithm" for counting 1 bits */
        for ( ; word != 0 ; word &= word - 1)
        {
            nonZeroBitCount++;
        }
    }

    dst->nonZeroBitCount = nonZeroBitCount;

    return FM_OK;

}   /* end fmUnionBitArrays */



/*****************************************************************************/
/** fmIsBitInBitArray
 * \ingroup intBitArray
 *
 * \desc            Returns a bollean indicating if the specified bit is
 *                  set in the bit array.
 *
 * \note            Note that this routine does no error checking to make
 *                  sure the specified bit number is not out of range for 
 *                  the bit array, 
 *
 * \param[in]       bitArray is a pointer to the bit array.
 *
 * \param[in]       bitNumber is the bit number to test.
 *
 * \return          TRUE if the specified bit is set.
 * \return          FALSE if the specified bit is not set.
 *
 *****************************************************************************/
fm_bool fmIsBitInBitArray(fm_bitArray *bitArray, fm_int bitNumber)
{
    fm_bool   bitValue = FALSE;
    
    fmGetBitArrayBit(bitArray, bitNumber, &bitValue);
    return bitValue;
    
}   /* end fmIsBitInBitArray */





/*****************************************************************************/
/** fmAndBitArrays
 * \ingroup intBitArray
 *
 * \desc            For each bit in src1 and src2, computes the destination
 *                  bit as the logical AND of the result.
 *
 * \param[out]      src1 is a pointer to the first bit array. 
 *
 * \param[out]      src2 is a pointer to the second bit array. 
 *
 * \param[out]      dst is a pointer to the result. 
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAndBitArrays(fm_bitArray *src1, 
                         fm_bitArray *src2, 
                         fm_bitArray *dst)
{
    fm_int  i;
    fm_uint word;

    if (dst->wordCount != src1->wordCount)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    dst->nonZeroBitCount = 0;

    for ( i = 0 ; i < src1->wordCount ; i++ )
    {
        word = src1->bitArrayData[i] & src2->bitArrayData[i];
        dst->bitArrayData[i] = word;

        /* "Brian Kernighan Algorithm" for counting 1 bits */
        for ( ; word != 0 ; word &= word - 1)
        {
            dst->nonZeroBitCount++;
        }
    }

    return FM_OK;

} /* end fmAndBitArrays */



/*****************************************************************************/
/** fmCopyBitArray
 * \ingroup intBitArray
 *
 * \desc            Copies a bit array to another bit array of the same size.
 *
 * \param[in,out]   dest points to the destination bit array.
 *
 * \param[in]       src points to the source bit array.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the two arrays are not the
 *                  same size.
 *
 *****************************************************************************/
fm_status fmCopyBitArray(fm_bitArray* dest, const fm_bitArray* src)
{

    if (src->bitsPerWord != dest->bitsPerWord ||
        src->bitCount != dest->bitCount ||
        src->wordCount != dest->wordCount ||
        src->bitArrayData == NULL ||
        dest->bitArrayData == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_MEMCPY_S( dest->bitArrayData, 
                 dest->wordCount * sizeof(fm_uint),
                 src->bitArrayData, 
                 src->wordCount * sizeof(fm_uint) );

    dest->nonZeroBitCount = src->nonZeroBitCount;

    return FM_OK;

}   /* end fmCopyBitArray */




/*****************************************************************************/
/** fmGetBitArrayBitCount
 * \ingroup intBitArray
 *
 * \desc            Gets the value of a bit count from a bit array.
 *
 * \note            The bit array must have been previously initialized with
 *                  a call to ''fmCreateBitArray''.
 *
 * \param[in]       bitArray points to the ''fm_bitArray'' object.
 *
 * \param[out]      bitCount points to caller-allocated storage where this
 *                  function is to place the bit count value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if bitArray points to NULL.
 *
 *****************************************************************************/
fm_status fmGetBitArrayBitCount(fm_bitArray *bitArray,
                                fm_int      *bitCount)
{
    fm_status status;

    status = FM_OK;

    if (bitArray == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *bitCount = bitArray->bitCount;

    return status;

}   /* end fmGetBitArrayBitCount */
