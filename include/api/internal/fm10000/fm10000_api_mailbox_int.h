/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mailbox_int.h
 * Creation Date:   May 13th, 2013
 * Description:     Definitions related to FM10000 support for mailbox.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_MAILBOX_INT_H
#define __FM_FM10000_API_MAILBOX_INT_H

/* Tail and head indexes are defined per queue and we need to calculate
 * offsets for MBMEM and queues. */
#define CALCULATE_PF_OFFSET_FROM_QUEUE_INDEX(index)     \
    (FM10000_MAILBOX_PF_MIN_QUEUE_INDEX + index)
#define CALCULATE_QUEUE_INDEX_FROM_PF_OFFSET(offset)    \
    (index - FM10000_MAILBOX_PF_MIN_QUEUE_INDEX)

#define CALCULATE_SM_OFFSET_FROM_QUEUE_INDEX(index)     \
    (FM10000_MAILBOX_SM_MIN_QUEUE_INDEX + index)
#define CALCULATE_QUEUE_INDEX_FROM_SM_OFFSET(offset)    \
    (index - FM10000_MAILBOX_SM_MIN_QUEUE_INDEX)

/* Macros for incrementing/decrementing mailbox queue indexes.
 * Index 0 is reserved for control header both for PF and SM queues. */
#define INCREMENT_PF_QUEUE_INDEX(index)                 \
    if (++index > FM10000_MAILBOX_PF_MAX_QUEUE_INDEX)   \
        index = (FM10000_MAILBOX_PF_MIN_QUEUE_INDEX + 1)
#define DECREMENT_PF_QUEUE_INDEX(index)                 \
    if (--index <= FM10000_MAILBOX_PF_MIN_QUEUE_INDEX)  \
        index = (FM10000_MAILBOX_PF_MAX_QUEUE_INDEX)

#define INCREMENT_SM_QUEUE_INDEX(index)                 \
    if (++index > FM10000_MAILBOX_SM_MAX_QUEUE_INDEX)   \
        index = (FM10000_MAILBOX_SM_MIN_QUEUE_INDEX + 1)
#define DECREMENT_SM_OFFSET_FOR_MBMEM(index)            \
    if (--index <= FM10000_MAILBOX_SM_MIN_QUEUE_INDEX)  \
        index = (FM10000_MAILBOX_SM_MAX_QUEUE_INDEX)

#define INCREMENT_QUEUE_INDEX(index)                    \
    if (++index >= FM10000_MAILBOX_QUEUE_SIZE)          \
        index = (FM10000_MAILBOX_QUEUE_MIN_INDEX)
#define DECREMENT_QUEUE_INDEX(index)                    \
    if (--index < FM10000_MAILBOX_QUEUE_MIN_INDEX)      \
        index = (FM10000_MAILBOX_QUEUE_SIZE - 1)

/* Macro for calculating used elements in mailbox queue. */
#define CALCULATE_USED_QUEUE_ELEMENTS(head, tail)       \
    (tail >= head) ? (tail - head) :                    \
    ((FM10000_MAILBOX_QUEUE_SIZE - head) + (tail -1))

/* SWAG PEP number is calculated according to the formula:
 * swagPepId = (realSw * NUMBER_OF_PEPS_IN_SWITCH) + realPepId */
#define CALCULATE_SWAG_PEP_ID(realSw, realPepId)   \
    ( (realSw * FM10000_NUM_PEPS) + realPepId )

/* Mailbox MBMEM register indexes for request (PF) and response (SM) queues. */
#define FM10000_MAILBOX_SM_MIN_QUEUE_INDEX          1024
#define FM10000_MAILBOX_SM_MAX_QUEUE_INDEX          1535
#define FM10000_MAILBOX_PF_MIN_QUEUE_INDEX          1536
#define FM10000_MAILBOX_PF_MAX_QUEUE_INDEX          2047


/* Mailbox MBMEM min index value for request/response size is 1.
 * 0 is reserved for mailbox control header. */
#define FM10000_MAILBOX_QUEUE_MIN_INDEX 1

/* Mailbox request/response queue size. */
#define FM10000_MAILBOX_QUEUE_SIZE       512

/* Mailbox glorts */
#define FM10000_MAILBOX_GLORT_BASE         0x4000
#define FM10000_MAILBOX_GLORT_MAX          0x75FF
#define FM10000_MAILBOX_GLORT_MASK         0xFF00
#define FM10000_MAILBOX_GLORT_MASK_CUT     0xFF80
#define FM10000_MAILBOX_GLORT_COUNT        0x900
#define FM10000_MAILBOX_GLORT_COUNT_CUT    0x480
#define FM10000_MAILBOX_GLORTS_PER_PEP     0x100
#define FM10000_MAILBOX_GLORTS_PER_PEP_CUT 0x80

/* Max number of switches in a SWAG supporting full glort range per PEP. */
#define FM10000_MAILBOX_MAX_SWITCHES_WITH_FULL_GLORT_RANGE  6

fm_status fm10000PCIeMailboxInterruptHandler(fm_int sw,
                                             fm_int pepNb);

fm_status fm10000WriteResponseMessage(fm_int                        sw,
                                      fm_int                        pepNb,
                                      fm_mailboxControlHeader *     ctrlHdr,
                                      fm_mailboxMessageId           msgTypeId,
                                      fm_mailboxMessageArgumentType argType,
                                      fm_voidptr *                  message);

fm_status fm10000ReadMailboxControlHdr(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr);

fm_status fm10000UpdateSmHeader(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *controlHeader,
                                fm_uint32                updateType);

fm_status fm10000ValidateMessageLength(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_mailboxMessageHeader *pfTrHdr);

fm_status fm10000ReadRequestArguments(fm_int                   sw,
                                      fm_int                   pepNb,
                                      fm_mailboxControlHeader *ctrlHdr,
                                      fm_mailboxMessageHeader *pfTrHdr,
                                      fm_uint16                argumentType,
                                      fm_uint16                argumentLength,
                                      fm_voidptr *             message);

fm_status fm10000ProcessLoopbackRequest(fm_int                   sw,
                                        fm_int                   pepNb,
                                        fm_mailboxControlHeader *controlHeader);

fm_status fm10000ProcessCreateFlowTableRequest(fm_int           sw,
                                               fm_int           pepNb,
                                               fm_int           tableIndex,
                                               fm_uint16        tableType,
                                               fm_flowCondition condition);

fm_status fm10000GetHardwareMailboxGlortRange(fm_uint16 *mailboxGlortBase,
                                              fm_uint16 *mailboxGlortCount,
                                              fm_uint16 *mailboxGlortMask,
                                              fm_uint16 *mailboxGlortsPerPep,
                                              fm_int     numberOfSWAGMembers);

fm_uint16 fm10000GetHardwareNumberOfPeps(void);

fm_status fm10000GetHardwareMaxMailboxGlort(fm_uint32 *glort);

fm_status fm10000FreeVirtualLogicalPort(fm_int sw,
                                        fm_int pepNb,
                                        fm_int firstPort,
                                        fm_int numberOfPorts);

fm_status fm10000AllocVirtualLogicalPort(fm_int  sw,
                                         fm_int  pepNb,
                                         fm_int  numberOfPorts,
                                         fm_int *firstPort,
                                         fm_int  useHandle,
                                         fm_int  firstGlort);

void fm10000SetLportGlortRange(fm_int              sw,
                               fm_int              pepNb,
                               fm_hostSrvLportMap *lportMap);

void fm10000SetHostSrvErrResponse(fm_int         sw,
                                  fm_int         pepNb,
                                  fm_hostSrvErr *srvErr);

fm_status fm10000MailboxAllocateDataStructures(fm_int sw);

fm_status fm10000MailboxFreeDataStructures(fm_int sw);

fm_status fm10000MailboxInit(fm_int sw);

fm_status fm10000MailboxFreeResources(fm_int sw);

fm_status fm10000SetMailboxGlobalInterrupts(fm_int  sw,
                                            fm_int  pepNb,
                                            fm_bool enable);

fm_status fm10000ResetPepMailboxVersion(fm_int sw, fm_int pepNb);

fm_status fm10000AllocVirtualLogicalPort(fm_int  sw,
                                         fm_int  pepNb,
                                         fm_int  numberOfPorts,
                                         fm_int *firstPort,
                                         fm_int  useHandle,
                                         fm_int  firstGlort);

fm_status fm10000FindInternalPortByMailboxGlort(fm_int    sw,
                                                fm_uint32 glort,
                                                fm_int *  logicalPort);

fm_status fm10000SetXcastFlooding(fm_int       sw,
                                  fm_int       pepNb,
                                  fm_xcastMode mode,
                                  fm_uint16    xcastFloodMode);

fm_status fm10000AssociateMcastGroupWithFlood(fm_int                sw,
                                              fm_int                swagPepNb,
                                              fm_int                floodPort,
                                              fm_intMulticastGroup *mcastGroup,
                                              fm_bool               associate);

fm_status fm10000GetMailboxGlortRange(fm_int     sw,
                                      fm_int     pepNb,
                                      fm_uint32 *glortBase,
                                      fm_int *   numberOfGlorts);

#endif /* __FM_FM10000_API_MAILBOX_INT_H */
