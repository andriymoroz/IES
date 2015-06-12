/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_logging_int.h
 * Creation Date:   June 18, 2007
 * Description:     The SDK logging subsystem
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

#ifndef __FM_FM_ALOS_LOGGING_INT_H
#define __FM_FM_ALOS_LOGGING_INT_H

#define FM_MAX_FUNC_NAME_LENGTH    63

/* descriptor of the Object ID range for a given category */
typedef struct
{
    /* range type */
    fm_loggingRangeType rangeType;

    /* min object ID */
    fm_int              minObjectId;

    /* min object ID */
    fm_int              maxObjectId;

}   fm_loggingObjIdRange;


/* global state structure for logging facility */
typedef struct
{
    /***************************************************
     * When checking to see if we have initialized yet,
     * we check this value against FM_LOG_MAGIC_NUMBER.
     * This ensures that even if the memory for this
     * structure is zeroed, we will not assign the state
     * to "initialized" until code has explicitly set
     * this to FM_LOG_MAGIC_NUMBER.
     **************************************************/

    fm_uint32            initMagicNumber;

    /***************************************************
     * General logging state
     **************************************************/

    /* The destination type */
    fm_loggingType       logType;

    /* The flags for verbosity */
    fm_uint32            verbosityMask;

    /* Global enable/disable */
    fm_bool              enabled;

    /* Filter conditions */
    fm_uint64            categoryMask;
    fm_uint64            levelMask;
    fm_uint64            legacyLoggingMask;
    fm_char              functionFilter[FM_LOG_MAX_FILTER_LEN];
    fm_char              fileFilter[FM_LOG_MAX_FILTER_LEN];
    fm_loggingObjIdRange range[64];

    /* protects access to the logging destination */
    void *               accessLock;

    /***************************************************
     * Logging state for the file mode
     **************************************************/

    fm_char              logFileName[FM_MAX_FILENAME_LENGTH];

    /***************************************************
     * Logging state for the memory buffer mode
     **************************************************/

    fm_char              logBuffer[FM_LOG_MAX_LINES][FM_LOG_MAX_LINE_SIZE];
    fm_uint32            currentPos;
    fm_uint32            maxLines;

    /***************************************************
     * Logging state for the call-back mode
     **************************************************/

    fm_logCallBack       fmLogCallback;
    fm_voidptr           fmLogCookie1;
    fm_voidptr           fmLogCookie2;

    /***************************************************
     * Fault injection state
     **************************************************/
#if defined(FM_ALOS_FAULT_INJECTION_POINTS) && (FM_ALOS_FAULT_INJECTION_POINTS==FM_ENABLED)
    fm_char              failFunction[FM_MAX_FUNC_NAME_LENGTH + 1];
    fm_status            failError;
#endif

} fm_loggingState;

#endif /* __FM_FM_ALOS_LOGGING_INT_H */
