/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mirror_int.h
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with port mirroring
 *                  groups
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

#ifndef __FM_FM_API_MIRROR_INT_H
#define __FM_FM_API_MIRROR_INT_H


/* holds information about port mirror groups */
typedef struct
{
    /* Group ID */
    fm_int              groupId;

    /* Port mask for ports being ingress mirrored.
     * One bit per cardinal port. */
    fm_bitArray         ingressPortUsed;

    /* Port mask for ports being egress mirrored.
     * One bit per cardinal port. 
     * This mirror may be RX egress or TX egress, depending upon the
     * mirror type requested. */
    fm_bitArray         egressPortUsed;

    /* Mirror destination port type: either port number or port mask */
    fm_portIdentifierType mirrorPortType;

    /* Logical port of a physical port or remote port under stacking */
    fm_int              mirrorLogicalPort;

    /* Port Mask for multiple destination port situations (such as LAGs).
     * One bit per cardinal port. */
    fm_bitArray         mirrorLogicalPortMask;

    /* TRUE if the mirror group is in use */
    fm_bool             used;

    /* Mirror Type (ingress, egress, bidirectional, etc.) */
    fm_mirrorType       mirrorType;

    /* Which mirroring mode is in use (overlay/explicit). Default is overlay.
     * TRUE for overlay, FALSE for explicit. */
    fm_bool             overlayMode;

    /* Egress Traffic Class */
    fm_int              egressPriority;

    /* Truncate Mirrored Frames */
    fm_bool             truncateFrames;

    /* Truncate Others non Mirrored Frames */
    fm_bool             truncateOtherFrames;

    /* Tree of all the configured VLAN1s specific for this group. The key is
     * the vlan while the value refer to the fm_mirrorVlanType direction. */
    fm_tree             vlan1s;

    /* Tree of all the configured VLAN2s specific for this group. The key is
     * the vlan while the value refer to the fm_mirrorVlanType direction. */
    fm_tree             vlan2s;

    /* Sample rate used for this group. Defaulted to -1 to indicate that none
     * of the sampler are used for this group. */
    fm_int              sample;

    /* Whether to filter mirrored frames using the FFU. */
    fm_bool             ffuFilter;

    /* Port number being TX mirrored. */
    fm_int              egressSrcPort;

    /* Encapsulating Vlan Id of mirrored frames */
    fm_int              encapVlan;

    /* Encapsulating Vlan Priority of mirrored frames */
    fm_byte             encapVlanPri;

    /* Trap Code ID of mirrored frames */
    fm_int              trapCodeId;

} fm_portMirrorGroup;


fm_status fmAllocatePortMirrorDataStructures(fm_switch *switchPtr);
fm_status fmInitPortMirror(fm_switch *switchPtr);
fm_status fmFreePortMirrorDataStructures(fm_switch *switchPtr);


/* returns the port to which a specified port is being mirrored, or -1 if
 *  the port is not being mirrored */
fm_int fmGetMirrorPortDest(fm_int sw, fm_int port, fm_mirrorType mirrorType);


#endif /* __FM_FM_API_MIRROR_INT_H */
