/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_qos_int.h
 * Creation Date:   2005
 * Description:     Contains functions dealing with the QOS settings, i.e.
 *                  watermarks, priority maps, etc.
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

#ifndef __FM_FM_API_QOS_INT_H
#define __FM_FM_API_QOS_INT_H

#define FM_QOS_MODE_MANUAL          0
#define FM_QOS_MODE_LOSSLESS_PAUSE  1

typedef struct _fm_wmParam
{
    fm_uint portNum;
    fm_uint memory;
    fm_uint segmentSize;
    fm_uint disableLevel;
    fm_uint numPausingFrames;
    fm_uint numBytesToBuffer;
    fm_uint globalPauseOnWm;
    fm_uint globalPauseOffWm;
    fm_uint rxPauseOnWm;
    fm_uint rxPauseOffWm;
    fm_uint rxSharedWm;
    fm_uint txSharedWm;
    fm_uint globalPrivWm;
    fm_uint globalHighWm;
    fm_uint globalLowWm;
    fm_uint cpuPrivWm;
    fm_uint cpuTxSharedWm;
    fm_uint rxPrivWm;
    fm_uint txPrivWm;
    fm_uint txTcPrivWm;
    fm_uint total;

} fm_wmParam;

/* Internal API functions. */
fm_status fmInitQOS(fm_switch *swstate);


#endif /* __FM_FM_API_QOS_INT_H */
