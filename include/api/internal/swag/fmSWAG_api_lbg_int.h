/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_lbg_int.h
 * Creation Date:   June 25, 2014
 * Description:     Functions for manipulating load balancing groups.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM_FMSWAG_API_LBG_INT_H
#define __FM_FMSWAG_API_LBG_INT_H

fm_status fmSWAGInitLBG(fm_int sw);
fm_status fmSWAGFreeLBG(fm_int sw);

/* Create and delete load balancing groups */
fm_status fmSWAGCreateLBG(fm_int        sw, 
                          fm_int *      lbgNumber, 
                          fm_LBGParams *params);
fm_status fmSWAGDeleteLBG(fm_int sw, fm_int lbgNumber);

/* Manage members of load balancing groups */
fm_status fmSWAGAddLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);
fm_status fmSWAGDeleteLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);

/* Handle setting attributes on load balancing groups */
fm_status fmSWAGSetLBGAttribute(fm_int sw,
                                fm_int lbgNumber,
                                fm_int attr,
                                void * value);
fm_status fmSWAGGetLBGAttribute(fm_int sw,
                                fm_int lbgNumber,
                                fm_int attr,
                                void * value);

/* Handle setting attributes on load balancing group members */
fm_status fmSWAGSetLBGPortAttribute(fm_int sw,
                                    fm_int lbgNumber,
                                    fm_int port,
                                    fm_int attr,
                                    void * value);
fm_status fmSWAGGetLBGPortAttribute(fm_int sw,
                                    fm_int lbgNumber,
                                    fm_int port,
                                    fm_int attr,
                                    void * value);
fm_status fmSWAGAllocateLBGs(fm_int    sw,
                             fm_uint   startGlort,
                             fm_uint   glortSize,
                             fm_int   *baseLbgHandle,
                             fm_int   *numLbgs,
                             fm_int   *step);

fm_status fmSWAGFreeStackLBGs(fm_int sw, fm_int baseLbgHandle);

fm_status fmSWAGGetGlortForLbgPort(fm_int     sw,
                                   fm_int     port,
                                   fm_uint32 *glort);

#endif /* __FM_FMSWAG_API_LBG_INT_H */
