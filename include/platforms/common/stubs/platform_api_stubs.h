/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_api_stubs.h
 * Creation Date:   2013
 * Description:     Default implementations of Platform API functions.
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

#ifndef __FM_PLATFORM_API_STUBS_H
#define __FM_PLATFORM_API_STUBS_H


/*****************************************************************************
 * Macros, Constants, and Types
 *****************************************************************************/

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketDirected. */
#ifndef FM_FM2000_SEND_PACKET_DIRECTED
#define FM_FM2000_SEND_PACKET_DIRECTED(sw, portList, numPorts, pkt, info)   \
        fm2000LCISendPacketDirected(sw, portList, numPorts, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketSwitched. */
#ifndef FM_FM2000_SEND_PACKET_SWITCHED      
#define FM_FM2000_SEND_PACKET_SWITCHED(sw, pkt)                             \
        fm2000LCISendPacketSwitched(sw, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketSwitched. */
#ifndef FM_FM2000_SEND_PACKET
#define FM_FM2000_SEND_PACKET(sw, info, pkt)                                \
        fm2000LCISendPacket(sw, info, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketDirected. */
#ifndef FM_FM4000_SEND_PACKET_DIRECTED
#define FM_FM4000_SEND_PACKET_DIRECTED(sw, portList, numPorts, pkt, info)   \
        fm4000GenericSendPacketDirected(sw, portList, numPorts, pkt, info)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketSwitched. */
#ifndef FM_FM4000_SEND_PACKET_SWITCHED      
#define FM_FM4000_SEND_PACKET_SWITCHED(sw, pkt)                             \
        fm4000GenericSendPacketSwitched(sw, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketISL. */
#ifndef FM_FM4000_SEND_PACKET_ISL
#define FM_FM4000_SEND_PACKET_ISL(sw, islTag, islTagFormat, pkt)            \
        fm4000GenericSendPacketISL(sw, islTag, islTagFormat, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketISL. */
#ifndef FM_FM4000_SEND_PACKET
#define FM_FM4000_SEND_PACKET(sw, info, pkt)                                \
        fm4000GenericSendPacket(sw, info, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketDirected. */
#ifndef FM_FM6000_SEND_PACKET_DIRECTED
#define FM_FM6000_SEND_PACKET_DIRECTED(sw, portList, numPorts, pkt, info)   \
        fm6000GenericSendPacketDirected(sw, portList, numPorts, pkt, info)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketSwitched. */
#ifndef FM_FM6000_SEND_PACKET_SWITCHED      
#define FM_FM6000_SEND_PACKET_SWITCHED(sw, pkt)                             \
        fm6000GenericSendPacketSwitched(sw, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketISL. */
#ifndef FM_FM6000_SEND_PACKET_ISL
#define FM_FM6000_SEND_PACKET_ISL(sw, islTag, islTagFormat, pkt)            \
        fm6000GenericSendPacketISL(sw, islTag, islTagFormat, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketISL. */
#ifndef FM_FM6000_SEND_PACKET
#define FM_FM6000_SEND_PACKET(sw, info, pkt)                                \
        fm6000GenericSendPacket(sw, info, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketDirected. */
#ifndef FM_FM10000_SEND_PACKET_DIRECTED
#define FM_FM10000_SEND_PACKET_DIRECTED(sw, portList, numPorts, pkt, info)  \
        fm10000GenericSendPacketDirected(sw, portList, numPorts, pkt, info)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketSwitched. */
#ifndef FM_FM10000_SEND_PACKET_SWITCHED      
#define FM_FM10000_SEND_PACKET_SWITCHED(sw, pkt)                            \
        fm10000GenericSendPacketSwitched(sw, pkt)
#endif

/* Can be overridden in platform_defines.h to call fmPlatformSendPacketISL. */
#ifndef FM_FM10000_SEND_PACKET_ISL
#define FM_FM10000_SEND_PACKET_ISL(sw, islTag, islTagFormat, pkt)           \
        fm10000GenericSendPacketISL(sw, islTag, islTagFormat, pkt)
#endif

/* Can be overridden in platform_defines.h */
#ifndef FM_PLATFORM_GET_HARDWARE_LAG_GLORT_RANGE
#ifdef FM_SUPPORT_FM10000
extern fm_status fm10000GetHardwareLagGlortRange(fm_uint32 *lagGlortBase, fm_uint32 *lagGlortCount);
#define FM_PLATFORM_GET_HARDWARE_LAG_GLORT_RANGE(lagGlortBase, lagGlortCount) \
        fm10000GetHardwareLagGlortRange(lagGlortBase, lagGlortCount)
#else
extern fm_status fmPlatformGetHardwareLagGlortRange(fm_uint32 *lagGlortBase, fm_uint32 *lagGlortCount);
#define FM_PLATFORM_GET_HARDWARE_LAG_GLORT_RANGE(lagGlortBase, lagGlortCount) \
        fmPlatformGetHardwareLagGlortRange(lagGlortBase, lagGlortCount)
#endif
#endif

/* Can be overridden in platform_defines.h */
#ifndef FM_PLATFORM_GET_HARDWARE_MCAST_GLORT_RANGE
#ifdef FM_SUPPORT_FM10000
extern fm_status fm10000GetHardwareMcastGlortRange(fm_uint32 *mcastGlortBase, fm_uint32 *mcastGlortCount);
#define FM_PLATFORM_GET_HARDWARE_MCAST_GLORT_RANGE(mcastGlortBase, mcastGlortCount) \
        fm10000GetHardwareMcastGlortRange(mcastGlortBase, mcastGlortCount)
#else
extern fm_status fmPlatformGetHardwareMcastGlortRange(fm_uint32 *mcastGlortBase, fm_uint32 *mcastGlortCount);
#define FM_PLATFORM_GET_HARDWARE_MCAST_GLORT_RANGE(mcastGlortBase, mcastGlortCount) \
        fmPlatformGetHardwareMcastGlortRange(mcastGlortBase, mcastGlortCount)
#endif
#endif

/* Can be overridden in platform_defines.h */
#ifndef FM_PLATFORM_GET_HARDWARE_MAILBOX_GLORT_RANGE
#ifdef FM_SUPPORT_FM10000
extern fm_status fm10000GetHardwareMailboxGlortRange(fm_uint16 *mailboxGlortBase,
                                                     fm_uint16 *mailboxGlortCount,
                                                     fm_uint16 *mailboxGlortMask,
                                                     fm_uint16 *mailboxGlortsPerPep,
                                                     fm_int     numberOfSWAGMembers);
#define FM_PLATFORM_GET_HARDWARE_MAILBOX_GLORT_RANGE(mailboxGlortBase,    \
                                                     mailboxGlortCount,   \
                                                     mailboxGlortMask,    \
                                                     mailboxGlortsPerPep, \
                                                     numberOfSWAGMembers) \
        fm10000GetHardwareMailboxGlortRange(mailboxGlortBase,    \
                                            mailboxGlortCount,   \
                                            mailboxGlortMask,    \
                                            mailboxGlortsPerPep, \
                                            numberOfSWAGMembers)
#else
extern fm_status fmPlatformGetHardwareMailboxGlortRange(fm_uint16 *mailboxGlortBase,
                                                        fm_uint16 *mailboxGlortCount,
                                                        fm_uint16 *mailboxGlortMask,
                                                        fm_uint16 *mailboxGlortsPerPep,
                                                        fm_int     numberOfSWAGMembers);
#define FM_PLATFORM_GET_HARDWARE_MAILBOX_GLORT_RANGE(mailboxGlortBase,    \
                                                     mailboxGlortCount,   \
                                                     mailboxGlortMask,    \
                                                     mailboxGlortsPerPep, \
                                                     numberOfSWAGMembers) \
        fmPlatformGetHardwareMailboxGlortRange(mailboxGlortBase,    \
                                               mailboxGlortCount,   \
                                               mailboxGlortMask,    \
                                               mailboxGlortsPerPep, \
                                               numberOfSWAGMembers)
#endif
#endif


/* Can be overridden in platform_defines.h */
#ifndef FM_PLATFORM_GET_HARDWARE_LBG_GLORT_RANGE
#ifdef FM_SUPPORT_FM10000
extern fm_status fm10000GetHardwareLbgGlortRange(fm_uint32 *lbgGlortBase, fm_uint32 *lbgGlortCount);
#define FM_PLATFORM_GET_HARDWARE_LBG_GLORT_RANGE(lbgGlortBase, lbgGlortCount) \
        fm10000GetHardwareLbgGlortRange(lbgGlortBase, lbgGlortCount)
#else
extern fm_status fmPlatformGetHardwareLbgGlortRange(fm_uint32 *lbgGlortBase, fm_uint32 *lbgGlortCount);
#define FM_PLATFORM_GET_HARDWARE_LBG_GLORT_RANGE(lbgGlortBase, lbgGlortCount) \
        fmPlatformGetHardwareLbgGlortRange(lbgGlortBase, lbgGlortCount)
#endif
#endif

/* Can be overridden in platform_defines.h */
#ifndef FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS
#ifdef FM_SUPPORT_FM10000
extern fm_uint16 fm10000GetHardwareNumberOfPeps(void);
#define FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS()    \
        fm10000GetHardwareNumberOfPeps()
#else
extern fm_uint16 fmPlatformGetHardwareNumberOfPeps(void);
#define FM_PLATFORM_GET_HARDWARE_NUMBER_OF_PEPS()    \
        fmPlatformGetHardwareNumberOfPeps()
#endif
#endif


#ifndef FM_PLATFORM_GET_HARDWARE_MAX_MAILBOX_GLORT
#ifdef FM_SUPPORT_FM10000
extern fm_status fm10000GetHardwareMaxMailboxGlort(fm_uint32 *glort);
#define FM_PLATFORM_GET_HARDWARE_MAX_MAILBOX_GLORT(glort)    \
        fm10000GetHardwareMaxMailboxGlort(glort)
#else
extern fm_status fmPlatformGetHardwareMaxMailboxGlort(fm_uint32 *glort);
#define FM_PLATFORM_GET_HARDWARE_MAX_MAILBOX_GLORT(glort)    \
        fmPlatformGetHardwareMaxMailboxGlort(glort)
#endif
#endif


#endif /* __FM_PLATFORM_API_STUBS_H */

