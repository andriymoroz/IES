/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util.h
 * Creation Date:   June 3, 2013
 * Description:     Utility functions.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#ifndef __FM_UTIL_H
#define __FM_UTIL_H

#define ALTA_I2C_ADDR       0x40

typedef fm_status (*fm_utilI2cWriteReadFunc)(int      fd,
                                             fm_int   dev,
                                             fm_byte *data,
                                             fm_int   numWrite,
                                             fm_int   numRead);

/* A more generic prototype */
typedef fm_status (*fm_utilI2cWriteReadHdnlFunc)(fm_uintptr handle,
                                                 fm_uint    device,
                                                 fm_byte   *data,
                                                 fm_uint    writeLen,
                                                 fm_uint    readLen);


typedef fm_status (*fm_utilRegRead32Func)(fm_uintptr handle,
                                          fm_uint    addr,
                                          fm_uint32 *value);

typedef fm_status (*fm_utilRegWrite32Func)(fm_uintptr handle,
                                           fm_uint    addr,
                                           fm_uint32  value);

typedef fm_status (*fm_utilRegRead32MultFunc)(fm_uintptr handle,
                                              fm_uint    addr,
                                              fm_uint    addrIncr,
                                              fm_uint32 *value,
                                              fm_uint    len);

typedef fm_status (*fm_utilRegWrite32MultFunc)(fm_uintptr handle,
                                               fm_uint    addr,
                                               fm_uint    addrIncr,
                                               fm_uint32 *value,
                                               fm_uint    len);

typedef void (*fm_setDebugFunc)(fm_int value);

typedef struct
{
    fm_text name;      /* field name */
    fm_int  start;     /* position in bits starting from 0-indexed LSB */
    fm_int  size;      /* length in bits */

} fmUtilRegisterField;


#define FM_UTIL_ERROR(...)  fmLogMessage(0, FM_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define FM_UTIL_WARN(...)   fmLogMessage(0, FM_LOG_LEVEL_WARNING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define FM_UTIL_INFO(...)   fmLogMessage(0, FM_LOG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define FM_UTIL_DEBUG(...)  fmLogMessage(0, FM_LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define FM_UTIL_PRINT(...)  fmLogMessage(0, FM_LOG_LEVEL_PRINT, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

#define FM_UTIL_HELP_FORMAT     "    %-30s - %-s\n"

#define FM_UTIL_FORMAT_B        "    %-20s: %d\n"
#define FM_UTIL_FORMAT_F        "    %-20s: %d 0x%x\n"
#define FM_UTIL_FORMAT_V        "    %-20s: %02x"
#define FM_UTIL_FORMAT_S        "    %-20s: %s\n"

#define DUMP_BIT(name, value, start) \
    printf(FM_UTIL_FORMAT_B, name, (fm_int) ( (value >> start ) & 0x1 ))

#define DUMP_VAL(name, value, start, size)                      \
    printf(FM_UTIL_FORMAT_F, name,                              \
           ( (value >> start ) &  ( ( 2 << (size -1) ) - 1 ) ), \
           ( (value >> start ) &  ( ( 2 << (size -1) ) - 1 ) ))

#define DUMP_FIELD(name, value, start, size, func)               \
    printf(FM_UTIL_FORMAT_S, name,                               \
           func((value >> start ) &  ( ( 2 << (size -1) ) - 1 )))\


fm_uint fmUtilGetIntervalMsec(struct timeval *begin, struct timeval *end);

fm_int fmUtilReadLine(char *prompt, char *line, int size);
void fmUtilReadLineRestoreTermSettings(void);

fm_status fmUtilParseInt(fm_char *string, fm_int *result);
fm_status fmUtilParseUint(fm_char *string, fm_uint *result);
fm_status fmUtilParseFloat(fm_char *string, fm_float *result);
fm_status fmUtilParseUint64(fm_char *string, fm_uint64 *result);
fm_status fmUtilParseList(fm_char *string, fm_uint *list, fm_uint listSize, fm_uint *len);
fm_int fmUtilSplit(fm_char *string, fm_char *tokList[], fm_int size);
fm_int fmUtilSplitDelim(fm_char *string, fm_text delim, fm_char *tokList[], fm_int size);

void fmUtilHexDump(fm_int addr, fm_byte *buf, fm_int nbytes);

fm_uint fmUtilGetTimeIntervalUsec(struct timeval *begin, struct timeval *end);
fm_uint fmUtilGetTimeIntervalMsec(struct timeval *begin, struct timeval *end);

fm_status fmUtilGetCpuInfo(fm_char *info, fm_int len);
fm_status fmUtilGetCaveCreekVersion(fm_char *version, fm_int len);

#endif /* __FM_UTIL_H */
