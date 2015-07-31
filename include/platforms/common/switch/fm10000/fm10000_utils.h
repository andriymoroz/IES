/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_utils.h
 * Creation Date:   August 2014
 * Description:     Switch utitily functions.
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

#ifndef __FM_FM10000_UTILS_H
#define __FM_FM10000_UTILS_H

#include <api/internal/fm10000/fm10000_api_hw_int.h>

/* The number of custom MACs stored and loaded by the NVM */
#define FM10000_NUM_CUSTOM_MAC              4

/* The BSM Interrupt Module Enable Mask Bits*/
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_0         (1 << 0)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_1         (1 << 1)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_2         (1 << 2)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_3         (1 << 3)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_4         (1 << 4)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_5         (1 << 5)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_6         (1 << 6)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_7         (1 << 7)
#define FM10000_UTIL_PCIE_BSM_INT_PCIE_8         (1 << 8)
#define FM10000_UTIL_PCIE_BSM_INT_BSM            (1 << 9)
#define FM10000_UTIL_PCIE_BSM_INT_INT_PIN        (1 << 10)
#define FM10000_UTIL_PCIE_BSM_INT_FIBM           (1 << 11)

/* BSM interrupt mask */

#define FM10000_UTIL_PCIE_BSM_INT_MASK (FM10000_UTIL_PCIE_BSM_INT_PCIE_0 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_1 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_2 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_3 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_4 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_5 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_6 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_7 | \
                                        FM10000_UTIL_PCIE_BSM_INT_PCIE_8)


/* Interrupt status bits */
#define FM10000_UTIL_INT_STATUS_OTHER       0x1
#define FM10000_UTIL_INT_STATUS_BSM         0x2


#define BIST_MODULE_PCIE_1                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_1)
#define BIST_MODULE_PCIE_2                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_2)
#define BIST_MODULE_PCIE_3                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_3)
#define BIST_MODULE_PCIE_4                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_4)
#define BIST_MODULE_PCIE_5                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_5)
#define BIST_MODULE_PCIE_6                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_6)
#define BIST_MODULE_PCIE_7                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_7)
#define BIST_MODULE_PCIE_8                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_8)
#define BIST_MODULE_EPL                      (1 << FM10000_BIST_CTRL_b_BistRun_EPL)
#define BIST_MODULE_FABRIC                   (1 << FM10000_BIST_CTRL_b_BistRun_FABRIC)
#define BIST_MODULE_TUNNEL                   (1 << FM10000_BIST_CTRL_b_BistRun_TUNNEL)
#define BIST_MODULE_BSM                      (1 << FM10000_BIST_CTRL_b_BistRun_BSM)
#define BIST_MODULE_CRM                      (1 << FM10000_BIST_CTRL_b_BistRun_CRM)
#define BIST_MODULE_FIBM                     (1 << FM10000_BIST_CTRL_b_BistRun_FIBM)
#define BIST_MODULE_SBM                      (1 << FM10000_BIST_CTRL_b_BistRun_SBM)


/* Structure that holds all the run-time configurable boot parameters. For
 * details on the parameters, refer to the description of the Liberty Trail
 * Boot Config Attributes.
 * 
 * e.g.
 * api.platform.config.switch.%d.bootCfg.<parameter> 
 * api.platform.config.switch.%d.bootCfg.pep.%d.<parameter>
 * 
 * Where <parameter> is the name of the structure member. 
 */
typedef struct _fm10000_bootCfg
{
    /* General documented boot parameters
     *
     * api.platform.config.switch.%d.bootCfg.<parameter>
     */ 
    fm_uint32   spiTransferMode;
    fm_uint32   spiTransferSpeed;
    fm_int      mgmtPep;
    fm_bool     systimeClockSource;
    fm_text     customMac[FM10000_NUM_CUSTOM_MAC];

    /* Per-PEP parameters
     *
     * api.platform.config.switch.%d.bootCfg.pep.%d.<parameter>
     */
    fm_bool     pepMode[FM10000_NUM_PEPS];
    fm_bool     pepEnable[FM10000_NUM_PEPS];
    fm_int      pepNumberOfLanes[FM10000_NUM_PEPS];
    fm_text     pepSerialNumber[FM10000_NUM_PEPS];
    fm_int      pepSubVendorId[FM10000_NUM_PEPS];
    fm_int      pepSubDeviceId[FM10000_NUM_PEPS];
    
    /* Per-PEP parameters (undocumented boot parameters)
     *  
     * api.platform.config.switch.%d.bootCfg.pep.%d.<parameter>
     */
    fm_bool     pepBar4Allowed[FM10000_NUM_PEPS];
    fm_int      pepMasterLane[FM10000_NUM_PEPS];
    fm_bool     pepAutoLaneFlip[FM10000_NUM_PEPS];
    fm_bool     pepRxLaneFlip[FM10000_NUM_PEPS];
    fm_bool     pepTxLaneFlip[FM10000_NUM_PEPS];

    /* General Undocumented boot parameters
     *
     * api.platform.config.switch.%d.bootCfg.<parameter>
     */
    fm_bool     skipPcieInitialization;
    fm_bool     skipMemRepair;
    fm_bool     enableBistCheck;
    fm_bool     enableFwVerify;

    /* Values here are programmed as-is into corresponding registers
     * PM_CLKOBS_CTRL and LSM_CLKOBS_CTRL. Set to -1 to leave chip default
     * values */
    fm_int      clkOutA;
    fm_int      clkOutB;
    fm_int      clkOutC;
    fm_int      clkOutDivA;
    fm_int      clkOutDivB;
    fm_int      clkOutDivC;

} fm10000_bootCfg;




/* Structure to carry information to the interrupt polling thread */
typedef struct _fm10000_bsmIsrParams
{
    fm_int                     sw;
    fm10000_bootCfg *          bootCfg;
    fm_registerReadUINT32Func  readFunc;
    fm_registerWriteUINT32Func writeFunc;

} fm10000_bsmIsrParams;



fm_status fm10000LoadBootCfg(fm_int sw, fm10000_bootCfg *bootCfg);

fm_status fm10000GetSwitchId(fm_int                     sw,
                             fm_switchFamily *          family,
                             fm_switchModel *           model,
                             fm_switchVersion *         version,
                             fm_registerReadUINT32Func  readFunc,
                             fm_registerWriteUINT32Func writeFunc);


/* Functions for Cold Boot and PCIE Interrupt Handling */
fm_status fm10000ColdResetInit(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_bool                    swIsr,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc);

fm_status fm10000PcieInitAfterReset(fm_int                     sw,
                                    fm10000_bootCfg *          bootCfg,
                                    fm_int                     pepId,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc);

fm_status fm10000SetupPCIeISR(fm_int                     sw,
                              fm_uint32                  mask,
                              fm_registerReadUINT32Func  readFunc,
                              fm_registerWriteUINT32Func writeFunc);

fm_status fm10000PCIeISR(fm_int                     sw,
                         fm10000_bootCfg *          bootCfg,
                         fm_registerReadUINT32Func  readFunc,
                         fm_registerWriteUINT32Func writeFunc);

fm_status fm10000PollInterrupt(fm_int                    sw,
                               fm_uint64                 intrMask,
                               fm_uint *                 intrStatus,
                               fm_registerReadUINT32Func readFunc);



/* Discrete boot functions */
fm_status fm10000CrStartClocks(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrConfigPcieMode(fm_int                     sw,
                                  fm10000_bootCfg *          bootCfg,
                                  fm_registerReadUINT32Func  readFunc,
                                  fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrDeassertColdReset(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrStartSbuses(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrMemoryRepair(fm_int                     sw,
                                fm10000_bootCfg *          bootCfg,
                                fm_registerReadUINT32Func  readFunc,
                                fm_registerWriteUINT32Func writeFunc);
fm_status fm10000RunBist(fm_int                     sw,
                         fm10000_bootCfg *          bootCfg,
                         fm_int                     mode,
                         fm_uint32                  mask,
                         fm_registerReadUINT32Func  readFunc,
                         fm_registerWriteUINT32Func writeFunc);

fm_status fm10000CrInitializeMemories(fm_int                     sw,
                                      fm10000_bootCfg *          bootCfg,
                                      fm_registerReadUINT32Func  readFunc,
                                      fm_registerWriteUINT32Func writeFunc);

fm_status fm10000CrConfigurePciePcsInterrupts(fm_int                     sw,
                                              fm10000_bootCfg *          bootCfg,
                                              fm_bool                    state,
                                              fm_registerReadUINT32Func  readFunc,
                                              fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrDeassertPcieWarmReset(fm_int                     sw,
                                         fm10000_bootCfg *          bootCfg,
                                         fm_registerReadUINT32Func  readFunc,
                                         fm_registerWriteUINT32Func writeFunc);
fm_status fm10000CrInitializePcieSerdes(fm_int                     sw,
                                        fm10000_bootCfg *          bootCfg,
                                        fm_registerReadUINT32Func  readFunc,
                                        fm_registerWriteUINT32Func writeFunc);


/* SBUS Util Services */
void fm10000UtilSbusSetDebug(fm_int debug);
fm_status fm10000UtilSbusInit(fm_int sw, 
                              fm_bool eplRing,
                              fm_registerReadUINT32Func  readFunc,
                              fm_registerWriteUINT32Func writeFunc);
fm_status fm10000UtilSbusRead(fm_int sw,
                              fm_bool eplRing,
                              fm_uint sbusAddr,
                              fm_uint sbusReg,
                              fm_uint32 *data,
                              fm_registerReadUINT32Func  readFunc,
                              fm_registerWriteUINT32Func writeFunc);
fm_status fm10000UtilSbusWrite(fm_int sw,
                               fm_bool eplRing,
                               fm_uint sbusAddr,
                               fm_uint sbusReg,
                               fm_uint32 data,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc);
fm_status fm10000UtilSbusReset(fm_int    sw,
                               fm_bool   eplRing,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc);
fm_status fm10000UtilSbusSbmReset(fm_int    sw,
                                  fm_bool   eplRing,
                                  fm_registerReadUINT32Func  readFunc,
                                  fm_registerWriteUINT32Func writeFunc);

fm_status fm10000UtilSerdesWrite(fm_int     sw,
                                 fm_int     serdes,
                                 fm_uint    regAddr,
                                 fm_uint32  value,
                                 fm_registerReadUINT32Func  readFunc,
                                 fm_registerWriteUINT32Func writeFunc);
fm_status fm10000UtilSerdesRead(fm_int     sw,
                                fm_int     serdes,
                                fm_uint    regAddr,
                                fm_uint32 *pValue,
                                fm_registerReadUINT32Func  readFunc,
                                fm_registerWriteUINT32Func writeFunc);

fm_status fm10000UtilSbmSpicoInt(fm_int         sw,
                                 fm_bool        ring,
                                 fm_int         sbusAddr,
                                 fm_uint        intNum,
                                 fm_uint32      param,
                                 fm_uint32     *pValue,
                                 fm_registerReadUINT32Func  readFunc,
                                 fm_registerWriteUINT32Func writeFunc);

fm_status fm10000UtilSerdesSpicoInt(fm_int      sw,
                                    fm_int      serDes,
                                    fm_uint     intNum,
                                    fm_uint32   param,
                                    fm_uint32  *pValue,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc);

/* Serdes and FW Image Loading */
fm_status fm10000UtilsLoadSpicoCode(fm_int sw,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc,
                                    fm_bool checkCrc);

/* PCIE ISR Services */
fm_status fm10000PcieCfgLinkTraining(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc);
fm_status fm10000PcieCfgLinkGen3CtrlReg(fm_int                     sw,
                                        fm10000_bootCfg *          bootCfg,
                                        fm_int                     pepId,
                                        fm_registerReadUINT32Func  readFunc,
                                        fm_registerWriteUINT32Func writeFunc);
fm_status fm10000PcieCfgLinkGen3Coef(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc);
fm_status fm10000PcieCfgHostLaneCtrl(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc);
fm_status fm10000PcieCfgltssmEnable(fm_int                     sw,
                                    fm10000_bootCfg *          bootCfg,
                                    fm_int                     pepId,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc);
fm_status fm10000PcieLoadPepSettings(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc);

#endif /* __FM_FM10000_UTILS_H */
