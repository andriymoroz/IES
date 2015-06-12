/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mirror.h
 * Creation Date:   March 27, 2006
 * Description:     Structures and functions for dealing with port mirroring
 *                  groups.
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

#ifndef __FM_FM_API_MIRROR_H
#define __FM_FM_API_MIRROR_H

/* Used for the FM_MIRROR_PRIORITY mirror group attribute to indicate that
 * mirrored frames should egress the mirror group's destination port with the
 * original priority of the mirrored frame. */
#define FM_MIRROR_PRIORITY_ORIGINAL        -1

/* The default sample rate for mirrored frames. A value of -1 indicates that
 * sampling is disabled. */
#define FM_MIRROR_SAMPLE_RATE_DISABLED     -1

/* Used for the FM_MIRROR_TX_EGRESS_PORT mirror group attribute to define the
 * port number being TX mirrored as the first logical egress port. */
#define FM_MIRROR_TX_EGRESS_PORT_FIRST     -1

/* Used for the FM_MIRROR_VLAN mirror group attribute to define the
 * encapsulating vlan of mirrored frames. */
#define FM_MIRROR_NO_VLAN_ENCAP            -1


/**************************************************/
/** \ingroup typeEnum
 *  A port mirror can act on ingress or egress.
 *  When acting on ingress, all incoming traffic on
 *  the mirrored port is sent to the mirror
 *  destination port. When acting on egress, all
 *  traffic that is to egress on the mirrored port
 *  is sent to the mirror destination port.
 *                                              \lb\lb
 *  Note that except where noted below, egress mirrored 
 *  traffic will appear at the mirror destination port 
 *  the way it appeared as it ingressed. That is, if the 
 *  switch alters the packet in any way before it egresses,
 *  the mirrored copy of that packet will not include 
 *  the changes to the packet.
 **************************************************/
typedef enum
{
    /** Mirror ingressing traffic on the mirrored port to the mirror
     *  destination port.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MIRROR_TYPE_INGRESS,

    /** Mirror egressing traffic on the mirrored port to the mirror
     *  destination port.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MIRROR_TYPE_EGRESS,

    /** Mirror ingressing and egressing traffic on the mirrored port to the
     *  mirror destination port.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MIRROR_TYPE_BIDIRECTIONAL,

    /** Mirror ingress traffic to the mirror destination port and drop the
     *  traffic ingressing the mirror ports.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_MIRROR_TYPE_REDIRECT,

    /** Mirror egressing traffic on the mirrored port to the mirror
     *  destination port.
     *
     *  \chips FM6000 */
    FM_MIRROR_TYPE_TX_EGRESS,

    /** Mirror ingressing and egressing traffic on the mirrored port to the
     *  mirror destination port. The mirrored copy will be identical to the
     *  ingress frame for packets mirrored on ingress and will be identical to
     *  the egress frame for packets mirrored on egress.
     *
     *  \chips FM6000 */
    FM_MIRROR_TYPE_RX_INGRESS_TX_EGRESS

} fm_mirrorType;


/**************************************************/
/** \ingroup typeEnum
 *  Used as an argument to ''fmAddMirrorVlan''.
 *  A VLAN mirror can act on ingress or egress.
 *  When acting on ingress, all incoming traffic on
 *  the mirrored VLAN is sent to the mirror
 *  destination port. When acting on egress, all
 *  traffic that is to egress on the mirrored VLAN
 *  is sent to the mirror destination port.
 **************************************************/
typedef enum
{
    /** VLAN condition only applied for traffic that ingresses with this
     *  specific VLAN.
     *  
     *  \chips FM6000 */
    FM_MIRROR_VLAN_INGRESS,

    /** VLAN condition only applied for traffic that egresses with this
     *  specific VLAN.
     *  
     *  \chips FM6000, FM10000 */
    FM_MIRROR_VLAN_EGRESS,

    /** VLAN condition applied for traffic that ingresses or egresses with this
     *  specific VLAN.
     *  
     *  \chips FM6000 */
    FM_MIRROR_VLAN_BIDIRECTIONAL

} fm_mirrorVlanType;


/****************************************************************************/
/** \ingroup constMirrorAttr
 *
 * Mirror Attributes, used as an argument to ''fmSetMirrorAttribute'' and
 * ''fmGetMirrorAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 *                                                                      \lb\lb
 ****************************************************************************/
enum _fm_mirrorAttr
{
    /** Type fm_int: Specifies the priority traffic class for the frame sent
     *  to the mirror destination port. Values range from 0 to 11. Default
     *  value is FM_MIRROR_PRIORITY_ORIGINAL, which indicates that the
     *  priority should be the original priority of the frame being
     *  mirrored.
     *
     *  \chips FM6000 */
    FM_MIRROR_PRIORITY,

    /** Type fm_bool: Specifies whether the mirrored frame sent to the
     *  mirror destination port is truncated: FM_ENABLED or FM_DISABLED
     *  (default).
     *
     *  \chips FM6000, FM10000 */
    FM_MIRROR_TRUNCATE,

    /** Type fm_int: To mirror frames with a particular sample rate expressed
     *  as a positive integer N, indicating that every Nth frame is to be
     *  sampled. For FM6000 devices, this value ranges from 1 to 65535.
     *  FM10000 devices support up to 16,777,216, with the default being
     *  FM_MIRROR_SAMPLE_RATE_DISABLED. The specified value will be rounded up
     *  or down to the nearest value supported by the hardware, so the value
     *  read back may not exactly match the value that was set.
     *                                                                  \lb\lb
     *  For FM6000 devices, only one sample rate can be specified for the
     *  entire switch, and all mirror groups will share the same value. The
     *  applied sample rate will be the last one specified.
     *  
     *  \chips FM6000, FM10000 */
    FM_MIRROR_SAMPLE_RATE,

    /** Type fm_bool: Specifies whether the mirrored frame is filtered using
     *  an ACL: FM_ENABLED or FM_DISABLED (default). If enabled, a frame
     *  will be mirrored only if it matches an ACL with action
     *  ''FM_ACL_ACTIONEXT_MIRROR_GRP'' and which identifies this mirror
     *  group AND if the mirror group specific conditions are met
     *  (ingress/egress port, vlan, sample rate).
     *                                                                  \lb\lb
     *  For FM10000 devices, this attribute must be set prior to the compilation
     *  of any ACL with action ''FM_ACL_ACTIONEXT_MIRROR_GRP'' that refers
     *  to this specific group. Once ACLs and Mirrors are bound together
     *  using this attribute for a specific group, the deletion of the
     *  latter must be done in a specific order. Any ACL Rules that refer to
     *  this group must be removed before the mirror group is deleted. The
     *  same sequence must be applied when this attribute is disabled.
     *
     *  \chips FM6000, FM10000 */
    FM_MIRROR_ACL,

    /** Type fm_int: Specifies the port number being TX mirrored if multiple
     *  egress ports are part of this mirror group. Value is the logical
     *  port selected with the default being FM_MIRROR_TX_EGRESS_PORT_FIRST,
     *  which indicates that the selected port is the first egress port of
     *  the mirror group.
     *
     *  \chips FM6000 */
    FM_MIRROR_TX_EGRESS_PORT,

    /** Type fm_bool: Specifies whether the mirrored frame sent to the other
     *  non-mirrored destination ports is truncated: FM_ENABLED or
     *  FM_DISABLED (default). The non-mirrored destination ports must be part
     *  of the truncate mask ''FM_MIRROR_TRUNCATE_MASK'' to be resized.
     *
     *  \chips FM6000 */
    FM_MIRROR_TRUNCATE_OTHERS,

    /** Type ''fm_bitArray'': The N-bit egress port mask used to manage
     *  truncation of other non-mirrored destination ports. Each bit position
     *  corresponds to the logical port number of the egress port. This mask
     *  only applies to mirror groups that enable the mirror attribute
     *  ''FM_MIRROR_TRUNCATE_OTHERS''. Only one truncate mask can be
     *  defined for the entire switch, and all mirror groups will share the 
     *  same value. The applied mask will be the last one specified.
     *                                                                  \lb\lb
     *  See ''fm_bitArray'' for a list of helper functions required to
     *  initialize, read, and write the ''fm_bitArray'' structure.
     *                                                                  \lb\lb
     *  This attribute is write only.
     *
     *  \chips FM6000 */
    FM_MIRROR_TRUNCATE_MASK,

    /** Type fm_int: Specifies the mirrored frame encapsulation vlan id.
     *  This value ranges from 1 to 4095, with the default being
     *  FM_MIRROR_NO_VLAN_ENCAP. When set, the mirrored traffic will have an
     *  additional vlan tag added in front of the existing packet. The
     *  Ethertype used in this case is the VLAN Ethertype configured for the
     *  port. The vlan priority of such encapsulated frame can be set using
     *  mirror attribute ''FM_MIRROR_VLAN_PRI''.
     *                                                                  \lb\lb
     *  Note: In a SWAG architecture, an encapsulation vlan MUST be
     *  configured prior to adding any mirrored ports or vlans. For
     *  consistent results across the SWAG, the vlan tagging state on the
     *  mirror port for the mirror vlan MUST be set to "tagged". The tagging
     *  state can be set using ''fmChangeVlanPortExt''.
     *
     *  \chips FM10000 */
    FM_MIRROR_VLAN,

    /** Type fm_byte: Specifies the mirrored frame encapsulation vlan
     *  priority. This value ranges from 0 to 15, with the default being 0.
     *                                                                  \lb\lb
     *  ''FM_MIRROR_VLAN'' must also be set if this attribute is specified.
     *
     *  \chips FM10000 */
    FM_MIRROR_VLAN_PRI,

    /** Type fm_int: Specifies the mirrored frame trap code id. Mirror Trap
     *  code is the result of ''FM_TRAPCODE_MIRROR_BASE'' +
     *  ''FM_MIRROR_TRAPCODE_ID''. Default trap code id is set to 0.
     *
     *  \chips FM10000 */
    FM_MIRROR_TRAPCODE_ID,

    /** UNPUBLISHED: For internal use only. */
    FM_MIRROR_ATTRIBUTE_MAX

};  /* end enum _fm_mirrorAttr */


/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmAddMirrorPort''. */
#define fmMirrorAddPort(sw, group, port) \
        fmAddMirrorPort( (sw), (group), (port) )

/** A legacy synonym for ''fmDeleteMirrorPort''. */
#define fmMirrorRemovePort(sw, group, port) \
        fmDeleteMirrorPort( (sw), (group), (port) )

/** A legacy synonym for ''fmGetMirrorPortFirst''. */
#define fmMirrorGetFirstPort(sw, group, firstPort)  \
        fmGetMirrorPortFirst( (sw), (group), (firstPort) )

/** A legacy synonym for ''fmGetMirrorPortNext''. */
#define fmMirrorGetNextPort(sw, group, currentPort, nextPort) \
        fmGetMirrorPortNext( (sw), (group), (currentPort), (nextPort) )

/** A legacy synonym for ''fmGetMirror''. */
#define fmGetMirrorGroup(sw, mirrorGroup, mirrorPort, mirrorType) \
        fmGetMirror( (sw), (mirrorGroup), (mirrorPort), (mirrorType) )

/** A legacy synonym for ''fmGetMirrorFirst''. */
#define fmGetMirrorGroupFirst(sw, firstGroup, mirrorPort, mirrorType) \
        fmGetMirrorFirst( (sw), (firstGroup), (mirrorPort), (mirrorType) )

/** A legacy synonym for ''fmGetMirrorNext''. */
#define fmGetMirrorGroupNext(sw, curGrp, nxtGrp, mirPort, mirType) \
        fmGetMirrorNext( (sw), (curGrp), (nxtGrp), (mirPort), (mirType) )

/** @} (end of Doxygen group) */


/* create and delete port mirroring groups */
fm_status fmCreateMirror(fm_int        sw,
                         fm_int        group,
                         fm_int        mirrorPort,
                         fm_mirrorType mirrorType);

fm_status fmDeleteMirror(fm_int sw, fm_int group);


/* set or change the mirror port after mirror creation */
fm_status fmSetMirrorDestination(fm_int sw, fm_int group, fm_int mirrorPort);
fm_status fmGetMirrorDestination(fm_int sw, fm_int group, fm_int *mirrorPort);
fm_status fmSetMirrorDestinationExt(fm_int             sw,
                                    fm_int             group,
                                    fm_portIdentifier *portId);
fm_status fmGetMirrorDestinationExt(fm_int             sw,
                                    fm_int             group,
                                    fm_portIdentifier *portId);


/* retrieve details about the existing port mirroring groups */
fm_status fmGetMirror(fm_int         sw,
                      fm_int         mirrorGroup,
                      fm_int *       mirrorPort,
                      fm_mirrorType *mirrorType);

fm_status fmGetMirrorFirst(fm_int         sw,
                           fm_int *       firstGroup,
                           fm_int *       mirrorPort,
                           fm_mirrorType *mirrorType);

fm_status fmGetMirrorNext(fm_int         sw,
                          fm_int         currentGroup,
                          fm_int *       nextGroup,
                          fm_int *       mirrorPort,
                          fm_mirrorType *mirrorType);

/* modify the port list for port mirror groups */
fm_status fmAddMirrorPort(fm_int sw, fm_int group, fm_int port);

fm_status fmAddMirrorPortExt(fm_int        sw,
                             fm_int        group,
                             fm_int        port,
                             fm_mirrorType type);

fm_status fmDeleteMirrorPort(fm_int sw, fm_int group, fm_int port);

fm_status fmGetMirrorPortFirst(fm_int sw, fm_int group, fm_int *firstPort);

fm_status fmGetMirrorPortNext(fm_int  sw,
                              fm_int  group,
                              fm_int  currentPort,
                              fm_int *nextPort);

fm_status fmGetMirrorPortFirstV2(fm_int         sw,
                                 fm_int         group,
                                 fm_int *       firstPort,
                                 fm_mirrorType *mirrorType);

fm_status fmGetMirrorPortNextV2(fm_int         sw,
                                fm_int         group,
                                fm_int         currentPort,
                                fm_int *       nextPort,
                                fm_mirrorType *mirrorType);

fm_status fmGetMirrorAttribute(fm_int sw,
                               fm_int group,
                               fm_int attr,
                               void * value);

fm_status fmSetMirrorAttribute(fm_int sw,
                               fm_int group,
                               fm_int attr,
                               void * value);

fm_status fmAddMirrorVlan(fm_int            sw,
                          fm_int            group,
                          fm_uint16         vlanID,
                          fm_mirrorVlanType direction);

fm_status fmAddMirrorVlanExt(fm_int            sw,
                             fm_int            group,
                             fm_vlanSelect     vlanSel,
                             fm_uint16         vlanID,
                             fm_mirrorVlanType direction);

fm_status fmDeleteMirrorVlan(fm_int sw, fm_int group, fm_uint16 vlanID);

fm_status fmDeleteMirrorVlanExt(fm_int        sw, 
                                fm_int        group, 
                                fm_vlanSelect vlanSel,
                                fm_uint16     vlanID);

fm_status fmGetMirrorVlanFirst(fm_int             sw,
                               fm_int             group,
                               fm_uint16 *        firstID,
                               fm_mirrorVlanType *direction);
fm_status fmGetMirrorVlanFirstExt(fm_int             sw,
                                  fm_int             group,
                                  fm_vlanSelect      vlanSel,
                                  fm_uint16 *        firstID,
                                  fm_mirrorVlanType *direction);
fm_status fmGetMirrorVlanNext(fm_int             sw,
                              fm_int             group,
                              fm_uint16          startID,
                              fm_uint16 *        nextID,
                              fm_mirrorVlanType *direction);
fm_status fmGetMirrorVlanNextExt(fm_int             sw,
                                 fm_int             group,
                                 fm_vlanSelect      vlanSel,
                                 fm_uint16          startID,
                                 fm_uint16 *        nextID,
                                 fm_mirrorVlanType *direction);


#endif /* __FM_FM_API_MIRROR_H */
