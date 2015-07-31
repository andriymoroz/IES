/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_xcvr.h
 * Creation Date:   2013
 * Description:     Platform transceiver specific definitions
 *
 * Copyright (c) 2011 - 2013, Intel Corporation
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

#ifndef __FM_PLATFORM_XCVR_H
#define __FM_PLATFORM_XCVR_H

typedef enum
{
    FM_PLATFORM_XCVR_TYPE_NOT_PRESENT,    /* Transceiver is not present */

    FM_PLATFORM_XCVR_TYPE_UNKNOWN,        /* Unable to decode tranceiver type */

    FM_PLATFORM_XCVR_TYPE_1000BASE_T,     /* SFP 10/100/1000 BASE-T module */

    FM_PLATFORM_XCVR_TYPE_SFP_DAC,        /* 10G Direct Attach Copper cable */

    FM_PLATFORM_XCVR_TYPE_SFP_OPT,        /* 1G/10G Optical module */

    FM_PLATFORM_XCVR_TYPE_QSFP_DAC,       /* 40G Direct Attach Copper cable */

    FM_PLATFORM_XCVR_TYPE_QSFP_AOC,       /* 40G Active Optical Cable */

    FM_PLATFORM_XCVR_TYPE_QSFP_OPT,       /* 40G Optical Separated Module */

    FM_PLATFORM_XCVR_TYPE_QSFP28_DAC,     /* 100G Direct Attach Copper cable */

    FM_PLATFORM_XCVR_TYPE_QSFP28_AOC,     /* 100G Active Optical Cable */

    FM_PLATFORM_XCVR_TYPE_QSFP28_OPT,     /* 100G Optical Separated Module */
 
} fm_platformXcvrType;


fm_text fmPlatformXcvrTypeGetName(fm_platformXcvrType type);
fm_bool fmPlatformXcvrIsOptical(fm_platformXcvrType type);
fm_uint fmPlatformXcvrEepromIsBaseCsumValid(fm_byte *eeprom);
fm_uint fmPlatformXcvrEepromIsExtCsumValid(fm_byte *eeprom);

void fmPlatformXcvrEepromDumpBaseExt(fm_byte *eeprom, fm_bool qsfp);
void fmPlatformXcvrSfppEepromDumpPage1(fm_byte *eeprom);
void fmPlatformXcvrQsfpEepromDumpPage0(fm_byte *eeprom);

fm_uint fmPlatformXcvrEepromGetLen(fm_byte *eeprom);
fm_platformXcvrType fmPlatformXcvrEepromGetType(fm_byte *eeprom);
fm_bool fmPlatformXcvrIs1G(fm_byte *eeprom);
fm_bool fmPlatformXcvrIs10G1G(fm_byte *eeprom);
fm_bool fmPlatformXcvrIs1000BaseT(fm_byte *eeprom);

#endif /* __FM_PLATFORM_XCVR_H */
