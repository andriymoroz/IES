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

#define CMD_COMPLETION_TIMEOUT      500 /* 500 msec */

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

#define NUM_DIAG_COUNTERS           13

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
 * \param[out]      val is a pointer to caller-allocated storage where this
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
    fm_status    status;
    fm_int       loop;
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;

    /* Issue the command code */
    status = RegisterWrite(handle, func, dev, CMD_REG, cmd);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Wait a maximum of 500 msec for command to complete */
    fmGetTime(&start);

    loop = 0;
    delTime = 0;
    while ( delTime < CMD_COMPLETION_TIMEOUT )
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
        loop++;

        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);

        /* Convert in msec */
        delTime = diff.sec*1000 + diff.usec/1000;
    }

    if ( delTime >= CMD_COMPLETION_TIMEOUT )
    {
        FM_LOG_PRINT("Command code timeout: delTime=%d msec (loop %d)\n",
                     delTime,
                     loop);
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
    fm_status    status;
    fm_byte      state;
    fm_int       loop;
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;

    /* Wait a maximum of 500 msec for calibration completion */
    fmGetTime(&start);

    loop = 0;
    delTime = 0;
    while ( delTime < CMD_COMPLETION_TIMEOUT )
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
        loop++;

        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);

        /* Convert in msec */
        delTime = diff.sec*1000 + diff.usec/1000;
    }

    if ( delTime >= CMD_COMPLETION_TIMEOUT )
    {
        FM_LOG_PRINT("Command code timeout: delTime=%d msec (loop %d)\n",
                     delTime,
                     loop);
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
/* ConfigureAllCrosspoint
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
 * \param[in]       cfg pointer to GN2412 configuration.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigureAllCrosspoint(fm_uintptr                  handle,
                                        fm_utilI2cWriteReadHdnlFunc func,
                                        fm_uint                     dev,
                                        fm_gn2412Cfg *              cfg)
{
    fm_status status;
    fm_uint16 reg;
    fm_byte   data;
    fm_int    i;

    reg = DATA_REG;

    /* Configure Cross-Connections per values provided in config file */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i+=2 )
    {
        data = (cfg->lane[i+1].rxPort << 4) | cfg->lane[i].rxPort;

        status = RegisterWrite(handle, func, dev, reg++, data);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Issue the "Configure all cross-connection" command code: 0x11 */
    status = IssueCommandCode(handle, func, dev, 0x11);

ABORT:
    return status;

}   /* end ConfigureAllCrosspoint */



/*****************************************************************************/
/* QueryAllCrosspoint
 * \ingroup platformUtils
 *
 * \desc            Returns the Rx port assigned as source to each Tx ports.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[out]      rxPortTbl points to caller-allocated storage where this
 *                  function should place the Rx ports.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAllCrosspoint(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev,
                                    fm_int                      rxPortTbl[])
{
    fm_status status;
    fm_uint16 reg;
    fm_int    i;
    fm_int    txPort;
    fm_byte   rxPort;

    /* Issue the "Query All Cross-Connections" command code: 0x10 */
    status = IssueCommandCode(handle, func, dev, 0x10);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Now read back the values from the data buffer registers DATA_REG[n]
       DATA_0_REG[3:0] => Tx Port 0 source
       DATA_0_REG[7:4] => Tx Port 1 source
       ...
       ...
       DATA_5_REG[3:0] => Tx Port 10 source
       DATA_5_REG[7:4] => Tx Port 11 source
    */
    reg = DATA_REG;
    txPort = 0;
    for ( i = 0; i < FM_GN2412_NUM_LANES/2 ; i++ )
    {
        status = RegisterRead(handle, func, dev, reg++, &rxPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        rxPortTbl[txPort++] = rxPort & 0xF;
        rxPortTbl[txPort++] = (rxPort >> 4) & 0xF;
    }

ABORT:
    return status;

}   /* end QueryAllCrosspoint */




/*****************************************************************************/
/* DumpAllCrosspoint
 * \ingroup platformUtils
 *
 * \desc            Dump all corss-connections.
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
static fm_status DumpAllCrosspoint(fm_uintptr                  handle,
                                   fm_utilI2cWriteReadHdnlFunc func,
                                   fm_uint                     dev)
{
    fm_status status;
    fm_int    rxPort[FM_GN2412_NUM_LANES];
    fm_int    txPort;

    status = QueryAllCrosspoint(handle, func, dev, rxPort);

    if ( status == FM_OK )
    {
        FM_LOG_PRINT("Cross-connections (dev 0x%2x)\n"
                     "============================\n"
                     " TxPort     RxPort \n",
                     dev);

        for ( txPort = 0 ; txPort < FM_GN2412_NUM_LANES ; txPort++ )
        {
            FM_LOG_PRINT("   %2d  <---  %2d\n", txPort, rxPort[txPort]);
        }
    }
    else
    {
        FM_LOG_PRINT("Error reading the cross-connections for dev 0x%x\n", dev);
    }

    return status;

}   /* end DumpAllCrosspoint */




/*****************************************************************************/
/* ConfigurePortPairs
 * \ingroup platformUtils
 *
 * \desc            Configure all interface port pairings.
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
    fm_uint16 reg;

    reg = DATA_REG;

    status = RegisterWrite(handle, func, dev, reg++, 0x10);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, 0x32);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, 0x54);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, 0x76);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, 0x98);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, 0xBA);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the command code: 0x15 */
    status = IssueCommandCode(handle, func, dev, 0x15);

ABORT:
    return status;

}   /* end ConfigurePortPairs */


/*****************************************************************************/
/* QueryAllPortPairings
 * \ingroup platformUtils
 *
 * \desc            Returns the Tx port paired to each Rx port.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[out]      txPortTbl points to caller-allocated storage where this
 *                  function should place the Tx ports.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAllPortPairings(fm_uintptr                  handle,
                                      fm_utilI2cWriteReadHdnlFunc func,
                                      fm_uint                     dev,
                                      fm_int                      txPortTbl[])
{
    fm_status status;
    fm_uint16 reg;
    fm_int    i;
    fm_int    rxPort;
    fm_byte   txPort;

    /* Issue the "Query All Cross-Connections" command code: 0x14 */
    status = IssueCommandCode(handle, func, dev, 0x14);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Now read back the values from the data buffer registers DATA_REG[n]
       DATA_0_REG[3:0] => Rx Port 0 Tx pair
       DATA_0_REG[7:4] => Rx Port 1 Tx pair
       ...
       ...
       DATA_5_REG[3:0] => Rx Port 10 Tx pair
       DATA_5_REG[7:4] => Rx Port 11 Tx pair
    */
    reg = DATA_REG;
    rxPort = 0;
    for ( i = 0; i < FM_GN2412_NUM_LANES/2 ; i++ )
    {
        status = RegisterRead(handle, func, dev, reg++, &txPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        txPortTbl[rxPort++] = txPort & 0xF;
        txPortTbl[rxPort++] = (txPort >> 4) & 0xF;
    }

ABORT:
    return status;

}   /* end QueryAllPortPairings */




/*****************************************************************************/
/* DumpAllPortPairings
 * \ingroup platformUtils
 *
 * \desc            Dump all port pairings.
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
static fm_status DumpAllPortPairings(fm_uintptr                  handle,
                                     fm_utilI2cWriteReadHdnlFunc func,
                                     fm_uint                     dev)
{
    fm_status status;
    fm_int    txPort[FM_GN2412_NUM_LANES];
    fm_int    rxPort;

    status = QueryAllPortPairings(handle, func, dev, txPort);

    if ( status == FM_OK )
    {
        FM_LOG_PRINT("Port Pairings (dev 0x%2x)\n"
                     "=========================\n"
                     " RxPort     TxPort \n",
                     dev);

        for ( rxPort = 0 ; rxPort < FM_GN2412_NUM_LANES ; rxPort++ )
        {
            FM_LOG_PRINT("   %2d  <---  %2d\n", rxPort, txPort[rxPort]);
        }
    }
    else
    {
        FM_LOG_PRINT("Error reading the cross-connections for dev 0x%x\n", dev);
    }

    return status;

}   /* end DumpAllPortPairings */



/*****************************************************************************/
/* ConfigureLaneDeEmphasis
 * \ingroup platformUtils
 *
 * \desc            Set the transmitter equalization (de-emphasis) for the
 *                  specified lane.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[in]       polarity is TX polarity.
 * 
 * \param[in]       preTap is the Transmitter Cm coefficient (pre-tap). 
 * 
 * \param[in]       att is the attenuation setting for transmitter 
 *                  (output swing). 
 * 
 * \param[in]       postTap is the Transmitter C1 coefficient (post-tap). 
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status ConfigureLaneDeEmphasis(fm_uintptr                  handle,
                                         fm_utilI2cWriteReadHdnlFunc func,
                                         fm_uint                     dev,
                                         fm_int                      lane,
                                         fm_int                      polarity,
                                         fm_int                      preTap,
                                         fm_int                      att,
                                         fm_int                      postTap)
{
    fm_status status;
    fm_uint16 reg;

    reg = DATA_REG;

    status = RegisterWrite(handle, func, dev, reg++, lane);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
#if 1
    status = RegisterWrite(handle, func, dev, reg++, polarity);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
#else
    i++;
#endif
    status = RegisterWrite(handle, func, dev, reg++, preTap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, att);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterWrite(handle, func, dev, reg++, postTap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the "Control Non-KR Transmit Equalization" command code: 0x17 */
    status = IssueCommandCode(handle, func, dev, 0x17);

ABORT:
    return status;

}   /* end ConfigureLaneDeEmphasis */




/*****************************************************************************/
/* QueryLaneDeEmphasis
 * \ingroup platformUtils
 *
 * \desc            Query the transmitter equalization (de-emphasis)
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[out]      polarity points to caller-allocated storage where this
 *                  function should place the Tx polarity value.
 * 
 * \param[out]      preTap points to caller-allocated storage where this
 *                  function should place the Transmitter Cm coefficient
 *                  (pre-tap). 
 * 
 * \param[out]      attenuation points to caller-allocated storage where this
 *                  function should place the attenuation setting for
 *                  transmitter (output swing). 
 * 
 * \param[out]      postTap points to caller-allocated storage where this
 *                  function should place the Transmitter C1 coefficient
 *                  (post-tap). 
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryLaneDeEmphasis(fm_uintptr                  handle,
                                     fm_utilI2cWriteReadHdnlFunc func,
                                     fm_uint                     dev,
                                     fm_int                      lane,
                                     fm_byte                    *polarity,
                                     fm_byte                    *preTap,
                                     fm_byte                    *attenuation,
                                     fm_byte                    *postTap)
{
    fm_status status;
    fm_uint16 reg;
    fm_byte   txPort;

    /* Write the lane number (Tx port index 0-11) */
    status = RegisterWrite(handle, func, dev, DATA_REG, lane);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the "Query Non-KR Transmit Equalization" command code: 0x16 */
    status = IssueCommandCode(handle, func, dev, 0x16);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Now read back the values from the data buffer registers DATA_REG[n]
       DATA_0_REG => TxPort index
       DATA_1_REG => Tx Polarity
       DATA_2_REG => Tx Pre-tap coefficient
       DATA_3_REG => Tx attenuation
       DATA_4_REG => Tx Post-tap coefficient
    */
    reg = DATA_REG;
    status = RegisterRead(handle, func, dev, reg++, &txPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    if ( txPort != lane )
    {
        FM_LOG_PRINT("!!! Wrong txPort 0x%x (expecting 0x%x)\n", txPort, lane);
    }

    status = RegisterRead(handle, func, dev, reg++, polarity);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterRead(handle, func, dev, reg++, preTap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterRead(handle, func, dev, reg++, attenuation);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterRead(handle, func, dev, reg++, postTap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    return status;

}   /* end QueryLaneDeEmphasis */




/*****************************************************************************/
/* DumpLaneDeEmphasis
 * \ingroup platformUtils
 *
 * \desc            Print the De-Emphasis values for the specified lane.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status DumpLaneDeEmphasis(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev,
                                    fm_int                      lane)
{
    fm_status status;
    fm_byte   polarity;
    fm_byte   preTap;
    fm_byte   attenuation;
    fm_byte   postTap;

    status = QueryLaneDeEmphasis(handle,
                                 func,
                                 dev,
                                 lane,
                                 &polarity,
                                 &preTap,
                                 &attenuation,
                                 &postTap);

    if ( status == FM_OK )
    {
        FM_LOG_PRINT("  %2d   %02Xh    %2d     %2d    %2d\n",
                     lane,
                     polarity,
                     preTap,
                     attenuation,
                     postTap);
    }
    else
    {
        FM_LOG_PRINT("Error reading De-Emphasis for dev 0x%x lane %d\n",
                     dev,
                     lane);
    }

    return status;

}   /* end DumpLaneDeEmphasis */




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
/* QueryAppMode
 * \ingroup platformUtils
 *
 * \desc            Query the application mode for the given lane
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to read.
 * 
 * \param[out]      mode points to caller-allocated storage where this
 *                  function should place the application mode value.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAppMode(fm_uintptr                  handle,
                              fm_utilI2cWriteReadHdnlFunc func,
                              fm_uint                     dev,
                              fm_int                      lane,
                              fm_byte                    *mode)
{
    fm_status status;
    fm_uint16 reg;
    fm_byte   rxPort;

    /* Write the lane number (Rx port index 0-11) */
    status = RegisterWrite(handle, func, dev, DATA_REG, lane);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the "Query Application Modes" command code: 0x18 */
    status = IssueCommandCode(handle, func, dev, 0x18);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Now read back the values from the data buffer registers DATA_REG[n]
       DATA_0_REG => RxPort index
       DATA_1_REG => Mode
    */
    reg = DATA_REG;
    status = RegisterRead(handle, func, dev, reg++, &rxPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    if ( rxPort != lane )
    {
        FM_LOG_PRINT("!!! Wrong rxPort 0x%x (expecting 0x%x)\n", rxPort, lane);
    }

    status = RegisterRead(handle, func, dev, reg++, mode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    return status;

}   /* end QueryAppMode */




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
/* DisableLane
 * \ingroup platformUtils
 *
 * \desc            Disable a data lane.
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
static fm_status DisableLane(fm_uintptr                  handle,
                             fm_utilI2cWriteReadHdnlFunc func,
                             fm_uint                     dev,
                             fm_int                      lane)
{
    fm_status status;

    /* Enable lane with continuous DFE mode */
    status = RegisterWrite(handle, func, dev, DATALANE_CTRL_0(lane), 0x0);

    return status;

}   /* end DisableLane */




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
 * \param[out]      appStatus points to caller-allocated storage where this
 *                  function should place the status for all lanes.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAppStatus(fm_uintptr                  handle,
                                fm_utilI2cWriteReadHdnlFunc func,
                                fm_uint                     dev,
                                fm_byte *                   appStatus)
{
    fm_status status;
    fm_int    i;
    fm_byte   state;

    /* Issue the command code: 0x1A */
    status = IssueCommandCode(handle, func, dev, 0x1A);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Read back each lane status */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        status = RegisterRead(handle, func, dev, DATA_REG + i, &state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        if (appStatus)
        {
            appStatus[i] = state;

        }
        else if (state != 0)
        {
            FM_LOG_PRINT("Retimer 0x%x Lane %d status 0x%x\n", dev, i, state);
        }
    }

ABORT:
    return status;

}   /* end QueryAppStatus */




/*****************************************************************************/
/* QueryAppRestartDiagCounts
 * \ingroup platformUtils
 *
 * \desc            Query the application restart diagnostic counts for the
 *                  specified lane on the specified retimer.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 * 
 * \param[in]       lane is the data lane to query.
 * 
 * \param[out]      reg points to caller-allocated storage where this
 *                  function should place the status for all lanes.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
static fm_status QueryAppRestartDiagCounts(fm_uintptr                  handle,
                                           fm_utilI2cWriteReadHdnlFunc func,
                                           fm_uint                     dev,
                                           fm_int                      lane,
                                           fm_byte *                   reg)
{
    fm_status status;
    fm_int    i;
    fm_byte   cnt;

    /* Write the lane number (Rx port index 0-11) */
    status = RegisterWrite(handle, func, dev, DATA_REG, lane);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Issue the command code: 0x1C */
    status = IssueCommandCode(handle, func, dev, 0x1C);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Read back the counters */
    for ( i = 0 ; i < NUM_DIAG_COUNTERS ; i++ )
    {
        status = RegisterRead(handle, func, dev, DATA_REG + i, &cnt);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        reg[i] = cnt;
    }

ABORT:
    return status;

}   /* end QueryAppRestartDiagCounts */


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
 * \param[out]      tbCfg points to caller-allocated storage where this
 *                  function should place the timebase configuration.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetTimebaseCfg(fm_int                timebase,
                                     fm_gn2412RefClk       refClk,
                                     fm_gn2412OutFreq      outFreq,
                                     fm_gn2412TimebaseCfg *tbCfg)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, 
                 "timebase=%d, refClk=%d, outFreq=%d\n", 
                 timebase, 
                 refClk,
                 outFreq);

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

    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

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
 * \param[out]      version points to caller-allocated storage where this
 *                  function should place the FW version number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
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

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, "dev=0x%x\n", dev);

    /* Read Version registers */
    for ( i = 0 ; i < FM_GN2412_VERSION_NUM_LEN ; i++ )
    {
        reg = FW_VERSION_BASE + i;
        status = RegisterRead(handle, func, dev, reg, &version[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

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
 * \param[out]      errCode0 points to caller-allocated storage where this
 *                  function should place the BOOT_ERROR_CODE_0_REG value.
 *
 * \param[out]      errCode1 points to caller-allocated storage where this
 *                  function should place the BOOT_ERROR_CODE_1_REG value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetBootErrorCode(fm_uintptr                  handle,
                                       fm_utilI2cWriteReadHdnlFunc func,
                                       fm_uint                     dev,
                                       fm_byte *                   errCode0,
                                       fm_byte *                   errCode1)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, "dev=0x%x\n", dev);

    status = RegisterRead(handle, func, dev, BOOT_ERROR_CODE_0, errCode0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    status = RegisterRead(handle, func, dev, BOOT_ERROR_CODE_1, errCode1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmUtilGN2412GetBootErrorCode */




/*****************************************************************************/
/* fmUtilGN2412DumpConnections
 * \ingroup platformUtils
 *
 * \desc            Dump the RxPort to TxPort connections and port pairings.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmUtilGN2412DumpConnections(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev)
{
    DumpAllCrosspoint(handle, func, dev);
    DumpAllPortPairings(handle, func, dev);

}   /* end fmUtilGN2412DumpConnections */




/*****************************************************************************/
/* fmUtilGN2412DumpTxEqualization
 * \ingroup platformUtils
 *
 * \desc            Dump the Tx equalization coefficients for all lanes
 *                  on the specified device.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmUtilGN2412DumpTxEqualization(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev)
{
    fm_int lane;

    FM_LOG_PRINT("Tx Equalizer coefficients (dev 0x%2x)\n"
                 "====================================\n"
                 " Lane  Pol  PreTap  Att  PostTap \n",
                 dev);

    for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
    {
        DumpLaneDeEmphasis(handle, func, dev, lane);
    }


}   /* end fmUtilGN2412DumpTxEqualization */




/*****************************************************************************/
/* fmUtilGN2412DumpAppMode
 * \ingroup platformUtils
 *
 * \desc            Dump the application mode for all lanes on the specified
 *                  device.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmUtilGN2412DumpAppMode(fm_uintptr                  handle,
                             fm_utilI2cWriteReadHdnlFunc func,
                             fm_uint                     dev)
{
    fm_status status;
    fm_byte   mode;
    fm_int    lane;

    FM_LOG_PRINT("Application Mode (dev 0x%2x)\n"
                 "============================================\n"
                 "Lane:   0  1  2  3  4  5  6  7  8  9 10 11\n"
                 "(hex)  ",
                 dev);

    for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
    {
        status = QueryAppMode(handle, func, dev, lane, &mode);
        if (status == FM_OK)
        {
            FM_LOG_PRINT("%02X ", mode);
        }
        else
        {
            FM_LOG_PRINT("Err");
        }
    }
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\n");


}   /* end fmUtilGN2412DumpAppMode */




/*****************************************************************************/
/* fmUtilGN2412DumpAppStatus
 * \ingroup platformUtils
 *
 * \desc            Dump the application status for all lanes on the specified
 *                  retimer.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmUtilGN2412DumpAppStatus(fm_uintptr                  handle,
                               fm_utilI2cWriteReadHdnlFunc func,
                               fm_uint                     dev)
{
    fm_status status;
    fm_byte   appStatus[FM_GN2412_NUM_LANES];
    fm_int    lane;


    FM_LOG_PRINT("All Application Status (dev 0x%2x)\n"
                 "============================================\n"
                 "Lane:   0  1  2  3  4  5  6  7  8  9 10 11\n"
                 "(hex)  ",
                 dev);

    status = QueryAppStatus(handle, func, dev, appStatus);

    if (status == FM_OK)
    {
        for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
        {
            FM_LOG_PRINT("%02X ",appStatus[lane]);
        }
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("\n");
    }
    else
    {
        FM_LOG_PRINT("Error reading the application status\n");
    }


}   /* end fmUtilGN2412DumpAppStatus */




/*****************************************************************************/
/* fmUtilGN2412DumpAppRestartDiagCnt
 * \ingroup platformUtils
 *
 * \desc            Dump the application restart diagnostic counts for the
 *                  specified retimer.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmUtilGN2412DumpAppRestartDiagCnt(fm_uintptr                  handle,
                                       fm_utilI2cWriteReadHdnlFunc func,
                                       fm_uint                     dev)
{
    fm_status status;
    fm_byte   counters[FM_GN2412_NUM_LANES][NUM_DIAG_COUNTERS];
    fm_int    lane;
    fm_int    i;


    FM_LOG_PRINT("Application Restart Diagnostic Counts(dev 0x%2x)\n"
                 "===============================================\n",
                 dev);

    for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
    {
        status = QueryAppRestartDiagCounts(handle,
                                           func, 
                                           dev, 
                                           lane, 
                                           &counters[lane][0]);
        if ( status != FM_OK )
        {
            FM_LOG_PRINT("Error reading the restart diagnostic counters\n");
            return;
        }
    }

    for ( i = 0 ; i < NUM_DIAG_COUNTERS ; i++ )
    {
        FM_LOG_PRINT("DATA_%02d ", i);
        for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
        {
            FM_LOG_PRINT("%3d ",counters[lane][i]);
        }
        FM_LOG_PRINT("\n");
    }
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\n");


}   /* end fmUtilGN2412DumpAppRestartDiagCnt */




/*****************************************************************************/
/* fmUtilGN2412RegisterRead
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
 * \param[out]      val is a pointer to caller-allocated storage where this
 *                  function should place the read byte.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412RegisterRead(fm_uintptr                  handle,
                                   fm_utilI2cWriteReadHdnlFunc func,
                                   fm_uint                     dev,
                                   fm_uint16                   reg,
                                   fm_byte *                   val)
{
    fm_status status;

    status = RegisterRead(handle, func, dev, reg, val);

    return status;

}   /* end fmUtilGN2412RegisterRead */




/*****************************************************************************/
/* fmUtilGN2412RegisterWrite
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
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412RegisterWrite(fm_uintptr                  handle,
                                    fm_utilI2cWriteReadHdnlFunc func,
                                    fm_uint                     dev,
                                    fm_uint16                   reg,
                                    fm_byte                     val)
{
    fm_status status;

    status = RegisterWrite(handle, func, dev, reg, val);

    return status;

}   /* end fmUtilGN2412RegisterWrite */




/*****************************************************************************/
/* fmUtilGN2412SetAppMode
 * \ingroup platformUtils
 *
 * \desc            Set the data lane application mode for the specified
 *                  device and lane.
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
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412SetAppMode(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev,
                                 fm_int                      lane,
                                 fm_int                      mode)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, 
                 "dev=0x%x, lane=%d, mode=0x%x\n", 
                 dev, 
                 lane,
                 mode);

    if ( lane >= 0 && lane < FM_GN2412_NUM_LANES )
    {
        /* Disable the data lane first */
        status = DisableLane(handle, func, dev, lane);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        /* Set the application mode */
        status = SetAppMode(handle, func, dev, lane, mode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

        /* Re-enable the lane */
        status = EnableLane(handle, func, dev, lane);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmUtilGN2412SetAppMode */




/*****************************************************************************/
/* fmUtilGN2412GetLaneTxEq
 * \ingroup platformUtils
 *
 * \desc            Get the transmitter equalization coefficients for the
 *                  specified device and lane
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[out]      polarity points to caller-allocated storage where this
 *                  function should place the Tx polarity value.
 * 
 * \param[out]      preTap points to caller-allocated storage where this
 *                  function should place the Transmitter Cm coefficient
 *                  (pre-tap). 
 * 
 * \param[out]      attenuation points to caller-allocated storage where this
 *                  function should place the attenuation setting for
 *                  transmitter (output swing). 
 * 
 * \param[out]      postTap points to caller-allocated storage where this
 *                  function should place the Transmitter C1 coefficient
 *                  (post-tap). 
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmUtilGN2412GetLaneTxEq(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev,
                                  fm_int                      lane,
                                  fm_int *                    polarity,
                                  fm_int *                    preTap,
                                  fm_int *                    attenuation,
                                  fm_int *                    postTap)
{
    fm_status status;
    fm_byte   pol;
    fm_byte   pre;
    fm_byte   att;
    fm_byte   post;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, "dev=0x%x lane=%d\n", dev, lane);

    status = QueryLaneDeEmphasis(handle,
                                 func,
                                 dev,
                                 lane,
                                 &pol,
                                 &pre,
                                 &att,
                                 &post);

    if ( status == FM_OK )
    {
        *polarity = pol;
        *preTap = pre;
        *attenuation = att;
        *postTap = post;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmUtilGN2412GetLaneTxEq */




/*****************************************************************************/
/* fmUtilGN2412SetLaneTxEq
 * \ingroup platformUtils
 *
 * \desc            Set the transmitter equalization (de-emphasis) for the
 *                  specified lane.
 *
 * \param[in]       handle is the handle to the I2C device.
 *
 * \param[in]       func is the I2C write read function to call.
 *
 * \param[in]       dev is the device I2C address.
 *
 * \param[in]       lane is the data lane to configure.
 * 
 * \param[in]       polarity is TX polarity.
 * 
 * \param[in]       preTap is the Transmitter Cm coefficient (pre-tap). 
 * 
 * \param[in]       attenuation is the attenuation setting for transmitter 
 *                  (output swing). 
 * 
 * \param[in]       postTap is the Transmitter C1 coefficient (post-tap). 
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412SetLaneTxEq(fm_uintptr                  handle,
                                  fm_utilI2cWriteReadHdnlFunc func,
                                  fm_uint                     dev,
                                  fm_int                      lane,
                                  fm_int                      polarity,
                                  fm_int                      preTap,
                                  fm_int                      attenuation,
                                  fm_int                      postTap)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, 
                 "dev=0x%x lane=%d pol=0x%x preTap=%d att=%d postTap=%d\n", 
                 dev, 
                 lane,
                 polarity,
                 preTap,
                 attenuation,
                 postTap);

    if ( lane >= 0 && lane < FM_GN2412_NUM_LANES )
    {
        status = ConfigureLaneDeEmphasis(handle,
                                         func,
                                         dev,
                                         lane,
                                         polarity,
                                         preTap,
                                         attenuation,
                                         postTap);
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmUtilGN2412SetLaneTxEq */




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
 * \param[in]       cfg pointer to GN2412 configuration.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilGN2412Initialize(fm_uintptr                  handle,
                                 fm_utilI2cWriteReadHdnlFunc func,
                                 fm_uint                     dev,
                                 fm_gn2412Cfg *              cfg)
{
    fm_status status;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, "dev=0x%x\n", dev);

    if (cfg == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Set firmware for API mode (0xff)*/
    status = RegisterWrite(handle, func, dev, EXTENDED_MODE, 0xFF);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Wait for receive termination calibration completion */
    status = WaitForCalibration(handle, func, dev, GENERIC_STAT);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Configure both timebases */
    for ( i = 0 ; i < FM_GN2412_NUM_TIMEBASES ; i++ )
    {
        status = ConfigureTimebase(handle, func, dev, i, &cfg->timebase[i]);
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
        status = ConfigureDataLane(handle, func, dev, i, 1, &cfg->timebase[1]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Configure all Crosspoint Connections */
    status = ConfigureAllCrosspoint(handle, func, dev, cfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    if (cfg->setTxEq)
    {
        /* Configure De-Emphasis per values provided in config file */
        for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
        {
            status = ConfigureLaneDeEmphasis(handle,
                                             func,
                                             dev,
                                             i,
                                             cfg->lane[i].polarity,
                                             cfg->lane[i].preTap,
                                             cfg->lane[i].attenuation,
                                             cfg->lane[i].postTap);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
        }
    }

    /* Configure port pairs */
    status = ConfigurePortPairs(handle, func, dev);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);

    /* Set the application mode for all lanes */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        status = SetAppMode(handle, func, dev, i, cfg->lane[i].appMode);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    /* Enable all lanes */
    for ( i = 0 ; i < FM_GN2412_NUM_LANES ; i++ )
    {
        status = EnableLane(handle, func, dev, i);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY,status);
    }

    status = QueryAppStatus(handle, func, dev, NULL);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmUtilGN2412Initialize */
