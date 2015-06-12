/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lag.h
 * Creation Date:   May 5, 2005
 * Description:     Structures and functions for dealing with link aggregation
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FM_API_LAG_H
#define __FM_FM_API_LAG_H

/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmCreateLAG''. */
#define fmCreateLAGExt(sw, lagNumber) \
        fmCreateLAG(sw, lagNumber) 

/** A legacy synonym for ''fmDeleteLAG''. */
#define fmDeleteLAGExt(sw, lagNumber) \
        fmDeleteLAG(sw, lagNumber) 

/** A legacy synonym for ''fmGetLAGList''. */
#define fmGetLAGListExt(sw, nLAG, lagNumbers, maxLags) \
        fmGetLAGList(sw, nLAG, lagNumbers, maxLags) 

/** A legacy synonym for ''fmGetLAGPortList''. */
#define fmGetLAGPortListExt(sw, lagNumber, nPorts, ports, maxPorts) \
        fmGetLAGPortList(sw, lagNumber, nPorts, ports, maxPorts) 

/** A legacy synonym for ''fmGetLAGFirst''. */
#define fmGetLAGFirstExt(sw, firstLagNumber) \
        fmGetLAGFirst(sw, firstLagNumber) 

/** A legacy synonym for ''fmGetLAGNext''. */
#define fmGetLAGNextExt(sw, currentLagNumber, nextLagNumber) \
        fmGetLAGNext(sw, currentLagNumber, nextLagNumber) 

/** A legacy synonym for ''fmGetLAGPortFirst''. */
#define fmGetLAGPortFirstExt(lagNumber, firstPort, firstSwitch) \
        fmGetLAGPortFirst(lagNumber, firstPort, firstSwitch) 

/** A legacy synonym for ''fmGetLAGPortNext''. */
#define fmGetLAGPortNextExt(sw, lagNumber, currentPort, nextPort) \
        fmGetLAGPortNext(sw, lagNumber, currentPort, nextPort) 

/** A legacy synonym for ''fmGetLAGAttribute''. */
#define fmGetLAGAttributeExt(sw, attribute, index, value) \
        fmGetLAGAttribute(sw, attribute, index, value) 

/** A legacy synonym for ''fmSetLAGAttribute''. */
#define fmSetLAGAttributeExt(sw, attribute, index, value) \
        fmSetLAGAttribute(sw, attribute, index, value) 

 /** @} (end of Doxygen group) */

fm_status fmCreateLAG(fm_int sw, fm_int *lagNumber);
fm_status fmDeleteLAG(fm_int sw, fm_int lagNumber);
fm_status fmAddLAGPort(fm_int sw, fm_int lagNumber, fm_int port);
fm_status fmDeleteLAGPort(fm_int sw, fm_int lagNumber, fm_int port);

fm_status fmGetLAGList(fm_int  sw,
                       fm_int* nLAG,
                       fm_int* lagNumbers,
                       fm_int  maxLags);
fm_status fmGetLAGPortList(fm_int  sw,
                           fm_int  lagNumber,
                           fm_int *nPorts,
                           fm_int *ports,
                           fm_int  maxPorts);
fm_status fmGetLAGFirst(fm_int sw, fm_int *firstLagNumber);
fm_status fmGetLAGNext(fm_int sw,
                       fm_int currentLagNumber,
                       fm_int *nextLagNumber);

fm_status fmGetLAGPortFirst(fm_int  sw,
                            fm_int  lagNumber,
                            fm_int *firstPort);
fm_status fmGetLAGPortNext(fm_int  sw,
                           fm_int  lagNumber,
                           fm_int  currentPort,
                           fm_int *nextPort);

fm_status fmGetLAGAttribute(fm_int sw, fm_int attribute, fm_int index, void *value);
fm_status fmSetLAGAttribute(fm_int sw, fm_int attribute, fm_int index, void *value);

/* converts LAG handle to/from logical port number */
fm_status fmLogicalPortToLAGNumber(fm_int  sw,
                                   fm_int  logicalPort,
                                   fm_int *lagNumber);
fm_status fmLAGNumberToLogicalPort(fm_int  sw,
                                   fm_int  lagNumber,
                                   fm_int *logicalPort);

#endif /* __FM_FM_API_LAG_H */
