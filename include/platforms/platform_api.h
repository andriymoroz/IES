/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_api.h
 * Creation Date:   January 8, 2013
 * Description:     Platform layer API services.
 * 
 * Specifies the interface between the API and the platform layer.
 * As a rule of thumb, the platform layer needs to implement or provide
 * stubs for all the functions in this file.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_PLATFORM_API_H
#define __FM_PLATFORM_API_H


/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/

/****************************************************************************/
/** Platform Lane Reversal Values
 *  \ingroup constPlatLaneRev
 *  \page portLaneReversals
 *
 *  The following set of bit masks may be ORed together to identify the
 *  lane reversal settings initialized by the platform layer implementation
 *  and returned by ''fmPlatformGetPortDefaultSettings''.
 ****************************************************************************/
/** \ingroup constPlatLaneRev
 * @{ */

/** Port lanes are not reversed. */
#define FM_LANE_REVERSE_NONE        0

/** Port RX lanes are reversed. */
#define FM_LANE_REVERSE_RX          1

/** Port TX lanes are reversed. */
#define FM_LANE_REVERSE_TX          2

/** Port RX and TX lanes are reversed. */
#define FM_LANE_REVERSE_RX_TX       (FM_LANE_REVERSE_RX | FM_LANE_REVERSE_TX)

/** @} (end of Doxygen group) */



/****************************************************************************/
/** Platform Lane Polarity Values
 *  \ingroup constPlatLanePol
 *  \page portLanePolarities
 *
 *  The following set of bit masks may be ORed together to identify the
 *  lane polarity inversion settings initialized by the platform layer 
 *  implementation and returned by ''fmPlatformGetPortDefaultSettings''.
 ****************************************************************************/
/** \ingroup constPlatLanePol
 * @{ */

/** Port lane polarity is not inverted. */
#define FM_POLARITY_INVERT_NONE     0

/** Port RX lane polarity is inverted. */
#define FM_POLARITY_INVERT_RX       1

/** Port TX lane polarity is inverted. */
#define FM_POLARITY_INVERT_TX       2

/** Port RX and TX lane polarity is inverted. */
#define FM_POLARITY_INVERT_RX_TX    (FM_POLARITY_INVERT_RX | FM_POLARITY_INVERT_TX)

/** @} (end of Doxygen group) */



/****************************************************************************/
/** Platform Interrupt Types
 *  \ingroup constPlatIntrType
 *  \page platIntrTypes
 *
 *  The following set of bit masks may be ORed together to specify the
 *  interrupt types to be processed by ''fmPlatformEnableInterrupt'' or
 *  ''fmPlatformGetInterrupt''.
 ****************************************************************************/
/** \ingroup constPlatIntrType
 * @{ */

/** No interrupt source specified. */
#define FM_INTERRUPT_SOURCE_NONE	0

/** Interrupt triggered by the API. */
#define FM_INTERRUPT_SOURCE_API   	1

/** Interrupt triggered by an Interrupt Service Routine. */
#define FM_INTERRUPT_SOURCE_ISR   	2

/** @} (end of Doxygen group) */



/*****************************************************************************
 * Function prototypes
 *****************************************************************************/


/**************************************************
 * Initialization and termination
 **************************************************/

fm_status fmPlatformInitialize(fm_int *nSwitches);
fm_status fmPlatformSwitchPreInitialize(fm_int sw);
fm_status fmPlatformSwitchPostInitialize(fm_int sw);
fm_status fmPlatformSwitchInitialize(fm_int sw);
fm_status fmPlatformSwitchTerminate(fm_int sw);
fm_status fmPlatformSwitchInserted(fm_int sw);
fm_status fmPlatformSwitchPreInsert(fm_int sw);
fm_status fmPlatformTerminate(void);

fm_status fmPlatformRelease(fm_int sw);
fm_status fmPlatformReset(fm_int sw);


/**************************************************
 * SWAG services
 **************************************************/

#ifdef FM_SUPPORT_SWAG
fm_status fmPlatformCreateSWAG(fm_int *sw);
fm_status fmPlatformDeleteSWAG(fm_int sw);

fm_status fmPlatformSWAGInitialize(fm_int sw);
#endif


/**************************************************
 * Interrupt management
 **************************************************/

fm_status fmPlatformEnableInterrupt(fm_int sw, fm_uint intrTypes);
fm_status fmPlatformDisableInterrupt(fm_int sw, fm_uint intrTypes);
fm_status fmPlatformGetInterrupt(fm_int sw, fm_uint intrTypes, fm_uint *intrSrc);


/**************************************************
 * Bypass mode support
 **************************************************/

fm_bool   fmPlatformBypassEnabled(fm_int sw);
fm_status fmPlatformSetBypassMode(fm_int sw, fm_bool mode);


/**************************************************
 * Part number services
 **************************************************/

fm_status fmPlatformGetSwitchPartNumber(fm_int sw, fm_switchPartNum *spn);


/**************************************************
 * Buffer management services
 **************************************************/

fm_status fmPlatformGetAvailableBuffers(fm_int *count);
fm_buffer *fmPlatformAllocateBuffer(void);
fm_status fmPlatformFreeBuffer(fm_buffer *buf);


/**************************************************
 * Packet transfer functions
 **************************************************/

fm_status fmPlatformReceivePackets(fm_int sw);

fm_status fmPlatformSendPackets(fm_int sw);

fm_status fmPlatformSendPacket(fm_int         sw,
                               fm_packetInfo *info,
                               fm_buffer *    packet);

fm_status fmPlatformSendPacketDirected(fm_int           sw, 
                                       fm_int *         portList,
                                       fm_int           numPorts, 
                                       fm_buffer *      packet,
                                       fm_packetInfoV2 *info);

fm_status fmPlatformSendPacketSwitched(fm_int sw, fm_buffer *packet);

fm_status fmPlatformSendPacketISL(fm_int          sw,
                                  fm_uint32 *     islTag,
                                  fm_islTagFormat islTagFormat,
                                  fm_buffer *     buffer);

fm_status fmPlatformReceiveProcess(fm_int     sw,
                                   fm_buffer *buffer,
                                   fm_uint32 *pIslTag,
                                   fm_uint    flags);

fm_status fmPlatformReceiveProcessV2(fm_int              sw,
                                     fm_buffer *         buffer,
                                     fm_uint32 *         pIslTag,
                                     fm_pktSideBandData *sbData);


/**************************************************
 * Port-related services
 **************************************************/

fm_status fmPlatformGetPortCapabilities(fm_int     sw,
                                        fm_int     physPort,
                                        fm_uint32 *capabilities);

#ifdef FM_SUPPORT_FM6000
fm_status fmPlatformGetPortDefaultSettings(fm_int  sw,
                                           fm_int  logPort,
                                           fm_int *activeMac,
                                           fm_int *ethMode,
                                           fm_int *ordering,
                                           fm_int *polarity);
#endif

fm_status fmPlatformMapLogicalPortToPhysical(fm_int  logicalSwitch,
                                             fm_int  logicalPort,
                                             fm_int *physSwitch,
                                             fm_int *physPort);

fm_status fmPlatformMapPhysicalPortToLogical(fm_int  physSwitch,
                                             fm_int  physPort,
                                             fm_int *logicalSwitch,
                                             fm_int *logicalPort);

fm_status fmPlatformNotifyPortState(fm_int sw,
                                    fm_int logPort,
                                    fm_int mac,
                                    fm_bool isConfig,
                                    fm_int state);

fm_status fmPlatformNotifyPortAttribute(fm_int sw,
                                        fm_int port,
                                        fm_int mac,
                                        fm_int lane,
                                        fm_int attribute,
                                        void * value);

fm_status fmPlatformMapPortLaneToEplLane(fm_int  sw,
                                         fm_int  logicalPort,
                                         fm_int  portLaneNum,
                                         fm_int *eplLane);

/**************************************************
 * Miscellaneous services
 **************************************************/

fm_status fmPlatformAddGlortToPortMapping(fm_int sw,
                                          fm_int glort,
                                          fm_int physPort);

fm_status fmPlatformSetRemoteGlortToLogicalPortMapping(fm_int  sw,
                                                       fm_int  glort,
                                                       fm_int  port,
                                                       fm_bool valid);

fm_status fmPlatformSetCpuPortVlanTag(fm_int sw, fm_uint16 vlanId, fm_bool tag);

#if defined(FM_SUPPORT_FM2000) || defined(FM_SUPPORT_FM4000)
fm_status fmPlatformGetPortClockSel(fm_int  sw,
                                    fm_int  physPort,
                                    fm_int  speed,
                                    fm_int *clockSel);
#endif

fm_bool   fmPlatformMACMaintenanceSupported(fm_int sw);

fm_status fmPlatformSetPortDefaultVlan(fm_int sw,
                                       fm_int physPort,
                                       fm_int defaultVlan);

#ifdef FM_SUPPORT_FM6000
fm_status fmPlatformLoadMicrocode(fm_int sw);
#endif

#if defined(FM_SUPPORT_FM6000) || defined(FM_SUPPORT_FM10000)
fm_status fmPlatformGetSchedulerConfig(fm_int sw, fm_schedulerConfig *sc);
#endif

#if defined(FM_SUPPORT_FM10000)
fm_status fmPlatformGenerateSchedulerPortList(fm_int sw);
fm_status fmPlatformAllocateSchedulerResources(fm_int sw);
fm_status fmPlatformGpioInterruptHandler(fm_switch *switchPtr);
#endif

fm_status fmPlatformGetHardwareLagGlortBase(fm_uint32 *lagGlortBase, fm_uint32 *lagGlortCount);

#endif /* __FM_PLATFORM_API_H */

