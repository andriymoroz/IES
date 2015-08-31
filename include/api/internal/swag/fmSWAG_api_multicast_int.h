/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_multicast_int.h
 * Creation Date:   June 10, 2008
 * Description:     Contains functions dealing with the state of
 *                  switch-aggregate multicast groups
 *
 * Copyright (c) 2008 - 2015, Intel Corporation
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

#ifndef __FM_FMSWAG_API_MULTICAST_INT_H
#define __FM_FMSWAG_API_MULTICAST_INT_H


/*****************************************************************************
 *
 * Multicast Group Entry Extension
 *
 *****************************************************************************/
typedef struct _fmSWAG_intMulticastGroup
{
    /* Pointer to the associated fm_intMulticastGroup structure */
    fm_intMulticastGroup *base;

    /* Index into SWAG multicast group list */
    fm_int                index;

    /* Array of switch-specific group numbers used for this group.
     * Subscript is the switch number, value is the multicast group
     * number(logical port) used by that switch.
     */
    fm_int                groups[FM_MAX_NUM_SWITCHES];

} fmSWAG_intMulticastGroup;


/*****************************************************************************
 *
 * Multicast Group functions
 *
 *****************************************************************************/
fm_status fmSWAGMcastGroupInit(fm_int sw, fm_bool swagInit);
fm_status fmSWAGMcastGroupFree(fm_int sw);
fm_status fmSWAGAllocateMcastGroups(fm_int  sw,
                                    fm_uint startGlort,
                                    fm_uint glortSize,
                                    fm_int *baseMcastGroupHandle,
                                    fm_int *numMcastGroups,
                                    fm_int *step);
fm_status fmSWAGFreeMcastGroups(fm_int sw,
                                fm_int baseMcastGroupHandle);
fm_status fmSWAGCreateMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fmSWAGDeleteMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fmSWAGSetMcastGroupAddress(fm_int                sw,
                                     fm_intMulticastGroup *group,
                                     fm_multicastAddress * address);
fm_status fmSWAGAddMcastGroupAddress(fm_int                sw,
                                     fm_intMulticastGroup *group,
                                     fm_multicastAddress * address);
fm_status fmSWAGDeleteMcastGroupAddress(fm_int                sw,
                                        fm_intMulticastGroup *group,
                                        fm_multicastAddress * address);
fm_status fmSWAGAddMulticastListener(fm_int                   sw,
                                     fm_intMulticastGroup *   group,
                                     fm_intMulticastListener *listener);
fm_status fmSWAGDeleteMulticastListener(fm_int                   sw,
                                        fm_intMulticastGroup *   group,
                                        fm_intMulticastListener *listener);
fm_status fmSWAGAttachMulticastGroupToMACAddress(fm_int                sw,
                                                 fm_intMulticastGroup *group);
fm_status fmSWAGAttachMulticastGroupToIPAddress(fm_int                sw,
                                                fm_intMulticastGroup *group);
fm_status fmSWAGDetachMulticastGroupFromAddress(fm_int                sw,
                                                fm_intMulticastGroup *group);
fm_status fmSWAGActivateMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fmSWAGDeactivateMcastGroup(fm_int sw, fm_intMulticastGroup *group);
fm_status fmSWAGSetMcastGroupAttribute(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_int                attr,
                                       void *                value);
fm_status fmSWAGGetMcastGroupAttribute(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_int                attr,
                                       void *                value);
fm_status fmSWAGAssignMcastGroup(fm_int sw, fm_int *handle);
fm_status fmSWAGUnassignMcastGroup(fm_int sw, fm_int handle);
fm_status fmSWAGGetMcastGroupUsed(fm_int                sw,
                                  fm_intMulticastGroup *mcastGroup,
                                  fm_bool *             used,
                                  fm_bool               resetFlag);
fm_status fmSWAGGetGlortForMulticastPort(fm_int     sw,
                                         fm_int     port,
                                         fm_uint32 *glort);
fm_int fmSWAGCompareMulticastAddresses(const void *key1,
                                       const void *key2);

fm_status fmSWAGGetAvailableMulticastListenerCount(fm_int  sw,
                                                   fm_int *count);

fm_status fmGetMcastGroupForSWAGGroup(fm_int                sw,
                                      fm_int                realSw,
                                      fm_intMulticastGroup *group,
                                      fm_int *              groupHandle);

fm_status fmSWAGConfigureMcastGroupAsHNIFlooding(fm_int  sw,
                                                 fm_int  mcastGroup,
                                                 fm_bool isHNIFlooding);

fm_status fmSWAGCreateSWAGMcastGroupsForSwitch(fm_int swArrayId, fm_int sw);

#endif /* __FM_FMSWAG_API_MULTICAST_INT_H */
