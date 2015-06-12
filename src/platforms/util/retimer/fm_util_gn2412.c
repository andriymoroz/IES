/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_gn2412.c
 * Creation Date:   April 2015
 * Description:     Functions for Semtech GN2412 retimer.
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

#include <fm_sdk.h>

#include <platforms/util/fm_util.h>
#include <platforms/util/retimer/fm_util_gn2412.h>

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define CMD_COMPLETION_TIMEOUT      500000 /* usec -> 500 msec */

/* uController TOP CSR */

#define TIMEBASE_CTRL(idx)          (0x70 + (idx * 4))
#define TIMEBASE_REF_CLK_15_8(idx)  (0x71 + (idx * 4))
#define TIMEBASE_REF_CLK_7_0(idx)   (0x72 + (idx * 4))
#define TIMEBASE_STAT(idx)          (0x73 + (idx * 4))

#define GENERIC_STAT                0x78

#define DATALANE_CTRL_0(idx)        (0x80 + (idx * 0x10))
#define DATALANE_CTRL_1(idx)        (0x81 + (idx * 0x10))
#define DATALANE_STAT_0(idx)        (0x82 + (idx * 0x10))
#define DATALANE_STAT_1(idx)        (0x83 + (idx * 0x10))
#define DATALANE_CAL_LOAD_OFF(idx)  (0x84 + (idx * 0x10))
#define DATALANE_CTRL_2(idx)        (0x85 + (idx * 0x10))

#define CMD_REG                     0x140
#define DATA_REG                    0x141

/* GN2412 CSR */

#define FW_VER_MAJOR                0x401
#define FW_VER_MINOR                0x402
#define FW_VER_NANO                 0x403
#define FW_VER_VARIANT              0x404
#define FW_VERSION_BASE             FW_VER_MAJOR

#define BOOT_ERROR_CODE_0           0x405
#define BOOT_ERROR_CODE_1           0x406

#define EXTENDED_MODE               0x42C


/* Timebase CSR */

#define TIMEBASE_CSR_BASE(idx)      (0x800 + (0x400 * idx))

#define PLL_OUT_OF_LOCK(idx)        (0x19 + TIMEBASE_CSR_BASE(idx))
#define IDAC_REF_HI(idx)            (0x23 + TIMEBASE_CSR_BASE(idx))
#define IDAC_REF_LO(idx)            (0x24 + TIMEBASE_CSR_BASE(idx))
#define SEL_DIV1(idx)               (0x25 + TIMEBASE_CSR_BASE(idx))
#define SEL_DIV_PLL(idx)            (0x26 + TIMEBASE_CSR_BASE(idx))
#define FEEDBACK_DIV_SELS(idx)      (0x29 + TIMEBASE_CSR_BASE(idx))
#define FEEDBACK_DIV_SELP(idx)      (0x2A + TIMEBASE_CSR_BASE(idx))
#define WORDCLK_SEL(idx)            (0x2B + TIMEBASE_CSR_BASE(idx))
#define SEL_ICHARGE_PUMP(idx)       (0x2F + TIMEBASE_CSR_BASE(idx))
#define SEL_CFILTER(idx)            (0x30 + TIMEBASE_CSR_BASE(idx))
#define SEL_RFILTER(idx)            (0x31 + TIMEBASE_CSR_BASE(idx))
#define M_DIV_SEL(idx)              (0x3C + TIMEBASE_CSR_BASE(idx))
#define SEL_RX_CLK(idx)             (0x3D + TIMEBASE_CSR_BASE(idx))
#define VCO_FREQ_CAL_4X(idx)        (0x46 + TIMEBASE_CSR_BASE(idx))
#define VCO_FREQ_CAL_CONTROL(idx)   (0x59 + TIMEBASE_CSR_BASE(idx))

/* Data Lane CSR */

#define DATALANE_CSR_BASE(idx)      (0x1000 + (0x400 * idx))

#define BIST_CLK_SEL(idx)           (0x074 + DATALANE_CSR_BASE(idx))
#define RATE_MODE(idx)              (0x108 + DATALANE_CSR_BASE(idx))
#define EN_FRACN_FRCDIV_MODE(idx)   (0x1CA + DATALANE_CSR_BASE(idx))
#define SR_FRACN_NDIV(idx)          (0x1CB + DATALANE_CSR_BASE(idx))
#define FCTRL_EXT_19_16(idx)        (0x1D8 + DATALANE_CSR_BASE(idx))
#define FCTRL_EXT_15_8(idx)         (0x1D9 + DATALANE_CSR_BASE(idx))
#define FCTRL_EXT_7_0(idx)          (0x1DA + DATALANE_CSR_BASE(idx))
#define EN_FRACN_ILO_MODE(idx)      (0x1FA + DATALANE_CSR_BASE(idx))
#define SR_FRACN_REFDIV(idx)        (0x1FC + DATALANE_CSR_BASE(idx))
#define SR_FRACN_VCO_BIAS(idx)      (0x204 + DATALANE_CSR_BASE(idx))
#define SR_FRACN_CAL_DIV2_SEL(idx)  (0x208 + DATALANE_CSR_BASE(idx))
#define SR_FRACN_REFCLK_SEL(idx)    (0x20C + DATALANE_CSR_BASE(idx))


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* Equivalent to table A-17 from GN2412 Programming guide.
   Timebase 0 output frequency = 5156.25
   Reference Clock Frequency   = 156.25<hz

   The only difference is vco_freq_cal_control_reg is not 0xE per the table
*/
static fm_gn2412TimebaseCfg timebaseTable0 =
{
    .sel_div1             = 0x1,
    .m_div_sel            = 0x0,
    .sel_icharge_pump     = 0x12,
    .sel_rfilter          = 0x1,
    .sel_cfilter          = 0x3,
    .feedback_div_sels    = 0x8,
    .feedback_div_selp    = 0xA,
    .vco_freq_cal_control = 0x10,
    .sel_div_pll          = 0x0,
    .vco_freq_cal_4x      = 0x0,
    .wordclk_sel          = 0x3,
    .idac_ref_hi          = 0x4,
    .idac_ref_lo          = 0x4,
};

/* Equivalent to table A-8 from GN2412 Programming guide.
   Timebase 1 output frequency = 5156.25
   Reference Clock Frequency   = 156.25<hz

   The only difference is vco_freq_cal_control_reg is not 0xF per the table
*/
static fm_gn2412TimebaseCfg timebaseTable1 =
{
    .sel_div1             = 0x0,
    .m_div_sel            = 0x0,
    .sel_icharge_pump     = 0x19,
    .sel_rfilter          = 0x3,
    .sel_cfilter          = 0x6,
    .feedback_div_sels    = 0x8,
    .feedback_div_selp    = 0xA,
    .vco_freq_cal_control = 0x10,
    .sel_div_pll          = 0x0,
    .vco_freq_cal_4x      = 0x1,
    .wordclk_sel          = 0x3,
    .idac_ref_hi          = 0x4,
    .idac_ref_lo          = 0x4,
};


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/* RegisterRead
 * \ingroup platformUtils
 *
 * \desc            Read a GN2412 register.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device address.
 *
 * \param[in]       reg is the register address.
 *
 * \param[in,out]   val is a pointer to caller-allocated storage where this
 *                  function should place the read byte.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RegisterRead(fm_uintptr                  handle,
                              fm_utilI2cWriteReadHdnlFunc func,
                              fm_uint                     dev,
                              fm_uint16                   reg,
                              fm_byte *                   val)
{
    fm_byte   data[2];
    fm_status status;

    /* Set register Address with MSB first */
    data[0] = (fm_byte)((reg >> 8) & 0xff);
    data[1] = (fm_byte)(reg & 0xff);
    status = func(handle, dev, data, 2, 0);

    /* Read the register */
    status = func(handle, dev, data, 0, 1);

    *val = data[0];

    return status;

}   /* end RegisterRead */



/*****************************************************************************/
/* RegisterWrite
 * \ingroup platformUtils
 *
 * \desc            Write to a GN2412 register.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       dev is the device address.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       reg is the register address.
 *
 * \param[in]       val is the value to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RegisterWrite(fm_uintptr                  handle,
                               fm_utilI2cWriteReadHdnlFunc func,
                               fm_uint                     dev,
                               fm_uint                     reg,
                               fm_byte                     val)
{
    fm_byte   data[3];
    fm_status status;

    /* Address MSB first */
    data[0] = (fm_byte)((reg >> 8) & 0xff);
    data[1] = (fm_byte)(reg & 0xff);
    data[2] = val;
    status = func(handle, dev, data, 3, 0);

    return status;

}   /* end RegisterWrite */



/*****************************************************************************/
/* IssueCommandCode
 * \ingroup platformUtils
 *
 * \desc            Issue the given command code and wait for the command
 *                  to complete.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       cmd is the command code to issue.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status IssueCommandCode(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev,
                                  fm_byte                     cmd)
{
    fm_status status;
    fm_int    delay;

    /* Issue the command code */
    status = RegisterWrite(handle, func, dev, CMD_REG, cmd);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Wait for command to complete */
    delay = 0;
    while ( delay < CMD_COMPLETION_TIMEOUT )
    {
        status = RegisterRead(handle, func, dev, CMD_REG, &cmd);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        if (cmd == 0)
        {
            /* Command completed */
            break;
        }

        /* wait 1 usec */
        fmDelayBy(0,1000);
        delay++;
    }

    if (delay >= CMD_COMPLETION_TIMEOUT)
    {
        status = FM_FAIL;
    }

ABORT:
    return status;

}   /* end IssueCommandCode */




/*****************************************************************************/
/* WaitForCalibration
 * \ingroup platformUtils
 *
 * \desc            Wait for calibration to complete.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       reg is the register to read to get calibration status.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status WaitForCalibration(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev,
                                    fm_uint                     reg)
{
    fm_status status;
    fm_byte   state;
    fm_int    delay;

    /* Wait for calibration completion */
    delay = 0;
    while ( delay < CMD_COMPLETION_TIMEOUT )
    {
        status = RegisterRead(handle, func, dev, reg, &state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        if ( state == 2 )
        {
            /* Calibration completed */
            break;
        }

        /* wait 1 usec */
        fmDelayBy(0,1000);
        delay++;
    }

    if (delay >= CMD_COMPLETION_TIMEOUT)
    {
        status = FM_FAIL;
    }

ABORT:
    return status;

}   /* end WaitForCalibration */




/*****************************************************************************/
/* ConfigureTimebase
 * \ingroup platformUtils
 *
 * \desc            Configure a timebase.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       timebase is the timebase number.
 *
 * \param[in]       cfg pointer to timebase configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigureTimebase(fm_uintptr                  handle,
                                   fm_utilI2cWriteReadHdnlFunc func,
                                   fm_uint                     dev,
                                   fm_int                      timebase,
                                   fm_gn2412TimebaseCfg *      cfg)
{
    fm_status status;
    fm_byte   val;
    fm_int    refClk;

    /* Route REF_CLK input pin to timebase */
    if ( timebase == 0 )
    {
        val = ( cfg->refClkSel == 0 ) ? 1 : 0;
    }
    else
    {
        val = ( cfg->refClkSel == 0 ) ? 0 : 1;
    }

    status = RegisterWrite(handle, func, dev, SEL_RX_CLK(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Set timebase reference clock input frequency */
    /* The value to be programmed into this register is a 16-bit integer
       value, calculated by taking the reference clock input frequency
       in MHz and rounding it up to the nearest integer value */
    refClk = ceil(cfg->refClk);
    val = (fm_byte)((refClk >> 8) & 0xFF);
    status = RegisterWrite(handle, func, dev, TIMEBASE_REF_CLK_15_8(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = (fm_byte)(refClk & 0xFF);
    status = RegisterWrite(handle, func, dev, TIMEBASE_REF_CLK_7_0(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Configure timebase registers */

    val = cfg->sel_div1;
    status = RegisterWrite(handle, func, dev, SEL_DIV1(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->m_div_sel;
    status = RegisterWrite(handle, func, dev, M_DIV_SEL(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->sel_icharge_pump;
    status = RegisterWrite(handle, func, dev, SEL_ICHARGE_PUMP(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->sel_rfilter;
    status = RegisterWrite(handle, func, dev, SEL_RFILTER(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->sel_cfilter;
    status = RegisterWrite(handle, func, dev, SEL_CFILTER(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->feedback_div_sels;
    status = RegisterWrite(handle, func, dev, FEEDBACK_DIV_SELS(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->feedback_div_selp;
    status = RegisterWrite(handle, func, dev, FEEDBACK_DIV_SELP(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->vco_freq_cal_control;
    status = RegisterWrite(handle, func, dev, VCO_FREQ_CAL_CONTROL(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->sel_div_pll;
    status = RegisterWrite(handle, func, dev, SEL_DIV_PLL(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->vco_freq_cal_4x;
    status = RegisterWrite(handle, func, dev, VCO_FREQ_CAL_4X(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->wordclk_sel;
    status = RegisterWrite(handle, func, dev, WORDCLK_SEL(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->idac_ref_hi;
    status = RegisterWrite(handle, func, dev, IDAC_REF_HI(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = cfg->idac_ref_lo;
    status = RegisterWrite(handle, func, dev, IDAC_REF_LO(timebase), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Enable timebase */
    status = RegisterWrite(handle, func, dev, TIMEBASE_CTRL(timebase), 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    return status;

}   /* end ConfigureTimebase */




/*****************************************************************************/
/* ConfigureDataLane
 * \ingroup platformUtils
 *
 * \desc            Configure a data lane.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 *
 * \param[in]       timebase is the timebase number used by this data lane.
 *
 * \param[in]       cfg pointer to timebase configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigureDataLane(fm_uintptr                  handle,
                                   fm_utilI2cWriteReadHdnlFunc func,
                                   fm_uint                     dev,
                                   fm_int                      lane,
                                   fm_int                      timebase,
                                   fm_gn2412TimebaseCfg *      cfg)
{
    fm_status status;
    fm_byte   val;
    fm_int    offset;

    /* Make sure the data lane is disabled */
    status = RegisterWrite(handle, func, dev, DATALANE_CTRL_0(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Configure the data lane per Table B-1 (GN2412 programming guide) */

    val = ( timebase == 0 ) ? 0 : 1;
    status = RegisterWrite(handle, func, dev, BIST_CLK_SEL(lane), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, RATE_MODE(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, EN_FRACN_ILO_MODE(lane), 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, EN_FRACN_FRCDIV_MODE(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = ( cfg->outFreq < 4500 ) ? 0 : 1;
    status = RegisterWrite(handle, func, dev, SR_FRACN_CAL_DIV2_SEL(lane), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = ( cfg->refClkSel == 0 ) ? 0 : 1;
    status = RegisterWrite(handle, func, dev, SR_FRACN_REFCLK_SEL(lane), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, SR_FRACN_NDIV(lane), 8);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, SR_FRACN_REFDIV(lane), 8);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    val = ( cfg->outFreq < 6250 ) ? 8 : 12;
    status = RegisterWrite(handle, func, dev, SR_FRACN_VCO_BIAS(lane), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, FCTRL_EXT_19_16(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, FCTRL_EXT_15_8(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, FCTRL_EXT_7_0(lane), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Set the calibration load offset */
    /* The value is calculated by dividing 128000 by the timebase output
       frequency in MHz and rounding the result up to the nearest integer. */
    offset = ceil( 128000 / cfg->outFreq );
    val = (fm_byte)(offset & 0xFF);
    status = RegisterWrite(handle, func, dev, DATALANE_CAL_LOAD_OFF(lane), val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Enable transmit output muting option. */
    status = RegisterWrite(handle, func, dev, DATALANE_CTRL_2(lane), 4);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    return status;

}   /* end ConfigureDataLane */




/*****************************************************************************/
/* ConfigureCrosspoint
 * \ingroup platformUtils
 *
 * \desc            Configure crosspoint connections.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigureCrosspoint(fm_uintptr                  handle,
                                     fm_utilI2cWriteReadHdnlFunc func,
                                     fm_uint                     dev)
{
    fm_status status;
    fm_int    i;

    /* IMPORTANT: This mapping is Cathedral Glen specific.
                  It should eventually be configurable
     */

    i = 0;
    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x76);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x98);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0xBA);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x10);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x54);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the "Configure all cross-connection" command code: 0x11 */
    status = IssueCommandCode(handle, func, dev, 0x11);

ABORT:
    return status;

}   /* end ConfigureCrosspoint */




/*****************************************************************************/
/* ConfigurePortPairs
 * \ingroup platformUtils
 *
 * \desc            Configure crosspoint connections.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigurePortPairs(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev)
{
    fm_status status;
    fm_int    i;

    /* IMPORTANT: This mapping is Cathedral Glen specific.
                  It should eventually be configurable
     */

    i = 0;
    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x10);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x54);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x76);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0x98);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), 0xBA);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the command code: 0x15 */
    status = IssueCommandCode(handle, func, dev, 0x15);

ABORT:
    return status;

}   /* end ConfigurePortPairs */



/*****************************************************************************/
/* SetAppMode
 * \ingroup platformUtils
 *
 * \desc            Set the data lane application mode to the given mode.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 *
 * \param[in]       mode is the application mode to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status SetAppMode(fm_uintptr                  handle,
                            fm_utilI2cWriteReadHdnlFunc func,
                            fm_uint                     dev,
                            fm_int                      lane,
                            fm_int                      mode)
{
    fm_status status;
    fm_int    i;

    i = 0;
    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), lane);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, (DATA_REG + i++), mode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the command code: 0x19 */
    status = IssueCommandCode(handle, func, dev, 0x19);

ABORT:
    return status;

}   /* end SetAppMode */




/*****************************************************************************/
/* EnableLane
 * \ingroup platformUtils
 *
 * \desc            Enable a data lane.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to enable.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status EnableLane(fm_uintptr                  handle,
                            fm_utilI2cWriteReadHdnlFunc func,
                            fm_uint                     dev,
                            fm_int                      lane)
{
    fm_status status;

    /* Enable lane with continuous DFE mode */
    status = RegisterWrite(handle, func, dev, DATALANE_CTRL_0(lane), 0x9);

    return status;

}   /* end EnableLane */




/*****************************************************************************/
/* QueryAppStatus
 * \ingroup platformUtils
 *
 * \desc            Query the application status.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to enable.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAppStatus(fm_uintptr                  handle,
                                fm_utilI2cWriteReadHdnlFunc func,
                                fm_uint                     dev,
                                fm_int                      numLane)
{
    fm_status status;
    fm_int    i;
    fm_byte   state;

    /* Issue the command code: 0x1A */
    status = IssueCommandCode(handle, func, dev, 0x1A);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Read back each lane status */
    for ( i = 0 ; i < numLane ; i++ )
    {
        status = RegisterRead(handle, func, dev, DATA_REG + i, &state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        if (state != 0)
        {
            FM_LOG_PRINT("Retimer 0x%x Lane %d status 0x%x\n", dev, i, state);
        }
    }

ABORT:
    return status;

}   /* end QueryAppStatus */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmUtilGN2412GetTimebaseCfg
 * \ingroup platformUtils
 *
 * \desc            Get the Retimer's firmware version.
 *
 * \param[in]       timebase is the timebase number.
 *
 * \param[in]       refClk is the reference clock input frequency.
 *
 * \param[in]       outFreq is the output frequency.
 *
 * \param[in,out]   tbCfg points to caller-allocated storage where this
 *                  function should place the timebase configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetTimebaseCfg(fm_int                timebase,
                                     fm_gn2412RefClk       refClk,
                                     fm_gn2412OutFreq      outFreq,
                                     fm_gn2412TimebaseCfg *tbCfg)
{
    fm_status status;

    /* Currently the only supported combination is
       RefClk = 156.25 MHz and Output Freq = 5156.25 MHz */

    if ( refClk  == FM_GN2412_REFCLK_156P25 &&
         outFreq == FM_GN2412_OUTFREQ_5156P25)
    {
        *tbCfg = (timebase == 0) ? timebaseTable0 : timebaseTable1;

        tbCfg->refClk  = 156.25;
        tbCfg->outFreq = 5156.25;

        status = FM_OK;
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    return status;

}   /* end fmUtilGN2412GetTimebaseCfg */




/*****************************************************************************/
/* fmUtilGN2412GetFirmwareVersion
 * \ingroup platformUtils
 *
 * \desc            Get the Retimer's firmware version.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in,out]   version points to caller-allocated storage where this
 *                  function should place the FW version number.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetFirmwareVersion(fm_uintptr                  handle,
                                         fm_utilI2cWriteReadHdnlFunc func,
                                         fm_uint                     dev,
                                         fm_byte *                   version)
{
    fm_status status;
    fm_uint16 reg;
    fm_uint   i;

    /* Read Version registers */
    for ( i = 0 ; i < FM_GN2412_VERSION_NUM_LEN ; i++ )
    {
        reg = FW_VERSION_BASE + i;
        status = RegisterRead(handle, func, dev, reg, &version[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

ABORT:
    return status;

}   /* end fmUtilGN2412GetFirmwareVersion */



/*****************************************************************************/
/* fmUtilGN2412GetBootErrorCode
 * \ingroup platformUtils
 *
 * \desc            Returns the Retimer's boot error code.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in,out]   errCode0 points to caller-allocated storage where this
 *                  function should place the BOOT_ERROR_CODE_0_REG value.
 *
 * \param[in,out]   errCode1 points to caller-allocated storage where this
 *                  function should place the BOOT_ERROR_CODE_1_REG value.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetBootErrorCode(fm_uintptr                  handle,
                                       fm_utilI2cWriteReadHdnlFunc func,
                                       fm_uint                     dev,
                                       fm_byte *                   errCode0,
                                       fm_byte *                   errCode1)
{
    fm_status status;

    status = RegisterRead(handle, func, dev, BOOT_ERROR_CODE_0, errCode0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterRead(handle, func, dev, BOOT_ERROR_CODE_1, errCode1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    return status;

}   /* end fmUtilGN2412GetBootErrorCode */




/*****************************************************************************/
/* fmUtilGN2412Initialize
 * \ingroup platformUtils
 *
 * \desc            Returns the Retimer's boot error code.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       tbCfg pointer to timebase 0 and 1 configuration.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmUtilGN2412Initialize(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev,
                                 fm_gn2412TimebaseCfg *      tbCfg)
{
    fm_status status;
    fm_int    i;

    /* Set firmware for API mode (0xff)*/
    status = RegisterWrite(handle, func, dev, EXTENDED_MODE, 0xFF);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Wait for receive termination calibration completion */
    status = WaitForCalibration(handle, func, dev, GENERIC_STAT);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Configure both timebases */
    for ( i = 0 ; i < FM_GN2412_NUM_TIMEBASES ; i++ )
    {
        status = ConfigureTimebase(handle, func, dev, i, &tbCfg[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Wait for both timebase calibration completion */
    for ( i = 0 ; i < FM_GN2412_NUM_TIMEBASES ; i++ )
    {
        status = WaitForCalibration(handle, func, dev, TIMEBASE_STAT(i));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Configure the data lanes */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        /* Currently all lanes use timebase 1
           This should eventually be configurable */
        status = ConfigureDataLane(handle, func, dev, i, 1, &tbCfg[1]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Configure Crosspoint Connections */
    status = ConfigureCrosspoint(handle, func, dev);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Configure port pairs */
    status = ConfigurePortPairs(handle, func, dev);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Set the application mode to Auto-config: 0x74 for all lanes */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        status = SetAppMode(handle, func, dev, i, 0x74);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Enable all lanes */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        status = EnableLane(handle, func, dev, i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    status = QueryAppStatus(handle, func, dev, FM_GN2412_NUM_LANES);

 ABORT:
    return status;

}   /* end fmUtilGN2412Initialize */
