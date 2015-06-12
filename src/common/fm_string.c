/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_string.c
 * Creation Date:  March 26, 2008
 * Description:    String manipulation functions missing from standard C.
 *
 * Copyright (c) 2008 - 2012, Intel Corporation
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
/** fmStringCopy
 * \ingroup intString
 *
 * \desc            Copy a string without overflowing the destination buffer.
 *
 * \note            This is similar to the non-standard C library
 *                  function strlcpy.
 *
 * \param[out]      dst is the destination buffer.  Unlike strncpy,
 *                  dst will always be nul-terminated.
 *
 * \param[in]       src is the string to be copied.
 *
 * \param[in]       dstSize is the total size of the destination buffer.
 *
 * \return          None
 *
 *****************************************************************************/
void fmStringCopy(char *dst, const char *src, fm_uint dstSize)
{
    fm_uint i;

    for (i = 0 ; i + 1 < dstSize && src[i] != 0 ; i++)
    {
        dst[i] = src[i];
    }

    dst[i] = 0;

}   /* end fmStringCopy */


/*****************************************************************************/
/** fmStringAppend
 * \ingroup intString
 *
 * \desc            Append one string to another, without overflowing the
 *                  destination buffer.
 *
 * \note            This is similar to the non-standard C library
 *                  function strlcat.
 *
 * \param[in,out]   dst is the string that src will be appended to.
 *
 * \param[in]       src is the string to be appended to dst.
 *
 * \param[in]       dstSize is the total size of the destination buffer.
 *
 * \return          None
 *
 *****************************************************************************/
void fmStringAppend(char *dst, const char *src, fm_uint dstSize)
{
    fm_uint i;
    fm_uint j;

    for (i = 0 ; i + 1 < dstSize && dst[i] != 0 ; i++)
    {
    }

    for (j = 0 ; i + 1 < dstSize && src[j] != 0 ; i++, j++)
    {
        dst[i] = src[j];
    }

    dst[i] = 0;

}   /* end fmStringAppend */


/*****************************************************************************/
/** fmStringDuplicate
 * \ingroup intString
 *
 * \desc            Allocate memory for a new string and copy an existing
 *                  string into it.
 *
 * \note            This is similar to the POSIX function strdup, but uses
 *                  fmAlloc to allocate memory.
 *
 * \param[in]       src is the string to be copied.
 *
 * \return          copy of src if successful
 * \return          NULL if out of memory
 *
 *****************************************************************************/
char *fmStringDuplicate(const char *src)
{
    fm_uint size;
    char *result;

    size = strlen(src);
    result = fmAlloc(size + 1);
    if (result != NULL)
    {
        FM_MEMCPY_S(result, size, src, size);

        /* Explicitly null-terminate the string (for Klocwork). */
        result[size] = 0;
    }

    return result;

}   /* end fmStringDuplicate */
