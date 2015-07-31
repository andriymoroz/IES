/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_debug_int.h
 * Creation Date:   April 23, 2013 (from fm6000_api_debug_int.h)
 * Description:     Prototypes for internal FM10000 debug functions.
 *
 * Copyright (c) 2008 - 2014, Intel Corporation
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

#ifndef __FM_FM10000_API_DEBUG_INT_H
#define __FM_FM10000_API_DEBUG_INT_H

void fm10000DbgDumpRegister(fm_int  sw,
                            fm_int  port,
                            fm_text regname);

fm_status fm10000DbgDumpRegisterV2(fm_int  sw,
                                   fm_int  indexA,
                                   fm_int  indexB,
                                   fm_text regname);

fm_status fm10000DbgDumpRegisterV3(fm_int  sw,
                                   fm_int  indexA,
                                   fm_int  indexB,
                                   fm_int  indexC,
                                   fm_text regname);

void fm10000DbgGetRegisterName(fm_int   sw,
                               fm_int   regId,
                               fm_uint  regAddress,
                               fm_text  regName,
                               fm_uint  regNameLength,
                               fm_bool *isPort,
                               fm_int * index0Ptr,
                               fm_int * index1Ptr,
                               fm_int * index2Ptr,
                               fm_bool  logicalPorts,
                               fm_bool  partialLongRegs);

void fm10000DbgReadRegister(fm_int  sw,
                            fm_int  firstIndex,
                            fm_int  secondIndex,
                            fm_text registerName,
                            void *  value);

void fm10000DbgWriteRegister(fm_int  sw,
                             fm_int  port,
                             fm_text regName,
                             fm_int  val);

fm_status fm10000DbgWriteRegisterV2(fm_int    sw,
                                    fm_int    wordOffset,
                                    fm_int    indexA,
                                    fm_int    indexB,
                                    fm_text   regName,
                                    fm_uint32 value);

fm_status fm10000DbgWriteRegisterV3(fm_int    sw,
                                    fm_int    wordOffset,
                                    fm_int    indexA,
                                    fm_int    indexB,
                                    fm_int    indexC,
                                    fm_text   regName,
                                    fm_uint32 value);

fm_status fm10000DbgWriteRegisterField(fm_int    sw, 
                                       fm_int    indexA, 
                                       fm_int    indexB, 
                                       fm_int    indexC, 
                                       fm_text   regName, 
                                       fm_text   fieldName, 
                                       fm_uint64 value);

void fm10000DbgListRegisters(fm_int  sw,
                             fm_bool showGlobals,
                             fm_bool showPorts);

void fm10000DbgTakeChipSnapshot(fm_int                sw,
                                fmDbgFulcrumSnapshot *pSnapshot,
                                fm_regDumpCallback    callback);

fm_status fm10000DbgGetRegInfo(fm_text    registerName,
                               fm_uint32 *registerAddr,
                               fm_int *   wordCnt,
                               fm_int *   idxMax0,
                               fm_int *   idxMax1,
                               fm_int *   idxMax2,
                               fm_int *   idxStep0,
                               fm_int *   idxStep1,
                               fm_int *   idxStep2);

fm_status fm10000DbgDumpRegField(fm_text    registerName,
                                 fm_uint32 *value);

fm_status fm10000DbgSetRegField(fm_text     registerName,
                                fm_text     fieldName,
                                fm_uint32 * regValue,
                                fm_uint64   fieldValue);
fm_status fm10000SerdesInitXDebugServicesInt(fm_int sw);
fm_status fm10000DumpPcieSerdesRegFields(fm_int regOff, fm_uint32 value);
fm_status fm10000DumpEplSerdesRegFields(fm_int regOff, fm_uint32 value);
fm_status fm10000DumpSpicoSerdesRegFields(fm_int intNum, fm_uint32 param, fm_uint32 value);

fm_status fm10000DbgTakeEyeDiagram(fm_int                sw,
                                   fm_int                port,
                                   fm_int                mac,
                                   fm_int                lane,
                                   fm_int               *count,
                                   fm_eyeDiagramSample **eyeDiagramPtr);

fm_status fm10000DbgPlotEyeDiagram(fm_eyeDiagramSample *sampleTable);

fm_status fm10000DbgDeleteEyeDiagram(fm_eyeDiagramSample *sampleTable);

fm_status fm10000DbgDumpPortMap(fm_int sw, fm_int port, fm_int portType);

fm_status fm10000DbgGetNominalSwitchVoltages(fm_int     sw,
                                             fm_uint32 *vdds,
                                             fm_uint32 *rawVdds,
                                             fm_uint32 *vddf,
                                             fm_uint32 *rawVddf);

#if 0
fm_status fm10000DbgSwitchSelfTest(fm_int sw);

fm_status fm10000DbgPolicerTest(fm_int  sw,
                                fm_int *portList,
                                fm_int  portCnt,
                                fm_bool mrlLimiter);

fm_status fm10000DbgShowSramErrors(fm_int sw);

fm_status fm10000DbgClearSramErrors(fm_int sw);

fm_status fm10000DbgDumpL2ArRuleHit(fm_int sw, fm_int slice);

fm_status fm10000DbgDumpL3ArRuleHit(fm_int sw, fm_int slice);

fm_status fm10000DbgDumpSwpriMap(fm_int sw, fm_int attr);

fm_status fm10000DbgDumpPortIdxMap(fm_int sw, fm_int port, fm_int attr);

fm_status fm10000DbgDumpPortMax(fm_int sw, fm_int port);



fm_status fm10000DbgGetNominalSwitchVoltages(fm_int     sw,
                                             fm_uint32 *vdd,
                                             fm_uint32 *rawVdd,
                                             fm_uint32 *vdds,
                                             fm_uint32 *rawVdds);
#endif

#endif  /* __FM_FM10000_API_DEBUG_INT_H */
