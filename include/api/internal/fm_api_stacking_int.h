/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stacking_int.h
 * Creation Date:   June 11, 2008
 * Description:     Internal prototypes for managing stacked intra and extra 
 *                  switch aggregate systems.
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_API_STACKING_INT_H
#define __FM_FM_API_STACKING_INT_H


/***************************************************
 * The internal version of the forwarding rule
 * object, pointing at the specific CAM entry that
 * the resource maps to.
 **************************************************/
typedef struct _fm_forwardRuleInternal
{
    /* The original rule */
    fm_forwardRule rule;

    /* Chip specific extensions to forward rules */
    void *extension;

} fm_forwardRuleInternal;

/***************************************************
 * This state structure holds all the per-switch
 * information related to stacking that is not
 * already held in some other state structure.
 **************************************************/
typedef struct _fm_stackingInfo
{
    /***************************************************
     * Holds a tree of forwarding rules by ID.  A 
     * different structure will hold them in LPM order.
     **************************************************/
    fm_tree fwdRules;

    /* Holds a list of used forwarding rule IDs */
    fm_bitArray usedRuleIDs;

} fm_stackingInfo;

fm_status fmInitStacking(fm_int sw);
fm_status fmFreeStackingResources(fm_int sw);
fm_status fmFindForwardingRulePortByGlort(fm_int    sw, 
                                          fm_uint32 glort, 
                                          fm_int *  logicalPort);
fm_status fmGetInternalPortFromRemotePort(fm_int sw, 
                                          fm_int remotePort,
                                          fm_int *internalPort);

#endif /* __FM_FM_API_STACKING_INT_H */
