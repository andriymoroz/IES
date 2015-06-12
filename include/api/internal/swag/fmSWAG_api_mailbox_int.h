/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_mailbox_int.h
 * Creation Date:   October 20, 2014
 * Description:     Contains functions dealing with switch-aggregate mailbox.
 *
 * Copyright (c) 2010 - 2014, Intel Corporation
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

#ifndef __FM_FMSWAG_API_MAILBOX_INT_H
#define __FM_FMSWAG_API_MAILBOX_INT_H

fm_status fmSWAGWriteResponseMessage(fm_int                        sw,
                                     fm_int                        swagPepNb,
                                     fm_mailboxControlHeader *     ctrlHdr,
                                     fm_mailboxMessageId           msgTypeId,
                                     fm_mailboxMessageArgumentType argType,
                                     fm_voidptr *                  message);

fm_status fmSWAGReadMailboxControlHdr(fm_int                   sw,
                                      fm_int                   swagPepNb,
                                      fm_mailboxControlHeader *ctrlHdr);

fm_status fmSWAGUpdateSmHeader(fm_int                   sw,
                               fm_int                   swagPepNb,
                               fm_mailboxControlHeader *controlHeader,
                               fm_uint32                updateType);

fm_status fmSWAGValidateMessageLength(fm_int                   sw,
                                      fm_int                   swagPepNb,
                                      fm_mailboxControlHeader *ctrlHdr,
                                      fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSWAGReadRequestArguments(fm_int                   sw,
                                     fm_int                   swagPepNb,
                                     fm_mailboxControlHeader *ctrlHdr,
                                     fm_mailboxMessageHeader *pfTrHdr,
                                     fm_uint16                argumentType,
                                     fm_uint16                argumentLength,
                                     fm_voidptr *             message);

fm_status fmSWAGProcessLoopbackRequest(fm_int                   sw,
                                       fm_int                   swagPepNb,
                                       fm_mailboxControlHeader *controlHeader);

fm_status fmSWAGProcessCreateFlowTableRequest(fm_int           sw,
                                              fm_int           swagPepNb,
                                              fm_int           tableIndex,
                                              fm_uint16        tableType,
                                              fm_flowCondition condition);

void fmSWAGSetLportGlortRange(fm_int              sw,
                              fm_int              swagPepNb,
                              fm_hostSrvLportMap *lportMap);

void fmSWAGSetHostSrvErrResponse(fm_int         sw,
                                 fm_int         swagPepNb,
                                 fm_hostSrvErr *srvErr);

fm_status fmSWAGMapPepToLogicalPort(fm_int  sw,
                                    fm_int  swagPepNb,
                                    fm_int *port );

fm_status fmSWAGAllocVirtualLogicalPort(fm_int  sw,
                                        fm_int  swagPepNb,
                                        fm_int  numberOfPorts,
                                        fm_int *swagFirstPort,
                                        fm_int  useHandle,
                                        fm_int  firstGlort);

fm_status fmSWAGFreeVirtualLogicalPort(fm_int  sw,
                                       fm_int  swagPepNb,
                                       fm_int  firstPort,
                                       fm_int  numberOfPorts);

fm_status fmSWAGGetSchedPortSpeedForPep(fm_int  sw,
                                        fm_int  swagPepNb,
                                        fm_int *speed);

fm_status fmSWAGMapGlortToPepNumber(fm_int    sw,
                                    fm_uint32 vsiGlort,
                                    fm_int *  pepNb);

fm_status fmSWAGMailboxInit(fm_int sw);

fm_status fmSWAGMailboxFreeResources(fm_int sw);

fm_status fmSWAGSetXcastFlooding(fm_int       sw,
                                 fm_int       swagPepNb,
                                 fm_xcastMode mode,
                                 fm_uint16    xcastFloodMode);

fm_status fmSWAGAssociateMcastGroupWithFlood(fm_int                sw,
                                             fm_int                swagPepNb,
                                             fm_int                floodPort,
                                             fm_intMulticastGroup *mcastGroup,
                                             fm_bool               associate);

fm_status fmSWAGGetMailboxGlortRange(fm_int     sw,
                                     fm_int     pepNb,
                                     fm_uint32 *glortBase,
                                     fm_int *   numberOfGlorts);

#endif /* __FM_FMSWAG_API_MAILBOX_INT_H */
