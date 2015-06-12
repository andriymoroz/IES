/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_multicast_int.h
 * Creation Date:   August 19, 2013
 * Description:     Contains functions dealing with Multicast Groups.
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

#ifndef __FM_FM10000_API_MULTICAST_INT_H
#define __FM_FM10000_API_MULTICAST_INT_H


typedef struct _fm10000_MulticastGroup
{
    fm_intMulticastGroup *parent;
    fm_int                mtableDestIndex;
    fm_bool               hasPepListeners;
    fm_bool               createdByPep;

} fm10000_MulticastGroup;


fm_status fm10000McastAddPortToLagReserveResources(fm_int sw,
                                                   fm_int lagIndex,
                                                   fm_int port);
fm_status fm10000McastReleaseReservation(fm_int sw);

fm_status fm10000FreeMcastResource(fm_int sw);

fm_status fm10000McastGroupInit(fm_int sw, fm_bool swagInit);

fm_status fm10000AllocateMcastGroups(fm_int    sw,
                                    fm_uint   startGlort,
                                    fm_uint   glortSize,
                                    fm_int    *baseMcastGroupHandle,
                                    fm_int    *numMcastGroups,
                                    fm_int    *step);
fm_status fm10000FreeMcastGroups(fm_int  sw,
                                 fm_int  baseMcastGroupHandle);

fm_status fm10000CreateMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fm10000DeleteMcastGroup(fm_int sw, fm_intMulticastGroup *group);

fm_status fm10000AddMulticastListener(fm_int                   sw,
                                      fm_intMulticastGroup *   group,
                                      fm_intMulticastListener *listener);
fm_status fm10000DeleteMulticastListener(fm_int                   sw,
                                         fm_intMulticastGroup *   group,
                                         fm_intMulticastListener *listener);

fm_status fm10000ActivateMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fm10000DeactivateMcastGroup(fm_int sw, fm_intMulticastGroup *group);

fm_status fm10000SetMcastGroupAttribute(fm_int                sw,
                                        fm_intMulticastGroup *group,
                                        fm_int                attr,
                                        void *                value);
fm_status fm10000GetMcastGroupAttribute(fm_int                sw,
                                        fm_intMulticastGroup *group,
                                        fm_int                attr,
                                        void *                value);
#if 0
fm_status fm10000GetMcastGroupTrigger(fm_int                sw,
                                      fm_intMulticastGroup *group,
                                      fm_int *              trigger);
fm_status fm10000McastInternalPortNotify(fm_int sw);
#endif 
fm_status fm10000GetHardwareMcastGlortRange(fm_uint32 *mcastGlortBase,
                                            fm_uint32 *mcastlagGlortCount);

fm_status fm10000GetMcastGroupHwIndex(fm_int                sw,
                                      fm_intMulticastGroup *group,
                                      fm_int *              hwIndex);

fm_status fm10000ConfigureMcastGroupAsHNIFlooding(fm_int  sw,
                                                  fm_int  mcastGroup,
                                                  fm_bool isHNIFlooding);

fm_status fm10000UpdateMcastHNIFloodingGroups(fm_int  sw,
                                              fm_int  port,
                                              fm_bool state);

#endif /* __FM_FM10000_API_MULTICAST_INT_H */
