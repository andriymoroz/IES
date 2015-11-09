/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_acl.c
 * Creation Date:  June 14, 2013
 * Description:    ACL compiler for FM10000.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#include <fm_sdk_fm10000_int.h>
#include <common/fm_version.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/* Bottom two bits of the TOS/TC octet are ECN.  DSCP starts above that. */
#define FM_DSCP_POS                                2


/* Simple helper macro that counts and returns the number of bits set in
 * value. */
#define ABSTRACT_SELECT(selKey) [selKey] =


#define FM_COUNT_SET_BITS(value, bits)                                         \
    {                                                                          \
        fm_int modifValue = value;                                             \
        for ((bits) = 0; modifValue; (bits)++)                                 \
        {                                                                      \
            modifValue &= modifValue - 1;                                      \
        }                                                                      \
    }


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/**
 * This array maps abstract selects to concrete mux.
 *
 * [0..3] = S0-S3.
 */
const fm_byte fmAbstractToConcrete8bits[][4] =
{
    ABSTRACT_SELECT(FM10000_ABSTRACT_NONE) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_7_0) {
        FM_FFU_MUX_SELECT_DIP_31_0,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_31_0,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_23_16) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_31_0,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_31_24) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_31_0
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_39_32) {
        FM_FFU_MUX_SELECT_DIP_63_32,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_47_40) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_63_32,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_55_48) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_63_32,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_63_56) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_63_32
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_71_64) {
        FM_FFU_MUX_SELECT_DIP_95_64,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_79_72) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_95_64,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_87_80) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_95_64,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_95_88) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_95_64
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_103_96) {
        FM_FFU_MUX_SELECT_DIP_127_96,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_111_104) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_127_96,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_119_112) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_127_96,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DIP_127_120) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DIP_127_96
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_7_0) {
        FM_FFU_MUX_SELECT_SIP_31_0,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_31_0,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_23_16) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_31_0,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_31_24) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_31_0
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_39_32) {
        FM_FFU_MUX_SELECT_SIP_63_32,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_47_40) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_63_32,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_55_48) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_63_32,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_63_56) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_63_32
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_71_64) {
        FM_FFU_MUX_SELECT_SIP_95_64,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_79_72) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_95_64,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_87_80) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_95_64,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_95_88) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_95_64
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_103_96) {
        FM_FFU_MUX_SELECT_SIP_127_96,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_111_104) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_127_96,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_119_112) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_127_96,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SIP_127_120) {
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SIP_127_96
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_7_0) {
        FM_FFU_MUX_SELECT_DMAC_15_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_15_0,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_15_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_15_0
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_23_16) {
        FM_FFU_MUX_SELECT_DMAC_31_16,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_31_16,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_31_24) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_31_16,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_31_16
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_39_32) {
        FM_FFU_MUX_SELECT_DMAC_47_32,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_47_32,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DMAC_47_40) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_47_32,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DMAC_47_32
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_7_0) {
        FM_FFU_MUX_SELECT_SMAC_15_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_15_0,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_15_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_15_0
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_23_16) {
        FM_FFU_MUX_SELECT_SMAC_31_16,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_31_16,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_31_24) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_31_16,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_31_16
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_39_32) {
        FM_FFU_MUX_SELECT_SMAC_47_32,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_47_32,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SMAC_47_40) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_47_32,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SMAC_47_32
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DGLORT_7_0) {
        FM_FFU_MUX_SELECT_DGLORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DGLORT,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_DGLORT_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DGLORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_DGLORT
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SGLORT_7_0) {
        FM_FFU_MUX_SELECT_SGLORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SGLORT,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SGLORT_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SGLORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SGLORT
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VLAN_VPRI1_7_0) {
        FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VLAN_VPRI1_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VLAN_VPRI2_7_0) {
        FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VLAN_VPRI2_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TYPE_7_0) {
        FM_FFU_MUX_SELECT_ETHER_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ETHER_TYPE,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TYPE_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ETHER_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ETHER_TYPE
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4DST_7_0) {
        FM_FFU_MUX_SELECT_L4DST,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4DST,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4DST_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4DST,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4DST
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4SRC_7_0) {
        FM_FFU_MUX_SELECT_L4SRC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4SRC,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4SRC_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4SRC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4SRC
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_L4DST_7_0) {
        FM_FFU_MUX_SELECT_MAP_L4DST,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4DST,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_L4DST_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4DST,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4DST
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_L4SRC_7_0) {
        FM_FFU_MUX_SELECT_MAP_L4SRC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4SRC,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_L4SRC_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4SRC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_L4SRC
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4A_7_0) {
        FM_FFU_MUX_SELECT_L4A,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4A,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4A_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4A,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4A
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4B_7_0) {
        FM_FFU_MUX_SELECT_L4B,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4B,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4B_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4B,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4B
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4C_7_0) {
        FM_FFU_MUX_SELECT_L4C,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4C,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4C_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4C,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4C
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4D_7_0) {
        FM_FFU_MUX_SELECT_L4D,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4D,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_L4D_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4D,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_L4D
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_VLAN_VPRI1_7_0) {
        FM_FFU_MUX_SELECT_MAP_VLAN_VPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_VLAN_VPRI,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_VLAN_VPRI1_15_8) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_VLAN_VPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_VLAN_VPRI
    }
};  /* end fmAbstractToConcrete8bits[][4] */

/**
 * This array maps abstract selects to concrete mux on a 4 bits granularity.
 *
 * [0..1] = S0.
 * [2..3] = S1.
 * [4..5] = S2.
 * [6..7] = S3.
 * [8..9] = S4.
 */
const fm_byte fmAbstractToConcrete4bits[][10] =
{
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_DIP) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_SIP) {
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_DMAC) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_SMAC) {
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_PROT) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_LENGTH) {
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_SRC) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_MAP_TYPE) {
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_USER_3_0) {
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_USER_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_USER
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_FTYPE) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SWPRI) {
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_IPMISC_3_0) {
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_IPMISC_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_MISC
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TOS_3_0) {
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TOS_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TOS
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_PROT_3_0) {
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_PROT_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_PROT
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TTL_3_0) {
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_TTL_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_TTL
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SRC_PORT_3_0) {
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_SRC_PORT_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_SRC_PORT
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VID_3_0) {
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VID_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VID_7_0
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VID_11_8) {
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_VPRI) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_VPRI_VID_11_8
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_RXTAG_3_0) {
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY
    },
    ABSTRACT_SELECT(FM10000_ABSTRACT_RXTAG_7_4) {
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG,
        FM10000_INVALID_KEY,
        FM_FFU_MUX_SELECT_RXTAG
    }

};  /* end fmAbstractToConcrete4bits[][10] */




static fm_byte fmL2DeepInspection[FM10000_MAX_ACL_NON_IP_PAYLOAD_BYTES] =
{
    FM10000_ABSTRACT_SIP_127_120,    /* 01 */
    FM10000_ABSTRACT_SIP_119_112,    /* 02 */
    FM10000_ABSTRACT_SIP_111_104,    /* 03 */
    FM10000_ABSTRACT_SIP_103_96,     /* 04 */
    FM10000_ABSTRACT_SIP_95_88,      /* 05 */
    FM10000_ABSTRACT_SIP_87_80,      /* 06 */
    FM10000_ABSTRACT_SIP_79_72,      /* 07 */
    FM10000_ABSTRACT_SIP_71_64,      /* 08 */
    FM10000_ABSTRACT_SIP_63_56,      /* 09 */
    FM10000_ABSTRACT_SIP_55_48,      /* 10 */
    FM10000_ABSTRACT_SIP_47_40,      /* 11 */
    FM10000_ABSTRACT_SIP_39_32,      /* 12 */
    FM10000_ABSTRACT_SIP_31_24,      /* 13 */
    FM10000_ABSTRACT_SIP_23_16,      /* 14 */
    FM10000_ABSTRACT_SIP_15_8,       /* 15 */
    FM10000_ABSTRACT_SIP_7_0,        /* 16 */
    FM10000_ABSTRACT_DIP_127_120,    /* 17 */
    FM10000_ABSTRACT_DIP_119_112,    /* 18 */
    FM10000_ABSTRACT_DIP_111_104,    /* 19 */
    FM10000_ABSTRACT_DIP_103_96,     /* 20 */
    FM10000_ABSTRACT_DIP_95_88,      /* 21 */
    FM10000_ABSTRACT_DIP_87_80,      /* 22 */
    FM10000_ABSTRACT_DIP_79_72,      /* 23 */
    FM10000_ABSTRACT_DIP_71_64,      /* 24 */
    FM10000_ABSTRACT_DIP_63_56,      /* 25 */
    FM10000_ABSTRACT_DIP_55_48,      /* 26 */
    FM10000_ABSTRACT_DIP_47_40,      /* 27 */
    FM10000_ABSTRACT_DIP_39_32,      /* 28 */
    FM10000_ABSTRACT_DIP_31_24,      /* 29 */
    FM10000_ABSTRACT_DIP_23_16,      /* 30 */
    FM10000_ABSTRACT_DIP_15_8,       /* 31 */
    FM10000_ABSTRACT_DIP_7_0         /* 32 */
};


static fm_byte fmL4DeepInspection[FM10000_MAX_ACL_L4_DEEP_INSPECTION_BYTES] =
{
    FM10000_ABSTRACT_L4A_15_8,       /* 01 */
    FM10000_ABSTRACT_L4A_7_0,        /* 02 */
    FM10000_ABSTRACT_L4B_15_8,       /* 03 */
    FM10000_ABSTRACT_L4B_7_0,        /* 04 */
    FM10000_ABSTRACT_L4C_15_8,       /* 05 */
    FM10000_ABSTRACT_L4C_7_0,        /* 06 */
    FM10000_ABSTRACT_L4D_15_8,       /* 07 */
    FM10000_ABSTRACT_L4D_7_0,        /* 08 */

    /* Only available if not IPv6 frame */
    FM10000_ABSTRACT_SIP_63_56,      /* 09 */
    FM10000_ABSTRACT_SIP_55_48,      /* 10 */
    FM10000_ABSTRACT_SIP_47_40,      /* 11 */
    FM10000_ABSTRACT_SIP_39_32,      /* 12 */
    FM10000_ABSTRACT_SIP_95_88,      /* 13 */
    FM10000_ABSTRACT_SIP_87_80,      /* 14 */
    FM10000_ABSTRACT_SIP_79_72,      /* 15 */
    FM10000_ABSTRACT_SIP_71_64,      /* 16 */
    FM10000_ABSTRACT_SIP_127_120,    /* 17 */
    FM10000_ABSTRACT_SIP_119_112,    /* 18 */
    FM10000_ABSTRACT_SIP_111_104,    /* 19 */
    FM10000_ABSTRACT_SIP_103_96,     /* 20 */
    FM10000_ABSTRACT_DIP_63_56,      /* 21 */
    FM10000_ABSTRACT_DIP_55_48,      /* 22 */
    FM10000_ABSTRACT_DIP_47_40,      /* 23 */
    FM10000_ABSTRACT_DIP_39_32,      /* 24 */
    FM10000_ABSTRACT_DIP_95_88,      /* 25 */
    FM10000_ABSTRACT_DIP_87_80,      /* 26 */
    FM10000_ABSTRACT_DIP_79_72,      /* 27 */
    FM10000_ABSTRACT_DIP_71_64,      /* 28 */
    FM10000_ABSTRACT_DIP_127_120,    /* 29 */
    FM10000_ABSTRACT_DIP_119_112,    /* 30 */
    FM10000_ABSTRACT_DIP_111_104,    /* 31 */
    FM10000_ABSTRACT_DIP_103_96      /* 32 */

};

/* Variable used to manage the FM_ACL_COMPILE_FLAG_TRY_ALLOC compiler flag */
static fm_bool compileTryAlloc = FALSE;
static fm_int  compileTryAllocFirstSlice;
static fm_int  compileTryAllocLastSlice;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

void fmFreeCompiledAclRule(void *value);
void fmFreeCompiledAcl(void *value);
void fmFreeCompiledAclInstance(void *value);
void fmFreeAclPortSet(void *value);
void fmFreeEcmpGroup(void *value);
void fmFreeCompiledPolicerEntry(void *value);

fm_status fmAddAbstractKey(fm_tree * abstractKey,
                           fm_byte   firstAbstract,
                           fm_byte   lastAbstract,
                           fm_byte   bitsPerKey,
                           fm_uint64 mask,
                           fm_uint64 value);
fm_status fmAddIpAbstractKey(fm_tree * abstractKey,
                             fm_byte   firstAbstract,
                             fm_ipAddr mask,
                             fm_ipAddr value);
fm_status fmAddDeepInsAbstractKey(fm_tree * abstractKey,
                                  fm_byte * abstractTable,
                                  fm_byte   tableSize,
                                  fm_byte * mask,
                                  fm_byte * value);

fm_status fmFillAbstractPortSetKeyTree(fm_aclErrorReporter *      errReport,
                                       fm_aclRule *               rule,
                                       fm_fm10000CompiledAclRule *compiledAclRule,
                                       fm_tree *                  abstractKey,
                                       fm_tree *                  portSetId);
fm_status fmFillAbstractKeyTree(fm_int               sw,
                                fm_aclErrorReporter *errReport,
                                fm_aclRule *         rule,
                                fm_tree *            abstractKey,
                                fm_tree *            portSetId);

fm_int fmCountConditionSliceUsage(fm_byte *muxSelect);
void fmInitializeConcreteKey(fm_byte *muxSelect);
fm_status fmConvertAbstractToConcreteKey(fm_tree *      abstractKeyTree,
                                         fm_byte *      muxSelect,
                                         fm_uint16 *    muxUsed);

fm_status fmCountActionSlicesNeeded(fm_int               sw,
                                    fm_aclErrorReporter *errReport,
                                    fm_aclRule *         rule,
                                    fm_int *             actionSlices);

void fmTranslateAclScenario(fm_int      sw,
                            fm_uint32   aclScenario,
                            fm_uint32  *validScenarios,
                            fm_bool     egressAcl);

fm_status fmConfigureConditionKey(fm_int                         sw,
                                  fm_aclErrorReporter *          errReport,
                                  fm_aclRule *                   rule,
                                  fm_int                         ruleNumber,
                                  fm_fm10000CompiledAcl *        compiledAcl,
                                  fm_fm10000CompiledAclInstance *compiledAclInst);
fm_status fmConfigureActionData(fm_int                      sw,
                                fm_aclErrorReporter *       errReport,
                                fm_fm10000CompiledPolicers *policers,
                                fm_tree *                   ecmpGroups,
                                fm_aclRule *                rule,
                                fm_fm10000CompiledAcl *     compiledAcl,
                                fm_fm10000CompiledAclRule * compiledAclRule);
fm_status fmConfigureEgressActionData(fm_aclRule *               rule,
                                      fm_aclErrorReporter *      errReport,
                                      fm_fm10000CompiledAclRule *compiledAclRule);
fm_status fmConvertAclPortToPortSet(fm_int                 sw,
                                    fm_tree *              globalPortSet,
                                    fm_fm10000CompiledAcl *compiledAcl,
                                    fm_int                 mappedValue,
                                    fm_uint64              key);

void fmUpdateValidSlice(fm_ffuSliceInfo*        sliceInfo,
                        fm_fm10000CompiledAcls *cacls);
fm_status fmUpdateMasterValid(fm_int sw, fm_fm10000CompiledAcls *cacls);
fm_status fmUpdateBstMasterValid(fm_int sw, fm_fm10000CompiledAcls *cacls);
fm_status fmApplyMapSrcPortId(fm_int    sw,
                              fm_tree * portSetIdTree);
void fmInitializeMuxSelect(fm_byte *srcArray,
                           fm_byte *dstArray);

fm_status fmSetEaclChunkCfg(fm_int    sw,
                            fm_int    chunk,
                            fm_uint32 scenarios,
                            fm_bool   cascade,
                            fm_tree * portTree);
fm_status fmResetFFUSlice(fm_int sw, fm_ffuSliceInfo *sliceInfo);
fm_status fmResetBSTSlice(fm_int sw, fm_ffuSliceInfo *sliceInfo);

fm_status fmGetSlicePosition(fm_int           sw,
                             fm_ffuSliceInfo *sliceInfo,
                             fm_int           nextFreeConditionSlice,
                             fm_int           nextFreeActionSlice,
                             fm_int *         conditionSlicePos,
                             fm_int *         actionSlicePos);
fm_status fmGetAclNumKey(fm_tree *  aclTree,
                         fm_int     acl,
                         fm_int     rule,
                         fm_uint64 *aclNumKey);
fm_status fmGetFFUSliceRange(fm_int sw, fm_int *firstSlice, fm_int *lastSlice);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** FreeCompiledAclsStruct
 * \ingroup intAcl
 *
 * \desc            Free a fm_fm10000CompiledAcls structure.
 *
 * \param[in,out]   cacls points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FreeCompiledAclsStruct(fm_fm10000CompiledAcls *cacls)
{
    fm_int i;

    if (cacls)
    {
        fmTreeDestroy(&cacls->ingressAcl, fmFreeCompiledAcl);
        fmTreeDestroy(&cacls->egressAcl, fmFreeCompiledAcl);
        fmTreeDestroy(&cacls->instance, fmFreeCompiledAclInstance);
        fmTreeDestroy(&cacls->portSetId, fmFreeAclPortSet);
        fmTreeDestroy(&cacls->ecmpGroups, fmFreeEcmpGroup);
        fmTreeDestroy(&cacls->mappers, fmFree);

        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            fmTreeDestroy(&cacls->policersId[i], NULL);
            fmTreeDestroy(&cacls->policers[i].acl, fmFree);
            fmTreeDestroy(&cacls->policers[i].policerEntry,
                          fmFreeCompiledPolicerEntry);
        }

        fmFree(cacls);
    }

}   /* end FreeCompiledAclsStruct */


/*****************************************************************************/
/** InitializeCompiledAcls
 * \ingroup intAcl
 *
 * \desc            Initialize a compiledAcls structure.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   cacls points to the compiledAcls structure to initialize.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void InitializeCompiledAcls(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls)
{
    fm_int i;

    fmTreeInit(&cacls->ingressAcl);
    fmTreeInit(&cacls->egressAcl);
    fmTreeInit(&cacls->instance);
    fmTreeInit(&cacls->portSetId);
    fmTreeInit(&cacls->ecmpGroups);
    fmTreeInit(&cacls->mappers);

    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        fmTreeInit(&cacls->policersId[i]);
        fmTreeInit(&cacls->policers[i].acl);
        fmTreeInit(&cacls->policers[i].policerEntry);
    }

}   /* end InitializeCompiledAcls */




/*****************************************************************************/
/** CloneEcmpGroup
 * \ingroup intAcl
 *
 * \desc            Clone the ecmp list.
 * 
 * \param[in]       value refer to the ecmp list element to clone.
 * 
 * \param[in]       funcArg in not used.
 *
 * \return          The cloned ecmp list.
 *
 *****************************************************************************/
static void * CloneEcmpGroup(void *value, void *funcArg)
{
    fm_status err;
    fm_dlist *ecmpList = (fm_dlist *) value;
    fm_dlist *ecmpListClone;
    fm_fm10000AclRule* ecmpRule;
    fm_fm10000AclRule* ecmpRuleClone;
    fm_dlist_node* node;

    FM_NOT_USED(funcArg);

    ecmpListClone = (fm_dlist *) fmAlloc(sizeof(fm_dlist));
    if (ecmpListClone == NULL)
    {
        return NULL;
    }
    FM_CLEAR(*ecmpListClone);

    fmDListInit(ecmpListClone);

    node = FM_DLL_GET_FIRST( (ecmpList), head );
    while (node != NULL)
    {
        ecmpRule = (fm_fm10000AclRule *) node->data;
        ecmpRuleClone = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
        if (ecmpRuleClone == NULL)
        {
            fmFreeEcmpGroup(ecmpListClone);
            return NULL;
        }
        FM_CLEAR(*ecmpRuleClone);

        ecmpRuleClone->aclNumber = ecmpRule->aclNumber;
        ecmpRuleClone->ruleNumber = ecmpRule->ruleNumber;
        err = fmDListInsertEnd(ecmpListClone,
                               (void*) ecmpRuleClone);
        if (err != FM_OK)
        {
            fmFree(ecmpRuleClone);
            fmFreeEcmpGroup(ecmpListClone);
            return NULL;
        }

        node = FM_DLL_GET_NEXT(node, next);
    }

    return ecmpListClone;

}   /* end CloneEcmpGroup */




/*****************************************************************************/
/** CloneInstances
 * \ingroup intAcl
 *
 * \desc            Clone the compiled acl instance structure.
 * 
 * \param[in]       value refer to the fm_fm10000CompiledAclInstance element
 *                  to clone.
 * 
 * \param[in]       funcArg refer to the cloned fm_fm10000CompiledAcls element.
 *
 * \return          The cloned portSet structure.
 *
 *****************************************************************************/
static void * CloneInstances(void *value, void *funcArg)
{
    fm_status   err;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_fm10000CompiledAcls *compiledClone = (fm_fm10000CompiledAcls*)funcArg;
    fm_fm10000CompiledAclInstance *compiledAclInst = (fm_fm10000CompiledAclInstance *) value;
    fm_fm10000CompiledAclInstance *compiledAclInstClone;

    compiledAclInstClone = (fm_fm10000CompiledAclInstance *) fmAlloc(sizeof(fm_fm10000CompiledAclInstance));
    if (compiledAclInstClone == NULL)
    {
        return NULL;
    }
    fmTreeInit(&compiledAclInstClone->acl);
    
    compiledAclInstClone->mergedAcls = compiledAclInst->mergedAcls;
    compiledAclInstClone->numRules = compiledAclInst->numRules;
    compiledAclInstClone->sliceInfo = compiledAclInst->sliceInfo;

    for (fmTreeIterInit(&itAcl, &compiledAclInst->acl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        err = fmTreeFind(&compiledClone->ingressAcl, aclNumber, &nextValue);
        if (err != FM_OK)
        {
            fmFreeCompiledAclInstance(compiledAclInstClone);
            return NULL;
        }

        err = fmTreeInsert(&compiledAclInstClone->acl,
                           aclNumber,
                           nextValue);
        if (err != FM_OK)
        {
            fmFreeCompiledAclInstance(compiledAclInstClone);
            return NULL;
        }

    }   /* end for (fmTreeIterInit(&itAcl, &compiledAclInst->acl) ... */

    if (err != FM_ERR_NO_MORE)
    {
        fmFreeCompiledAclInstance(compiledAclInstClone);
        return NULL;
    }

    return compiledAclInstClone;

}   /* end CloneInstances */


/*****************************************************************************/
/** ClonePortSetId
 * \ingroup intAcl
 *
 * \desc            Clone the aclPortSet structure.
 * 
 * \param[in]       value refer to the fm_portSet element to clone.
 * 
 * \param[in]       funcArg in not used.
 *
 * \return          The cloned portSet structure.
 *
 *****************************************************************************/
static void * ClonePortSetId(void *value, void *funcArg)
{
    fm_status   err;
    fm_portSet *portSet = (fm_portSet *) value;
    fm_portSet *portSetClone;

    FM_NOT_USED(funcArg);

    portSetClone = (fm_portSet *)fmAlloc(sizeof(fm_portSet));
    if (portSetClone == NULL)
    {
        return NULL;
    }

    FM_CLEAR(*portSetClone);

    err = fmCreateBitArray(&portSetClone->associatedPorts,
                           portSet->associatedPorts.bitCount);
    if(err != FM_OK)
    {
        fmFree(portSetClone);
        return NULL;
    }

    /* Initialize the portSet with the value defined in
     * the portSet entry found. */
    fmCopyBitArray(&portSetClone->associatedPorts,
                   &portSet->associatedPorts);

    portSetClone->mappedValue = portSet->mappedValue;

    return portSetClone;

}   /* end ClonePortSetId */




/*****************************************************************************/
/** ClonePolicersEntries
 * \ingroup intAcl
 *
 * \desc            Clone a compiled policer entry structure.
 *
 * \param[in]       value refer to the fm_fm10000CompiledPolicerEntry element
 *                  to clone.
 * 
 * \param[in]       funcArg in not used.
 *
 * \return          The cloned compiled policer entry structure.
 *
 *****************************************************************************/
static void * ClonePolicersEntries(void *value, void *funcArg)
{
    fm_status err;
    fm_dlist_node* node;
    fm_fm10000AclRule* aclRule;
    fm_fm10000AclRule* aclRuleClone;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry = (fm_fm10000CompiledPolicerEntry *) value;
    fm_fm10000CompiledPolicerEntry *compiledPolEntryClone;

    FM_NOT_USED(funcArg);

    compiledPolEntryClone = (fm_fm10000CompiledPolicerEntry *) fmAlloc(sizeof(fm_fm10000CompiledPolicerEntry));
    if (compiledPolEntryClone == NULL)
    {
        return NULL;
    }
    compiledPolEntryClone->countEntry = compiledPolEntry->countEntry;
    compiledPolEntryClone->policerId = compiledPolEntry->policerId;
    compiledPolEntryClone->committed = compiledPolEntry->committed;
    compiledPolEntryClone->excess = compiledPolEntry->excess;

    fmDListInit(&compiledPolEntryClone->policerRules);

    node = FM_DLL_GET_FIRST( (&compiledPolEntry->policerRules), head );
    while (node != NULL)
    {
        aclRule = (fm_fm10000AclRule *) node->data;
        aclRuleClone = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
        if (aclRuleClone == NULL)
        {
            fmFreeCompiledPolicerEntry(compiledPolEntryClone);
            return NULL;
        }
        FM_CLEAR(*aclRuleClone);

        aclRuleClone->aclNumber = aclRule->aclNumber;
        aclRuleClone->ruleNumber = aclRule->ruleNumber;
        err = fmDListInsertEnd(&compiledPolEntryClone->policerRules,
                               (void*) aclRuleClone);
        if (err != FM_OK)
        {
            fmFree(aclRuleClone);
            fmFreeCompiledPolicerEntry(compiledPolEntryClone);
            return NULL;
        }

        node = FM_DLL_GET_NEXT(node, next);
    }

    return compiledPolEntryClone;

}   /* end ClonePolicersEntries */




/*****************************************************************************/
/** ClonePolicersAcls
 * \ingroup intAcl
 *
 * \desc            Clone a policerAcls alement.
 *
 * \param[in]       value refer to the referenced acl element to clone.
 * 
 * \param[in]       funcArg in not used.
 *
 * \return          The cloned aclNumElement element.
 *
 *****************************************************************************/
static void * ClonePolicersAcls(void *value, void *funcArg)
{
    fm_int *aclNumElement = (fm_int *) value;
    fm_int *aclNumElementClone;

    FM_NOT_USED(funcArg);

    aclNumElementClone = (fm_int *) fmAlloc(sizeof(fm_int));
    if (aclNumElementClone == NULL)
    {
        return NULL;
    }
    *aclNumElementClone = *aclNumElement;

    return aclNumElementClone;

}   /* end ClonePolicersAcls */




/*****************************************************************************/
/** CloneCompiledAclRule
 * \ingroup intAcl
 *
 * \desc            Clone a compiledAclRule structure.
 *
 * \param[in]       value refer to the fm_fm10000CompiledAclRule element to
 *                  clone.
 * 
 * \param[in]       funcArg in not used.
 *
 * \return          The cloned compiledAclRule structure.
 *
 *****************************************************************************/
static void * CloneCompiledAclRule(void *value, void *funcArg)
{
    fm_fm10000CompiledAclRule *compiledAclRule = (fm_fm10000CompiledAclRule*)value;
    fm_fm10000CompiledAclRule* compiledAclRuleClone;

    FM_NOT_USED(funcArg);

    compiledAclRuleClone = fmAlloc(sizeof(fm_fm10000CompiledAclRule));

    if (compiledAclRuleClone == NULL)
    {
        return NULL;
    }

    FM_CLEAR(*compiledAclRuleClone);

    compiledAclRuleClone->aclNumber = compiledAclRule->aclNumber;
    FM_MEMCPY_S( compiledAclRuleClone->actions,
                 sizeof(compiledAclRuleClone->actions),
                 compiledAclRule->actions,
                 sizeof(compiledAclRuleClone->actions));
    compiledAclRuleClone->egressDropActions = compiledAclRule->egressDropActions;
    compiledAclRuleClone->numActions = compiledAclRule->numActions;
    compiledAclRuleClone->physicalPos = compiledAclRule->physicalPos;
    compiledAclRuleClone->cntAdjustPkts = compiledAclRule->cntAdjustPkts;
    compiledAclRuleClone->cntAdjustOctets = compiledAclRule->cntAdjustOctets;
    FM_MEMCPY_S( compiledAclRuleClone->policerIndex,
                 sizeof(compiledAclRuleClone->policerIndex),
                 compiledAclRule->policerIndex,
                 sizeof(compiledAclRuleClone->policerIndex));
    compiledAclRuleClone->ruleNumber = compiledAclRule->ruleNumber;
    FM_MEMCPY_S( compiledAclRuleClone->sliceKey,
                 sizeof(compiledAclRuleClone->sliceKey),
                 compiledAclRule->sliceKey,
                 sizeof(compiledAclRuleClone->sliceKey));
    compiledAclRuleClone->valid = compiledAclRule->valid;
    compiledAclRuleClone->portSetId = compiledAclRule->portSetId;

    return compiledAclRuleClone;

}   /* end CloneCompiledAclRule */




/*****************************************************************************/
/** CloneCompiledAcl
 * \ingroup intAcl
 *
 * \desc            Clone a compiledAcl structure.
 *
 * \param[in]       value refer to the fm_fm10000CompiledAcl element to clone.
 * 
 * \param[in]       funcArg refer to the already cloned fm_fm10000CompiledAcls
 *                  structure.
 *
 * \return          The cloned compiledAcl structure.
 *
 *****************************************************************************/
static void * CloneCompiledAcl(void *value, void *funcArg)
{
    fm_fm10000CompiledAcls *compiledClone = (fm_fm10000CompiledAcls*)funcArg;
    fm_fm10000CompiledAcl * compiledAcl = (fm_fm10000CompiledAcl*)value;
    fm_fm10000CompiledAcl* compiledAclClone;
    fm_treeIterator itPort;
    void *nextValue;
    fm_uint64 portNumber;
    fm_status err;

    compiledAclClone = fmAlloc(sizeof(fm_fm10000CompiledAcl));
    if (compiledAclClone == NULL)
    {
        return NULL;
    }

    FM_CLEAR(*compiledAclClone);

    compiledAclClone->aclNum = compiledAcl->aclNum;
    compiledAclClone->aclParts = compiledAcl->aclParts;
    compiledAclClone->firstAclPart = compiledAcl->firstAclPart;
    compiledAclClone->aclInstance = compiledAcl->aclInstance;
    compiledAclClone->aclKeepUnusedKeys = compiledAcl->aclKeepUnusedKeys;
    compiledAclClone->internal = compiledAcl->internal;
    compiledAclClone->chunk = compiledAcl->chunk;
    compiledAclClone->numChunk = compiledAcl->numChunk;
    compiledAclClone->numRules = compiledAcl->numRules;
    compiledAclClone->sliceInfo = compiledAcl->sliceInfo;

    FM_MEMCPY_S( compiledAclClone->muxSelect,
                 sizeof(compiledAclClone->muxSelect),
                 compiledAcl->muxSelect,
                 sizeof(compiledAcl->muxSelect));

    FM_MEMCPY_S( compiledAclClone->muxUsed,
                 sizeof(compiledAclClone->muxUsed),
                 compiledAcl->muxUsed,
                 sizeof(compiledAcl->muxUsed));

    FM_MEMCPY_S( compiledAclClone->caseLocation,
                 sizeof(compiledAclClone->caseLocation),
                 compiledAcl->caseLocation,
                 sizeof(compiledAcl->caseLocation));

    compiledAclClone->sliceInfo.selects = compiledAclClone->muxSelect;
    compiledAclClone->sliceInfo.caseLocation = compiledAclClone->caseLocation;

    fmTreeInit(&compiledAclClone->rules);

    /* Clone each rules */
    err = fmTreeClone(&compiledAcl->rules,
                      &compiledAclClone->rules,
                      CloneCompiledAclRule,
                      NULL);
    if (err != FM_OK)
    {
        fmFreeCompiledAcl(compiledAclClone);
        return NULL;
    }

    /* Clone the portSet tree */
    if (compiledAclClone->aclParts == 0)
    {
        compiledAclClone->portSetId = fmAlloc(sizeof(fm_tree));
        if (compiledAclClone->portSetId == NULL)
        {
            fmFreeCompiledAcl(compiledAclClone);
            return NULL;
        }
        FM_CLEAR(*compiledAclClone->portSetId);

        fmTreeInit(compiledAclClone->portSetId);

        for (fmTreeIterInit(&itPort, compiledAcl->portSetId) ;
             (err = fmTreeIterNext(&itPort, &portNumber, &nextValue)) ==
                    FM_OK ; )
        {
            if (nextValue != NULL)
            {
                if (portNumber == FM10000_SPECIAL_PORT_PER_ACL_KEY)
                {
                    err = fmTreeFind(&compiledClone->portSetId,
                                     ((fm_uint64) FM10000_ACL_PORTSET_TYPE_GLOBAL << FM10000_ACL_PORTSET_TYPE_POS) |
                                     compiledAclClone->aclNum,
                                     &nextValue);
                    if (err != FM_OK)
                    {
                        fmFreeCompiledAcl(compiledAclClone);
                        return NULL;
                    }
                }
                else
                {
                    err = fmTreeFind(&compiledClone->portSetId, portNumber, &nextValue);
                    if (err != FM_OK)
                    {
                        fmFreeCompiledAcl(compiledAclClone);
                        return NULL;
                    }
                }
            }
            err = fmTreeInsert(compiledAclClone->portSetId, portNumber, nextValue);
            if (err != FM_OK)
            {
                fmFreeCompiledAcl(compiledAclClone);
                return NULL;
            }
        }   /* end for (fmTreeIterInit(&itPort, compiledAcl->portSetId)... */

        if (err != FM_ERR_NO_MORE)
        {
            fmFreeCompiledAcl(compiledAclClone);
            return NULL;
        }
    }

    return compiledAclClone;

}   /* end CloneCompiledAcl */


/*****************************************************************************/
/** CloneCompiledAcls
 * \ingroup intAcl
 *
 * \desc            Clone a compiledAcls structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cacls points to the compiledAcls structure to clone.
 *
 * \return          The cloned compiledAcls structure.
 *
 *****************************************************************************/
static fm_fm10000CompiledAcls* CloneCompiledAcls(fm_int                  sw,
                                                 fm_fm10000CompiledAcls *cacls)
{
    fm_fm10000CompiledAcls *compiledClone;
    fm_treeIterator itAcl;
    fm_treeIterator itMappers;
    fm_uint64 aclNumber;
    fm_uint64 mapperId;
    fm_uint64 computedMapperId;
    void *nextValue;
    fm_status err;
    fm_fm10000CompiledAcl* compiledAclClone;
    fm_fm10000CompiledAcl* compiledAclFirstParts;
    fm_int i;
    void *mapperClone;
    fm_int sizeOfMapper;

    compiledClone = (fm_fm10000CompiledAcls *)
        fmAlloc( sizeof(fm_fm10000CompiledAcls) );

    if (compiledClone == NULL)
    {
        return NULL;
    }

    FM_MEMSET_S( compiledClone,
                 sizeof(fm_fm10000CompiledAcls),
                 0,
                 sizeof(fm_fm10000CompiledAcls) );

    /* Basic Initialization of the compiledAcls structure */
    InitializeCompiledAcls(sw, compiledClone);

    compiledClone->actionValid = cacls->actionValid;
    compiledClone->compilerStats = cacls->compilerStats;
    compiledClone->sliceValid = cacls->sliceValid;
    compiledClone->valid = cacls->valid;
    compiledClone->usedPortSet = cacls->usedPortSet;
    compiledClone->prevSliceValid = cacls->prevSliceValid;
    compiledClone->prevActionValid = cacls->prevActionValid;

    /* Clone policersId */
    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        err = fmTreeClone(&cacls->policersId[i],
                          &compiledClone->policersId[i],
                          NULL,
                          NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Clone Policer Bank Configuration */
    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        compiledClone->policers[i].indexLastPolicer = cacls->policers[i].indexLastPolicer;
        compiledClone->policers[i].ingressColorSource = cacls->policers[i].ingressColorSource;
        compiledClone->policers[i].markDSCP = cacls->policers[i].markDSCP;
        compiledClone->policers[i].markSwitchPri = cacls->policers[i].markSwitchPri;

        err = fmTreeClone(&cacls->policers[i].acl,
                          &compiledClone->policers[i].acl,
                          ClonePolicersAcls,
                          NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmTreeClone(&cacls->policers[i].policerEntry,
                          &compiledClone->policers[i].policerEntry,
                          ClonePolicersEntries,
                          NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Clone portSetId */
    err = fmTreeClone(&cacls->portSetId,
                      &compiledClone->portSetId,
                      ClonePortSetId,
                      NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Clone ingressAcl */
    err = fmTreeClone(&cacls->ingressAcl,
                      &compiledClone->ingressAcl,
                      CloneCompiledAcl,
                      compiledClone);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Link the portSet of the subsequent ACL to the master one */
    for (fmTreeIterInit(&itAcl, &compiledClone->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAclClone = (fm_fm10000CompiledAcl *) nextValue;

        if (compiledAclClone->aclParts != 0)
        {
            err = fmTreeFind(&compiledClone->ingressAcl,
                             FM_ACL_GET_MASTER_KEY(compiledAclClone->aclNum),
                             (void*) &compiledAclFirstParts);
            if (err != FM_OK)
            {
                goto ABORT;
            }
            compiledAclClone->portSetId = compiledAclFirstParts->portSetId;
        }

    }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

    if (err != FM_ERR_NO_MORE)
    {
        goto ABORT;
    }

    /* Clone egressAcl */
    err = fmTreeClone(&cacls->egressAcl,
                      &compiledClone->egressAcl,
                      CloneCompiledAcl,
                      compiledClone);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Clone instances */
    err = fmTreeClone(&cacls->instance,
                      &compiledClone->instance,
                      CloneInstances,
                      compiledClone);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Clone ecmpGroups */
    err = fmTreeClone(&cacls->ecmpGroups,
                      &compiledClone->ecmpGroups,
                      CloneEcmpGroup,
                      NULL);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Clone mappers */
    for (fmTreeIterInit(&itMappers, &cacls->mappers) ;
         (err = fmTreeIterNext(&itMappers, &mapperId, &nextValue)) == FM_OK ; )
    {
        err = fmGetMapperKeyAndSize(sw,
                                    (mapperId >> FM_MAPPER_TYPE_KEY_POS) & FM_LITERAL_64(0xff),
                                    nextValue,
                                    &computedMapperId,
                                    &sizeOfMapper);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        mapperClone = fmAlloc(sizeOfMapper);
        if (mapperClone == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_MEMCPY_S(mapperClone, sizeOfMapper, nextValue, sizeOfMapper);

        err = fmTreeInsert(&compiledClone->mappers,
                           mapperId,
                           mapperClone);
        if (err != FM_OK)
        {
            fmFree(mapperClone);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }   /* end for (fmTreeIterInit(&itMappers, &cacls->mappers) ... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    return compiledClone;

ABORT:

    FreeCompiledAclsStruct(compiledClone);
    return NULL;

}   /* end CloneCompiledAcls */


/*****************************************************************************/
/** FoundFree8BitsMux
 * \ingroup intAcl
 *
 * \desc            Find a free mux position for this specific abstract key
 *                  and configure this mux position accordingly.
 *
 * \param[in]       abstractKey refer to the abstract key to translate.
 *
 * \param[in,out]   muxSelect points to the structure that hold the current
 *                  mux configuration.
 * 
 * \param[in,out]   muxUsed points to the structure that hold the current
 *                  mux usage.
 *
 * \return          TRUE if free position found.
 *                  FALSE if free position not found.
 *
 *****************************************************************************/
static fm_bool FoundFree8BitsMux(fm_uint64  abstractKey,
                                 fm_byte   *muxSelect,
                                 fm_uint16 *muxUsed)
{
    fm_int i;

    /* Only process 8 bit key */
    for (i = 0 ; i < (FM_FFU_SELECTS_PER_MINSLICE - 1) ; i++)
    {
        if ((fmAbstractToConcrete8bits[abstractKey][i] != FM10000_INVALID_KEY) &&
            (muxSelect[i] == FM10000_UNUSED_KEY))
        {
            muxSelect[i] = fmAbstractToConcrete8bits[abstractKey][i];
            *muxUsed |= (0x3 << (i * 2));
            return TRUE;
        }
    }

    return FALSE;

}   /* end FoundFree8BitsMux */


/*****************************************************************************/
/** FoundExisting8BitsMux
 * \ingroup intAcl
 *
 * \desc            Find a previously configured mux position for this specific
 *                  abstract key and reserve this mux position accordingly.
 *
 * \param[in]       abstractKey refer to the abstract key to translate.
 *
 * \param[in]       muxSelect points to the structure that hold the current
 *                  mux configuration.
 * 
 * \param[in,out]   muxUsed points to the structure that hold the current
 *                  mux usage.
 *
 * \return          TRUE if free position found.
 *                  FALSE if free position not found.
 *
 *****************************************************************************/
static fm_bool FoundExisting8BitsMux(fm_uint64  abstractKey,
                                     fm_byte   *muxSelect,
                                     fm_uint16 *muxUsed)
{
    fm_int i;

    for (i = 0 ; i < FM_FFU_SELECTS_PER_MINSLICE ; i++)
    {
        if (fmAbstractToConcrete4bits[abstractKey][i * 2] == muxSelect[i])
        {
            *muxUsed |= (0x1 << (i * 2));
            return TRUE;
        }
        else if (fmAbstractToConcrete4bits[abstractKey][(i * 2) + 1] == muxSelect[i])
        {
            *muxUsed |= (0x1 << ((i * 2) + 1));
            return TRUE;
        }
    }

    return FALSE;

}   /* end FoundExisting8BitsMux */


/*****************************************************************************/
/** FoundFree4BitsMux
 * \ingroup intAcl
 *
 * \desc            Find a free mux position for this specific abstract key
 *                  and configure this mux position accordingly.
 *
 * \param[in]       abstractKey refer to the abstract key to translate.
 *
 * \param[in,out]   muxSelect points to the structure that hold the current
 *                  mux configuration.
 * 
 * \param[in,out]   muxUsed points to the structure that hold the current
 *                  mux usage.
 *
 * \return          TRUE if free position found.
 *                  FALSE if free position not found.
 *
 *****************************************************************************/
static fm_bool FoundFree4BitsMux(fm_uint64  abstractKey,
                                 fm_byte *  muxSelect,
                                 fm_uint16 *muxUsed)
{
    fm_int i;

    for (i = FM_FFU_SELECTS_PER_MINSLICE - 1 ; i >= 0 ; i--)
    {
        if ((fmAbstractToConcrete4bits[abstractKey][i * 2] != FM10000_INVALID_KEY) &&
            (muxSelect[i] == FM10000_UNUSED_KEY))
        {
            muxSelect[i] = fmAbstractToConcrete4bits[abstractKey][i * 2];
            *muxUsed |= (0x1 << (i * 2));
            return TRUE;
        }
        else if ((fmAbstractToConcrete4bits[abstractKey][(i * 2) + 1] != FM10000_INVALID_KEY) &&
                 (muxSelect[i] == FM10000_UNUSED_KEY))
        {
            muxSelect[i] = fmAbstractToConcrete4bits[abstractKey][(i * 2) + 1];
            *muxUsed |= (0x1 << ((i * 2) + 1));
            return TRUE;
        }
    }

    return FALSE;

}   /* end FoundFree4BitsMux */


/*****************************************************************************/
/** ConfigureConditionKeyForMux
 * \ingroup intAcl
 *
 * \desc            This function go over all the possible abstract key and
 *                  try to find the one already configured by the mux. If
 *                  a mux fit an abstract key that is also part of the
 *                  abstract key tree then its value and mask are configured
 *                  at the proper key and keyMask position.
 *
 * \param[in]       selectedMux refer to the mux configuration.
 *
 * \param[in]       muxPosition refer to the mux position [S0..S4].
 *                  This function assumes that the value of muxPosition is
 *                  in the range of 0-4 and will malfunction if this is
 *                  not true.
 *
 * \param[in]       abstractKey point to the filled abstract key containing
 *                  all the key to configure for this rule.
 *
 * \param[in,out]   key points to the 36 bits key value of the rule.
 *
 * \param[in,out]   keyMask points to the 36 bits key mask of the rule.
 * 
 * \return          Nothing.
 *
 *****************************************************************************/
static void ConfigureConditionKeyForMux(fm_byte     selectedMux,
                                        fm_byte     muxPosition,
                                        fm_tree *   abstractKey,
                                        fm_uint64 * key,
                                        fm_uint64 * keyMask)
{
    fm_int i;
    fm_uint64 value;
    fm_uint64 mask;
    void *nextValue;

    /* Only try to match 8bits key when the muxPosition to process is not the
     * top key. */
    if (muxPosition < 4)
    {
        /* Scan all the 8 bits abstract key. */
        for (i = FM10000_ABSTRACT_NONE ; i < FM10000_FIRST_4BITS_ABSTRACT_KEY ; i++)
        {
            /* Found a match of concrete-->abstract */
            if (fmAbstractToConcrete8bits[i][muxPosition] == selectedMux)
            {
                /* Is this abstract key is needed by this rule? */
                if (fmTreeFind(abstractKey, i, &nextValue) == FM_OK)
                {
                    /* Yes it is, configure the abstract value and mask to the
                     * key and keyMask at the proper position. */
                    value = (fm_uintptr)nextValue & 0xff;
                    mask  = ((fm_uintptr)nextValue & 0xff00) >> 8;
                    *key |= (value << (muxPosition * 8));
                    *keyMask |= (mask << (muxPosition * 8));
                }
            }
        }
    }
    /* Validate range of muxPosition to be between [0..4] */
    else if (muxPosition > 4)
    {
        FM_LOG_FATAL(FM_LOG_CAT_ACL, "Invalid muxPosition %d\n", muxPosition);
        return;
    }

    /* Scan all the 4 bits abstract key. */
    for (i = FM10000_FIRST_4BITS_ABSTRACT_KEY ; i < FM10000_NUM_ABSTRACT ; i++)
    {
        /* Every 4 bits key can be placed either in the top key or in one
         * of the other 8 bits key. When placed in a 8 bits key, this key only
         * occupy half of the whole key. */
        if (fmAbstractToConcrete4bits[i][muxPosition * 2] == selectedMux)
        {
            if (fmTreeFind(abstractKey, i, &nextValue) == FM_OK)
            {
                value = (fm_uintptr)nextValue & 0xf;
                mask  = ((fm_uintptr )nextValue & 0xf00) >> 8;
                *key |= (value << (muxPosition * 8));
                *keyMask |= (mask << (muxPosition * 8));
            }
        }
        else if (fmAbstractToConcrete4bits[i][(muxPosition * 2) + 1] == selectedMux)
        {
            if (fmTreeFind(abstractKey, i, &nextValue) == FM_OK)
            {
                value = (fm_uintptr)nextValue & 0xf;
                mask  = ((fm_uintptr)nextValue & 0xf00) >> 8;
                *key |= (value << ((muxPosition * 8) + 4));
                *keyMask |= (mask << ((muxPosition * 8) + 4));
            }
        }
    }

}   /* end ConfigureConditionKeyForMux */




/*****************************************************************************/
/** AllocatePortSetId
 * \ingroup intAcl
 *
 * \desc            This function assign a mapped portSet value for each
 *                  portSet previously defined.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in,out]   cacls points to the compiled acls structure to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AllocatePortSetId(fm_int                  sw,
                                   fm_aclErrorReporter *   errReport,
                                   fm_fm10000CompiledAcls *cacls)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm_treeIterator itAcl;
    fm_treeIterator itPort;
    void *nextValue;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_uint64 key;
    fm_portSet *portSet;
    fm_portSet *portSetEntry;
    fm_int i;
    fm_bool portSetLockTaken = FALSE;
    fm_bool firstPortSet = TRUE;
    fm_fm10000MapOwnerType owner;

    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &key, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        /* PortSet manipulation could only be done on the master ACL */
        if ((fmTreeSize(compiledAcl->portSetId) == 0) ||
            (compiledAcl->aclParts != 0))
        {
            continue;
        }

        /* Take ownership of the MAP_SRC resource */
        if (firstPortSet)
        {
            err = fm10000GetMapOwnership(sw,
                                         &owner,
                                         FM_FM10000_MAP_RESOURCE_SRC_PORT);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (owner == FM_FM10000_MAP_OWNER_NONE)
            {
                err = fm10000SetMapOwnership(sw,
                                             FM_FM10000_MAP_OWNER_ACL,
                                             FM_FM10000_MAP_RESOURCE_SRC_PORT);
            }
            else if (owner != FM_FM10000_MAP_OWNER_ACL)
            {
                err = FM_ERR_FFU_RES_OWNED;
            }

            if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            firstPortSet = FALSE;
        }

        /* This ACL is associated with a set of ports. */
        if (fmTreeFind(compiledAcl->portSetId,
                       FM10000_SPECIAL_PORT_PER_ACL_KEY,
                       &nextValue) == FM_OK)
        {
            /* Find empty slot in MAP_SRC */
            for (i = FM10000_ACL_PORTSET_PER_RULE_FIRST;
                 i < FM10000_ACL_PORTSET_PER_RULE_NUM;
                 i++)
            {
                /* If a global entry found, take it. */
                if (~(cacls->usedPortSet) & (1 << i))
                {
                    err = fmConvertAclPortToPortSet(sw,
                                                    &cacls->portSetId,
                                                    compiledAcl,
                                                    i,
                                                    FM10000_SPECIAL_PORT_PER_ACL_KEY);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Take the mapper position out of the free pool. */
                    cacls->usedPortSet |= (1 << i);
                    break;
                }
            }
            if (i == FM10000_ACL_PORTSET_PER_RULE_NUM)
            {
                err = FM_ERR_NO_PORT_SET;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            
        }
        else
        {
            /* Compute the number of portSet that are not already defined in
             * the global portSet tree. */
            for (fmTreeIterInit(&itPort, compiledAcl->portSetId) ;
                  (err = fmTreeIterNext(&itPort, &key, &nextValue)) == FM_OK ; )
            {
                if (fmTreeFind(&cacls->portSetId, key, &nextValue) == FM_OK)
                {
                    /* This portSet is already defined in the global tree so
                     * extract its portSet and update the ACL tree with this
                     * value. */
                    portSet = (fm_portSet *) nextValue;
                    err = fmTreeRemoveCertain(compiledAcl->portSetId,
                                              key,
                                              NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeInsert(compiledAcl->portSetId, key, portSet);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Reinitialize the iterator to the next key. */
                    err = fmTreeIterInitFromSuccessor(&itPort,
                                                      compiledAcl->portSetId,
                                                      key);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                }
            }
            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Now configure each portSet not available in the global tree to
             * use one of the remaining MAP_SRC bit. */
            for (fmTreeIterInit(&itPort, compiledAcl->portSetId) ;
                  (err = fmTreeIterNext(&itPort, &key, &nextValue)) == FM_OK ; )
            {
                if (fmTreeFind(&cacls->portSetId, key, &nextValue) ==
                    FM_ERR_NOT_FOUND)
                {
                    portSet = (fm_portSet *)fmAlloc(sizeof(fm_portSet));
                    if (portSet == NULL)
                    {
                        err = FM_ERR_NO_MEM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    FM_CLEAR(*portSet);

                    err = fmCreateBitArray(&portSet->associatedPorts,
                                           switchPtr->numCardinalPorts);
                    if(err != FM_OK)
                    {
                        fmFree(portSet);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Protect the software state of portSetTree and its entries */
                    TAKE_PORTSET_LOCK(sw);
                    portSetLockTaken = TRUE;

                    /* Is this portSet defined in the switch? */
                    if (fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                                   key & FM_PORTSET_MASK,
                                   &nextValue) == FM_OK)
                    {
                        portSetEntry = (fm_portSet *) nextValue;

                        /* Initialize the portSet with the value defined in
                         * the portSet entry found. */
                        fmCopyBitArray(&portSet->associatedPorts,
                                       &portSetEntry->associatedPorts);
                    }
                    else
                    {
                        fmDeleteBitArray(&portSet->associatedPorts);
                        fmFree(portSet);
                        fm10000FormatAclStatus(errReport, TRUE,
                                               "portSet handle %lld not defined "
                                               "in the ACL subsystem\n",
                                               key);
                        err = FM_ERR_INVALID_ACL_RULE;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    DROP_PORTSET_LOCK(sw);
                    portSetLockTaken = FALSE;
                    
                    /* Insert this portSet in the global tree. */
                    err = fmTreeInsert(&cacls->portSetId, key, portSet);
                    if (err != FM_OK)
                    {
                        fmDeleteBitArray(&portSet->associatedPorts);
                        fmFree(portSet);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Assign this portSet to one of the free position. */
                    for (i = FM10000_ACL_PORTSET_PER_RULE_FIRST;
                         i < FM10000_ACL_PORTSET_PER_RULE_NUM;
                         i++)
                    {
                        if (~(cacls->usedPortSet) & (1 << i))
                        {
                            /* Reserve this position. */
                            cacls->usedPortSet |= (1 << i);
                            portSet->mappedValue = i;
                            break;
                        }
                    }
                    if (i == FM10000_ACL_PORTSET_PER_RULE_NUM)
                    {
                        err = FM_ERR_NO_PORT_SET;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    /* Update the value associated to this key in the portSet
                     * tree of this specific ACL. */
                    err = fmTreeRemoveCertain(compiledAcl->portSetId,
                                              key,
                                              NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    err = fmTreeInsert(compiledAcl->portSetId, key, portSet);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    /* Reinitialize the iterator at the same position. This
                     * needs to be done since the tree has been updated while
                     * being iterated. */
                    err = fmTreeIterInitFromSuccessor(&itPort,
                                                      compiledAcl->portSetId,
                                                      key);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

            }   /* end for (fmTreeIterInit(&itPort, ...) */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

    }   /* end for (fmTreeIterInit(&itAcl, ...) */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    err = FM_OK;

ABORT:

    if (portSetLockTaken)
    {
        DROP_PORTSET_LOCK(sw);
    }

    return err;

}   /* end AllocatePortSetId */


/*****************************************************************************/
/** FillCompileStats
 * \ingroup intAcl
 *
 * \desc            Fill the compilation statistics.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \return          None
 *
 *****************************************************************************/
static void FillCompileStats(fm_fm10000CompiledAcls *cacls)
{
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_status err = FM_OK;
    fm_int i;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_int lastSlice = -1;
    fm_int firstSlice = FM_FM10000_NUM_FFU_SLICES + 1;
    fm_uint32 numRulesChained = 0;

    FM_CLEAR(cacls->compilerStats);

    /* Update the policers stats */
    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        if(fmTreeSize(&cacls->policers[i].policerEntry))
        {
            cacls->compilerStats.policerBanksUsed++;
        }
    }

    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        /* Master ACL */
        if (compiledAcl->aclParts == 0)
        {
            numRulesChained = compiledAcl->numRules;
        }
        /* Subsequent ACLs */
        else
        {
            numRulesChained += compiledAcl->numRules;
        }

        if (numRulesChained > cacls->compilerStats.mostRulesUsed)
        {
            cacls->compilerStats.mostRulesUsed = numRulesChained;
            cacls->compilerStats.aclWithMostRules = compiledAcl->aclNum;
        }

        cacls->compilerStats.slicesUsed++;

        if (compiledAcl->sliceInfo.actionEnd > lastSlice)
        {
            lastSlice = compiledAcl->sliceInfo.actionEnd;
        }

        if (compiledAcl->sliceInfo.keyStart < firstSlice)
        {
            firstSlice = compiledAcl->sliceInfo.keyStart;
        }
    }

    if (lastSlice >= 0)
    {
        cacls->compilerStats.minSlicesIngress = (lastSlice - firstSlice + 1);
    }

    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        if (compiledAcl->numRules > cacls->compilerStats.mostRulesUsed)
        {
            cacls->compilerStats.mostRulesUsed = compiledAcl->numRules;
            cacls->compilerStats.aclWithMostRules = aclNumber;
        }

        cacls->compilerStats.minSlicesEgress = compiledAcl->sliceInfo.actionEnd -
                                               compiledAcl->sliceInfo.keyStart + 1;
        cacls->compilerStats.chunksUsed += compiledAcl->numChunk;
        cacls->compilerStats.slicesUsed++;

        if (compiledAcl->sliceInfo.actionEnd > lastSlice)
        {
            lastSlice = compiledAcl->sliceInfo.actionEnd;
        }

        if (compiledAcl->sliceInfo.keyStart < firstSlice)
        {
            firstSlice = compiledAcl->sliceInfo.keyStart;
        }
    }

    if (lastSlice >= 0)
    {
        cacls->compilerStats.minSlicesTotal = (lastSlice - firstSlice + 1);
        cacls->compilerStats.lastMinSliceUsed = lastSlice;
        cacls->compilerStats.firstMinSliceUsed = firstSlice;
    }

}   /* end FillCompileStats */




/*****************************************************************************/
/** FormatCompileStats
 * \ingroup intAcl
 *
 * \desc            Formats the compilation statistics.
 *
 * \param[in]       cstats point to the filled compilerStats struct.
 *
 * \param[in,out]   errReport is the object which accepts the status messages.
 *
 * \return          None
 *
 *****************************************************************************/
static void FormatCompileStats(fm_aclCompilerStats* cstats,
                               fm_aclErrorReporter* errReport)
{
    char                parenthetical[32];

    FM_CLEAR(parenthetical);

    if (cstats->aclWithMostRules >= 0)
    {
        FM_SNPRINTF_S(parenthetical, sizeof(parenthetical),
                      "(acl %d) ", cstats->aclWithMostRules);
    }

    /*
     * We generate the messages by calling FormatStatusMessage with
     * FALSE for the isError argument, since this is just status
     * information and not an error.
     */
    fm10000FormatAclStatus(errReport, FALSE,
                           "Used %u minslices for ingress ACLs.\n"
                           "Used %u minslices for egress ACLs.\n"
                           "Used %u minslices for all FFU ACLs.\n"
                           "Used %u as first minslice for all FFU ACLs.\n"
                           "Used %u as last minslice for all FFU ACLs.\n"
                           "Used %u (out of %u) counter banks.\n"
                           "Largest number of CAM lines used by an "
                           "ingress ACL %sis %u.\n"
                           "Used %u (out of %u) egress chunks.\n",
                           cstats->minSlicesIngress,
                           cstats->minSlicesEgress,
                           cstats->minSlicesTotal,
                           cstats->firstMinSliceUsed,
                           cstats->lastMinSliceUsed,
                           cstats->policerBanksUsed,
                           FM_FM10000_POLICER_BANK_MAX,
                           parenthetical,
                           cstats->mostRulesUsed,
                           cstats->chunksUsed,
                           FM10000_FFU_EGRESS_CHUNK_VALID_ENTRIES);

    if (cstats->aclsSkipped || cstats->rulesSkipped)
    {
        fm10000FormatAclStatus(errReport,
                               FALSE,
                               "Skipped %u unassigned ACL%s "
                               "and %u unassigned rule%s.\n",
                               cstats->aclsSkipped,
                               (cstats->aclsSkipped != 1) ? "s" : "",
                               cstats->rulesSkipped,
                               (cstats->rulesSkipped != 1) ? "s" : "");
    }

}   /* end FormatCompileStats */




/*****************************************************************************/
/** AddAclRouteElement
 * \ingroup intAcl
 *
 * \desc            This function populate the ecmpGroups tree with all the
 *                  rule that make use of the routing action.
 *
 * \param[in,out]   ecmpGroups points to the ECMP group tree.
 *
 * \param[in]       ecmpGroupId is the ECMP group id to insert into the tree.
 *
 * \param[in]       aclNumber is the ACL id linked to this ECMP group.
 *
 * \param[in]       ruleNumber is the ACL rule linked to this ECMP group.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddAclRouteElement(fm_tree* ecmpGroups,
                                    fm_int   ecmpGroupId,
                                    fm_int   aclNumber,
                                    fm_int   ruleNumber)
{
    fm_status err = FM_OK;
    fm_dlist* ecmpList;
    fm_fm10000AclRule* ecmpRule;

    err = fmTreeFind(ecmpGroups, ecmpGroupId, (void**) &ecmpList);

    /* Create the list */
    if (err == FM_ERR_NOT_FOUND)
    {
        ecmpList = (fm_dlist *) fmAlloc(sizeof(fm_dlist));
        if (ecmpList == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_CLEAR(*ecmpList);

        fmDListInit(ecmpList);
        err = fmTreeInsert(ecmpGroups,
                           ecmpGroupId,
                           (void*) ecmpList);
        if (err != FM_OK)
        {
            fmFree(ecmpList);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Add elements to the list */
    if (err == FM_OK)
    {
        ecmpRule = (fm_fm10000AclRule *) fmAlloc(sizeof(fm_fm10000AclRule));
        if (ecmpRule == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_CLEAR(*ecmpRule);

        ecmpRule->aclNumber = aclNumber;
        ecmpRule->ruleNumber = ruleNumber;

        err = fmDListInsertEnd(ecmpList,
                               (void*) ecmpRule);
        if (err != FM_OK)
        {
            fmFree(ecmpRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    else
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    return err;

}   /* end AddAclRouteElement */




/*****************************************************************************/
/** BreakCompiledAcl
 * \ingroup intAcl
 *
 * \desc            This function breaks ACLs with more than 1024 rules into
 *                  multiple ACLs that would be chained together.
 *
 * \param[in,out]   cacls points to the compiled ACL structure to modify.
 *
 * \param[in]       aclNumber is the key relative to the ACL that needs to be
 *                  fragments.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status BreakCompiledAcl(fm_fm10000CompiledAcls *cacls,
                                  fm_uint64 aclNumber)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAcl* compiledAclParts;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_int parts;
    fm_int numToMove;
    fm_treeIterator itRule;
    fm_uint64 ruleNumber;
    fm_int i;
    fm_bool firstAclPart = TRUE;

    err = fmTreeFind(&cacls->ingressAcl, aclNumber, (void**) &compiledAcl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    compiledAcl->firstAclPart = FALSE;

    /* Define the number of parts needed */
    parts = (compiledAcl->numRules - 1) / FM10000_MAX_RULE_PER_ACL_PART;

    for (; parts > 0 ; parts--)
    {
        compiledAclParts = fmAlloc(sizeof(fm_fm10000CompiledAcl));
        if (compiledAclParts == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_CLEAR(*compiledAclParts);

        compiledAclParts->aclParts = parts;

        /* Reinitialize the parts that start the group */
        if (firstAclPart)
        {
            compiledAclParts->firstAclPart = TRUE;
            firstAclPart = FALSE;
        }

        compiledAclParts->aclNum = compiledAcl->aclNum;
        compiledAclParts->aclKeepUnusedKeys = compiledAcl->aclKeepUnusedKeys;
        compiledAclParts->internal = compiledAcl->internal;
        compiledAclParts->aclInstance = compiledAcl->aclInstance;
        fmTreeInit(&compiledAclParts->rules);
        compiledAclParts->portSetId = compiledAcl->portSetId;
        compiledAclParts->numRules = 0;
        compiledAclParts->sliceInfo = compiledAcl->sliceInfo;

        FM_MEMCPY_S( compiledAclParts->muxSelect,
                     sizeof(compiledAclParts->muxSelect),
                     compiledAcl->muxSelect,
                     sizeof(compiledAcl->muxSelect));

        FM_MEMCPY_S( compiledAclParts->muxUsed,
                     sizeof(compiledAclParts->muxUsed),
                     compiledAcl->muxUsed,
                     sizeof(compiledAcl->muxUsed));

        FM_MEMCPY_S( compiledAclParts->caseLocation,
                     sizeof(compiledAclParts->caseLocation),
                     compiledAcl->caseLocation,
                     sizeof(compiledAcl->caseLocation));

        compiledAclParts->sliceInfo.selects = compiledAclParts->muxSelect;
        compiledAclParts->sliceInfo.caseLocation = compiledAclParts->caseLocation;

        err = fmTreeInsert(&cacls->ingressAcl,
                           aclNumber + parts,
                           compiledAclParts);
        if (err != FM_OK)
        {
            fmFree(compiledAclParts);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Distribute the excess rule into the subsequent ACLs */
        numToMove = compiledAcl->numRules % FM10000_MAX_RULE_PER_ACL_PART;
        if (!numToMove)
        {
            numToMove = FM10000_MAX_RULE_PER_ACL_PART;
        }

        for (i = 0 ; i < numToMove ; i++)
        {
            fmTreeIterInitBackwards(&itRule, &compiledAcl->rules);
            err = fmTreeIterNext(&itRule, &ruleNumber, (void**) &compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmTreeInsert(&compiledAclParts->rules,
                               ruleNumber,
                               compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmTreeRemoveCertain(&compiledAcl->rules, ruleNumber, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAcl->numRules--;
            compiledAclParts->numRules++;
        }
    }

ABORT:

    return err;

}   /* end BreakCompiledAcl */




/*****************************************************************************/
/** GetSliceUsage
 * \ingroup intAcl
 *
 * \desc            This function gets the current ACL slice usage
 *
 * \param[in]       cacls points to the compiled ACL structure to scan.
 *
 * \param[out]      firstSlice will be filled with the first slice ID currently
 *                  used in this compiled ACL structure.
 *
 * \param[out]      lastSlice will be filled with the last slice ID currently
 *                  used in this compiled ACL structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status GetSliceUsage(fm_fm10000CompiledAcls *cacls,
                               fm_int *firstSlice,
                               fm_int *lastSlice)
{
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;

    *firstSlice = FM10000_FFU_SLICE_VALID_ENTRIES + 1;
    *lastSlice = -1;

    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        if (compiledAcl->sliceInfo.keyStart < *firstSlice)
        {
            *firstSlice = compiledAcl->sliceInfo.keyStart;
        }

        if (compiledAcl->sliceInfo.actionEnd > *lastSlice)
        {
            *lastSlice = compiledAcl->sliceInfo.actionEnd;
        }
    }

    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        if (compiledAcl->sliceInfo.keyStart < *firstSlice)
        {
            *firstSlice = compiledAcl->sliceInfo.keyStart;
        }

        if (compiledAcl->sliceInfo.actionEnd > *lastSlice)
        {
            *lastSlice = compiledAcl->sliceInfo.actionEnd;
        }
    }

    if (*lastSlice > 0)
    {
        return FM_OK;
    }
    else
    {
        return FM_ERR_NO_ACLS;
    }

}   /* end GetSliceUsage */


/*****************************************************************************
 * Private Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmFreeCompiledAclRule
 * \ingroup intAcl
 *
 * \desc            Free a fm_fm10000CompiledAclRule structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeCompiledAclRule(void *value)
{
    fm_fm10000CompiledAclRule *compiledAclRule = (fm_fm10000CompiledAclRule*)value;
    if (compiledAclRule != NULL)
    {
        fmFree(value);
    }

}   /* end fmfreeCompiledAclRule */




/*****************************************************************************/
/** fmFreeCompiledAcl
 * \ingroup intAcl
 *
 * \desc            Free a fm_fm10000CompiledAcl structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeCompiledAcl(void *value)
{
    fm_fm10000CompiledAcl *compiledAcl = (fm_fm10000CompiledAcl*)value;
    if (compiledAcl != NULL)
    {
        if (compiledAcl->aclParts == 0 && compiledAcl->portSetId != NULL)
        {
            fmTreeDestroy(compiledAcl->portSetId, NULL);
            fmFree(compiledAcl->portSetId);
        }
        fmTreeDestroy(&compiledAcl->rules, fmFreeCompiledAclRule);
        fmFree(value);
    }

}   /* end fmfreeCompiledAcl */




/*****************************************************************************/
/** fmFreeCompiledAclInstance
 * \ingroup intAcl
 *
 * \desc            Free a fm_fm10000CompiledAclInstance structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeCompiledAclInstance(void *value)
{
    fm_fm10000CompiledAclInstance *compiledAclInst = (fm_fm10000CompiledAclInstance*)value;
    if (compiledAclInst != NULL)
    {
        fmTreeDestroy(&compiledAclInst->acl, NULL);
        fmFree(value);
    }

}   /* end fmFreeCompiledAclInstance */




/*****************************************************************************/
/** fmFreeAclPortSet
 * \ingroup intAcl
 *
 * \desc            Free a fm_portSet structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeAclPortSet(void *value)
{
    fm_portSet *aclPortSet = (fm_portSet*)value;
    if (aclPortSet != NULL)
    {
        fmDeleteBitArray(&aclPortSet->associatedPorts);
        fmFree(value);
    }

}   /* end fmFreeAclPortSet */




/*****************************************************************************/
/** fmFreeEcmpGroup
 * \ingroup intAcl
 *
 * \desc            Free a fm_dlist* structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeEcmpGroup(void *value)
{
    fm_dlist *ecmpGroup = (fm_dlist*)value;
    if (ecmpGroup != NULL)
    {
        fmDListFreeWithDestructor(ecmpGroup, fmFree);
        fmFree(value);
    }

}   /* end fmFreeEcmpGroup */




/*****************************************************************************/
/** fmFreeCompiledPolicerEntry
 * \ingroup intAcl
 *
 * \desc            Free a fm_fm10000CompiledPolicerEntry* structure.
 *
 * \param[in,out]   value points to the structure to free.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeCompiledPolicerEntry(void *value)
{
    fm_fm10000CompiledPolicerEntry *compiledPolEntry = (fm_fm10000CompiledPolicerEntry*)value;
    if (compiledPolEntry != NULL)
    {
        fmDListFreeWithDestructor(&compiledPolEntry->policerRules, fmFree);
        fmFree(value);
    }

}   /* end fmFreeCompiledPolicerEntry */




/*****************************************************************************/
/** fmAddAbstractKey
 * \ingroup intAcl
 *
 * \desc            Fill the abstract key tree with all the required key.
 *
 * \param[in,out]   abstractKey points to the abstract key tree to fill.
 *
 * \param[in]       firstAbstract is the first abstract key for this particular
 *                  condition. The mask associated with this key are the
 *                  lowest bitsPerKey significant bits.
 *
 * \param[in]       lastAbstract is the last abstract key for this particular
 *                  condition.
 *
 * \param[in]       bitsPerKey refer to the number of bits assigned to each
 *                  abstract key.
 *
 * \param[in]       mask refer to the masked bits of a particular condition.
 *                  The mask must be ~0 to add an abstract key to the tree.
 *
 * \param[in]       value refer to the matching data of a particular condition.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAddAbstractKey(fm_tree * abstractKey,
                           fm_byte   firstAbstract,
                           fm_byte   lastAbstract,
                           fm_byte   bitsPerKey,
                           fm_uint64 mask,
                           fm_uint64 value)
{
    fm_status err;
    fm_byte currentAbstract;
    fm_byte maskPerKey;

    if (bitsPerKey == FM10000_4BITS_ABSTRACT_KEY)
    {
        maskPerKey = 0xf;
    }
    else
    {
        maskPerKey = 0xff;
    }

    for (currentAbstract = firstAbstract;
         currentAbstract <= lastAbstract;
         currentAbstract++)
    {
        /* Is this 4 or 8 bits key needed? */
        if (mask & maskPerKey)
        {
            /* Keep the abstract key value and mask as node data. */
            err = fmTreeInsert(abstractKey,
                               currentAbstract,
                               (void*) ((fm_uintptr)((mask & maskPerKey) << 8) |
                                        (fm_uintptr)(value & maskPerKey)));
            if (err == FM_ERR_ALREADY_EXISTS)
            {
                err = FM_OK;
            }
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        }
        mask >>= bitsPerKey;
        value >>= bitsPerKey;
    }
    return FM_OK;

}   /* end fmAddAbstractKey */




/*****************************************************************************/
/** fmAddIpAbstractKey
 * \ingroup intAcl
 *
 * \desc            Fill the abstract key tree with all the IP required key.
 *
 * \param[in,out]   abstractKey points to the abstract key tree to fill.
 *
 * \param[in]       firstAbstract is the first IP abstract key for this
 *                  particular condition.
 *
 * \param[in]       mask refer to the masked bits of this IP condition.
 *                  The mask must be ~0 to add an abstract key to the tree.
 *
 * \param[in]       value refer to the matching data of this IP condition.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAddIpAbstractKey(fm_tree * abstractKey,
                             fm_byte   firstAbstract,
                             fm_ipAddr mask,
                             fm_ipAddr value)
{
    fm_status err;
    fm_byte currentAbstract;
    fm_int FourBytesBlocks;
    fm_int i;
    fm_int j;
    fm_uintptr maskWord;
    fm_uintptr valueWord;

    /* IPv6 address have four set of four bytes to process. */
    if (mask.isIPv6)
    {
        FourBytesBlocks = 4;
    }
    else
    {
        FourBytesBlocks = 1;
    }

    currentAbstract = firstAbstract;
    for (i = 0; i < FourBytesBlocks; i++)
    {
        /* The IP address is defined as network byte order,
         * not host order.
         */
        maskWord  = ntohl(mask.addr[i]);
        valueWord = ntohl(value.addr[i]);
        for (j = 0; j < 4 ; j++, currentAbstract++)
        {
            if (maskWord & 0xff)
            {
                err = fmTreeInsert(abstractKey,
                                   currentAbstract,
                                   (void*) (((maskWord & 0xff) << 8) |
                                           (valueWord & 0xff)));
                if (err == FM_ERR_ALREADY_EXISTS)
                {
                    err = FM_OK;
                }
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

            }
            maskWord >>= FM10000_8BITS_ABSTRACT_KEY;
            valueWord >>= FM10000_8BITS_ABSTRACT_KEY;
        }
    }
    return FM_OK;

}   /* end fmAddIpAbstractKey */




/*****************************************************************************/
/** fmAddDeepInsAbstractKey
 * \ingroup intAcl
 *
 * \desc            Fill the abstract key tree with all the required key.
 *
 * \param[in,out]   abstractKey points to the abstract key tree to fill.
 *
 * \param[in]       abstractTable points to the abstract sorted list of the
 *                  data channel used to store this specific deep inspection
 *                  condition.
 *
 * \param[in]       tableSize is the size of the abstractTable.
 *
 * \param[in]       mask refer to the masked bits of a particular condition.
 *                  The mask must be ~0 to add an abstract key to the tree.
 *
 * \param[in]       value refer to the matching data of a particular condition.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAddDeepInsAbstractKey(fm_tree * abstractKey,
                                  fm_byte * abstractTable,
                                  fm_byte   tableSize,
                                  fm_byte * mask,
                                  fm_byte * value)
{
    fm_status err;
    fm_int    i;

    for (i = 0 ; i < tableSize ; i++)
    {
        if (mask[i])
        {
            /* Keep the abstract key value and mask as node data. */
            err = fmTreeInsert(abstractKey,
                               abstractTable[i],
                               (void*) ((fm_uintptr)(mask[i] << 8) |
                                        (fm_uintptr)(value[i])));
            if (err == FM_ERR_ALREADY_EXISTS)
            {
                err = FM_OK;
            }
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        }
    }

    return FM_OK;

}   /* end fmAddDeepInsAbstractKey */




/*****************************************************************************/
/** fmFillAbstractPortSetKeyTree
 * \ingroup intAcl
 *
 * \desc            Add all the portSet related abstract key from this
 *                  particular rule to the acl abstract tree.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in]       rule points to the defined acl rule that can contain
 *                  a portSet condition that may needs to be translated.
 *
 * \param[in,out]   compiledAclRule points to the compiled acl structure to
 *                  update. If NULL, the abstractKey tree will be filled
 *                  with MAP_SRC key with no specific data.
 *
 * \param[in,out]   abstractKey points to the abstract key tree to fill.
 *
 * \param[in,out]   portSetId points to the portSetId tree that represent the
 *                  configured acl. This tree also contain the determined
 *                  mapped value of each portSet.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFillAbstractPortSetKeyTree(fm_aclErrorReporter *      errReport,
                                       fm_aclRule *               rule,
                                       fm_fm10000CompiledAclRule *compiledAclRule,
                                       fm_tree *                  abstractKey,
                                       fm_tree *                  portSetId)
{
    void *found;
    fm_aclCondition cond = rule->cond;
    fm_aclValue *value = &rule->value;
    fm_status err = FM_OK;
    fm_portSet *portSetEntry;

    FM_NOT_USED(errReport);

    /* All the rule will have the same portSet condition in that case. */
    if ( (portSetId != NULL) &&
         (fmTreeFind(portSetId, FM10000_SPECIAL_PORT_PER_ACL_KEY, &found) == FM_OK) )
    {
        if (compiledAclRule != NULL)
        {
            portSetEntry = (fm_portSet *) found;
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_MAP_SRC,
                                   FM10000_ABSTRACT_MAP_SRC,
                                   FM10000_4BITS_ABSTRACT_KEY,
                                   1 << portSetEntry->mappedValue,
                                   1 << portSetEntry->mappedValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_MAP_SRC,
                                   FM10000_ABSTRACT_MAP_SRC,
                                   FM10000_4BITS_ABSTRACT_KEY,
                                   0xf,
                                   0xf);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    /* Only the rule with the portSet condition will need this portSet key. */
    else
    {
        if ( ((cond & FM_ACL_MATCH_INGRESS_PORT_SET) != 0) &&
             (value->portSet != FM_PORT_SET_ALL) )
        {
            if ( (portSetId != NULL) && (compiledAclRule != NULL) )
            {
                err = fmTreeFind(portSetId,
                                 value->portSet & FM_PORTSET_MASK,
                                 &found);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                compiledAclRule->portSetId = value->portSet;
                portSetEntry = (fm_portSet *) found;
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_MAP_SRC,
                                       FM10000_ABSTRACT_MAP_SRC,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       1 << portSetEntry->mappedValue,
                                       1 << portSetEntry->mappedValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else
            {
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_MAP_SRC,
                                       FM10000_ABSTRACT_MAP_SRC,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       0xf,
                                       0xf);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

ABORT:

    return err;

}   /* end fmFillAbstractPortSetKeyTree */




/*****************************************************************************/
/** fmFillAbstractKeyTree
 * \ingroup intAcl
 *
 * \desc            Add all the required abstract key from this particular
 *                  rule to the acl abstract tree. The tree functionality
 *                  also make sure that no more then one occurence of each key
 *                  is present in the structure. This function is also
 *                  responsable to fill the acl portSetId tree.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  condition that needs to be translated in abstract key.
 *
 * \param[in,out]   abstractKey points to the abstract key tree to fill.
 *
 * \param[in,out]   portSetId points to the portSetId tree to fill.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFillAbstractKeyTree(fm_int               sw,
                                fm_aclErrorReporter *errReport,
                                fm_aclRule *         rule,
                                fm_tree *            abstractKey,
                                fm_tree *            portSetId)
{
    void *found;
    fm_aclCondition cond = rule->cond;
    fm_aclValue *value = &rule->value;
    fm_status err = FM_OK;
    fm_byte rxTag = 0;
    fm_byte rxTagMask = 0;
    fm_int  physicalPort;

    /* Key related to any port will be added independently */
    if ( (cond & FM_ACL_MATCH_INGRESS_PORT_SET) != 0 && err == FM_OK )
    {
        if (portSetId == NULL)
        {
            fm10000FormatAclStatus(errReport, TRUE,
                                   "FM_ACL_MATCH_INGRESS_PORT_SET acl rule "
                                   "condition can't be defined for egress "
                                   "acls.\n");
            return FM_ERR_UNSUPPORTED;
        }
        else if (fmTreeFind(portSetId, FM10000_SPECIAL_PORT_PER_ACL_KEY, &found) == FM_OK)
        {
            /* Don't let the user configure port set and port per ACL */
            fm10000FormatAclStatus(errReport, TRUE,
                                   "FM_ACL_MATCH_INGRESS_PORT_SET acl rule "
                                   "condition can't be defined if the acl is "
                                   "assigned to a set of ports.\n");
            return FM_ERR_UNSUPPORTED;
        }
        /* FM_PORT_SET_ALL act the same as not having this acl rule
         * condition defined. */
        else if (value->portSet != FM_PORT_SET_ALL)
        {
            /* Only add this portSet if it's not already here. */
            err = fmTreeInsert(portSetId,
                               value->portSet & FM_PORTSET_MASK,
                               NULL);
            if (err == FM_ERR_ALREADY_EXISTS)
            {
                err = FM_OK;
            }
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

        }
    }

    /* Return error if unsupported condition are entered. */
    if ( (cond & ~FM10000_ACL_SUP_COND) != 0 && err == FM_OK )
    {
        fm10000FormatAclStatus(errReport, TRUE,
                               "acl rule with unsupported condition entered.\n");
        err = FM_ERR_UNSUPPORTED;
    }

    /**************************************************
     * Add keys
     **************************************************/
    if ( (cond & FM_ACL_MATCH_SRC_PORT) != 0 && err == FM_OK )
    {
        /* This is a special case on SWAG configuration for switches that does
         * not map to this specific source port. In that case, the rule must
         * be inserted but can't hit. */
        if (value->logicalPort == FM_PORT_DROP)
        {
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_SRC_PORT_3_0,
                                   FM10000_ABSTRACT_SRC_PORT_7_4,
                                   FM10000_4BITS_ABSTRACT_KEY,
                                   0xff,
                                   0xff);
        }
        else
        {
            err = fmMapLogicalPortToPhysical(GET_SWITCH_PTR(sw),
                                             value->logicalPort,
                                             &physicalPort);

            if ( fmIsCardinalPort(sw, value->logicalPort) &&
                 (err == FM_OK) )
            {
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_SRC_PORT_3_0,
                                       FM10000_ABSTRACT_SRC_PORT_7_4,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       0xff,
                                       physicalPort);
            }
            else
            {
                fm10000FormatAclStatus(errReport, TRUE,
                                       "acl rule with invalid logical port.\n");
            }
        }
    }

    if ( (cond & FM_ACL_MATCH_SRC_MAC) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_SMAC_7_0,
                               FM10000_ABSTRACT_SMAC_47_40,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->srcMask,
                               value->src);
    }

    if ( (cond & FM_ACL_MATCH_DST_MAC) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_DMAC_7_0,
                               FM10000_ABSTRACT_DMAC_47_40,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->dstMask,
                               value->dst);
    }

    if ( (cond & FM_ACL_MATCH_ETHERTYPE) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_TYPE_7_0,
                               FM10000_ABSTRACT_TYPE_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->ethTypeMask,
                               value->ethType);
    }

    if ( (cond & FM_ACL_MATCH_VLAN) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_VID_3_0,
                               FM10000_ABSTRACT_VID_11_8,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->vlanIdMask & 0xfff,
                               value->vlanId);
    }

    if ( (cond & FM_ACL_MATCH_PRIORITY) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_VPRI,
                               FM10000_ABSTRACT_VPRI,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->vlanPriMask,
                               value->vlanPri);
    }

    if ( (cond & FM_ACL_MATCH_VLAN2) != 0 && err == FM_OK )
    {
        /* If both vlan2 and priority2 are set, merge them */
        if ((cond & FM_ACL_MATCH_PRIORITY2) != 0)
        {
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_VLAN_VPRI2_7_0,
                                   FM10000_ABSTRACT_VLAN_VPRI2_15_8,
                                   FM10000_8BITS_ABSTRACT_KEY,
                                   ((value->vlanPri2Mask & 0xf) << 12) | 
                                    (value->vlanId2Mask & 0xfff),
                                   ((value->vlanPri2 & 0xf) << 12) |
                                    (value->vlanId2 & 0xfff));
        }
        else
        {
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_VLAN_VPRI2_7_0,
                                   FM10000_ABSTRACT_VLAN_VPRI2_15_8,
                                   FM10000_8BITS_ABSTRACT_KEY,
                                   value->vlanId2Mask & 0xfff,
                                   value->vlanId2);
        }
    }
    else if ( (cond & FM_ACL_MATCH_PRIORITY2) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_VLAN_VPRI2_15_8,
                               FM10000_ABSTRACT_VLAN_VPRI2_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               (value->vlanPri2Mask & 0xf) << 4,
                               (value->vlanPri2 & 0xf) << 4);
    }

    if ( (cond & FM_ACL_MATCH_SRC_IP) != 0 && err == FM_OK )
    {
        err = fmAddIpAbstractKey(abstractKey,
                                 FM10000_ABSTRACT_SIP_7_0,
                                 value->srcIpMask,
                                 value->srcIp);
    }

    if ( (cond & FM_ACL_MATCH_DST_IP) != 0 && err == FM_OK )
    {
        err = fmAddIpAbstractKey(abstractKey,
                                 FM10000_ABSTRACT_DIP_7_0,
                                 value->dstIpMask,
                                 value->dstIp);
    }

    if ( (cond & FM_ACL_MATCH_TTL) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_TTL_3_0,
                               FM10000_ABSTRACT_TTL_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->ttlMask,
                               value->ttl);
    }

    if ( (cond & FM_ACL_MATCH_PROTOCOL) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_PROT_3_0,
                               FM10000_ABSTRACT_PROT_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->protocolMask,
                               value->protocol);
    }

    if ( (cond & FM_ACL_MATCH_DSCP) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_TOS_3_0,
                               FM10000_ABSTRACT_TOS_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->dscpMask << FM_DSCP_POS,
                               value->dscp << FM_DSCP_POS);
    }

    if ( (cond & FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_L4SRC_7_0,
                               FM10000_ABSTRACT_L4SRC_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->L4SrcMask,
                               value->L4SrcStart);
    }

    if ( (cond & FM_ACL_MATCH_L4_DST_PORT_WITH_MASK) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_L4DST_7_0,
                               FM10000_ABSTRACT_L4DST_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->L4DstMask,
                               value->L4DstStart);
    }

    /* TCP Flags share the same field as the ethertype. */
    if ( (cond & FM_ACL_MATCH_TCP_FLAGS) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_L4A_7_0,
                               FM10000_ABSTRACT_L4A_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->tcpFlagsMask,
                               value->tcpFlags);
    }

    if ( (cond & FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT) != 0 && err == FM_OK )
    {
        err = fmAddDeepInsAbstractKey(abstractKey,
                                      fmL4DeepInspection,
                                      FM10000_MAX_ACL_L4_DEEP_INSPECTION_BYTES,
                                      value->L4DeepInspectionExtMask,
                                      value->L4DeepInspectionExt);
    }

    if ((cond & FM_ACL_MATCH_SWITCH_PRIORITY) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_SWPRI,
                               FM10000_ABSTRACT_SWPRI,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->switchPriMask,
                               value->switchPri);
    }

    if ((cond & FM_ACL_MATCH_VLAN_TAG_TYPE) != 0 && err == FM_OK)
    {
        switch (value->vlanTag)
        {
            case FM_ACL_VLAN_TAG_TYPE_NONE:
                rxTagMask |= (1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT);
                break;

            case FM_ACL_VLAN_TAG_TYPE_STANDARD:
            case FM_ACL_VLAN_TAG_TYPE_VLAN1:
                rxTag     |= 1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT;
                rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT;
                break;

            case FM_ACL_VLAN_TAG_TYPE_VLAN2:
                rxTag     |= 1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
                rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
                break;

            case FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN1:
                rxTag     |=  1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT;
                rxTagMask |= (1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT);
                break;

            case FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN2:
                rxTag     |=  1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
                rxTagMask |= (1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT);
                break;

            case FM_ACL_VLAN_TAG_TYPE_VLAN1_VLAN2:
                rxTag     |= (1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT);
                rxTagMask |= (1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT);
                break;

            case FM_ACL_VLAN_TAG_TYPE_VLAN2_UNTAG:
                rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
        }
    }

    if ((cond & FM_ACL_MATCH_SOURCE_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_SRC,
                               FM10000_ABSTRACT_MAP_SRC,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedSourcePortMask,
                               value->mappedSourcePort);
    }

    if ((cond & FM_ACL_MATCH_PROTOCOL_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_PROT,
                               FM10000_ABSTRACT_MAP_PROT,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedProtocolMask,
                               value->mappedProtocol);
    }

    if ((cond & FM_ACL_MATCH_DST_MAC_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_DMAC,
                               FM10000_ABSTRACT_MAP_DMAC,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedDstMacMask,
                               value->mappedDstMac);
    }

    if ((cond & FM_ACL_MATCH_SRC_MAC_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_SMAC,
                               FM10000_ABSTRACT_MAP_SMAC,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedSrcMacMask,
                               value->mappedSrcMac);
    }

    if ((cond & FM_ACL_MATCH_ETH_TYPE_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_TYPE,
                               FM10000_ABSTRACT_MAP_TYPE,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedEthTypeMask,
                               value->mappedEthType);
    }

    if ((cond & FM_ACL_MATCH_IP_LENGTH_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_LENGTH,
                               FM10000_ABSTRACT_MAP_LENGTH,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedIpLengthMask,
                               value->mappedIpLength);
    }

    if ((cond & FM_ACL_MATCH_DST_IP_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_DIP,
                               FM10000_ABSTRACT_MAP_DIP,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedDstIpMask,
                               value->mappedDstIp);
    }

    if ((cond & FM_ACL_MATCH_SRC_IP_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_SIP,
                               FM10000_ABSTRACT_MAP_SIP,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->mappedSrcIpMask,
                               value->mappedSrcIp);
    }

    if ((cond & FM_ACL_MATCH_L4_SRC_PORT_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_L4SRC_7_0,
                               FM10000_ABSTRACT_MAP_L4SRC_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->mappedL4SrcPortMask,
                               value->mappedL4SrcPort);
    }

    if ((cond & FM_ACL_MATCH_L4_DST_PORT_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_L4DST_7_0,
                               FM10000_ABSTRACT_MAP_L4DST_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->mappedL4DstPortMask,
                               value->mappedL4DstPort);
    }

    if ((cond & FM_ACL_MATCH_VLAN_MAP) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_MAP_VLAN_VPRI1_7_0,
                               FM10000_ABSTRACT_MAP_VLAN_VPRI1_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->mappedVlanIdMask & 0xfff,
                               value->mappedVlanId);
    }

    if ( (cond & FM_ACL_MATCH_NON_IP_PAYLOAD) != 0 && err == FM_OK )
    {
        err = fmAddDeepInsAbstractKey(abstractKey,
                                      fmL2DeepInspection,
                                      FM10000_MAX_ACL_NON_IP_PAYLOAD_BYTES,
                                      value->nonIPPayloadMask,
                                      value->nonIPPayload);
    }

    if ( (cond & FM_ACL_MATCH_TOS) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_TOS_3_0,
                               FM10000_ABSTRACT_TOS_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->tosMask,
                               value->tos);
    }

    if ( (cond & FM_ACL_MATCH_ISL_FTYPE) != 0 && err == FM_OK )
    {
        if ( (value->fType != FM_ACL_ISL_FTYPE_NORMAL) &&
             (value->fType != FM_ACL_ISL_FTYPE_SPECIAL) )
        {
            err = FM_ERR_UNSUPPORTED;
        }
        else
        {
            err = fmAddAbstractKey(abstractKey,
                                   FM10000_ABSTRACT_FTYPE,
                                   FM10000_ABSTRACT_FTYPE,
                                   FM10000_4BITS_ABSTRACT_KEY,
                                   0xc,
                                   value->fType << 2);
        }
    }

    if ( (cond & FM_ACL_MATCH_ISL_USER) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_USER_3_0,
                               FM10000_ABSTRACT_USER_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->islUserMask,
                               value->islUser);
    }

    if ((cond & FM_ACL_MATCH_SRC_GLORT) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_SGLORT_7_0,
                               FM10000_ABSTRACT_SGLORT_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->srcGlortMask,
                               value->srcGlort);
    }

    if ((cond & FM_ACL_MATCH_DST_GLORT) != 0 && err == FM_OK)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_DGLORT_7_0,
                               FM10000_ABSTRACT_DGLORT_15_8,
                               FM10000_8BITS_ABSTRACT_KEY,
                               value->dstGlortMask,
                               value->dstGlort);
    }

    if ( (cond & FM_ACL_MATCH_FRAG) != 0 && err == FM_OK )
    {
        switch (value->fragType)
        {
            case FM_ACL_FRAG_COMPLETE:
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_IPMISC_3_0,
                                       FM10000_ABSTRACT_IPMISC_7_4,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT) |
                                       (1 << FM_FM10000_FFU_IP_MISC_MORE_FRAG_BIT),
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT));
                break;

            case FM_ACL_FRAG_HEAD:
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_IPMISC_3_0,
                                       FM10000_ABSTRACT_IPMISC_7_4,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT) |
                                       (1 << FM_FM10000_FFU_IP_MISC_MORE_FRAG_BIT),
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT) |
                                       (1 << FM_FM10000_FFU_IP_MISC_MORE_FRAG_BIT));
                break;

            case FM_ACL_FRAG_SUB:
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_IPMISC_3_0,
                                       FM10000_ABSTRACT_IPMISC_7_4,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT),
                                       0);
                break;

            case FM_ACL_FRAG_COMPLETE_OR_HEAD:
                err = fmAddAbstractKey(abstractKey,
                                       FM10000_ABSTRACT_IPMISC_3_0,
                                       FM10000_ABSTRACT_IPMISC_7_4,
                                       FM10000_4BITS_ABSTRACT_KEY,
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT),
                                       (1 << FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT));
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
        }
    }

    if ( (cond & FM_ACL_MATCH_FRAME_TYPE) != 0 && err == FM_OK )
    {
        switch (value->frameType)
        {
            case FM_ACL_FRAME_TYPE_NON_IP:
                rxTagMask |= (1 << FM_FM10000_FFU_RXTAG_IPV4_BIT) |
                             (1 << FM_FM10000_FFU_RXTAG_IPV6_BIT);
                break;

            case FM_ACL_FRAME_TYPE_IPV4:
                rxTag     |= 1 << FM_FM10000_FFU_RXTAG_IPV4_BIT;
                rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_IPV4_BIT;
                break;

            case FM_ACL_FRAME_TYPE_IPV6:
                rxTag     |= 1 << FM_FM10000_FFU_RXTAG_IPV6_BIT;
                rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_IPV6_BIT;
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
        }
    }

    if ( (cond & FM_ACL_MATCH_FLAGS) != 0 && err == FM_OK )
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_IPMISC_3_0,
                               FM10000_ABSTRACT_IPMISC_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               value->flagsMask,
                               value->flags);
    }

    /* Properly map the scenario flag (FM6000 centric) to FM10000 */
    if ( (cond & FM_ACL_MATCH_SCENARIO_FLAGS) != 0 && err == FM_OK )
    {
        /* FM_FM10000_FFU_RXTAG_FTAG_BIT Handling */
        if ( (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_ISL_TYPE_F56) ||
             (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_ISL_TYPE_F64) )
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_FTAG_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_FTAG_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_VLAN1_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_VLAN1_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_VLAN2_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_VLAN2_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_IPV4_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_IP_TYPE_IPV4)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_IPV4_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_IPV4_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_IPV6_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_IP_TYPE_IPV6)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_IPV6_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_IPV6_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_MPLS_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_MPLS_ENCAP)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_MPLS_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_MPLS_ENCAP_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_MPLS_BIT;
        }

        /* FM_FM10000_FFU_RXTAG_CUSTOM_BIT Handling */
        if (value->scenarioFlags & FM_ACL_SCENARIO_FLAG_CUSTOM_TAG)
        {
            rxTag |= 1 << FM_FM10000_FFU_RXTAG_CUSTOM_BIT;
        }
        if (value->scenarioFlagsMask & FM_ACL_SCENARIO_FLAG_CUSTOM_TAG_MASK)
        {
            rxTagMask |= 1 << FM_FM10000_FFU_RXTAG_CUSTOM_BIT;
        }
    }

    /* If one or more conditions are related to the RxTag */
    if (rxTag || rxTagMask)
    {
        err = fmAddAbstractKey(abstractKey,
                               FM10000_ABSTRACT_RXTAG_3_0,
                               FM10000_ABSTRACT_RXTAG_7_4,
                               FM10000_4BITS_ABSTRACT_KEY,
                               rxTagMask,
                               rxTag);
    }

    return err;

}   /* end fmFillAbstractKeyTree */




/*****************************************************************************/
/** fmCountConditionSliceUsage
 * \ingroup intAcl
 *
 * \desc            Count the number of slices needed for a specific set of
 *                  mux.
 *
 * \param[in]       muxSelect point to the structure to take action on.
 *
 * \return          Number of slices needed.
 *
 *****************************************************************************/
fm_int fmCountConditionSliceUsage(fm_byte *muxSelect)
{
    fm_int i;
    fm_int j;

    for (i = 1 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
    {
        for (j = 0 ; j < FM_FFU_SELECTS_PER_MINSLICE ; j++)
        {
            if ( muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] != FM10000_UNUSED_KEY )
            {
                break;
            }
        }

        if (j == FM_FFU_SELECTS_PER_MINSLICE)
        {
            break;
        }
    }
    return i;

}   /* end fmCountConditionSliceUsage */




/*****************************************************************************/
/** fmInitializeConcreteKey
 * \ingroup intAcl
 *
 * \desc            Initialize the concrete key structure
 *
 * \param[in]       muxSelect point to the structure to initialize.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmInitializeConcreteKey(fm_byte *muxSelect)
{
    /* Initialize all the Mux of this ACL to "unused" */
    FM_MEMSET_S(muxSelect,
                FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES,
                FM10000_UNUSED_KEY,
                FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES);

}   /* end fmInitializeConcreteKey */




/*****************************************************************************/
/** fmConvertAbstractToConcreteKey
 * \ingroup intAcl
 *
 * \desc            Convert the abstract key set into a concrete set of mux
 *                  that cover all the required key. The abstract key are
 *                  ordered by difficulty meaning that the most difficult
 *                  key will be processed first down to the easier one that
 *                  can be fit in almost any mux.
 *
 * \param[in]       abstractKeyTree point to the abstract key tree to convert.
 *
 * \param[in,out]   muxSelect points to the acl structure to fill
 *                  with the proper mux configuration.
 * 
 * \param[in,out]   muxUsed points to the acl structure to fill
 *                  with the proper mux usage.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmConvertAbstractToConcreteKey(fm_tree *   abstractKeyTree,
                                         fm_byte *   muxSelect,
                                         fm_uint16 * muxUsed)
{
    fm_treeIterator itKey;
    fm_uint64       abstractKey;
    void *          nextValue;
    fm_status       err;
    fm_int          i;

    /* Process all the abstract key that was previously inserted in the tree. */
    for (fmTreeIterInit(&itKey, abstractKeyTree) ;
         (err = fmTreeIterNext(&itKey, &abstractKey, &nextValue)) == FM_OK ; )
    {
        /* 8 bits key */
        if (abstractKey < FM10000_FIRST_4BITS_ABSTRACT_KEY)
        {
            /* Try to find a free position for this key and configure the mux
             * accordingly. */
            for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
            {
                if (FoundFree8BitsMux(abstractKey,
                                      &muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE],
                                      &muxUsed[i]))
                {
                    break;
                }
            }
        }
        /* 4 bits key */
        else
        {
            /* If the abstract 4 bits key needed is already configure as part
             * of a 8 bits key then no need to add another key. We may needs
             * to set the proper 4 bits usage. */
            for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
            {
                if (FoundExisting8BitsMux(abstractKey,
                                          &muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE],
                                          &muxUsed[i]))
                {
                    break;
                }
            }

            /* This abstract 4 bits key was not found as a subset of a 8 bits
             * key, we now need to try to find a free position for this key and
             * configure the mux accordingly. Note that the top position is prefer
             * against all the other 8 bits key. */
            if (i == FM10000_FFU_SLICE_VALID_ENTRIES)
            {
                for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
                {
                    if (FoundFree4BitsMux(abstractKey,
                                          &muxSelect[i * FM_FFU_SELECTS_PER_MINSLICE],
                                          &muxUsed[i]))
                    {
                        break;
                    }
                }
            }
        }
        /* Did not find a free position for this abstract key. */
        if (i == FM10000_FFU_SLICE_VALID_ENTRIES)
        {
            err = FM_ERR_ACLS_TOO_BIG;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    err = FM_OK;

ABORT:

    return err;

}   /* end fmConvertAbstractToConcreteKey */




/*****************************************************************************/
/** fmCountActionSlicesNeeded
 * \ingroup intAcl
 *
 * \desc            Count the number of action slice needed for a particular
 *                  acl rule.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in]       rule point to the entered acl rule.
 *
 * \param[in]       actionSlices points to caller-allocated storage where this
 *                  function should store the number of action slices needed
 *                  for this particular acl rule.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCountActionSlicesNeeded(fm_int               sw,
                                    fm_aclErrorReporter *errReport,
                                    fm_aclRule *         rule,
                                    fm_int *             actionSlices)
{
    fm_int dataChannel = 0;
    fm_int tagChannel = 0;
    fm_status err;
    fm_tunnelGlortUser glortUser;

    /* FLAGS */
    if ( (rule->action & FM_ACL_ACTIONEXT_PERMIT) ||
         (rule->action & FM_ACL_ACTIONEXT_DENY) ||
         (rule->action & FM_ACL_ACTIONEXT_TRAP) ||
         (rule->action & FM_ACL_ACTIONEXT_LOG) ||
         (rule->action & FM_ACL_ACTIONEXT_NOROUTE) ||
         (rule->action & FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP) )
    {
        dataChannel++;
    }

    /* TRIG */
    if ( (rule->action & FM_ACL_ACTIONEXT_TRAP_ALWAYS) ||
         (rule->action & FM_ACL_ACTIONEXT_MIRROR_GRP) ||
         (rule->action & FM_ACL_ACTIONEXT_SET_TRIG_ID) )
    {
        dataChannel++;
    }

    /* USER */
    if ( (rule->action & FM_ACL_ACTIONEXT_SET_USER) )
    {
        dataChannel++;
    }

    /* ROUTE_ARP or ROUTE_GLORT */
    if ( (rule->action & FM_ACL_ACTIONEXT_REDIRECT) ||
         (rule->action & FM_ACL_ACTIONEXT_ROUTE) ||
         (rule->action & FM_ACL_ACTIONEXT_SET_FLOOD_DEST) ||
         (rule->action & FM_ACL_ACTIONEXT_LOAD_BALANCE) ||
         (rule->action & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL) )
    {
        if (rule->action & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL)
        {
            /* Redirecting to a tunnel group can either uses both GLORT and
             * USER or GLORT only. */
            err = fm10000GetTunnelAttribute(sw,
                                            rule->param.tunnelGroup,
                                            rule->param.tunnelRule,
                                            FM_TUNNEL_GLORT_USER,
                                            &glortUser);
            if (err != FM_OK)
            {
                return err;
            }

            /* If this tunnel group is set to uses both user and glort, the
             * rule can't define individual set user action. */
            if (glortUser.userMask != 0)
            {
                if (rule->action & FM_ACL_ACTIONEXT_SET_USER)
                {
                    return FM_ERR_INVALID_ACL_RULE;
                }
                dataChannel++;
            }
        }

        dataChannel++;
    }

    /* SET_VLAN or SET_PRI */
    if ( (rule->action & FM_ACL_ACTIONEXT_SET_VLAN) ||
         (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN) ||
         (rule->action & FM_ACL_ACTIONEXT_POP_VLAN) ||
         (rule->action & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY) ||
         (rule->action & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY) ||
         (rule->action & FM_ACL_ACTIONEXT_SET_DSCP) )
    {
        if ( ((rule->action & FM_ACL_ACTIONEXT_SET_VLAN) ||
              (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN) ||
              (rule->action & FM_ACL_ACTIONEXT_POP_VLAN)) &&
             (rule->action & FM_ACL_ACTIONEXT_SET_DSCP) )
        {
            dataChannel += 2;
        }
        else
        {
            /* Two FFU Actions are needed if both VlanPri and SwitchPri must
             * be set to different value */
            if ( (rule->action & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY) &&
                 (rule->action & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY) &&
                 (rule->param.vlanPriority != rule->param.switchPriority) )
            {
                dataChannel++;
            }
            dataChannel++;
        }
    }

    if ( (rule->action & FM_ACL_ACTIONEXT_COUNT) )
    {
        tagChannel++;
    }

    if ( (rule->action & FM_ACL_ACTIONEXT_POLICE) )
    {
        tagChannel++;
    }

    if ( (dataChannel == 0) && (tagChannel == 0) )
    {
        /* Acl rule with no action defined. This can be useful to prevent hit
         * on lower precedence rule. */
        *actionSlices = 1;
        return FM_OK;
    }
    else if (dataChannel > tagChannel)
    {
        *actionSlices = dataChannel;
        return FM_OK;
    }
    else
    {
        *actionSlices = tagChannel;
        return FM_OK;
    }

}   /* end fmCountActionSlicesNeeded */




/*****************************************************************************/
/** fmTranslateAclScenario
 * \ingroup intAcl
 *
 * \desc            Translate the ACL scenario into its FFU variant.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       aclScenario is a bitmask that refers to the ACL scenario to
 *                  match on.
 *
 * \param[in,out]   validScenarios points to the translated scenarios.
 * 
 * \param[in]       egressAcl must be set to TRUE if the scenario to translate
 *                  apply to an egress ACL or false otherwise.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmTranslateAclScenario(fm_int      sw,
                            fm_uint32   aclScenario,
                            fm_uint32  *validScenarios,
                            fm_bool     egressAcl)
{
    fm_uint32 frameType = 0;
    fm_uint32 routingType = 0;

    if (aclScenario & FM_ACL_SCENARIO_NONIP)
    {
        frameType |=
            FM_FFU_SCN_NOTIP_SW            |
            FM_FFU_SCN_NOTIP_SWGLORT       |
            FM_FFU_SCN_NOTIP_MGMT          |
            FM_FFU_SCN_NOTIP_SPECIAL       |
            FM_FFU_SCN_NOTIP_ROUTABLE      |
            FM_FFU_SCN_NOTIP_RTDGLORT      |
            FM_FFU_SCN_NOTIP_ROUTABLEMCAST |
            FM_FFU_SCN_NOTIP_RTDMCASTGLORT ;
    }

    if (aclScenario & FM_ACL_SCENARIO_IPv4)
    {
        frameType |=
            FM_FFU_SCN_IPv4_SW            |
            FM_FFU_SCN_IPv4_SWGLORT       |
            FM_FFU_SCN_IPv4_MGMT          |
            FM_FFU_SCN_IPv4_SPECIAL       |
            FM_FFU_SCN_IPv4_ROUTABLE      |
            FM_FFU_SCN_IPv4_RTDGLORT      |
            FM_FFU_SCN_IPv4_ROUTABLEMCAST |
            FM_FFU_SCN_IPv4_RTDMCASTGLORT ;
    }

    if (aclScenario & FM_ACL_SCENARIO_IPv6)
    {
        frameType |=
            FM_FFU_SCN_IPv6_SW                  |
            FM_FFU_SCN_IPv6_SWGLORT             |
            FM_FFU_SCN_IPv6_MGMT                |
            FM_FFU_SCN_IPv6_SPECIAL             |
            FM_FFU_SCN_IPv6_ROUTABLE            |
            FM_FFU_SCN_IPv6_RTDGLORT            |
            FM_FFU_SCN_IPv6_ROUTABLEMCAST       |
            FM_FFU_SCN_IPv6_RTDMCASTGLORT       |
            FM_FFU_SCN_IPv4INIPv6_SW            |
            FM_FFU_SCN_IPv4INIPv6_SWGLORT       |
            FM_FFU_SCN_IPv4INIPv6_MGMT          |
            FM_FFU_SCN_IPv4INIPv6_SPECIAL       |
            FM_FFU_SCN_IPv4INIPv6_ROUTABLE      |
            FM_FFU_SCN_IPv4INIPv6_RTDGLORT      |
            FM_FFU_SCN_IPv4INIPv6_ROUTABLEMCAST |
            FM_FFU_SCN_IPv4INIPv6_RTDMCASTGLORT ;
    }

    if (aclScenario & FM_ACL_SCENARIO_SWITCHED)
    {
        routingType |=
            FM_FFU_SCN_NOTIP_SW      |
            FM_FFU_SCN_IPv4_SW       |
            FM_FFU_SCN_IPv6_SW       |
            FM_FFU_SCN_IPv4INIPv6_SW ;
    }

    if (aclScenario & FM_ACL_SCENARIO_UNICAST_ROUTED)
    {
        routingType |=
            FM_FFU_SCN_NOTIP_ROUTABLE       |
            FM_FFU_SCN_IPv4_ROUTABLE        |
            FM_FFU_SCN_IPv6_ROUTABLE        |
            FM_FFU_SCN_IPv4INIPv6_ROUTABLE;
    }

    if (aclScenario & FM_ACL_SCENARIO_MULTICAST_ROUTED)
    {
        routingType |=
            FM_FFU_SCN_NOTIP_ROUTABLEMCAST      |
            FM_FFU_SCN_IPv4_ROUTABLEMCAST       |
            FM_FFU_SCN_IPv6_ROUTABLEMCAST       |
            FM_FFU_SCN_IPv4INIPv6_ROUTABLEMCAST ;
    }

    /* Egress ACL match on GLORT type by default to properly handle
       SWAG, PEP and TE. */
    if ( (aclScenario & FM_ACL_SCENARIO_SWITCHED_GLORT) ||
         (egressAcl && (aclScenario & FM_ACL_SCENARIO_SWITCHED)) )
    {
        routingType |=
            FM_FFU_SCN_NOTIP_SWGLORT      |
            FM_FFU_SCN_IPv4_SWGLORT       |
            FM_FFU_SCN_IPv6_SWGLORT       |
            FM_FFU_SCN_IPv4INIPv6_SWGLORT ;
    }

    if ( (aclScenario & FM_ACL_SCENARIO_UCAST_ROUTED_GLORT) ||
         (egressAcl && (aclScenario & FM_ACL_SCENARIO_UNICAST_ROUTED)) )
    {
        routingType |=
            FM_FFU_SCN_NOTIP_RTDGLORT   |
            FM_FFU_SCN_IPv4_RTDGLORT    |
            FM_FFU_SCN_IPv6_RTDGLORT    |
            FM_FFU_SCN_IPv4INIPv6_RTDGLORT;
    }

    if ( (aclScenario & FM_ACL_SCENARIO_MCAST_ROUTED_GLORT) ||
         (egressAcl && (aclScenario & FM_ACL_SCENARIO_MULTICAST_ROUTED)) )
    {
        routingType |=
            FM_FFU_SCN_NOTIP_RTDMCASTGLORT  |
            FM_FFU_SCN_IPv4_RTDMCASTGLORT   |
            FM_FFU_SCN_IPv6_RTDMCASTGLORT   |
            FM_FFU_SCN_IPv4INIPv6_RTDMCASTGLORT;
    }

    if (aclScenario & FM_ACL_SCENARIO_SPECIAL_GLORT)
    {
        routingType |=
            FM_FFU_SCN_NOTIP_SPECIAL      |
            FM_FFU_SCN_IPv4_SPECIAL       |
            FM_FFU_SCN_IPv6_SPECIAL       |
            FM_FFU_SCN_IPv4INIPv6_SPECIAL;
    }

    *validScenarios = (frameType & routingType);

}   /* end fmTranslateAclScenario */




/*****************************************************************************/
/** fmConfigureConditionKey
 * \ingroup intAcl
 *
 * \desc            This function fill all the key and keyMask of one specific
 *                  acl rule based on the acl rule condition.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  condition value and mask that must be configured.
 *
 * \param[in]       ruleNumber refer to the logical rule number of the rule
 *                  to configure.
 *
 * \param[in,out]   compiledAcl points to the compiled acl structure to
 *                  update.
 * 
 * \param[in]       compiledAclInst points to the compiled acl instance
 *                  structure if this ACL shares some resources with others.
 *                  NULL value can also be provided in case this ACL does not
 *                  provides instance.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmConfigureConditionKey(fm_int                         sw,
                                  fm_aclErrorReporter *          errReport,
                                  fm_aclRule *                   rule,
                                  fm_int                         ruleNumber,
                                  fm_fm10000CompiledAcl *        compiledAcl,
                                  fm_fm10000CompiledAclInstance *compiledAclInst)
{
    fm_status err = FM_OK;
    fm_int numConditionSlice;
    fm_fm10000CompiledAclRule *compiledAclRule;
    void *nextValue;
    fm_tree abstractKey;
    fm_int i;
    fm_byte caseValue = 0;
    fm_byte caseMask = 0;

    /* Compute the number of condition slices to configure. */
    numConditionSlice = compiledAcl->sliceInfo.keyEnd -
                        compiledAcl->sliceInfo.keyStart + 1;

    err = fmTreeFind(&compiledAcl->rules, ruleNumber, &nextValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    compiledAclRule = (fm_fm10000CompiledAclRule *) nextValue;

    fmTreeInit(&abstractKey);

    /* Fill the abstract key based on the requirement of this specific rule. */
    err = fmFillAbstractKeyTree(sw,
                                errReport,
                                rule,
                                &abstractKey,
                                compiledAcl->portSetId);
    if (err != FM_OK)
    {
        fmTreeDestroy(&abstractKey, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmFillAbstractPortSetKeyTree(errReport,
                                       rule,
                                       compiledAclRule,
                                       &abstractKey,
                                       compiledAcl->portSetId);
    if (err != FM_OK)
    {
        fmTreeDestroy(&abstractKey, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Case is shared among multiple mutually exclusive scenarios? */
    if ( (compiledAclInst != NULL) &&
         (compiledAclInst->mergedAcls == TRUE) )
    {
        caseValue = compiledAcl->sliceInfo.kase;
        caseMask  = 0xf;
    }

    for (i = 0 ; i < numConditionSlice ; i++)
    {
        /* By default, each acl rule don't use the case selection */
        compiledAclRule->sliceKey[i].kase.value = caseValue;
        compiledAclRule->sliceKey[i].kase.mask = caseMask;
        compiledAclRule->sliceKey[i].key = FM_LITERAL_64(0);
        compiledAclRule->sliceKey[i].keyMask = FM_LITERAL_64(0);

        if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4] != FM10000_UNUSED_KEY)
        {
            ConfigureConditionKeyForMux(compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4],
                                        4,
                                        &abstractKey,
                                        &compiledAclRule->sliceKey[i].key,
                                        &compiledAclRule->sliceKey[i].keyMask);
        }

        if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE)] != FM10000_UNUSED_KEY)
        {
            ConfigureConditionKeyForMux(compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE)],
                                        0,
                                        &abstractKey,
                                        &compiledAclRule->sliceKey[i].key,
                                        &compiledAclRule->sliceKey[i].keyMask);
        }

        if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1] != FM10000_UNUSED_KEY)
        {
            ConfigureConditionKeyForMux(compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 1],
                                        1,
                                        &abstractKey,
                                        &compiledAclRule->sliceKey[i].key,
                                        &compiledAclRule->sliceKey[i].keyMask);
        }

        if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2] != FM10000_UNUSED_KEY)
        {
            ConfigureConditionKeyForMux(compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 2],
                                        2,
                                        &abstractKey,
                                        &compiledAclRule->sliceKey[i].key,
                                        &compiledAclRule->sliceKey[i].keyMask);
        }

        if (compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3] != FM10000_UNUSED_KEY)
        {
            ConfigureConditionKeyForMux(compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 3],
                                        3,
                                        &abstractKey,
                                        &compiledAclRule->sliceKey[i].key,
                                        &compiledAclRule->sliceKey[i].keyMask);
        }
    }

    fmTreeDestroy(&abstractKey, NULL);

    return err;

}   /* end fmConfigureConditionKey */




/*****************************************************************************/
/** fmConfigureEgressActionData
 * \ingroup intAcl
 *
 * \desc            This function fill the action part of the compiled acl
 *                  to reflect the action configured into the acl rule.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  action that must be configured.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in,out]   compiledAclRule points to the compiled acl structure to
 *                  update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmConfigureEgressActionData(fm_aclRule *               rule,
                                      fm_aclErrorReporter *      errReport,
                                      fm_fm10000CompiledAclRule *compiledAclRule)
{
    fm_status err = FM_OK;
    fm_int ortoActions;
    fm_aclActionExt actionToSet;

    compiledAclRule->actions[0].action = FM_FFU_ACTION_NOP;
    compiledAclRule->actions[0].bank = 0;
    compiledAclRule->actions[0].counter = 0;
    compiledAclRule->actions[0].precedence = 0;

    /* Look for action group that must not be defined together. */
    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_PERMIT) |
                      (rule->action & FM_ACL_ACTIONEXT_DENY),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_PERMIT and "
                               "FM_ACL_ACTIONEXT_DENY together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    actionToSet = rule->action;

    compiledAclRule->egressDropActions = FALSE;
    if (actionToSet & FM_ACL_ACTIONEXT_PERMIT)
    {
        actionToSet &= ~FM_ACL_ACTIONEXT_PERMIT;
    }

    if (actionToSet & FM_ACL_ACTIONEXT_DENY)
    {
        compiledAclRule->egressDropActions = TRUE;
        actionToSet &= ~FM_ACL_ACTIONEXT_DENY;
    }

    if (actionToSet != 0)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "egress acl rule can't define other action than "
                               "FM_ACL_ACTIONEXT_PERMIT and FM_ACL_ACTIONEXT_DENY\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    return err;

}   /* end fmConfigureEgressActionData */




/*****************************************************************************/
/** fmConfigureActionData
 * \ingroup intAcl
 *
 * \desc            This function fill the action part of the compiled acl
 *                  to reflect the action configured into the acl rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   errReport is the object used to report compilation errors.
 *
 * \param[in]       policers points to the policer array.
 *
 * \param[in]       ecmpGroups points to the ECMP group tree.
 *
 * \param[in]       rule points to the defined acl rule that contain all the
 *                  action that must be configured.
 *
 * \param[in]       compiledAcl points to the compiled acl structure.
 *
 * \param[in,out]   compiledAclRule points to the compiled acl rule structure
 *                  to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmConfigureActionData(fm_int                      sw,
                                fm_aclErrorReporter *       errReport,
                                fm_fm10000CompiledPolicers *policers,
                                fm_tree *                   ecmpGroups,
                                fm_aclRule *                rule,
                                fm_fm10000CompiledAcl *     compiledAcl,
                                fm_fm10000CompiledAclRule * compiledAclRule)
{
    fm_int ortoActions;
    fm_int minAclPrecedence;
    fm_int maxAclPrecedence;
    fm_int ruleAclPrecedence;
    fm_int i;
    fm_int bank;
    fm_int counterIndex;
    fm_aclActionExt actionToSet;
    fm_uint32 glort;
    fm_int ecmpGroupId;
    fm_status err = FM_OK;
    fm_switch *switchPtr;
    fm10000_switch * switchExt;
    fm_intMulticastGroup *group;
    fm_ffuActionType currentActionType;
    fm_ffuActionData currentActionData;
    fm_int ffuResId;
    fm_int ffuResIdMask;
    fm_fm10000CompiledPolicerEntry* compiledPolEntry;
    void *nextValue;
    fm_int arpBaseIndex;
    fm_int arpBlockLength;
    fm_int pathCount;
    fm_int pathCountType;
    fm_tunnelGlortUser glortUser;
    fm_bool tunnelWithUser = FALSE;
    fm_int  lbgLogicalPort;
    fm_LBGMode lbgMode;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

#ifdef FM_SUPPORT_SWAG
    if (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "FM_ACL_ACTIONEXT_PUSH_VLAN is "
                               "unsupported in SWAG environment, "
                               "use FM_ACL_ACTIONEXT_SET_VLAN "
                               "instead\n");
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    if (rule->action & FM_ACL_ACTIONEXT_POP_VLAN)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "FM_ACL_ACTIONEXT_POP_VLAN is "
                               "unsupported in SWAG environment\n");
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
#endif

    /* Look for action group that must not be defined together. */
    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_PERMIT) |
                      (rule->action & FM_ACL_ACTIONEXT_DENY),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_PERMIT and "
                               "FM_ACL_ACTIONEXT_DENY together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_MIRROR_GRP) |
                      (rule->action & FM_ACL_ACTIONEXT_TRAP_ALWAYS) |
                      (rule->action & FM_ACL_ACTIONEXT_SET_TRIG_ID),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_MIRROR_GRP or "
                               "FM_ACL_ACTIONEXT_TRAP_ALWAYS or "
                               "FM_ACL_ACTIONEXT_SET_TRIG_ID together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_REDIRECT) |
                      (rule->action & FM_ACL_ACTIONEXT_LOAD_BALANCE) |
                      (rule->action & FM_ACL_ACTIONEXT_ROUTE) |
                      (rule->action & FM_ACL_ACTIONEXT_SET_FLOOD_DEST) |
                      (rule->action & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_REDIRECT or "
                               "FM_ACL_ACTIONEXT_LOAD_BALANCE or "
                               "FM_ACL_ACTIONEXT_ROUTE or "
                               "FM_ACL_ACTIONEXT_SET_FLOOD_DEST or "
                               "FM_ACL_ACTIONEXT_REDIRECT_TUNNEL together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN) |
                      (rule->action & FM_ACL_ACTIONEXT_POP_VLAN),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_PUSH_VLAN and "
                               "FM_ACL_ACTIONEXT_POP_VLAN together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    FM_COUNT_SET_BITS((rule->action & FM_ACL_ACTIONEXT_SET_VLAN) |
                      (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN),
                      ortoActions  );
    if (ortoActions > 1)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule can't define action "
                               "FM_ACL_ACTIONEXT_SET_VLAN and "
                               "FM_ACL_ACTIONEXT_PUSH_VLAN together\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if ( (rule->action & FM_ACL_ACTIONEXT_POP_VLAN) &&
        ((rule->action & FM_ACL_ACTIONEXT_SET_VLAN) == 0) )
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "acl rule action FM_ACL_ACTIONEXT_POP_VLAN "
                               "must always be paired with "
                               "FM_ACL_ACTIONEXT_SET_VLAN\n");
        err = FM_ERR_INVALID_ACL_RULE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    actionToSet = rule->action;

    /* Determine hardware precedence values to use. */
    minAclPrecedence = switchExt->aclPrecedenceMin;
    maxAclPrecedence = switchExt->aclPrecedenceMax;

    if ((rule->action & FM_ACL_ACTIONEXT_SET_PRECEDENCE))
    {
        if ( (rule->param.precedence != 0)
             && ( (minAclPrecedence > rule->param.precedence) ||
                  (maxAclPrecedence < rule->param.precedence) ) )
        {
            fm10000FormatAclStatus(errReport,
                                   TRUE,
                                   "acl rule action "
                                   "FM_ACL_ACTIONEXT_SET_PRECEDENCE "
                                   "with invalid precedence value\n");
            err = FM_ERR_INVALID_ACL_RULE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        ruleAclPrecedence = rule->param.precedence;
        actionToSet &= ~FM_ACL_ACTIONEXT_SET_PRECEDENCE;
    }
    else
    {
        /* Default Rule Precedence */
        ruleAclPrecedence = minAclPrecedence;
    }

    for (i = 0 ; i < compiledAclRule->numActions ; i++)
    {
        currentActionType = FM_FFU_ACTION_NOP;
        FM_CLEAR(currentActionData);
        bank = 0;
        counterIndex = 0;

        /* FLAGS */
        if (currentActionType == FM_FFU_ACTION_NOP)
        {
            if (actionToSet & FM_ACL_ACTIONEXT_PERMIT)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.drop = FM_FFU_FLAG_CLEAR;
                actionToSet &= ~FM_ACL_ACTIONEXT_PERMIT;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_DENY)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.drop = FM_FFU_FLAG_SET;
                actionToSet &= ~FM_ACL_ACTIONEXT_DENY;
            }

            if (actionToSet & FM_ACL_ACTIONEXT_TRAP)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.trap = FM_FFU_FLAG_SET;
                actionToSet &= ~FM_ACL_ACTIONEXT_TRAP;
            }

            if (actionToSet & FM_ACL_ACTIONEXT_LOG)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.log = FM_FFU_FLAG_SET;
                actionToSet &= ~FM_ACL_ACTIONEXT_LOG;
            }

            if (actionToSet & FM_ACL_ACTIONEXT_NOROUTE)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.noRoute = FM_FFU_FLAG_SET;
                actionToSet &= ~FM_ACL_ACTIONEXT_NOROUTE;
            }
    
            if (actionToSet & FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP)
            {
                currentActionType = FM_FFU_ACTION_SET_FLAGS;
                currentActionData.flags.captureTime = 
                         rule->param.captureEgressTimestamp ? FM_FFU_FLAG_SET :
                                                              FM_FFU_FLAG_CLEAR;
                actionToSet &= ~FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP;
            }

        }

        /* TRIG */
        if (currentActionType == FM_FFU_ACTION_NOP)
        {
            if (actionToSet & FM_ACL_ACTIONEXT_TRAP_ALWAYS)
            {
                err = fm10000GetAclTrapAlwaysId(sw, &ffuResId);
                if (err != FM_OK)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_TRAP_ALWAYS "
                                           "can't be defined\n");
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                currentActionType = FM_FFU_ACTION_SET_TRIGGER;

                currentActionData.trigger.value = ffuResId;
                currentActionData.trigger.mask = ffuResId;
                actionToSet &= ~FM_ACL_ACTIONEXT_TRAP_ALWAYS;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_MIRROR_GRP)
            {
                err = fm10000GetMirrorId(sw,
                                         rule->param.mirrorGrp,
                                         &ffuResId,
                                         &ffuResIdMask);
                if ( (err != FM_OK) ||
                     (ffuResId == FM10000_MIRROR_NO_FFU_RES) )
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_MIRROR_GRP "
                                           "with invalid Mirror Group\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                currentActionType = FM_FFU_ACTION_SET_TRIGGER;

                currentActionData.trigger.value = ffuResId;
                currentActionData.trigger.mask = ffuResIdMask;
                actionToSet &= ~FM_ACL_ACTIONEXT_MIRROR_GRP;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_SET_TRIG_ID)
            {
                currentActionType = FM_FFU_ACTION_SET_TRIGGER;

                currentActionData.trigger.value = rule->param.trigId;
                currentActionData.trigger.mask = rule->param.trigIdMask;
                actionToSet &= ~FM_ACL_ACTIONEXT_SET_TRIG_ID;
            }
        }

        /* USER */
        if (currentActionType == FM_FFU_ACTION_NOP)
        {
            if (actionToSet & FM_ACL_ACTIONEXT_SET_USER)
            {
                currentActionType = FM_FFU_ACTION_SET_USER;

                currentActionData.user.value = rule->param.user;
                currentActionData.user.mask = rule->param.userMask;
                actionToSet &= ~FM_ACL_ACTIONEXT_SET_USER;
            }
            else if ( (actionToSet & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL) &&
                      tunnelWithUser)
            {
                currentActionType = FM_FFU_ACTION_SET_USER;

                currentActionData.user.value = glortUser.user;
                currentActionData.user.mask = glortUser.userMask;
                actionToSet &= ~FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
            }
        }

        /* ROUTE_ARP or ROUTE_GLORT */
        if (currentActionType == FM_FFU_ACTION_NOP)
        {
            if ( (actionToSet & FM_ACL_ACTIONEXT_REDIRECT) ||
                 (actionToSet & FM_ACL_ACTIONEXT_SET_FLOOD_DEST) )
            {
                /* Redirecting to a L3 Mcast group must use the NEXTHOP table */
                err = fmTreeFind(&switchPtr->mcastPortTree,
                                 rule->param.logicalPort,
                                 (void **) &group);
                if ( (err == FM_OK) &&
                     (group->hasL3Resources == TRUE) &&
                     (actionToSet & FM_ACL_ACTIONEXT_REDIRECT) )
                {
                    /* Find ECMP group linked to this multicast logical port. */
                    ecmpGroupId = group->ecmpGroup;

                    err = fm10000GetECMPGroupArpInfo(sw,
                                                     ecmpGroupId,
                                                     NULL,
                                                     &arpBaseIndex,
                                                     &pathCount,
                                                     &pathCountType);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_REDIRECT or "
                                               "FM_ACL_ACTIONEXT_SET_FLOOD_DEST "
                                               "with invalid L3 MCast Group Id\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    currentActionType = FM_FFU_ACTION_ROUTE_ARP;
                    currentActionData.arp.arpIndex = arpBaseIndex;
                    currentActionData.arp.count = pathCount;
                    currentActionData.arp.arpType = pathCountType;

                    err = AddAclRouteElement(ecmpGroups,
                                             ecmpGroupId,
                                             compiledAclRule->aclNumber,
                                             compiledAclRule->ruleNumber);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                /* L2 Switch to this GLORT without any VLAN replication */
                else
                {
                    currentActionType = FM_FFU_ACTION_ROUTE_LOGICAL_PORT;
                    currentActionData.logicalPort = rule->param.logicalPort;

                    err = fmGetLogicalPortGlort(sw,
                                                rule->param.logicalPort,
                                                &glort);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_REDIRECT or "
                                               "FM_ACL_ACTIONEXT_SET_FLOOD_DEST "
                                               "with invalid logical port\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    if (actionToSet & FM_ACL_ACTIONEXT_SET_FLOOD_DEST)
                    {
                        currentActionType = FM_FFU_ACTION_ROUTE_FLOOD_DEST;
                    }
                }

                actionToSet &= ~(FM_ACL_ACTIONEXT_REDIRECT |
                                 FM_ACL_ACTIONEXT_SET_FLOOD_DEST);
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_LOAD_BALANCE)
            {
                err = fm10000GetLBGAttribute(sw, 
                                             rule->param.lbgNumber, 
                                             FM_LBG_GROUP_MODE, 
                                             &lbgMode);
                if (err != FM_OK)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_LOAD_BALANCE "
                                           "with invalid lbg number\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                if (lbgMode == FM_LBG_MODE_MAPPED_L234HASH)
                {
                    err = fm10000GetLBGAttribute(sw,
                                                 rule->param.lbgNumber,
                                                 FM_LBG_LOGICAL_PORT,
                                                 &lbgLogicalPort);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_LOAD_BALANCE "
                                               "with invalid lbg number\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    currentActionType = FM_FFU_ACTION_ROUTE_LOGICAL_PORT;
                    currentActionData.logicalPort = lbgLogicalPort;

                    err = fmGetLogicalPortGlort(sw,
                                                lbgLogicalPort,
                                                &glort);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_LOAD_BALANCE "
                                               "with invalid logical port found for lbg\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                }
                else
                {
                    /* LBG needs to extract nexthop information */
                    err = fm10000GetLBGInfo(sw,
                                            rule->param.lbgNumber,
                                            &arpBaseIndex,
                                            &arpBlockLength);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_LOAD_BALANCE "
                                               "with invalid lbg number\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    err = fm10000GetArpTablePathCountParameters(arpBlockLength,
                                                                &pathCount,
                                                                &pathCountType);
                    if (err != FM_OK)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_LOAD_BALANCE "
                                               "with invalid size\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }
                    currentActionType = FM_FFU_ACTION_ROUTE_ARP;
                    currentActionData.arp.arpIndex = arpBaseIndex;
                    currentActionData.arp.count = pathCount;
                    currentActionData.arp.arpType = pathCountType;
                }

                actionToSet &= ~FM_ACL_ACTIONEXT_LOAD_BALANCE;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_ROUTE)
            {
                currentActionType = FM_FFU_ACTION_ROUTE_ARP;

                ecmpGroupId = rule->param.groupId;

                err = fm10000GetECMPGroupArpInfo(sw,
                                                 ecmpGroupId,
                                                 NULL,
                                                 &arpBaseIndex,
                                                 &pathCount,
                                                 &pathCountType);
                if (err != FM_OK)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_ROUTE with "
                                           "invalid ECMP Group Id\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                err = AddAclRouteElement(ecmpGroups,
                                         ecmpGroupId,
                                         compiledAclRule->aclNumber,
                                         compiledAclRule->ruleNumber);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fm10000RegisterEcmpClient(sw,
                                                ecmpGroupId,
                                                FM10000_ECMP_GROUP_CLIENT_ACL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                currentActionType = FM_FFU_ACTION_ROUTE_ARP;
                currentActionData.arp.arpIndex = arpBaseIndex;
                currentActionData.arp.count = pathCount;
                currentActionData.arp.arpType = pathCountType;

                actionToSet &= ~FM_ACL_ACTIONEXT_ROUTE;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_REDIRECT_TUNNEL)
            {
                currentActionType = FM_FFU_ACTION_ROUTE_GLORT;
                
                err = fm10000GetTunnelAttribute(sw,
                                                rule->param.tunnelGroup,
                                                rule->param.tunnelRule,
                                                FM_TUNNEL_GLORT_USER,
                                                &glortUser);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                currentActionData.glort = glortUser.glort;

                /* Require two actions slice to cover glort and user */
                if (glortUser.userMask != 0)
                {
                    tunnelWithUser = TRUE;
                }
                else
                {
                    actionToSet &= ~FM_ACL_ACTIONEXT_REDIRECT_TUNNEL;
                }
            }
        }

        /* SET_VLAN or SET_PRI */
        if (currentActionType == FM_FFU_ACTION_NOP)
        {
            if ( (rule->action & FM_ACL_ACTIONEXT_SET_VLAN) ||
                 (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN) ||
                 (rule->action & FM_ACL_ACTIONEXT_POP_VLAN) )
            {
                currentActionType = FM_FFU_ACTION_SET_FIELDS;

                currentActionData.fields.fieldType = FM_FFU_FIELD_VLAN;

                if (rule->action & FM_ACL_ACTIONEXT_POP_VLAN)
                {
                    if (rule->param.vlan >= FM_MAX_VLAN)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_POP_VLAN "
                                               "with invalid vlan\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    currentActionData.fields.fieldValue = rule->param.vlan;
                    currentActionData.fields.txTag = FM_FFU_TXTAG_DEL_TAG;
                }
                else if (rule->action & FM_ACL_ACTIONEXT_PUSH_VLAN)
                {
                    if (rule->param.vlan >= FM_MAX_VLAN)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_PUSH_VLAN "
                                               "with invalid vlan\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    currentActionData.fields.fieldValue = rule->param.vlan;
                    currentActionData.fields.txTag = FM_FFU_TXTAG_ADD_TAG;
                }
                else if (rule->action & FM_ACL_ACTIONEXT_SET_VLAN)
                {
                    if (rule->param.vlan >= FM_MAX_VLAN)
                    {
                        fm10000FormatAclStatus(errReport,
                                               TRUE,
                                               "acl rule action "
                                               "FM_ACL_ACTIONEXT_SET_VLAN "
                                               "with invalid vlan\n");
                        err = FM_ERR_INVALID_ACL_PARAM;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    }

                    currentActionData.fields.fieldValue = rule->param.vlan;
                    currentActionData.fields.txTag = FM_FFU_TXTAG_NORMAL;
                }
                /* Should never go there */
                else
                {
                    err = FM_FAIL;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                actionToSet &= ~(FM_ACL_ACTIONEXT_SET_VLAN |
                                 FM_ACL_ACTIONEXT_PUSH_VLAN |
                                 FM_ACL_ACTIONEXT_POP_VLAN);
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_SET_DSCP)
            {
                if (rule->param.dscp >= FM_MAX_DSCP_PRIORITIES)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_SET_DSCP "
                                           "with invalid dscp\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                currentActionType = FM_FFU_ACTION_SET_FIELDS;

                currentActionData.fields.fieldType = FM_FFU_FIELD_DSCP;
                currentActionData.fields.fieldValue = rule->param.dscp;
                actionToSet &= ~FM_ACL_ACTIONEXT_SET_DSCP;
            }
            else if ( (actionToSet & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY) ||
                      (actionToSet & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY) )
            {
                currentActionType = FM_FFU_ACTION_SET_FIELDS;
                currentActionData.fields.fieldType = FM_FFU_FIELD_NEITHER;
            }

            if ( (actionToSet & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY) &&
                 (actionToSet & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY) &&
                 (rule->param.switchPriority == rule->param.vlanPriority) )
            {
                if (rule->param.vlanPriority >= FM_MAX_VLAN_PRIORITIES)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY "
                                           "with invalid vlan priority\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                currentActionData.fields.setPri = TRUE;
                currentActionData.fields.setVpri = TRUE;
                currentActionData.fields.priority = rule->param.vlanPriority;
                actionToSet &= ~(FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY |
                                 FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY);
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY)
            {
                if (rule->param.vlanPriority >= FM_MAX_VLAN_PRIORITIES)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY "
                                           "with invalid vlan priority\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                currentActionData.fields.setVpri = TRUE;
                currentActionData.fields.priority = rule->param.vlanPriority;
                actionToSet &= ~FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY;
            }
            else if (actionToSet & FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY)
            {
                if (rule->param.switchPriority >= FM_MAX_SWITCH_PRIORITIES)
                {
                    fm10000FormatAclStatus(errReport,
                                           TRUE,
                                           "acl rule action "
                                           "FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY "
                                           "with invalid switch priority\n");
                    err = FM_ERR_INVALID_ACL_PARAM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }

                currentActionData.fields.setPri = TRUE;
                currentActionData.fields.priority = rule->param.switchPriority;
                actionToSet &= ~FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY;
            }
        }

        /* Policer/Counter configuration */
        if (actionToSet & FM_ACL_ACTIONEXT_COUNT)
        {
            for (bank = 0 ; bank < FM_FM10000_POLICER_BANK_MAX ; bank++)
            {
                if (compiledAclRule->policerIndex[bank] != 0)
                {
                    err = fmTreeFind(&policers[bank].policerEntry,
                                     compiledAclRule->policerIndex[bank],
                                     &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                    if (compiledPolEntry->countEntry == TRUE)
                    {
                        counterIndex = compiledAclRule->policerIndex[bank];
                        actionToSet &= ~FM_ACL_ACTIONEXT_COUNT;
                        break;
                    }
                }
            }
            if (bank == FM_FM10000_POLICER_BANK_MAX)
            {
                fm10000FormatAclStatus(errReport,
                                       TRUE,
                                       "acl rule action "
                                       "FM_ACL_ACTIONEXT_COUNT failed\n");
                err = FM_ERR_INVALID_ACL_PARAM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
        else if (actionToSet & FM_ACL_ACTIONEXT_POLICE)
        {
            for (bank = 0 ; bank < FM_FM10000_POLICER_BANK_MAX ; bank++)
            {
                if (compiledAclRule->policerIndex[bank] != 0)
                {
                    err = fmTreeFind(&policers[bank].policerEntry,
                                     compiledAclRule->policerIndex[bank],
                                     &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                    if (compiledPolEntry->countEntry == FALSE)
                    {
                        counterIndex = compiledAclRule->policerIndex[bank];
                        actionToSet &= ~FM_ACL_ACTIONEXT_POLICE;
                        break;
                    }
                }
            }
            if (bank == FM_FM10000_POLICER_BANK_MAX)
            {
                fm10000FormatAclStatus(errReport,
                                       TRUE,
                                       "acl rule action "
                                       "FM_ACL_ACTIONEXT_POLICE failed\n");
                err = FM_ERR_INVALID_ACL_PARAM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        compiledAclRule->actions[i].action = currentActionType;
        compiledAclRule->actions[i].data = currentActionData;
        compiledAclRule->actions[i].bank = bank;
        compiledAclRule->actions[i].counter = counterIndex;

        /* This is to fix precedence issue on count action. In fact a counter
         * index equal 0 refer to no count action. In that case, the action
         * precedence must be lower than a real count action to make sure the 
         * counter index is not reset. This is why FM10000 only supports 4 
         * precedence level that are mapped to the real 8 precedence level by 
         * always reserving two values for each precedence (one for count and 
         * the second for no count). */ 
        if (counterIndex)
        {
            compiledAclRule->actions[i].precedence = (ruleAclPrecedence * 2) + 1;
        }
        else
        {
            compiledAclRule->actions[i].precedence = (ruleAclPrecedence * 2);
        }
    }

    if (actionToSet)
    {
        fm10000FormatAclStatus(errReport,
                               TRUE,
                               "one or more acl rule actions are not "
                               "supported\n");
        err = FM_ERR_INVALID_ACL_RULE;
    }

ABORT:

    return err;

}   /* end fmConfigureActionData */




/*****************************************************************************/
/** fmConvertAclPortToPortSet
 * \ingroup intAcl
 *
 * \desc            This function configure an internal portSet representation
 *                  based on the global ingress port configured for a specific
 *                  acl.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   globalPortSet points to the switch portSet representation
 *                  to update with this new portSet.
 *
 * \param[in,out]   compiledAcl points to compiled acl structure to update with
 *                  this new portSet.
 *
 * \param[in]       mappedValue refer to the predetermined mapped value of this
 *                  portSet.
 *
 * \param[in]       key refer to the key used to reach this portSet in the acl
 *                  portSet tree.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmConvertAclPortToPortSet(fm_int                 sw,
                                    fm_tree *              globalPortSet,
                                    fm_fm10000CompiledAcl *compiledAcl,
                                    fm_int                 mappedValue,
                                    fm_uint64              key)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    void *nextValue;
    fm_acl *acl;
    fm_portSet *portSet;
    fm_bool bitValue;
    fm_int cpi;
    fm_int port;

    /* Find the global acl structure that represent the compiled
     * version of this acl to extract the associated ports. */
    err = fmTreeFind(&switchPtr->aclInfo.acls,
                     compiledAcl->aclNum,
                     &nextValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    acl = (fm_acl *) nextValue;

    portSet = (fm_portSet *)fmAlloc(sizeof(fm_portSet));
    if (portSet == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    FM_CLEAR(*portSet);

    err = fmCreateBitArray(&portSet->associatedPorts,
                           switchPtr->numCardinalPorts);
    if (err != FM_OK)
    {
        fmFree(portSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Insert this new portSet to the global portSet tree of
     * the switch. */
    err = fmTreeInsert(globalPortSet,
                       ((fm_uint64) FM10000_ACL_PORTSET_TYPE_GLOBAL << FM10000_ACL_PORTSET_TYPE_POS) |
                       compiledAcl->aclNum,
                       portSet);
    if (err != FM_OK)
    {
        fmFree(portSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Extract the ingress port configured from this ACL and
     * configure the portSet accordingly. */
    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        err = fmGetAclAssociatedPort(sw,
                                     acl,
                                     port,
                                     FM_ACL_TYPE_INGRESS,
                                     &bitValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (bitValue)
        {
            err = fmSetPortInBitArray(sw,
                                      &portSet->associatedPorts,
                                      port,
                                      TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    portSet->mappedValue = mappedValue;

    /* Update the value associated to this key in the portSet
     * tree of this specific ACL. */
    err = fmTreeRemoveCertain(compiledAcl->portSetId,
                              key,
                              NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmTreeInsert(compiledAcl->portSetId,
                       key,
                       portSet);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

ABORT:

    return err;

}   /* end fmConvertAclPortToPortSet */




/*****************************************************************************/
/** fmUpdateValidSlice
 * \ingroup intAcl
 *
 * \desc            This function update the global slice validity information
 *                  of the system.
 *
 * \param[in]       sliceInfo is the structure that represent the current
 *                  slice utilization of this specific acl.
 *
 * \param[in,out]   cacls points to the compiled acls structure to update.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmUpdateValidSlice(fm_ffuSliceInfo *       sliceInfo,
                        fm_fm10000CompiledAcls *cacls)
{
    fm_int i;

    for (i = sliceInfo->keyStart ; i <= sliceInfo->keyEnd ; i++)
    {
        cacls->sliceValid |= 1 << i;
    }

    for (i = sliceInfo->keyEnd ; i <= sliceInfo->actionEnd ; i++)
    {
        cacls->actionValid |= 1 << i;
    }

}   /* end fmUpdateValidSlice */




/*****************************************************************************/
/** fmApplyMapSrcPortId
 * \ingroup intAcl
 *
 * \desc            This function apply the compiled portSet configuration
 *                  to the mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSetIdTree points to the tree filled with compiled
 *                  portSet to apply.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmApplyMapSrcPortId(fm_int    sw,
                              fm_tree * portSetIdTree)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm_status       err = FM_OK;
    fm_treeIterator itPortSet;
    fm_portSet *    portSetEntry;
    fm_fm10000MapSrcPortCfg mapSrcPortCfg[FM10000_MAX_PORT + 1];
    void *          nextValue;
    fm_int          cpi;
    fm_uint64       portSetNumber;
    fm_int          physPort;

    FM_CLEAR(mapSrcPortCfg);

    for (fmTreeIterInit(&itPortSet, portSetIdTree) ;
         (err = fmTreeIterNext(&itPortSet, &portSetNumber, &nextValue)) ==
                FM_OK ; )
    {
        portSetEntry = (fm_portSet*) nextValue;

        for (cpi = 0 ; ; cpi++)
        {
            err = fmFindBitInBitArray(&portSetEntry->associatedPorts,
                                      cpi,
                                      TRUE,
                                      &cpi);

            if (err != FM_OK || cpi < 0)
            {
                break;
            }

            fmMapCardinalPortInternal(switchPtr, cpi, NULL, &physPort);

            if (portSetEntry->mappedValue < FM10000_ACL_PORTSET_PER_RULE_NUM)
            {
                mapSrcPortCfg[physPort].mapSrc |= 1 << portSetEntry->mappedValue;
            }
        }
    }
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    /* Enumerate physical ports. */
    for (physPort = 0 ; physPort <= switchPtr->maxPhysicalPort; physPort++)
    {
        err = fm10000SetMapSourcePort(sw,
                                      physPort,
                                      &mapSrcPortCfg[physPort],
                                      FM_FM10000_MAP_SRC_ID,
                                      TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    }

    return err;

}   /* end fmApplyMapSrcPortId */




/*****************************************************************************/
/** fmInitializeMuxSelect
 * \ingroup intAcl
 *
 * \desc            This function initialize a set of mux based on a compiled
 *                  set by making sure that all the mux select entered are
 *                  valid.
 *
 * \param[in]       srcArray point to the compiled mux select structure to
 *                  base the initialization on.
 *
 * \param[out]      dstArray points to the compiled mux select structure to
 *                  initialize.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmInitializeMuxSelect(fm_byte *srcArray,
                           fm_byte *dstArray)
{
    fm_int i;

    /* Remove all the virtual key with valid value. */
    for (i = 0 ;
         i < FM_FFU_SELECTS_PER_MINSLICE * FM10000_FFU_SLICE_VALID_ENTRIES ;
         i+= FM_FFU_SELECTS_PER_MINSLICE)
    {
        if (srcArray[i] == FM10000_UNUSED_KEY)
        {
            dstArray[i] = FM_FFU_MUX_SELECT_DIP_31_0;
        }
        else
        {
            dstArray[i] = srcArray[i];
        }

        if (srcArray[i + 1] == FM10000_UNUSED_KEY)
        {
            dstArray[i + 1] = FM_FFU_MUX_SELECT_DIP_31_0;
        }
        else
        {
            dstArray[i + 1] = srcArray[i + 1];
        }

        if (srcArray[i + 2] == FM10000_UNUSED_KEY)
        {
            dstArray[i + 2] = FM_FFU_MUX_SELECT_DIP_31_0;
        }
        else
        {
            dstArray[i + 2] = srcArray[i + 2];
        }

        if (srcArray[i + 3] == FM10000_UNUSED_KEY)
        {
            dstArray[i + 3] = FM_FFU_MUX_SELECT_DIP_31_0;
        }
        else
        {
            dstArray[i + 3] = srcArray[i + 3];
        }

        if (srcArray[i + 4] == FM10000_UNUSED_KEY)
        {
            dstArray[i + 4] = FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP;
        }
        else
        {
            dstArray[i + 4] = srcArray[i + 4];
        }
    }

}   /* end fmInitializeMuxSelect */




/*****************************************************************************/
/** fmUpdateMasterValid
 * \ingroup intAcl
 *
 * \desc            Update the ACL part of the FFU_SLICE_MASTER_VALID register
 *                  to reflect the configuration of cacls.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cacls is the compiledAcls structure used to update the
 *                  master valid register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUpdateMasterValid(fm_int sw, fm_fm10000CompiledAcls *cacls)
{
    fm_status err = FM_OK;
    fm_uint32 sliceValid;
    fm_uint32 egressValid;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_int i;
    fm_int j;
    fm_uint64 value;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "cacls = %p\n",
                 sw,
                 (void*) cacls);

    /* Update the Master Valid */
    TAKE_REG_LOCK(sw);

    err = fm10000GetFFUMasterValid(sw,
                                   &sliceValid,
                                   &egressValid,
                                   TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Step 1: Enable transition from 0 --> 1 on both condition and action */
    for (i = firstAclSlice ; i <= lastAclSlice ; i++)
    {
        sliceValid |= cacls->sliceValid & (1 << i);
        sliceValid |= cacls->actionValid & (1 << i);
    }

    if (lastAclSlice == FM10000_ACL_EGRESS_SLICE_POS)
    {
        egressValid = cacls->chunkValid ;
    }

    err = fm10000SetFFUMasterValid(sw,
                                   sliceValid,
                                   egressValid,
                                   TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Step 2: Clear ValidLow and ValidHigh on all scenario for slice with
     *         transition from 1 --> 0 */
    for (i = firstAclSlice ; i <= lastAclSlice ; i++)
    {
        if ( (cacls->prevSliceValid & (1 << i)) &&
            ((cacls->sliceValid & (1 << i)) == 0) )
        {
            for (j = 0 ; j < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; j++)
            {
                err = fmRegCacheReadUINT64V2(sw,
                                             &fm10000CacheFfuSliceCfg,
                                             i,
                                             j,
                                             &value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                FM_SET_BIT64(value, FM10000_FFU_SLICE_CFG, ValidLow, 0);
                FM_SET_BIT64(value, FM10000_FFU_SLICE_CFG, ValidHigh, 0);

                err = fmRegCacheWriteUINT64V2(sw,
                                              &fm10000CacheFfuSliceCfg,
                                              i,
                                              j,
                                              value);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    /* Step 3: Clear the FFU_SLICE_CASCADE_ACTION when actionValid == 0 starting
     *         from the last slice. */
    for (i = lastAclSlice ; i >= firstAclSlice ; i--)
    {
        if ( (cacls->actionValid & (1 << i)) == 0 )
        {
            err = fmRegCacheWriteUINT64(sw,
                                        &fm10000CacheFfuSliceCascadeAction,
                                        i,
                                        0LL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Transition from 1 --> 0, clearing the whole action ram */
            if (cacls->prevActionValid & (1 << i))
            {
                for (j = 0 ; j < FM10000_FFU_SLICE_SRAM_ENTRIES_0 ; j++ )
                {
                    /* Clearing the whole action slice when disabled. This is
                     * done using NOP action (BIT_SET action with mask and
                     * value == 0) */
                    err = fmRegCacheWriteUINT64V2(sw,
                                                  &fm10000CacheFfuSliceSram,
                                                  i,
                                                  j,
                                                  0x00400000LL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
        }
    }

    /* Step 4: Clear the FFU_SLICE_CFG when sliceValid == 0 starting from the
     *         last slice. */
    for (i = lastAclSlice ; i >= firstAclSlice ; i--)
    {
        if ( (cacls->sliceValid & (1 << i)) == 0 )
        {
            for (j = 0 ; j < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; j++)
            {
                err = fmRegCacheWriteUINT64V2(sw,
                                              &fm10000CacheFfuSliceCfg,
                                              i,
                                              j,
                                              0LL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    /* Step 5: Disable transition from 1 --> 0 on both condition and action */
    for (i = firstAclSlice ; i <= lastAclSlice ; i++)
    {
        sliceValid &= ~(1 << i);
        sliceValid |= cacls->sliceValid & (1 << i);
        sliceValid |= cacls->actionValid & (1 << i);
    }

    err = fm10000SetFFUMasterValid(sw,
                                   sliceValid,
                                   egressValid,
                                   TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Save the last configuration for next round */
    cacls->prevSliceValid = cacls->sliceValid;
    cacls->prevActionValid = cacls->actionValid;


ABORT:

    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmUpdateMasterValid */




/*****************************************************************************/
/** fmSetEaclChunkCfg
 * \ingroup intAcl
 *
 * \desc            Configure an egress acl chunk.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       chunk is the chunk to configure.
 * 
 * \param[in]       scenarios is the chunk scenario to configure.
 * 
 * \param[in]       cascade indicate whether to start a priority hit cascade
 *                  across chunks.
 *
 * \param[in]       portTree is a tree containing all the port to configure or
 *                  NULL to reset the egress acl port association.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetEaclChunkCfg(fm_int    sw,
                            fm_int    chunk,
                            fm_uint32 scenarios,
                            fm_bool   cascade,
                            fm_tree * portTree)
{
    fm_status       err = FM_OK;
    fm_uint64       physPortMask = 0;
    fm_treeIterator itPort;
    fm_int          physPort;
    void *          nextValue;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm_uint64       key;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "chunk = %d, "
                 "scenarios = %u, "
                 "cascade = %d, "
                 "portTree = %p\n",
                 sw,
                 chunk,
                 scenarios,
                 cascade,
                 (void*) portTree);

    /* If ports are defined, add them to the bitArray */
    if (portTree != NULL)
    {
        for (fmTreeIterInit(&itPort, portTree) ;
             (err = fmTreeIterNext(&itPort, &key, &nextValue)) == FM_OK ; )
        {
            err = fmMapLogicalPortToPhysical(switchPtr, (fm_int) key, &physPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            physPortMask |= (FM_LITERAL_U64(1) << physPort);

        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Update the egress acl port association */
    err = fm10000ConfigureFFUEaclChunk(sw,
                                       chunk,
                                       scenarios,
                                       physPortMask,
                                       cascade,
                                       TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmSetEaclChunkCfg */




/*****************************************************************************/
/** fmResetFFUSlice
 * \ingroup intAcl
 *
 * \desc            Reset one or multiple slice and the rule that are part
 *                  of those slices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sliceInfo refer to the slices to reset.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmResetFFUSlice(fm_int sw, fm_ffuSliceInfo *sliceInfo)
{
    fm_status err = FM_OK;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "sliceInfo = %p\n",
                 sw,
                 (void*) sliceInfo);

    /* Initialize all rule to invalid */
    for (i = 0 ; i < FM10000_MAX_RULE_PER_ACL_PART ; i++)
    {
        err = fm10000SetFFURuleValid(sw,
                                     sliceInfo,
                                     i,
                                     FALSE,
                                     TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmResetFFUSlice */




/*****************************************************************************/
/** fmGetSlicePosition
 * \ingroup intAcl
 *
 * \desc            Get the next possible position for an ingress compiled acl.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sliceInfo refer to the compiled acl ffu slice info to fit.
 *
 * \param[in]       nextFreeConditionSlice is the first free condition slice
 *                  available.
 *
 * \param[in]       nextFreeActionSlice is the first free action slice
 *                  available.
 *
 * \param[out]      conditionSlicePos is the computed condition slice position
 *                  that match the compiledAcl to fit.
 *
 * \param[out]      actionSlicePos is the computed action slice position that
 *                  match the compiledAcl to fit.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetSlicePosition(fm_int sw,
                             fm_ffuSliceInfo *sliceInfo,
                             fm_int nextFreeConditionSlice,
                             fm_int nextFreeActionSlice,
                             fm_int *conditionSlicePos,
                             fm_int *actionSlicePos)
{
    fm_int actionSlices;
    fm_int conditionSlices;
    fm_int actionSliceHole;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sliceInfo = %p, "
                 "nextFreeConditionSlice = %d, "
                 "nextFreeActionSlice = %d, "
                 "conditionSlicePos = %p, "
                 "actionSlicePos = %p\n",
                 (void*) sliceInfo,
                 nextFreeConditionSlice,
                 nextFreeActionSlice,
                 (void*) conditionSlicePos,
                 (void*) actionSlicePos);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Compute the number of extended action slices needed. */
    actionSlices = sliceInfo->actionEnd - sliceInfo->keyEnd;
    conditionSlices = sliceInfo->keyEnd - sliceInfo->keyStart;

    /* If extended action slices are needed, see if its possible to cascade
     * them with the previous ACL condition. */
    if (actionSlices > 0)
    {
        actionSliceHole = nextFreeActionSlice - nextFreeConditionSlice;

        /* The hole is big enought to insert all the extended action
         * slices. */
        if (actionSliceHole > actionSlices)
        {
            *actionSlicePos = nextFreeConditionSlice;
        }
        /* The hole is not big enought so use it entirely and grab the
         * remaining action slice from the next ACL. */
        else
        {
            *actionSlicePos = nextFreeConditionSlice - actionSlices +
                                                       actionSliceHole;
        }
    }
    else
    {
        *actionSlicePos = nextFreeConditionSlice;
    }

    /* Configure the first condition slice position to use. */
    *conditionSlicePos = *actionSlicePos - conditionSlices;

    /* The ACL image crossed the slice boundary allowed for the ACL
     * subsystem. */
    if (*conditionSlicePos < firstAclSlice)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_ACLS_TOO_BIG);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end fmGetSlicePosition */




/*****************************************************************************/
/** fmGetAclNumKey
 * \ingroup intAcl
 *
 * \desc            Get the acl key that refer to this acl/rule pair.
 *
 * \param[in]       aclTree refer to the acl tree to seach on.
 *
 * \param[in]       acl is the acl id to find.
 *
 * \param[in]       rule is the rule id to find.
 *
 * \param[out]      aclNumKey is the returned key located into the aclTree that
 *                  refer to the entered acl/rule pair.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetAclNumKey(fm_tree *  aclTree,
                         fm_int     acl,
                         fm_int     rule,
                         fm_uint64 *aclNumKey)
{
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    void* value;


    *aclNumKey = FM_ACL_GET_MASTER_KEY(acl);

    while ((err = fmTreeFind(aclTree, *aclNumKey, (void*) &compiledAcl)) == FM_OK)
    {
        if (fmTreeFind(&compiledAcl->rules, rule, &value) == FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
        }
        (*aclNumKey)++;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmGetAclNumKey */




/*****************************************************************************/
/** fmGetFFUSliceRange
 * \ingroup intAcl
 *
 * \desc            Get the FFU slice range currently owned by the FFU subsystem
 *                  or the assigned one if flag FM_ACL_COMPILE_FLAG_TRY_ALLOC
 *                  is set during compilation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstSlice is used to return current first slice available.
 *
 * \param[out]      lastSlice is used to return current last slice available.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetFFUSliceRange(fm_int sw, fm_int *firstSlice, fm_int *lastSlice)
{
    fm_status err = FM_OK;

    if (compileTryAlloc)
    {
        *firstSlice = compileTryAllocFirstSlice;
        *lastSlice  = compileTryAllocLastSlice;
    }
    else
    {
        /* Get ACL slice boundaries */
        err = fm10000GetFFUSliceOwnership(sw,
                                          FM_FFU_OWNER_ACL,
                                          firstSlice,
                                          lastSlice);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fmGetFFUSliceRange */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000InitAclErrorReporter
 * \ingroup intAcl
 *
 * \desc            Initialize an fm_aclErrorReporter structure to accept
 *                  error messages from the compilation.
 *
 * \param[out]      errReport is the fm_aclErrorReporter structure to be
 *                  initialized.
 *
 * \param[in]       statusText is the buffer into which error messages
 *                  should be written.
 *
 * \param[in]       statusTextLength is the size of the statusText
 *                  buffer in bytes.
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000InitAclErrorReporter(fm_aclErrorReporter *   errReport,
                                 fm_text                 statusText,
                                 fm_int                  statusTextLength)
{
    errReport->statusText = statusText;
    errReport->statusTextLength = statusTextLength;
    errReport->numErrors = 0;

}   /* end fm10000InitAclErrorReporter */




/*****************************************************************************/
/** fm10000FormatAclStatus
 * \ingroup intAcl
 *
 * \desc            Write a message into an fm_aclErrorReporter,
 *                  using a printf-style format string.
 *
 * \note            This is a more general version of AddErrorMessage.
 *
 * \param[in,out]   errReport is the object which accepts the error message.
 *                  It's okay for errReport to be NULL, in which case
 *                  nothing is done.
 *
 * \param[in]       isError indicates whether the error counter should
 *                  be incremented (i. e. is this truly an error or just
 *                  a status message)
 *
 * \param[in]       format is the printf-style format.  It should end in
 *                  a newline.
 *
 * \param[in]       ... is the printf var-args argument list.
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000FormatAclStatus(fm_aclErrorReporter *errReport,
                            fm_bool              isError,
                            const char *         format,
                            ...)
{
    fm_int len;
    va_list args;

    if (errReport != NULL && errReport->statusTextLength > 0)
    {
        va_start(args, format);
        len = FM_VSNPRINTF_S(errReport->statusText,
                             errReport->statusTextLength,
                             format, 
                             args);
        va_end(args);

        if (len > 0)
        {
            errReport->statusTextLength -= len;
            errReport->statusText += len;
        }
    }

    if (errReport != NULL && isError)
    {
        errReport->numErrors++;
    }

}   /* end fm10000FormatAclStatus */




/*****************************************************************************/
/** fm10000ACLCompile
 * \ingroup intAcl
 *
 * \desc            Compile the complete set of configured and enabled ACLs.
 *                  ACLs must be compiled after access lists and their
 *                  associated rules are created. After successfully
 *                  compiling, the ACLs will not take effect until they
 *                  are written to the hardware by calling ''fmApplyACL''.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      statusText points to caller-allocated storage where this
 *                  function should place compiler error message text
 *                  (if FM_ERR_ACL_COMPILE is returned) or informative
 *                  statistics about the compilation (if FM_OK is returned).
 *
 * \param[in]       statusTextLength is the number of bytes pointed to by
 *                  statusText.
 *
 * \param[in]       flags is a bitmask. See ''ACL Compiler Flags''.
 *
 * \param[in]       value points to the compiler extended flags entered.
 *                  The data type of value is dependent on the flags selected.
 *                  See ''ACL Compiler Flags'' for a description of the data
 *                  type required for each extended flag.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ACL_COMPILE if the ACL compiler was unable to
 *                  produce a valid ACL "binary image" from the current ACL
 *                  configuration.
 *
 *****************************************************************************/
fm_status fm10000ACLCompile(fm_int    sw,
                            fm_text   statusText,
                            fm_int    statusTextLength,
                            fm_uint32 flags,
                            void *    value)
{
    fm_status err = FM_OK;
    fm_aclErrorReporter errReport;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls *caclsRetry = NULL;
    fm_aclInfo *info;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_treeIterator itInstance;
    fm_uint64 aclNumber;
    fm_uint64 instance;
    void *nextValue;
    fm_aclRule *rule;
    fm_acl *acl;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_tree abstractKey;
    fm_int  actionSlices;
    fm_int  maxActionSlices;
    fm_int  firstAclSlice;
    fm_int  lastAclSlice;
    fm_int  nextFreeActionSlice;
    fm_int  nextFreeConditionSlice;
    fm_int  nextFreeChunk;
    fm_int  actionSlicePos;
    fm_int  conditionSlicePos;
    fm_int  nextPhysical;
    fm_bool bitValue;
    fm_int  cpi;
    fm_int  port;
    fm_int  numChunk;
    fm_bool srcMap;
    fm_fm10000CompiledAcls* compiledClone;
    fm_aclCompilerTryAlloc *aclCompilerTryAlloc;
    fm_fm10000CompiledAclInstance* compiledAclInst;
    fm_byte currentCase;
    fm_byte muxTemp;
    fm_int i;
    fm_int j;
    fm_int previousInst;
    fm_bool strictCount;
    fm_int internalAcl;
    fm_bool egressAcl;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "statusText = %p, "
                 "statusTextLength = %d, "
                 "flags = 0x%x, "
                 "value = %p\n",
                 sw,
                 (void*) statusText,
                 statusTextLength,
                 flags,
                 (void*) value);

    FM_CLEAR(abstractKey);

    fm10000InitAclErrorReporter(&errReport, statusText, statusTextLength);

    info = &switchPtr->aclInfo;

    /**************************************************
     * Verify that mapper entry entered using
     * fmAddMapperEntry are valid.
     **************************************************/
    err = fm10000VerifyMappers(sw, &errReport, &srcMap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (flags & FM_ACL_COMPILE_FLAG_TRY_ALLOC)
    {
        aclCompilerTryAlloc = (fm_aclCompilerTryAlloc *)value;
        if ( (aclCompilerTryAlloc->aclFirstSlice < 0) ||
             (aclCompilerTryAlloc->aclFirstSlice > aclCompilerTryAlloc->aclLastSlice) ||
             (aclCompilerTryAlloc->aclLastSlice > FM10000_FFU_SLICE_VALID_ENTRIES))
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            compileTryAlloc = TRUE;
            compileTryAllocFirstSlice = aclCompilerTryAlloc->aclFirstSlice;
            compileTryAllocLastSlice = aclCompilerTryAlloc->aclLastSlice;
        }
    }

    /* Non disruptive tentative would be done here */
    if (flags & FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE)
    {
        compiledClone = CloneCompiledAcls(sw, switchExt->appliedAcls);
        if (compiledClone)
        {
            if (flags & FM_ACL_COMPILE_FLAG_INTERNAL)
            {
                internalAcl = *( (fm_int *) value );
            }
            else
            {
                internalAcl = -1;
            }

            err = fm10000NonDisruptCompile(sw,
                                           compiledClone,
                                           internalAcl,
                                           FALSE);
            if (err == FM_OK)
            {
                FillCompileStats(compiledClone);

                /**************************************************
                * Print statistics on success, such as the number
                * of minslices used.
                **************************************************/
                FormatCompileStats(&compiledClone->compilerStats, &errReport);

                if (flags & FM_ACL_COMPILE_FLAG_RETURN_STATS)
                {
                    if (flags & FM_ACL_COMPILE_FLAG_TRY_ALLOC)
                    {
                        aclCompilerTryAlloc = (fm_aclCompilerTryAlloc *)value;
                        FM_MEMCPY_S( &aclCompilerTryAlloc->compilerStats,
                                     sizeof(aclCompilerTryAlloc->compilerStats),
                                     &compiledClone->compilerStats,
                                     sizeof(fm_aclCompilerStats));
                    }
                    else
                    {
                        FM_MEMCPY_S( value,
                                     sizeof(fm_aclCompilerStats),
                                     &compiledClone->compilerStats,
                                     sizeof(fm_aclCompilerStats) );
                    }
                }
            }
        }
        else
        {
            err = FM_ERR_ACL_COMPILE;
        }
        compileTryAlloc = FALSE;
        FreeCompiledAclsStruct(compiledClone);
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    /* If non disruptive failed, try compiling it from scratch */
    caclsRetry = (fm_fm10000CompiledAcls *)
        fmAlloc( sizeof(fm_fm10000CompiledAcls) );

    if (caclsRetry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    FM_CLEAR(*caclsRetry);

    InitializeCompiledAcls(sw, caclsRetry);

    /* Reserve srcMap for mapper if needed. */
    if (srcMap)
    {
        caclsRetry->usedPortSet |= (1 << FM10000_ACL_PORTSET_OWNER_POS);
        caclsRetry->usedPortSet |= 0xf;
    }

    strictCount = switchExt->aclStrictCount;

    /* sanity check */
    err = fmTreeValidate(&info->acls);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    previousInst = FM_ACL_NO_INSTANCE;
    /**************************************************
     * Pre-process each ACL in the configuration.
     **************************************************/
    for (fmTreeIterInit(&itAcl, &info->acls) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        /* abstractKey is used to store all the 4 or 8 bits abstract key
         * needed by each ACL. */
        fmTreeInit(&abstractKey);
        acl = (fm_acl *) nextValue;

        /* ACL with no rule will be skip. */
        if (fmTreeSize(&acl->rules) == 0)
        {
            continue;
        }

        /* ACL with assigned port will be skip if no ports are part of that
         * switch. This situation only make sense on SWAG. */
        if ( ((acl->aclPortType == FM_ACL_PORT_TYPE_INGRESS) &&
              (acl->numberOfPorts[FM_ACL_TYPE_INGRESS] == 0)) ||
             ((acl->aclPortType == FM_ACL_PORT_TYPE_EGRESS) &&
              (acl->numberOfPorts[FM_ACL_TYPE_EGRESS] == 0)) )
        {
            continue;
        }

        compiledAcl = fmAlloc(sizeof(fm_fm10000CompiledAcl));
        if (compiledAcl == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_CLEAR(*compiledAcl);

        compiledAcl->aclParts = 0;
        compiledAcl->firstAclPart = TRUE;
        compiledAcl->aclNum = aclNumber;
        compiledAcl->aclKeepUnusedKeys = acl->aclKeepUnusedKeys;
        compiledAcl->internal = acl->internal;
        compiledAcl->aclInstance = acl->instance;
        compiledAcl->numRules = 0;
        compiledAcl->sliceInfo.selects = compiledAcl->muxSelect;

        /* caseLocation[] will be automatically initialized to
         * FM_FFU_CASE_NOT_MAPPED with initial FM_CLEAR */
        compiledAcl->sliceInfo.caseLocation = compiledAcl->caseLocation;

        fmTreeInit(&compiledAcl->rules);

        compiledAcl->portSetId = fmAlloc(sizeof(fm_tree));
        if (compiledAcl->portSetId == NULL)
        {
            fmFree(compiledAcl);
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        FM_CLEAR(*compiledAcl->portSetId);

        fmTreeInit(compiledAcl->portSetId);

        egressAcl = (acl->numberOfPorts[FM_ACL_TYPE_EGRESS] != 0) ? TRUE : FALSE;

        fmTranslateAclScenario(sw,
                               acl->scenarios,
                               &compiledAcl->sliceInfo.validScenarios,
                               egressAcl);

        /********************************************************************
         * The port association of an ACL could be either per ACL or per rules.
         * In the first option, the port type could be either classify as
         * egress ports of ingress ports and this is how the compiler
         * distinguish between ingress and egress ACL.
         ********************************************************************/
        if (egressAcl)
        {
            err = fmTreeInsert(&caclsRetry->egressAcl, aclNumber, compiledAcl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            fmTreeDestroy(&abstractKey, NULL);
            continue;
        }
        else
        {
            err = fmTreeInsert(&caclsRetry->ingressAcl,
                               FM_ACL_GET_MASTER_KEY(aclNumber),
                               compiledAcl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (acl->numberOfPorts[FM_ACL_TYPE_INGRESS])
            {
                err = fmTreeInsert(compiledAcl->portSetId,
                                   FM10000_SPECIAL_PORT_PER_ACL_KEY,
                                   NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        /* sanity check */
        err = fmTreeValidate(&acl->rules);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Find the rule that needs the maximum action slice and save this
         * value to use it in slice allocation. */
        maxActionSlices = 0;

        /**************************************************
         * Process each rule in the ACL.
         **************************************************/
        for (fmTreeIterInit(&itRule, &acl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber,
                                   &nextValue)) == FM_OK ; )
        {
            rule = (fm_aclRule *) nextValue;

            compiledAclRule = fmAlloc(sizeof(fm_fm10000CompiledAclRule));
            if (compiledAclRule == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            FM_CLEAR(*compiledAclRule);

            compiledAclRule->aclNumber = aclNumber;
            compiledAclRule->ruleNumber = ruleNumber;
            compiledAclRule->valid = rule->state;
            compiledAclRule->portSetId = FM_PORT_SET_ALL;

            err = fmTreeInsert(&compiledAcl->rules,
                               ruleNumber,
                               compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* The abstract key tree contains all the abstract keys needed for
             * the whole ACL except for the key related to the port selection.*/
            err = fmFillAbstractKeyTree(sw,
                                        &errReport,
                                        rule,
                                        &abstractKey,
                                        compiledAcl->portSetId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Add key related to the port selection */
            err = fmFillAbstractPortSetKeyTree(&errReport,
                                               rule,
                                               NULL,
                                               &abstractKey,
                                               compiledAcl->portSetId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmCountActionSlicesNeeded(sw,
                                            &errReport,
                                            rule,
                                            &actionSlices);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAclRule->numActions = actionSlices;

            if (maxActionSlices < actionSlices)
            {
                maxActionSlices = actionSlices;
            }

            err = fm10000PreallocatePolicerBank(sw,
                                                &errReport,
                                                caclsRetry,
                                                rule,
                                                compiledAclRule,
                                                strictCount);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAcl->numRules++;

        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        if (maxActionSlices > FM10000_ACL_MAX_ACTIONS_PER_RULE)
        {
            fm10000FormatAclStatus(&errReport, TRUE,
                                   "Number of actions slices needed for ACL %d "
                                   "exceeded the maximum supported.\n",
                                   compiledAcl->aclNum);
            err = FM_ERR_INVALID_ACL_RULE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        fmInitializeConcreteKey(compiledAcl->muxSelect);

        /* Convert all the abstract key needed for this ACL into a concrete set
         * of mux configuration that cover all of them. */
        err = fmConvertAbstractToConcreteKey(&abstractKey,
                                             compiledAcl->muxSelect,
                                             compiledAcl->muxUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* At this point, all the slice information are configured as if they
         * start at the position 0 up to the last action meaning that actionEnd
         * plus one equal the number of slice needed for this ACL. */
        compiledAcl->sliceInfo.keyStart = 0;
        compiledAcl->sliceInfo.keyEnd =
                           fmCountConditionSliceUsage(compiledAcl->muxSelect) - 1;
        compiledAcl->sliceInfo.actionEnd = compiledAcl->sliceInfo.keyEnd +
                                           maxActionSlices - 1;

        /* The abstract key tree is only valid for each independent ACL. */
        fmTreeDestroy(&abstractKey, NULL);

        /* Process instance specific functionality */
        if (compiledAcl->aclInstance != FM_ACL_NO_INSTANCE)
        {
            if (compiledAcl->aclInstance < 0)
            {
                fm10000FormatAclStatus(&errReport, TRUE,
                                       "Invalid Instance selected for ACL %d.\n",
                                       compiledAcl->aclNum);
                err = FM_ERR_INVALID_ACL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            err = fmTreeFind(&caclsRetry->instance,
                             compiledAcl->aclInstance,
                             (void**) &compiledAclInst);
            if (err == FM_ERR_NOT_FOUND)
            {
                /* First user of this instance must create it */
                compiledAclInst = fmAlloc(sizeof(fm_fm10000CompiledAclInstance));
                if (compiledAclInst == NULL)
                {
                    err = FM_ERR_NO_MEM;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
                FM_CLEAR(*compiledAclInst);
                fmTreeInit(&compiledAclInst->acl);

                err = fmTreeInsert(&caclsRetry->instance,
                                   compiledAcl->aclInstance,
                                   compiledAclInst);
                if (err != FM_OK)
                {
                    fmFreeCompiledAclInstance(compiledAclInst);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                }
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            else if (previousInst != compiledAcl->aclInstance)
            {
                fm10000FormatAclStatus(&errReport, TRUE,
                                       "Instance %d must only contains "
                                       "consecutive ACL Id.\n",
                                       compiledAcl->aclInstance);
                err = FM_ERR_INVALID_ACL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* Add this ACL to be part of the instance */
            err = fmTreeInsert(&compiledAclInst->acl,
                               FM_ACL_GET_MASTER_KEY(aclNumber),
                               compiledAcl);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Validates the scenario to be mutually exclusive with all the
             * others already defined. */
            if (compiledAclInst->sliceInfo.validScenarios &
                compiledAcl->sliceInfo.validScenarios)
            {
                fm10000FormatAclStatus(&errReport, TRUE,
                                       "Instance %d specify ACLs with non "
                                       "mutually exclusive scenarios.\n",
                                       compiledAcl->aclInstance);
                err = FM_ERR_INVALID_ACL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }

            /* The instance scenario contains all the individual one. */
            compiledAclInst->sliceInfo.validScenarios |= compiledAcl->sliceInfo.validScenarios;

            /* Slice Range used by the instance is the maximum of all the ACLs
             * that belongs to this instance. Slice range does not includes
             * case selection key at this point. */
            if (compiledAcl->sliceInfo.keyEnd > compiledAclInst->sliceInfo.keyEnd)
            {
                compiledAclInst->sliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd;
            }

            if (compiledAcl->sliceInfo.actionEnd > compiledAclInst->sliceInfo.actionEnd)
            {
                compiledAclInst->sliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd;
            }

            /* Update the total number of rules that belongs to this
             * instance. */
            compiledAclInst->numRules += compiledAcl->numRules;
        }

        previousInst = compiledAcl->aclInstance;
    }

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Now process the Egress ACL. All the Egress ACL must be grouped together
     * and share the same set of TCAM key. */
    fmTreeInit(&abstractKey);
    for (fmTreeIterInit(&itAcl, &caclsRetry->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        err = fmTreeFind(&info->acls, aclNumber, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        acl = (fm_acl *) nextValue;

        /* Extract the egress port configured from this ACL and
         * configure the portSet accordingly. */
        for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
        {
            port = GET_LOGICAL_PORT(sw, cpi);

            err = fmGetAclAssociatedPort(sw,
                                         acl,
                                         port,
                                         FM_ACL_TYPE_EGRESS,
                                         &bitValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (bitValue)
            {
                /* Use the portSet tree to store the egress port configured. */
                err = fmTreeInsert(compiledAcl->portSetId,
                                   port,
                                   NULL);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

        for (fmTreeIterInit(&itRule, &acl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber,
                                   &nextValue)) == FM_OK ; )
        {
            rule = (fm_aclRule *) nextValue;

            compiledAclRule = fmAlloc(sizeof(fm_fm10000CompiledAclRule));
            if (compiledAclRule == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
            FM_CLEAR(*compiledAclRule);

            compiledAclRule->aclNumber = aclNumber;
            compiledAclRule->ruleNumber = ruleNumber;
            compiledAclRule->valid = rule->state;

            err = fmTreeInsert(&compiledAcl->rules,
                               ruleNumber,
                               compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* The abstract key tree contain all the abstract key needed for
             * the all the egress ACLs.*/
            err = fmFillAbstractKeyTree(sw,
                                        &errReport,
                                        rule,
                                        &abstractKey,
                                        NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Add key related to the ingress port selection */
            err = fmFillAbstractPortSetKeyTree(&errReport,
                                               rule,
                                               NULL,
                                               &abstractKey,
                                               NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAcl->numRules++;
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Configure all egress ACL with the same slice range. */
    for (fmTreeIterInit(&itAcl, &caclsRetry->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        fmInitializeConcreteKey(compiledAcl->muxSelect);

        /* Convert all the abstract key needed for all the egress ACL into a
         * concrete set of mux configuration that cover all of them. */
        err = fmConvertAbstractToConcreteKey(&abstractKey,
                                             compiledAcl->muxSelect,
                                             compiledAcl->muxUsed);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* At this point, all the slice information are configured as if they
         * start at the position 0 up to the last action meaning that actionEnd
         * plus one equal the number of slice needed for this ACL. */
        compiledAcl->sliceInfo.keyStart = 0;
        compiledAcl->sliceInfo.keyEnd =
                           fmCountConditionSliceUsage(compiledAcl->muxSelect) - 1;
        compiledAcl->sliceInfo.actionEnd = compiledAcl->sliceInfo.keyEnd;

    }
    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    fmTreeDestroy(&abstractKey, NULL);

    /*************************************************************************
     * At this point, all the ACL/ACL-rule was pre-processed as if each of them
     * was compiled individually. Now, all of them will be processed as a
     * whole.
     *************************************************************************/

    /* Select the portSet ID to use for each ACL/ACL-rule. */
    err = AllocatePortSetId(sw, &errReport, caclsRetry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Start filling the ACL at last position to keep the maximum free space
     * between the routing resources and the ACL one. */
    nextFreeActionSlice = lastAclSlice;
    nextFreeConditionSlice = lastAclSlice;
    nextFreeChunk = FM10000_ACL_EGRESS_CHUNK_NUM - 1;
    for (fmTreeIterInit(&itAcl, &caclsRetry->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        err = fmTreeFind(&info->acls, aclNumber, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        acl = (fm_acl *) nextValue;

        if (lastAclSlice != FM10000_ACL_EGRESS_SLICE_POS)
        {
            fm10000FormatAclStatus(&errReport, TRUE,
                                   "Incorrect FFU slice allocation, Egress ACL "
                                   "needs slice position %d.\n",
                                   FM10000_ACL_EGRESS_SLICE_POS);
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Update the slice information with real position. */
        compiledAcl->sliceInfo.keyStart = FM10000_ACL_EGRESS_SLICE_POS -
                                          compiledAcl->sliceInfo.keyEnd;
        compiledAcl->sliceInfo.keyEnd = FM10000_ACL_EGRESS_SLICE_POS;
        compiledAcl->sliceInfo.actionEnd = FM10000_ACL_EGRESS_SLICE_POS;

        /* Update the next free position for the next ACL to compile. */
        nextFreeConditionSlice = compiledAcl->sliceInfo.keyStart - 1;
        nextFreeActionSlice = FM10000_ACL_EGRESS_SLICE_POS - 1;

        /* Update the bitmask used to maintain the validity of each slice. */
        fmUpdateValidSlice(&compiledAcl->sliceInfo, caclsRetry);

        /* Compute the number of chunk needed for this ACL. Chunk can be merge
         * together to support more then 32 rule per ACL. */
        numChunk = ((compiledAcl->numRules - 1) / FM10000_ACL_EGRESS_CHUNK_SIZE) + 1;
        compiledAcl->numChunk = numChunk;

        if ((nextFreeChunk - (numChunk - 1)) < 0)
        {
            fm10000FormatAclStatus(&errReport, TRUE,
                                   "No more egress chunk available.\n");
            err = FM_ERR_ACL_COMPILE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        nextPhysical = ((nextFreeChunk + 1) * FM10000_ACL_EGRESS_CHUNK_SIZE) - 1;
        compiledAcl->chunk = nextFreeChunk;
        nextFreeChunk -= numChunk;

        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) == FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
            err = fmTreeFind(&acl->rules, ruleNumber, &nextValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            rule = (fm_aclRule *) nextValue;

            compiledAclRule->physicalPos = nextPhysical--;

            /* Configure the condition key and key mask based on the entered
             * acl rule. */
            err = fmConfigureConditionKey(sw,
                                          &errReport,
                                          rule,
                                          ruleNumber,
                                          compiledAcl,
                                          NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Configure the action data based on the entered acl rule. */
            err = fmConfigureEgressActionData(rule, &errReport, compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }   /* end for (fmTreeIterInit(&itAcl, &caclsRetry->egressAcl)... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    for (fmTreeIterInit(&itInstance, &caclsRetry->instance) ;
         (err = fmTreeIterNext(&itInstance, &instance, &nextValue)) == FM_OK ; )
    {
        compiledAclInst = (fm_fm10000CompiledAclInstance*) nextValue;

        if ( (compiledAclInst->numRules > FM10000_MAX_RULE_PER_ACL_PART) ||
             (fmTreeSize(&compiledAclInst->acl) > FM_MAX_ACL_PER_INSTANCE) )
        {
            compiledAclInst->mergedAcls = FALSE;
        }
        else
        {
            compiledAclInst->mergedAcls = TRUE;
            currentCase = 0;

            /* Find a free position for the Case matching */
            for (fmTreeIterInit(&itAcl, &compiledAclInst->acl) ;
                 (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
            {
                compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

                for (i = 0 ; i < compiledAcl->sliceInfo.keyEnd + 1 ; i++)
                {
                    /* TopKey Position is already partially taken or not taken
                     * at all. */
                    if ((compiledAcl->muxUsed[i] & 0x100) == 0)
                    {
                        compiledAcl->caseLocation[i] = FM_FFU_CASE_TOP_LOW_NIBBLE;
                        break;
                    }
                    else if ((compiledAcl->muxUsed[i] & 0x200) == 0)
                    {
                        compiledAcl->caseLocation[i] = FM_FFU_CASE_TOP_HIGH_NIBBLE;
                        break;
                    }
                    /* One of the 8-bit key can be swapped with the Top position
                     * to free up a position for the case condition. */
                    else if ((compiledAcl->muxUsed[i] & 0xff) != 0xff)
                    {
                        for (j = 0 ; j < (FM_FFU_SELECTS_PER_MINSLICE - 1) ; j++)
                        {
                            if ((compiledAcl->muxUsed[i] & (0x1 << (j * 2))) == 0)
                            {
                                compiledAcl->caseLocation[i] = FM_FFU_CASE_TOP_LOW_NIBBLE;
                                break;

                            }
                            else if ((compiledAcl->muxUsed[i] & (0x1 << ((j * 2) + 1))) == 0)
                            {
                                compiledAcl->caseLocation[i] = FM_FFU_CASE_TOP_HIGH_NIBBLE;
                                break;
                            }
                        }

                        /* Swap position */
                        muxTemp = compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4];
                        compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + 4] =
                            compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j];

                        compiledAcl->muxSelect[(i * FM_FFU_SELECTS_PER_MINSLICE) + j] = muxTemp;

                        compiledAcl->muxUsed[i] &= ~0x300;
                        compiledAcl->muxUsed[i] |= ((compiledAcl->muxUsed[i] >> (j * 2)) & 0x3) << 8;
                        compiledAcl->muxUsed[i] |= (0x3 << (j * 2));
                        break;
                    }
                }

                /* Was not possible to find way to insert case condition in
                 * current ffu slice range. */
                if (i == compiledAcl->sliceInfo.keyEnd + 1)
                {
                    /* Increase ffu slice range by one */
                    compiledAcl->sliceInfo.keyEnd++;
                    compiledAcl->sliceInfo.actionEnd++;
                    compiledAcl->caseLocation[i] = FM_FFU_CASE_TOP_HIGH_NIBBLE;

                    /* Increase the instance ffu slice range if necessary. */
                    if (compiledAcl->sliceInfo.keyEnd > compiledAclInst->sliceInfo.keyEnd)
                    {
                        compiledAclInst->sliceInfo.keyEnd = compiledAcl->sliceInfo.keyEnd;
                    }

                    if (compiledAcl->sliceInfo.actionEnd > compiledAclInst->sliceInfo.actionEnd)
                    {
                        compiledAclInst->sliceInfo.actionEnd = compiledAcl->sliceInfo.actionEnd;
                    }
                }

                compiledAcl->sliceInfo.kase = currentCase++;

            }   /* end for (fmTreeIterInit(&itAcl, &compiledAclInst->acl)... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }

    }   /* end for (fmTreeIterInit(&itInstance, &caclsRetry->instance)... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    for (fmTreeIterInit(&itAcl, &caclsRetry->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        if (compiledAcl->numRules > FM10000_MAX_RULE_PER_ACL_PART)
        {
            err = BreakCompiledAcl(caclsRetry, aclNumber);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fmTreeIterInitFromSuccessor(&itAcl,
                                              &caclsRetry->ingressAcl,
                                              aclNumber);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }   /* end for (fmTreeIterInit(&itAcl, &caclsRetry->ingressAcl)... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    previousInst = FM_ACL_NO_INSTANCE;
    nextPhysical = 0;
    for (fmTreeIterInit(&itAcl, &caclsRetry->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        err = fmTreeFind(&info->acls,
                         aclNumber >> FM10000_ACL_NUM_KEY_POS,
                         &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        acl = (fm_acl *) nextValue;

        /* The maximum number of rule for each ACL is limited to the maximum of
         * hardware resources per slices. */
        if (compiledAcl->numRules > FM10000_MAX_RULE_PER_ACL_PART)
        {
            fm10000FormatAclStatus(&errReport, TRUE,
                                   "Number of acl rules (%d) for ACL %d has been "
                                   "exceeded.\n",
                                   compiledAcl->numRules,
                                   compiledAcl->aclNum);
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        if (compiledAcl->aclInstance == FM_ACL_NO_INSTANCE)
        {
            err = fmGetSlicePosition(sw,
                                     &compiledAcl->sliceInfo,
                                     nextFreeConditionSlice,
                                     nextFreeActionSlice,
                                     &conditionSlicePos,
                                     &actionSlicePos);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAclInst = NULL;
            nextPhysical = 0;
        }
        else
        {
            err = fmTreeFind(&caclsRetry->instance,
                             compiledAcl->aclInstance,
                             (void**) &compiledAclInst);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (compiledAclInst->mergedAcls == FALSE)
            {
                err = fmGetSlicePosition(sw,
                                         &compiledAcl->sliceInfo,
                                         nextFreeConditionSlice,
                                         nextFreeActionSlice,
                                         &conditionSlicePos,
                                         &actionSlicePos);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                nextPhysical = 0;
            }
            else if (compiledAcl->aclInstance != previousInst)
            {
                err = fmGetSlicePosition(sw,
                                         &compiledAclInst->sliceInfo,
                                         nextFreeConditionSlice,
                                         nextFreeActionSlice,
                                         &conditionSlicePos,
                                         &actionSlicePos);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                nextPhysical = 0;
            }

        }
        
        /* Update the slice information with real position. */
        compiledAcl->sliceInfo.keyStart = conditionSlicePos;
        compiledAcl->sliceInfo.keyEnd += conditionSlicePos;
        compiledAcl->sliceInfo.actionEnd += conditionSlicePos;

        /* Update the next free position for the next ACL to compile. */
        nextFreeConditionSlice = conditionSlicePos - 1;
        nextFreeActionSlice = actionSlicePos - 1;

        /* Update the bitmask used to maintain the validity of each slice. */
        fmUpdateValidSlice(&compiledAcl->sliceInfo, caclsRetry);

        /* Now configure each rule in regards with the key selected. The
         * physical position of the rule start at 0 up to the number of rule
         * minus one. */
        for (fmTreeIterInitBackwards(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) == FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;
            err = fmTreeFind(&acl->rules, ruleNumber, &nextValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            rule = (fm_aclRule *) nextValue;

            compiledAclRule->physicalPos = nextPhysical++;

            /* Configure the condition key and key mask based on the entered
             * acl rule. */
            err = fmConfigureConditionKey(sw,
                                          &errReport,
                                          rule,
                                          ruleNumber,
                                          compiledAcl,
                                          compiledAclInst);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fm10000ConfigurePolicerBank(sw,
                                              &errReport,
                                              caclsRetry,
                                              rule,
                                              compiledAclRule,
                                              strictCount);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            /* Configure the action data based on the entered acl rule. */
            err = fmConfigureActionData(sw,
                                        &errReport,
                                        caclsRetry->policers,
                                        &caclsRetry->ecmpGroups,
                                        rule,
                                        compiledAcl,
                                        compiledAclRule);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        previousInst = compiledAcl->aclInstance;

    }   /* end for (fmTreeIterInit(&itAcl, &caclsRetry->ingressAcl)... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    err = FM_OK;

    if (errReport.numErrors > 0)
    {
        /* Not supposed to be here, this error must have been caught previously. */
        err = FM_ERR_ACL_COMPILE;
    }
    else
    {
        /* If we got here, the acl image must be valid except if this
         * compilation is a validation based on a virtual ffu slice allocation. */
        if (!(flags & FM_ACL_COMPILE_FLAG_TRY_ALLOC))
        {
            caclsRetry->valid = TRUE;
        }

        FillCompileStats(caclsRetry);

        /**************************************************
         * Print statistics on success, such as the number
         * of minslices used.
         **************************************************/
        FormatCompileStats(&caclsRetry->compilerStats, &errReport);

        if (flags & FM_ACL_COMPILE_FLAG_RETURN_STATS)
        {
            if (flags & FM_ACL_COMPILE_FLAG_TRY_ALLOC)
            {
                aclCompilerTryAlloc = (fm_aclCompilerTryAlloc *)value;
                FM_MEMCPY_S( &aclCompilerTryAlloc->compilerStats,
                             sizeof(aclCompilerTryAlloc->compilerStats),
                             &caclsRetry->compilerStats,
                             sizeof(fm_aclCompilerStats));
            }
            else
            {
                FM_MEMCPY_S( value,
                             sizeof(fm_aclCompilerStats),
                             &caclsRetry->compilerStats,
                             sizeof(fm_aclCompilerStats) );
            }
        }
    }

ABORT:

    if (fmTreeIsInitialized(&abstractKey))
    {
        fmTreeDestroy(&abstractKey, NULL);
    }

    compileTryAlloc = FALSE;
    /* Update the compiled acls structure. */
    FreeCompiledAclsStruct(switchExt->compiledAcls);
    switchExt->compiledAcls = caclsRetry;

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ACLCompile */




/*****************************************************************************/
/** fm10000ACLApplyExt
 * \ingroup intAcl
 *
 * \desc            Apply an ACL "binary image," created with a call to
 *                  ''fmCompileACL'', to the FM10000 device.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags is use to distinguish between standard and non
 *                  disruptive apply.
 *
 * \param[in,out]   value points to the compiler extended flags entered.
 *                  The data type of value is dependent on the flags selected.
 *                  See ''ACL Apply Flags'' for a description of the data
 *                  type required for each extended flag.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL_IMAGE if compiled ACLs are invalid.
 * \return          FM_ERR_ACLS_TOO_BIG if the compiled ACLs will not fit
 *                  in the portion of the FFU allocated to ACLs.
 *
 *****************************************************************************/
fm_status fm10000ACLApplyExt(fm_int    sw,
                             fm_uint32 flags,
                             void *    value)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls *cacls;
    fm_fm10000CompiledAcls *aacls;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_byte tmpMuxSelectArray[FM_FFU_SELECTS_PER_MINSLICE *
                              FM10000_FFU_SLICE_VALID_ENTRIES];
    const fm_byte* muxSelectPtrTmp;
    fm_int  i;
    fm_int  j;
    fm_bool egressInitialized;
    fm_aclInfo *info;
    fm_acl *acl;
    fm_int firstAclSlice;
    fm_int lastAclSlice;
    fm_ffuSliceInfo tmpSliceId;
    fm_uint32 validScenariosTmp;
    fm_ffuOwnerType owner;
    fm_bitArray *originalPortMask = NULL;
    fm_bitArray zeroPortMask;
    fm_bool zeroPortMaskInit = FALSE;
    fm_int numPorts;
    fm_int cpi;
    fm_int port;
    fm_aclCompilerStats *stats = NULL;
    fm_int internalAcl;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, flags = 0x%x, stats = %p\n",
                 sw, flags, (void*) stats);

    cacls = switchExt->compiledAcls;
    aacls = switchExt->appliedAcls;

    if (flags & FM_ACL_APPLY_FLAG_RETURN_STATS)
    {
        stats = (fm_aclCompilerStats *)value;
    }

    /* Non disruptive apply would be done here */
    if (flags & FM_ACL_APPLY_FLAG_NON_DISRUPTIVE)
    {
        if (switchExt->appliedAcls)
        {
            if (flags & FM_ACL_APPLY_FLAG_INTERNAL)
            {
                internalAcl = *( (fm_int *) value );
            }
            else
            {
                internalAcl = -1;
            }

            err = fm10000NonDisruptCompile(sw,
                                           switchExt->appliedAcls,
                                           internalAcl,
                                           TRUE);
            if (stats)
            {
                FillCompileStats(switchExt->appliedAcls);
                FM_MEMCPY_S( stats,
                             sizeof(fm_aclCompilerStats),
                             &switchExt->appliedAcls->compilerStats,
                             sizeof(fm_aclCompilerStats) );
            }
        }
        else
        {
            err = FM_ERR_INVALID_ACL_IMAGE;
        }

        /* Invalidate the previously compiled standard ACL image. */
        if (cacls != NULL)
        {
            cacls->valid = FALSE;
        }

        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }
    else if (flags != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ACL,
                     "Invalid apply flag %u\n",
                     flags);
        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_ERR_INVALID_ARGUMENT);
    }

    /* Disruptive apply */
    if ((cacls == NULL) ||
        (cacls->valid != TRUE))
    {
        err = FM_ERR_NO_ACLS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Stop Traffic */
    numPorts = switchPtr->numCardinalPorts;

    err = fmCreateBitArray(&zeroPortMask, numPorts);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ACL, err);

    zeroPortMaskInit = TRUE;

    err = fmSetBitArrayBlock(&zeroPortMask, 0, numPorts, 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    originalPortMask = fmAlloc(numPorts * sizeof(fm_bitArray));
    if (originalPortMask == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }
    FM_MEMSET_S( originalPortMask,
                 numPorts * sizeof(fm_bitArray),
                 0,
                 numPorts * sizeof(fm_bitArray) );

    FM_TAKE_PORT_ATTR_LOCK(sw);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        err = fmCreateBitArray(&originalPortMask[cpi], numPorts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        if (switchPtr->perLagMgmt && fmPortIsInALAG(sw, port))
        {
            port = fmGetLAGForPort(sw, port);
        }

        err = fm10000GetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_MASK_WIDE,
                                      &originalPortMask[cpi]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port = GET_LOGICAL_PORT(sw, cpi);

        if (switchPtr->perLagMgmt && fmPortIsInALAG(sw, port))
        {
            port = fmGetLAGForPort(sw, port);
        }

        err = fm10000SetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_MASK_WIDE,
                                      &zeroPortMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmApplyMapSrcPortId(sw,
                              &cacls->portSetId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fm10000ApplyPolicerCfg(sw, cacls->policers);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Reset the whole ACL slice range. */
    for (i = firstAclSlice ; i <= lastAclSlice ; i++)
    {
        tmpSliceId.keyStart = i;
        tmpSliceId.keyEnd = i;
        tmpSliceId.actionEnd = i;
        tmpSliceId.validScenarios = 0xffffffff;
        err = fmResetFFUSlice(sw, &tmpSliceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        for (j = 0 ; j < FM10000_FFU_SLICE_SRAM_ENTRIES_0 ; j++ )
        {
            /* Clearing the whole action slice when disabled. This is
             * done using NOP action (BIT_SET action with mask and
             * value == 0) */
            err = fmRegCacheWriteUINT64V2(sw,
                                          &fm10000CacheFfuSliceSram,
                                          i,
                                          j,
                                          0x00400000LL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Apply all the egress ACLs */
    egressInitialized = FALSE;
    cacls->chunkValid = 0;

    for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        /* ACL only make use of one scenario for all the egress ACL configured */
        if (egressInitialized == FALSE)
        {
            fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelectArray);

            compiledAcl->sliceInfo.validLow = TRUE;
            compiledAcl->sliceInfo.validHigh = TRUE;

            muxSelectPtrTmp = compiledAcl->sliceInfo.selects;
            compiledAcl->sliceInfo.selects = tmpMuxSelectArray;
            validScenariosTmp = compiledAcl->sliceInfo.validScenarios;
            compiledAcl->sliceInfo.validScenarios = 0xffffffff;

            err = fm10000ConfigureFFUSlice(sw,
                                           &compiledAcl->sliceInfo,
                                           TRUE);
            compiledAcl->sliceInfo.validScenarios = validScenariosTmp;
            compiledAcl->sliceInfo.selects = muxSelectPtrTmp;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            egressInitialized = TRUE;
        }

        for (i = 0 ; i < compiledAcl->numChunk ; i++)
        {
            cacls->chunkValid |= 1 << (compiledAcl->chunk - i);

            err = fmSetEaclChunkCfg(sw,
                                    compiledAcl->chunk - i,
                                    compiledAcl->sliceInfo.validScenarios,
                                    (i == 0) ? TRUE : FALSE,
                                    compiledAcl->portSetId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Apply each rule individually */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                    FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

            err = fm10000SetFFURule(sw,
                                    &compiledAcl->sliceInfo,
                                    compiledAclRule->physicalPos,
                                    compiledAclRule->valid,
                                    compiledAclRule->sliceKey,
                                    compiledAclRule->actions,
                                    FALSE, /* Live */
                                    TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            err = fm10000SetFFUEaclAction(sw,
                                          compiledAclRule->physicalPos,
                                          compiledAclRule->egressDropActions,
                                          TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }   /* end for (fmTreeIterInit(&itAcl, &cacls->egressAcl) ... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Remove any egress port association of unused egress acl. */
    for (i = 0 ; i < FM10000_ACL_EGRESS_CHUNK_NUM ; i++)
    {
        if ((cacls->chunkValid & (1 << i)) == 0)
        {
            err = fmSetEaclChunkCfg(sw, i, 0, FALSE, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }

    /* Apply all the ingress ACLs */
    for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ;
         (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

        fmInitializeMuxSelect(compiledAcl->muxSelect, tmpMuxSelectArray);

        compiledAcl->sliceInfo.validLow = TRUE;
        compiledAcl->sliceInfo.validHigh = TRUE;

        muxSelectPtrTmp = compiledAcl->sliceInfo.selects;
        compiledAcl->sliceInfo.selects = tmpMuxSelectArray;

        err = fm10000ConfigureFFUSlice(sw,
                                       &compiledAcl->sliceInfo,
                                       TRUE);
        compiledAcl->sliceInfo.selects = muxSelectPtrTmp;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Apply each rule individually */
        for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
             (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                    FM_OK ; )
        {
            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

            err = fm10000SetFFURule(sw,
                                    &compiledAcl->sliceInfo,
                                    compiledAclRule->physicalPos,
                                    compiledAclRule->valid,
                                    compiledAclRule->sliceKey,
                                    compiledAclRule->actions,
                                    FALSE, /* Live */
                                    TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    err = fmUpdateMasterValid(sw, cacls);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Move the compiled acls structure to the applied one. */
    FreeCompiledAclsStruct(switchExt->appliedAcls);
    switchExt->appliedAcls = cacls;
    switchExt->compiledAcls = NULL;

    err = fm10000ApplyMappers(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
    {
        if (fmTreeSize(&cacls->policers[i].acl) == 0)
        {
            err = fm10000GetPolicerOwnership(sw, &owner, i);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            if (owner == FM_FFU_OWNER_ACL)
            {
                err = fm10000SetPolicerOwnership(sw, FM_FFU_OWNER_NONE, i);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }
    }

    /**************************************************
     * Return compiler statistics.
     **************************************************/
    if (stats)
    {
        FillCompileStats(switchExt->appliedAcls);
        FM_MEMCPY_S( stats,
                     sizeof(fm_aclCompilerStats),
                     &switchExt->appliedAcls->compilerStats,
                     sizeof(fm_aclCompilerStats) );
    }

ABORT:

    /* Restart Traffic */
    if (zeroPortMaskInit)
    {
        fmDeleteBitArray(&zeroPortMask);
    }

    if (originalPortMask)
    {
        for (cpi = 0; cpi < numPorts; cpi++)
        {
            port = GET_LOGICAL_PORT(sw, cpi);
            if (switchPtr->perLagMgmt && fmPortIsInALAG(sw, port))
            {
                port = fmGetLAGForPort(sw, port);
            }

            /* Make sure the BitArray is initialized */
            if (originalPortMask[cpi].bitCount)
            {
                fm10000SetPortAttribute(sw,
                                        port,
                                        FM_PORT_ACTIVE_MAC,
                                        FM_PORT_LANE_NA,
                                        FM_PORT_MASK_WIDE,
                                        &originalPortMask[cpi]);

                fmDeleteBitArray(&originalPortMask[cpi]);
            }
        }

        fmFree(originalPortMask);
        FM_DROP_PORT_ATTR_LOCK(sw);
    }

    info = &switchPtr->aclInfo;

    /* Reinitialize the apply pending lists */
    for (fmTreeIterInit(&itAcl, &info->acls) ;
         (fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
    {
        acl = (fm_acl *) nextValue;

        if (fmTreeSize(&acl->addedRules))
        {
            fmTreeDestroy(&acl->addedRules, NULL);
            fmTreeInit(&acl->addedRules);
        }

        if (fmTreeSize(&acl->removedRules))
        {
            fmTreeDestroy(&acl->removedRules, NULL);
            fmTreeInit(&acl->removedRules);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ACLApplyExt */




/*****************************************************************************/
/** fm10000GetACLCountExt
 * \ingroup intAcl
 *
 * \desc            Retrieve the frame and octet counts associated with an
 *                  FM_ACL_ACTION_COUNT or FM_ACL_ACTIONEXT_COUNT ACL rule.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number for which to retrive the count.
 *
 * \param[in]       rule is the rule number for which to retrive the count.
 *
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_aclCounters'' where this function should place the
 *                  counter values.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is not a counting rule.
 *
 *****************************************************************************/
fm_status fm10000GetACLCountExt(fm_int          sw,
                                fm_int          acl,
                                fm_int          rule,
                                fm_aclCounters *counters)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls *aacls = switchExt->appliedAcls;
    void *nextValue;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_int i;
    fm_uint64 frameCount;
    fm_uint64 byteCount;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, acl = %d, rule = %d, counters = %p\n",
                 sw, acl, rule, (void*) counters);

    if (aacls == NULL)
    {
        err = FM_ERR_NO_ACLS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Is this ACL Id refer to an ingress ACL? */
    aclTree = &aacls->ingressAcl;
    err = fmGetAclNumKey(aclTree, acl, rule, &aclNumKey);

    if (err == FM_OK)
    {
        err = fmTreeFind(aclTree, aclNumKey, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        err = fmTreeFind(&compiledAcl->rules, rule, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

        counters->cntPkts = compiledAclRule->cntAdjustPkts;
        counters->cntOctets = compiledAclRule->cntAdjustOctets;
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            if (compiledAclRule->policerIndex[i] != 0)
            {
                err = fmTreeFind(&aacls->policers[i].policerEntry,
                                 compiledAclRule->policerIndex[i],
                                 &nextValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                if (compiledPolEntry->countEntry)
                {
                    err = fm10000GetPolicerCounter(sw,
                                                   i,
                                                   compiledAclRule->policerIndex[i],
                                                   &frameCount,
                                                   &byteCount);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    counters->cntPkts += frameCount;
                    counters->cntOctets += byteCount;
                    break;
                }
            }
        }

        /* Counter Entry not found */
        if (i == FM_FM10000_POLICER_BANK_MAX)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetACLCountExt */




/*****************************************************************************/
/** fm10000ResetACLCount
 * \ingroup intAcl
 *
 * \desc            Reset an ACL counter.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number for which to reset the count.
 *
 * \param[in]       rule is the rule number for which to reset the count.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if rule is not a counting rule.
 *
 *****************************************************************************/
fm_status fm10000ResetACLCount(fm_int sw,
                               fm_int acl,
                               fm_int rule)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls *aacls = switchExt->appliedAcls;
    void *nextValue;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledPolicerEntry *compiledPolEntry;
    fm_int i;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d, acl = %d, rule = %d\n",
                 sw, acl, rule);

    if (aacls == NULL)
    {
        err = FM_ERR_NO_ACLS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Is this ACL Id refer to an ingress ACL? */
    aclTree = &aacls->ingressAcl;
    err = fmGetAclNumKey(aclTree, acl, rule, &aclNumKey);

    if (err == FM_OK)
    {
        err = fmTreeFind(aclTree, aclNumKey, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
        err = fmTreeFind(&compiledAcl->rules, rule, &nextValue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

        compiledAclRule->cntAdjustPkts = 0LL;
        compiledAclRule->cntAdjustOctets = 0LL;
        for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
        {
            if (compiledAclRule->policerIndex[i] != 0)
            {
                err = fmTreeFind(&aacls->policers[i].policerEntry,
                                 compiledAclRule->policerIndex[i],
                                 &nextValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                if (compiledPolEntry->countEntry)
                {
                    err = fm10000SetPolicerCounter(sw,
                                                   i,
                                                   compiledAclRule->policerIndex[i],
                                                   0LL,
                                                   0LL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                    break;
                }
            }
        }

        /* Counter Entry not found */
        if (i == FM_FM10000_POLICER_BANK_MAX)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ResetACLCount */




/*****************************************************************************/
/** fm10000GetACLEgressCount
 * \ingroup intAcl
 *
 * \desc            Return the value of the egress ACL counter for a
 *                  specific port.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the port to get the counter for.
 *
 * \param[out]      counters points to a caller-allocated structure of type
 *                  ''fm_aclCounters'' where this function should place the
 *                  counter values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetACLEgressCount(fm_int          sw,
                                   fm_int          logicalPort,
                                   fm_aclCounters *counters)
{
    fm_status err;
    fm_int physicalPort;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d, logicalPort = %d\n",
                 sw, logicalPort);

    err = fmMapLogicalPortToPhysical(GET_SWITCH_PTR(sw),
                                     logicalPort,
                                     &physicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fm10000GetFFUEaclCounter(sw,
                                   physicalPort,
                                   &counters->cntPkts);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetACLEgressCount */




/*****************************************************************************/
/** fm10000ResetACLEgressCount
 * \ingroup intAcl
 *
 * \desc            Clear the egress ACL counter for a
 *                  specific port.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the port to clear the counter for.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ResetACLEgressCount(fm_int sw,
                                     fm_int logicalPort)
{
    fm_status err;
    fm_int physicalPort;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d, logicalPort = %d\n",
                 sw, logicalPort);

    err = fmMapLogicalPortToPhysical(GET_SWITCH_PTR(sw),
                                     logicalPort,
                                     &physicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fm10000SetFFUEaclCounter(sw, physicalPort, 0);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ResetACLEgressCount */




/*****************************************************************************/
/** fm10000AclInit
 * \ingroup intAcl
 *
 * \desc            Initialize the compiledAcls members of the FM10000 switch
 *                  extension.
 *
 * \note            This function is called from fm10000InitSwitch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000AclInit(fm_int sw)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status       err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d\n",
                 sw);

    if (switchExt->appliedAcls != NULL)
    {
        FreeCompiledAclsStruct(switchExt->appliedAcls);
        switchExt->appliedAcls = NULL;
    }

    if (switchExt->compiledAcls != NULL)
    {
        FreeCompiledAclsStruct(switchExt->compiledAcls);
        switchExt->compiledAcls = NULL;
    }

    switchExt->appliedAcls = fmAlloc( sizeof(fm_fm10000CompiledAcls) );

    if (switchExt->appliedAcls == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }
    FM_CLEAR(*switchExt->appliedAcls);

    InitializeCompiledAcls(sw, switchExt->appliedAcls);

    switchExt->aclPrecedenceMin =
        GET_FM10000_PROPERTY()->ffuAclPrecedenceMin;

    switchExt->aclPrecedenceMax =
        GET_FM10000_PROPERTY()->ffuAclPrecedenceMax;

    switchExt->aclStrictCount =
        GET_FM10000_PROPERTY()->ffuAclStrictCountPolice;

    
ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000AclInit */




/*****************************************************************************/
/** fm10000AclFree
 * \ingroup intAcl
 *
 * \desc            Free the memory used by the compiledAcls members of the
 *                  FM10000 switch extension.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000AclFree(fm_int sw)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d\n",
                 sw);

    FreeCompiledAclsStruct(switchExt->compiledAcls);
    switchExt->compiledAcls = NULL;
    FreeCompiledAclsStruct(switchExt->appliedAcls);
    switchExt->appliedAcls = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);

}   /* end fm10000AclFree */




/*****************************************************************************/
/** fm10000UpdateACLRule
 * \ingroup intAcl
 *
 * \desc            Update a rule with the given condition, value, action and
 *                  option to an ACL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number (0 to ''FM_MAX_ACLS'').
 *
 * \param[in]       rule is the rule number (0 to ''FM_MAX_ACL_RULES'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 *
 *****************************************************************************/
fm_status fm10000UpdateACLRule(fm_int sw,
                               fm_int acl,
                               fm_int rule)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls* compiledClone;
    fm_aclInfo *info;
    fm_acl *aclEntry;
    fm_status err = FM_OK;
    fm_int internalAcl;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "acl = %d, "
                 "rule = %d\n",
                 sw,
                 acl,
                 rule);

    /* Invalidate the standard ACL image */
    if (switchExt->compiledAcls != NULL)
    {
        switchExt->compiledAcls->valid = FALSE;
    }

    /* Try to update the rule without shifting everything around if the
     * inserted conditions are already part of the current acl configuration. */
    err = fm10000NonDisruptQuickUpdate(sw, acl, rule);
    if ( (err == FM_OK) ||
         (err != FM_ERR_INVALID_ACL_RULE) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ACL, err);
    }

    /* This update needs full recompile/apply to complete. */
    compiledClone = CloneCompiledAcls(sw, switchExt->appliedAcls);

    info = &switchPtr->aclInfo;

    if (compiledClone)
    {
        err = fmTreeFind(&info->acls, acl, (void**) &aclEntry);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Trig a remove/insert sequence of the rule */
        err = fmTreeInsert(&aclEntry->removedRules, rule, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fmTreeInsert(&aclEntry->addedRules, rule, NULL);
        if (err != FM_OK)
        {
            fmTreeRemoveCertain(&aclEntry->removedRules, rule, NULL);
        }

        /* Internal ACL needs to be specified */
        if (aclEntry->internal)
        {
            internalAcl = acl;
        }
        else
        {
            internalAcl = -1;
        }

        /* Virtually try it */
        err = fm10000NonDisruptCompile(sw,
                                       compiledClone,
                                       internalAcl,
                                       FALSE);
        if (err != FM_OK)
        {
            fmTreeRemove(&aclEntry->removedRules, rule, NULL);
            fmTreeRemove(&aclEntry->addedRules, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }

        /* Apply it for real */
        err = fm10000NonDisruptCompile(sw,
                                       switchExt->appliedAcls,
                                       internalAcl,
                                       TRUE);
        if (err != FM_OK)
        {
            fmTreeRemove(&aclEntry->removedRules, rule, NULL);
            fmTreeRemove(&aclEntry->addedRules, rule, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
    }
    else
    {
        err = FM_ERR_ACL_COMPILE;
    }

ABORT:

    FreeCompiledAclsStruct(compiledClone);

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000UpdateACLRule */




/*****************************************************************************/
/** fm10000SetACLRuleState
 * \ingroup intAcl
 *
 * \desc            Update the valid state of a rule.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number (0 to ''FM_MAX_ACLS'').
 *
 * \param[in]       rule is the rule number (0 to ''FM_MAX_ACL_RULES'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ACL if acl does not exist.
 * \return          FM_ERR_INVALID_ACL_RULE if rule is not valid.
 *
 *****************************************************************************/
fm_status fm10000SetACLRuleState(fm_int sw,
                                 fm_int acl,
                                 fm_int rule)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_aclInfo *info;
    fm_acl *aclEntry;
    fm_aclRule *aclRuleEntry;
    fm_status err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, "
                 "acl = %d, "
                 "rule = %d\n",
                 sw,
                 acl,
                 rule);

    /* Invalidate the standard ACL image */
    if (switchExt->compiledAcls != NULL)
    {
        switchExt->compiledAcls->valid = FALSE;
    }

    info = &switchPtr->aclInfo;

    err = fmTreeFind(&info->acls, acl, (void**) &aclEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmTreeFind(&aclEntry->rules, rule, (void**) &aclRuleEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    /* Is this ACL Id refer to an ingress ACL? */
    aclTree = &switchExt->appliedAcls->ingressAcl;
    err = fmGetAclNumKey(aclTree, acl, rule, &aclNumKey);

    if (err == FM_OK)
    {
        err = fmTreeFind(aclTree,
                         aclNumKey,
                         (void**) &compiledAcl);
    }
    else if (err == FM_ERR_NOT_FOUND)
    {
        err = fmTreeFind(&switchExt->appliedAcls->egressAcl,
                         acl,
                         (void**) &compiledAcl);
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    err = fmTreeFind(&compiledAcl->rules,
                     rule,
                     (void**) &compiledAclRule);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    compiledAclRule->valid = aclRuleEntry->state;

    err = fm10000SetFFURuleValid(sw,
                                 &compiledAcl->sliceInfo,
                                 compiledAclRule->physicalPos,
                                 compiledAclRule->valid,
                                 TRUE);
ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000SetACLRuleState */




/*****************************************************************************/
/** fm10000NotifyAclEcmpChange
 * \ingroup intAcl
 *
 * \desc            Update ACL with route action that refer to a modified
 *                  ECMP group resulting in a modification of the ARP index
 *                  entry or the number of ARP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group to update.
 * 
 * \param[in]       oldIndex is the old ARP index used for this group.
 *
 * \param[in]       newIndex is the new ARP index to use for this group.
 *
 * \param[in]       pathCount is the new ARP count to configure for this group.
 * 
 * \param[in]       pathCountType is the new ARP count type to configure for
 *                  this group.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NotifyAclEcmpChange(fm_int    sw,
                                     fm_int    groupId,
                                     fm_int    oldIndex,
                                     fm_uint16 newIndex,
                                     fm_int    pathCount,
                                     fm_int    pathCountType)
{
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status       err = FM_OK;
    fm_dlist *      ecmpList;
    fm_dlist_node * node;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_int i;
    fm_fm10000AclRule *ecmpRule;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, groupId = %d, oldIndex = %d, newIndex = %d, "
                 "pathCount = %d, pathCountType = %d\n",
                 sw,
                 groupId,
                 oldIndex,
                 newIndex,
                 pathCount,
                 pathCountType);

    FM_NOT_USED(oldIndex);

    /* Update the compiled ACL */
    if (switchExt->compiledAcls != NULL)
    {
        err = fmTreeFind(&switchExt->compiledAcls->ecmpGroups,
                         groupId,
                         (void**) &ecmpList);
        if (err == FM_OK)
        {
            node = FM_DLL_GET_FIRST( (ecmpList), head );

            while (node != NULL)
            {
                ecmpRule = (fm_fm10000AclRule *) node->data;

                /* Is this ACL Id refer to an ingress ACL? */
                aclTree = &switchExt->compiledAcls->ingressAcl;
                err = fmGetAclNumKey(aclTree,
                                     ecmpRule->aclNumber,
                                     ecmpRule->ruleNumber,
                                     &aclNumKey);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fmTreeFind(aclTree,
                                 aclNumKey,
                                 (void**) &compiledAcl);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fmTreeFind(&compiledAcl->rules,
                                 ecmpRule->ruleNumber,
                                 (void**) &compiledAclRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if (compiledAclRule->actions[i].action == FM_FFU_ACTION_ROUTE_ARP)
                    {
                        if (pathCount > 0)
                        {
                            compiledAclRule->actions[i].data.arp.arpIndex = newIndex;
                            compiledAclRule->actions[i].data.arp.count = pathCount;
                            compiledAclRule->actions[i].data.arp.arpType = pathCountType;
                        }
                        else
                        {
                            compiledAclRule->actions[i].data.arp.arpIndex = 0;
                            compiledAclRule->actions[i].data.arp.count = 1;
                            compiledAclRule->actions[i].data.arp.arpType = 0;
                        }
                        
                        break;
                    }
                }

                node = FM_DLL_GET_NEXT(node, next);
            }
        }
        else if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Update the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        err = fmTreeFind(&switchExt->appliedAcls->ecmpGroups,
                         groupId,
                         (void**) &ecmpList);
        if (err == FM_OK)
        {
            node = FM_DLL_GET_FIRST( (ecmpList), head );

            while (node != NULL)
            {
                ecmpRule = (fm_fm10000AclRule *) node->data;

                /* Is this ACL Id refer to an ingress ACL? */
                aclTree = &switchExt->appliedAcls->ingressAcl;
                err = fmGetAclNumKey(aclTree,
                                     ecmpRule->aclNumber,
                                     ecmpRule->ruleNumber,
                                     &aclNumKey);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fmTreeFind(aclTree,
                                 aclNumKey,
                                 (void**) &compiledAcl);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                err = fmTreeFind(&compiledAcl->rules,
                                 ecmpRule->ruleNumber,
                                 (void**) &compiledAclRule);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if (compiledAclRule->actions[i].action == FM_FFU_ACTION_ROUTE_ARP)
                    {
                        if (pathCount > 0)
                        {
                            compiledAclRule->actions[i].data.arp.arpIndex = newIndex;
                            compiledAclRule->actions[i].data.arp.count = pathCount;
                            compiledAclRule->actions[i].data.arp.arpType = pathCountType;
                        }
                        else
                        {
                            compiledAclRule->actions[i].data.arp.arpIndex = 0;
                            compiledAclRule->actions[i].data.arp.count = 1;
                            compiledAclRule->actions[i].data.arp.arpType = 0;
                        }

                        err = fm10000SetFFURule(sw,
                                                &compiledAcl->sliceInfo,
                                                compiledAclRule->physicalPos,
                                                compiledAclRule->valid,
                                                compiledAclRule->sliceKey,
                                                compiledAclRule->actions,
                                                FALSE, /* Live */
                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
                        break;
                    }
                }

                node = FM_DLL_GET_NEXT(node, next);
            }
        }
        else if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000NotifyAclEcmpChange */




/*****************************************************************************/
/** fm10000ValidateAclTriggerId
 * \ingroup intAcl
 *
 * \desc            Go over all the ACL-Rule Compiler and Applied and try to
 *                  match at least one of them with the FFU Trigger Id entered.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       trigId is the FFU Trigger Id to match on.
 *
 * \param[out]      referenced is set if at least one ACL refer to this FFU
 *                  Trigger Id.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ValidateAclTriggerId(fm_int   sw,
                                      fm_int   trigId,
                                      fm_bool *referenced)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, trigId = 0x%x\n",
                 sw,
                 trigId);

#if 0
    /* Validate the compiled ACL */
    if (switchExt->compiledAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->compiledAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_SET_TRIGGER) &&
                         (compiledAclRule->actions[i].data.trigger.value == trigId) )
                    {
                        *referenced = TRUE;
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }
#endif

    /* Only Validate the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_SET_TRIGGER) &&
                         (compiledAclRule->actions[i].data.trigger.value == trigId) )
                    {
                        *referenced = TRUE;
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }

    /* Did not found any ACL-rule that reference this Trigger Id. */
    *referenced = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ValidateAclTriggerId */




/*****************************************************************************/
/** fm10000ValidateAclArpBaseIndexId
 * \ingroup intAcl
 *
 * \desc            Go over all the ACL-Rule Compiler and Applied and try to
 *                  match at least one of them with the arpBlockIndex entered.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       arpBlockIndex is the arp index to match on.
 *
 * \param[out]      referenced is set if at least one ACL refer to this arp
 *                  index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ValidateAclArpBaseIndexId(fm_int    sw,
                                           fm_uint16 arpBlockIndex,
                                           fm_bool * referenced)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, arpBlockIndex = 0x%x\n",
                 sw,
                 arpBlockIndex);

    /* Only Validate the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_ROUTE_ARP) &&
                         (compiledAclRule->actions[i].data.arp.arpIndex == arpBlockIndex) )
                    {
                        *referenced = TRUE;
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }

    /* Did not found any ACL-rule that reference this arp index. */
    *referenced = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ValidateAclArpBaseIndexId */




/*****************************************************************************/
/** fm10000ValidateAclEcmpId
 * \ingroup intAcl
 *
 * \desc            Go over all the ACL-Rule Compiler and Applied and try to
 *                  match at least one of them with the arpBlockIndex entered.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       groupId is the ECMP group ID.
 *
 * \param[out]      referenced is set if at least one ACL refer to this arp
 *                  index.
 * 
 * \param[out]      acl is optional and can be used to retrieve the first
 *                  acl that refer to this ecmp group. Set it to NULL if
 *                  this information is not important.
 * 
 * \param[out]      rule is optional and can be used to retrieve the first
 *                  rule that refer to this ecmp group. Set it to NULL if
 *                  this information is not important.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ValidateAclEcmpId(fm_int   sw,
                                   fm_int   groupId,
                                   fm_bool *referenced,
                                   fm_int  *acl,
                                   fm_int  *rule)
{
    fm_switch *        switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *   switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status          err = FM_OK;
    void *             nextValue;
    fm_fm10000AclRule *ecmpRule;
    fm_dlist *         ecmpList;
    fm_dlist_node *    node;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, groupId = 0x%x\n",
                 sw,
                 groupId);

    /* Only Validate the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        err = fmTreeFind(&switchExt->appliedAcls->ecmpGroups,
                         groupId,
                         &nextValue);
        if (err == FM_OK)
        {
            *referenced = TRUE;

            if ((acl != NULL) || (rule != NULL))
            {
                ecmpList = (fm_dlist *) nextValue;
                node = FM_DLL_GET_FIRST( (ecmpList), head );
                if (node != NULL)
                {
                    ecmpRule = (fm_fm10000AclRule *) node->data;

                    if (acl)
                    {
                        *acl = ecmpRule->aclNumber;
                    }

                    if (rule)
                    {
                        *rule = ecmpRule->ruleNumber;
                    }
                }
            }
            FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
        }
        else if (err == FM_ERR_NOT_FOUND)
        {
            *referenced = FALSE;
            FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
        }
    }

    /* Did not found any ACL-rule that reference this ecmp group. */
    *referenced = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ValidateAclEcmpId */




/*****************************************************************************/
/** fm10000ValidateAclLogicalPort
 * \ingroup intAcl
 *
 * \desc            Go over all the Applied ACL-Rules and returns whether 
 *                  a logical port is used in the ACL Route action.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       logicalPort is the logical port to match on.
 *
 * \param[out]      referenced is set if at least one ACL refer to this 
 *                  logical port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ValidateAclLogicalPort(fm_int      sw,
                                        fm_int      logicalPort,
                                        fm_bool *   referenced)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, logicalPort = %d\n",
                 sw,
                 logicalPort);

    /* Only Validate the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_ROUTE_LOGICAL_PORT) &&
                         (compiledAclRule->actions[i].data.logicalPort == logicalPort) )
                    {
                        *referenced = TRUE;
                        FM_LOG_EXIT(FM_LOG_CAT_ACL, FM_OK);
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }

    /* Did not find any ACL-rule that reference this logical port. */
    *referenced = FALSE;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ValidateAclLogicalPort */




/*****************************************************************************/
/** fm10000UpdateAclTriggerMask
 * \ingroup intAcl
 *
 * \desc            Update ACL with trigger action that refer to a modified
 *                  FFU Trigger Mask.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       oldMask is the original FFU Trigger Mask to match on.
 *
 * \param[in]       newMask is the new FFU Trigger Mask to use.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateAclTriggerMask(fm_int sw,
                                      fm_int oldMask,
                                      fm_int newMask)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_treeIterator itRule;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint64 ruleNumber;
    fm_int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, newMask = 0x%x\n",
                 sw,
                 newMask);

    /* Update the compiled ACL */
    if (switchExt->compiledAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->compiledAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_SET_TRIGGER) &&
                         (compiledAclRule->actions[i].data.trigger.mask == oldMask) )
                    {
                        compiledAclRule->actions[i].data.trigger.mask = newMask;
                        break;
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }

    /* Update the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            for (fmTreeIterInit(&itRule, &compiledAcl->rules) ;
                 (err = fmTreeIterNext(&itRule, &ruleNumber, &nextValue)) ==
                        FM_OK ; )
            {
                compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

                for (i = 0 ; i < compiledAclRule->numActions ; i++)
                {
                    if ( (compiledAclRule->actions[i].action == FM_FFU_ACTION_SET_TRIGGER) &&
                         (compiledAclRule->actions[i].data.trigger.mask == oldMask) )
                    {
                        compiledAclRule->actions[i].data.trigger.mask = newMask;

                        err = fm10000SetFFURule(sw,
                                                &compiledAcl->sliceInfo,
                                                compiledAclRule->physicalPos,
                                                compiledAclRule->valid,
                                                compiledAclRule->sliceKey,
                                                compiledAclRule->actions,
                                                FALSE, /* Live */
                                                TRUE);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                        break;
                    }
                }
            }   /* end for (fmTreeIterInit(&itRule, &compiledAcl->rules) ... */

            if (err != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
            }
        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000UpdateAclTriggerMask */




/*****************************************************************************/
/** fm10000GetACLInUseSliceCount
 * \ingroup intAcl
 *
 * \desc            Returns the number of TCAM slices that are in use for ACLs.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          Number of TCAM slices currently being used for ACLs.
 *
 *****************************************************************************/
fm_int fm10000GetACLInUseSliceCount(fm_int sw)
{
    fm_status      err = FM_OK;
    fm_int         firstSlice;
    fm_int         lastSlice;
    fm_switch *    switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *switchExt = (fm10000_switch *) switchPtr->extension;
    fm_int         slicesUsed = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d\n",
                 sw);

    if (switchExt->appliedAcls != NULL)
    {
        err = GetSliceUsage(switchExt->appliedAcls, &firstSlice, &lastSlice);
        if (err == FM_OK)
        {
            slicesUsed = (lastSlice - firstSlice) + 1;
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ACL,
                       slicesUsed,
                       "slicesUsed = %d\n",
                       slicesUsed);

}   /* end fm10000GetACLInUseSliceCount */




/*****************************************************************************/
/** fm10000AclProcessFFUPartitionChange
 * \ingroup intAcl
 *
 * \desc            Determines whether ACLs are affected by an FFU partition
 *                  change. If they are, the operation is rejected.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       newAllocations points to the record containing the new
 *                  FFU slice assignments.
 *
 * \param[in]       simulated specifies whether to simulate the operation or
 *                  perform it for real.  TRUE means simulate, FALSE means
 *                  real.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INSUFFICIENT_ACL_SPACE if the new FFU allocations
 *                  don't provide sufficient space for existing ACLs.
 * \return          FM_ERR_TRAFFIC_STOP_REQUIRED if the new FFU allocations
 *                  would require a traffic stop (i.e., stop traffic, change
 *                  the allocations, reapply the ACLs, then restart traffic).
 *
 *****************************************************************************/
fm_status fm10000AclProcessFFUPartitionChange(fm_int                        sw,
                                              const fm_ffuSliceAllocations *newAllocations,
                                              fm_bool                       simulated)
{
    fm_status      err = FM_OK;
    fm_int         firstSlice;
    fm_int         lastSlice;
    fm_switch *    switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch *switchExt = (fm10000_switch *) switchPtr->extension;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, newAllocations = %p, simulated = %d\n",
                 sw,
                 (void *) newAllocations,
                 simulated);

    FM_NOT_USED(simulated);

    if (switchExt->appliedAcls != NULL)
    {
        err = GetSliceUsage(switchExt->appliedAcls, &firstSlice, &lastSlice);
        if (err == FM_OK)
        {
            if ( (newAllocations->aclFirstSlice > firstSlice) ||
                 (newAllocations->aclLastSlice < lastSlice) )
            {
                err = FM_ERR_INSUFFICIENT_ACL_SPACE;
            }
        }
        else
        {
            err = FM_OK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000AclProcessFFUPartitionChange */




/*****************************************************************************/
/** fm10000GetACLRuleAttribute
 * \ingroup intAcl
 *
 * \desc            Get an attribute of a specific ACL rule.
 *
 * \note            Switch is assumed to already be validated and
 *                  protected, and ACL lock already acquired.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       acl is the ACL number.
 *
 * \param[in]       rule is the rule number.
 * 
 * \param[in]       attr is the ACL Rule attribute to get
 *                  (see ''ACL Rule Attributes'').
 *
 * \param[out]      value points to caller-supplied storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_ACLS if no matching ACLs found.
 * \return          FM_ERR_NOT_FOUND if not key was not found.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid attribute.
 *
 *****************************************************************************/
fm_status fm10000GetACLRuleAttribute(fm_int sw,
                                     fm_int acl,
                                     fm_int rule,
                                     fm_int attr,
                                     void * value)
{
    fm_status err = FM_OK;
    fm_switch *     switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_fm10000CompiledAcls *aacls = switchExt->appliedAcls;
    void *nextValue;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_fm10000CompiledAclRule* compiledAclRule;
    fm_fm10000CompiledPolicerEntry* compiledPolEntry;
    fm_int i;
    fm_uint64 aclNumKey;
    fm_tree *aclTree;
    fm_aclRulePolicerInfo *ruleInfo; 

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, acl = %d, rule = %d, attr = %d, value = %p\n",
                 sw, acl, rule, attr, (void*) value);

    if (aacls == NULL)
    {
        err = FM_ERR_NO_ACLS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    if (attr == FM_ACL_RULE_POLICER_INFO)
    {
        ruleInfo = (fm_aclRulePolicerInfo *) value;

        /* Is this ACL Id refer to an ingress ACL? */
        aclTree = &aacls->ingressAcl;
        err = fmGetAclNumKey(aclTree, acl, rule, &aclNumKey);

        if (err == FM_OK)
        {
            err = fmTreeFind(aclTree, aclNumKey, &nextValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;
            err = fmTreeFind(&compiledAcl->rules, rule, &nextValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

            compiledAclRule = (fm_fm10000CompiledAclRule*) nextValue;

            for (i = 0 ; i < FM_FM10000_POLICER_BANK_MAX ; i++)
            {
                ruleInfo->type[i]  = FM_ACL_BANK_TYPE_NONE;
                ruleInfo->index[i] = 0;

                if (compiledAclRule->policerIndex[i] != 0)
                {
                    err = fmTreeFind(&aacls->policers[i].policerEntry,
                                     compiledAclRule->policerIndex[i],
                                     &nextValue);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

                    compiledPolEntry = (fm_fm10000CompiledPolicerEntry*) nextValue;

                    if (compiledPolEntry->countEntry)
                    {
                        ruleInfo->type[i]  = FM_ACL_BANK_TYPE_COUNT;
                        ruleInfo->index[i] = compiledAclRule->policerIndex[i];
                    }
                    else
                    {
                        ruleInfo->type[i]  = FM_ACL_BANK_TYPE_POLICE;
                        ruleInfo->index[i] = compiledAclRule->policerIndex[i];
                    }
                }
            }
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetACLRuleAttribute */




/*****************************************************************************/
/** fm10000GetAclTrapAlwaysId
 * \ingroup intAcl
 *
 * \desc            This function gets the FFU Trigger Id of the Trap Always
 *                  ACL Action Trigger. This function also creates and
 *                  configure such trigger if not available.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      ffuResId is used to return the FFU Trigger Id.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetAclTrapAlwaysId(fm_int sw, fm_int *ffuResId)
{
    fm_switch *         switchPtr;
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_uint32           trigBitValue;
    fm_status err = FM_OK;
    fm_bool trigCreated = FALSE;
    fm_bool ffuTrigResAllocated = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL, "sw = %d\n", sw);

    err = fm10000GetTrigger(sw,
                            FM10000_TRIGGER_GROUP_ACL_SPECIAL,
                            FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS,
                            &trigCond,
                            &trigAction);
    /* Trigger not configured */
    if (err == FM_ERR_INVALID_TRIG)
    {
        /* Need one FFU Trigger Id bit to carry this action from FFU to
         * Trigger */
        err = fm10000AllocateTriggerResource(sw,
                                             FM_TRIGGER_RES_FFU,
                                             &trigBitValue,
                                             TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        ffuTrigResAllocated = TRUE;

        /* Trigger that match on ACL action FM_ACL_ACTIONEXT_TRAP_ALWAYS */
        err = fm10000CreateTrigger(sw,
                                   FM10000_TRIGGER_GROUP_ACL_SPECIAL,
                                   FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS,
                                   TRUE,
                                   "ACLTrapAlwaysTrigger");
        /* Free Trigger slot available? */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        trigCreated = TRUE;

        /* Configure Trigger action */
        err = fmInitTriggerAction(sw, &trigAction);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        switchPtr = GET_SWITCH_PTR(sw);
        trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_REDIRECT;
        trigAction.param.newDestGlort = (switchPtr->glortInfo.cpuBase & 0xFF00) | FM10000_TRAP_FFU;
        trigAction.param.newDestGlortMask = 0xFFFF;
        trigAction.param.newDestPortset = FM_PORT_SET_CPU;

        err = fm10000SetTriggerAction(sw,
                                      FM10000_TRIGGER_GROUP_ACL_SPECIAL,
                                      FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS,
                                      &trigAction,
                                      TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        /* Configure the Trigger Conditions */
        err = fmInitTriggerCondition(sw, &trigCond);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        trigCond.cfg.matchFFU = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
        trigCond.param.ffuId = (1 << trigBitValue);
        trigCond.param.ffuIdMask = (1 << trigBitValue);
        trigCond.cfg.matchFtypeMask = (FM_TRIGGER_FTYPE_NORMAL |
                                       FM_TRIGGER_FTYPE_SPECIAL);
        trigCond.cfg.HAMask = 0xffffffffffffffffLL;

        trigCond.cfg.rxPortset = FM_PORT_SET_ALL;
        trigCond.cfg.matchTx = FM_TRIGGER_TX_MASK_DOESNT_CONTAIN;
        trigCond.cfg.txPortset = FM_PORT_SET_NONE;

        err = fm10000SetTriggerCondition(sw,
                                         FM10000_TRIGGER_GROUP_ACL_SPECIAL,
                                         FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS,
                                         &trigCond,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

        err = fm10000NotifyFloodingTrapAlwaysId(sw, trigCond.param.ffuId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    }   /* end if (err == FM_ERR_INVALID_TRIG) */

    *ffuResId = trigCond.param.ffuId;

    /* Normal Path return here */
    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

ABORT:

    /* Error handling */
    if (ffuTrigResAllocated)
    {
        fm10000FreeTriggerResource(sw, FM_TRIGGER_RES_FFU, trigBitValue, TRUE);
    }

    if (trigCreated)
    {
        fm10000DeleteTrigger(sw,
                             FM10000_TRIGGER_GROUP_ACL_SPECIAL,
                             FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS,
                             TRUE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetAclTrapAlwaysId */




/*****************************************************************************/
/** fm10000ValidateACLAttribute
 * \ingroup intAcl
 *
 * \desc            Determine whether the specified ACL attribute is
 *                  implementable on this switch model.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the ACL attribute (see 'ACL Attributes') to set.
 *
 * \return          FM_OK if rule is implementable
 * \return          FM_ERR_UNSUPPORTED if rule is not implementable
 *
 *****************************************************************************/
fm_status fm10000ValidateACLAttribute(fm_int sw, fm_int attr)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, attr = %d\n",
                 sw,
                 attr);

    FM_NOT_USED(sw);

    switch (attr)
    {
        case FM_ACL_KEEP_UNUSED_KEYS:
        case FM_ACL_INSTANCE:
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000ValidateACLAttribute */




/*****************************************************************************/
/** fm10000GetAclFfuRuleUsage
 * \ingroup intAcl
 *
 * \desc            Returns the number of FFU rules currently used by the ACL
 *                  subsystem and also the number of rules available. This
 *                  service is internally used by the mailbox API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      ffuRuleUsed points to caller-provided storage into
 *                  which the function writes the number of FFU rules currently
 *                  configured in the system.
 *
 * \param[out]      ffuRuleAvailable points to caller-provided storage into
 *                  which the function writes the number of FFU rules still
 *                  available.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetAclFfuRuleUsage(fm_int  sw,
                                    fm_int *ffuRuleUsed,
                                    fm_int *ffuRuleAvailable)
{
    fm_switch *      switchPtr = GET_SWITCH_PTR(sw);
    fm10000_switch * switchExt = (fm10000_switch *) switchPtr->extension;
    fm_status        err = FM_OK;
    fm_fm10000CompiledAcl* compiledAcl;
    fm_treeIterator itAcl;
    fm_uint64 aclNumber;
    void *nextValue;
    fm_uint32 numRules;
    fm_int firstUsedSlice;
    fm_int lastUsedSlice;
    fm_int firstAclSlice;
    fm_int lastAclSlice;

    FM_LOG_ENTRY(FM_LOG_CAT_ACL,
                 "sw = %d, ffuRuleUsed = %p, ffuRuleAvailable = %p\n",
                 sw,
                 (void *) ffuRuleUsed,
                 (void *) ffuRuleAvailable);

    *ffuRuleUsed      = 0;
    *ffuRuleAvailable = 0;
    firstUsedSlice   = -1;

    /* Compute usage on the applied ACL */
    if (switchExt->appliedAcls != NULL)
    {
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->ingressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            numRules = fmTreeSize(&compiledAcl->rules);
            if (numRules)
            {
                *ffuRuleUsed += numRules;

                /* Compute the number of rules of the last part */
                numRules = (numRules % FM10000_MAX_RULE_PER_ACL_PART);
                if (numRules)
                {
                    *ffuRuleAvailable += (FM10000_MAX_RULE_PER_ACL_PART -
                                          numRules);
                }
            }

        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }

        numRules = 0;
        for (fmTreeIterInit(&itAcl, &switchExt->appliedAcls->egressAcl) ;
             (err = fmTreeIterNext(&itAcl, &aclNumber, &nextValue)) == FM_OK ; )
        {
            compiledAcl = (fm_fm10000CompiledAcl*) nextValue;

            numRules += fmTreeSize(&compiledAcl->rules);

        }   /* end for (fmTreeIterInit(&itAcl, &cacls->ingressAcl) ... */

        if (err != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
        }
        else
        {
            err = FM_OK;
        }

        /* Egress ACL all share the same FFU slice and can't be divided in
         * parts */
        if (numRules)
        {
            *ffuRuleUsed += numRules;
            *ffuRuleAvailable += (FM10000_MAX_RULE_PER_ACL_PART - numRules);
        }

        err = GetSliceUsage(switchExt->appliedAcls,
                            &firstUsedSlice,
                            &lastUsedSlice);
        if (err == FM_ERR_NO_ACLS)
        {
            firstUsedSlice = -1;
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);
    }

    /* Compute the free ffu slice area */
    err = fmGetFFUSliceRange(sw, &firstAclSlice, &lastAclSlice);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, err);

    if (firstUsedSlice >= 0)
    {
        lastAclSlice = firstUsedSlice - 1;
    }

    numRules = FM10000_MAX_RULE_PER_ACL_PART * (lastAclSlice - firstAclSlice + 1);

    /* ffuRuleAvailable is equal to: (number of rules available in the current
     * table) + free ACL slice * FM10000_MAX_RULE_PER_ACL_PART */
    *ffuRuleAvailable += numRules;


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ACL, err);

}   /* end fm10000GetAclFfuRuleUsage */
