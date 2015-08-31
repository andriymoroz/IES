/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_utils.c
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

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <fm_sdk_fm10000_int.h>
#include <platforms/common/switch/fm10000/fm10000_utils.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_VPD             0xAE21

#define MAX_BUF_SIZE            256

/* Times specified based on EAS requirements */
#define TIME_PLL_RESET_ASSERT_NSEC           500                /* 500ns */
#define TIME_PLL_LOCK_NSEC                   1000000            /* 1ms   */
#define TIME_PLL_POST_CONFIGURE_DELAY_NSEC   100                /* 100ns */
#define TIME_MEM_REPAIR_NSEC                 160000             /* 160us */
#define TIME_BIST_WAIT                       800000             /* 800us */
#define TIME_POLLING_INTRRUPT                1000000            /* 1ms */

#define MEM_REPAIR_RETRIES                   10

#define PCIE_RING                            0
#define EPL_RING                             1

/* BIST definitions */
#define BIST_MEM_CHECK                       0
#define BIST_MEM_INIT                        1

#define BIST_MODULE_PCIE_0                   (1 << FM10000_BIST_CTRL_b_BistRun_PCIE_0)
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

/* GEN3 Equalization */
#define GEN3_EQ_NUM_PRESETS                  11
#define GEN3_EQ_FS                           24    /* Full Swing */
#define GEN3_EQ_LF                           8     /* Low Frequency Value */

/* EEPROM Addressing */
#define FM10000_EEPROM_ISR_OFFSET            0x004000
#define FM10000_EEPROM_BANK_A_BASE           0x080000
#define FM10000_EEPROM_BANK_A_ISR            (FM10000_EEPROM_BANK_A_BASE +  \
                                              FM10000_EEPROM_ISR_OFFSET)


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


fm_status fm10000UtilSbusInit(fm_int    sw,
                              fm_bool   eplRing,
                              fm_registerReadUINT32Func  readFunc,
                              fm_registerWriteUINT32Func writeFunc)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(eplRing);
    FM_NOT_USED(readFunc);
    FM_NOT_USED(writeFunc);

    return FM_ERR_UNSUPPORTED;
}

fm_status fm10000UtilSbusWrite(fm_int    sw,
                               fm_bool   eplRing,
                               fm_uint   sbusAddr,
                               fm_uint   sbusReg,
                               fm_uint32 data,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(eplRing);
    FM_NOT_USED(sbusAddr);
    FM_NOT_USED(sbusReg);
    FM_NOT_USED(data);
    FM_NOT_USED(readFunc);
    FM_NOT_USED(writeFunc);

    return FM_ERR_UNSUPPORTED;
}

fm_status fm10000UtilsLoadSpicoCode(fm_int sw,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc,
                                    fm_bool checkCrc)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(readFunc);
    FM_NOT_USED(writeFunc);
    FM_NOT_USED(checkCrc);

    return FM_ERR_UNSUPPORTED;
}


/*****************************************************************************/
/** _writeFunc64
 * \ingroup intPlatform
 *
 * \desc            Writes a 64-bit register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the register address to write.
 *
 * \param[in]       value is the 64-bit data value to write.
 * 
 * \param[in]       writeFunc is a function pointer to the 32-bit register
 *                  write function.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status _writeFunc64(fm_int                     sw, 
                       fm_uint32                  addr, 
                       fm_uint64                  value, 
                       fm_registerWriteUINT32Func writeFunc)
{
    fm_status err;
    fm_uint32 lo;
    fm_uint32 hi;

    lo = value & 0xFFFFFFFF;
    hi = (value >> 32) & 0xFFFFFFFF;
    
    err = writeFunc(sw, addr + 0, lo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, addr + 1, hi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    return err;
}




/*****************************************************************************/
/** _readFunc64
 * \ingroup intPlatform
 *
 * \desc            Read a 64-bit register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the register address to read.
 *
 * \param[out]      value points to storage where the 64-bit read data value
 *                  will be stored by this function.
 * 
 * \param[in]       readFunc is a function pointer to the 32-bit register
 *                  read function.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status _readFunc64(fm_int                      sw, 
                       fm_uint32                  addr, 
                       fm_uint64 *                value, 
                       fm_registerReadUINT32Func  readFunc)
{
    fm_status err;
    fm_uint32 lo;
    fm_uint32 hi;
    
    err = readFunc(sw, addr + 0, &lo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = readFunc(sw, addr + 1, &hi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    *value = ( ((fm_uint64)(hi)) << 32) | ((fm_uint64)(lo));

ABORT:
    return err;
}


/* Read/Write MACROs */
#define writeFunc64(sw, addr, value)    _writeFunc64(sw, addr, value, writeFunc)
#define readFunc64(sw, addr, value)     _readFunc64(sw, addr, value, readFunc)




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/* fm10000LoadBootCfg
 *
 * \desc            Load the boot configuration for EBI/I2C boot
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      bootCfg points to caller-allocated storage where this
 *                  function should place the obtained values.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000LoadBootCfg(fm_int sw, fm10000_bootCfg *bootCfg)
{
    fm_status err = FM_OK;
    fm_char   buf[MAX_BUF_SIZE+1];
    fm_int    valBool;
    fm_int    valInt;
    fm_text   valText;
    fm_int    pepId;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    if (bootCfg == NULL)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /**************************************************
     * Get SPI Transfer Mode and Speed
     **************************************************/

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_SPI_TRANSFER_MODE, 
                  sw);
    valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_SPI_TRANSFER_MODE);

    bootCfg->spiTransferMode = valInt;

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_SPI_TRANSFER_SPEED, 
                  sw);
    valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_SPI_TRANSFER_SPEED);

    bootCfg->spiTransferSpeed = valInt;

    /**************************************************
     * Get MGMT PEP
     **************************************************/

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_MGMT_PEP, 
                  sw);
    valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_MGMT_PEP);

    bootCfg->mgmtPep = valInt;

    /**************************************************
     * Get Systime Clock Source
     **************************************************/

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_SYSTIME_CLK_SRC, 
                  sw);
    valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_SYSTIME_CLK_SRC);

    bootCfg->systimeClockSource = valBool;
    
    /**************************************************
     * Unpublished boot config parameters
     **************************************************/

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_SKIP_PCIE_INIT, 
                  sw);
    valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_SKIP_PCIE_INIT);

    bootCfg->skipPcieInitialization = valBool;

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_SKIP_MEM_REPAIR, 
                  sw);
    valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_SKIP_MEM_REPAIR);

    bootCfg->skipMemRepair = valBool;

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_ENABLE_BIST_CHECK, 
                  sw);
    valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_ENABLE_BIST_CHECK);

    bootCfg->enableBistCheck = valBool;

    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_BOOTCFG_ENABLE_FW_VERIFY, 
                  sw);
    valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_ENABLE_FW_VERIFY);

    bootCfg->enableFwVerify = valBool;

    /**************************************************
     * PEP boot config parameters
     **************************************************/

    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_MODE, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_MODE);

        bootCfg->pepMode[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_ENABLE, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_ENABLE);

        bootCfg->pepEnable[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED);

        bootCfg->pepBar4Allowed[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_NUM_LANES, 
                      sw,
                      pepId);
        valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_NUM_LANES);

        bootCfg->pepNumberOfLanes[pepId] = valInt;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_SUB_VENDOR_ID, 
                      sw,
                      pepId);
        valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_SUB_VENDOR_ID);

        bootCfg->pepSubVendorId[pepId] = valInt;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_SUB_DEVICE_ID, 
                      sw,
                      pepId);
        valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_SUB_DEVICE_ID);

        bootCfg->pepSubDeviceId[pepId] = valInt;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_SN, 
                      sw,
                      pepId);
        valText = fmGetTextApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_SN);

        bootCfg->pepSerialNumber[pepId] = valText;

        /* Unpublished PEP boot params */
        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_BAR4_ALLOWED);

        bootCfg->pepBar4Allowed[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_AUTO_LANE_FLIP, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_AUTO_LANE_FLIP);

        bootCfg->pepAutoLaneFlip[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_RX_LANE_FLIP, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_RX_LANE_FLIP);

        bootCfg->pepRxLaneFlip[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_TX_LANE_FLIP, 
                      sw,
                      pepId);
        valBool = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_TX_LANE_FLIP);

        bootCfg->pepTxLaneFlip[pepId] = valBool;

        FM_SNPRINTF_S(buf, 
                      MAX_BUF_SIZE, 
                      FM_AAK_API_PLATFORM_BOOTCFG_PEP_MASTER_LANE, 
                      sw,
                      pepId);
        valInt = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_BOOTCFG_PEP_MASTER_LANE);

        bootCfg->pepMasterLane[pepId] = valInt;

    }   /* end for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++) */

    /* leave the clock out as-is */
    bootCfg->clkOutA = -1;
    bootCfg->clkOutB = -1;
    bootCfg->clkOutC = -1;
    bootCfg->clkOutDivA = -1;
    bootCfg->clkOutDivB = -1;
    bootCfg->clkOutDivC = -1;

    


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm1000LoadBootCfg */




/*****************************************************************************/
/* fm10000GetSwitchId
 *
 * \desc            Returns switch family, model and version.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[out]      family points to caller-provided storage where the
 *                  chip family will be stored.
 *
 * \param[out]      model points to caller-provided storage where the
 *                  chip model will be stored.
 *
 * \param[out]      version points to caller-provided storage where the
 *                  chip version will be stored.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetSwitchId(fm_int                     sw,
                             fm_switchFamily *          family,
                             fm_switchModel *           model,
                             fm_switchVersion *         version,
                             fm_registerReadUINT32Func  readFunc,
                             fm_registerWriteUINT32Func writeFunc)
{
    fm_status   err;
    fm_uint32   rv;
    fm_uint32   chipVersion;

    FM_NOT_USED(writeFunc);

    /* assign default */
    *family  = FM_SWITCH_FAMILY_UNKNOWN;
    *model   = FM_SWITCH_MODEL_UNKNOWN;
    *version = FM_SWITCH_VERSION_UNKNOWN;

    err = readFunc(sw, FM10000_VITAL_PRODUCT_DATA(), &rv);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if (rv == FM10000_VPD)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                     "Identified this device as an FM10000 series device\n");
        *family = FM_SWITCH_FAMILY_FM10000;
        *model  = FM_SWITCH_MODEL_FM10440;

        err = readFunc(sw, FM10000_CHIP_VERSION(), &rv);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        chipVersion = FM_GET_FIELD(rv, FM10000_CHIP_VERSION, Version);

        if (chipVersion == FM10000_CHIP_VERSION_A0)
        {
            *version = FM_SWITCH_VERSION_FM10440_A0;
        }
        else if (chipVersion == FM10000_CHIP_VERSION_B0)
        {
            *version = FM_SWITCH_VERSION_FM10440_B0;
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to identify switch %d: Version=0x%x\n",
                         sw, chipVersion);
        }
    }
    else
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unable to identify switch %d: VPD=0x%08x\n", sw, rv);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fm10000GetSwitchId */




/*****************************************************************************/
/* fm10000CrStartClocks
 *
 * \desc            Cold Reset Start Clocks
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrStartClocks(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 rv;
    fm_bool   pciePllLocked;
    fm_bool   eplPllLocked;
    fm_bool   fabricPllLocked;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    /* Defaults + NReset = 0 */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Nreset = 0\n");

    err = writeFunc(sw, FM10000_PLL_PCIE_CTRL(),   0x0028640a);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_EPL_CTRL(),    0x0018b81a);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_FABRIC_CTRL(), 0x001cbc1a);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Wait 500ns */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Wait 500ns\n");

    fmDelay(0, TIME_PLL_RESET_ASSERT_NSEC);

    /* NReset = 1 */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Nreset = 1\n");

    err = writeFunc(sw, FM10000_PLL_PCIE_CTRL(),   0x0028640b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_EPL_CTRL(),    0x0018b81b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_FABRIC_CTRL(), 0x001cbc1b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Wait 1ms for PLL lock */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Wait 1ms for PLL Lock\n");

    fmDelay(0, TIME_PLL_LOCK_NSEC);

    /* Read/Validate PLL Lock */
    err = readFunc(sw, FM10000_PLL_PCIE_STAT(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    pciePllLocked = FM_GET_BIT(rv, FM10000_PLL_PCIE_STAT, PllLocked);

    err = readFunc(sw, FM10000_PLL_EPL_STAT(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    eplPllLocked = FM_GET_BIT(rv, FM10000_PLL_PCIE_STAT, PllLocked);

    err = readFunc(sw, FM10000_PLL_FABRIC_STAT(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    fabricPllLocked = FM_GET_BIT(rv, FM10000_PLL_PCIE_STAT, PllLocked);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  PCIE   PLL Locked = %d\n", pciePllLocked);
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  EPL    PLL Locked = %d\n", eplPllLocked);
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  FABRIC PLL Locked = %d\n", fabricPllLocked);

    if (pciePllLocked   == 0 ||
        eplPllLocked    == 0 ||
        fabricPllLocked == 0)
    {
        /* Added for indication only, the PLL Locked fields may be innacurate */
        FM_LOG_WARNING(FM_LOG_CAT_PLATFORM, "!!!! PLL Not Locked !!!!\n");
    }

    /* OutMuxSel = 1 */ 
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  PLL_XXX_CTRL.OutMuxSel = 1\n");

    err = writeFunc(sw, FM10000_PLL_PCIE_CTRL(),   0x0128640b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_EPL_CTRL(),    0x0118b81b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, FM10000_PLL_FABRIC_CTRL(), 0x011cbc1b);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Wait 100ns */
    fmDelay(0, TIME_PLL_POST_CONFIGURE_DELAY_NSEC);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                 "  Configuring CLKOUT for debugging purposes\n");

    err = readFunc(sw, FM10000_PM_CLKOBS_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if (bootCfg->clkOutA != -1)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "     CLKOUT_A = %d DivA = %d\n", 
                     bootCfg->clkOutA, 
                     bootCfg->clkOutDivA);
        FM_SET_FIELD(rv, FM10000_PM_CLKOBS_CTRL, SelA, bootCfg->clkOutA); 
        FM_SET_FIELD(rv, FM10000_PM_CLKOBS_CTRL, DivA, bootCfg->clkOutDivA);
    }

    if (bootCfg->clkOutB != -1)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "     CLKOUT_B = %d DivB = %d\n", 
                     bootCfg->clkOutB, 
                     bootCfg->clkOutDivB);
        FM_SET_FIELD(rv, FM10000_PM_CLKOBS_CTRL, SelB, bootCfg->clkOutB);
        FM_SET_FIELD(rv, FM10000_PM_CLKOBS_CTRL, DivB, bootCfg->clkOutDivB);
    }
    
    err = writeFunc(sw, FM10000_PM_CLKOBS_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = readFunc(sw, FM10000_LSM_CLKOBS_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if (bootCfg->clkOutC != -1)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "     CLKOUT_C = %d DivC = %d\n", 
                     bootCfg->clkOutC, 
                     bootCfg->clkOutDivC);
        FM_SET_FIELD(rv, FM10000_LSM_CLKOBS_CTRL, SelC, bootCfg->clkOutC);
        FM_SET_FIELD(rv, FM10000_LSM_CLKOBS_CTRL, DivC, bootCfg->clkOutDivC);
    }

    err = writeFunc(sw, FM10000_LSM_CLKOBS_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    /* Set the systime clock source */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                 "  Setting SystimeClockSource = %d\n", 
                 bootCfg->systimeClockSource);

    err = readFunc(sw, FM10000_DEVICE_CFG(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, 
               FM10000_DEVICE_CFG, 
               SystimeClockSource, 
               bootCfg->systimeClockSource);

    err = writeFunc(sw, FM10000_DEVICE_CFG(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrStartClocks */




/*****************************************************************************/
/* fm10000CrConfigPcieMode
 *
 * \desc            Configure the PCIE Mode
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrConfigPcieMode(fm_int                     sw,
                                  fm10000_bootCfg *          bootCfg,
                                  fm_registerReadUINT32Func  readFunc,
                                  fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    for (i = 0; i < FM10000_NUM_PEPS-1; i += 2)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "  PEP[%d] Mode = %s\n",
                     i,
                     bootCfg->pepMode[i] ? "2x4" : "1x8");
    }

    for (i = 0; i < FM10000_NUM_PEPS; i++)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "  PEP[%d] Enable = %d\n",
                     i, 
                     bootCfg->pepEnable[i]);
    }

    err = readFunc(sw, FM10000_DEVICE_CFG(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeMode_0, bootCfg->pepMode[0]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeMode_1, bootCfg->pepMode[2]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeMode_2, bootCfg->pepMode[4]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeMode_3, bootCfg->pepMode[6]);

    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_0, bootCfg->pepEnable[0]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_1, bootCfg->pepEnable[1]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_2, bootCfg->pepEnable[2]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_3, bootCfg->pepEnable[3]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_4, bootCfg->pepEnable[4]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_5, bootCfg->pepEnable[5]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_6, bootCfg->pepEnable[6]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_7, bootCfg->pepEnable[7]);
    FM_SET_BIT(rv, FM10000_DEVICE_CFG, PCIeEnable_8, bootCfg->pepEnable[8]);

    err = writeFunc(sw, FM10000_DEVICE_CFG(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrConfigPcieMode */




/*****************************************************************************/
/* fm10000CrDeassertColdReset
 *
 * \desc            De-assert Cold Reset
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrDeassertColdReset(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    err = readFunc(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, ColdReset, 0);

    err = writeFunc(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrDeassertColdReset */




/*****************************************************************************/
/* fm10000CrStartSbuses
 *
 * \desc            Start Sbuses
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrStartSbuses(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Taking PCIE SBUS out of reset\n"); 

        err = readFunc(sw, FM10000_SBUS_PCIE_CFG(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_BIT(rv, FM10000_SBUS_PCIE_CFG, SBUS_ControllerReset, 0);

        err = writeFunc(sw, FM10000_SBUS_PCIE_CFG(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        err = fm10000UtilSbusInit(sw, PCIE_RING, readFunc, writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Taking EPL SBUS out of reset\n"); 

    err = readFunc(sw, FM10000_SBUS_EPL_CFG(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_SBUS_EPL_CFG, SBUS_ControllerReset, 0);

    err = writeFunc(sw, FM10000_SBUS_EPL_CFG(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusInit(sw, EPL_RING, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrStartSbuses */




/*****************************************************************************/
/* fm10000CrMemoryRepair
 *
 * \desc            Repair Memories
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrMemoryRepair(fm_int                     sw,
                                fm10000_bootCfg *          bootCfg,
                                fm_registerReadUINT32Func  readFunc,
                                fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    err = readFunc(sw, FM10000_REI_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_REI_CTRL, AutoLoadEnable, 1);

    err = writeFunc(sw, FM10000_REI_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_REI_CTRL, Reset, 0);

    err = writeFunc(sw, FM10000_REI_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Per EAS, MEM repair take 1.6ms
       1.6ms = MEM_REPAIR_RETRIES * TIME_MEM_REPAIR_NSEC */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Polling for Memory Repair Success\n"); 

    for (i = 0; i < MEM_REPAIR_RETRIES; i++)
    {
        fmDelay(0, TIME_MEM_REPAIR_NSEC);

        err = readFunc(sw, FM10000_REI_STAT(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        /* PASS==1 && FAIL==0*/
        if (rv == 0x1)
        {
            break;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                 "  Memory Repair Sucess= %s\n",
                 rv == 0x1 ? "PASS" : "FAIL" ); 

    if (rv != 0x1)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrMemoryRepair */




/*****************************************************************************/
/* fm10000RunBist
 *
 * \desc            Generic Function to run BIST
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       mode can either be BIST_MEM_INIT or BIST_MEM_CHECK
 * 
 * \param[in]       mask is a bit mask of the modules on which BIST should
 *                  be run. See BIST_MODULE_XXX.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000RunBist(fm_int                     sw,
                         fm10000_bootCfg *          bootCfg,
                         fm_int                     mode,
                         fm_uint32                  mask,
                         fm_registerReadUINT32Func  readFunc,
                         fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint64 rv64;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    err = readFunc64(sw, FM10000_BIST_CTRL(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                 "  Setting BIST Mode to %s on: \n", 
                 mode == BIST_MEM_CHECK ? "MEM_CHECK" : "MEM_INIT"); 

    if (mask & BIST_MODULE_PCIE_0)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_0, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_0, \n");
    }

    if (mask & BIST_MODULE_PCIE_1)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_1, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_1, \n");
    }

    if (mask & BIST_MODULE_PCIE_2)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_2, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_2, \n");
    }

    if (mask & BIST_MODULE_PCIE_3)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_3, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_3, \n");
    }

    if (mask & BIST_MODULE_PCIE_4)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_4, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_4, \n");
    }

    if (mask & BIST_MODULE_PCIE_5)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_5, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_5, \n");
    }

    if (mask & BIST_MODULE_PCIE_6)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_6, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_6, \n");
    }

    if (mask & BIST_MODULE_PCIE_7)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_7, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_7, \n");
    }

    if (mask & BIST_MODULE_PCIE_8)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_8, mode);
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    PCIE_8, \n"); 
    }

    if (mask & BIST_MODULE_EPL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_EPL, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    EPL, \n");
    }

    if (mask & BIST_MODULE_FABRIC)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FABRIC, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    FABRIC, \n");
    }

    if (mask & BIST_MODULE_TUNNEL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_TUNNEL, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    TUNNEL, \n");
    }

    if (mask & BIST_MODULE_BSM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_BSM, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    BSM, \n");
    }

    if (mask & BIST_MODULE_CRM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_CRM, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    CRM, \n");
    }

    if (mask & BIST_MODULE_FIBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FIBM, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    FIBM, \n");
    }

    if (mask & BIST_MODULE_SBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_SBM, mode); 
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "    SBM, \n");
    }

    err = writeFunc64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = readFunc64(sw, FM10000_BIST_CTRL(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                 "  FM10000_BIST_CTRL = 0x%" FM_FORMAT_64 "x\n",
                 rv64); 

    /* Intentionnaly sequenced after setting the mode */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Running BIST...\n"); 

    if (mask & BIST_MODULE_PCIE_0)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_0, 1); 
    }

    if (mask & BIST_MODULE_PCIE_1)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_1, 1); 
    }

    if (mask & BIST_MODULE_PCIE_2)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_2, 1); 
    }

    if (mask & BIST_MODULE_PCIE_3)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_3, 1); 
    }

    if (mask & BIST_MODULE_PCIE_4)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_4, 1); 
    }

    if (mask & BIST_MODULE_PCIE_5)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_5, 1); 
    }

    if (mask & BIST_MODULE_PCIE_6)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_6, 1); 
    }

    if (mask & BIST_MODULE_PCIE_7)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_7, 1); 
    }

    if (mask & BIST_MODULE_PCIE_8)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_8, 1); 
    }

    if (mask & BIST_MODULE_EPL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_EPL, 1); 
    }

    if (mask & BIST_MODULE_FABRIC)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FABRIC, 1); 
    }

    if (mask & BIST_MODULE_TUNNEL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_TUNNEL, 1); 
    }

    if (mask & BIST_MODULE_BSM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_BSM, 1); 
    }

    if (mask & BIST_MODULE_CRM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_CRM, 1); 
    }

    if (mask & BIST_MODULE_FIBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FIBM, 1); 
    }

    if (mask & BIST_MODULE_SBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_SBM, 1); 
    }
    
    err = writeFunc64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* wait 0.8ms */
    fmDelay(0, TIME_BIST_WAIT);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Stopping BIST (Run = 0)...\n"); 

    if (mask & BIST_MODULE_PCIE_0)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_0, 0); 
    }

    if (mask & BIST_MODULE_PCIE_1)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_1, 0); 
    }

    if (mask & BIST_MODULE_PCIE_2)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_2, 0); 
    }

    if (mask & BIST_MODULE_PCIE_3)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_3, 0); 
    }

    if (mask & BIST_MODULE_PCIE_4)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_4, 0); 
    }

    if (mask & BIST_MODULE_PCIE_5)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_5, 0); 
    }

    if (mask & BIST_MODULE_PCIE_6)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_6, 0); 
    }

    if (mask & BIST_MODULE_PCIE_7)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_7, 0); 
    }

    if (mask & BIST_MODULE_PCIE_8)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_PCIE_8, 0); 
    }

    if (mask & BIST_MODULE_EPL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_EPL, 0); 
    }

    if (mask & BIST_MODULE_FABRIC)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FABRIC, 0); 
    }

    if (mask & BIST_MODULE_TUNNEL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_TUNNEL, 0); 
    }

    if (mask & BIST_MODULE_BSM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_BSM, 0); 
    }

    if (mask & BIST_MODULE_CRM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_CRM, 0); 
    }

    if (mask & BIST_MODULE_FIBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FIBM, 0);
    }

    if (mask & BIST_MODULE_SBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_SBM, 0); 
    }

    err = writeFunc64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Intentionnaly sequenced after stopping BIST run */
    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "  Clearing BIST Mode (Mode = 0)...\n"); 

    if (mask & BIST_MODULE_PCIE_0)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_0, 0);
    }

    if (mask & BIST_MODULE_PCIE_1)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_1, 0);
    }

    if (mask & BIST_MODULE_PCIE_2)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_2, 0);
    }

    if (mask & BIST_MODULE_PCIE_3)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_3, 0);
    }

    if (mask & BIST_MODULE_PCIE_4)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_4, 0);
    }

    if (mask & BIST_MODULE_PCIE_5)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_5, 0);
    }

    if (mask & BIST_MODULE_PCIE_6)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_6, 0);
    }

    if (mask & BIST_MODULE_PCIE_7)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_7, 0);
    }

    if (mask & BIST_MODULE_PCIE_8)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_PCIE_8, 0);
    }

    if (mask & BIST_MODULE_EPL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_EPL,    0);
    }

    if (mask & BIST_MODULE_FABRIC)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FABRIC, 0);
    }

    if (mask & BIST_MODULE_TUNNEL)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_TUNNEL, 0);
    }

    if (mask & BIST_MODULE_BSM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_BSM,    0);
    }

    if (mask & BIST_MODULE_CRM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_CRM,    0);
    }

    if (mask & BIST_MODULE_FIBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FIBM,   0);
    }

    if (mask & BIST_MODULE_SBM)
    {
        FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_SBM,    0);
    }

    err = writeFunc64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000RunBist */




/*****************************************************************************/
/* fm10000CrInitializeMemories
 *
 * \desc            Initialize memories by running BIST.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrInitializeMemories(fm_int                     sw,
                                      fm10000_bootCfg *          bootCfg,
                                      fm_registerReadUINT32Func  readFunc,
                                      fm_registerWriteUINT32Func writeFunc)
{
    return fm10000RunBist(sw, 
                         bootCfg, 
                         BIST_MEM_INIT, 
                         BIST_MODULE_PCIE_0 |
                         BIST_MODULE_PCIE_1 |
                         BIST_MODULE_PCIE_2 |
                         BIST_MODULE_PCIE_3 |
                         BIST_MODULE_PCIE_4 |
                         BIST_MODULE_PCIE_5 |
                         BIST_MODULE_PCIE_6 |
                         BIST_MODULE_PCIE_7 |
                         BIST_MODULE_PCIE_8 |
                         BIST_MODULE_EPL    |
                         BIST_MODULE_FABRIC |
                         BIST_MODULE_TUNNEL |
                         BIST_MODULE_BSM    |
                         BIST_MODULE_CRM    |
                         BIST_MODULE_FIBM   |
                         BIST_MODULE_SBM,
                         readFunc,
                         writeFunc);

}   /* end fm10000CrInitializeMemories */




/*****************************************************************************/
/* fm10000CrConfigurePciePcsInterrupts
 *
 * \desc            Configure PCIE PCS Interrupts
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       state determines if the PCS Interrupts should
 *                  be enabled (TRUE) or not (FALSE)
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrConfigurePciePcsInterrupts(fm_int                     sw,
                                              fm10000_bootCfg *          bootCfg,
                                              fm_bool                    state,
                                              fm_registerReadUINT32Func  readFunc,
                                              fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 addr;
    fm_uint64 rv64;
    fm_int    i;
    fm_int    j;
    fm_int    numSerdes;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    for (i = 0; i < FM10000_NUM_PEPS; i++)
    {
        if (!bootCfg->pepEnable[i])
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "  PEP #%d: PEP not enabled, skipping\n",
                         i);
        }
        else
        {
            numSerdes = (i == 8) ? 1 : FM10000_PCIE_SERDES_CTRL_ENTRIES;

            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "  PEP #%d: FM10000_PCIE_SERDES_CTRL[0..%d].enable = %d\n",
                         i,
                         numSerdes-1,
                         !state);

            for (j = 0; j < numSerdes; j++)
            {
                addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_SERDES_CTRL(j, 0), i);

                err = readFunc64(sw, addr, &rv64);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

                FM_SET_BIT64(rv64, FM10000_PCIE_SERDES_CTRL, Enable, !state);
                
                err = writeFunc64(sw, addr, rv64);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            }
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrConfigurePciePcsInterrupts */




/*****************************************************************************/
/* fm10000CrDeassertPcieWarmReset
 *
 * \desc            Deassert PCIE Warm Reset
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrDeassertPcieWarmReset(fm_int                     sw,
                                         fm10000_bootCfg *          bootCfg,
                                         fm_registerReadUINT32Func  readFunc,
                                         fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    for (i = 0; i < FM10000_NUM_PEPS; i++)
    {
        if (!bootCfg->pepEnable[i])
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "  PEP #%d: PEP not enabled, skipping\n",
                         i);
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                         "  PEP #%d: Taking PEP out of reset\n",
                         i);
        }
    }

    err = readFunc(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_0, !bootCfg->pepEnable[0]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_1, !bootCfg->pepEnable[1]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_2, !bootCfg->pepEnable[2]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_3, !bootCfg->pepEnable[3]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_4, !bootCfg->pepEnable[4]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_5, !bootCfg->pepEnable[5]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_6, !bootCfg->pepEnable[6]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_7, !bootCfg->pepEnable[7]);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, PCIeReset_8, !bootCfg->pepEnable[8]);

    err = writeFunc(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrDeassertPcieWarmReset */




/*****************************************************************************/
/* fm10000CrInitializePcieSerdes
 *
 * \desc            Initilize PCIE Serdes
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrInitializePcieSerdes(fm_int                     sw,
                                        fm10000_bootCfg *          bootCfg,
                                        fm_registerReadUINT32Func  readFunc,
                                        fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    /* Initialize PCIe Serdes in parallel */

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x00060050, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x00051032, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x0026000E, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x00260102, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x002602A0, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = fm10000UtilSbusWrite(sw, PCIE_RING, 0xFF, 0x03, 0x00265201, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrInitializePcieSerdes */




/*****************************************************************************/
/* fm10000CrConfigureBsmInterrupts
 *
 * \desc            Configure the BSM Registers and Interrupts
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrConfigureBsmInterrupts(fm_int                     sw,
                                          fm10000_bootCfg *          bootCfg,
                                          fm_registerReadUINT32Func  readFunc,
                                          fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 rv;
    fm_int    pepId;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    /* Configure the BSM Registers.
     *
     * data[23:0]  - 24-bit Interrupt Address
     * data[29:27] - SPI Transfer Speed
     * data[31:30] - SPI Transfer Mode */
    rv  = FM10000_EEPROM_BANK_A_ISR & 0xFFFFFF; 
    rv |= (bootCfg->spiTransferSpeed & 0x7) << 27;
    rv |= (bootCfg->spiTransferMode & 0x2) << 30;

    err = writeFunc(sw, FM10000_BSM_ARGS(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Configure the Interrupt Masks for PCIE Related Events */
    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        err = readFunc(sw, FM10000_PCIE_PF_ADDR(FM10000_PCIE_IB(), pepId), &rv); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_BIT(rv, FM10000_PCIE_IP, HotReset, 0);
        FM_SET_BIT(rv, FM10000_PCIE_IP, PFLR, 0);
        FM_SET_BIT(rv, FM10000_PCIE_IP, DataPathReset, 0);
        FM_SET_BIT(rv, FM10000_PCIE_IP, VPD_Request, 0);
        FM_SET_BIT(rv, FM10000_PCIE_IP, OutOfReset, 0);

        err = writeFunc(sw, FM10000_PCIE_PF_ADDR(FM10000_PCIE_IB(), pepId), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000CrConfigureBsmInterrupts */




/*****************************************************************************/
/* fm10000PcieCfgLinkTraining
 *
 * \desc            Configure Link Training Paramaters such as link width,
 *                  autoFlip.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieCfgLinkTraining(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint   addr;
    fm_uint32 rv;
    fm_uint32 masterLane;
    fm_uint32 masterPep;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_10C(), pepId);
    
    err = readFunc(sw, addr, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_PL_REG_10C, 
                 fieldA, 
                 bootCfg->pepNumberOfLanes[pepId]);

    FM_SET_BIT(rv, 
               FM10000_PCIE_PL_REG_10C, 
               fieldB, 
               bootCfg->pepAutoLaneFlip[pepId]);

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_010(), pepId);
    
    err = readFunc(sw, addr, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_PL_REG_010, 
                 fieldA, 
                 ((2 * bootCfg->pepNumberOfLanes[pepId]) - 1));

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Set PCIE_CFG_PCIE_LINK_CAP[i].MaxLinkWidth = NL; */ 
    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_CFG_PCIE_LINK_CAP(), pepId);

    err = readFunc(sw, addr, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_CFG_PCIE_LINK_CAP, 
                 MaxLinkWidth, 
                 bootCfg->pepNumberOfLanes[pepId]);

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Set PCIE_HOST_LANE_CTRL[i].MasterLane, based on NL and link reversal */
    if (bootCfg->pepMasterLane[pepId] == -1)
    {
        masterPep = pepId % 2;

        /* Even PEP = 0, Odd PEP = 4 */
        masterLane = (masterPep == 0) ? 0 : 4; 
    }
    else
    {
        masterLane = bootCfg->pepMasterLane[pepId];
    }

    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_HOST_LANE_CTRL(), pepId);

    err = readFunc(sw, addr, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_HOST_LANE_CTRL, 
                 MasterLane, 
                 masterLane);

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieCfgLinkTraining */




/*****************************************************************************/
/* fm10000PcieCfgLinkGen3CtrlReg
 *
 * \desc            Configure Gen3 Equalization
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieCfgLinkGen3CtrlReg(fm_int                     sw,
                                        fm10000_bootCfg *          bootCfg,
                                        fm_int                     pepId,
                                        fm_registerReadUINT32Func  readFunc,
                                        fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint   addr;
    fm_uint32 rv;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_1A8(), pepId);
    
    err = readFunc(sw, addr, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, 
               FM10000_PCIE_PL_REG_1A8, 
               fieldA, 
               1);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_PL_REG_1A8, 
                 fieldB, 
                 (1<<4));

    FM_SET_BIT(rv, 
               FM10000_PCIE_PL_REG_1A8, 
               fieldC, 
               0);

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieCfgLinkGen3CtrlReg */




/*****************************************************************************/
/* fm10000PcieCfgLinkGen3Coef
 *
 * \desc            Configure the PCIe Gen3 equalization coefficients
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieCfgLinkGen3Coef(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint   addr;
    fm_uint32 rv;
    fm_int    preset;
    fm_uint32 preCursor;
    fm_uint32 cursor;
    fm_uint32 postCursor;

    const fm_int PREC[GEN3_EQ_NUM_PRESETS] = {0, 0, 0, 0, 0, 10, 8, 10, 8, 6, 0}; 
    const fm_int POST[GEN3_EQ_NUM_PRESETS] = {4, 6, 5, 8, 0,  0, 0, 5,  8, 0, 2};
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    for (preset = 0; preset < GEN3_EQ_NUM_PRESETS; preset++ )
    {
        preCursor = (PREC[preset] == 0) ? 0 : round((double)(GEN3_EQ_FS)/PREC[preset]);
        
        if (preset < 10)
        {
            postCursor = (POST[preset] == 0) ? 0 : round((double)(GEN3_EQ_FS)/POST[preset]);
        }
        else
        {
            postCursor = round(((double)(GEN3_EQ_FS)-GEN3_EQ_LF)/2);
        }

        cursor = GEN3_EQ_FS - preCursor - postCursor;

        addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_19C(), pepId);
        rv = preset;

        err = writeFunc(sw, addr, rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_198(), pepId);
        rv = 0;

        FM_SET_FIELD(rv, 
                     FM10000_PCIE_PL_REG_198, 
                     fieldA,
                     preCursor);

        FM_SET_FIELD(rv, 
                     FM10000_PCIE_PL_REG_198, 
                     fieldB,
                     cursor);

        FM_SET_FIELD(rv, 
                     FM10000_PCIE_PL_REG_198, 
                     fieldC,
                     postCursor);

        err = writeFunc(sw, addr, rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_194(), pepId);
    rv = 0;

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_PL_REG_194, 
                 fieldA,
                 GEN3_EQ_LF);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_PL_REG_194, 
                 fieldB,
                 GEN3_EQ_FS);

    err = writeFunc(sw, addr, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieCfgLinkGen3Coef */




/*****************************************************************************/
/* fm10000PcieCfgHostLaneCtrl
 *
 * \desc            Configure host lane control.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieCfgHostLaneCtrl(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc)
{
    fm_status err;
    fm_uint32 addr;
    fm_uint32 rv;

    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_HOST_LANE_CTRL(), pepId);

    err = readFunc(sw, addr, &rv);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_PCIE_HOST_LANE_CTRL, IntDisableAutoAssert, 1);

    err = writeFunc(sw, addr, rv);
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieCfgHostLaneCtrl */




/*****************************************************************************/
/* fm10000PcieCfgltssmEnable
 *
 * \desc            Enable LTSSM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieCfgltssmEnable(fm_int                     sw,
                                    fm10000_bootCfg *          bootCfg,
                                    fm_int                     pepId,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc)
{
    fm_status err;
    fm_uint32 addr;
    fm_uint32 rv;

    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_CTRL(), pepId);

    err = readFunc(sw, addr, &rv);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_PCIE_CTRL, LTSSM_ENABLE, 1);

    /* BAR4_Allowed is CLear on Write 1, make sure we don't alter
     * its value. */
    FM_SET_BIT(rv, FM10000_PCIE_CTRL, BAR4_Allowed, 0);

    err = writeFunc(sw, addr, rv);
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieCfgltssmEnable */




/*****************************************************************************/
/* fm10000Eui64ToRegister
 *
 * \desc            Converts a EUI-64 string into two 32-bit words for writing
 *                  in FM10000 registers.
 * 
 *                  str = 00:A0:C9:FF:FF:23:45:67
 *                  L = 0xFFC9A000 H = 0x674523FF 
 * 
 * \param[in]       str is the EUI-64 string
 *
 * \param[out]      low is pointer to the caller allocated storage where
 *                  lower word should be stored.
 * 
 * \param[out]      high is pointer to the caller allocated storage where
 *                  higher word should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000Eui64ToRegister(fm_text    str, 
                                 fm_uint32 *word0, 
                                 fm_uint32 *word1)
{
    fm_status   err = FM_OK;
    fm_uint32   b[8];
    int         scanCount;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "str=%s\n", str);

    /* Try with the ':' delimiter */
    scanCount = FM_SSCANF_S(str,
                            "%2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x",
                            &b[0],
                            &b[1],
                            &b[2],
                            &b[3],
                            &b[4],
                            &b[5],
                            &b[6],
                            &b[7]);

    /* Try with the '-' delimiter */
    if (scanCount == 0)
    {
        scanCount = FM_SSCANF_S(str,
                                "%2x-%2x-%2x-%2x-%2x-%2x-%2x-%2x",
                                &b[0],
                                &b[1],
                                &b[2],
                                &b[3],
                                &b[4],
                                &b[5],
                                &b[6],
                                &b[7]);
    }

    if (scanCount != 8)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    *word0  = b[0] & 0xFF;
    *word0 |= (b[1] & 0xFF) << 8;
    *word0 |= (b[2] & 0xFF) << 16;
    *word0 |= (b[3] & 0xFF) << 24;

    *word1  = b[4] & 0xFF;
    *word1 |= (b[5] & 0xFF) << 8;
    *word1 |= (b[6] & 0xFF) << 16;
    *word1 |= (b[7] & 0xFF) << 24;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000Eui64ToRegister */




/*****************************************************************************/
/* fm10000PcieLoadPepSettings
 *
 * \desc            Loads the PEP settings
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieLoadPepSettings(fm_int                     sw,
                                     fm10000_bootCfg *          bootCfg,
                                     fm_int                     pepId,
                                     fm_registerReadUINT32Func  readFunc,
                                     fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 addr;
    fm_uint32 rv;
    fm_uint32 word0;
    fm_uint32 word1;
    fm_bool   bar4Allowed;
    fm_bool   rxLaneFlip;
    fm_bool   txLaneFlip;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    err = fm10000Eui64ToRegister(bootCfg->pepSerialNumber[pepId], 
                                 &word0, 
                                 &word1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* FM10000_PCIE_CFG_SPD_NUMBER_* */
    err = writeFunc(sw, 
                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_CFG_SPD_NUMBER_L(), pepId),
                    word0 );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, 
                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_CFG_SPD_NUMBER_H(), pepId),
                    word1 );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* FM10000_PCIE_SM_AREA */
    err = writeFunc(sw, 
                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_SM_AREA(0), pepId),
                    word0 );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = writeFunc(sw, 
                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_SM_AREA(1), pepId),
                    word1 );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* PCIE_CFG_SUBID */
    rv = 0;
    FM_SET_FIELD(rv, 
                 FM10000_PCIE_CFG_SUBID, 
                 SubVendorID, 
                 bootCfg->pepSubVendorId[pepId]);

    FM_SET_FIELD(rv, 
                 FM10000_PCIE_CFG_SUBID, 
                 SubDeviceID, 
                 bootCfg->pepSubDeviceId[pepId]);

    err = writeFunc(sw, 
                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_CFG_SUBID(), pepId),
                    rv );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* PCIE_CTRL.BAR4_Allowed
     * PCIE_CTRL.RxLaneFlipEn
     * PCIE_CTRL.TxLaneFlipEn */
    if (bootCfg->mgmtPep == -1)
    {
        bar4Allowed = bootCfg->pepBar4Allowed[pepId];
    }
    else if (bootCfg->mgmtPep == pepId)
    {
        bar4Allowed = 1;
    }
    else
    {
        bar4Allowed = 0;
    }

    rxLaneFlip = bootCfg->pepAutoLaneFlip[pepId] == 1 ? 0 : bootCfg->pepRxLaneFlip[pepId];
    txLaneFlip = bootCfg->pepAutoLaneFlip[pepId] == 1 ? 0 : bootCfg->pepTxLaneFlip[pepId];

    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_CTRL(), pepId);

    err = readFunc(sw, addr, &rv );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_BIT(rv, FM10000_PCIE_CTRL, BAR4_Allowed, !bar4Allowed);
    FM_SET_BIT(rv, FM10000_PCIE_CTRL, RxLaneflipEn, rxLaneFlip);
    FM_SET_BIT(rv, FM10000_PCIE_CTRL, TxLaneflipEn, txLaneFlip);

    err = writeFunc(sw, addr, rv );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieLoadPepSettings */




/*****************************************************************************/
/* fm10000PcieInitAfterReset
 *
 * \desc            Take the PEP out of reset on a "OutOfReset" Interrupt
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       pepId is the PEP's Id in the 0..8 range.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PcieInitAfterReset(fm_int                     sw,
                                    fm10000_bootCfg *          bootCfg,
                                    fm_int                     pepId,
                                    fm_registerReadUINT32Func  readFunc,
                                    fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d pepId=%d\n", sw, pepId);

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 11 (partial).\n");

    /*********************************
     * EAS Step 11. On PCIe reset or 
     * WARM reset events or on transition 
     * from D3hot to D0u, BSM loads 
     * the following PEP settings from 
     * EEPROM.
     *********************************/
   
    /* Execute Step 11 to match EEPROM sequence (i.e. before LTSSM enable)
     * The parameters such as masterLane and {rx,tx}LaneFlip might require
     * having been configured. */
    err = fm10000PcieLoadPepSettings(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 1. BSM or SM sets the 
     * link width, in multiple places
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 1.\n");

    err = fm10000PcieCfgLinkTraining(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 2. BSM or SM sets the 
     * PCIe Gen3 equalization control 
     * register:
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 2.\n");

    err = fm10000PcieCfgLinkGen3CtrlReg(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 2. BSM or SM programs 
     * the PCIe Gen3 equalization 
     * coefficients:
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 3.\n");

    err = fm10000PcieCfgLinkGen3Coef(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 4. BSM or SM sets 
     * PCIE_HOST_LANE_CTRL[i].IntDisableAutoAssert = 1
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 4.\n");
    err = fm10000PcieCfgHostLaneCtrl(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 5. BSM or SM allows 
     * link initialization to begin. 
     * Once the link is up, most of the 
     * following steps can be handled 
     * by the PF host driver.
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS InitAfterReset Step 5.\n");
    err = fm10000PcieCfgltssmEnable(sw, bootCfg, pepId, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PcieInitAfterReset */




/*****************************************************************************/
/* fm10000ColdResetInit
 *
 * \desc            Configure PCIe to bring up PCIe interface.
 *                  This function brings up PCIe similar to
 *                  boot image initialization.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 * 
 * \param[in]       swIsr indicates if PCIe ISR is done by software.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000ColdResetInit(fm_int                     sw,
                               fm10000_bootCfg *          bootCfg,
                               fm_bool                    swIsr,
                               fm_registerReadUINT32Func  readFunc,
                               fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_timestamp t1;
    fm_timestamp t2;
    fm_timestamp tDiff;

    fmGetTime(&t1);
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (bootCfg == NULL)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Starting Cold Reset Init\n");

    /*********************************
     * EAS Step 8. Start Clocks 
     *  
     * PCIE   Output Clock = 500.000000 MHz 
     * EPL    Output Clock = 798.611111 MHz 
     * FABRIC Output Clock = 699.404762 MHz 
     *  
     * SystimeClockSource 
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 8. Start Clocks\n");

    err = fm10000CrStartClocks(sw, bootCfg, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    
    /*********************************
     * EAS Step 9. Configure PCIE Mode 
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 9. Configure PCIE Mode\n"); 

        err = fm10000CrConfigPcieMode(sw, bootCfg, readFunc, writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 10. De-Assert COLD_RESET
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 10. De-Assert COLD_RESET\n");

    err = fm10000CrDeassertColdReset(sw, bootCfg, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 11. Start SBuses
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 11. Start SBuses\n"); 

    err = fm10000CrStartSbuses(sw, bootCfg, readFunc, writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 12. Repair Memories
     *********************************/

    if (!bootCfg->skipMemRepair)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 12. Repair Memories\n"); 

        err = fm10000CrMemoryRepair(sw, bootCfg, readFunc, writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * Bist Memory Check
     *********************************/

    if (bootCfg->enableBistCheck)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "Run BIST Memory Check\n"); 

        err = fm10000RunBist(sw, 
                             bootCfg, 
                             BIST_MEM_CHECK, 
                             BIST_MODULE_PCIE_0 | 
                             BIST_MODULE_PCIE_1 |
                             BIST_MODULE_PCIE_2 |
                             BIST_MODULE_PCIE_3 |
                             BIST_MODULE_PCIE_4 |
                             BIST_MODULE_PCIE_5 |
                             BIST_MODULE_PCIE_6 |
                             BIST_MODULE_PCIE_7 |
                             BIST_MODULE_PCIE_8 |
                             BIST_MODULE_EPL    |
                             BIST_MODULE_FABRIC |
                             BIST_MODULE_TUNNEL |
                             BIST_MODULE_BSM    |
                             BIST_MODULE_CRM    |
                             BIST_MODULE_FIBM   |
                             BIST_MODULE_SBM,
                             readFunc,
                             writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 13. Initialize Memories
     *********************************/

    FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, "EAS Step 13. Initialize Memories\n"); 

    err = fm10000CrInitializeMemories(sw, 
                                      bootCfg, 
                                      readFunc, 
                                      writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /*********************************
     * EAS Step 13.9 Disable PCIe 
     * PCS Interrupts
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "EAS Step 13.9 Disable PCIe PCS Interrupts\n"); 

        err = fm10000CrConfigurePciePcsInterrupts(sw, 
                                                  bootCfg, 
                                                  FALSE, 
                                                  readFunc, 
                                                  writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 14. Program PCIe 
     * Serdes
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "EAS Step 14. Program PCIe Serdes\n"); 

        /* Note that SBUS frequency has been set in
         * step 11. in fm10000UtilSbusInit() */

        err = fm10000UtilsLoadSpicoCode(sw, 
                                        readFunc, 
                                        writeFunc, 
                                        bootCfg->enableFwVerify);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 16. De-Assert Warm Reset 
     * Intentionnaly put before step 15. 
     * See bz27251.
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "EAS Step 16. De-Assert Warm Reset\n"); 

        err = fm10000CrDeassertPcieWarmReset(sw, bootCfg, readFunc, writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 15. Initialize PCIE 
     * Serdes
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "EAS Step 15. Initialize PCIE Serdes\n"); 

        err = fm10000CrInitializePcieSerdes(sw, bootCfg, readFunc, writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /*********************************
     * EAS Step 15.9 Enable PCIe 
     * PCS Interrupts
     *********************************/

    if (!bootCfg->skipPcieInitialization)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                     "EAS Step 15.9 Enable PCIe PCS Interrupts\n"); 

        err = fm10000CrConfigurePciePcsInterrupts(sw, 
                                                  bootCfg, 
                                                  TRUE, 
                                                  readFunc, 
                                                  writeFunc);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /**********************************
     * UNDOCUMENTED STEP - Configure 
     * BSM and Interrupts.
     *********************************/

    err = fm10000CrConfigureBsmInterrupts(sw, 
                                          bootCfg, 
                                          readFunc, 
                                          writeFunc);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Enable Interrupt on the "INT_N" pin and not on the mgmtPep (because
     * the PEP is not up at this point). */
    if (swIsr)
    {
        err = fm10000SetupPCIeISR(sw, 
                                  FM10000_UTIL_PCIE_BSM_INT_INT_PIN,
                                  readFunc, 
                                  writeFunc);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }
    else
    {
        /* We rely on the EEPROM for interrupts, enable the BSM interrupt mask */
        err = fm10000SetupPCIeISR(sw, 
                                  FM10000_UTIL_PCIE_BSM_INT_BSM,
                                  readFunc, 
                                  writeFunc);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    fmGetTime(&t2);

    fmSubTimestamps(&t2, &t1, &tDiff);

    FM_LOG_PRINT("fm10000ColdResetInit:    %d,%d sec\n", (fm_uint)tDiff.sec, (fm_uint)tDiff.usec/1000);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000ColdResetInit */




/*****************************************************************************/
/* fm10000SetupPCIeISR
 *
 * \desc            Setup which module(s) should be receiving the PCIE BSM
 *                  Interrupts. Normally, only one module should be enabled.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       mask selects which module should get the
 *                  PCIE (Boot State Machine Related) Interrutps. See
 *                  FM10000_UTIL_PCIE_BSM_INT_PCIE_0 for the list of
 *                  maskable bits.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000SetupPCIeISR(fm_int                     sw,
                              fm_uint32                  mask,
                              fm_registerReadUINT32Func  readFunc,
                              fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint64 rv64;
    fm_int    pepId;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    /************************************************* 
     * First Disable ALL BSM Related interrupts on 
     * all modules.  
     *************************************************/ 

    /* FM10000_INTERRUPT_MASK_INT */
    err = readFunc64(sw, FM10000_INTERRUPT_MASK_INT(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_INT, PCIE_BSM, 1);

    err = writeFunc64(sw, FM10000_INTERRUPT_MASK_INT(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* FM10000_INTERRUPT_MASK_BSM */
    err = readFunc64(sw, FM10000_INTERRUPT_MASK_BSM(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_BSM, PCIE_BSM, 1);

    err = writeFunc64(sw, FM10000_INTERRUPT_MASK_BSM(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* FM10000_INTERRUPT_MASK_FIBM */
    err = readFunc64(sw, FM10000_INTERRUPT_MASK_FIBM(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_FIBM, PCIE_BSM, 1);

    err = writeFunc64(sw, FM10000_INTERRUPT_MASK_FIBM(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* FM10000_INTERRUPT_MASK_PCIE[0..8] */
    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        err = readFunc64(sw, FM10000_INTERRUPT_MASK_PCIE(pepId, 0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_PCIE, PCIE_BSM, 1);

        err = writeFunc64(sw, FM10000_INTERRUPT_MASK_PCIE(pepId, 0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /************************************************* 
     * Enable BSM Related interrupts on 
     * all selected modules.  
     *************************************************/ 

    /* FM10000_INTERRUPT_MASK_PCIE[0..8] */
    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        if (mask & (FM10000_UTIL_PCIE_BSM_INT_PCIE_0 << pepId))
        {
            err = readFunc64(sw, FM10000_INTERRUPT_MASK_PCIE(pepId, 0), &rv64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

            FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_PCIE, PCIE_BSM, 0);

            err = writeFunc64(sw, FM10000_INTERRUPT_MASK_PCIE(pepId, 0), rv64);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        }
        
    }

    /* FM10000_INTERRUPT_MASK_INT */
    if (mask & FM10000_UTIL_PCIE_BSM_INT_INT_PIN)
    {
        err = readFunc64(sw, FM10000_INTERRUPT_MASK_INT(0), &rv64); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_INT, PCIE_BSM, 0);

        err = writeFunc64(sw, FM10000_INTERRUPT_MASK_INT(0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* FM10000_INTERRUPT_MASK_BSM */
    if (mask & FM10000_UTIL_PCIE_BSM_INT_BSM)
    {
        err = readFunc64(sw, FM10000_INTERRUPT_MASK_BSM(0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_BSM, PCIE_BSM, 0);

        err = writeFunc64(sw, FM10000_INTERRUPT_MASK_BSM(0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* FM10000_INTERRUPT_MASK_FIBM */
    if (mask & FM10000_UTIL_PCIE_BSM_INT_FIBM)
    {
        err = readFunc64(sw, FM10000_INTERRUPT_MASK_FIBM(0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        FM_SET_FIELD64(rv64, FM10000_INTERRUPT_MASK_FIBM, PCIE_BSM, 0);

        err = writeFunc64(sw, FM10000_INTERRUPT_MASK_FIBM(0), rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000SetupPCIeISR */




/*****************************************************************************/
/* fm10000PCIeISR
 *
 * \desc            PCIe interrupt service routine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       bootCfg is a pointer to the boot configuration parameters.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       writeFunc is the function pointer to write 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PCIeISR(fm_int                     sw,
                         fm10000_bootCfg *          bootCfg,
                         fm_registerReadUINT32Func  readFunc,
                         fm_registerWriteUINT32Func writeFunc)
{
    fm_status err = FM_OK;
    fm_uint32 addr;
    fm_uint64 global;
    fm_uint32 pcieInt;
    fm_int    pepId;
    fm_uint32 pepIb;
    fm_uint32 pepIp;
    fm_uint32 pepIpResetMask;
    fm_uint32 bistModule;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    /* read current global interrupt state */
    err = readFunc64(sw, FM10000_GLOBAL_INTERRUPT_DETECT(0), &global );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    pcieInt = FM_GET_FIELD64(global, FM10000_GLOBAL_INTERRUPT_DETECT, PCIE_BSM);

    /* Loop through each PEP interrupt pending */
    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        if (pcieInt & (1 << (FM10000_GLOBAL_INTERRUPT_DETECT_b_PCIE_BSM_0 + pepId)))
        {
            /* Read the Boot Interrupt Mask  */
            addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IB(), pepId);

            err = readFunc(sw, addr, &pepIb );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

            /* Read Interrupt Pending */
            addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), pepId);

            err = readFunc(sw, addr, &pepIp );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

            /* If no interrupt of interest, continue to the next PEP */
            if ( (pepIp & ~pepIb) == 0)
            {
                continue;
            }

            /* For the HOT_RESET, PFLR and DATA_PATH_RESET Interrupts, we
             * need to initialize memories */
            if ( (pepIp & FM10000_INT_PCIE_IP_HOT_RESET) ||
                 (pepIp & FM10000_INT_PCIE_IP_PFLR) ||
                 (pepIp & FM10000_INT_PCIE_IP_DATA_PATH_RESET) )
            {   
                /* Clear the IP before executing memory init such that
                 * if another reset event occurs in the middle of this 
                 * one (requiring a mem init), it can be handled later */
                addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), pepId);

                pepIpResetMask = 0;
                FM_SET_BIT(pepIpResetMask, FM10000_PCIE_IP, HotReset, 1);
                FM_SET_BIT(pepIpResetMask, FM10000_PCIE_IP, PFLR, 1);
                FM_SET_BIT(pepIpResetMask, FM10000_PCIE_IP, DataPathReset, 1);
                
                err = writeFunc(sw, addr, pepIpResetMask );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

                bistModule = BIST_MODULE_PCIE_0 << pepId;

                FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                             "Got (HotReset | PFLR | DataPathReset) Interrupt, "
                             "running Memory Init for PEP %d\n", 
                             pepId);

                err = fm10000RunBist(sw, 
                                     bootCfg, 
                                     BIST_MEM_INIT, 
                                     bistModule, 
                                     readFunc, 
                                     writeFunc);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            }

            if (pepIp & FM10000_INT_PCIE_IP_OUT_OF_RESET)
            {
                /* Clear the IP before executing memory init such that
                 * if another reset event occurs in the middle of this 
                 * one, it can be handled later */
                addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), pepId);

                pepIpResetMask = 0;
                FM_SET_BIT(pepIpResetMask, FM10000_PCIE_IP, OutOfReset, 1);
                
                err = writeFunc(sw, addr, pepIpResetMask );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

                FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                             "Got (OutOfReset) Interrupt, "
                             "Initialize after reset for PEP %d\n", 
                             pepId);

/* There's no HotReset Interrupt on cold reset. Re-run Bist Mem Init?? */
#if 0
                bistModule = BIST_MODULE_PCIE_0 << pepId;

                err = fm10000RunBist(sw, 
                                     bootCfg, 
                                     BIST_MEM_INIT, 
                                     bistModule, 
                                     readFunc, 
                                     writeFunc);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
#endif

                err = fm10000PcieInitAfterReset(sw, 
                                               bootCfg, 
                                               pepId, 
                                               readFunc, 
                                               writeFunc);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            }

            if (pepIp & FM10000_INT_PCIE_IP_VPD_REQUEST)
            {
                /* Clear the IP before such that if another event occurs
                 * in the middle of this one, it can be handled later */
                addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), pepId);

                pepIpResetMask = 0;
                FM_SET_BIT(pepIpResetMask, FM10000_PCIE_IP, VPD_Request, 1);
                
                err = writeFunc(sw, addr, pepIpResetMask );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

                FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM, 
                             "Got (VPD_Request) Interrupt on PEP %d\n", 
                             pepId);

                FM_LOG_WARNING(FM_LOG_CAT_PLATFORM, 
                               "VPD Request Interrupt Not Supported Yet\n");
            }
        }
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PCIeISR */




/*****************************************************************************/
/* fm10000PollInterrupt
 *
 * \desc            Indicate if there is is an interrupt pending.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       intMask is the interrupt mask to check agaist..
 * 
 * \param[out]      intrStatus points to caller-provided storage into which the
 *                  interrupt status is stored.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000PollInterrupt(fm_int                    sw,
                               fm_uint64                 intMask,
                               fm_uint *                 intrStatus,
                               fm_registerReadUINT32Func readFunc)
{
    fm_status err;
    fm_uint64 global;
    fm_uint64 pendingIrq;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    *intrStatus = 0;

    err = readFunc64(sw, FM10000_GLOBAL_INTERRUPT_DETECT(0), &global);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    pendingIrq = global & ~intMask;

    if (pendingIrq & FM10000_UTIL_PCIE_BSM_INT_MASK)
    {
        /* At least one BSM interrupt pending */
        *intrStatus = FM10000_UTIL_INT_STATUS_BSM;
    }

    if (pendingIrq & ~FM10000_UTIL_PCIE_BSM_INT_MASK)
    {
        /* Other interrupt type pending */
        *intrStatus |= FM10000_UTIL_INT_STATUS_OTHER;
    }

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000PollInterrupt */
