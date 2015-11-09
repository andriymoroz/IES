/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_pca.h
 * Creation Date:   April 2015
 * Description:     GN2412 Retimer device functions
 *
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __FM_UTIL_GN2412_H
#define __FM_UTIL_GN2412_H

#define FM_GN2412_NUM_TIMEBASES         2
#define FM_GN2412_NUM_LANES             12

#define FM_GN2412_VERSION_NUM_LEN       4

#define FM_GN2412_DEF_LANE_POLARITY     5
#define FM_GN2412_DEF_LANE_PRE_TAP      0
#define FM_GN2412_DEF_LANE_ATT          15
#define FM_GN2412_DEF_LANE_POST_TAP     0

/* Current firmware version supported by the software.
   Note: the FW_NANO_REG and FW_VARIANT_REG are not checked.
 */
#define FM_GN2412_FW_VER_MAJOR          3
#define FM_GN2412_FW_VER_MINOR          9

typedef enum
{
    FM_GN2412_REFCLK_156P25, /* Reference clock input frequency: 156.25 MHz */

} fm_gn2412RefClk;

typedef enum
{
    FM_GN2412_OUTFREQ_5156P25, /* Timebase output frequency: 5156.25 MHz */

} fm_gn2412OutFreq;

/**************************************************
 * GN2412 Timebase registers structure
 **************************************************/
typedef struct _fm_gn2412TimebaseCfg
{
    fm_byte idac_ref_hi;
    fm_byte idac_ref_lo;
    fm_byte sel_div1;
    fm_byte sel_div_pll;
    fm_byte feedback_div_sels;
    fm_byte feedback_div_selp;
    fm_byte wordclk_sel;
    fm_byte sel_icharge_pump;
    fm_byte sel_cfilter;
    fm_byte sel_rfilter;
    fm_byte m_div_sel;
    fm_byte vco_freq_cal_4x;
    fm_byte vco_freq_cal_control;

    /* Timebase reference clock select. */
    fm_int refClkSel;

    /* Timebase reference clock input frequency. */
    fm_float refClk;

    /* Timebase output frequency. */
    fm_float outFreq;

} fm_gn2412TimebaseCfg;


typedef struct _fm_gn2412LaneCfg
{
    /* Tx polarity */
    fm_int polarity;

    /* Tx Pre-Tap coefficient */
    fm_int preTap;

    /* Tx attenuation */
    fm_int attenuation;

    /* Tx Post-Tap coefficient */
    fm_int postTap;

    /* Contains the Rx Port assigned as source to this Tx Port
       (TxPort == index to this structure) */
    fm_int rxPort;

    /* Application mode for this lane */
    fm_int appMode;

} fm_gn2412LaneCfg;


typedef struct _fm_gn2412Cfg
{
    /* Timebases configuration */
    fm_gn2412TimebaseCfg timebase[FM_GN2412_NUM_TIMEBASES];

    /* Lanes configuration */
    fm_gn2412LaneCfg lane[FM_GN2412_NUM_LANES];

    /* Indicate to configure TX equalization coefficients or not. */
    fm_bool setTxEq;

} fm_gn2412Cfg;


fm_status fmUtilGN2412GetTimebaseCfg(fm_int                timebase,
                                     fm_gn2412RefClk       refClk,
                                     fm_gn2412OutFreq      outFreq,
                                     fm_gn2412TimebaseCfg *tbCfg);

fm_status fmUtilGN2412GetFirmwareVersion(fm_uintptr                  handle,
                                         fm_utilI2cWriteReadHdnlFunc func,
                                         fm_uint                     dev,
                                         fm_byte *                   version);

fm_status fmUtilGN2412GetBootErrorCode(fm_uintptr                  handle,
                                       fm_utilI2cWriteReadHdnlFunc func,
                                       fm_uint                     dev,
                                       fm_byte *                   errCode0,
                                       fm_byte *                   errCode1);

fm_status fmUtilGN2412Initialize(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev,
                                 fm_gn2412Cfg *              cfg);

void fmUtilGN2412DumpConnections(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev);

void fmUtilGN2412DumpTxEqualization(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev);

fm_status fmUtilGN2412SetAppMode(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev,
                                 fm_int                      lane,
                                 fm_int                      mode);

void fmUtilGN2412DumpAppMode(fm_uintptr                  handle,
                             fm_utilI2cWriteReadHdnlFunc func,
                             fm_uint                     dev);

void fmUtilGN2412DumpAppStatus(fm_uintptr                  handle,
                               fm_utilI2cWriteReadHdnlFunc func,
                               fm_uint                     dev);
void fmUtilGN2412DumpAppStatusV2(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev);
void fmUtilGN2412DumpMissionCount(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev);

void fmUtilGN2412DumpAppRestartDiagCnt(fm_uintptr                  handle,
                                       fm_utilI2cWriteReadHdnlFunc func,
                                       fm_uint                     dev);
void fmUtilGN2412DumpAppRestartDiagCntV2(fm_uintptr                  handle,
                                         fm_utilI2cWriteReadHdnlFunc func,
                                         fm_uint                     dev);
fm_status fmUtilGN2412GetLaneTxEq(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev,
                                  fm_int                      lane,
                                  fm_int *                    polarity,
                                  fm_int *                    preTap,
                                  fm_int *                    attenuation,
                                  fm_int *                    postTap);

fm_status fmUtilGN2412SetLaneTxEq(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev,
                                  fm_int                      lane,
                                  fm_int                      polarity,
                                  fm_int                      preTap,
                                  fm_int                      attenuation,
                                  fm_int                      postTap);

fm_status fmUtilGN2412RegisterRead(fm_uintptr                  handle,
                                   fm_utilI2cWriteReadHdnlFunc func,
                                   fm_uint                     dev,
                                   fm_uint16                   reg,
                                   fm_byte *                   val);

fm_status fmUtilGN2412RegisterWrite(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev,
                                    fm_uint16                   reg,
                                    fm_byte                     val);

#endif /* __FM_UTIL_GN2412_H */
