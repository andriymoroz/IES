/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_c11_annex_k.h
 * Creation Date:  June 27, 2012
 * Description:    Functions and macros to provide equivalent functionality
 *                 to those functions/macros documented in ISO C 2011, Annex K.
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

#ifndef __FM_FM_C11_ANNEX_K_H
#define __FM_FM_C11_ANNEX_K_H

/* Some of the macros below use variadic macro features. The C standard
 * specifies that at least one variadic argument is required. GCC implemented
 * an extension using ## to suppress the trailing comma when no variadic
 * arguments are present. Microsoft has apparently implemented the same
 * extension in MSVC, since code compiles cleanly there as well. If other
 * compilers turn out to not support it, this file will need more compiler-
 * specific conditional code in order to handle the cases. */

#ifdef __STDC_LIB_EXT1__

/* C11 Annex K is supported. Implement the macros as direct calls
 * to the real functions. */


#define FM_FPRINTF_S(stream, format, ...)                                   \
    (fm_int) fprintf_s( (FILE * restrict) stream,                           \
                        (const char * restrict) format,                     \
                        ##__VA_ARGS__)
#define FM_PRINTF_S(format, ...)                                            \
    (fm_int) printf_s( (const char * restrict) format,                      \
                       ##__VA_ARGS__ )

#define FM_SNPRINTF_S(s, n, format, ...)                                    \
    (fm_int) snprintf_s( (char * restrict) s,                               \
                         (rsize_t) n,                                       \
                         (const char * restrict) format,                    \
                         ##__VA_ARGS__ )

#define FM_SPRINTF_S(s, n, format, ...)                                     \
    (fm_int) sprintf_s( (char * restrict) s,                                \
                        (rsize_t) n,                                        \
                        (const char * restrict) format,                     \
                        ##__VA_ARGS__ )

#define FM_SSCANF_S(s, format, ...)                                         \
    (fm_int) sscanf_s( (const char * restrict) s,                           \
                       (const char * restrict) format,                      \
                       ##__VA_ARGS__ )

#define FM_VFPRINTF_S(stream, format, arg)                                  \
    (fm_int) vfprintf_s( (FILE * restrict) stream,                          \
                         (const char * restrict) format,                    \
                         arg )

#define FM_VPRINTF_S(format, arg)                                           \
    (fm_int) vprintf_s( (const char * restrict) format,                     \
                        arg )

#define FM_VSNPRINTF_S(s, n, format, arg)                                   \
    (fm_int) vsnprintf_s( (char * restrict) s,                              \
                          (rsize_t) n,                                      \
                          (const char * restrict) format,                   \
                          arg )

#define FM_MEMCPY_S(s1, s1max, s2, n)                                       \
    memcpy_s( (void * restrict) s1,                                         \
              (rsize_t) s1max,                                              \
              (const void * restrict) s2,                                   \
              (rsize_t) n )

#define FM_MEMMOVE_S(s1, s1max, s2, n)                                      \
    memmove_s( (void * restrict) s1,                                        \
             (rsize_t) s1max,                                               \
             (const void * restrict) s2,                                    \
             (rsize_t) n )

#define FM_STRCPY_S(s1, s1max, s2)                                          \
    strcpy_s( (char * restrict) s1,                                         \
               (rsize_t) s1max,                                             \
               (const char * restrict) s2 )

#define FM_STRCAT_S(s1, s1max, s2)                                          \
    strcat_s( (char * restrict) s1,                                         \
               (rsize_t) s1max,                                             \
               (const char * restrict) s2 )

#define FM_STRNCPY_S(s1, s1max, s2, n)                                         \
    strncpy_s( (char * restrict) s1,                                        \
               (rsize_t) s1max,                                             \
               (const char * restrict) s2,                                  \
               (rsize_t) n )

#define FM_STRNCAT_S(s1, s1max, s2, n)                                         \
    strncat_s( (char * restrict) s1,                                        \
               (rsize_t) s1max,                                             \
               (const char * restrict) s2,                                  \
               (rsize_t) n )

#define FM_STRTOK_S(s1, s1max, s2)                                          \
    (fm_char *) strtok_s( (char * restrict) s1,                             \
                          (rsize_t * restrict) s1max,                       \
                          (const char * restrict) s2,                       \
                          (char ** restrict) ptr )

#define FM_MEMSET_S(s, smax, c, n)                                          \
    memset_s( s, (rsize_t) smax, c, (rsize_t) n)

#define FM_STRNLEN_S(s, maxsize)                                            \
    (fm_uint) strnlen_s( s, (rsize_t) maxsize )

#define FM_STRERROR_S(s, maxsize, errnum)                                   \
    (errno_t) strerror_s( (char *) s,                                       \
                          (rsize_t) maxsize,                                \
                          (errno_t) errnum) 


#else

typedef int errno_t;
typedef unsigned int rsize_t;

#ifndef RSIZE_MAX
#define RSIZE_MAX (1024*1024*1024)      /* default RSIZE_MAX to 1GB */
#endif

/* C11 Annex K is not supported. We must emulate those functions */

fm_int fmFprintf_s(FILE *stream, const char *format, ...);
fm_int fmPrintf_s(const char *format, ...);
fm_int fmSnprintf_s(char *s, rsize_t n, const char *format, ...);
fm_int fmSprintf_s(char *s, rsize_t n, const char *format, ...);
fm_int fmSscanf_s(const char *s, const char *format, ...);
fm_int fmVfprintf_s(FILE * stream, const char *format, va_list arg);
fm_int fmVprintf_s(const char *format, va_list arg);
fm_int fmVsnprintf_s(char *s, rsize_t n, const char *format, va_list arg);
errno_t fmMemcpy_s(void *s1, rsize_t s1max, const void *s2, rsize_t n);
errno_t fmMemmove_s(void *s1, rsize_t s1max, const void *s2, rsize_t n);
errno_t fmStrcpy_s(char *s1, rsize_t s1max, const char *s2);
errno_t fmStrcat_s(char *s1, rsize_t s1max, const char *s2);
errno_t fmStrncpy_s(char *s1, rsize_t s1max, const char *s2, rsize_t n);
errno_t fmStrncat_s(char *s1, rsize_t s1max, const char *s2, rsize_t n);
fm_char * fmStrtok_s(char *s1, rsize_t *s1max, const char *s2, char **ptr);
errno_t fmMemset_s(void *s, rsize_t smax, fm_int c, rsize_t n);
rsize_t fmStrnlen_s(const char *s, rsize_t maxsize);
errno_t fmStrerror_s(char *s, rsize_t maxsize, errno_t errnum);

#define FM_FPRINTF_S(stream, format, ...)  fmFprintf_s(stream, format, ##__VA_ARGS__)
#define FM_PRINTF_S(format, ...)           fmPrintf_s(format, ##__VA_ARGS__)
#define FM_SNPRINTF_S(s, n, format, ...)   fmSnprintf_s(s, n, format, ##__VA_ARGS__)
#define FM_SPRINTF_S(s, n, format, ...)    fmSprintf_s(s, n, format, ##__VA_ARGS__)
#define FM_SSCANF_S(s, format, ...)        fmSscanf_s(s, format, ##__VA_ARGS__)
#define FM_VFPRINTF_S(stream, format, arg) fmVfprintf_s(stream, format, arg)
#define FM_VPRINTF_S(format, arg)          fmVprintf_s(format, arg)
#define FM_VSNPRINTF_S(s, n, format, arg)  fmVsnprintf_s(s, n, format, arg)
#define FM_MEMCPY_S(s1, s1max, s2, n)      fmMemcpy_s(s1, s1max, s2, n)
#define FM_MEMMOVE_S(s1, s1max, s2, n)     fmMemmove_s(s1, s1max, s2, n)
#define FM_STRCPY_S(s1, s1max, s2)         fmStrcpy_s(s1, s1max, s2)
#define FM_STRCAT_S(s1, s1max, s2)         fmStrcat_s(s1, s1max, s2)
#define FM_STRNCPY_S(s1, s1max, s2, n)     fmStrncpy_s(s1, s1max, s2, n)
#define FM_STRNCAT_S(s1, s1max, s2, n)     fmStrncat_s(s1, s1max, s2, n)
#define FM_STRTOK_S(s1, s1max, s2, ptr)    fmStrtok_s(s1, s1max, s2, ptr)
#define FM_MEMSET_S(s, smax, c, n)         fmMemset_s(s, smax, c, n)
#define FM_STRNLEN_S(s, maxsize)           fmStrnlen_s(s, maxsize)
#define FM_STRERROR_S(s, maxsize, errnum)  fmStrerror_s(s, maxsize, errnum)


#endif


#endif  /* __FM_FM_C11_ANNEX_K_H */

