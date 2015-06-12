/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_port_int.h
 * Creation Date:   April 30, 2008
 * Description:     Contains functions dealing with the state of
 *                  switch-aggregate individual ports
 *
 * Copyright (c) 2008 - 2011, Intel Corporation
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

#ifndef __FM_FMSWAG_API_PORT_INT_H
#define __FM_FMSWAG_API_PORT_INT_H


fm_status fmSWAGSetPortState(fm_int sw,
                             fm_int port,
                             fm_int mac,
                             fm_int mode,
                             fm_int subMode);

fm_status fmSWAGGetPortState(fm_int  sw,
                             fm_int  port,
                             fm_int  mac,
                             fm_int  numBuffers,
                             fm_int *numLanes,
                             fm_int *mode,
                             fm_int *state,
                             fm_int *info);

fm_status fmSWAGGetNumPortLanes(fm_int sw, 
                                fm_int port, 
                                fm_int mac, 
                                fm_int *numLanes);

fm_status fmSWAGIsPortDisabled( fm_int   sw, 
                                fm_int   port, 
                                fm_int   mac, 
                                fm_bool *isDisabled );

fm_status fmSWAGIsPciePort(fm_int   sw,
                           fm_int   port,
                           fm_bool *isPciePort);

fm_status fmSWAGIsSpecialPort(fm_int   sw,
                              fm_int   port,
                              fm_bool *isSpecialPort);

fm_status fmSWAGSetPortQOS(fm_int sw,
                           fm_int port,
                           fm_int attr,
                           fm_int index,
                           void * value);
fm_status fmSWAGGetPortQOS(fm_int sw,
                           fm_int port,
                           fm_int attr,
                           fm_int index,
                           void * value);
fm_status fmSWAGSetFaultState(fm_int  sw,
                              fm_int  port,
                              fm_bool enable);

fm_status fmSWAGReleasePort(fm_int sw, fm_int port);

fm_status fmSWAGNotifyLinkEvent(fm_int sw, fm_int port);


/* Statistics. */
fm_status fmSWAGGetPortCounters(fm_int           sw,
                                fm_int           port,
                                fm_portCounters *counters);
fm_status fmSWAGResetPortCounters(fm_int           sw,
                                  fm_int           port);

fm_bool fmSWAGIsPerLagPortAttribute(fm_int sw, fm_uint attr);


#endif /* __FM_FMSWAG_API_PORT_INT_H */
