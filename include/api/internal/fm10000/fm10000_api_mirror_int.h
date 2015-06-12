/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mirror_int.h
 * Creation Date:   May 15, 2013.
 * Description:     Definitions related to FM10000 support for mirroring.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#ifndef __FM_FM10000_API_MIRROR_INT_H
#define __FM_FM10000_API_MIRROR_INT_H


/* Definitions related to mirror resources */
#define FM10000_MIRROR_NO_FFU_RES              -1
#define FM10000_MIRROR_NO_VLAN_RES             -1

/* Various definitions related to mirror trigger handling */
#define FM10000_MIRROR_TRIG_GROUP_SIZE          2
#define FM10000_MIRROR_TRIG_NAME_SIZE          64
#define FM10000_MIRROR_TRIG_MAX_SAMPLE   0xffffff
#define FM10000_MIRROR_CPU_CODE_BASE         0x50
#define FM10000_MIRROR_NUM_TRAPCODE_ID         16

typedef struct _fm_fm10000PortMirrorGroup
{
    /* Profile index used for this mirror group */
    fm_int mirrorProfile;

    /* FFU Trig ID used if needed or FM10000_MIRROR_NO_FFU_RES */
    fm_int ffuResId;

    /* FFU Trig ID mask used if needed or FM10000_MIRROR_NO_FFU_RES */
    fm_int ffuResIdMask;

    /* Vlan Trig ID used if needed or FM10000_MIRROR_NO_VLAN_RES */
    fm_int vlanResId;

    /* PortSet Id used to carry the ingress port mask */
    fm_int rxPortSet;

    /* PortSet Id used to carry the egress port mask */
    fm_int txPortSet;

} fm_fm10000PortMirrorGroup;


fm_status fm10000MirrorInit(fm_int sw);

fm_status fm10000GetMirrorId(fm_int  sw,
                             fm_int  group,
                             fm_int *ffuResId,
                             fm_int *ffuResIdMask);

fm_status fm10000CreateMirror(fm_int              sw,
                              fm_portMirrorGroup *grp);

fm_status fm10000DeleteMirror(fm_int              sw,
                              fm_portMirrorGroup *grp);

fm_status fm10000WritePortMirrorGroup(fm_int              sw,
                                      fm_portMirrorGroup *grp);

fm_status fm10000SetMirrorAttribute(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_int              attr,
                                    void *              value);

fm_status fm10000GetMirrorAttribute(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_int              attr,
                                    void *              value);

fm_status fm10000AddMirrorVlan(fm_int              sw,
                               fm_portMirrorGroup *grp,
                               fm_vlanSelect       vlanSel,
                               fm_uint16           vlanID,
                               fm_mirrorVlanType   direction);

fm_status fm10000DeleteMirrorVlan(fm_int              sw,
                                  fm_portMirrorGroup *grp,
                                  fm_vlanSelect       vlanSel,
                                  fm_uint16           vlanID);

fm_status fm10000DbgDumpMirror(fm_int sw);

#endif
