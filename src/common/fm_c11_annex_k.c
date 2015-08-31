/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_c11_annex_k.c
 * Creation Date:  June 27, 2012
 * Description:    Functions that emulate some of the functions documented
 *                 in C-2011 Annex K specification.
 *                 These functions do NOT attempt to implement all of the
 *                 runtime-constraints specified by Annex K. Many of those
 *                 constraints would require complete implementation of
 *                 printf/scanf-style parsing.
 *
 * Copyright (c) 2012 - 2015, Intel Corporation
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
/** fmFprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to a file.
 *
 * \note            This emulates fprintf_s.
 *
 * \param[in]       stream points to the file. It must not be NULL.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_int fmFprintf_s(FILE *stream, const char *format, ...)
{
    va_list args;
    fm_int  retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for fprintf_s:
     * 1. Neither stream nor format shall be a null pointer.
     * 2. The %n specifier shall not appear in the string pointed to by format.
     * 3. Any argument to fprintf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 4. If there is a runtime-constraint violation, fprintf_s does not
     *    attempt to produce further output, and it is unspecified to what
     *    extent fprint_f produced output before discovering the runtime-
     *    contstraint violation.
     * This function relies on fmVfprintf_s to provide all run-time constraints.
    **************************************************************************/

    va_start(args, format);

    retval = fmVfprintf_s(stream, format, args);

    va_end(args);

    return retval;

}   /* end fmFprintf_s */




/*****************************************************************************/
/** fmPrintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to stdout.
 *
 * \note            This emulates printf_s.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_int fmPrintf_s(const char *format, ...)
{
    va_list args;
    fm_int  retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for printf_s:
     * 1. format shall not be a null pointer.
     * 2. The %n specifier shall not appear in the string pointed to by format.
     * 3. Any argument to printf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 4. If there is a runtime-constraint violation, printf_s does not
     *    attempt to produce further output, and it is unspecified to what
     *    extent print_f produced output before discovering the runtime-
     *    contstraint violation.
     * This function relies on fmVfprintf_s to provide all run-time constraints.
    **************************************************************************/

    va_start(args, format);

    retval = fmVfprintf_s(stdout, format, args);

    va_end(args);

    return retval;

}   /* end fmPrintf_s */




/*****************************************************************************/
/** fmSnprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to a string buffer.
 *
 * \note            This emulates snprintf_s.
 *
 * \param[in]       s points to the output string. It must not be NULL.
 * 
 * \param[in]       n is the size of the output buffer.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_int fmSnprintf_s(char *s, rsize_t n, const char *format, ...)
{
    va_list args;
    fm_int  retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for snprintf_s:
     * 1. Neither s nor format shall be a null pointer.
     * 2. 'n' shall neither equal zero nor be greater than RSIZE_MAX.
     * 3. The %n specifier shall not appear in the string pointed to by format.
     * 4. Any argument to printf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 5. If there is a runtime-constraint violation, then if s is not a NULL
     *    pointer and n is greater than zero and less than RSIZE_MAX, then the
     *    snprintf_s function sets s[0] to the null character.
     *
     * This function relies on fmVsnprintf_s to provide all run-time constraints.
    **************************************************************************/

    va_start(args, format);

    retval = fmVsnprintf_s(s, n, format, args);

    va_end(args);

    return retval;

}   /* end fmSnprintf_s */




/*****************************************************************************/
/** fmSprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to a string buffer.
 *
 * \note            This emulates sprintf_s.
 *
 * \param[in]       s points to the output string. It must not be NULL.
 * 
 * \param[in]       n is the size of the output buffer.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_int fmSprintf_s(char *s, rsize_t n, const char *format, ...)
{
    va_list args;
    fm_int  retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for sprintf_s:
     * 1. Neither s nor format shall be a null pointer.
     * 2. 'n' shall neither equal zero nor be greater than RSIZE_MAX.
     * 3. The %n specifier shall not appear in the string pointed to by format.
     * 4. Any argument to printf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 5. If there is a runtime-constraint violation, then if s is not a NULL
     *    pointer and n is greater than zero and less than RSIZE_MAX, then the
     *    snprintf_s function sets s[0] to the null character.
     *
     * This function relies on fmVsnprintf_s to provide all run-time constraints.
    **************************************************************************/

    va_start(args, format);

    retval = fmVsnprintf_s(s, n, format, args);

    va_end(args);

    return retval;

}   /* end fmSprintf_s */




/*****************************************************************************/
/** fmSscanf_s
 * \ingroup intCommon
 *
 * \desc            Parse a string into component parts.
 *
 * \note            This emulates sscanf_s.
 *
 * \param[in]       s points to the input string. It must not be NULL.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_int fmSscanf_s(const char *s, const char *format, ...)
{
    va_list args;
    fm_int  retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for sprintf_s:
     * 1. Neither s nor format shall be a null pointer.
     * 2. Any argument indirected through in order to store converted input
     *    shall not be a null pointer.
     * 3. If there is a runtime-constraint violation, the sscanf_s function
     *    does not attempt to perform further input, and it is unspecified to
     *    what extent sscanf_s performed input before discovering the
     *    runtime-constraint violation.
    **************************************************************************/

    if (s == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 's' argument to fmSscanf_s\n");
        return -EINVAL;
    }

    if (format == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 'format' argument to fmSscanf_s\n");
        return -EINVAL;
    }

    va_start(args, format);

    retval = vsscanf(s, format, args);

    va_end(args);

    return retval;

}   /* end fmSscanf_s */




/*****************************************************************************/
/** fmVfprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to a file.
 *
 * \note            This emulates fprintf_s.
 *
 * \param[in]       stream points to the file.
 *
 * \param[in]       format is the format-control string.
 *
 * \param[in]       arg contains to a va_list containing the parameters
 *                  needed for the format string.
 *
 * \return          The number of characters written, or a negative number
 *                  if an output error or runtime constraint violation
 *                  occurred.
 *
 *****************************************************************************/
fm_int fmVfprintf_s(FILE * stream, const char *format, va_list arg)
{
    fm_int retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for vfprintf_s:
     * 1. Neither stream nor format shall be a null pointer.
     * 2. The %n specifier shall not appear in the string pointed to by format.
     * 3. Any argument to vfprintf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 4. If there is a runtime-constraint violation, vfprintf_s does not
     *    attempt to produce further output, and it is unspecified to what
     *    extent vfprint_f produced output before discovering the runtime-
     *    contstraint violation.
     * For the purposes of emulating vfprintf_s, this function implements
     * item 1 only.
    **************************************************************************/

    if (stream == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 'stream' argument to fmVfprintf_s\n");
        return -EINVAL;
    }

    if (format == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 'format' argument to fmVfprintf_s\n");
        return -EINVAL;
    }

    retval = vfprintf(stream, format, arg);

    return retval;

}   /* end fmVfprintf_s */




/*****************************************************************************/
/** fmVprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and print output to stdout.
 *
 * \note            This emulates vprintf_s.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \param[in]       arg contains to a va_list containing the parameters
 *                  needed for the format string.
 *
 * \return          The number of characters written, or a negative number
 *                  if an output error or runtime constraint violation
 *                  occurred.
 *
 *****************************************************************************/
fm_int fmVprintf_s(const char *format, va_list arg)
{
    fm_int retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for vprintf_s:
     * 1. Format shall not be a null pointer.
     * 2. The %n specifier shall not appear in the string pointed to by format.
     * 3. Any argument to vprintf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 4. If there is a runtime-constraint violation, vprintf_s does not
     *    attempt to produce further output, and it is unspecified to what
     *    extent vprint_f produced output before discovering the runtime-
     *    contstraint violation.
     * This function relies upon fmVfprintf_s to perform runtime-constraint
     * validation.
    **************************************************************************/
    retval = fmVfprintf_s(stdout, format, arg);

    return retval;

}   /* end fmVprintf_s */




/*****************************************************************************/
/** fmVsnprintf_s
 * \ingroup intCommon
 *
 * \desc            Format and store output into a string buffer.
 *
 * \note            This emulates vsnprintf_s.
 *
 * \param[out]      s points to the output buffer. It must not be NULL.
 * 
 * \param[in]       n is the size of the output buffer.
 *
 * \param[in]       format is the format-control string. It must not be NULL.
 *
 * \param[in]       arg contains to a va_list containing the parameters
 *                  needed for the format string.
 *
 * \return          The number of characters that would have been written had
 *                  n been sufficiently large, not counting the terminating
 *                  null character, or a negative value if a runtime
 *                  constraint violation occurred.
 *
 *****************************************************************************/
fm_int fmVsnprintf_s(char *s, rsize_t n, const char *format, va_list arg)
{
    fm_int retval;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for vsnprintf_s:
     * 1. Neither s nor format shall be a null pointer.
     * 2. n shall neither equal zero nor be greater than RSIZE_MAX.
     * 3. The %n specifier shall not appear in the string pointed to by format.
     * 4. Any argument to vsnprintf_s corresponding to a %s specifier shall not
     *    be a NULL pointer.
     * 5. If there is a runtime-constraint violation, then if s is not a NULL
     *    pointer and n is greater than zero and less than RSIZE_MAX, then the
     *    snprintf_s function sets s[0] to the null character.
     * For the purposes of emulating vfprintf_s, this function implements
     * item 1 only.
    **************************************************************************/

    if (s == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 'stream' argument to fmVsnprintf_s\n");
        return -EINVAL;
    }

    if (format == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 'format' argument to fmVsnprintf_s\n");
        return -EINVAL;
    }

    if ( (n == 0) || (n > RSIZE_MAX) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "'n' value out of range in fmVsnprintf_s: %u\n", n);
        return -EINVAL;
    }

    retval = vsnprintf(s, n, format, arg);

    return retval;

}   /* end fmVsnprintf_s */




/*****************************************************************************/
/** fmMemcpy_s
 * \ingroup intCommon
 *
 * \desc            Copy memory.
 *
 * \note            This emulates memcpy_s.
 *
 * \param[out]      s1 points to the output buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input buffer. It must not be NULL.
 *
 * \param[in]       n is the number of bytes to copy.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmMemcpy_s(void *s1, rsize_t s1max, const void *s2, rsize_t n)
{
    char *      s1start;
    char *      s1end;
    const char *s2start;
    const char *s2end;
    fm_bool     validCopy;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for memcpy_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. Neither s1max nor n shall be greater than RSIZE_MAX.
     * 3. s1 and s2 buffers may not overlap.
     * 4. If there is a runtime-constraint violation, memcpy_s stores zeros
     *    in the first s1max bytes of the object pointed to by s1 if s1 is
     *    not a null pointer and s1max is not greater than RSIZE_MAX.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 's1' argument to FM_MEMCPY_S\n");
        return EINVAL;
    }

    if (s1max > RSIZE_MAX)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_MEMCPY_S: %d\n", s1max);
        return EINVAL;
    }

    validCopy = TRUE;

    if ( (s2 == NULL) || ( n > RSIZE_MAX ) || (n > s1max) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid argument to FM_MEMCPY_S\n");
        validCopy = FALSE;
    }

    /* Check for overlapping buffers */
    s1start = (char *) s1;
    s1end   = s1start + s1max - 1;
    s2start = (char *) s2;
    s2end   = s2start + n - 1;

    if ( ( (s2start >= s1start) && (s2start <= s1end) )
         || ( (s2end >= s1start) && (s2end <= s1end) )
         || ( (s1start >= s2start) && (s1start <= s2end) )
         || ( (s1end >= s2start) && (s1end <= s2end) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "Overlapping source/destination buffers in "
                      "FM_MEMCPY_S, dest=%p, source=%p\n",
                      (void *) s1,
                      (void *) s2 );
        FM_LOG_CALL_STACK(FM_LOG_CAT_GENERAL, FM_LOG_LEVEL_FATAL);
        validCopy = FALSE;
    }

    if (validCopy)
    {
        memcpy(s1, s2, n);
        return 0;
    }

    memset(s1, 0, s1max);
    return EINVAL;

}   /* end fmMemcpy_s */




/*****************************************************************************/
/** fmMemmove_s
 * \ingroup intCommon
 *
 * \desc            Copy memory between overlapping buffers.
 *
 * \note            This emulates memmove_s.
 *
 * \param[out]      s1 points to the output buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input buffer. It must not be NULL.
 *
 * \param[in]       n is the number of bytes to copy.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmMemmove_s(void *s1, rsize_t s1max, const void *s2, rsize_t n)
{
    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for memmove_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. Neither s1max nor n shall be greater than RSIZE_MAX.
     * 3. If there is a runtime-constraint violation, memmove_s stores zeros
     *    in the first s1max bytes of the object pointed to by s1 if s1 is
     *    not a null pointer and s1max is not greater than RSIZE_MAX.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 's1' argument to FM_MEMMOVE_S\n");
        return EINVAL;
    }

    if (s1max > RSIZE_MAX)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_MEMMOVE_S: %u\n", s1max);
        return EINVAL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null pointer for 's2' argument to FM_MEMMOVE_S\n");
        memset(s1, 0, s1max);
        return EINVAL;
    }

    if ( (n > RSIZE_MAX) || (n > s1max) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid n value in FM_MEMMOVE_S (n=%u s1max=%u)\n",
                     n,
                     s1max);
        memset(s1, 0, s1max);
        return EINVAL;
    }

    memmove(s1, s2, n);
    return 0;

}   /* end fmMemmove_s */




/*****************************************************************************/
/** fmMemset_s
 * \ingroup intCommon
 *
 * \desc            Set a memory buffer to a specified value.
 *
 * \note            This emulates memset_s.
 *
 * \param[out]      s points to the output buffer. It must not be NULL.
 *
 * \param[in]       smax is the size of s, in bytes.
 *
 * \param[in]       c is the character to be stored.
 *
 * \param[in]       n is the number of bytes to store.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmMemset_s(void *s, rsize_t smax, fm_int c, rsize_t n)
{
    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for memset_s:
     * 1. s shall not be a null pointer.
     * 2. Neither smax nor n shall be greater than RSIZE_MAX.
     * 3. n shall not be greater than smax.
     * 4. If there is a runtime-constraint violation, memset_s stores c
     *    in the first smax bytes of the object pointed to by s if s is
     *    not a null pointer and smax is not greater than RSIZE_MAX.
    **************************************************************************/

    if (s == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null 's' pointer in FM_MEMSET_S\n");
        return EINVAL;
    }

    if (smax > RSIZE_MAX)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid smax value in FM_MEMSET_S: %u\n", smax);
        return EINVAL;
    }

    if ( (n > RSIZE_MAX) || (n > smax) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid argument to FM_MEMSET_S\n");
        memset(s, c, smax);
        return EINVAL;
    }

    memset(s, c, n);
    return 0;

}   /* end fmMemset_s */




/*****************************************************************************/
/** fmStrcpy_s
 * \ingroup intCommon
 *
 * \desc            Copy a string.
 *
 * \note            This emulates strcpy_s.
 *
 * \param[out]      s1 points to the output string buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input string buffer. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmStrcpy_s(char *s1, rsize_t s1max, const char *s2)
{
    const char *s1end;
    const char *s2end;
    fm_uint     s2len;
    fm_bool     validCopy;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for strcpy_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. s1max shall not be greater than RSIZE_MAX.
     * 3. s1max shall not equal zero.
     * 4. s1 and s2 buffers may not overlap.
     * 5. If there is a runtime-constraint violation, then if s1 is not a null
     *    pointer and s1max is greater than zero and not greater than RSIZE_MAX,
     *    then strncpy_s sets s1[0] to the null character.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s1 pointer in FM_STRCPY_S\n");
        return EINVAL;
    }

    if (s1max > RSIZE_MAX)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_STRCPY_S: %u\n", s1max);
        return EINVAL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s2 pointer in FM_STRCPY_S\n");
        if (s1max > 0)
        {
            s1[0] = 0;
        }
        return EINVAL;
    }

    validCopy = TRUE;

    /* Check for overlapping buffers */
    s1end = s1 + s1max - 1;
    s2len = strlen(s2);
    s2end = s2 + s2len;

    if ( ( (s2 >= s1) && (s2 <= s1end) )
         || ( (s2end >= s1) && (s2end <= s1end) )
         || ( (s1 >= s2) && (s1 <= s2end) )
         || ( (s1end >= s2) && (s1end <= s2end) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "Overlapping buffers in FM_STRCPY_S, s1=%p, s2=%p\n",
                      (void *) s1,
                      (void *) s2 );
        validCopy = FALSE;
    }

    if ( (s2len + 1) > s1max )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "source buffer too long: s1max=%d, s2len=%d\n",
                      s1max,
                      s2len);
        validCopy = FALSE;
    }

    if (validCopy)
    {
#if defined(__KLOCWORK__)
        strcpy_s(s1, s1max, s2);
#else
        strcpy(s1, s2);
#endif
        return 0;
    }

    s1[0] = 0;
    return EINVAL;

}   /* end fmStrcpy_s */




/*****************************************************************************/
/** fmStrcat_s
 * \ingroup intCommon
 *
 * \desc            Concatenate string s2 onto the end of string s1.
 *
 * \note            This emulates strcat_s.
 *
 * \param[out]      s1 points to the output string buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input string buffer. It must not be NULL.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmStrcat_s(char *s1, rsize_t s1max, const char *s2)
{
    char *      s1end;
    const char *s2end;
    fm_bool     validCopy;
    rsize_t     m;
    rsize_t     s1len;
    rsize_t     s2len;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for strcat_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. s1max shall not be greater than RSIZE_MAX.
     * 3. s1max shall not equal zero.
     * 4. Let m denote the value s1max - strnlen_s(s1, s1max) upon entry to
     *    strncat_s.
     * 5. m shall not equal zero.
     * 6. If strlen(s2) shall not be greater than m.
     * 7. s1 and s2 buffers may not overlap.
     * 8. If there is a runtime-constraint violation, then if s1 is not a null
     *    pointer and s1max is greater than zero and not greater than RSIZE_MAX,
     *    then strcat_s sets s1[0] to the null character.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s1 pointer in FM_STRCAT_S\n");
        return EINVAL;
    }

    if ( (s1max == 0) || (s1max > RSIZE_MAX) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_STRCAT_S: %u\n", s1max);
        return EINVAL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s2 pointer in FM_STRCAT_S\n");
        s1[0] = 0;
        return EINVAL;
    }

    s1len = strnlen(s1, s1max);
    s2len = strlen(s2);
    m     = s1max - s1len;

    validCopy = TRUE;

    if (m == 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "'m' value is zero in FM_STRCAT_S\n");
        validCopy = FALSE;
    }
    else if (s2len >= m)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Destination string too big for available buffer "
                     "in FM_STRCAT_S, s2len=%u, m=%u\n",
                     s2len,
                     m);
        validCopy =  FALSE;
    }

    /* Check for overlapping buffers */
    s1end   = s1 + s1max - 1;
    s2end   = s2 + s2len;

    if ( ( (s2 >= s1) && (s2 <= s1end) )
         || ( (s2end >= s1) && (s2end <= s1end) )
         || ( (s1 >= s2) && (s1 <= s2end) )
         || ( (s1end >= s2) && (s1end <= s2end) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "Overlapping buffers in FM_STRCAT_S, s1=%p, s2=%p\n",
                      (void *) s1,
                      (void *) s2 );
        validCopy = FALSE;
    }

    if (validCopy)
    {
        s1[s1len + s2len] = 0;
        strcat(s1, s2);
        return 0;
    }

    s1[0] = 0;
    return EINVAL;

}   /* end fmStrcat_s */




/*****************************************************************************/
/** fmStrncpy_s
 * \ingroup intCommon
 *
 * \desc            Copy a string.
 *
 * \note            This emulates strncpy_s.
 *
 * \param[out]      s1 points to the output string buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input string buffer. It must not be NULL.
 *
 * \param[in]       n is the number of characters to copy.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmStrncpy_s(char *s1, rsize_t s1max, const char *s2, rsize_t n)
{
    const char *s1end;
    const char *s2end;
    rsize_t     s1len;
    rsize_t     s2len;
    fm_bool     validCopy;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for strncpy_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. Neither s1max nor n shall be greater than RSIZE_MAX.
     * 3. s1max shall not equal zero. 
     * 4. If n is not less than s1max, then s1max shall be greater than 
     *    strnlen_s(s2, s1max). 
     * 5. s1 and s2 buffers may not overlap.
     * 6. If there is a runtime-constraint violation, then if s1 is not a null
     *    pointer and s1max is greater than zero and not greater than RSIZE_MAX,
     *    then strncpy_s sets s1[0] to the null character.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s1 pointer in FM_STRNCPY_S\n");
        return EINVAL;
    }

    if (s1max > RSIZE_MAX || s1max == 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_STRNCPY_S: %u\n", s1max);
        return EINVAL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s2 pointer in FM_STRCAT_S\n");
        s1[0] = 0;
        return EINVAL;
    }

    validCopy = TRUE;

    if ( (n > RSIZE_MAX) || (n > s1max) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid n value in FM_STRNCPY_S: %u\n", n);
        validCopy = FALSE;
    }

    s2len = strnlen(s2, s1max);

    /* If n is not less than s1max, then s1max shall be greater than
     * strnlen_s(s2, s1max). */
    if ( (n >= s1max) && (s1max <= s2len) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid values in FM_STRNCPY_S: "
                     "s1max=%u, n=%u s2len=%u\n",
                     s1max,
                     n,
                     s2len);
        validCopy = FALSE;
    }

    /* Check for overlapping buffers */
    s1end   = s1 + s1max - 1;
    s2end   = s2 + n - 1;

    if ( ( (s2 >= s1) && (s2 <= s1end) )
         || ( (s2end >= s1) && (s2end <= s1end) )
         || ( (s1 >= s2) && (s1 <= s2end) )
         || ( (s1end >= s2) && (s1end <= s2end) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "Overlapping buffers in FM_STRNCPY_S, s1=%p, s2=%p\n",
                      (void *) s1,
                      (void *) s2 );
        validCopy = FALSE;
    }

    if (validCopy)
    {
        strncpy(s1, s2, n);
        s1len = (s2len < s1max) ? s2len : n;
        s1[s1len] = 0;
        return 0;
    }

    s1[0] = 0;
    return EINVAL;

}   /* end fmStrncpy_s */




/*****************************************************************************/
/** fmStrncat_s
 * \ingroup intCommon
 *
 * \desc            Concatenate string s2 onto the end of string s1.
 *
 * \note            This emulates strncat_s.
 *
 * \param[out]      s1 points to the output string buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input string buffer. It must not be NULL.
 *
 * \param[in]       n is the number of characters to copy.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmStrncat_s(char *s1, rsize_t s1max, const char *s2, rsize_t n)
{
    char *      s1end;
    const char *s2end;
    fm_bool     validCopy;
    rsize_t     m;
    rsize_t     s1len;
    rsize_t     s2len;

    /*************************************************************************
     * C11 Annex K specifies the following run-time constraints for strncat_s:
     * 1. Neither s1 nor s2 shall be a null pointer.
     * 2. Neither s1max nor n shall be greater than RSIZE_MAX.
     * 3. s1max shall not equal zero.
     * 4. Let m denote the value s1max - strnlen_s(s1, s1max) upon entry to
     *    strncat_s.
     * 5. m shall not equal zero.
     * 6. If n is not less than m, then m shall be greater than strnlen_s(s2, m).
     * 7. s1 and s2 buffers may not overlap.
     * 8. If there is a runtime-constraint violation, then if s1 is not a null
     *    pointer and s1max is greater than zero and not greater than RSIZE_MAX,
     *    then strncat_s sets s1[0] to the null character.
    **************************************************************************/

    if (s1 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s1 pointer in FM_STRNCAT_S\n");
        return EINVAL;
    }

    if ( (s1max == 0) || (s1max > RSIZE_MAX) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid s1max value in FM_STRNCAT_S: %u\n", s1max);
        return EINVAL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s2 pointer in FM_STRNCAT_S\n");
        s1[0] = 0;
        return EINVAL;
    }

    s1len = strnlen(s1, s1max);
    s2len = strnlen(s2, n);
    m     = s1max - s1len;

    validCopy = TRUE;

    if (m == 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "'m' value is zero in FM_STRNCAT_S\n");
        validCopy = FALSE;
    }
    else if (n > RSIZE_MAX)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid 'n' value in FM_STRNCAT_S: %u\n", n);
        validCopy = FALSE;
    }
    else if ( (n >= m) && (m <= s2len) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Incompatible 'n' and 'm' values in FM_STRNCAT_S"
                     ", n=%u, m=%u\n",
                     n,
                     m);
        validCopy =  FALSE;
    }

    /* Check for overlapping buffers */
    s1end   = s1 + s1max - 1;
    s2end   = s2 + n - 1;

    if ( ( (s2 >= s1) && (s2 <= s1end) )
         || ( (s2end >= s1) && (s2end <= s1end) )
         || ( (s1 >= s2) && (s1 <= s2end) )
         || ( (s1end >= s2) && (s1end <= s2end) ) )
    {
        FM_LOG_FATAL( FM_LOG_CAT_GENERAL,
                      "Overlapping buffers in FM_STRNCAT_S, s1=%p, s2=%p\n",
                      (void *) s1,
                      (void *) s2 );
        validCopy = FALSE;
    }

    if (validCopy)
    {
        s1[s1max - m + n] = 0;
        strncat(s1, s2, n);
        return 0;
    }

    s1[0] = 0;
    return EINVAL;

}   /* end fmStrncat_s */




/*****************************************************************************/
/** fmStrnlen_s
 * \ingroup intCommon
 *
 * \desc            Determine the length of a string.
 *
 * \note            This emulates strnlen_s.
 *
 * \param[in]       s points to the input string buffer. It must not be NULL.
 *
 * \param[in]       maxsize is the maximum size of the string buffer in bytes.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
rsize_t fmStrnlen_s(const char *s, rsize_t maxsize)
{

    if (s == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s pointer in FM_STRNLEN_S\n");
        return 0;
    }

    return strnlen(s, maxsize);

}   /* end fmStrnlen_s */




/*****************************************************************************/
/** fmStrtok_s
 * \ingroup intCommon
 *
 * \desc            Break a string into a sequence of tokens.
 *
 * \note            This emulates strntok_s.
 *
 * \param[out]      s1 points to the output string buffer. It must not be NULL.
 *
 * \param[in]       s1max is the size of s1, in bytes.
 *
 * \param[in]       s2 points to the input string buffer. It must not be NULL.
 * 
 * \param[out]      ptr points to a caller-provided location in which this
 *                  function stores state to be used in subsequent calls.
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
fm_char * fmStrtok_s(char *s1, rsize_t *s1max, const char *s2, char **ptr)
{
    rsize_t maxlen;
    rsize_t usedlen;
    char *  origptr;
    char *  retptr;

    if (s1max == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s1max pointer in FM_STRTOK_S\n");
        return NULL;
    }

    if (s2 == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s2 pointer in FM_STRTOK_S\n");
        return NULL;
    }

    if (ptr == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null ptr pointer in FM_STRTOK_S\n");
        return NULL;
    }

    maxlen = *s1max;

    if (s1 != NULL)
    {
        /* First call in sequence */
        if ( strnlen(s1, maxlen) >= maxlen )
        {
            /* This is allowed by strtok_s. We should never be using
             * strings that are not null-terminated. Since strtok_r
             * will not do the right thing anyway, we will just reject
             * it here. */
            FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                         "s1 string too long in FM_STRTOK_S, maxlen=%u\n",
                         maxlen);
            return NULL;
        }

        origptr = s1;
    }
    else if (*ptr == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null *ptr pointer in FM_STRTOK_S\n");
        return NULL;
    }
    else
    {
        origptr = *ptr;
    }

    retptr = strtok_r(s1, s2, ptr);

    usedlen = *ptr - origptr;
    *s1max -= usedlen;

    return retptr;

}   /* end fmStrtok_s */




/*****************************************************************************/
/** fmStrerror_s
 * \ingroup intCommon
 *
 * \desc            Map an error number to a message string.
 *
 * \note            This emulates strerror_s.
 *
 * \param[out]      s points to the output string buffer. It must not be NULL.
 *
 * \param[in]       maxsize is the maximum size of the string buffer in bytes.
 *
 * \param[in]       errnum is the value to be mapped
 *
 * \return          0 if successful.
 *
 *****************************************************************************/
errno_t fmStrerror_s(char *s, rsize_t maxsize, errno_t errnum)
{
    char *buf;

    if (s == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Null s pointer in FM_STRERROR_S\n");
        return EINVAL;
    }

    if ( (maxsize == 0) || (maxsize > RSIZE_MAX) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_GENERAL,
                     "Invalid maxsize value in FM_STRERROR_S: %u\n", maxsize);
        return EINVAL;
    }
    
    buf = strerror_r(errnum, s, maxsize);
    
    if (s != buf)
    {
        s[0] = '\0';
        return FM_STRCAT_S(s, maxsize, buf);
    }

    return 0;

}   /* end fmStrerror_s */
