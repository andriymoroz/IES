/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_mgmt.h
 * Creation Date:   June 2, 2014
 * Description:     Platform transceiver management functions.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#ifndef __FM_PLATFORM_XCVR_MGMT_H
#define __FM_PLATFORM_XCVR_MGMT_H

#define XCVR_EEPROM_CACHE_SIZE  128

typedef struct
{
    /* Current ETH mode set on the port associated to this transceiver */
    fm_ethMode ethMode;

    /* Indicates whether the transceiver is disabled in config */
    fm_bool disabled;

    /* Indicates whether module is present */
    fm_bool present;

    /* Indicates whether the transceiver has AN enabled */
    fm_bool anEnabled;

    /* Number of retries to config transceiver */
    fm_int configRetries;

    /* Indicates whether base eeprom checksum is valid */
    fm_bool eepromBaseValid;

    /* Indicates whether ext eeprom checksum is valid */
    fm_bool eepromExtValid;

    /* Number of retries to read EEPROM again */
    fm_int eepromReadRetries;

    /* Copy of the EEPROM content */
    fm_byte eeprom[XCVR_EEPROM_CACHE_SIZE];

    /* Current transceiver state (sideband signals) */
    fm_uint32 modState;

    /* Transceiver module type read from the eeprom */
    fm_platformXcvrType type;

    /* Transceiver cable length read from the eeprom */
    fm_int cableLength;

} fm_platXcvrInfo;

fm_status fmPlatformMgmtTakeSwitchLock(fm_int sw);
fm_status fmPlatformMgmtDropSwitchLock(fm_int sw);
fm_status fmPlatformMgmtInit(fm_int sw);
fm_status fmPlatformMgmtXcvrInitialize(fm_int sw);
void fmPlatformMgmtEnableInterrupt(fm_int sw);
void fmPlatformMgmtSignalInterrupt(fm_int sw, fm_int gpio);
void fmPlatformMgmtSignalPollingThread(fm_int sw);
void fmPlatformMgmtNotifyEthModeChange(fm_int sw, 
                                       fm_int port, 
                                       fm_ethMode mode);
fm_status fmPlatformMgmtGetTransceiverType(fm_int               sw,
                                           fm_int               port,
                                           fm_platformXcvrType *xcvrType,
                                           fm_int              *xcvrLen,
                                           fm_bool *            isOptical);

fm_status fmPlatformMgmtConfigSfppXcvrAutoNeg(fm_int sw,
                                              fm_int port,
                                              fm_bool enable);
fm_status fmPlatformMgmtEnableXcvr(fm_int sw, fm_int port, fm_bool enable);
fm_status fmPlatformMgmtDumpPort(fm_int sw, fm_int port);

fm_status fmPlatformMgmtEnableCableAutoDetection(fm_int  sw, 
                                                 fm_int  port, 
                                                 fm_bool enable);

#endif /* __FM_PLATFORM_XCVR_MGMT_H */
