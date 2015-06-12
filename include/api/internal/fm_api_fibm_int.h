/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_fibm_int.h
 * Creation Date:   May 22, 2008
 * Description:     Helper FIBM functions
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

#ifndef __FM_FM_API_FIBM_INT_H
#define __FM_FM_API_FIBM_INT_H

/**************************************************
 * The following macros are used to enable and
 * disable FIBM batch processing. They should be
 * used where multiple register writes appear
 * together, usually in a loop. 
 *
 * Note that a register read flushes the batch, so
 * these macros will not help accelerate alternating 
 * reads and writes.
 **************************************************/
    
#define FM_BEGIN_FIBM_BATCH(sw, err, cat)                                   \
            (err) = fmFibmStartBatching( (sw), TRUE );                      \
            if ( (err) != FM_OK &&                                          \
                 (err) != FM_ERR_UNINITIALIZED &&                           \
                 (err) != FM_ERR_UNSUPPORTED )                              \
            {                                                               \
                FM_LOG_ABORT_ON_ERR( (cat), (err) );                        \
            }                                                               \
            else                                                            \
            {                                                               \
                (err) = FM_OK;                                              \
            }

#define FM_END_FIBM_BATCH(sw, err)                                          \
            (err) = fmFibmStartBatching( (sw), FALSE );                     \
            if ( (err) == FM_ERR_UNINITIALIZED ||                           \
                 (err) == FM_ERR_UNSUPPORTED )                              \
            {                                                               \
                (err) = FM_OK;                                              \
            }


/* Helper FIBM functions */
fm_bool fmFibmSlaveIsPortMgmt(fm_int sw, fm_int physPort);
fm_bool fmFibmSlaveIsLogicalPortMgmt(fm_int sw, fm_int logicalPort);
fm_int fmFibmSlaveGetMasterSwitch(fm_int slaveSw);

fm_status fmFibmStartBatching(fm_int sw, fm_bool start);

#endif /* __FM_FM_API_FIBM_INT_H */
