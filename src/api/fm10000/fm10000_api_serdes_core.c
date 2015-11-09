/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_serdes_core.c
 * Creation Date:   October 22, 2014
 * Description:     File containing a low level functions to manage the
 *                  FM10000 PCIe and Ethernet Serdes
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define STR_EQ(str1, str2) (strcasecmp(str1, str2) == 0)




static fm_int serDesWidthModeArray[FM10000_LANE_BITRATE_MAX] =
{

    FM10000_SERDES_WIDTH_40,


    FM10000_SERDES_WIDTH_20,


    FM10000_SERDES_WIDTH_20,


    FM10000_SERDES_WIDTH_10,


    FM10000_SERDES_WIDTH_10,


    FM10000_SERDES_WIDTH_10,


    FM10000_SERDES_WIDTH_10
};




static fm_int serDesRateSelArray[FM10000_LANE_BITRATE_MAX] =
{
    FM10000_SERDES_DIVIDER_ETHMODE_25G,
    FM10000_SERDES_DIVIDER_ETHMODE_10G,
    FM10000_SERDES_DIVIDER_ETHMODE_6G,
    FM10000_SERDES_DIVIDER_ETHMODE_2500X,
    FM10000_SERDES_DIVIDER_ETHMODE_1000X,
    FM10000_SERDES_DIVIDER_ETHMODE_1000X,
    FM10000_SERDES_DIVIDER_ETHMODE_1000X
};



/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_status ParseHex(fm_char   *string,
                          fm_uint64 *result);
static fm_status DecodeTxPattern(fm_text                    pattern,
                                 fm10000SerdesTxDataSelect *dataSel,
                                 fm_uint32                  pattern10Bit[],
                                 fm_int                    *pattern10BitSize);
static fm_status DecodeRxPattern(fm_text                 pattern,
                                 fm10000SerdesRxCmpData *cmpData);

static fm_status DbgSerdesDumpInt(fm_int  sw,
                                  fm_int  serdes,
                                  fm_bool detailed);
static fm_status DbgSerdesDumpStatusInt(fm_int  sw,
                                        fm_int  serdes);
static fm_status DbgSerdesDumpDfeStatusInt(fm_int   sw,
                                           fm_int   serdes,
                                           fm_bool  detailed);
static fm_status DbgSerdesPrnKrInfo(fm_int   sw,
                                    fm_int   serdes);
static fm_status DbgSerdesDumpSpicoSbmVersionsInt(fm_int sw,
                                                  fm_int serdes);
static fm_status DbgSerdesDumpRegistersInt(fm_int     sw,
                                           fm_int     serDes);
static fm_status DbgSerdesResetStatsInt(fm_int     sw,
                                        fm_int     serDes);
static fm_status DbgSerdesGetInt01Bits(fm_int   sw,
                                       fm_int   serDes,
                                       fm_uint  mask,
                                       fm_uint *val);
static fm_status DbgSerdesSetTxRxEnableInt(fm_int sw,
                                           fm_int serDes,
                                           fm_bool enTx,
                                           fm_bool enRx,
                                           fm_bool enOutput);
static fm_status DbgSerdesInitInt(fm_int  sw,
                                  fm_int  serDes,
                                  fm_uint dataWidth,
                                  fm_uint rateSel);
static fm_status DbgSerdesResetSerDesInt(fm_int sw,
                                         fm_int port);
static fm_status DbgSerdesReadSerDesRegisterInt(fm_int     sw,
                                                fm_int     serDes,
                                                fm_uint    regAddr,
                                                fm_uint32 *pValue);
static fm_status DbgSerdesWriteSerDesRegisterInt(fm_int     sw,
                                                 fm_int     serDes,
                                                 fm_uint    regAddr,
                                                 fm_uint32  value);
static fm_status DbgSerdesSetSerDesTxPatternInt(fm_int  sw,
                                                fm_int  serDes,
                                                fm_text pattern);
static fm_status DbgSerdesSetSerDesRxPatternInt(fm_int  sw,
                                                fm_int  serDes,
                                                fm_text pattern);
static fm_status DbgSerdesSetSerdesPolarityInt(fm_int  sw,
                                               fm_int  serDes,
                                               fm_text polarityStr);
static fm_status DbgSerdesSetSerdesLoopbackInt(fm_int  sw,
                                               fm_int  serDes,
                                               fm_text loopbackStr);
static fm_status DbgSerdesInjectErrorsInt(fm_int              sw,
                                          fm_int              serdes,
                                          fm10000SerdesSelect serdesSel,
                                          fm_uint             numErrors);
static fm_status DbgSerdesReadSBusRegisterInt(fm_int     sw,
                                              fm_int     sbusDevID,
                                              fm_int     devRegID,
                                              fm_uint32 *pValue);
static fm_status DbgSerdesWriteSBusRegisterInt(fm_int     sw,
                                               fm_int     sbusDevID,
                                               fm_int     devRegID,
                                               fm_uint32  value);
static fm_status DbgSerdesInterruptSpicoInt(fm_int      sw,
                                            fm_int      cmd,
                                            fm_int      param,
                                            fm_int      timeout,
                                            fm_uint32  *pResult);
static fm_status DbgSerdesSetDfeParmeter(fm_int      sw,
                                         fm_int      serDes,
                                         fm_uint32   paramSelector,
                                         fm_uint32   paramValue);
static fm_status DbgSerdesGetDfeParmeter(fm_int      sw,
                                         fm_int      serDes,
                                         fm_uint32   paramSelector,
                                         fm_uint32 * pParamValue);
static fm_bool SerdesValidateAttenuationCoefficients(fm_int  att,
                                                     fm_int  pre,
                                                     fm_int  post);
static fm_status SerdesGetEyeSimpleMetric(fm_int  sw,
                                          fm_int  serDes,
                                          fm_int *pEyeScore,
                                          fm_int *pHeightmV);
static fm_status SerDesGetEyeDiagram(fm_int                 sw,
                                     fm_int                 serDes,
                                     fm_eyeDiagramSample   *pSampleTable);


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

extern fm10000_serdesMap fm10000SerdesMap[FM10000_NUM_SERDES];


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_text lbModeStr[] =
{
    "SERDES_LB_OFF",
    "SERDES_LB_INTERNAL_ON",
    "SERDES_LB_INTERNAL_OFF",
    "SERDES_LB_PARALLEL_ON_REFCLK",
    "SERDES_LB_PARALLEL_ON_RX_CLK",
    "SERDES_LB_PARALLEL_OFF",

};


static fm_text polarityStr[] =
{
    "SERDES_POLARITY_NONE",
    "SERDES_POLARITY_INVERT_TX",
    "SERDES_POLARITY_INVERT_RX",
    "SERDES_POLARITY_INVERT_TX_RX",

};


static fm_text txEqSelectStr[] =
{
    "SERDES_EQ_SEL_PRECUR",
    "SERDES_EQ_SEL_ATTEN",
    "SERDES_EQ_SEL_POSTCUR",
    "SERDES_EQ_SEL_ALL",

};



static fm_text rxTermStr[] =
{
    "SERDES_RX_TERM_AGND",
    "SERDES_RX_TERM_AVDD",
    "SERDES_RX_TERM_FLOAT",

};


static fm_text txDataSelectStr[] =
{
    "SERDES_TX_DATA_SEL_CORE",
    "SERDES_TX_DATA_SEL_PRBS7",
    "SERDES_TX_DATA_SEL_PRBS9",
    "SERDES_TX_DATA_SEL_PRBS11",
    "SERDES_TX_DATA_SEL_PRBS15",
    "SERDES_TX_DATA_SEL_PRBS23",
    "SERDES_TX_DATA_SEL_PRBS31",
    "SERDES_TX_DATA_SEL_USER",
    "SERDES_TX_DATA_SEL_LOOPBACK"
};


static fm_text rxCmpDataStr[] =
{
    "SERDES_RX_CMP_DATA_OFF",
    "SERDES_RX_CMP_DATA_PRBS7",
    "SERDES_RX_CMP_DATA_PRBS9",
    "SERDES_RX_CMP_DATA_PRBS11",
    "SERDES_RX_CMP_DATA_PRBS15",
    "SERDES_RX_CMP_DATA_PRBS23",
    "SERDES_RX_CMP_DATA_PRBS31",
    "SERDES_RX_CMP_DATA_SELF_SEED",

};


static fm_text serdesSelectStr[] =
{
    "SERDES_SEL_TX",
    "SERDES_SEL_RX",
    "SERDES_SEL_TX_RX"
};


static fm_text dfeModeStr[] =
{
    "Static",
    "One-Shot",
    "Continuous",
    "KR",
    "iCal-only"
};

static fm_text krPcalModeStr[] =
{
    "Disabled",
    "One-Shot",
    "Continuous"
};

static fm_text dfeTuneModeStr[] =
{
    "SERDES_DFE_TUNE_COARSE",
    "SERDES_DFE_TUNE_FINE",
    "SERDES_DFE_TUNE_START_ADAPTIVE",
    "SERDES_DFE_TUNE_STOP_ADAPTIVE",
    "SERDES_DFE_TUNE_DISABLE"
};

static const fm_uint16 *pCurSerdesImage = NULL;
static fm_int           curRerdesImageSize = 0;
static fm_uint32        curSerdesCodeVersionBuildId = 0;

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** ParseHex
 * \ingroup intSerDes
 *
 * \desc            Parse the string into unsigned 64-bit integer
 *
 * \return          integer value representation of the input string.
 *
 *****************************************************************************/
static fm_status ParseHex(fm_char *string, fm_uint64 *result)
{
    fm_text err = NULL;

    if (string == NULL)
    {
        *result = 0;
        return FM_ERR_INVALID_ARGUMENT;
    }

    *result = strtoul(string, &err, 16);

    return strlen(err) ? FM_FAIL : FM_OK;

}




/*****************************************************************************/
/** DecodeTxPattern
 * \ingroup intSerdes
 *
 * \desc            Convert pattern text string into SERDES configuration.
 *
 * \param[in]       pattern is the pattern text representation.
 *
 * \param[out]      dataSel points to the caller-allocated storage where this
 *                  function will place the corresponding data select mode.
 *
 * \param[out]      pattern10Bit points to the caller-allocated storage where
 *                  this function will place the decoded 10-bit user pattern.
 *                  The size of this array must be 8 to store up to 80 bits.
 *
 * \param[out]      pattern10BitSize points to the caller-allocated storage where
 *                  this function will place the size of the 10-bit pattern.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DecodeTxPattern(fm_text                    pattern,
                                 fm10000SerdesTxDataSelect *dataSel,
                                 fm_uint32                  pattern10Bit[],
                                 fm_int                    *pattern10BitSize)
{
    fm_status status;
    fm_int    strLen;
    fm_char   tempStr[11];
    fm_uint64 val64;

    if (pattern == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    strLen = strlen(pattern);

    if (STR_EQ(pattern, "prbs7"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS7;
    }
    else if (STR_EQ(pattern, "prbs15"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS15;
    }
    else if (STR_EQ(pattern, "prbs23"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS23;
    }
    else if (STR_EQ(pattern, "prbs31"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS31;
    }
    else if (STR_EQ(pattern, "prbs11"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS11;
    }
    else if (STR_EQ(pattern, "prbs9"))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS9;
    }
    else if (STR_EQ(pattern, "core"))
    {

        *dataSel = FM10000_SERDES_TX_DATA_SEL_CORE;
    }
    else if (pattern[0] == '0' && pattern[1] == 'x')
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
        if (strLen == 5)
        {
            status = ParseHex(pattern + 2, &val64);
            pattern10Bit[0] = val64 & 0x3FF;
            *pattern10BitSize = 1;
        }
        else if (strLen == 7)
        {
            status = ParseHex(pattern + 2, &val64);
            pattern10Bit[0] = val64 & 0x3FF;
            pattern10Bit[1] = (val64 >> 10) & 0x3FF;
            *pattern10BitSize = 2;
        }
        else if (strLen == 12)
        {
            status = ParseHex(pattern + 2, &val64);
            pattern10Bit[0] = val64 & 0x3FF;
            pattern10Bit[1] = (val64 >> 10) & 0x3FF;
            pattern10Bit[2] = (val64 >> 20) & 0x3FF;
            pattern10Bit[3] = (val64 >> 30) & 0x3FF;
            *pattern10BitSize = 4;
        }
        else if (strLen == 22)
        {
            FM_MEMCPY_S(tempStr, sizeof(tempStr), pattern + 2, 10);
            tempStr[10] = '\0';
            status = ParseHex(tempStr, &val64);
            pattern10Bit[0] = val64 & 0x3FF;
            pattern10Bit[1] = (val64 >> 10) & 0x3FF;
            pattern10Bit[2] = (val64 >> 20) & 0x3FF;
            pattern10Bit[3] = (val64 >> 30) & 0x3FF;

            if (status == FM_OK)
            {
                status = ParseHex(pattern + 12, &val64);
                pattern10Bit[4] = val64 & 0x3FF;
                pattern10Bit[5] = (val64 >> 10) & 0x3FF;
                pattern10Bit[6] = (val64 >> 20) & 0x3FF;
                pattern10Bit[7] = (val64 >> 30) & 0x3FF;
                *pattern10BitSize = 8;
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "User pattern must be in 10, 20, 40, or 80 bits\n");
            return FM_ERR_INVALID_ARGUMENT;
        }
        if (status)
        {
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Invalid hex string '%s'.\n", pattern);
            return status;
        }
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Unsupported pattern '%s'.\n"
                     "Supported pattern: prbs7, prbs9, prbs11, prbs15, prbs23, prbs31\n"
                     "                   And user pattern of hex string up to 80 bits\n",
                     pattern);
        return FM_ERR_INVALID_ARGUMENT;
    }

    return FM_OK;

}




/*****************************************************************************/
/** DecodeRxPattern
 * \ingroup intSerdes
 *
 * \desc            Convert pattern text string into SERDES configuration.
 *
 * \param[in]       pattern is the pattern text representation.
 *
 * \param[out]      cmpData points to caller-allocated storage where this function
 *                  should place the data.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DecodeRxPattern(fm_text                 pattern,
                                 fm10000SerdesRxCmpData *cmpData)
{
    fm_int    strLen;
    fm_char   tempStr[11];

    if (pattern == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    strLen = strlen(pattern);

    if (STR_EQ(pattern, "prbs7"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS7;
    }
    else if (STR_EQ(pattern, "prbs15"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS15;
    }
    else if (STR_EQ(pattern, "prbs23"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS23;
    }
    else if (STR_EQ(pattern, "prbs31"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS31;
    }
    else if (STR_EQ(pattern, "prbs11"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS11;
    }
    else if (STR_EQ(pattern, "prbs9"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS9;
    }
    else if (STR_EQ(pattern, "off"))
    {

        *cmpData = FM10000_SERDES_RX_CMP_DATA_OFF;
    }
    else if (STR_EQ(pattern, "selfseed"))
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_OFF;
    }
    else if (pattern[0] == '0' && pattern[1] == 'x')
    {
        *cmpData = FM10000_SERDES_RX_CMP_DATA_SELF_SEED;
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Unsupported pattern '%s'.\n"
                     "Supported pattern: prbs7, prbs9, prbs11, prbs15, prbs23, prbs31\n"
                     "                   And off or selfseed\n",
                     pattern);
        return FM_ERR_INVALID_ARGUMENT;
    }

    return FM_OK;

}




/*****************************************************************************/
/** DbgSerdesDumpInt
 * \ingroup intSerdes
 *
 * \desc            Dump SERDES configurations.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       detailed specifies whether to dump for detailed config.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesDumpInt(fm_int  sw,
                                  fm_int  serdes,
                                  fm_bool detailed)
{
    fm_status                   status;
    fm_bool                     signalOk;
    fm_bool                     sbmSpicoIsRunning;
    fm10000SerdesLbMode         lbMode;
    fm10000SerdesPolarity       polarity;
    fm10000SerdesTxDataSelect   dataSel;
    fm10000SerdesRxCmpData      cmpData;
    fm10000SerdesRxTerm         rxTerm;
    fm_uint32                   counter;
    fm_uint32                   uval;
    fm_int                      preEmp;
    fm_int                      atten;
    fm_int                      postEmp;
    fm_uint32                   buildId;
    fm_uint32                   sbmVersionBuildId;
    fm10000_lane                *pLaneExt;
    fm10000_laneDfe             *pLaneDfe;
    fm_int                      eyeScore;
    fm_int                      heightmV;


    FM_LOG_PRINT("########## SERDES: %d ##########\n", serdes);

    sbmSpicoIsRunning = fm10000SbmSpicoIsRunning(sw, FM10000_SERDES_RING_EPL);
    FM_LOG_PRINT("SBM Spico           : %s\n", sbmSpicoIsRunning ? "RUNNING" : "STOPPED");

    status = fm10000SbmSpicoDoCrc(sw, FM10000_SERDES_RING_EPL, FM10000_SBUS_SPICO_BCAST_ADDR);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("SBM CRC             : %s\n", status ? "FAILED" : "PASSED");

    status = fm10000SbmGetBuildRevisionId(sw, FM10000_SERDES_RING_EPL,  &sbmVersionBuildId);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("SBM Build ID        : %4.4x\n", sbmVersionBuildId & 0xffff);
    FM_LOG_PRINT("SBM Version ID      : %4.4x\n", sbmVersionBuildId>>16);

    status = fm10000SerdesSpicoDoCrc(sw, serdes);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("SPICO CRC           : %s\n", status ? "FAILED" : "PASSED");

    status = fm10000SerDesGetBuildRevisionId(sw, serdes, &buildId);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("SPICO Build ID      : %4.4x\n", buildId & 0xffff);
    FM_LOG_PRINT("SPICO Version ID    : %4.4x\n", buildId>>16);

    status = fm10000SerdesGetLoopbackMode(sw, serdes, &lbMode);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("Loopback            : %s\n", lbModeStr[lbMode]);

    fm10000SerdesGetPolarity(sw, serdes, &polarity);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("Polarity            : %s\n", polarityStr[polarity]);

    status = fm10000SerdesGetTxDataSelect(sw, serdes, &dataSel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("TX Data Sel         : %s\n", txDataSelectStr[dataSel]);

    status = fm10000SerdesGetRxCmpData(sw, serdes, &cmpData);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("RX Data Sel         : %s\n", rxCmpDataStr[cmpData]);

    status = fm10000SerdesGetErrors(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, &counter, FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("Error Counter       : %u\n", counter);

    status = fm10000SerdesGetSignalOk(sw, serdes, &signalOk);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("signalOk            : %d\n", signalOk);

    status = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_PRECUR, &preEmp);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("PreCursor           : %d\n", preEmp);
    if (preEmp & 0x8000)
    {
        preEmp |= 0xffff0000;
    }
    status = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_ATTEN, &atten);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("Cursor              : %d\n", atten);
    status = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_POSTCUR, &postEmp);
    if (postEmp & 0x8000)
    {
        postEmp |= 0xffff0000;
    }
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("PostCursor          : %d\n", postEmp);

    status = fm10000SerdesGetRxTerm(sw, serdes, &rxTerm);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    FM_LOG_PRINT("RX Termination      : %s\n", rxTermStr[rxTerm]);


    pLaneExt = GET_LANE_EXT(sw, serdes);
    pLaneDfe = &pLaneExt->dfeExt;
    FM_LOG_PRINT("SW Eye Height        : %d (%d mV)\n", pLaneDfe->eyeScoreHeight/4, pLaneDfe->eyeScoreHeightmV);

    status = fm10000SerdesGetEyeHeight(sw, serdes, &eyeScore, &heightmV);
    FM_LOG_PRINT("Eye Height          : %d (%d mV)\n", eyeScore/4, heightmV);


    FM_LOG_PRINT("Stop Cycles Number  : %d\n", pLaneDfe->stopCycleCnt);
    FM_LOG_PRINT("Stop Coarse Avg Dly : %d\n", pLaneDfe->stopCoarseDelayAvg/1000);
    FM_LOG_PRINT("Stop Coarse Max Dly : %d\n", pLaneDfe->stopCoarseDelayMax);
    FM_LOG_PRINT("Stop Fine   Avg Dly : %d\n", pLaneDfe->stopFineDelayAvg/1000);
    FM_LOG_PRINT("Stop Fine   Max Dly : %d\n", pLaneDfe->stopFineDelayMax);

    status = fm10000SerdesDfeTuningGetStatus (sw, serdes, &uval);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    FM_LOG_PRINT("          %33s : %d\n", "CoarseInProgress",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, CoarseInProgress));
    FM_LOG_PRINT("          %33s : %d\n", "FineInProgress",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, FineInProgress));
    FM_LOG_PRINT("          %33s : %d\n", "AdaptiveEnabled",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, AdaptiveEnabled));


    FM_LOG_PRINT("\n");
    return status;

}




/*****************************************************************************/
/** DbgSerdesDumpStatusInt
 * \ingroup intSerdes
 *
 * \desc            Dump SERDES general status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesDumpStatusInt(fm_int  sw,
                                        fm_int  serdes)
{
    fm_status                   err;
    fm10000_switch             *switchExt;
    fm_bool                     txRdy;
    fm_bool                     rxRdy;
    fm_bool                     signalOk;
    fm10000_lane               *pLaneExt;
    fm10000SerdesLbMode         lbMode;
    fm10000SerdesPolarity       polarity;
    fm10000SerdesTxDataSelect   dataSel;
    fm10000SerdesRxCmpData      cmpData;
    fm10000SerdesRxTerm         rxTerm;
    fm_uint32                   counter;
    fm_int                      preCursor;
    fm_int                      atten;
    fm_int                      postCursor;
    fm_int                      eyeScore;
    fm_int                      heightmV;

    pLaneExt  = GET_LANE_EXT(sw, serdes);

    FM_LOG_PRINT("## SERDES: %-2d ###################\n", serdes);
    FM_LOG_PRINT("##   General status #############\n");

    switchExt = GET_SWITCH_EXT(sw);
    FM_LOG_PRINT("KR support          : %s\n", switchExt->serdesSupportsKR? "YES" : "NO");
    FM_LOG_PRINT("TxReady/RxReady     : ");
    txRdy = FALSE;
    rxRdy = FALSE;
    err = fm10000SerdesGetTxRxReadyStatus(sw,serdes,&txRdy,&rxRdy);
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%c/%c\n",txRdy?'Y':'N', rxRdy?'Y':'N');
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    FM_LOG_PRINT("Cfg Speed           : ");

    if (err == FM_OK && txRdy && rxRdy)
    {
        if (pLaneExt->bitRate >= 0 && pLaneExt->bitRate < FM10000_LANE_BITRATE_MAX)
        {
            FM_LOG_PRINT("(%d)- %s\n", pLaneExt->bitRate,fm10000LaneBitRatesMap[pLaneExt->bitRate]);
        }
        else
        {
            FM_LOG_PRINT("Invalid bitRate: %d\n", pLaneExt->bitRate);
        }
    }
    else
    {
        FM_LOG_PRINT("---\n");
    }


    err = fm10000SerdesGetLoopbackMode(sw, serdes, &lbMode);
    FM_LOG_PRINT("Loopback            : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%s\n", lbModeStr[lbMode]);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetPolarity(sw, serdes, &polarity);
    FM_LOG_PRINT("Polarity            : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%s\n", polarityStr[polarity]);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetTxDataSelect(sw, serdes, &dataSel);
    FM_LOG_PRINT("TX Data Sel         : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%s\n", txDataSelectStr[dataSel]);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetRxCmpData(sw, serdes, &cmpData);
    FM_LOG_PRINT("RX Data Sel         : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%s\n", rxCmpDataStr[cmpData]);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetErrors(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, &counter, FALSE);
    FM_LOG_PRINT("Error Counter       : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%u\n", counter);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetSignalOk(sw, serdes, &signalOk);
    FM_LOG_PRINT("signalOk            : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%d\n", signalOk);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_PRECUR, &preCursor);
    FM_LOG_PRINT("PreCursor           : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%d\n", preCursor);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_ATTEN, &atten);
    FM_LOG_PRINT("Atten               : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%d\n", atten);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetTxEq(sw, serdes, FM10000_SERDES_EQ_SEL_POSTCUR, &postCursor);
    FM_LOG_PRINT("PostCursor          : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%d\n", postCursor);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetRxTerm(sw, serdes, &rxTerm);
    FM_LOG_PRINT("RX Termination      : ");
    if (err == FM_OK)
    {
        FM_LOG_PRINT("%s\n", rxTermStr[rxTerm]);
    }
    else
    {
        FM_LOG_PRINT("ERROR\n");
    }

    err = fm10000SerdesGetEyeHeight(sw, serdes, &eyeScore, &heightmV);
    FM_LOG_PRINT("Initial Eye Height  : %d (%d mV)\n", eyeScore/4, heightmV);

    FM_LOG_PRINT("Forced Reset Count  : %d \n", pLaneExt->fResetCnt);

    FM_LOG_PRINT("\n");
    return err;

}




/*****************************************************************************/
/** DbgSerdesDumpDfeStatusInt
 * \ingroup intSerdes
 *
 * \desc            Dump SERDES DFE status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       detailed specifies whether to dump for detailed config.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesDumpDfeStatusInt(fm_int   sw,
                                           fm_int   serdes,
                                           fm_bool  detailed)
{
    fm_status                   err;
    fm_uint32                   uval;
    fm10000_lane               *pLaneExt;
    fm10000_laneDfe            *pLaneDfe;


    FM_LOG_PRINT("## SERDES: %-2d ###################\n", serdes);
    FM_LOG_PRINT("##   DFE status #################\n");


    pLaneExt = GET_LANE_EXT(sw, serdes);
    pLaneDfe = &pLaneExt->dfeExt;


    if (pLaneDfe->eyeScoreHeight >= 0)
    {
        FM_LOG_PRINT("%30s: %d (%d mV)\n", "Eye Height", pLaneDfe->eyeScoreHeight/4, pLaneDfe->eyeScoreHeightmV);
    }
    else
    {
        FM_LOG_PRINT("%30s: NA\n", "Eye Height");
    }


    if (pLaneExt->dfeMode <= 4)
    {
        FM_LOG_PRINT("          %33s : %s\n", "DFE Mode",dfeModeStr[pLaneExt->dfeMode]);
    }
    else
    {
        FM_LOG_PRINT("          %33s : %s\n", "DFE Mode","Invalid DFE mode");
    }


    FM_LOG_PRINT("          %33s : %d\n", "Number of Start Cycles",pLaneDfe->startCycleCnt);
    FM_LOG_PRINT("          %33s : %d\n", "iCal Avg Delay",pLaneDfe->iCalDelayAvg/1000);
    FM_LOG_PRINT("          %33s : %d\n", "iCal Max Delay",pLaneDfe->iCalDelayMax);
    FM_LOG_PRINT("          %33s : %d\n", "pCal Avg Delay",pLaneDfe->pCalDelayAvg/1000);
    FM_LOG_PRINT("          %33s : %d\n", "pCal Max Delay",pLaneDfe->pCalDelayMax);

    FM_LOG_PRINT("          %33s : %-6d ms\n", "iCal Lst Delay",pLaneDfe->iCalDelayLastMs);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "iCal Avg Delay",pLaneDfe->iCalDelayAvgMs/1000);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "iCal Max Delay",pLaneDfe->iCalDelayMaxMs);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "pCal Lst Delay",pLaneDfe->pCalDelayLastMs);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "pCal Avg Delay",pLaneDfe->pCalDelayAvgMs/1000);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "pCal Max Delay",pLaneDfe->pCalDelayMaxMs);


    FM_LOG_PRINT("          %33s : %d\n", "Number of Stop Cycles",pLaneDfe->stopCycleCnt);
    FM_LOG_PRINT("          %33s : %d\n", "Number of Forced Stop Cycles",pLaneDfe->forcedStopCycleCnt);
    FM_LOG_PRINT("          %33s : %d\n", "Stop Coarse Avg Delay",pLaneDfe->stopCoarseDelayAvg/1000);
    FM_LOG_PRINT("          %33s : %d\n", "Stop Coarse Max Delay",pLaneDfe->stopCoarseDelayMax);
    FM_LOG_PRINT("          %33s : %d\n", "Stop Fine Avg Delay",pLaneDfe->stopFineDelayAvg/1000);
    FM_LOG_PRINT("          %33s : %d\n", "Stop Fine Max Delay",pLaneDfe->stopFineDelayMax);

    FM_LOG_PRINT("          %33s : %-6d ms\n", "Stop Tuning Lst Delay",pLaneDfe->stopTuningDelayLstMs);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "Stop Tuning Avg Delay",pLaneDfe->stopTuningDelayAvgMs/1000);
    FM_LOG_PRINT("          %33s : %-6d ms\n", "Stop Tuning Max Delay",pLaneDfe->stopTuningDelayMaxMs);

    err = fm10000SerdesDfeTuningGetStatus (sw, serdes, &uval);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    FM_LOG_PRINT("          %33s : %d\n", "CoarseInProgress",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, CoarseInProgress));
    FM_LOG_PRINT("          %33s : %d\n", "FineInProgress",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, FineInProgress));
    FM_LOG_PRINT("          %33s : %d\n", "AdaptiveEnabled",
                 FM_GET_BIT(uval, FM10000_SERDES_DFE_PARAM_DFE_STATUS, AdaptiveEnabled));

    FM_LOG_PRINT("\n");
    return err;

}




/*****************************************************************************/
/** DbgSerdesPrnKrInfo
 * \ingroup intSerdes
 *
 * \desc            Dump basic KR status info.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesPrnKrInfo(fm_int   sw,
                                    fm_int   serdes)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm10000_lane   *pLaneExt;
    fm10000_laneKr *pLaneKr;


    switchExt = GET_SWITCH_EXT(sw);

    err = FM_OK;

    if ( !switchExt->serdesSupportsKR )
    {
        FM_LOG_PRINT("KR training not supported by this build version\n");
    }
    else
    {
        pLaneExt = GET_LANE_EXT(sw, serdes);
        pLaneKr  = &pLaneExt->krExt;


        FM_LOG_PRINT("\nSerdes #%d, KR Training Basic Debug Info\n", serdes);
        FM_LOG_PRINT("-------------------------------------------------------------------\n");

        if (pLaneKr->pCalMode <= 2)
        {
            FM_LOG_PRINT("  %41s : %s\n", "KR pCal Mode",krPcalModeStr[pLaneKr->pCalMode]);
        }

        FM_LOG_PRINT("  %41s : %d\n", "Number of KR training cycles",pLaneKr->startKrCycleCnt);
        FM_LOG_PRINT("  %41s : %d\n", "KR training failures",pLaneKr->krErrorCnt);
        FM_LOG_PRINT("  %41s : %d\n", "KR training timeouts",pLaneKr->krTimeoutCnt);
        FM_LOG_PRINT("  %41s : %-6d ms\n", "KR training Lst Delay",pLaneKr->krTrainingDelayLastMs);
        FM_LOG_PRINT("  %41s : %-6d ms\n", "KR training Avg Delay",pLaneKr->krTrainingDelayAvgMs/1000);
        FM_LOG_PRINT("  %41s : %-6d ms\n\n", "KR training Max Delay",pLaneKr->krTrainingDelayMaxMs);
    }

    return err;

}




/*****************************************************************************/
/** DbgSerdesDumpSpicoSbmVersionsInt
 * \ingroup intSerdes
 *
 * \desc            Dump Spico and Sbm image versions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesDumpSpicoSbmVersionsInt(fm_int sw,
                                                  fm_int serdes)
{
    fm_status                   err;
    fm10000_switch             *switchExt;
    fm_bool                     sbmSpicoIsRunning;
    fm_uint32                   buildId;
    fm_uint32                   sbmVersionBuildId;


    FM_LOG_PRINT("## SERDES: %-2d ###################\n", serdes);
    FM_LOG_PRINT("##   Image versions & CRCs ######\n");

    switchExt = GET_SWITCH_EXT(sw);

    sbmSpicoIsRunning = fm10000SbmSpicoIsRunning(sw, FM10000_SERDES_RING_EPL);
    FM_LOG_PRINT("SBM Spico           : %s\n", sbmSpicoIsRunning ? "RUNNING" : "STOPPED");

    err = fm10000SbmSpicoDoCrc(sw, FM10000_SERDES_RING_EPL, FM10000_SBUS_SPICO_BCAST_ADDR);
    FM_LOG_PRINT("SBM CRC             : %s\n", err ? "FAILED" : "PASSED");

    err = fm10000SbmGetBuildRevisionId(sw, FM10000_SERDES_RING_EPL,  &sbmVersionBuildId);
    if (err == FM_OK)
    {
        FM_LOG_PRINT("SBM Build ID        : %4.4x\n", sbmVersionBuildId & 0xffff);
        FM_LOG_PRINT("SBM Version ID      : %4.4x\n", sbmVersionBuildId>>16);
    }
    else
    {
        FM_LOG_PRINT("SBM Build ID        : ERROR\n");
        FM_LOG_PRINT("SBM Version ID      : ERROR\n");
    }

    err = fm10000SerdesSpicoDoCrc(sw, serdes);
    FM_LOG_PRINT("SPICO CRC           : %s\n", err ? "FAILED" : "PASSED");

    err = fm10000SerDesGetBuildRevisionId(sw, serdes, &buildId);
    if (err == FM_OK)
    {
        FM_LOG_PRINT("SPICO Build ID      : %4.4x\n", buildId & 0xffff);
        FM_LOG_PRINT("SPICO Version ID    : %4.4x\n", buildId>>16);
        FM_LOG_PRINT("KR support          : %s\n", switchExt->serdesSupportsKR? "YES" : "NO");
    }
    else
    {
        FM_LOG_PRINT("SPICO Build ID      : ERROR\n");
        FM_LOG_PRINT("SPICO Version ID    : ERROR\n");
    }


    FM_LOG_PRINT("\n");
    return err;

}




/*****************************************************************************/
/** DbgSerdesDumpRegistersInt
 * \ingroup intSerdes
 *
 * \desc            Dump SERDES configurations.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesDumpRegistersInt(fm_int     sw,
                                           fm_int     serDes)
{
    fm_status              err;
    fm_uint32              val;
    fm10000_switch        *switchExt;

    switchExt = GET_SWITCH_EXT(sw);

    err = FM_OK;

    if (switchExt->serdesBypassSbus == TRUE)
    {
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                        "Serdes=0x%2.2x: cannot dump SerDes Registers, SBus not available\n",
                        serDes);
    }
    else
    {
        FM_LOG_PRINT("##### Register Dump SerDes: %d #####\n", serDes);

        fm10000SbusSetDebug (1);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_00, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_01, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_02, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_03, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_04, &val);
        err = fm10000SerdesRead(sw, serDes, 0x05, &val);
        err = fm10000SerdesRead(sw, serDes, 0x06, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_07, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_08, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_09, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_0A, &val);

        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_0B, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_0C, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_FD, &val);
        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_FF, &val);

        fm10000SbusSetDebug (0);
        FM_LOG_PRINT("\n");
    }

    return err;

}




/*****************************************************************************/
/** DbgSerdesResetStatsInt
 * \ingroup intSerdes
 *
 * \desc            Reset serdes statistical counters and accumulators.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesResetStatsInt(fm_int     sw,
                                        fm_int     serDes)
{
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;
    fm10000_laneKr     *pLaneKr;


    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;
    pLaneKr  = &pLaneExt->krExt;

    pLaneExt->fResetCnt            = 0;
    pLaneDfe->startCycleCnt        = 0;
    pLaneDfe->iCalDelayAvg         = 0;
    pLaneDfe->iCalDelayMax         = 0;
    pLaneDfe->pCalDelayAvg         = 0;
    pLaneDfe->pCalDelayMax         = 0;
    pLaneDfe->iCalDelayLastMs      = 0;
    pLaneDfe->iCalDelayAvgMs       = 0;
    pLaneDfe->iCalDelayMaxMs       = 0;
    pLaneDfe->pCalDelayLastMs      = 0;
    pLaneDfe->pCalDelayAvgMs       = 0;
    pLaneDfe->pCalDelayMaxMs       = 0;
    pLaneDfe->stopCycleCnt         = 0;
    pLaneDfe->forcedStopCycleCnt   = 0;
    pLaneDfe->stopCoarseDelayAvg   = 0;
    pLaneDfe->stopCoarseDelayMax   = 0;
    pLaneDfe->stopFineDelayAvg     = 0;
    pLaneDfe->stopFineDelayMax     = 0;
    pLaneDfe->stopTuningDelayLstMs = 0;
    pLaneDfe->stopTuningDelayAvgMs = 0;
    pLaneDfe->stopTuningDelayMaxMs = 0;

    pLaneKr->startKrCycleCnt       = 0;
    pLaneKr->krErrorCnt            = 0;
    pLaneKr->krTimeoutCnt          = 0;
    pLaneKr->krTrainingDelayLastMs = 0;
    pLaneKr->krTrainingDelayAvgMs  = 0;
    pLaneKr->krTrainingDelayMaxMs  = 0;


    return FM_OK;

}




/*****************************************************************************/
/** DbgSerdesGetInt01Bits
 * \ingroup intSerdes
 *
 * \desc            Get the SERDES interrupt 01 bits which are set in mask.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       mask is the mask value.
 *
 * \param[in]       val is the caller-allocated storage where the function
 *                  will return the value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesGetInt01Bits(fm_int   sw,
                                       fm_int   serDes,
                                       fm_uint  mask,
                                       fm_uint *val)
{
    fm_status   status;
    fm_uint     bits = 0;
    fm_uint32   val32;

    status = FM_OK;

    if ( mask & 0x03 )
    {


        status = fm10000SerdesSpicoInt(sw, serDes, (1 << 14) | 0x026, 0x00, &val32);
        bits  |= (0x03 & val32);
    }
    if ( mask & 0x04 )
    {
        status = fm10000SerdesDmaRead(sw, serDes, FM10000_SERDES_DMA_TYPE_ESB, FM10000_AVSD_ESB_ADDR_0X213, &val32);

        if (fm10000SerdesSpicoIsRunning(sw, serDes))
        {

            status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x4000 | (FM10000_AVSD_ESB_ADDR_0X213 & 0x3fff), &val32);
            if (status == FM_OK)
            {
                status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X1A, 0x00, &val32);
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Serdes=%d, Spico is not running\n",
                            serDes);
        }

        bits |= (0x02 & val32) << 1;
    }

    *val = bits;

    return status;

}




/*****************************************************************************/
/** DbgSerdesSetTxRxEnableInt
 * \ingroup intSerdes
 *
 * \desc            Dbg function that controls Tx/Rx/output serdes enable
 *                  commands.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       enTx specifies whether to enable Tx
 *
 * \param[in]       enRx specifies whether to enable Rx
 *
 * \param[in]       enOutput specifies whether to enable Tx output
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetTxRxEnableInt(fm_int  sw,
                                           fm_int  serDes,
                                           fm_bool enTx,
                                           fm_bool enRx,
                                           fm_bool enOutput)
{
    fm_status     status;
    fm_uint       mask;
    fm_uint       val;
    fm_timestamp  start;
    fm_timestamp  end;
    fm_timestamp  diff;
    fm_uint       delTime;
    fm_uint       loopCnt;

    status = DbgSerdesGetInt01Bits(sw, serDes, ~0x07, &mask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    if (enTx)     mask |= 0x01;
    if (enRx)     mask |= 0x02;
    if (enOutput) mask |= 0x04;

    status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X01, mask, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


    mask &= 0x03;
    delTime = 0;
    loopCnt = 0;
    fmGetTime(&start);
    while (delTime < FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        loopCnt++;
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        status = DbgSerdesGetInt01Bits(sw, serDes, mask, &val);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

        if ((val & mask) == mask)
        {
            return status;
        }
        fmDelay(0, 500*loopCnt );
    }
    return FM_FAIL;

}




/*****************************************************************************/
/** DbgSerdesInitInt
 * \ingroup intSerdes
 *
 * \desc            Initialize specified SERDES, only for debug or test mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       dataWidth is the with mode of either 10, 20, or 40.
 *
 * \param[in]       rateSel is the SERDES TX rate and ratio.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesInitInt(fm_int  sw,
                                  fm_int  serDes,
                                  fm_uint dataWidth,
                                  fm_uint rateSel)
{
    fm_status           status;
    fm_uint32           retVal;
    fm_uint32           val;
    fm_int              serdesDbgLvl;
    fm_bool             isEplRing;
    fm_uint             sbusAddr;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);


    if (dataWidth != 10 &&
        dataWidth != 20 &&
        dataWidth != 40)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Data width can only be 10, 20, or 40.\n");
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
    }

    if (rateSel != 8 && rateSel != 20 && rateSel != 66 && rateSel != 165 &&
        rateSel != 25 && rateSel != 50 && rateSel != 80 && rateSel != 0)
    {
        FM_LOG_PRINT("WARNING: unexpected ratSel=%u\n"
                     "Use:\n"
                     "   0 ->  powerdown\n"
                     "   8 -> 1G\n"
                     "  20 -> 2.5G\n"
                     "  66 -> 10G\n"
                     " 165 -> 25G\n"
                     "  25 -> 2.5G Gen1\n"
                     "  50 -> 5.0G Gen2\n"
                     "  80 -> 8.0G Gen3\n",
                     rateSel);
    }

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

    if (serdesDbgLvl > 0)
    {
        fm10000SbusSetDebug(1);
    }

    isEplRing = (fm10000SerdesMap[serDes].ring == FM10000_SERDES_RING_EPL);

    if (!isEplRing)
    {

        sbusAddr = fm10000SerdesMap[serDes].sbusAddr + 1;
        status = fm10000SbusWrite(sw, isEplRing, sbusAddr, 0x0, 0x8);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    }


    status = fm10000SerdesResetSpico(sw,serDes);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


    status = DbgSerdesSetTxRxEnableInt(sw, serDes, FALSE, FALSE, FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    if (rateSel)
    {

        val = 0x0;
        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X11, BIT_0, 1);
        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X11, BIT_1, 1);
        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X11, val, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        val = 0x0;
        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X0B, BIT_0, 0);
        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0B, val, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        val = 0;
        FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X05, FIELD_1, (rateSel & 0xFF));
        FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X05, FIELD_2, ((rateSel >> 8) & 0x7));



        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X05, BIT_15, 1);

        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X05, val, &retVal);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        val = 0;
        switch (dataWidth)
        {
            case 10:

                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_1, 0);
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_2, 0);
                break;
            case 20:

                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_1, 2);
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_2, 2);
            break;
            case 40:

                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_1, 3);
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X14, FIELD_2, 3);
            break;
        }
        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X14, val, &retVal);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        status = DbgSerdesSetTxRxEnableInt(sw, serDes, TRUE, TRUE, TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        status = fm10000SerdesInitSignalOk(sw, serDes, 0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        status = fm10000SerdesSetRxTerm(sw, serDes,
                   (isEplRing ?
                   FM10000_SERDES_RX_TERM_AVDD:FM10000_SERDES_RX_TERM_FLOAT));
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        status = fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_OFF);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


        status = fm10000SerdesSetTxDataSelect(sw,
                                              serDes,
                                              FM10000_SERDES_TX_DATA_SEL_CORE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
        status = fm10000SerdesSetRxCmpData(sw,
                                           serDes,
                                           FM10000_SERDES_RX_CMP_DATA_OFF);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);



        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X17, 0, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    }

    if (serdesDbgLvl > 0)
    {
        fm10000SbusSetDebug(0);
    }

    return FM_OK;

}




/*****************************************************************************/
/** DbgSerdesResetSerDesInt
 * \ingroup intDiag
 *
 * \desc            Reset SERDES to default state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port associated with the serdes.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesResetSerDesInt(fm_int sw,
                                         fm_int port)
{
    fm_status status;
    fm_int    serdes;

    status = fm10000MapLogicalPortToSerdes(sw, port, &serdes);
    if (status == FM_OK)
    {

        printf("Need support: port %d serdes %d\n", port, serdes);
    }

    return status;

}




/*****************************************************************************/
/** DbgSerdesReadSerDesRegisterInt
 * \ingroup intDiag
 *
 * \desc            Read the content of the specified SERDES register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       regAddr is the register address within the device.
 *
 * \param[out]      pValue is a pointer to a caller-allocated area where this
 *                  function will return the content of the register
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesReadSerDesRegisterInt(fm_int     sw,
                                                fm_int     serDes,
                                                fm_uint    regAddr,
                                                fm_uint32 *pValue)
{
    fm_status              err;
    fm10000_switch        *switchExt;
    fm_int                 serdesDbgLvl;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, serDes=%d, regAddr=0x%4.4x, pValue=%p\n",
                         sw,
                         serDes,
                         regAddr,
                         (void *) pValue);

    switchExt = GET_SWITCH_EXT(sw);

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

    err = FM_OK;

    if (switchExt->serdesBypassSbus == TRUE)
    {
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                        "Serdes=0x%2.2x: cannot read/dump SerDes register, SBus not available\n",
                        serDes);
        if (serdesDbgLvl > 0)
        {
            FM_LOG_PRINT("Serdes=0x%2.2x: cannot read/dump SerDes register, SBus not available\n",
                            serDes);
        }
    }
    else
    {
        err = fm10000SerdesRead(sw, serDes, regAddr, pValue);

    }


    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** DbgSerdesWriteSerDesRegisterInt
 * \ingroup intDiag
 *
 * \desc            Write to SERDES registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       regAddr is the register address within the device.
 *
 * \param[in]       value is the value to be written to the register.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesWriteSerDesRegisterInt(fm_int     sw,
                                                 fm_int     serDes,
                                                 fm_uint    regAddr,
                                                 fm_uint32  value)
{
    fm_status              err;
    fm10000_switch        *switchExt;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, serDes=%d, regAddr=%0x4.4x, value=0x%4.4x\n",
                         sw,
                         serDes,
                         regAddr,
                         value);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus == TRUE)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "SBus is not available, serdes==%d, register=0x%2.2x, value=0x%4.4xx\n",
                     serDes,
                     regAddr,
                     value);
    }
    else
    {
        err = fm10000SerdesWrite(sw, serDes, regAddr, value);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** DbgSerdesSetSerDesTxPatternInt
 * \ingroup intSerDes
 *
 * \desc            Configure SerDes Tx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       pattern is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetSerDesTxPatternInt(fm_int  sw,
                                                fm_int  serDes,
                                                fm_text pattern)
{
    fm_status               status;
    fm10000SerdesTxDataSelect dataSel;
    fm_uint32               pattern10Bit[8];
    fm_int                  pattern10BitSize;

    pattern10BitSize = 0;

    status = DecodeTxPattern(pattern, &dataSel, pattern10Bit, &pattern10BitSize);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    if (dataSel == FM10000_SERDES_TX_DATA_SEL_USER)
    {
        status = fm10000SerdesSetUserDataPattern(sw,
                                                 serDes,
                                                 FM10000_SERDES_SEL_TX,
                                                 pattern10Bit,
                                                 pattern10BitSize);
    }
    else
    {
        status = fm10000SerdesSetTxDataSelect(sw, serDes, dataSel);
    }

    return status;

}




/*****************************************************************************/
/** DbgSerdesSetSerDesRxPatternInt
 * \ingroup intSerDes
 *
 * \desc            Configure SerDes Rx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       pattern is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetSerDesRxPatternInt(fm_int  sw,
                                                fm_int  serDes,
                                                fm_text pattern)
{
    fm_status               status;
    fm10000SerdesRxCmpData  cmpData;

    status = DecodeRxPattern(pattern, &cmpData);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);


    status = fm10000SerdesSetBasicCmpMode(sw,serDes);

    if (status == FM_OK)
    {
        status = fm10000SerdesSetRxCmpData(sw, serDes, cmpData);
    }

    if (status == FM_OK)
    {
        status = fm10000ResetSerdesErrorCounter(sw, serDes);
    }

    return status;

}




/*****************************************************************************/
/** DbgSerdesSetSerdesPolarityInt
 * \ingroup intSerdes
 *
 * \desc            Set polarity for a given SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       polarityStr is a string to specify tx, rx or txrx..
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetSerdesPolarityInt(fm_int  sw,
                                               fm_int  serDes,
                                               fm_text polarityStr)
{

    if (polarityStr == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (STR_EQ(polarityStr, "none") || STR_EQ(polarityStr, "off"))
    {
        return fm10000SerdesSetPolarity(sw, serDes, FM10000_SERDES_POLARITY_NONE);
    }
    else if (STR_EQ(polarityStr, "invertTx"))
    {
        return fm10000SerdesSetPolarity(sw, serDes, FM10000_SERDES_POLARITY_INVERT_TX);
    }
    else if (STR_EQ(polarityStr, "invertRx"))
    {
        return fm10000SerdesSetPolarity(sw, serDes, FM10000_SERDES_POLARITY_INVERT_RX);
    }
    else if (STR_EQ(polarityStr, "invertTxRx"))
    {
        return fm10000SerdesSetPolarity(sw, serDes, FM10000_SERDES_POLARITY_INVERT_TX_RX);
    }
    else
    {
        FM_LOG_PRINT("Valid serdes polarity commands: none, invertTx, invertRx, or invertTxRx\n");
    }


    return FM_ERR_INVALID_ARGUMENT;

}




/*****************************************************************************/
/** DbgSerdesSetSerdesLoopbackInt
 * \ingroup intSerdes
 *
 * \desc            Set loopback mode for a given SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       loopbackStr is a string to specify off, tx2rx.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetSerdesLoopbackInt(fm_int  sw,
                                               fm_int  serDes,
                                               fm_text loopbackStr)
{
    if (loopbackStr == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (STR_EQ(loopbackStr, "off"))
    {
        return fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_OFF);
    }
    else if (STR_EQ(loopbackStr, "on"))
    {
        return fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_INTERNAL_ON);
    }
    else if (STR_EQ(loopbackStr, "rx2tx"))
    {
        return fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_PARALLEL_ON_RX_CLK);
    }
    else
    {
        FM_LOG_PRINT("Valid serdes loopback commands: off, on, or rx2tx\n");
    }

    return FM_ERR_INVALID_ARGUMENT;

}




/*****************************************************************************/
/** DbgSerdesInjectErrorsInt
 * \ingroup intSerdes
 *
 * \desc            Inject errors into SERDES TX or RX stream.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       serdesSel is SERDES selection mode. See fm10000SerdesSelect.
 *
 * \param[in]       numErrors is the number of errors to inject.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesInjectErrorsInt(fm_int              sw,
                                          fm_int              serdes,
                                          fm10000SerdesSelect serdesSel,
                                          fm_uint             numErrors)
{

    return fm10000SerdesInjectErrors(sw, serdes, serdesSel, numErrors);


}




/*****************************************************************************/
/** DbgSerdesReadSBusRegisterInt
 * \ingroup intDiag
 *
 * \desc            Read the content of SBus registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sbusDevID is the device ID on the SBus. The first 8-bit
 *                  specifies the SBUS address, and bit 9 specifies
 *                  either EPL or PCIE ring.
 *
 * \param[in]       devRegID is the register ID within the device.
 *
 * \param[out]      pValue is a pointer to a caller-allocated area where this
 *                  function will return the content of the register
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesReadSBusRegisterInt(fm_int     sw,
                                              fm_int     sbusDevID,
                                              fm_int     devRegID,
                                              fm_uint32 *pValue)
{
    fm_status            err;
    fm10000_switch      *switchExt;
    fm_uint              sbusAddr;
    fm_uint              regAddr;
    fm_serdesRing        ring;
    fm_bool              isEplRing;


    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus == TRUE)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "SBus is not available, sbusDvId=%d, register=0x%2.2x, value=0x%p\n",
                     sbusDevID,
                     devRegID,
                     (void*) pValue);
    }
    else
    {
        sbusAddr = sbusDevID & 0xFF;
        ring     = (sbusDevID >> 8) & 0x1 ?
                    FM10000_SERDES_RING_PCIE : FM10000_SERDES_RING_EPL;
        isEplRing = (ring == FM10000_SERDES_RING_EPL);
        regAddr  = devRegID;
        err = fm10000SbusRead(sw, isEplRing, sbusAddr, regAddr, pValue);
    }

    return err;

}




/*****************************************************************************/
/** DbgSerdesWriteSBusRegisterInt
 * \ingroup intDiag
 *
 * \desc            Read the content of SBus registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sbusDevID is the device ID on the SBus. The first 8-bit
 *                  specifies the SBUS address, and bit 9 specifies
 *                  either EPL or PCIE ring.
 *
 * \param[in]       devRegID is the register ID within the device.
 *
 * \param[in]       value is the value to be written to the register.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesWriteSBusRegisterInt(fm_int     sw,
                                               fm_int     sbusDevID,
                                               fm_int     devRegID,
                                               fm_uint32  value)
{
    fm_status            err;
    fm10000_switch      *switchExt;
    fm_uint              sbusAddr;
    fm_uint              regAddr;
    fm_serdesRing        ring;
    fm_bool              isEplRing;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, sbusDevID=0x%2.2x, devRegID=%0x2.2x, value=0x%4.4x\n",
                         sw,
                         sbusDevID,
                         devRegID,
                         value);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus == TRUE)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "SBus is not available, sbusDvId=%d, register=0x%2.2x, value=0x%4.4xx\n",
                     sbusDevID,
                     devRegID,
                     value);
    }
    else
    {
        sbusAddr = sbusDevID & 0xFF;
        ring     = (sbusDevID >> 8) & 0x1 ?
                    FM10000_SERDES_RING_PCIE : FM10000_SERDES_RING_EPL;
        isEplRing = (ring == FM10000_SERDES_RING_EPL);
        regAddr  = devRegID;

        err = fm10000SbusWrite(sw, isEplRing, sbusAddr, regAddr, value);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}



/*****************************************************************************/
/** DbgSerdesInterruptSpicoInt
 * \ingroup intSBus
 *
 * \desc            Send the specified SPICO interrupt command to the
 *                  SBM or SERDES SPICO controller, waits for the command
 *                  to complete, and returns the command's data result.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cmd is the SPICO interrupt command. The first 16-bit
 *                  is for the interrupt number. Bit 16 to 23 is for
 *                  serdes number when in SERDES mode.
 *                  Bit 24 specifies PCIE or EPL.
 *                  Bit 25 specifies SBM or SERDES SPICO controller.
 *
 * \param[in]       param is the 16-bit SPICO command argument.
 *
 * \param[in]       timeout specifies the maximum length of time to wait
 *                  for the interrupt command to complete. (nanoseconds)
 *
 * \param[out]      pResult points to a caller-supplied location to receive
 *                  the 16-bit data result.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesInterruptSpicoInt(fm_int      sw,
                                            fm_int      cmd,
                                            fm_int      param,
                                            fm_int      timeout,
                                            fm_uint32  *pResult)
{
    fm_status       err;
    fm_int          serdes;
    fm_uint         intNum;
    fm_serdesRing   ring;
    fm_int          sbusAddr;

    FM_NOT_USED(timeout);

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, cmd=0x%4.4x, param=0x%4.4x, timeout=%d, pResult=%p\n",
                         sw,
                         cmd,
                         param,
                         timeout,
                         (void*) pResult);

    intNum = cmd & 0xFFFF;

    if (cmd & (1 << 25))
    {
        FM_LOG_PRINT("SBM interrupt, ");
        sbusAddr = (cmd >> 16)& 0xFF;
        ring     = (cmd >> 24) & 0x1 ?
                    FM10000_SERDES_RING_PCIE : FM10000_SERDES_RING_EPL;

        if (ring == FM10000_SERDES_RING_EPL)
        {
            FM_LOG_PRINT("EPL Ring\n");
        }
        else
        {
            FM_LOG_PRINT("PCIe Ring\n");
        }

        err = fm10000SbmSpicoInt(sw, ring, sbusAddr, intNum, param, pResult);
    }
    else
    {

        serdes = (cmd >> 16) & 0xFF;
        FM_LOG_PRINT("Serdes %d interrupt: 0x%2.2x, input: 0x%4.4x\n",serdes, intNum, param);
        err = fm10000SerdesSpicoInt(sw, serdes, intNum, param, pResult);
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** DbgSerdesSetDfeParmeter
 * \ingroup intSBus
 *
 * \desc            Set, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector to paramValue.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       paramSelector is the selector of the parameter to be set
 *
 * \param[in]       paramValue is the value to be set on the selected parameter
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesSetDfeParmeter(fm_int      sw,
                                         fm_int      serDes,
                                         fm_uint32   paramSelector,
                                         fm_uint32   paramValue)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(serDes);
    FM_NOT_USED(paramSelector);
    FM_NOT_USED(paramValue);

    FM_LOG_PRINT("\n *** Feature not Available ***\n\n");

    return FM_OK;

}




/*****************************************************************************/
/** DbgSerdesGetDfeParmeter
 * \ingroup intSBus
 *
 * \desc            Get, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       paramSelector is the selector of the parameter to be set
 *
 * \param[in]       pParamValue is a pointer to a caller-allocated area where
 *                  this function will return the value of the selected
 *                  parameter.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status DbgSerdesGetDfeParmeter(fm_int      sw,
                                         fm_int      serDes,
                                         fm_uint32   paramSelector,
                                         fm_uint32 * pParamValue)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(serDes);
    FM_NOT_USED(paramSelector);
    FM_NOT_USED(pParamValue);

    FM_LOG_PRINT("\n *** Feature not Available ***\n\n");

    return FM_OK;

}




/*****************************************************************************/
/** SerdesValidateAttenuationCoefficients
 * \ingroup intSerdes
 *
 * \desc            Validate that the range of each coefficient and the addition
 *                  of theirs absolute values are inside the allowed ranges.
 *                  If one of these conditions is not satisfied, the subjacent
 *                  hardware returns an error.
 *                  Note well: this function do not check for the limit compliant
 *                  values.
 *
 * \param[in]       att is the Tx EQ attenuation (cursor)
 *
 * \param[in]       pre is the Tx EQ pre-cursor
 *
 * \param[in]       post is the Tx EQ post-cursor
 *
 * \return          TRUE if the range of each coefficient and its comabination
 *                  is inside the allowed ranges, and FALSE otherwise.
 *
 *****************************************************************************/
static fm_bool SerdesValidateAttenuationCoefficients(fm_int  att,
                                                     fm_int  pre,
                                                     fm_int  post)
{
    fm_bool         valid;


    valid = FALSE;


    if ( (att  < 0)  || (att  > 31) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Att (%d) is out of range (0..31)\n",
                     att);
    }
    else if ( (pre  <-7)  || (pre  > 15) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes PreCursor (%d) is out of range (-7..15)\n",
                     pre);
    }
    else if ( (post <-31) || (post > 31) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes PostCursor (%d) is out of range (-31..31)\n",
                     post);

    }
    else if ( (abs(att)+abs(pre)+abs(post)) > 32)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes (abs(Att)+abs(PreCursor)+abs(PostCursor)) > 32\n");

    }
    else
    {
        valid = TRUE;
    }

    return valid;

}




/*****************************************************************************/
/** SerdesGetEyeSimpleMetric
 * \ingroup intSerdes
 *
 * \desc            Return a simple eye metric in the ranges [0..64] (pEyeScore)
 *                  and [0..1000] (pHeightmV).
 *                  Value is derived from SerDes tuning, and returns 0 if tuning
 *                  was not completed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pEyeScore points to the caller-allocated storage where this
 *                  function will return the eye metric [0..64]. It may be NULL.
 *
 * \param[out]      pHeightmV points to the caller-allocated storage where this
 *                  function will return the eye metric [0..1000]. It may be
 *                  NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SerdesGetEyeSimpleMetric(fm_int  sw,
                                          fm_int  serDes,
                                          fm_int *pEyeScore,
                                          fm_int *pHeightmV)
{
    fm_status       err;
    fm_uint         index;
    fm_uint32       results;
    fm_uint32       value1;
    fm_uint32       value2;
    fm_uint32       vDiff;
    fm10000_lane   *pLaneExt;

    err = FM_OK;
    results = 1000;
    value1 = 0;
    value2 = 0;

    for( index = 0; index < 8 && err == FM_OK; index += 2 )
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26_READ,
                                    (4 << 12) | ((index+0) << 8),
                                    &value1);

        if (err == FM_OK)
        {
            err = fm10000SerdesSpicoInt(sw,
                                        serDes,
                                        FM10000_SPICO_SERDES_INTR_0X26_READ,
                                        (4 << 12) | ((index+1) << 8),
                                        &value2);
        }


        value1 = (value1 & 0x8000) ? value1 | 0xffff0000 : value1;
        value2 = (value2 & 0x8000) ? value2 | 0xffff0000 : value2;
        vDiff  = abs(value2 - value1);

        results = (vDiff < results)? vDiff : results;
    }

    if (err == FM_OK)
    {
        pLaneExt = GET_LANE_EXT(sw, serDes);

        pLaneExt->dfeExt.eyeScoreHeight = results;
        pLaneExt->dfeExt.eyeScoreHeightmV = (results * 1000) / 256;

        if (pEyeScore)
        {
            *pEyeScore = pLaneExt->dfeExt.eyeScoreHeight;
        }

        if (pHeightmV)
        {
            *pHeightmV = pLaneExt->dfeExt.eyeScoreHeightmV;
        }
    }
    else
    {

        if (pEyeScore)
        {
            *pEyeScore = -1;
        }

        if (pHeightmV)
        {
            *pHeightmV = 0;
        }
    }

    return err;

}



/*****************************************************************************/
/** SerDesGetEyeScore
 * \ingroup intSerdes
 *
 * \desc            Dummy  "GetEyeScore" function. This feature is
 *                  not available.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pEyeHeight is a pointer to a caller-allocated area where
 *                  this function will return eye height.
 *
 * \param[out]      pEyeWidth is a pointer to a caller-allocated area where
 *                  this function will return eye width.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SerDesGetEyeScore(fm_int sw,
                                   fm_int serDes,
                                   fm_int *pEyeHeight,
                                   fm_int *pEyeWidth)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(serDes);

    *pEyeHeight = -1;
    *pEyeWidth  = -1;

    return FM_OK;
}



/*****************************************************************************/
/** SerDesGetEyeDiagram
 * \ingroup intSerdes
 *
 * \desc            Dummy  "GetEyeDiagram" function. This feature is
 *                  not available.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pSampleTable is a pointer to a caller-allocated area where
 *                  this function will return FM10000_PORT_EYE_SAMPLES
 *                  eye sample points.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SerDesGetEyeDiagram(fm_int                 sw,
                                     fm_int                 serDes,
                                     fm_eyeDiagramSample   *pSampleTable)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(serDes);
    FM_NOT_USED(pSampleTable);

    FM_LOG_PRINT("\n *** Feature not Available ***\n\n");

    return FM_OK;
}


/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/**  fm10000SerdesInitXServicesInt
 * \ingroup intSerdes
 *
 * \desc            Performs the initialization of extended services and
 *                  debug function accesses.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitXServicesInt(fm_int sw)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;
    err = FM_OK;


    serdesPtr->SerdesGetEyeHeight       = SerdesGetEyeSimpleMetric;
    serdesPtr->SerdesGetEyeScore        = SerDesGetEyeScore;
    serdesPtr->SerdesGetEyeDiagram      = SerDesGetEyeDiagram;


    serdesPtr->dbgDump                  = DbgSerdesDumpInt;
    serdesPtr->dbgDumpStatus            = DbgSerdesDumpStatusInt;
    serdesPtr->dbgDumpDfeStatus         = DbgSerdesDumpDfeStatusInt;
    serdesPtr->dbgDumpKrStatus          = DbgSerdesPrnKrInfo;
    serdesPtr->dbgDumpSpicoSbmVersions  = DbgSerdesDumpSpicoSbmVersionsInt;
    serdesPtr->dbgDumpRegisters         = DbgSerdesDumpRegistersInt;
    serdesPtr->dbgResetStats            = DbgSerdesResetStatsInt;
    serdesPtr->dbgInit                  = DbgSerdesInitInt;
    serdesPtr->dbgResetSerdes           = DbgSerdesResetSerDesInt;
    serdesPtr->dbgReadRegister          = DbgSerdesReadSerDesRegisterInt;
    serdesPtr->dbgWriteRegister         = DbgSerdesWriteSerDesRegisterInt;
    serdesPtr->dbgSetTxPattern          = DbgSerdesSetSerDesTxPatternInt;
    serdesPtr->dbgSetRxPattern          = DbgSerdesSetSerDesRxPatternInt;
    serdesPtr->dbgSetPolarity           = DbgSerdesSetSerdesPolarityInt;
    serdesPtr->dbgSetLoopback           = DbgSerdesSetSerdesLoopbackInt;
    serdesPtr->dbgInjectErrors          = DbgSerdesInjectErrorsInt;
    serdesPtr->dbgReadSbusRegister      = DbgSerdesReadSBusRegisterInt;
    serdesPtr->dbgWriteSbusRegister     = DbgSerdesWriteSBusRegisterInt;
    serdesPtr->dbgInterruptSpico        = DbgSerdesInterruptSpicoInt;
    serdesPtr->dbgSetDfeParameter       = DbgSerdesSetDfeParmeter;
    serdesPtr->dbgGetDfeParameter       = DbgSerdesGetDfeParmeter;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/** fm10000SerdesCheckId
 * \ingroup intSerdes
 *
 * \desc            Validate the ID of the serDes and set pValidId to TRUE
 *                  is the ID is the expected on.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serDesId to be validated.
 *
 * \param[in]       pValidId points to the caller-allocated storage where
 *                  this function will place the requested value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pRomImg is NULL when numWords
 *                  is non zero.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesCheckId(fm_int   sw,
                               fm_int   serDes,
                               fm_bool *pValidId)
{
    fm_status       err;
    fm_uint32       val;
    fm10000_switch *switchExt;

    err = FM_OK;

    if (pValidId == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        switchExt = GET_SWITCH_EXT(sw);

        if (switchExt->serdesBypassSbus == FALSE)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_FF, &val);
            *pValidId = (err == FM_OK && (val == 0x1));
        }
    }
    return err;

}



/*****************************************************************************/
/** fm10000SerdesEnableSerDesInterrupts
 * \ingroup intSerdes
 *
 * \desc            Enable interrupts at serdes macro level.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesEnableSerDesInterrupts(fm_int   sw,
                                              fm_int   serDes)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_uint32       val;
    fm_uint32       sbusAddr;
    fm_serdesRing   ring;

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    if (switchExt->serdesBypassSbus == FALSE)
    {

        err = fm10000MapSerdesToSbus(sw, serDes, &sbusAddr, &ring);

        if (err == FM_OK)
        {
            val = 0;



            err = fm10000SbusWrite(sw, (ring == FM10000_SERDES_RING_EPL), sbusAddr, FM10000_SERDES_REG_08, val);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDisableSerDesInterrupts
 * \ingroup intSerdes
 *
 * \desc            Disable interrupts at serdes macro level.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDisableSerDesInterrupts(fm_int   sw,
                                               fm_int   serDes)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_uint32       val;
    fm_uint32       sbusAddr;
    fm_serdesRing   ring;


    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    if (switchExt->serdesBypassSbus == FALSE)
    {

        err = fm10000MapSerdesToSbus(sw, serDes, &sbusAddr, &ring);

        if (err == FM_OK)
        {


            val = 0;

            FM_SET_BIT(val, FM10000_SERDES_REG_08, BIT_4, 1);
            FM_SET_BIT(val, FM10000_SERDES_REG_08, BIT_5, 1);

            err = fm10000SbusWrite(sw, (ring == FM10000_SERDES_RING_EPL), sbusAddr, FM10000_SERDES_REG_08, val);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesTxRxEnaCtrl
 * \ingroup intSerdes
 *
 * \desc            Enable/Disable serdes Tx/Rx sections and Tx output
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \param[in]       enaCtrl value and mask to be used to control Tx/Rx/output.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesTxRxEnaCtrl(fm_int    sw,
                                   fm_int    serDes,
                                   fm_uint32 enaCtrl)
{
    fm_status     err;
    fm10000_lane *pLaneExt;
    fm_uint32     mask;
    fm_uint32     serDesEnableStatus;


    pLaneExt  = GET_LANE_EXT(sw, serDes);
    serDesEnableStatus = pLaneExt->serDesEnableCache;

    mask = (enaCtrl >> 3) & 0x07;

    serDesEnableStatus = ((serDesEnableStatus & ~mask) | (enaCtrl & mask)) & 0x07;


    err = fm10000SerdesSpicoWrOnlyInt(sw,
                                      serDes,
                                      FM10000_SPICO_SERDES_INTR_0X01,
                                      serDesEnableStatus);
    if (err == FM_OK)
    {
        pLaneExt->serDesEnableCache = serDesEnableStatus;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDisable
 * \ingroup intSerdes
 *
 * \desc            Disable the serdes (Tx/Rx sections and Tx output).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDisable(fm_int    sw,
                               fm_int    serDes)
{
    fm_status     err;
    fm10000_lane *pLaneExt;
    fm_uint32     result;


    pLaneExt  = GET_LANE_EXT(sw, serDes);



    fmDelay(0, FM10000_SERDES_RESET_DELAY);

    err = fm10000SerdesSpicoIntSBusWrite(sw, serDes, FM10000_SPICO_SERDES_INTR_0X01, 0);
    if (err == FM_OK)
    {

        err = fm10000SerdesSpicoIntSBusReadFast(sw, serDes, &result);
    }

    if (err != FM_OK || result != FM10000_SPICO_SERDES_INTR_0X01)
    {

        err = fm10000SerdesResetSpico(sw, serDes);


        if (pLaneExt->fResetCnt < 0xffffffff)
        {
            pLaneExt->fResetCnt++;
        }
    }


    if (err == FM_OK)
    {
        pLaneExt->serDesEnableCache = 0;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetBitRate
 * \ingroup intSerdes
 *
 * \desc            Set Serdes bit rate dividers according to rateSel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \param[in]       rateSel rate selector, see ''serDesRateSelArray''
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetBitRate(fm_int  sw,
                                  fm_int  serDes,
                                  fm_uint rateSel)
{
    fm_status     err;
    fm_uint32     intData;


    intData = 0;
    FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X05, FIELD_1, (rateSel & 0xFF));
    FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X05, FIELD_2, ((rateSel >> 8) & 0x7));


    FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X05, BIT_12, 1);
    FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X05, BIT_15, 1);

    err = fm10000SerdesSpicoWrOnlyInt(sw,
                                      serDes,
                                      FM10000_SPICO_SERDES_INTR_0X05,
                                      intData);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetWidthMode
 * \ingroup intSerdes
 *
 * \desc            Set Serdes Width mode according to widthMode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \param[in]       widthMode width mode, see ''fm_serdesWidthMode''
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetWidthMode(fm_int             sw,
                                    fm_int             serDes,
                                    fm_serdesWidthMode widthMode)
{
    fm_status     err;
    fm_uint32     intData;

    intData = 0;
    FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X14, FIELD_1, FM10000_SERDES_WIDTH_20);
    FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X14, FIELD_2, FM10000_SERDES_WIDTH_20);

    fmDelay(0, FM10000_SERDES_RESET_DELAY);
    err = fm10000SerdesSpicoWrOnlyInt(sw,
                                      serDes,
                                      FM10000_SPICO_SERDES_INTR_0X14,
                                      intData);
    if (err == FM_OK)
    {
        fmDelay(0, FM10000_SERDES_RESET_DELAY);
        intData = 0;
        FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X14, FIELD_1, widthMode);
        FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X14, FIELD_2, widthMode);

        err = fm10000SerdesSpicoWrOnlyInt(sw,
                                          serDes,
                                          FM10000_SPICO_SERDES_INTR_0X14,
                                          intData);
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,serDes,
                        "Serdes=0x%2.2x: Error setting width mode\n",
                         serDes);
    }

    fmDelay(0, FM10000_SERDES_RESET_DELAY);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetPllCalibrationMode
 * \ingroup intSerdes
 *
 * \desc            Set PLL calibration mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the Id of the target serdes.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetPllCalibrationMode(fm_int    sw,
                                             fm_int    serDes)
{
    fm_status     err;
    fm10000_lane *pLaneExt;
    fm_uint32     pllCalMode;
    fm_uint32     value;


    pLaneExt  = GET_LANE_EXT(sw, serDes);
    value = 0;
    pllCalMode = pLaneExt->pllCalibrationMode;

    switch (pllCalMode & 0x03)
    {
        case FM10000_SERDES_PLL_CALIBRATION_DISABLED:

            break;
        case FM10000_SERDES_PLL_CALIBRATION_ENABLED:
            FM_SET_BIT(value, FM10000_SPICO_SERDES_INTR_0X11, BIT_0, 1);
            FM_SET_BIT(value, FM10000_SPICO_SERDES_INTR_0X11, BIT_1, 1);
            break;
        case FM10000_SERDES_PLL_CALIBRATION_ADAPTIVE:
            if (++pLaneExt->pllCalibrationCycleCnt > FM10000_SERDES_PLL_CAL_CNT_THRESHOLD)
            {
                FM_SET_BIT(value, FM10000_SPICO_SERDES_INTR_0X11, BIT_0, 1);
                FM_SET_BIT(value, FM10000_SPICO_SERDES_INTR_0X11, BIT_1, 1);

                pLaneExt->pllCalibrationCycleCnt = (((fm_uint32)rand()) % FM10000_SERDES_PLL_CAL_CNT_THRESHOLD) / 10;
            }
            break;
        default:
            break;

    }

    err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X11, value);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSpicoIntSBusWrite
 * \ingroup intSerdes
 *
 * \desc            Perform SERDES SPICO interrupt write.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoIntSBusWrite(fm_int     sw,
                                         fm_int     serdes,
                                         fm_uint    intNum,
                                         fm_uint32  param)
{
    fm_status    err;
    fm_uint32    val;
    fm_uint32    intr;
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;
    fm_uint      loopCnt;

    err = FM_OK;

    delTime = 0;
    loopCnt = 0;
    fmGetTime(&start);
    while (delTime < FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        loopCnt++;
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        err = fm10000SerdesRead(sw, serdes, FM10000_SERDES_REG_04, &val);

        if (err == FM_OK)
        {
            if (FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_16) ||
                FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_17))
            {
                if (delTime > 5)
                {
                    fmDelay(0, 5000);
                }
                continue;
            }
        }
        break;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt ERROR, serdes 0x%02x. Reg[0x4] = 0x%8.8x\n",
                      serdes, val);
    }
    else if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = fm10000SerdesRead(sw, serdes, FM10000_SERDES_REG_03, &intr);
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt 0x%x param=0x%4.4x timeout %u msec %u iterations, "
                     "serdes 0x%02x. Reg[0x4] = 0x%8.8x\n",
                     (intr >> 16), (intr & 0xFFFF), delTime, loopCnt, serdes, val);
        err = FM_FAIL;
    }
    else
    {
        err = fm10000SerdesWrite(sw, serdes, FM10000_SERDES_REG_03, (intNum << 16) | param);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSpicoIntSBusRead
 * \ingroup intSerdes
 *
 * \desc            Read SERDES SPICO interrupt result.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      pValue points to the caller-allocated storage where this
 *                  function will place the result. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoIntSBusRead(fm_int      sw,
                                        fm_int      serdes,
                                        fm_uint32  *pValue)
{
    fm_status    err;
    fm_uint32    val;
    fm_uint32    intr;
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;
    fm_uint      loopCnt;

    err = FM_OK;

    if (pValue != NULL)
    {
        *pValue = 0;
    }

    delTime = 0;
    loopCnt = 0;
    fmGetTime(&start);
    while (delTime < FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        loopCnt++;
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        err = fm10000SerdesRead(sw, serdes, FM10000_SERDES_REG_04, &val);

        if (err == FM_OK)
        {
            if (FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_16) ||
                FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_17))
            {



                if (delTime > 5)
                {
                    fmDelay(0, 5000);
                }
                continue;
            }
        }
        break;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt ERROR, serdes 0x%02x. Reg[0x4] = 0x%8.8x\n",
                      serdes, val);
    }
    else if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = fm10000SerdesRead(sw, serdes, FM10000_SERDES_REG_03, &intr);
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt 0x%x param=0x%4.4x timeout %u msec %u iterations, "
                     "serdes 0x%02x. Reg[0x4] = 0x%8.8x\n",
                     (intr >> 16), (intr & 0xFFFF), delTime, loopCnt, serdes, val);
        err = FM_FAIL;
    }
    else if (pValue != NULL)
    {

        *pValue = val;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSpicoIntSBusReadFast
 * \ingroup intSerdes
 *
 * \desc            Read SERDES SPICO interrupt result using a short timeout.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      pValue points to the caller-allocated storage where this
 *                  function will place the result. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoIntSBusReadFast(fm_int      sw,
                                            fm_int      serdes,
                                            fm_uint32  *pValue)
{
    fm_status    err;
    fm_uint32    val;
    fm_uint      delTime;
    fm_uint      loopCnt;

    err = FM_OK;

    if (pValue != NULL)
    {
        *pValue = 0;
    }

    delTime = 0;
    loopCnt = 0;

    while (loopCnt < FM10000_SERDES_INT_FAST_TIMEOUT_CYCLES)
    {
        loopCnt++;

        err = fm10000SerdesRead(sw, serdes, FM10000_SERDES_REG_04, &val);

        if (err == FM_OK)
        {
            if (FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_16) ||
                FM_GET_BIT(val, FM10000_SERDES_REG_04, BIT_17))
            {
                fmDelay(0, 100000);
                continue;
            }
        }
        break;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt ERROR, serdes 0x%02x. Reg[0x4] = 0x%8.8x\n",
                      serdes, val);
    }
    else if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = FM_FAIL;
    }
    else if (pValue != NULL)
    {
        *pValue = val;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SbmSpicoIntWrite
 * \ingroup intSerdes
 *
 * \desc            Perform SBUS master SPICO interrupt write.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmSpicoIntWrite(fm_int        sw,
                                  fm_serdesRing ring,
                                  fm_uint       sbusAddr,
                                  fm_uint       intNum,
                                  fm_uint32     param)
{
    fm_status err;
    fm_uint32 reg07;
    fm_bool   isEplRing;


    isEplRing = (ring == FM10000_SERDES_RING_EPL);

    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_02, (param << 16) | intNum);

    if (err == FM_OK)
    {

        err = fm10000SbusRead(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_07, &reg07);

        if (err == FM_OK)
        {
            FM_SET_BIT(reg07, FM10000_SPICO_REG_07, BIT_0, 1);
            err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_07, reg07);

            if (err == FM_OK)
            {

                FM_SET_BIT(reg07, FM10000_SPICO_REG_07, BIT_0, 0);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_07, reg07);
            }
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SbmSpicoIntRead
 * \ingroup intSerdes
 *
 * \desc            Read SBUS master SPICO interrupt result.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       pValue points to the caller-allocated storage where this
 *                  function will place the result.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmSpicoIntRead(fm_int         sw,
                                 fm_serdesRing  ring,
                                 fm_uint        sbusAddr,
                                 fm_uint32     *pValue)
{
    fm_status    err;
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;
    fm_uint      loopCnt;

    err = FM_OK;
    delTime = 0;
    loopCnt = 0;

    if (pValue == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
    }

    fmGetTime(&start);
    while (delTime < FM10000_SBM_INTERRUPT_TIMEOUT_MSEC)
    {
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        err = fm10000SbusRead(sw, (ring == FM10000_SERDES_RING_EPL), sbusAddr, 0x08, pValue);

        if (err != FM_OK)
        {

            break;
        }
        else
        {

            if ((*pValue & 0x8000) || (*pValue & 0x3ff) == 0)
            {


                if (delTime > 5)
                {
                    fmDelay(0, 5000);
                }
            }
            else
            {

                break;
            }
        }
    }


    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "SMB Interrupt ERROR, SBUS 0x%02x, reg[0x8] = 0x%x\n",
                     sbusAddr,
                     pValue != NULL? *pValue : 0);
    }
    else if (delTime >= FM10000_SBM_INTERRUPT_TIMEOUT_MSEC)
    {
        err = FM_FAIL;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "SBM Interrupt timeout %u msec %u iterations, SBUS 0x%02x, reg[0x8] = 0x%x\n",
                     delTime,
                     loopCnt,
                     sbusAddr,
                     *pValue);
    }
    else
    {
        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SERDES,
                             "SBM Interrupt time: %u msec, SBUS 0x%2.2x, reg[0x8] = 0x%8.8x\n",
                             delTime,
                             sbusAddr,
                             *pValue);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSpicoUploadImage
 * \ingroup intSerdes
 *
 * \desc            Upload the SerDes SPICO image to specified SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies the EPL or the PCIe ring
 *
 * \param[in]       serdesAddr is the target SerDes address. If this address
 *                  is the serdes broadcast address ''FM10000_SERDES_EPL_BCAST'',
 *                  then the image will be uploaded on all SerDes SPICOS
 *                  simultaneaoulsy.
 *
 * \param[in]       pRomImg is the buffer containing the SerDes SPICO image.
 *
 * \param[in]       numWords is the size of the image. It may be set to 0,
 *                  in which case the upload stage will be skipped.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pRomImg is NULL when numWords
 *                  is non zero.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoUploadImage(fm_int           sw,
                                        fm_serdesRing    ring,
                                        fm_int           serdesAddr,
                                        const fm_uint16 *pRomImg,
                                        fm_int           numWords)
{
    fm_status       err;
    fm_int          addr;
    fm_uint32       val;
    fm_bool         eplRing;
    fm_timestamp    tStart;
    fm_timestamp    tEnd;
    fm_timestamp    tDelta;
    fm_uint32       reg07;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, serdesAddr=%d, pRomImg=%p, numWords=%d\n",
                  sw,
                  ring,
                  serdesAddr,
                  (void *) pRomImg,
                  numWords);

    err = FM_OK;
    eplRing = (ring == FM10000_SERDES_RING_EPL);

    if (numWords > 0)
    {
        if (pRomImg == NULL)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {

            reg07 = 0;
            FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_0, 1);
            FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_4, 1);
            err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_07, reg07);


            if (err == FM_OK)
            {
                FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_0, 0);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_07, reg07);
            }


            if (err == FM_OK)
            {
                val = 0;
                FM_SET_BIT(val, FM10000_SERDES_REG_00, BIT_30, 1);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_00, val);
            }


            if (err == FM_OK)
            {
                val = 0;
                FM_SET_BIT(val, FM10000_SERDES_REG_08, BIT_4, 1);
                FM_SET_BIT(val, FM10000_SERDES_REG_08, BIT_5, 1);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_08, val);
            }

            fmGetTime(&tStart);


            for (addr = 0; addr < numWords && err == FM_OK; addr += 3)
            {
                val = 0xc0000000 | pRomImg[addr];
                if ((addr+1) < numWords)
                {
                    val |= (pRomImg[addr+1] << 10);

                    if ((addr+2) < numWords)
                    {
                        val |= (pRomImg[addr+2] << 20);
                    }
                }
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_0A, val);
            }

            if (err == FM_OK)
            {
                if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
                {
                    fmGetTime(&tEnd);
                    fmSubTimestamps(&tEnd, &tStart, &tDelta);
                    FM_LOG_PRINT("SerDes upload time: %d,%d sec\n",
                                 (fm_uint)tDelta.sec, (fm_uint)tDelta.usec/1000);
                }
            }


            if (err == FM_OK)
            {
                val = 0;
                FM_SET_BIT(val, FM10000_SERDES_REG_00, BIT_30, 0);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_00, val);
            }


            if (err == FM_OK)
            {
                val = 0x20000000;
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_01, val);
            }


            if (err == FM_OK)
            {
                val = 0;
                FM_SET_BIT(val, FM10000_SERDES_REG_0B, BIT_18, 1);
                FM_SET_BIT(val, FM10000_SERDES_REG_0B, BIT_19, 1);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_0B, val);
            }


            if (err == FM_OK)
            {
                FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_0, 1);
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_07, reg07);


                if (err == FM_OK)
                {
                    FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_0, 0);
                    err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_07, reg07);
                }

                if (err == FM_OK)
                {
                    FM_SET_BIT(reg07, FM10000_SERDES_REG_07, BIT_1, 1);
                    err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_07, reg07);
                }
            }


            if (err == FM_OK)
            {

                val = 0;
                err = fm10000SbusWrite(sw, eplRing, serdesAddr, FM10000_SERDES_REG_08, val);
            }
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesSwapUploadImage
 * \ingroup intSerdes
 *
 * \desc            Upload the SPICO swap image. Note that only some serdes
 *                  firmwares require a swap image. A non null swapNumWords
 *                  value indicates that there is a swap code to upload. This
 *                  function uploads that code on top of the SBus master code
 *                  and it is destinated to be used with production version.
 *                  For development versions see
 *                  ''fm10000SerdesSwapAltUploadImage'' instead.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       pSwapRomImg points to the buffer containing the Swap
 *                  image.
 *
 * \param[in]       swapNumWords is the size of the Swap image. It may be
 *                  equal to 0, in which case the upload stage will be
 *                  skipped.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSwapUploadImage(fm_int           sw,
                                       fm_serdesRing    ring,
                                       fm_uint          sbusAddr,
                                       const fm_uint16 *pSwapRomImg,
                                       fm_int           swapNumWords)
{
    fm_status       err;
    fm_int          index;
    fm_bool         isEplRing;
    fm_uint32       addr;
    fm_uint32       reg01;
    fm_uint32       reg05;
    fm_timestamp    tStart;
    fm_timestamp    tEnd;
    fm_timestamp    tDelta;



    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, sbusAddr=%2.2x, pSwapRomImg=%p, swapNumWords=%d\n",
                  sw,
                  ring,
                  sbusAddr,
                  (void *) pSwapRomImg,
                  swapNumWords);

    err = FM_OK;
    reg05 = 0;
    reg01 = 0;
    isEplRing = (ring == FM10000_SERDES_RING_EPL);

    if (swapNumWords > 0)
    {
        if (pSwapRomImg == NULL ||
            sbusAddr != FM10000_SBUS_SPICO_BCAST_ADDR)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {

            if (err == FM_OK)
            {
                err = fm10000SbmSpicoInt(sw, FM10000_SERDES_RING_EPL, FM10000_SBUS_SPICO_BCAST_ADDR, 0x1C, 0, &addr);


                addr >>= 16;
            }

            if (err == FM_OK)
            {
                if (addr != 0xFFFF)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"Swap Image initial load Address: 0x%4.4x, (%d)\n",
                                 addr,
                                 addr);



                    FM_SET_BIT(reg05, FM10000_SPICO_REG_05, BIT_0, 1);
                    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_05, reg05);
                }
                else
                {

                    err = FM_FAIL;
                    FM_LOG_ERROR(FM_LOG_CAT_SERDES,"Invalid build: cannot upload Swap Image\n");
                }
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_SERDES,"Cannot get Swap Image initial load Address\n");
            }


            if (err == FM_OK)
            {
                err = fm10000SbusRead(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, &reg01);
                if (err == FM_OK)
                {
                    FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_9, 1);
                    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, reg01);
                }
            }

            if (err == FM_OK)
            {
                fmGetTime(&tStart);


                fm10000SbusWrite(sw, isEplRing, sbusAddr, 0x03, 0x00000000 | addr);
                fm10000SbusWrite(sw, isEplRing, sbusAddr, 0x03, 0x80000000 | addr);


                for ( index = 0; index < swapNumWords-2 && err == FM_OK; index+=3)
                {
                    fm10000SbusWrite(sw,
                                     isEplRing,
                                     sbusAddr,
                                     0x14,
                                     0xc0000000 | pSwapRomImg[index] | (pSwapRomImg[index+1] << 10 ) | (pSwapRomImg[index+2] << 20 ) );
                }

                if( swapNumWords - index == 2 )
                {
                    fm10000SbusWrite(sw,
                                     isEplRing,
                                     sbusAddr,
                                     0x14,
                                     0x80000000 | pSwapRomImg[index] | (pSwapRomImg[index+1] << 10 ) );
                }
                else if( swapNumWords - index == 1 )
                {
                    fm10000SbusWrite(sw,
                                     isEplRing,
                                     sbusAddr,
                                     0x14,
                                     0x40000000 | pSwapRomImg[index] );
                }

                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_03, 0x00000000);

                FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"Last address of Swap Image=0x%4.4x, (%d)\n",
                             (addr+index),
                             (addr+index));
            }


            if (err == FM_OK)
            {
                if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
                {
                    fmGetTime(&tEnd);
                    fmSubTimestamps(&tEnd, &tStart, &tDelta);
                    FM_LOG_PRINT("Swap upload time:   %d,%d sec\n",
                                 (fm_uint)tDelta.sec, (fm_uint)tDelta.usec/1000);
                }

                FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_9, 0);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, reg01);

                if (err == FM_OK)
                {
                    FM_SET_BIT(reg05, FM10000_SPICO_REG_05, BIT_0, 0);
                    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_05, reg05);
                }
            }
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesSwapAltUploadImage
 * \ingroup intSerdes
 *
 * \desc            Upload the SPICO swap image. Note that only some serdes
 *                  firmwares require a swap image. A non null swapNumWords
 *                  value indicates that there is a swap code to upload. This
 *                  is an alternate function that uploads swap code to XDMEM,
 *                  detinated to be used with development firmware versions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       pSwapRomImg points to the buffer containing the Swap
 *                  image.
 *
 * \param[in]       swapNumWords is the size of the Swap image. It may be
 *                  equal to 0, in which case the upload stage will be
 *                  skipped.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSwapAltUploadImage(fm_int         sw,
                                          fm_serdesRing    ring,
                                          fm_uint          sbusAddr,
                                          const fm_uint16 *pSwapRomImg,
                                          fm_int           swapNumWords)
{
    fm_status       err;
    fm_int          index;
    fm_uint32       data;
    fm_bool         isEplRing;
    fm_uint32       addr;
    fm_uint32       reg01;
    fm_uint32       reg05;
    fm_timestamp    tStart;
    fm_timestamp    tEnd;
    fm_timestamp    tDelta;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, sbusAddr=%2.2x, pSwapRomImg=%p, swapNumWords=%d\n",
                  sw,
                  ring,
                  sbusAddr,
                  (void *) pSwapRomImg,
                  swapNumWords);

    err = FM_OK;
    isEplRing = (ring == FM10000_SERDES_RING_EPL);

    if (swapNumWords > 0)
    {
        if (pSwapRomImg == NULL || sbusAddr != FM10000_SBUS_SPICO_BCAST_ADDR)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            if (err == FM_OK)
            {

                reg05 = 0;
                FM_SET_BIT(reg05, FM10000_SPICO_REG_05, BIT_0, 1);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_05, reg05);
            }


            if (err == FM_OK)
            {
                err = fm10000SbusRead(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, &reg01);
                if (err == FM_OK)
                {
                    FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_10, 1);
                    FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_11, 1);
                    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, reg01);
                }
            }


            addr = 0x0400;
            fmGetTime(&tStart);


            for ( index = 0; index < swapNumWords && err == FM_OK; index++)
            {
                data = 0x8000 | (pSwapRomImg[index] << 16) | (addr+index);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_04, data);
            }


            if (err == FM_OK)
            {
                if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
                {
                    fmGetTime(&tEnd);
                    fmSubTimestamps(&tEnd, &tStart, &tDelta);
                    FM_LOG_PRINT("Swap upload time:   %d,%d sec\n", (fm_uint)tDelta.sec, (fm_uint)tDelta.usec/1000);
                }

                FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_10, 0);
                FM_SET_BIT(reg01, FM10000_SPICO_REG_01, BIT_11, 0);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, reg01);

                if (err == FM_OK)
                {
                    FM_SET_BIT(reg05, FM10000_SPICO_REG_05, BIT_0, 0);
                    err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_05, reg05);
                }
            }
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SbmSpicoUploadImage
 * \ingroup intSerdes
 *
 * \desc            Upload the SBUS master SPICO image.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is SBUS address.
 *
 * \param[in]       pSbmRomImg points to the buffer containing the SBus master
 *                  SPICO image.
 *
 * \param[in]       sbmNumWords is the size of the SBM image. It may be set
 *                  to 0, in which case the whole upload stage, including the
 *                  swap upload, will be skipped.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmSpicoUploadImage(fm_int           sw,
                                     fm_serdesRing    ring,
                                     fm_uint          sbusAddr,
                                     const fm_uint16 *pSbmRomImg,
                                     fm_int           sbmNumWords)
{
    fm_status       err;
    fm_int          addr;
    fm_uint32       val;
    fm_uint32       data;
    fm_bool         isEplRing;
    fm_timestamp    tStart;
    fm_timestamp    tEnd;
    fm_timestamp    tDelta;



    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, sbusAddr=%d, pSbmRomImg=%p, sbmNumWords=%d\n",
                  sw,
                  ring,
                  sbusAddr,
                  (void *) pSbmRomImg,
                  sbmNumWords);

    err = FM_OK;
    isEplRing = (ring == FM10000_SERDES_RING_EPL);

    if (sbmNumWords > 0)
    {
        if (pSbmRomImg == NULL ||
            sbusAddr != FM10000_SBUS_SPICO_BCAST_ADDR)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {

            val = 0;
            FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_6, 1);
            FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_7, 1);
            err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, val);

            if (err == FM_OK)
            {

                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_7, 0);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, val);
            }

            if (err == FM_OK)
            {

                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_9, 1);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, val);
            }


            fmGetTime(&tStart);

            for (addr = 0 ; addr < sbmNumWords && err == FM_OK; addr++)
            {
                data = 0x80000000 | (pSbmRomImg[addr] << 16) | addr;
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, 0x03, data);
            }

            if (err == FM_OK)
            {
                if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
                {
                    fmGetTime(&tEnd);
                    fmSubTimestamps(&tEnd, &tStart, &tDelta);
                    FM_LOG_PRINT("SBM upload time:    %d,%d sec\n", (fm_uint)tDelta.sec, (fm_uint)tDelta.usec/1000);
                }


                val = 0;
                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_6, 1);
                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_9, 0);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, val);
            }

            if (err == FM_OK)
            {

                val = 0;
                FM_SET_BIT(val, FM10000_SPICO_REG_16, BIT_18, 1);
                FM_SET_BIT(val, FM10000_SPICO_REG_16, BIT_19, 1);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_16, val);
            }

            if (err == FM_OK)
            {

                val = 0;
                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_6, 1);
                FM_SET_BIT(val, FM10000_SPICO_REG_01, BIT_8, 1);
                err = fm10000SbusWrite(sw, isEplRing, sbusAddr, FM10000_SPICO_REG_01, val);
            }

        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesSpicoDoCrc
 * \ingroup intSerdes
 *
 * \desc            Perform SERDES SPICO CRC. This function split the
 *                  command into two sequences, START and CHECK. The
 *                  caller can perform the START sequence on all the SERDES
 *                  and then come back and perform the CHECK sequence.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful and CRC passes.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoDoCrc(fm_int                       sw,
                                  fm_int                       serDes)
{
    fm_status   err;
    fm_uint32   crc;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, serdes=%d\n",
                 sw,
                 serDes);

    err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X3C, 0, &crc);

    if (err == FM_OK)
    {
        if (crc == 00)
        {
            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes,
                            "SerDes CRC PASSED on serdes 0x%02x.\n",
                            serDes);
        }
        else
        {
            err = FM_FAIL;
            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes,
                            "SerDes CRC FAILED on serdes 0x%02x. CRC interrupt returned 0x%4.4x\n",
                            serDes,
                            crc);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SbmSpicoDoCrc
 * \ingroup intSerdes
 *
 * \desc            Perform SERDES SPICO CRC. This function split the
 *                  command into two sequences, START and CHECK. The
 *                  caller can perform the START sequence on all the SERDES
 *                  and then come back and perform the CHECK sequence.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring is the EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \return          FM_OK if successful and CRC passes.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmSpicoDoCrc(fm_int                       sw,
                               fm_serdesRing                ring,
                               fm_uint                      sbusAddr)
{
    fm_status   err;
    fm_uint32   crc;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, sbusAddr=0x%2.2x\n",
                 sw,
                 ring,
                 sbusAddr);

    err = fm10000SbmSpicoInt(sw, ring, sbusAddr, FM10000_SPICO_REG_02, 0, &crc);

    if (err == FM_OK)
    {
        crc = crc >> 16;




        if (crc == 0xffff)
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_SERDES, "SBM CRC FAILED on %s ring. CRC interrupt returned 0x%02x\n",
                         (ring? "EPL":"PCIe"),
                         crc);
        }
        else if (crc == 0x0001)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "SBM CRC PASSED on %s ring\n",
                         (ring? "EPL":"PCIe"));
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "Unexpected value performing SBM CRC on %s ring: %4.4x\n",
                         (ring? "EPL":"PCIe"), crc);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SwapImageDoCrc
 * \ingroup intSerdes
 *
 * \desc            Compute and check the CRC OF the loaded swap image.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring is the EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       swapCrcCode is the interrupt to be used to run CRC on
 *                  swap image
 *
 * \return          FM_OK if successful and CRC passes.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SwapImageDoCrc(fm_int                       sw,
                                fm_serdesRing                ring,
                                fm_uint                      sbusAddr,
                                fm_int                       swapCrcCode)
{
    fm_status   err;
    fm_uint32   crc;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%d, sbusAddr=0x%2.2x, swapCrcCode=0x%2.2x\n",
                 sw,
                 ring,
                 sbusAddr,
                 swapCrcCode);

    err = fm10000SbmSpicoIntWrite(sw, ring, sbusAddr, swapCrcCode, 0);

    if (err == FM_OK)
    {
        err = fm10000SbmSpicoIntRead(sw, ring, sbusAddr, &crc);

        if (err == FM_OK)
        {
            crc = crc >> 16;




            if (crc == 0xffff)
            {
                err = FM_FAIL;
                FM_LOG_ERROR(FM_LOG_CAT_SERDES, "IMEM_Swap CRC FAILED on %s ring. CRC interrupt (0x%2.2x) returned 0x%4.4x\n",
                             (ring? "EPL":"PCIe"),
                             swapCrcCode,
                             crc);
            }
            else if (crc == 0x0001)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "IMEM Swap CRC PASSED on %s ring\n",
                             (ring? "EPL":"PCIe"));
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "Unexpected value performing swap image CRC on %s ring: %4.4x\n",
                             (ring? "EPL":"PCIe"), crc);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SbmGetBuildRevisionId
 * \ingroup intSerdes
 *
 * \desc            Get the Build-ID and Revision-Id of the code loaded in the
 *                  bus master controller for the specified ring
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring is the EPL or PCIE ring.
 *
 * \param[in]       pValue points to the caller-allocated storage where this
 *                  function will place the requested value. On suceess,
 *                  bits[31..16] will be equal to the Revision-Id and
 *                  bits[15..0] equal to the Build-Id.
 *
 * \return          FM_OK if successful and CRC passes.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmGetBuildRevisionId(fm_int        sw,
                                       fm_serdesRing ring,
                                       fm_uint      *pValue)
{
    fm_status   err;
    fm_uint32   retVal;
    fm_uint     sbusAddr;

    if (pValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *pValue = 0;

        sbusAddr = FM10000_SBUS_SPICO_BCAST_ADDR;

        err = fm10000SbmSpicoInt(sw, ring, sbusAddr, FM10000_SPICO_SBUS_INTR_0x00, 0, &retVal);

        if (err == FM_OK)
        {
            *pValue = retVal & 0xffff0000;
            retVal = 0;

            err = fm10000SbmSpicoInt(sw, ring, sbusAddr, FM10000_SPICO_SBUS_INTR_0x01, 0, &retVal);

            if (err == FM_OK)
            {
                *pValue |= ((retVal >> 16 ) & 0xffff);
            }
        }

        FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"SBM ring #%d, Revision/Build: 0x%8.8x\n", ring, *pValue);
    }
    return err;

}




/*****************************************************************************/
/** fm10000SerDesGetBuildRevisionId
 * \ingroup intSerdes
 *
 * \desc            Get the Build-ID and Revision-Id of the code loaded in the
 *                  specified SerDes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      pValue points to the caller-allocated storage where this
 *                  function will place the requested value. On suceess,
 *                  bits[31..16] will be equal to the Revision-Id and
 *                  bits[15..0] equal to the Build-Id.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is out of range or pValue
 *                  is a NULL pointer.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesGetBuildRevisionId(fm_int    sw,
                                          fm_int    serdes,
                                          fm_uint  *pValue)
{
    fm_status   err;
    fm_uint32   retVal;

    err = FM_OK;

    if (pValue == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *pValue = 0;

        err = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X00, 0, &retVal);

        if (err == FM_OK)
        {
            *pValue = retVal<<16;

            err = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X3F, 0, &retVal);

            if ( err == FM_OK)
            {
                *pValue |= retVal & 0xffff;
            }
        }
    }

    return err;

}




/*****************************************************************************/
/**  fm10000SerdesChckCrcVersionBuildId
 * \ingroup intSerdes
 *
 * \desc            Verify the CRC, the image version and the image build for
 *                  every serdes in the range [firstSerde, lastSerdes]. This
 *                  function validates all the specified serdes, even if it
 *                  finds errors. Serdes numbers must be correlative.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       firstSerdes is the first serdes to check.
 *
 * \param[in]       lastSerdes is the last serdes to check
 *
 * \param[in]       expectedCodeVersionBuildId is the expected version (upper
 *                  16 bits) and build-Id (lower 16 bits)
 *
 * \return          FM_OK if successful if CRC, version and build-Id are
 *                  the expected ones for all serdes.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesChckCrcVersionBuildId(fm_int     sw,
                                             fm_int     firstSerdes,
                                             fm_int     lastSerdes,
                                             fm_uint32  expectedCodeVersionBuildId)
{
    fm_status   err;
    fm_status   localErr;
    fm_int      serdes;
    fm_uint32   versionBuildId;
    fm_int      serdesDbgLvl;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, firstSerdes=%d, lastSerdes=%d, expectedCodeVersionBuildId=0x%8.8x\n",
                 sw,
                 firstSerdes,
                 lastSerdes,
                 expectedCodeVersionBuildId);

    err = FM_OK;
    versionBuildId = 0;

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

    for (serdes = firstSerdes; serdes <= lastSerdes; serdes++)
    {

        if (!fm10000SerdesCheckIfIsActive(sw, serdes))
        {
            continue;
        }


        localErr = fm10000SerdesSpicoDoCrc(sw, serdes);

        if (localErr == FM_OK)
        {
             localErr = fm10000SerDesGetBuildRevisionId(sw, serdes, &versionBuildId);

             if (localErr == FM_OK)
             {
                if ( versionBuildId != expectedCodeVersionBuildId)
                {
                    localErr = FM_FAIL;

                    FM_LOG_ERROR(FM_LOG_CAT_SERDES, "EPL ring: Serdes %d: Bad image Version/Build-Id=0x%8.8x, expected=0x%8.8x\n",
                                 serdes,
                                 versionBuildId,
                                 expectedCodeVersionBuildId);
                }
                else if (serdesDbgLvl > 0)
                {
                    FM_LOG_PRINT(" EPL ring, SerDes #%d: CRC is OK, image version=0x%4.4x, BuildId=0x%4.4x\n",
                                 serdes,
                                 versionBuildId >> 16,
                                 versionBuildId && 0xFFFF);
                }
             }
             else
             {
                 FM_LOG_ERROR(FM_LOG_CAT_SERDES, "EPL ring: Cannot verify Serdes SPICO Version and/or Build-Id, serdes=%d\n", serdes);
             }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_SERDES, "EPL ring: Bad CRC on serdes #%d\n", serdes);
        }


        err = (err != FM_OK)? err : localErr;
    }

    if (err == FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "EPL ring, all SerDes: CRC is OK, image version=0x%4.4x, BuildId=0x%4.4x\n",
                     expectedCodeVersionBuildId >> 16,
                     expectedCodeVersionBuildId && 0xFFFF);

        if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
        {
            if (versionBuildId > 0)
            {
                FM_LOG_PRINT(" EPL ring, all SerDes are OK\n");
            }
        }
    }


    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/**  fm10000SbmChckCrcVersionBuildId
 * \ingroup intSerdes
 *
 * \desc            Verify the CRC, the image version and the image build for
 *                  SBus master (SBM) image.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies the EPL or the PCIe ring
 *
 * \param[in]       expectedCodeVersionBuildId is the expected version (upper
 *                  16 bits) and build-Id (lower 16 bits)
 *
 * \return          FM_OK if successful if CRC, version and build-Id are OK.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmChckCrcVersionBuildId(fm_int     sw,
                                          fm_int     ring,
                                          fm_uint32  expectedCodeVersionBuildId)
{
    fm_status       err;
    fm_uint32       versionBuildId;
    fm_serDesOpMode serdesOpMode;
    fm10000_switch *switchExt;
    fm_int          serdesDbgLvl;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%s, expectedCodeVersionBuildId=0x%8.8x\n",
                 sw,
                 (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe",
                 expectedCodeVersionBuildId);

    switchExt = GET_SWITCH_EXT(sw);

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

    err = fm10000SerdesGetOpMode(sw, 0, &serdesOpMode, NULL, NULL);

    if (err == FM_OK)
    {
        if (switchExt->serdesBypassSbus == TRUE ||
            serdesOpMode == FM_SERDES_OPMODE_STUB_SM )
        {

            FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "%s ring, cannot be verified CRC & SBM image\n",
                         (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");

            if (serdesDbgLvl > 0)
            {
                FM_LOG_PRINT(" %s ring, cannot be verified CRC & SBM image\n",
                             (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
            }
        }
        else
        {

            err = fm10000SbmSpicoDoCrc(sw, ring, FM10000_SBUS_SPICO_BCAST_ADDR);

            if (err == FM_OK)
            {

                err = fm10000SbmGetBuildRevisionId(sw, ring, &versionBuildId);

                if (err == FM_OK)
                {
                    if ( versionBuildId != expectedCodeVersionBuildId)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES, "%s ring: Bad SBM SPICO image Version or Build-Id=0x%8.8x, expected=0x%8.8x\n",
                                     (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe",
                                     versionBuildId,
                                     expectedCodeVersionBuildId);
                    }
                    else
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_SERDES, "%s ring: SBM SPICO image Version=0x%4.4x, BuildId=0x%4.4x\n",
                                     (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe",
                                     versionBuildId >> 16,
                                     versionBuildId & 0xFFFF);

                        if (serdesDbgLvl > 0)
                        {
                            FM_LOG_PRINT(" %s ring: SBM SPICO image Version=0x%4.4x, BuildId=0x%4.4x\n",
                                         (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe",
                                         versionBuildId >> 16,
                                         versionBuildId & 0xFFFF);
                        }
                    }
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_SERDES, "%s ring: Cannot verify SBM SPICO image Version and Build-Id\n",
                                 (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
                }
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_SERDES, "%s ring: Bad CRC of SBM SPICO image\n",
                             (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}





/*****************************************************************************/
/**  fm10000SwapImageCheckCrc
 * \ingroup intSerdes
 *
 * \desc            Verify the CRC of the loaded swap image.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies the EPL or the PCIe ring
 *
 * \param[in]       swapCrcCode is the interrupt to be used to run CRC on
 *                  swap image
 *
 * \return          FM_OK if successful if CRC, version and build-Id are OK
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SwapImageCheckCrc(fm_int     sw,
                                   fm_int     ring,
                                   fm_int     swapCrcCode)
{
    fm_status   err;
    fm_int      serdesDbgLvl;



    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, ring=%s, swapCrcCode=0x%2.2x\n",
                 sw,
                 (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe",
                 swapCrcCode);

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

    err = fm10000SwapImageDoCrc(sw, ring, FM10000_SBUS_SPICO_BCAST_ADDR, swapCrcCode);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES, "%s ring: Bad CRC of Swap SPICO image\n",
                     (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
    }
    else if (serdesDbgLvl > 0)
    {
        FM_LOG_PRINT(" %s ring: Swap SPICO image CRC Ok\n",
                     (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SpicoRamBist
 * \ingroup intSerdes
 *
 * \desc            Perform RAM BIST on SPICO. This function split the
 *                  command into two sequences, START and CHECK. The
 *                  caller can perform the START sequence on all the SERDES
 *                  and then come back and perform the CHECK sequence.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies EPL or PCIE ring.
 *
 * \param[in]       sbusAddr is the SBUS address.
 *
 * \param[in]       cmd is the command type. See fm10000SerdesSpicoBistCmdType.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SpicoRamBist(fm_int                        sw,
                              fm_serdesRing                 ring,
                              fm_uint                       sbusAddr,
                              fm10000SerdesSpicoBistCmdType cmd)
{
    fm_status   status;
    fm_uint32   data;
    fm_bool     eplRing = (ring == FM10000_SERDES_RING_EPL);
    fm_timestamp start;
    fm_timestamp end;
    fm_timestamp diff;
    fm_uint      delTime;

    if (sbusAddr == FM10000_SBUS_SERDES_BCAST_ADDR)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
    else if (sbusAddr == FM10000_SBUS_SPICO_BCAST_ADDR)
    {
        if (cmd & FM10000_SPICO_BIST_CMD_START)
        {
            status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x00, 0x03);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
            status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x00, 0x05);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
        }

        if (cmd & FM10000_SPICO_BIST_CMD_CHECK)
        {
            data = 0x0;
            fmGetTime(&start);
            delTime = 0;
            while (delTime < FM10000_SERDES_BIST_TIMEOUT_MSEC)
            {
                fmGetTime(&end);
                fmSubTimestamps(&end, &start, &diff);
                delTime = diff.sec*1000 + diff.usec/1000;
                status = fm10000SbusRead(sw, eplRing, sbusAddr, 0x00, &data);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

                if (data & 0x18)
                {

                    status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x00, 0x00);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

                    if ((data & 0x18) != 0x08)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES, "RAM BIST on SBus addr 0x%02x=0x%02x failed.\n", sbusAddr, data);
                        return FM_FAIL;
                    }
                    return FM_OK;
                }
            }
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "SPICO RAM BIST timed out in %u msec on SBus address 0x%02x=0x%02x.\n",
                         delTime, sbusAddr, data);

            fm10000SbusWrite(sw, eplRing, sbusAddr, 0x00, 0x00);
            return FM_FAIL;
        }
    }
    else
    {
        if (cmd & FM10000_SPICO_BIST_CMD_START)
        {
            status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x09, 0x08);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

            status = fm10000SbusRead(sw, eplRing, sbusAddr, 0x9, &data);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

            if (data != 0x08)
            {
                FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                    "RAM BIST on SBus addr 0x%02x failed. Returned value: 0x%02x.\n",
                    sbusAddr, data);
                fm10000SbusWrite(sw, eplRing, sbusAddr, 0x09, 0x08);
                return FM_FAIL;
            }

            status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x09, 0x09);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

        }

        if (cmd & FM10000_SPICO_BIST_CMD_CHECK)
        {
            fmGetTime(&start);
            delTime = 0;
            while (delTime < FM10000_SERDES_BIST_TIMEOUT_MSEC)
            {
                fmGetTime(&end);
                fmSubTimestamps(&end, &start, &diff);
                delTime = diff.sec*1000 + diff.usec/1000;
                status = fm10000SbusRead(sw, eplRing, sbusAddr, 0x9, &data);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

                if (((data & 0x3f00) == 0x300) || (data & 0x3f0000))
                {
                    status = fm10000SbusWrite(sw, eplRing, sbusAddr, 0x09, 0x08);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

                    if ((data & 0x3f0000) != 0x0)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                            "RAM BIST on SBus addr 0x%02x failed. Returned value: 0x%02x.\n",
                             sbusAddr, data);
                        return FM_FAIL;
                    }
                    return FM_OK;
                }
            }
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "SPICO RAM BIST timed out in %u msec on SBus address 0x%02x.\n",
                          delTime, sbusAddr);

            fm10000SbusWrite(sw, eplRing, sbusAddr, 0x09, 0x08);
            return FM_FAIL;
        }
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesSpicoInt02Retry
 * \ingroup intSerdes
 *
 * \desc            Perform SERDES interrupt 02 with retry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       data is the interrupt 02 data.
 *
 * \param[in]       timeoutMsec is the timeout in milliseconds to retry.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status fm10000SerdesSpicoInt02Retry(fm_int sw, fm_int serDes, fm_uint32 data, fm_uint timeoutMsec)
{
    fm_status     status;
    fm_timestamp  start;
    fm_timestamp  end;
    fm_timestamp  diff;
    fm_uint       delTime;
    fm_uint32     retVal;

    delTime = 0;
    status  = FM_FAIL;
    fmGetTime(&start);

    while (delTime < timeoutMsec)
    {
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;

        status = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X02, data, &retVal);
        if (retVal == FM10000_SPICO_SERDES_INTR_0X02)
        {
            status = FM_OK;
            break;
        }

        fmDelay(0, FM10000_SERDES_CONFIG_DELAY);
    }

    return status;

}




/*****************************************************************************/
/** fm10000SerdesSpicoIsRunning
 * \ingroup intSerdes
 *
 * \desc            Return whether a given SerDes SPICO is running or not.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          TRUE if the specified SPICO is running, otherwise FALSE.
 *
 *****************************************************************************/
fm_bool fm10000SerdesSpicoIsRunning(fm_int sw,
                                    fm_int serDes)
{
    fm_status       err;
    fm_uint32       pc_1;
    fm_uint32       pc_2;
    fm_uint32       intr;
    fm_uint32       memBist;
    fm_uint32       stepping;
    fm_uint32       enable;
    fm_uint32       error;
    fm_bool         isRunning;
    fm10000_switch *switchExt;


    isRunning = FALSE;

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus == TRUE)
    {



        isRunning = TRUE;
        err = FM_OK;
    }
    else
    {
        pc_1     = 0xDEAD;
        pc_2     = 0xDEAD;
        intr     = 0xDEAD;
        memBist  = 0xDEAD;
        stepping = 0xDEAD;
        enable   = 0xDEAD;
        error    = 0xDEAD;


        err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_25, &pc_1);

        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_25, &pc_2);
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_04, &intr);
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_09, &memBist);
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_07, &enable);
        }

#if 0
        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_20, &stepping);
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesRead(sw, serDes, FM10000_SERDES_REG_2A, &error);
        }
#endif

        if (err == FM_OK)
        {
            if ( (((pc_1 != 0x2) && (pc_1 != 0xffff)) || (pc_1 != pc_2)) &&
                 !(FM_GET_BIT(intr, FM10000_SERDES_REG_04, BIT_16) ||
                   FM_GET_BIT(intr, FM10000_SERDES_REG_04, BIT_16)) &&
                 !(FM_GET_BIT(memBist, FM10000_SERDES_REG_09, BIT_0)))
            {
                isRunning = TRUE;
                FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"SerDes SPICO # %d is running\n", serDes);
            }
            else
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"SerDes SPICO # %d is NOT running\n",serDes);
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,"Cannot determine if SerDes SPICO # %d is running. "
                         "pc=%X %X, intr=%X memBist=%X step=%X enable=%X error=%X\n",
                         serDes, pc_1, pc_2, intr, memBist, stepping, enable, error);
        }
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SWITCH, serDes,
                        "SerDes=0x%2.2x, Error checking if Spico is running\n",
                        serDes);
    }

    return isRunning;

}




/*****************************************************************************/
/** fm10000SbmSpicoIsRunning
 * \ingroup intSerdes
 *
 * \desc            Return whether SBM SPICO is running or not.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies the EPL or the PCIe ring.
 *
 * \return          TRUE if the specified SPICO is running, otherwise FALSE.
 *
 *****************************************************************************/
fm_bool fm10000SbmSpicoIsRunning(fm_int sw,
                                 fm_int ring)
{
    fm_status err;
    fm_uint32 pc;
    fm_bool   isRunning;


    isRunning = FALSE;

    err = fm10000SbusRead(sw, (ring == FM10000_SERDES_RING_EPL), FM10000_SBUS_SPICO_BCAST_ADDR, 0x0a, &pc);

    if (err == FM_OK)
    {

        if ((pc != 0x2) )
        {
            isRunning = TRUE;
            FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"SBM SPICO in %s ring is running\n",(ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"SBM SPICO in %s ring is NOT running\n",(ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
        }
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SERDES,
                     "Cannot determine if SBM SPICO in %s ring is running\n",
                     (ring == FM10000_SERDES_RING_EPL)? "EPL" : "PCIe");
    }

    return isRunning;

}




/*****************************************************************************/
/** fm10000SerdesDmaRead
 * \ingroup intSerdes
 *
 * \desc            Perform DMA read on specified serdes and register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       type is dma type. See fm10000SerdesDmaType.
 *
 * \param[in]       addr is the register address.
 *
 * \param[out]      pReadValue points to the caller-allocated storage where this
 *                  function will place the result.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDmaRead(fm_int               sw,
                               fm_int               serDes,
                               fm10000SerdesDmaType type,
                               fm_uint              addr,
                               fm_uint32           *pReadValue)
{
    fm_status       err;
    fm_uint32       initVal20;
    fm_uint32       initVal01;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, type=%d, addr=0x%2.2x, pReadValue=%p\n",
                    sw,
                    serDes,
                    type,
                    addr,
                    (void*) pReadValue);

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    switch (type)
    {
        case FM10000_SERDES_DMA_TYPE_DMAREG:
        {

            err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x8000 | (addr & 0x00ff), NULL);
            if (err == FM_OK)
            {

                err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X1A, 0x00, pReadValue);
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_LSB:
        {
            err =  fm10000SerdesSpicoInt(sw, serDes, (1 << 14) | (addr & 0x3fff), 0x00, pReadValue);
            break;
        }
        case FM10000_SERDES_DMA_TYPE_LSB_DIRECT:
        {
            if (switchExt->serdesBypassSbus == TRUE)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Serdes=%d, SBus is not available for Serdes DMA LSB direct write access to addr=%4.4x\n",\
                                serDes,
                                addr);
            }
            else
            {
                err = fm10000SerdesWrite(sw, serDes, 0x02, ((addr & 0x1ff) << 16));
                if (err == FM_OK)
                {
                    err = fm10000SerdesRead(sw, serDes, 0x40, pReadValue);
                }
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_ESB:
        {

            if (fm10000SerdesSpicoIsRunning(sw, serDes))
            {

                err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x4000 | (addr & 0x3fff), pReadValue);
                if (err == FM_OK)
                {
                    err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X1A, 0x00, pReadValue);
                }
            }
            else
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Serdes=%d, Spico is not running\n",
                                serDes);
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_DMEM:
        {
            if (switchExt->serdesBypassSbus == TRUE)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Serdes=%d, SBus is not available for Serdes DMA DMEM write access to addr=%4.4x\n",\
                                serDes,
                                addr);
            }
            else
            {

                err = fm10000SerdesReadModifyWrite(sw, serDes, 0x20, 0x03, 0x01, &initVal20);

                if (err == FM_OK)
                {
                    err = fm10000SerdesReadModifyWrite(sw,
                                                       serDes,
                                                       0x01,
                                                       0x40000000 | (addr & 0x3ff),
                                                       0x400003ff,
                                                       &initVal01);
                }

                if (err == FM_OK)
                {
                    err = fm10000SerdesRead(sw, serDes, 0x01, pReadValue);
                    if (err == FM_OK)
                    {
                        *pReadValue = (*pReadValue >> 12) & 0xFFFF;
                    }
                }

                if (err == FM_OK)
                {

                    err = fm10000SerdesWrite(sw, serDes, 0x01, initVal01);
                    if (err == FM_OK)
                    {

                        err = fm10000SerdesWrite(sw, serDes, 0x20, initVal20);
                    }
                }
            }
            break;
        }
        default:
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }

    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Serdes=%d, Error during serdes DMA reading\n",
                        serDes);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesDmaWrite
 * \ingroup intSerdes
 *
 * \desc            Perform DMA write on specified serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       type is dma type. See fm10000SerdesDmaType.
 *
 * \param[in]       addr is the register address.
 *
 * \param[in]       data is the data to be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDmaWrite(fm_int               sw,
                                fm_int               serDes,
                                fm10000SerdesDmaType type,
                                fm_uint              addr,
                                fm_uint32            data)
{
    fm_status   err;
    fm_uint32   initVal20;
    fm_uint32   initVal01;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, type=%d, addr=0x%2.2x, data=0x%4.4x\n",
                    sw,
                    serDes,
                    type,
                    addr,
                    data);

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    switch (type)
    {
        case FM10000_SERDES_DMA_TYPE_DMAREG:
        {

            err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x8000 | (addr & 0x00ff), NULL);
            if (err == FM_OK)
            {

                err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X19, data, NULL);
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_LSB:
        {
            err = fm10000SerdesSpicoInt(sw, serDes, (2 << 14) | (addr & 0x3fff), data, NULL);
            break;
        }
        case FM10000_SERDES_DMA_TYPE_LSB_DIRECT:
        {
            if (switchExt->serdesBypassSbus == TRUE)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Serdes=%d, SBus is not available for Serdes DMA LSB direct write access to addr=%4.4x\n",\
                                serDes,
                                addr);
            }
            else
            {
                err = fm10000SerdesWrite(sw, serDes, 0x02, (0<<31) | ((addr & 0x1ff) << 16) | (data & 0xffff));

                if (err == FM_OK)
                {
                    err = fm10000SerdesWrite(sw, serDes, 0x02, (1<<31) | ((addr & 0x1ff) << 16) | (data & 0xffff));
                }
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_ESB:
        {

            err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x4000 | (addr & 0x3fff), NULL);
            if (err == FM_OK)
            {
                err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X19, data, NULL);
            }
            break;
        }
        case FM10000_SERDES_DMA_TYPE_DMEM:
        {
            if (switchExt->serdesBypassSbus == TRUE)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Serdes=%d, SBus is not available for Serdes DMA DMEM read access to addr=%4.4x\n",\
                                serDes,
                                addr);
            }
            else
            {

                err = fm10000SerdesReadModifyWrite(sw, serDes, 0x20, 0x03, 0x01, &initVal20);
                if (err == FM_OK)
                {
                    err = fm10000SerdesReadModifyWrite(sw,
                                                       serDes,
                                                       0x01, 0x40000000 | (addr & 0x3ff) | ((data & 0xffff) << 12),
                                                       0x4ffff3ff,
                                                       &initVal01);
                    if (err == FM_OK)
                    {
                        err = fm10000SerdesReadModifyWrite(sw,
                                                           serDes,
                                                           0x01,
                                                           0x80000000,
                                                           0x80000000,
                                                           NULL);
                    }
                }
                if (err == FM_OK)
                {

                    err = fm10000SerdesWrite(sw, serDes, 0x01, initVal01);
                    if (err == FM_OK)
                    {

                        err = fm10000SerdesWrite(sw, serDes, 0x20, initVal20);
                    }
                }
            }
            break;
        }
        default:
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }

    }


    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Serdes=%d, Error during serdes DMA writing\n",
                        serDes);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesDmaReadModifyWrite
 * \ingroup intSerdes
 *
 * \desc            Perform DMA write-modify-write on specified serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       type is dma type. See fm10000SerdesDmaType.
 *
 * \param[in]       regAddr is the register address.
 *
 * \param[in]       data contains the bit pattern that will be forced to 1.
 *
 * \param[in]       mask is the bit pattern that will be forced to 0.
 *
 * \param[out]      pReadValue points to the a caller-allocated storage where
 *                  this function will write the original read register value.
 *                  It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDmaReadModifyWrite(fm_int                sw,
                                          fm_int                serDes,
                                          fm10000SerdesDmaType  type,
                                          fm_uint               regAddr,
                                          fm_uint32             data,
                                          fm_uint32             mask,
                                          fm_uint32            *pReadValue)
{
    fm_status err;
    fm_uint32 readVal;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, type=%d, regAddr=0x%2.2x, data=0x%4.4x, mask=0x%4.4x, pReadValue=%p\n",
                    sw,
                    serDes,
                    type,
                    regAddr,
                    data,
                    mask,
                    (void*) pReadValue);

    if (pReadValue)
    {
        *pReadValue = 0;
    }

    err = fm10000SerdesDmaRead(sw, serDes, type, regAddr, &readVal);

    if (err == FM_OK)
    {
        err = fm10000SerdesDmaWrite(sw, serDes, type, regAddr, (data & mask) | (readVal & ~mask));

        if (err == FM_OK && pReadValue != NULL)
        {
            *pReadValue = readVal;
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesResetSpico
 * \ingroup intSerdes
 *
 * \desc            Reset specified SERDES SPICO.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesResetSpico(fm_int sw,
                                  fm_int serDes)
{
    fm_status              err;
    fm_uint32              val;
    fm10000_serDesSmMode   serdesSmMode;
    fm10000_switch        *switchExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                    sw,
                    serDes);

    switchExt = GET_SWITCH_EXT(sw);
    err = fm10000SerdesGetOpMode(sw, serDes, NULL, &serdesSmMode, NULL);

    if (err == FM_OK)
    {

        if (serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE )
        {
            if (switchExt->serdesBypassSbus == FALSE)
            {

                fmDelay(0, FM10000_SERDES_RESET_DELAY);
                err = fm10000SerdesWrite(sw, serDes, 0, 0x00);
                fmDelay(0, FM10000_SERDES_RESET_DELAY);

                if (err == FM_OK)
                {

                    val = 0;
                    FM_SET_BIT(val, FM10000_SERDES_REG_07, BIT_0, 1);
                    FM_SET_BIT(val, FM10000_SERDES_REG_07, BIT_1, 0);
                    FM_SET_BIT(val, FM10000_SERDES_REG_07, BIT_4, 1);
                    err = fm10000SerdesWrite(sw, serDes, FM10000_SERDES_REG_07, val);
                    fmDelay(0, FM10000_SERDES_RESET_DELAY);
                }

                if (err == FM_OK)
                {

                    val = 0;
                    FM_SET_BIT(val, FM10000_SERDES_REG_0B, BIT_18, 1);
                    FM_SET_BIT(val, FM10000_SERDES_REG_0B, BIT_19, 1);
                    err = fm10000SerdesWrite(sw, serDes, FM10000_SERDES_REG_0B, val);
                    fmDelay(0, FM10000_SERDES_RESET_DELAY);
                }

                if (err == FM_OK)
                {

                    val = 0;
                    FM_SET_BIT(val, FM10000_SERDES_REG_07, BIT_4, 1);
                    err = fm10000SerdesWrite(sw, serDes, FM10000_SERDES_REG_07, val);
                    fmDelay(0, FM10000_SERDES_RESET_DELAY);
                }

                if (err == FM_OK)
                {

                    val = 0;
                    FM_SET_BIT(val, FM10000_SERDES_REG_07, BIT_1, 1);
                    err = fm10000SerdesWrite(sw, serDes, FM10000_SERDES_REG_07, val);
                    fmDelay(0, FM10000_SERDES_RESET_DELAY);
                }

                if (err == FM_OK)
                {

                    val = 0;
                    err = fm10000SerdesWrite(sw, serDes, FM10000_SERDES_REG_08, val);
                    fmDelay(0, FM10000_SERDES_RESET_DELAY);
                }
            }
            else
            {


                FM_LOG_PRINT("WARNING: SBus is not available, SerDes Spico cannot be reset\n");
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSpicoSetup
 * \ingroup intSerdes
 *
 * \desc            Reset specified SERDES SPICO and verify the image CRC,
 *                  reload the image is required.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoSetup(fm_int sw,
                                  fm_int serDes)
{
    fm_status       err;
    fm_status       localErr;
    fm_uint         sbusAddr;
    fm_serdesRing   ring;
    fm10000_lane   *pLaneExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                    sw,
                    serDes);


    err = fm10000SerdesResetSpico(sw, serDes);
    fmDelay(0, FM10000_SERDES_RESET_DELAY);

    if (err == FM_OK)
    {
        localErr = fm10000SerdesSpicoDoCrc(sw, serDes);
        fmDelay(0, FM10000_SERDES_RESET_DELAY);

        if (localErr != FM_OK)
        {
            if (pCurSerdesImage != NULL && curRerdesImageSize != 0)
            {
                fm10000MapSerdesToSbus(sw, serDes, &sbusAddr, &ring);

                err = fm10000SerdesSpicoUploadImage(sw,
                                                    ring,
                                                    sbusAddr,
                                                    pCurSerdesImage,
                                                    curRerdesImageSize);

                if (err == FM_OK)
                {
                    err = fm10000SerdesSpicoDoCrc(sw, serDes);

                    if (err == FM_OK)
                    {
                        err = fm10000SerdesResetSpico(sw, serDes);

                        pLaneExt = GET_LANE_EXT(sw, serDes);

                        pLaneExt->serdesRestoredCnt++;
                    }
                    else
                    {

                        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                        "SerDes CRC FAILED on serdes 0x%02x. Recover Michanism Failed\n",
                                        serDes);
                    }
                }

            }

        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSpicoSaveImageParam
 * \ingroup intSerdes
 *
 * \desc            Save the Spico image pointer and size.
 *
 * \param[in]       pRomImg is the buffer containing the SerDes SPICO image.
 *
 * \param[in]       numWords is the size of the image. It may be set to 0,
 *                  in which case the upload stage will be skipped.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoSaveImageParam(const fm_uint16 *pRomImg,
                                           fm_int           numWords)
{
    pCurSerdesImage = pRomImg;
    curRerdesImageSize = numWords;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesSpicoSaveImageParamV2
 * \ingroup intSerdes
 *
 * \desc            Save the Spico image pointer and size.
 *
 * \param[in]       pRomImg is the buffer containing the SerDes SPICO image.
 *
 * \param[in]       numWords is the size of the image. It may be set to 0,
 *                  in which case the upload stage will be skipped.
 *
 * \param[in]       serdesFwVersionBuildId is the version and the Build ID
 *                  of the current SPICO image.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoSaveImageParamV2(const fm_uint16 *pRomImg,
                                             fm_int           numWords,
                                             fm_uint32        serdesFwVersionBuildId)
{
    pCurSerdesImage = pRomImg;
    curRerdesImageSize = numWords;
    curSerdesCodeVersionBuildId = serdesFwVersionBuildId;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesGetTxRxReadyStatus
 * \ingroup intSerdes
 *
 * \desc            Return SERDES TX and RX ready status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pTxRdy points to the caller-allocated storage where this
 *                  function will place the TX ready status. It may be NULL.
 *
 * \param[out]      pRxRdy points to the caller-allocated storage where this
 *                  function will place the RX ready status. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetTxRxReadyStatus(fm_int    sw,
                                          fm_uint   serDes,
                                          fm_bool  *pTxRdy,
                                          fm_bool  *pRxRdy)
{
    fm_status             err;
    fm_uint32             val;
    fm10000_serDesSmMode  serdesSmMode;
    fm_int                epl;
    fm_int                lane;
    fm_uint32             laneSerdesStatus;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm_bool               txRdy;
    fm_bool               rxRdy;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pTxRdy=%p, pRxRdy=%p\n",
                    sw,
                    serDes,
                    (void*)pTxRdy,
                    (void*)pRxRdy);



    err = fm10000SerdesGetOpMode(sw, serDes, NULL, &serdesSmMode, NULL);

    if (err == FM_OK)
    {
        switchExt = GET_SWITCH_EXT(sw);

        if (fm10000SerdesMap[serDes].ring == FM10000_SERDES_RING_EPL  &&
            switchExt->serdesIntUseLaneSai == TRUE                    &&
            serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE)
        {


            switchPtr = GET_SWITCH_PTR(sw);


            err = fm10000MapSerdesToEplLane( sw, serDes, &epl, &lane);

            if (err == FM_OK)
            {
                err = switchPtr->ReadUINT32(sw, FM10000_LANE_SERDES_STATUS(epl, lane), &laneSerdesStatus);

                if (err == FM_OK)
                {

                    FM_LOG_DEBUG(FM_LOG_CAT_SERDES,
                                 "serdes=%2.2d, epl=%d, lane=%d, LANE_SERDES_STATUS= 0x%8.8x\n",
                                 serDes,
                                 epl,
                                 lane,
                                 laneSerdesStatus);

                    txRdy = FM_GET_BIT( laneSerdesStatus, FM10000_LANE_SERDES_STATUS, TxRdy) ? TRUE : FALSE;
                    rxRdy = FM_GET_BIT( laneSerdesStatus, FM10000_LANE_SERDES_STATUS, RxRdy) ? TRUE : FALSE;
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                 "Cannot read SERDES_STATUS, serdes=%2.2d, epl=%d, lane=%d\n",
                                 serDes,
                                 epl,
                                 lane);
                }
            }
        }
        else
        {
            if (serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE)
            {
                err = fm10000SerdesDmaRead(sw,
                                           serDes,
                                           FM10000_SERDES_DMA_TYPE_LSB,
                                           FM10000_AVSD_LSB_ADDR_0X026,
                                           &val);
                txRdy = (val & 0x1) ? TRUE : FALSE;
                rxRdy = (val & 0x2) ? TRUE : FALSE;
            }
            else
            {

                txRdy = TRUE;
                rxRdy = TRUE;
            }
        }
    }

    if (err == FM_OK)
    {
        if (pTxRdy != NULL)
        {
            *pTxRdy = txRdy;
        }
        if (pRxRdy != NULL)
        {
            *pRxRdy = rxRdy;
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesInitSignalOk
 * \ingroup intSerdes
 *
 * \desc            Initialize signal OK status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[in]       threshold is the threshold value to set.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitSignalOk(fm_int sw,
                                    fm_int serdes,
                                    fm_int threshold)
{
    fm_status   err;


    err = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X20, 0x20, NULL);

    if (err == FM_OK)
    {

        err = fm10000SerdesDmaReadModifyWrite(sw,
                                              serdes,
                                              FM10000_SERDES_DMA_TYPE_ESB,
                                              FM10000_AVSD_ESB_ADDR_0X080,
                                              (threshold & 0xf) << 2,
                                              0x3c,
                                              NULL);
        if (err == FM_OK)
        {
            fm10000SerdesGetSignalOk(sw, serdes, NULL);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetSignalOk
 * \ingroup intSerdes
 *
 * \desc            Get signal OK status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pSignalOk points to the caller-allocated storage where
 *                  this function will write the signal_ok status. It may be
 *                  NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetSignalOk(fm_int   sw,
                                   fm_int   serDes,
                                   fm_bool *pSignalOk)
{
    fm_status             err;
    fm_uint32             val;
    fm10000_serDesSmMode  serdesSmMode;
    fm_int                epl;
    fm_int                lane;
    fm_uint32             laneSerdesStatus;
    fm_uint32             serdesCoreStatus;
    fm_switch *           switchPtr;
    fm10000_switch *      switchExt;
    fm_bool               sigOk;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pSignalOk=%p\n",
                    sw,
                    serDes,
                    (void*) pSignalOk);


    if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {

        err = fm10000SerdesGetOpMode(sw, serDes, NULL, &serdesSmMode, NULL);

        if (err == FM_OK)
        {
            switchExt = GET_SWITCH_EXT(sw);

            if (fm10000SerdesMap[serDes].ring == FM10000_SERDES_RING_EPL  &&
                switchExt->serdesIntUseLaneSai == TRUE                    &&
                serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE)
            {


                switchPtr = GET_SWITCH_PTR(sw);


                err = fm10000MapSerdesToEplLane( sw, serDes, &epl, &lane);

                if (err == FM_OK)
                {
                    err = switchPtr->ReadUINT32(sw, FM10000_LANE_SERDES_STATUS(epl, lane), &laneSerdesStatus);

                    if (err == FM_OK)
                    {

                        FM_LOG_DEBUG(FM_LOG_CAT_SERDES,
                                     "serdes=%2.2d, epl=%d, lane=%d, LANE_SERDES_STATUS= 0x%8.8x\n",
                                     serDes,
                                     epl,
                                     lane,
                                     laneSerdesStatus);
                        if (pSignalOk != NULL)
                        {
                            serdesCoreStatus = FM_GET_FIELD(laneSerdesStatus, FM10000_LANE_SERDES_STATUS, CoreStatus);
                            *pSignalOk = serdesCoreStatus & (1<<4) ? TRUE : FALSE;
                        }
                    }
                    else
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                     "Cannot read SERDES_STATUS, serdes=%2.2d, epl=%d, lane=%d\n",
                                     serDes,
                                     epl,
                                     lane);
                    }
                }
            }
            else
            {
                if (serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE)
                {
                    err = fm10000SerdesDmaRead(sw,
                                               serDes,
                                               FM10000_SERDES_DMA_TYPE_LSB,
                                               FM10000_AVSD_LSB_ADDR_0X026,
                                               &val);

                    sigOk = (val & 0x0010) ? FALSE : TRUE;
                    if (!sigOk)
                    {

                        err = fm10000SerdesDmaReadModifyWrite(sw,
                                                              serDes,
                                                              FM10000_SERDES_DMA_TYPE_LSB,
                                                              FM10000_AVSD_LSB_ADDR_0X026,
                                                              0,
                                                              0x0010,
                                                              NULL);
                    }

                    if (pSignalOk)
                    {
                        *pSignalOk = sigOk;
                    }
                }
                else
                {

                    if (pSignalOk)
                    {
                        *pSignalOk = TRUE;
                    }
                }
            }
        }

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SERDES, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetKrTrainingStatus
 * \ingroup intSerdes
 *
 * \desc            Get signal_ok and kr_failure statuses durign KR training.
 *                  In case of error, the returned values remain indeterminated.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pKrSignalOk points to the caller-allocated storage where
 *                  this function will write the signak_ok status. It may be
 *                  NULL.
 *
 * \param[out]      pKrTrainingFailure points to the caller-allocated storage
 *                  where this function will write the kr failure indication
 *                  status. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetKrTrainingStatus(fm_int   sw,
                                           fm_int   serDes,
                                           fm_bool *pKrSignalOk,
                                           fm_bool *pKrTrainingFailure)
{
    fm_status       err;
    fm_uint32       value;
    fm10000_switch *switchExt;
    fm_bool         krSignalOk;
    fm_bool         krFailure;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pKrSignalOk=%p, pKrTrainingFailure=%p\n",
                    sw,
                    serDes,
                    (void*)pKrSignalOk,
                    (void*)pKrTrainingFailure);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus || !switchExt->serdesSupportsKR )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {

        err = fm10000SerdesDmaRead(sw,
                                   serDes,
                                   FM10000_SERDES_DMA_TYPE_LSB_DIRECT,
                                   FM10000_AVSD_LSB_ADDR_0X027,
                                   &value);

        if (err == FM_OK)
        {
            krSignalOk = (value & 0x0010)? TRUE : FALSE;
            krFailure  = (value & 0x0001)? TRUE : FALSE;

            if (pKrSignalOk != NULL)
            {
                *pKrSignalOk = krSignalOk;
            }

            if (pKrTrainingFailure != NULL)
            {
                *pKrTrainingFailure = krFailure;
            }

            FM_LOG_DEBUG_VERBOSE_V2(FM_LOG_CAT_SERDES, serDes,
                                    "SerDes=%-2d, KR status=0x%4.4x  %s   %s\n",
                                    serDes,
                                    value,
                                    krSignalOk? "signalOK" : "        ",
                                    krFailure? "KR_failure" : "          ");
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetLoopbackMode
 * \ingroup intSerdes
 *
 * \desc            Set SERDES loopback mode. This function should not be
 *                  called directly from the port layer, loopback are
 *                  managed by the serdes state machine instead.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      mode is the loopback mode. See fm10000SerdesLbMode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetLoopbackMode(fm_int              sw,
                                       fm_int              serdes,
                                       fm10000SerdesLbMode mode)
{
    fm_status   err;
    fm_uint32   intData;
    fm_uint32   int0x30Data;

    err = FM_OK;
    intData = 0;
    int0x30Data = 0;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serDes=%d, mode=%d\n",
                    sw,
                    serdes,
                    mode);

    switch (mode)
    {
        case FM10000_SERDES_LB_OFF:
            err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X30, int0x30Data);

            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_8, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_0, 0);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_9, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_4, 0);
            break;
        case FM10000_SERDES_LB_INTERNAL_ON:
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_8, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_0, 1);
            break;
        case FM10000_SERDES_LB_INTERNAL_OFF:
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_8, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_0, 0);
            break;
        case FM10000_SERDES_LB_PARALLEL_ON_RX_CLK:
            FM_SET_FIELD(int0x30Data, FM10000_SPICO_SERDES_INTR_0X30, FIELD_2, 0x01);
            err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X30, int0x30Data);
        case FM10000_SERDES_LB_PARALLEL_ON_REFCLK:
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_9, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_4, 1);
            break;
        case FM10000_SERDES_LB_PARALLEL_OFF:
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_9, 1);
            FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X08, BIT_4, 0);
            break;
        default:
            err = FM_ERR_INVALID_ARGUMENT;
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X08, intData);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetLoopbackMode
 * \ingroup intSerdes
 *
 * \desc            Get SERDES loopback mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      mode points to the caller-allocated storage where this
 *                  function will place the SERDES loopback mode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetLoopbackMode(fm_int sw, fm_int serdes, fm10000SerdesLbMode *mode)
{
    fm_status   status;
    fm_uint32   val;


    status = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X024, &val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    if (val & 0x0002)
    {
        *mode = FM10000_SERDES_LB_INTERNAL_ON;
        return FM_OK;
    }

    status = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X021, &val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
    if (val & 0x0020)
    {

        *mode = FM10000_SERDES_LB_PARALLEL_ON_REFCLK;
        return FM_OK;
    }

    *mode = FM10000_SERDES_LB_OFF;
    return status;

}




/*****************************************************************************/
/** fm10000SerdesSetPolarity
 * \ingroup intSerdes
 *
 * \desc            Set SERDES polarity.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      polarity is the polarity. See fm10000SerdesPolarity.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetPolarity(fm_int sw, fm_int serdes, fm10000SerdesPolarity polarity)
{
    fm_status   status;
    fm_uint32   intData;

    intData = 0;

    switch (polarity)
    {
        case  FM10000_SERDES_POLARITY_NONE:
            intData = 0x0300 | 0x0000;
            break;
        case  FM10000_SERDES_POLARITY_INVERT_RX:
            intData = 0x0300 | 0x0010;
            break;
        case  FM10000_SERDES_POLARITY_INVERT_TX:
            intData = 0x0300 | 0x0001;
            break;
        case  FM10000_SERDES_POLARITY_INVERT_TX_RX:
            intData = 0x0300 | 0x0011;
            break;
    }

    status = fm10000SerdesSpicoInt(sw, serdes , FM10000_SPICO_SERDES_INTR_0X13, intData, NULL);

    return status;

}




/*****************************************************************************/
/** fm10000SerdesGetPolarity
 * \ingroup intSerdes
 *
 * \desc            Get SERDES polarity.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      pPolarity points to the caller-allocated storage where this
 *                  function will place the SERDES polarity.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetPolarity(fm_int                 sw,
                                   fm_int                 serdes,
                                   fm10000SerdesPolarity *pPolarity)
{
    fm_status               err;
    fm_uint32               txPol;
    fm_uint32               rxPol;
    fm10000_serDesSmMode    serdesSmMode;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, polarity=%p\n",
                     sw,
                     serdes,
                     (void*)pPolarity);

    err = FM_OK;

    if (pPolarity == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {

        err = fm10000SerdesGetOpMode(sw, serdes, NULL, &serdesSmMode, NULL);

        if (err == FM_OK)
        {
            if (serdesSmMode == FM10000_SERDES_USE_STUB_STATE_MACHINE)
            {

                *pPolarity = FM10000_SERDES_POLARITY_NONE;
            }
            else
            {
                err = fm10000SerdesDmaRead(sw,
                                           serdes,
                                           FM10000_SERDES_DMA_TYPE_ESB,
                                           FM10000_AVSD_ESB_ADDR_0X211,
                                           &txPol);
                if (err == FM_OK)
                {
                    err = fm10000SerdesDmaRead(sw,
                                               serdes,
                                               FM10000_SERDES_DMA_TYPE_ESB,
                                               FM10000_AVSD_ESB_ADDR_0X060,
                                               &rxPol);
                    if (err == FM_OK)
                    {
                        if ((txPol & 0x8) && (rxPol & 0x8))
                        {
                            *pPolarity = FM10000_SERDES_POLARITY_INVERT_TX_RX;
                        }
                        else if ((txPol & 0x8))
                        {
                            *pPolarity = FM10000_SERDES_POLARITY_INVERT_TX;
                        }
                        else if ((rxPol & 0x8))
                        {
                            *pPolarity = FM10000_SERDES_POLARITY_INVERT_RX;
                        }
                        else
                        {
                            *pPolarity = FM10000_SERDES_POLARITY_NONE;
                        }
                    }
                }
            }

        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetTxEq
 * \ingroup intSerdes
 *
 * \desc            Set SERDES TX equalization.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       select is the equalization type. See fm10000SerdesEqSelect.
 *
 * \param[in]       txEq is the equalization to set.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetTxEq(fm_int                sw,
                               fm_int                serdes,
                               fm10000SerdesEqSelect select,
                               fm_int                txEq)
{
    fm_status       err;
    fm_uint32       val;
    fm_serDesOpMode serdesOpMode;
    fm10000_switch *switchExt;

    switchExt = GET_SWITCH_EXT(sw);

    err = fm10000SerdesGetOpMode(sw, serdes, &serdesOpMode, NULL, NULL);

    if (err == FM_OK &&
        switchExt->serdesBypassSbus == FALSE &&
        serdesOpMode != FM_SERDES_OPMODE_STUB_SM )
    {

        val = 0;
        FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X15, FIELD_1, (txEq & 0xff));
        FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X15, FIELD_2, select);


        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X15, BIT_8, 0);

        err= fm10000SerdesSpicoWrOnlyInt(sw,
                                         serdes,
                                         FM10000_SPICO_SERDES_INTR_0X15,
                                         val);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetTxEq
 * \ingroup intSerdes
 *
 * \desc            Get SERDES TX equalization.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       select is the equalization type. See fm10000SerdesEqSelect.
 *
 * \param[in]       txEq points to the caller-allocated storage where this
 *                  function will place the requested equalization.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetTxEq(fm_int sw,
                               fm_int serdes,
                               fm10000SerdesEqSelect select,
                               fm_int *txEq)
{
    fm_status   err;
    fm_uint32   val;
    fm_uint32   retVal;
    fm_serDesOpMode serdesOpMode;
    fm10000_switch *switchExt;

    switchExt = GET_SWITCH_EXT(sw);

    err = fm10000SerdesGetOpMode(sw, serdes, &serdesOpMode, NULL, NULL);

    if (err == FM_OK &&
        switchExt->serdesBypassSbus == FALSE &&
        serdesOpMode != FM_SERDES_OPMODE_STUB_SM )
    {
        val = 0;
        FM_SET_BIT(val, FM10000_SPICO_SERDES_INTR_0X15, BIT_8, 1);

        switch (select)
        {
            case FM10000_SERDES_EQ_SEL_PRECUR:
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X15, FIELD_2, 0);
            break;
            case FM10000_SERDES_EQ_SEL_ATTEN:
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X15, FIELD_2, 1);
            break;
            case FM10000_SERDES_EQ_SEL_POSTCUR:
                FM_SET_FIELD(val, FM10000_SPICO_SERDES_INTR_0X15, FIELD_2, 2);
            break;
            default:
                FM_LOG_EXIT(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
        }

        err = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X15, val, &retVal);

        if (err == FM_OK)
        {
            *txEq = retVal;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetRxTerm
 * \ingroup intSerdes
 *
 * \desc            Set SERDES RX termination.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       rxTerm is the termination value. See fm10000SerdesRxTerm.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetRxTerm(fm_int              sw,
                                 fm_int              serDes,
                                 fm10000SerdesRxTerm rxTerm)
{
    fm_status err;
    fm_uint32 data;

    data = 0;
    err  = FM_OK;

    switch (rxTerm)
    {
        case FM10000_SERDES_RX_TERM_AGND:
        {
            FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_0, 0);
            FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_1, 0);
            break;
        }
        case FM10000_SERDES_RX_TERM_AVDD:
        {
            FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_0, 1);
            FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_1, 0);
            break;
        }
        case FM10000_SERDES_RX_TERM_FLOAT:
        {
            FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_1, 1);
            break;
        }
        default:
            err = FM_ERR_INVALID_ARGUMENT;
    }

    if (err == FM_OK)
    {

        FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X2B, BIT_5, 0);

        err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X2B, data);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetRxTerm
 * \ingroup intSerdes
 *
 * \desc            Get SERDES RX termination.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      rxTerm points to the caller-allocated storage where this
 *                  function will place the requested termination mode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetRxTerm(fm_int sw, fm_int serdes, fm10000SerdesRxTerm *rxTerm)
{
    fm_status   status;
    fm_uint32   val;

    *rxTerm = FM10000_SERDES_RX_TERM_AGND;

    status = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X024, &val);
    if (val & 0x0400)
    {
        *rxTerm = FM10000_SERDES_RX_TERM_FLOAT;
        return status;
    }
    status = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_ESB, FM10000_AVSD_ESB_ADDR_0X020, &val);
    *rxTerm = (val & 0x0400) ? FM10000_SERDES_RX_TERM_AVDD : FM10000_SERDES_RX_TERM_AGND;

    return status;

}




/*****************************************************************************/
/** fm10000SerdesSetBasicCmpMode
 * \ingroup intSerdes
 *
 * \desc            Set SERDES basic compare mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetBasicCmpMode(fm_int   sw,
                                       fm_int   serdes)
{
    fm_status   status;
    fm_int      mode;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d\n",
                    sw,
                    serdes);


    mode = 0x0203;

    status = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X03, mode, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, status);

}




/*****************************************************************************/
/** fm10000SerdesSetDataCoreSource
 * \ingroup intSerdes
 *
 * \desc            Set SERDES data pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[in]       serdesSel is SERDES selection mode. See fm10000SerdesSelect.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetDataCoreSource(fm_int                  sw,
                                         fm_int                  serdes,
                                         fm10000SerdesSelect     serdesSel)
{
    fm_status   err;
    fm_uint32   intData;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, serdesSel=%d\n",
                    sw,
                    serdes,
                    serdesSel);

    intData     = 0;

    switch (serdesSel)
    {
        case FM10000_SERDES_SEL_TX:

            intData = 0x1ff;
            break;
        case FM10000_SERDES_SEL_RX:

            intData = 0x2ff;
            break;
        case FM10000_SERDES_SEL_TX_RX:

            intData = 0x3ff;
            break;
    }

    err = fm10000SerdesSpicoInt02Retry(sw,serdes,intData,FM10000_SERDES_INT02_TIMEOUT_MSEC);

    if (err == FM_OK)
    {

        err = fm10000SerdesDmaWrite(sw,serdes, FM10000_SERDES_DMA_TYPE_DMAREG, 0x21, 0x0c00);
    }
    else
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serdes,
                        "Serdes=%d, Cannot set data data source\n",
                        serdes);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetTxDataSelect
 * \ingroup intSerdes
 *
 * \desc            Set SERDES Tx data pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[in]       dataSel is data select value. See fm10000SerdesDataSelect.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetTxDataSelect(fm_int                    sw,
                                       fm_int                    serdes,
                                       fm10000SerdesTxDataSelect dataSel)
{
    fm_status   err;
    fm_uint32   intData;
    fm_uint     prbs;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, dataSel=%d\n",
                     sw,
                     serdes,
                     dataSel);

    prbs        = 0;
    intData     = 0;
    err         = FM_OK;

    if (dataSel == FM10000_SERDES_TX_DATA_SEL_CORE)
    {
        err = fm10000SerdesSetDataCoreSource(sw, serdes, FM10000_SERDES_SEL_TX);
    }
    else
    {
        switch (dataSel)
        {
            case FM10000_SERDES_TX_DATA_SEL_PRBS7:
                prbs = 0;
                break;
            case FM10000_SERDES_TX_DATA_SEL_PRBS9:
                prbs = 1;
                break;
            case FM10000_SERDES_TX_DATA_SEL_PRBS11:
                prbs = 2;
                break;
            case FM10000_SERDES_TX_DATA_SEL_PRBS15:
                prbs = 3;
                break;
            case FM10000_SERDES_TX_DATA_SEL_PRBS23:
                prbs = 4;
                break;
            case FM10000_SERDES_TX_DATA_SEL_PRBS31:
                prbs = 5;
                break;
            case FM10000_SERDES_TX_DATA_SEL_USER:
                prbs = 7;
                break;
        default:
                err = FM_ERR_INVALID_ARGUMENT;
        }

        FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X02, FIELD_1, prbs);
        FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X02, BIT_8, 1);
        FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X02, BIT_5, 1);

        err = fm10000SerdesSpicoInt02Retry(sw, serdes, intData, FM10000_SERDES_INT02_TIMEOUT_MSEC);

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetRxCmpData
 * \ingroup intSerdes
 *
 * \desc            Set SERDES Rx compare data.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[in]       cmpData is data select value. See fm10000SerdesDataSelect.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetRxCmpData(fm_int                    sw,
                                    fm_int                    serdes,
                                    fm10000SerdesRxCmpData    cmpData)
{
    fm_status   err;
    fm_uint32   intData;
    fm_uint     prbs;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, cmpData=%d\n",
                     sw,
                     serdes,
                     cmpData);

    prbs        = 0;
    intData     = 0;
    err         = FM_OK;

    if (cmpData != FM10000_SERDES_RX_CMP_DATA_OFF)
    {
        switch (cmpData)
        {
            case FM10000_SERDES_RX_CMP_DATA_PRBS7:
                prbs = 0;
                break;
            case FM10000_SERDES_RX_CMP_DATA_PRBS9:
                prbs = 1;
                break;
            case FM10000_SERDES_RX_CMP_DATA_PRBS11:
                prbs = 2;
                break;
            case FM10000_SERDES_RX_CMP_DATA_PRBS15:
                prbs = 3;
                break;
            case FM10000_SERDES_RX_CMP_DATA_PRBS23:
                prbs = 4;
                break;
            case FM10000_SERDES_RX_CMP_DATA_PRBS31:
                prbs = 5;
                break;
            case FM10000_SERDES_RX_CMP_DATA_SELF_SEED:
                prbs = 7;
                break;
            default:
                break;
        }

        FM_SET_FIELD(intData, FM10000_SPICO_SERDES_INTR_0X02, FIELD_1, prbs);
        FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X02, BIT_9, 1);
        FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X02, BIT_5, 1);
        FM_SET_BIT(intData, FM10000_SPICO_SERDES_INTR_0X02, BIT_4, 1);
    }
    else
    {
        intData = 0x2FF;
    }

    err = fm10000SerdesSpicoInt02Retry(sw, serdes, intData, FM10000_SERDES_INT02_TIMEOUT_MSEC);

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesDisablerbsGen
 * \ingroup intSerdes
 *
 * \desc            Disable the PRBS generator.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes number.
 *
 * \param[in]       serdesSel is SERDES selection mode (Tx, Rx or txRx). See
 *                  fm10000SerdesSelect.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesDisablePrbsGen(fm_int                  sw,
                                      fm_int                  serDes,
                                      fm10000SerdesSelect     serdesSel)
{
    fm_status   err;
    fm_uint32   intData;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, serdesSel=%d\n",
                    sw,
                    serDes,
                    serdesSel);

    err = FM_OK;

    switch (serdesSel)
    {
        case FM10000_SERDES_SEL_TX:
            intData = 0x1ff;
            break;
        case FM10000_SERDES_SEL_RX:
            intData = 0x2ff;
            break;
        case FM10000_SERDES_SEL_TX_RX:
            intData = 0x3ff;
            break;
        default:
           FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, FM_ERR_INVALID_ARGUMENT);
    }

    err = fm10000SerdesSpicoInt02Retry(sw, serDes, intData, FM10000_SERDES_INT02_TIMEOUT_MSEC);

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetTxDataSelect
 * \ingroup intSerdes
 *
 * \desc            Get SERDES TX data select.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      dataSel points to the caller-allocated storage where this
 *                  function will place the requested data select value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetTxDataSelect(fm_int                   sw,
                                      fm_int                   serdes,
                                      fm10000SerdesTxDataSelect *dataSel)
{
    fm_status   err;
    fm_uint32   val;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d,  dataSel=%p\n",
                    sw,
                    serdes,
                    (void*)dataSel);

    err  = FM_OK;

    *dataSel = FM10000_SERDES_TX_DATA_SEL_CORE;

    err = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X021, &val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);
    if (val & (1 << 5))
    {
        *dataSel = FM10000_SERDES_TX_DATA_SEL_LOOPBACK;
    }
    else if (val & (1 << 4))
    {
        err = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X029, &val);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

        switch (val & 0x07)
        {
            case 0:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS7;
                break;
            case 1:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS9;
                break;
            case 2:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS11;
                break;
            case 3:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS15;
                break;
            case 4:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS23;
                break;
            case 5:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS31;
                break;
            default:
                *dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
                break;
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}



/*****************************************************************************/
/** fm10000SerdesGetRxCmpData
 * \ingroup intSerdes
 *
 * \desc            Get SERDES RX compare data.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[out]      cmpData points to caller-allocated storage where this function
 *                  should place the data.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetRxCmpData(fm_int                   sw,
                                    fm_int                   serdes,
                                    fm10000SerdesRxCmpData  *cmpData)
{
    fm_status   err;
    fm_uint32   val;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d,  cmpData=%p\n",
                    sw,
                    serdes,
                    (void*)cmpData);

    err = fm10000SerdesDmaRead(sw, serdes, FM10000_SERDES_DMA_TYPE_LSB, FM10000_AVSD_LSB_ADDR_0X02A, &val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    switch (val & 0x07)
    {
        case 0:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS7;
            break;
        case 1:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS9;
            break;
        case 2:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS11;
            break;
        case 3:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS15;
            break;
        case 4:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS23;
            break;
        case 5:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS31;
            break;
        case 7:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_SELF_SEED;
            break;
        default:
            *cmpData = FM10000_SERDES_RX_CMP_DATA_OFF;
            break;
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetUserDataPattern
 * \ingroup intSerdes
 *
 * \desc            Set SERDES user data pattern. The size of the user
 *                  pattern may be 10, 20, 40 and 80 bits. When the pattern
 *                  is bigger than 10 bits, it is divided in chunks of 10 bits
 *                  each. The number of chunks is indicated by patternSize.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes ID.
 *
 * \param[in]       serdesSel is SERDES selection mode, it may be Tx, Rx
 *                  or TxRx. See fm10000SerdesSelect.
 *
 * \param[in]       pPattern10Bit points to the array of 10-bit chunks.
 *
 * \param[in]       patternSize is the number of 10 bit pattern chunks.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetUserDataPattern(fm_int              sw,
                                          fm_int              serDes,
                                          fm10000SerdesSelect serdesSel,
                                          fm_uint32          *pPattern10Bit,
                                          fm_int              patternSize)
{
    fm_status   err;
    fm_int      cnt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, serdesSel=%d, pattern10Bit=%p, patternSize=%d\n",
                    sw,
                    serDes,
                    serdesSel,
                    (void*) pPattern10Bit,
                    patternSize);


    if ( pPattern10Bit == NULL ||
        (patternSize != 1 && patternSize != 2 && patternSize != 4 && patternSize != 8) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
    }


    err = fm10000SerdesDisablePrbsGen(sw, serDes, serdesSel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    if (serdesSel == FM10000_SERDES_SEL_TX || serdesSel == FM10000_SERDES_SEL_TX_RX)
    {

        err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X18, 0x0, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);


        for (cnt = 0 ; cnt < 8 && err == FM_OK; cnt++)
        {
            err = fm10000SerdesSpicoInt(sw,
                                        serDes,
                                        FM10000_SPICO_SERDES_INTR_0X19,
                                        (pPattern10Bit[cnt%patternSize] & 0x3FF),
                                        NULL);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);
        }


        err = fm10000SerdesSetTxDataSelect(sw,
                                           serDes,
                                           FM10000_SERDES_TX_DATA_SEL_USER);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);
    }
    if (serdesSel == FM10000_SERDES_SEL_RX || serdesSel == FM10000_SERDES_SEL_TX_RX)
    {

    }


    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetErrors
 * \ingroup intSerdes
 *
 * \desc            Get SERDES error counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       type is the DMA type. See fm10000SerdesDmaType.
 *
 * \param[out]      pCounter points to the caller-allocated storage where this
 *                  function will replace the requested counter. It may be
 *                  NULL.
 *
 * \param[in]       clearCounter specifies whether to clear counter after
 *                  obtaining the value or not.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetErrors(fm_int                 sw,
                                 fm_int                 serDes,
                                 fm10000SerdesDmaType   type,
                                 fm_uint               *pCounter,
                                 fm_bool                clearCounter)
{
    fm_status              err;
    fm_uint32              cntCntl;
    fm_uint32              val;
    fm10000_serDesSmMode   serdesSmMode;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, type=%d, pCounter=%p, clearCounter=%d\n",
                    sw,
                    serDes,
                    type,
                    (void*)pCounter,
                    clearCounter);


    err = fm10000SerdesGetOpMode(sw, serDes, NULL, &serdesSmMode, NULL);

    if (err == FM_OK)
    {
        if (serdesSmMode != FM10000_SERDES_USE_STUB_STATE_MACHINE)
        {
            err = fm10000SerdesDmaRead(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00D, &cntCntl);

            if (err == FM_OK)
            {
                if (cntCntl & 0x0008)
                {
                    err = fm10000SerdesDmaWrite(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00D, cntCntl & 0xFFF7);
                }

                if (err == FM_OK && pCounter != NULL)
                {

                    err = fm10000SerdesDmaRead(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00E, &val);

                    if (err == FM_OK)
                    {
                        *pCounter = val & 0xFFFF;


                        err = fm10000SerdesDmaRead(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00F, &val);

                        if (err == FM_OK)
                        {
                            (*pCounter) += (val & 0xFFFF) << 16;
                        }

                    }

                }


                if (clearCounter)
                {
                    err = fm10000SerdesDmaWrite(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00D, cntCntl | 0x0001);
                }


                if (err == FM_OK)
                {
                    err = fm10000SerdesDmaWrite(sw, serDes, type, FM10000_AVSD_LSB_ADDR_0X00D, cntCntl);
                }
            }
        }
        else
        {

            if (pCounter != NULL)
            {
                *pCounter = 0;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesInjectErrors
 * \ingroup intSerdes
 *
 * \desc            Inject errors into SERDES TX or RX stream.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       serdesSel is SERDES selection mode. See fm10000SerdesSelect.
 *
 * \param[in]       numErrors is the number of errors to inject.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInjectErrors(fm_int              sw,
                                    fm_int              serdes,
                                    fm10000SerdesSelect serdesSel,
                                    fm_uint             numErrors)
{
    fm_status   status;
    fm_uint     cnt;
    fm_uint32   val;

    if (numErrors == 0)
    {
        numErrors = 1;
    }

    switch (serdesSel)
    {
        case FM10000_SERDES_SEL_TX:
            status = fm10000SerdesSpicoInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X1B, numErrors, NULL);
            break;
        case FM10000_SERDES_SEL_RX:
            status = fm10000SerdesDmaRead(sw,
                                           serdes,
                                           FM10000_SERDES_DMA_TYPE_LSB,
                                           FM10000_AVSD_LSB_ADDR_0X02B,
                                           &val);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

            for (cnt = 0; cnt < numErrors; cnt++)
            {

                status = fm10000SerdesDmaWrite(sw,
                                               serdes,
                                               FM10000_SERDES_DMA_TYPE_LSB,
                                               FM10000_AVSD_LSB_ADDR_0X02B,
                                               val & ~0x2);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

                status = fm10000SerdesDmaWrite(sw,
                                               serdes,
                                               FM10000_SERDES_DMA_TYPE_LSB,
                                               FM10000_AVSD_LSB_ADDR_0X02B,
                                               val | 0x2);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);
            }
            break;
        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    return status;

}




/*****************************************************************************/
/** fm10000GetSerdesWidthModeRateSel
 * \ingroup intSerdes
 *
 * \desc            Return SERDES rate select and width mode.
 *
 * \param[in]       serDes is the target SerDes. It is only used for
 *                  logging tracking.
 *
 * \param[in]       bitRate specifies the bit rate to be configured. See
 *                  ''fm10000_laneBitrate'' for the allowed set of values.
 *
 * \param[out]      pWidthMode the pointer to a caller-allocated area where
 *                  the function will return the width mode. According to the
 *                  bit rate its value may be set to 10, 20 or 40 bits. The
 *                  returned value must be ignored if the specified bit rate
 *                  es invalid. It may be a NULL pointer.
 *
 * \param[out]      pRateSel the pointer to a caller-allocated area where
 *                  the function will return the rate/ratio value. The
 *                  returned value must be ignored if the specified bit rate
 *                  is invalid. It may be a NULL pointer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if ethMode is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetSerdesWidthModeRateSel(fm_int              serDes,
                                           fm_int              bitRate,
                                           fm_serdesWidthMode *pWidthMode,
                                           fm_uint            *pRateSel)
{
    fm_status   err;

    err = FM_OK;

    if (bitRate <= FM10000_LANE_BITRATE_UNKNOWN ||
        bitRate >= FM10000_LANE_BITRATE_MAX)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Invalid bit rate spec= %d\n", bitRate);
    }
    else
    {
        if (pWidthMode != NULL)
        {
            *pWidthMode = serDesWidthModeArray[bitRate];
            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Select widht mode=%d\n", *pWidthMode);
        }

        if (pRateSel != NULL)
        {
            *pRateSel = serDesRateSelArray[bitRate];
            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Select SerDes divider=%d\n", *pRateSel);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000ConfigurePcslBitSlip
 * \ingroup intSerdes
 *
 * \desc            Configures bits slip mechanism. Epl serdes only.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the target SerDes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if serDes is invalid.
 *
 *****************************************************************************/
fm_status fm10000ConfigurePcslBitSlip(fm_int    sw,
                                      fm_int    serDes)
{
    fm_status   err;
    fm_switch * switchPtr;
    fm_int      epl;
    fm_int      lane;
    fm_uint32   pcsl_cfg;
    fm_uint32   intParam;
    fm_uint32   intReturn;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                     sw,
                     serDes);


    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_ERR_INVALID_ARGUMENT;


    if (serDes < 0                    ||
        serDes >= FM10000_NUM_SERDES  ||
        fm10000SerdesMap[serDes].ring != FM10000_SERDES_RING_EPL)
    {

        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Invalid serDes specification; serDes=%d\n", serDes);
    }
    else
    {

        err = fm10000MapSerdesToEplLane(sw, serDes, &epl, &lane);

        if (err == FM_OK)
        {


            intParam = 0;
            FM_SET_BIT(intParam, FM10000_SPICO_SERDES_INTR_0X0C, BIT_7, 1);

            err = fm10000SerdesSpicoInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0C, intParam, &intReturn);
        }

        if (err == FM_OK)
        {
            err = switchPtr->ReadUINT32(sw, FM10000_PCSL_CFG(epl, lane), &pcsl_cfg);


            FM_SET_BIT(pcsl_cfg, FM10000_PCSL_CFG, RxBitSlipInitial, (intReturn & 0x01) );
            FM_SET_BIT(pcsl_cfg, FM10000_PCSL_CFG, RxBitSlipEnable, 0x01);

            if (err == FM_OK)
            {
                err = switchPtr->WriteUINT32(sw, FM10000_PCSL_CFG(epl, lane), pcsl_cfg);
            }
        }

        if (err == FM_OK)
        {

            intParam = 0;
            FM_SET_BIT(intParam, FM10000_SPICO_SERDES_INTR_0X0C, BIT_8, 1);

            err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0C, intParam);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesConfigurePhaseSlip
 * \ingroup intSerdes
 *
 * \desc            Configures serdes Tx and Rx phase slips. Epl serdes only.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the target SerDes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if serDes is invalid.
 *
 *****************************************************************************/
fm_status fm10000SerdesConfigurePhaseSlip(fm_int    sw,
                                          fm_int    serDes)
{
    fm_status     err;
    fm10000_lane *pLaneExt;
    fm_uint32     intParam;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                     sw,
                     serDes);


    err = FM_OK;


    if (serDes < 0                    ||
        serDes >= FM10000_NUM_SERDES  ||
        fm10000SerdesMap[serDes].ring != FM10000_SERDES_RING_EPL)
    {

        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Invalid serDes specification; serDes=%d\n", serDes);
    }
    else
    {
        pLaneExt  = GET_LANE_EXT(sw, serDes);


        if (pLaneExt->pllCalibrationMode & (1 << 4))
        {

            intParam  = (pLaneExt->txPhaseSlip) & 0x001f;


            intParam |= 0x8000;
            err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0D, intParam);
        }

        if (err == FM_OK)
        {

            if (pLaneExt->pllCalibrationMode & (1 << 5))
            {

                intParam  = (pLaneExt->rxPhaseSlip & 0x003f) << 8 ;

                intParam |= 0x8000;
                err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0E, intParam);
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesTxPattern
 *  \ingroup intSerdes
 *
 * \desc            Enable the generation of a Test Pattern for
 *                  a given Serdes. To be used for Built-In Self Tests or
 *                  similar diagnostic functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       submode is the BIST submode (see ''Port State Submodes'').
 *
 * \param[in]       customData0 holds until 40 custom data to be used with
 *                  any of the fixed-size custom test pattern submodes. Only
 *                  lower 40 bits customData0[0..39] are relevant. In the case
 *                  of 80 bits patterns, customData0 holds the 40 LSB of the
 *                  whole pattern.
 *
 * \param[in]       customData1 holds the 40 MSB of an 80 bits custom data
 *                  pattern to be used with 80 bit fixed-size custom test
 *                  pattern submodes. Only lower 40 bits customData1[0..39]
 *                  are relevant.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesTxPattern(fm_int    sw,
                                    fm_int    serdes,
                                    fm_int    submode,
                                    fm_uint64 customData0,
                                    fm_uint64 customData1)
{
    fm_status                   err;
    fm10000SerdesTxDataSelect   dataSel;
    fm_int                      cnt;
    fm_uint32                   pattern10Bit[8];
    fm_int                      pattern10BitSize;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, submode=%d, customData0=0x%llx, customData1=0x%llx\n",
                     sw,
                     serdes,
                     submode,
                     customData0,
                     customData1);

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serdes);

    err = FM_OK;
    pattern10BitSize = 1;


    switch ( submode )
    {
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TXRX_PRBS_128:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS7;
            break;

        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_32K:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS15;
            break;

        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_8M:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS23;
            break;

        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TXRX_PRBS_2G:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS31;
            break;

        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_2048:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS11;
            break;

        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_512B:
            dataSel = FM10000_SERDES_TX_DATA_SEL_PRBS9;
            break;

        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TXRX_IDLECHAR:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_IDLECHAR;
            break;

        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TXRX_LOWFREQ:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_LOWFREQ;
            break;

        case FM_BIST_TX_HIGHFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_HIGHFREQ;
            break;

        case FM_BIST_TX_MIXEDFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_MIXEDFREQ;
            pattern10BitSize = 2;
            break;

        case FM_BIST_TX_SQUARE8:
        case FM_BIST_TXRX_SQUARE8:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_SQUARE8_0;
            customData1 = FM10000_SERDES_BIST_PATTERN_SQUARE8_1;
            pattern10BitSize = 8;
            break;

        case FM_BIST_TX_SQUARE10:
        case FM_BIST_TXRX_SQUARE10:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 = FM10000_SERDES_BIST_PATTERN_SQUARE10;
            pattern10BitSize = 2;
            break;

        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM10:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 &= 0x3ff;
            break;

        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM20:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 &= 0xFFFFF;
            pattern10BitSize = 2;
            break;

        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM40:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 &= 0xFFFFFFFFFFLL;
            pattern10BitSize = 4;
            break;

        case FM_BIST_TX_CUSTOM80:
        case FM_BIST_TXRX_CUSTOM80:
            dataSel = FM10000_SERDES_TX_DATA_SEL_USER;
            customData0 &= 0xFFFFFFFFFFLL;
            customData1 &= 0xFFFFFFFFFFLL;
            pattern10BitSize = 8;
            break;

        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TX_CJPAT:
        case FM_BIST_TXRX_CJPAT:







            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Unsupported BIST submode: %d\n",
                         submode );
            err = FM_ERR_UNSUPPORTED;
            break;
        default:
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Invalid BIST submode: %d\n",
                         submode );
            err = FM_ERR_INVALID_SUBMODE;

    }

    if (err == FM_OK)
    {
        if (dataSel == FM10000_SERDES_TX_DATA_SEL_USER)
        {
            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serdes,
                            "Bist UserPattern(%d): 0x%llx:%llx\n",
                            pattern10BitSize,
                            customData0,
                            customData1);
            for (cnt = 0 ; cnt < pattern10BitSize ; cnt ++)
            {
                if (cnt <= 3)
                {
                    pattern10Bit[cnt] = (customData0 >> (cnt * 10)) & 0x3FF;
                }
                else
                {
                    pattern10Bit[cnt] = (customData1 >> ((cnt-4) * 10)) & 0x3FF;
                }
            }

            err = fm10000SerdesSetUserDataPattern(sw,
                                                  serdes,
                                                  FM10000_SERDES_SEL_TX,
                                                  pattern10Bit,
                                                  pattern10BitSize);
        }
        else
        {
            err = fm10000SerdesSetTxDataSelect(sw, serdes, dataSel);
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X14, 0x33);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesRxPattern
 *  \ingroup intSerdes
 *
 * \desc            Enable the comparison of data received by
 *                  a Serdes with a given test pattern. To be used for Built-In
 *                  Self Tests or similar diagnostic functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       submode is the BIST submode (see ''Port State Submodes'').
 *
 * \param[in]       customData0 holds until 40 custom data to be used with
 *                  any of the fixed-size custom test pattern submodes. Only
 *                  lower 40 bits customData0[0..39] are relevant. In the case
 *                  of 80 bits patterns, customData0 holds the 40 LSB of the
 *                  whole pattern.
 *
 * \param[in]       customData1 holds the 40 MSB of an 80 bits custom data
 *                  pattern to be used with 80 bit fixed-size custom test
 *                  pattern submodes. Only lower 40 bits customData1[0..39]
 *                  are relevant.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesRxPattern(fm_int    sw,
                                    fm_int    serdes,
                                    fm_int    submode,
                                    fm_uint64 customData0,
                                    fm_uint64 customData1)
{
    fm_status               err;
    fm10000SerdesRxCmpData  cmpData;

    FM_NOT_USED(customData0);
    FM_NOT_USED(customData1);

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serdes,
                    "sw=%d, serdes=%d, submode=%d, customData0=0x%llx, customData1=0x%llx\n",
                     sw,
                     serdes,
                     submode,
                     customData0,
                     customData1);

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serdes);

    err = FM_OK;
    cmpData = FM10000_SERDES_RX_CMP_DATA_OFF;


    switch ( submode )
    {
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_TXRX_PRBS_128:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS7;
            break;

        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_32K:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS15;
            break;

        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_8M:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS23;
            break;

        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_TXRX_PRBS_2G:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS31;
            break;

        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_2048:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS11;
            break;

        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_512B:
            cmpData = FM10000_SERDES_RX_CMP_DATA_PRBS9;
            break;

        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        case FM_BIST_TXRX_CUSTOM80:
            cmpData = FM10000_SERDES_RX_CMP_DATA_SELF_SEED;
            break;

        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_RX_CJPAT:
        case FM_BIST_TXRX_CJPAT:






            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Unsupported BIST submode: %d\n",
                         submode );
            err = FM_ERR_UNSUPPORTED;
            break;
        default:
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Invalid BIST submode: %d\n",
                         submode );
            err = FM_ERR_INVALID_SUBMODE;

    }



    if (err == FM_OK)
    {
        err = fm10000SerdesSetBasicCmpMode(sw,serdes);
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSetRxCmpData(sw, serdes, cmpData);
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X0C, 0);
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoWrOnlyInt(sw, serdes, FM10000_SPICO_SERDES_INTR_0X14, 0x33);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serdes, err);

}




/*****************************************************************************/
/** fm10000ClearSerdesTxPattern
 * \ingroup intSerdes
 *
 * \desc            Disables the generation of a Test Pattern for
 *                  a given Serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ClearSerdesTxPattern(fm_int sw, fm_int serdes)
{
    fm_status status;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serdes);

    status = fm10000SerdesSetTxDataSelect(sw,
                                          serdes,
                                          FM10000_SERDES_TX_DATA_SEL_CORE);

    return status;

}




/*****************************************************************************/
/** fm10000ClearSerdesRxPattern
 * \ingroup intSerdes
 *
 * \desc            Disable the comparison of data received by
 *                  a Serdes with a given test pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ClearSerdesRxPattern(fm_int sw, fm_int serdes)
{
    fm_status status;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serdes);

    status = fm10000SerdesSetRxCmpData(sw,
                                       serdes,
                                       FM10000_SERDES_RX_CMP_DATA_OFF);

    return status;

}




/*****************************************************************************/
/** fm10000ResetSerdesErrorCounter
 * \ingroup intSerdes
 *
 * \desc            Clear SerDes error counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ResetSerdesErrorCounter(fm_int sw,
                                         fm_int serDes)
{
    fm_status   err;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                    sw,
                    serDes);

    if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {
        err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X17, 0);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000GetSerdesErrorCounter
 * \ingroup intSerdes
 *
 * \desc            Return SERDES error counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pCounter pointer to a caller-defined memory location where
 *                  this function returns the value of the error counter.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000GetSerdesErrorCounter(fm_int     sw,
                                       fm_int     serDes,
                                       fm_uint32 *pCounter)
{
    fm_status   err;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pCounter=%p\n",
                    sw,
                    serDes,
                    (void*) pCounter);

    if (pCounter == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {
        err = fm10000SerdesGetErrors(sw, serDes, FM10000_SERDES_DMA_TYPE_LSB, pCounter, FALSE);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesCursor
 * \ingroup intSerdes
 *
 * \desc            Set SERDES cursor or attenuation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       cursor is the SERDES cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesCursor(fm_int     sw,
                                 fm_int     serDes,
                                 fm_int     cursor)
{
    fm_status       err;
    fm_laneAttr    *pLaneAttr;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, cursor=%d\n",
                    sw,
                    serDes,
                    cursor);

    if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {
        pLaneAttr = GET_LANE_ATTR( sw, serDes );

        if ( !SerdesValidateAttenuationCoefficients(cursor, pLaneAttr->preCursor,pLaneAttr->postCursor) )
        {
            err = FM_ERR_INVALID_VALUE;

            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,serDes,
                            "SerDes #%d: cursor is out of range\n",
                            serDes);
        }
        else
        {
            err = fm10000SerdesSetTxEq(sw, serDes, FM10000_SERDES_EQ_SEL_ATTEN, cursor);

            if (err == FM_OK)
            {
                pLaneAttr->cursor = cursor;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesPreCursor
 * \ingroup intSerdes
 *
 * \desc            Set SERDES pre-cursor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       preCursor is the SERDES pre-cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesPreCursor(fm_int    sw,
                                    fm_int    serDes,
                                    fm_int    preCursor)
{
    fm_status       err;
    fm_laneAttr    *pLaneAttr;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, preCursor=%d\n",
                    sw,
                    serDes,
                    preCursor);


    if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {
        pLaneAttr = GET_LANE_ATTR( sw, serDes );

        if ( !SerdesValidateAttenuationCoefficients(pLaneAttr->cursor, preCursor, pLaneAttr->postCursor) )
        {
            err = FM_ERR_INVALID_VALUE;

            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,serDes,
                            "SerDes #%d: preCursor is out of range\n",
                            serDes);
        }
        else
        {
            err = fm10000SerdesSetTxEq(sw, serDes, FM10000_SERDES_EQ_SEL_PRECUR, preCursor & 0xff);

            if (err == FM_OK)
            {
                pLaneAttr->preCursor = preCursor;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesPostCursor
 * \ingroup intSerdes
 *
 * \desc            Set SERDES post-cursor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       postCursor is the SERDES post-cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesPostCursor(fm_int    sw,
                                     fm_int    serDes,
                                     fm_int    postCursor)
{
    fm_status       err;
    fm_laneAttr    *pLaneAttr;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, postCursor=%d\n",
                    sw,
                    serDes,
                    postCursor);

    if ( (serDes < 0) || (serDes >= FM10000_NUM_SERDES) )
    {
        err = FM_ERR_INVALID_ARGUMENT;


        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Invalid SERDES number: %d\n",
                      serDes );
    }
    else
    {
        pLaneAttr = GET_LANE_ATTR( sw, serDes );

        if ( !SerdesValidateAttenuationCoefficients(pLaneAttr->cursor, pLaneAttr->preCursor, postCursor) )
        {
            err = FM_ERR_INVALID_VALUE;

            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,serDes,
                            "SerDes #%d: Postcursor is out of range\n",
                            serDes);
        }
        else
        {
            err = fm10000SerdesSetTxEq(sw, serDes, FM10000_SERDES_EQ_SEL_POSTCUR, postCursor & 0xff);

            if (err == FM_OK)
            {
                pLaneAttr->postCursor = postCursor;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetSerdesLanePolarity
 * \ingroup intSerdes
 *
 * \desc            Set TX and RX lane polarity on a given Serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number on which to operate.
 *
 * \param[in]       invertTx set to TRUE if TX lane polarity needs inverted.
 *
 * \param[in]       invertRx set to TRUE if RX lane polarity needs inverted.
 *
 * \return          FM_OK if successfull
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetSerdesLanePolarity(fm_int sw,
                                       fm_int serdes,
                                       fm_bool invertTx,
                                       fm_bool invertRx)
{
    fm10000SerdesPolarity   polarity;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serdes);

    if (invertTx && invertRx)
    {
        polarity = FM10000_SERDES_POLARITY_INVERT_TX_RX;
    }
    else if (invertTx)
    {
        polarity = FM10000_SERDES_POLARITY_INVERT_TX;
    }
    else if (invertRx)
    {
        polarity = FM10000_SERDES_POLARITY_INVERT_RX;
    }
    else
    {
        polarity = FM10000_SERDES_POLARITY_NONE;
    }

    return fm10000SerdesSetPolarity(sw, serdes, polarity);

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningStartICal
 * \ingroup intSerdes
 *
 * \desc            Start DFE tuning iCal (initial calibration)
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningStartICal(fm_int     sw,
                                          fm_int     serDes)
{
    fm_status   err;
    fm_uint32   dfeControl;
    fm_int      param;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    err = fm10000GetPortOptModeDfeParam(sw, serDes, 0, &param);

    if (err == FM_OK)
    {
        err = fm10000SerdesConfigDfeParam(sw, serDes, 0, param);
    }

    if (err == FM_OK)
    {

        dfeControl = 0x01;
        err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0A, dfeControl);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningStartPCalSingleExec
 * \ingroup intSerdes
 *
 * \desc            Start DFE tuning pCal (periodic calibration), single
 *                  execution
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningStartPCalSingleExec(fm_int     sw,
                                                    fm_int     serDes)
{
    fm_status   err;
    fm_uint32   dfeControl;
    fm_int      param;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    err = fm10000GetPortOptModeDfeParam(sw, serDes, 1, &param);

    if (err == FM_OK)
    {
        err = fm10000SerdesConfigDfeParam(sw, serDes, 0, param);
    }


    dfeControl = 0x02;
    err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0A, dfeControl);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningStartPCalContinuous
 * \ingroup intSerdes
 *
 * \desc            Start DFE tuning pCal (periodic calibration), continuous
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningStartPCalContinuous(fm_int     sw,
                                                    fm_int     serDes)
{
    fm_status   err;
    fm_uint32   dfeControl;
    fm_int      param;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    err = fm10000GetPortOptModeDfeParam(sw, serDes, 1, &param);

    if (err == FM_OK)
    {
        err = fm10000SerdesConfigDfeParam(sw, serDes, 0, param);
    }


    dfeControl = 0x06;
    err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0A, dfeControl);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningStopAll
 * \ingroup intSerdes
 *
 * \desc            Stop DFE tuning, both iCal and pCal
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningStopAll(fm_int     sw,
                                        fm_int     serDes)
{
    fm_status   err;
    fm_uint32   dfeControl;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    dfeControl = 0x00;
    err = fm10000SerdesSpicoWrOnlyInt(sw, serDes, FM10000_SPICO_SERDES_INTR_0X0A, dfeControl);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningGetStatus
 * \ingroup intSerdes
 *
 * \desc            Read the DFE tuning status for the specified serdes
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pDfeStatus is a pointer to a caller-allocated area where
 *                  this function will return the dfe tuning status.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if pDfeStatus is a NULL pointer
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningGetStatus(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint32 *pDfeStatus)
{
    fm_status   err;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    if (pDfeStatus == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26_READ,
                                    FM10000_SERDES_DFE_PARAM_DFE_STATUS_REG,
                                    pDfeStatus);

        FM_LOG_DEBUG_VERBOSE_V2(FM_LOG_CAT_SERDES, serDes,
                                "SerDes=%-2d, dfeStatus=%4.4x\n",
                                serDes,
                                *pDfeStatus);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningGetICalStatus
 * \ingroup intSerdes
 *
 * \desc            Get the iCal status for the specified serdes. TRUE is
 *                  returned if iCal is in progress or FALSE, otherwise.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pICalStatus is a pointer to a caller-allocated area where
 *                  this function will return the status of iCal.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if pDfeStatus is a NULL pointer
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningGetICalStatus(fm_int     sw,
                                              fm_int     serDes,
                                              fm_bool   *pICalStatus)
{
    fm_status   err;
    fm_uint32   dfeStatus;


    if (pICalStatus == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fm10000SerdesDfeTuningGetStatus (sw, serDes, &dfeStatus);

        if (err == FM_OK)
        {
            *pICalStatus = ((dfeStatus & 0x11) == 0x11) ? TRUE : FALSE;

            FM_LOG_DEBUG_VERBOSE_V2(FM_LOG_CAT_SERDES, serDes,
                                    "SerDes=%-2d, iCal complete=%s\n",
                                    serDes,
                                    *pICalStatus? "FALSE": "TRUE");
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningCheckICalConvergence
 * \ingroup intSerdes
 *
 * \desc            Check if iCal was succesful or not. The criterion is
 *                  to perform a quick validation that was suggested by the
 *                  serdes provider. If iCal is succesful, this function
 *                  returns iCalSuccessful equal to TRUE or FALSE otherwise.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pICalSuccessful is a pointer to a caller-allocated area
 *                  where this function will return TRUE if iCal is
 *                  consiedered successful or FALSE otherwise.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if pDfeStatus is a NULL pointer
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningCheckICalConvergence(fm_int     sw,
                                                     fm_int     serDes,
                                                     fm_bool   *pICalSuccessful)
{
    fm_status       err;
    fm10000_lane *  pLaneExt;
    fm_uint32       value1;
    fm_uint32       value2;
    fm_uint32       vDiff;


    if (pICalSuccessful == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pLaneExt = GET_LANE_EXT(sw, serDes);
        *pICalSuccessful = FALSE;


        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26_READ,
                                    ((4 << 12) | 0),
                                    &value1);
        if (err == FM_OK)
        {
            err = fm10000SerdesSpicoInt(sw,
                                        serDes,
                                        FM10000_SPICO_SERDES_INTR_0X26_READ,
                                        ((4 << 12) | (1 << 8)),
                                        &value2);
        }

        if (err == FM_OK)
        {

            value1 = (value1 & 0x8000) ? value1 | 0xffff0000 : value1;
            value2 = (value2 & 0x8000) ? value2 | 0xffff0000 : value2;
            vDiff  = abs(value2 - value1);


            if ( vDiff >= pLaneExt->dfeExt.dfeDataLevThreshold)
            {
                *pICalSuccessful = TRUE;
            }

            FM_LOG_DEBUG_VERBOSE_V2(FM_LOG_CAT_SERDES, serDes,
                                    "SerDes=%-2d, level=%d, threshold=%d, susccessful=%s\n",
                                    serDes,
                                    vDiff,
                                    pLaneExt->dfeExt.dfeDataLevThreshold,
                                    *pICalSuccessful? "TRUE" : "FALSE");
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningGetPCalStatus
 * \ingroup intSerdes
 *
 * \desc            Get the pCal status for the specified serdes. TRUE is
 *                  returned is pCal is in progress or FALSE, otherwise.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pPCalStatus is a pointer to a caller-allocated area where
 *                  this function will return the status of pCal.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if pDfeStatus is a NULL pointer
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningGetPCalStatus (fm_int     sw,
                                               fm_int     serDes,
                                               fm_bool   *pPCalStatus)
{
    fm_status   err;
    fm_uint32   dfeStatus;



    if (pPCalStatus == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fm10000SerdesDfeTuningGetStatus (sw, serDes, &dfeStatus);
        if (err == FM_OK)
        {
            *pPCalStatus = (dfeStatus & 0x02) ? TRUE : FALSE;

            FM_LOG_DEBUG_VERBOSE_V2(FM_LOG_CAT_SERDES, serDes,
                                    "SerDes=%-2d, pCal complete=%s\n",
                                    serDes,
                                    *pPCalStatus? "FALSE": "TRUE");
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningStop
 * \ingroup intSerdes
 *
 * \desc            Stop DFE tuning on the specified serdes
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningStop(fm_int     sw,
                                     fm_int     serDes)
{
    fm_status     err;
    fm_status     err2;
    fm_uint32     dfeStatus;
    fm_bool       dfeXCalStatus;
    fm_bool       signalOk;
    fm_uint32     coarseTimeoutCnt;
    fm_uint32     fineTimeoutCnt;
    fm10000_lane *pLaneExt;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    pLaneExt  = GET_LANE_EXT(sw, serDes);


    coarseTimeoutCnt = 0;
    fineTimeoutCnt = 0;

    pLaneExt->dfeExt.refTimeMs = fm10000SerdesGetTimestampMs();
    err = fm10000SerdesIncrStatsCounter(sw,serDes,1);

    if (err == FM_OK)
    {

        err = fm10000SerdesDfeTuningGetStatus(sw, serDes, &dfeStatus);
    }

    if (err == FM_OK)
    {
        if (dfeStatus & 0x03)
        {
            fm10000SerdesDfeTuningStopAll(sw,serDes);


            do
            {
                err = fm10000SerdesDfeTuningGetICalStatus(sw,serDes, &dfeXCalStatus);

                if (err != FM_OK || !dfeXCalStatus )
                {
                    break;
                }

                fmDelay(0, FM10000_SERDES_DFE_STOP_CYCLE_DELAY);

            } while (++coarseTimeoutCnt < FM10000_SERDES_ICAL_STOP_MAX_CYCLES);


            if (err == FM_OK && coarseTimeoutCnt < FM10000_SERDES_ICAL_STOP_MAX_CYCLES)
            {
                do
                {
                    err = fm10000SerdesDfeTuningGetPCalStatus(sw,serDes, &dfeXCalStatus);

                    if (err != FM_OK || !dfeXCalStatus )
                    {
                        break;
                    }

                    fmDelay(0, FM10000_SERDES_DFE_STOP_CYCLE_DELAY);

                } while (++fineTimeoutCnt < FM10000_SERDES_PCAL_STOP_MAX_CYCLES);
            }
        }

        if (coarseTimeoutCnt < FM10000_SERDES_ICAL_STOP_MAX_CYCLES &&
            fineTimeoutCnt   < FM10000_SERDES_PCAL_STOP_MAX_CYCLES)
        {
            if (dfeStatus & 0x40)
            {

                err = fm10000SerdesDfeTuningStartPCalSingleExec(sw,serDes);

                if (err == FM_OK)
                {
                    do
                    {
                        err = fm10000SerdesDfeTuningGetPCalStatus(sw,serDes, &dfeXCalStatus);

                        if (err != FM_OK || !dfeXCalStatus )
                        {
                            break;
                        }

                        fmDelay(0, FM10000_SERDES_DFE_STOP_CYCLE_DELAY);

                    } while (++fineTimeoutCnt < FM10000_SERDES_PCAL_STOP_MAX_CYCLES);
                }
            }
        }

        if (coarseTimeoutCnt >= FM10000_SERDES_ICAL_STOP_MAX_CYCLES ||
            fineTimeoutCnt   >= FM10000_SERDES_PCAL_STOP_MAX_CYCLES)
        {
            err = fm10000SerdesForcedDfeStop(sw,serDes);
            pLaneExt->dfeExt.forcedStopCycleCnt++;
        }
    }

    if (coarseTimeoutCnt > 0)
    {
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes, "Delay to stop coarse tuning=%d cycles\n", coarseTimeoutCnt);
    }

    if (fineTimeoutCnt > 0)
    {
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes, "Delay to stop fine tuning=%d cycles\n", fineTimeoutCnt);
    }


    err2 = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

    if (err2 == FM_OK && (!signalOk || pLaneExt->dfeExt.retryCntr == 0))
    {
        err2 = fm10000SerdesDfeTuningReset(sw,serDes);
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Serdes=%d: Cannot stop DFE\n",
                        serDes);
    }

    if (err2 != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Serdes=%d: Cannot reset DFE\n",
                        serDes);
    }


    pLaneExt->dfeExt.eyeScoreHeight   = -1;
    pLaneExt->dfeExt.eyeScoreHeightmV = -1;


    pLaneExt->dfeExt.dfeTuningStat = 0;


    fm10000SerdesSaveStopTuningStatsInfo(sw,serDes,coarseTimeoutCnt,fineTimeoutCnt);
    fm10000SerdesSaveStopTuningDelayInfo(sw,serDes);

    return err;

}




/*****************************************************************************/
/** fm10000SerdesForcedDfeStop
 * \ingroup intSerdes
 *
 * \desc            Recovers the serdes from an abnormal condition. This
 *                  function only may be called if there was a timeout halting
 *                  DFE tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesForcedDfeStop(fm_int       sw,
                                     fm_int       serDes)
{
    fm_status           err;
    fm_laneAttr        *pLaneAttr;
    fm10000_lane       *pLaneExt;
    fm_bool             txRdy;
    fm_bool             rxRdy;
    fm_int              loopCnt;
    fm_int              serdesDbgLvl;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d\n",
                    sw,
                    serDes);

    pLaneExt  = GET_LANE_EXT(sw, serDes);
    pLaneAttr = &(pLaneExt->base->attributes);

    serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;


    err = fm10000SerdesResetSpico(sw,serDes);

    if ( err == FM_OK )
    {
        err = fm10000SerdesTxRxEnaCtrl(sw,
                                       serDes,
                                       FM10000_SERDES_CTRL_TX_ENA_MASK     |
                                       FM10000_SERDES_CTRL_RX_ENA_MASK     |
                                       FM10000_SERDES_CTRL_OUTPUT_ENA_MASK | 0);
    }

    if ( err == FM_OK )
    {
        err = fm10000SerdesSetBitRate(sw,serDes,pLaneExt->rateSel);

        if ( err == FM_OK )
        {
             err = fm10000SetPcslCfgWidthMode(sw, serDes, pLaneExt->widthMode);
        }
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSetTxDataSelect(sw,
                                           serDes,
                                           FM10000_SERDES_TX_DATA_SEL_CORE);
    }

    if ( err == FM_OK )
    {
        err = fm10000SetSerdesCursor(sw, serDes, pLaneAttr->cursor);

        if (err == FM_OK)
        {
            err = fm10000SetSerdesPreCursor(sw, serDes, pLaneAttr->preCursor);

            if (err == FM_OK)
            {
                err = fm10000SetSerdesPostCursor(sw, serDes, pLaneAttr->postCursor);
            }
        }
    }

    if ( err == FM_OK )
    {
        err = fm10000SerdesSetPllCalibrationMode(sw, serDes);

        if (err == FM_OK)
        {
            err = fm10000SerdesConfigurePhaseSlip(sw,serDes);

            if (err == FM_OK)
            {
                err = fm10000SerdesSetRxTerm(sw, serDes, pLaneExt->rxTermination);
            }
        }
    }

    if ( err == FM_OK )
    {
        err = fm10000SetSerdesLanePolarity(sw,
                                           serDes,
                                           (pLaneAttr->txPolarity != 0),
                                           (pLaneAttr->rxPolarity != 0));
    }


    if ( err == FM_OK )
    {
        err = fm10000SerdesTxRxEnaCtrl(sw,
                                       serDes,
                                       FM10000_SERDES_CTRL_TX_ENA_MASK |
                                       FM10000_SERDES_CTRL_TX_ENA      |
                                       FM10000_SERDES_CTRL_RX_ENA_MASK |
                                       FM10000_SERDES_CTRL_RX_ENA);
        loopCnt = 100;

        while (err == FM_OK)
        {
            err = fm10000SerdesGetTxRxReadyStatus(sw,serDes,&txRdy,&rxRdy);

            if (txRdy && rxRdy && err == FM_OK)
            {
                break;
            }

            if (--loopCnt <= 0)
            {
                err = FM_FAIL;
            }

            fmDelay(0,1000000);
        }

        if (loopCnt <= 0)
        {
            err = FM_FAIL;

            if (serdesDbgLvl > 0)
            {
                FM_LOG_PRINT("Serdes %d Recovery timeout\n", serDes);
            }
        }
        else
        {
            if (serdesDbgLvl > 0)
            {
                FM_LOG_PRINT("Serdes %d Recovery counter=%d\n",serDes, 100-loopCnt);
            }
        }

        if (err == FM_OK)
        {
            err = fm10000ConfigurePcslBitSlip(sw, serDes);

            if (err == FM_OK)
            {
                err = fm10000SerdesSetWidthMode(sw,serDes,pLaneExt->widthMode);
            }
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesTxRxEnaCtrl(sw,
                                           serDes,
                                           FM10000_SERDES_CTRL_OUTPUT_ENA_MASK  |
                                           FM10000_SERDES_CTRL_OUTPUT_ENA);
        }

        fmDelay(0, FM10000_SERDES_RESET_DELAY);

        if (err == FM_OK)
        {
            err = fm10000SerdesInitSignalOk(sw, serDes, 0);
        }

        fmDelay(0,3*FM10000_SERDES_RESET_DELAY);

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningConfig
 * \ingroup intSerdes
 *
 * \desc            Configures the serdes to run DFE tuning. Note that this
 *                  function only configures hardware related values and
 *                  state variables are not initialized.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningConfig(fm_int     sw,
                                       fm_int     serDes)
{
    fm_status        err;
    fm10000_lane *   pLaneExt;
    fm10000_laneDfe *pLaneDfe;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;

    err = fm10000SerdesSpicoInt(sw,
                                serDes,
                                FM10000_SPICO_SERDES_INTR_0X26,
                                ((2 << 12) | ( 0 << 8) | (pLaneDfe->dfe_HF & 0xff)),
                                NULL);

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26,
                                    ((2 << 12) | ( 1 << 8) | (pLaneDfe->dfe_LF & 0xff)),
                                    NULL );
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26,
                                    ((2 << 12) | ( 2 << 8) | (pLaneDfe->dfe_DC & 0xff)),
                                    NULL );
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26,
                                    ((2 << 12) | ( 3 << 8) | (pLaneDfe->dfe_BW & 0xff)),
                                    NULL );
    }

    if (err == FM_OK &&
        (GET_PROPERTY()->dfeAllowEarlyLinkUp == TRUE))
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26,
                                    0x5b01,
                                    NULL );
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetEyeHeight
 * \ingroup intSerdes
 *
 * \desc            Returne an eye height metric for the given serdes.
 *                  This function return an estimation of both a eye score,
 *                  which range is [0..64] and the absolute height of the
 *                  eye in mV.
 *                  A score range [0..64] is used to keep backward compatibility.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pEyeScore points to the caller-allocated storage where this
 *                  function will return an eye metric estimation of the eye
 *                  height [0..64]. It may be NULL.
 *
 * \param[out]      pHeightmV points to the caller-allocated storage where this
 *                  function will return an estimation of the the eye height
 *                  in mV. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetEyeHeight(fm_int  sw,
                                    fm_int  serDes,
                                    fm_int *pEyeScore,
                                    fm_int *pHeightmV)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_OK;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->SerdesGetEyeHeight != NULL)
    {
        err = serdesPtr->SerdesGetEyeHeight(sw, serDes, pEyeScore, pHeightmV);
    }
    else
    {

        if (pEyeScore)
        {
            *pEyeScore = -1;
        }

        if (pHeightmV)
        {
            *pHeightmV = 0;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesGetEyeHeightnWidth
 * \ingroup intSerdes
 *
 * \desc            Return an eye height/width metric for the given serdes.
 *                  This function return an estimation of both a eye score,
 *                  which range is [0..64]
 *                  A score range [0..64] is used to keep backward compatibility.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pEyeHeight points to the caller-allocated storage where this
 *                  function will return an eye metric estimation of the eye
 *                  height [0..64]. It may be NULL.
 *
 * \param[out]      pEyeWidth points to the caller-allocated storage where this
 *                  function will return an eye metric estimation of the eye
 *                  width [0..64]. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesGetEyeHeightWidth(fm_int  sw,
                                         fm_int  serDes,
                                         fm_int *pEyeHeight,
                                         fm_int *pEyeWidth)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_OK;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->SerdesGetEyeScore != NULL)
    {
        err = serdesPtr->SerdesGetEyeScore(sw, serDes, pEyeHeight, pEyeWidth);
    }
    else
    {

        if (pEyeHeight)
        {
            *pEyeHeight = -1;
        }

        if (pEyeWidth)
        {
            *pEyeWidth = -1;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesGetEyeDiagram
 * \ingroup intSerdes
 *
 * \desc            Debug function that dumps the 8 most recent requests made
 *                  by remote. Most recent request first. It may be called only
 *                  after a successful KR training, otherwise it returns an
 *                  error.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pSampleTable is a pointer to a caller-allocated area where
 *                  this function will return FM10000_PORT_EYE_SAMPLES
 *                  eye sample points.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesGetEyeDiagram(fm_int                 sw,
                                     fm_int                 serDes,
                                     fm_eyeDiagramSample   *pSampleTable)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    fm_bool         signalOk;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->SerdesGetEyeDiagram != NULL)
    {
        err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

        if (!signalOk)
        {
            FM_LOG_PRINT("No active signal detected\n");
            return FM_OK;
        }
        err = serdesPtr->SerdesGetEyeDiagram(sw, serDes, pSampleTable);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDfeTuningReset
 * \ingroup intSerdes
 *
 * \desc            Reset DFE tuning state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesDfeTuningReset(fm_int     sw,
                                      fm_int     serDes)
{
    fm_status   err;
    fm_int      index;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    for (index=0; index < 12; index++)
    {
        err = fm10000SerdesSpicoInt(sw,
                                    serDes,
                                    FM10000_SPICO_SERDES_INTR_0X26,
                                    ((3 << 12) | ( index << 8)),
                                    NULL);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetupKrConfigClause92
 * \ingroup intSerdes
 *
 * \desc            Configures initial PRBS seed value according clause 92.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       relLane clause 92 lane ID.
 *
 * \param[out]      pSeed points to the caller-allocated storage where
 *                  this function will return the seed for the current lane.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetupKrConfigClause92(fm_int     sw,
                                             fm_int     serDes,
                                             fm_int     relLane,
                                             fm_uint32 *pSeed)
{
    fm_status   err;
    fm_uint32   krConfig;

    if (relLane > 3)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (*pSeed == 0)
        {
            switch(relLane)
            {
                case 3 :
                    {
                        *pSeed = 0x7b6;         ;
                        break;
                    }
                case 2 :
                    {
                        *pSeed = 0x72d;         ;
                        break;
                    }
                case 1 :
                    {
                        *pSeed = 0x645;        ;
                        break;
                    }
                case 0 :
                default:
                    {
                        *pSeed = 0x57e;        ;
                        break;
                    }
            }
        }

        krConfig = ((relLane) & 0x0fff) | (3 << 12);
        err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D , krConfig);

        if (err == FM_OK)
        {
            krConfig = ((*pSeed) & 0x0fff) | (4 << 12);
            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
        }

        if (err == FM_OK)
        {


            krConfig = (0x01) | (2 << 12);
            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetupKrConfigClause84
 * \ingroup intSerdes
 *
 * \desc            Configures initial PRBS seed value according clause 84.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       relLane clause 84 lane ID.
 *
 * \param[out]      pSeed points to the caller-allocated storage where
 *                  this function will return the seed for the current lane.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetupKrConfigClause84(fm_int     sw,
                                             fm_int     serDes,
                                             fm_int     relLane,
                                             fm_uint32 *pSeed)
{
    fm_status   err;
    fm_uint32   krConfig;

    if (relLane > 3)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (*pSeed == 0)
        {
            *pSeed = relLane + 2;
        }


        krConfig = (0x04 | (3 << 12));
        err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);

        if (err == FM_OK)
        {
            krConfig = ((*pSeed) & 0x0fff) | (4 << 12);
            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
        }

        if (err == FM_OK)
        {


            krConfig = (0x01) | (2 << 12);
            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSetupKrConfig
 * \ingroup intSerdes
 *
 * \desc            Configures several KR parameters such initial preCursor,
 *                  Cursor and postCursor, max equalization values, etc.
 *                  This function must be called when KR training is not
 *                  running otherwise it will fail.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pKrIsRunning points to the caller-allocated storage where
 *                  this function will indicate if KR is actively running.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetupKrConfig(fm_int       sw,
                                     fm_int       serDes,
                                     fm_bool     *pKrIsRunning)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm10000_lane   *pLaneExt;
    fm_laneAttr    *pLaneAttr;
    fm10000_laneKr *pLaneKr;
    fm_uint32       krConfig;
    fm_uint32       value;
    fm_bool         krIsRunning;


    switchExt = GET_SWITCH_EXT(sw);

    if ( !switchExt->serdesSupportsKR )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        pLaneExt    = GET_LANE_EXT(sw, serDes);
        pLaneAttr   = GET_LANE_ATTR(sw, serDes);

        pLaneKr     = &pLaneExt->krExt;
        krIsRunning = FALSE;
        err         = FM_OK;

        if (pKrIsRunning != NULL)
        {
            *pKrIsRunning = krIsRunning;
        }



        if (pLaneKr->resetParameters)
        {
            err = fm10000SerdesSpicoInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, 0, &value);
            if (err == FM_OK && value == 0)
            {
                krIsRunning = TRUE;
            }
        }

        if (pLaneKr->clause == FM10000_PMD_CONTROL_CLAUSE92)
        {
            err = fm10000SerdesSetupKrConfigClause92( sw, serDes, pLaneKr->relLane, &pLaneKr->seed);
        }
        else if (pLaneKr->clause == FM10000_PMD_CONTROL_CLAUSE84)
        {
            err = fm10000SerdesSetupKrConfigClause84(sw, serDes, pLaneKr->relLane, &pLaneKr->seed);
        }
        else if (pLaneKr->clause == FM10000_PMD_CONTROL_FV16G)
        {
            krConfig  = (1 << 12);
            krConfig |= (pLaneKr->opt_TT_FECreq) ? 0x01 : 0;
            krConfig |= (pLaneKr->opt_TT_TF)     ? 0x02 : 0;
            krConfig |= (pLaneKr->opt_TT_FECcap) ? 0x04 : 0;

            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);

        }

        if (err == FM_OK &&
            pLaneAttr->txLaneEnableConfigKrInit)
        {

            krConfig  = (0x09 << 12);
            krConfig |= pLaneAttr->initializePreCursor;

            FM_LOG_DEBUG2_V2( FM_LOG_CAT_SERDES,
                              serDes,
                              "Sw#%d serDes=%d, PMD config(0x%2.2x)=0x%4.4x\n",
                              sw,
                              serDes,
                              FM10000_SPICO_SERDES_INTR_0X3D,
                              krConfig);

            err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);

            if (err == FM_OK)
            {
                krConfig  = (0x0a << 12);
                krConfig |= pLaneAttr->initializeCursor;

                FM_LOG_DEBUG2_V2( FM_LOG_CAT_SERDES,
                                  serDes,
                                  "Sw#%d serDes=%d, PMD config(0x%2.2x)=0x%4.4x\n",
                                  sw,
                                  serDes,
                                  FM10000_SPICO_SERDES_INTR_0X3D,
                                  krConfig);

                err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
            }

            if (err == FM_OK)
            {
                krConfig  = (0x0b << 12);
                krConfig |= pLaneAttr->initializePostCursor;

                FM_LOG_DEBUG2_V2( FM_LOG_CAT_SERDES,
                                  serDes,
                                  "Sw#%d serDes=%d, PMD config(0x%2.2x)=0x%4.4x\n",
                                  sw,
                                  serDes,
                                  FM10000_SPICO_SERDES_INTR_0X3D,
                                  krConfig);

                err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
            }

            if (err == FM_OK)
            {
                krConfig  = (0x02 << 12);
                krConfig |= pLaneAttr->kr_xconfig2;

                FM_LOG_DEBUG2_V2( FM_LOG_CAT_SERDES,
                                  serDes,
                                  "Sw#%d serDes=%d, PMD config(0x%2.2x)=0x%4.4x\n",
                                  sw,
                                  serDes,
                                  FM10000_SPICO_SERDES_INTR_0X3D,
                                  krConfig);

                err = fm10000SerdesSpicoWrOnlyInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X3D, krConfig);
            }
        }

        if (err == FM_OK)
        {
            err = fm10000SerdesSpicoInt(sw,serDes, FM10000_SPICO_SERDES_INTR_0X26, 0x5c00, NULL);
        }


        if (pKrIsRunning != NULL)
        {
            *pKrIsRunning = krIsRunning;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000StartKrTraining
 * \ingroup intSerDes
 *
 * \desc            Action starting KR training on this SerDes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000StartKrTraining(fm_int  sw,
                                 fm_int  serDes)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm10000_lane   *pLaneExt;
    fm_laneAttr    *pLaneAttr;
    fm10000_laneKr *pLaneKr;
    fm_uint32       krCntrl;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d serdes=%d\n",
                     sw, serDes);

    switchExt = GET_SWITCH_EXT(sw);

    if ( !switchExt->serdesSupportsKR )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        pLaneExt  = GET_LANE_EXT(sw, serDes);
        pLaneAttr = GET_LANE_ATTR(sw, serDes);
        pLaneKr   = &pLaneExt->krExt;
        pLaneKr->eyeScoreHeight = 0;


        err = fm10000StopKrTraining(sw,serDes,TRUE);

        if (err == FM_OK)
        {

            err = fm10000SerdesSetupKrConfig(sw, serDes, NULL);
        }


        if (err == FM_OK)
        {
            krCntrl = 0;
            pLaneKr->krTrainingCtrlCnt = FM10000_SERDES_KR_TRAINING_MAX_WAIT;

            if (pLaneKr->invrtAdjPolarity)
            {
                FM_SET_BIT(krCntrl, FM10000_SPICO_SERDES_INTR_0X04, BIT_3, 1);
            }

            if (pLaneKr->disaTimeout)
            {
                FM_SET_BIT(krCntrl, FM10000_SPICO_SERDES_INTR_0X04, BIT_4, 1);
            }

            if (pLaneKr->disaTxEqAdjReq)
            {
                FM_SET_BIT(krCntrl, FM10000_SPICO_SERDES_INTR_0X04, BIT_5, 1);
            }

            if (pLaneAttr->txLaneEnableConfigKrInit)
            {
                pLaneAttr->preCursorDecOnPreset  &= 0x0f;
                pLaneAttr->postCursorDecOnPreset &= 0x0f;

                FM_SET_FIELD(krCntrl, FM10000_SPICO_SERDES_INTR_0X04, FIELD_2, pLaneAttr->preCursorDecOnPreset);
                FM_SET_FIELD(krCntrl, FM10000_SPICO_SERDES_INTR_0X04, FIELD_3, pLaneAttr->postCursorDecOnPreset);
                pLaneAttr->kr_xconfig1 &= 0xf8;
                krCntrl |= pLaneAttr->kr_xconfig1;
            }
        }

        if (err == FM_OK)
        {

            FM_SET_FIELD(krCntrl,
                         FM10000_SPICO_SERDES_INTR_0X04,
                         FIELD_1,
                         FM10000_SERDES_KR_OP_MODE_START_TRNG_ENABLD);

            FM_LOG_DEBUG2_V2( FM_LOG_CAT_SERDES,
                              serDes,
                              "Sw#%d serDes=%d, PMD ctrl(0x%2.2x)=0x%4.4x\n",
                              sw,
                              serDes,
                              FM10000_SPICO_SERDES_INTR_0X04,
                              krCntrl);

            err = fm10000SerdesSpicoWrOnlyInt(sw,
                                              serDes,
                                              FM10000_SPICO_SERDES_INTR_0X04,
                                              krCntrl);



            pLaneKr->refTimeMs = fm10000SerdesGetTimestampMs();
            fm10000SerdesIncrKrStatsCounter(sw,serDes,0);

        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000StopKrTraining
 * \ingroup intSerDes
 *
 * \desc            Action stopping the KR training on this SerDes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       waitSignalOk if TRUE, wait for signalOK after stopping
 *                  KR training, otherwise return inmediatly after that.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000StopKrTraining(fm_int  sw,
                                fm_int  serDes,
                                fm_bool waitSignalOk)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_int          waitSignalOkCnt;
    fm_bool         signalOk;
    fm_uint32       result;
    fm10000_lane   *pLaneExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d serdes=%d, waitSignalOk=%s\n",
                    sw,
                    serDes,
                    waitSignalOk? "TRUE" :"FALSE");

    switchExt = GET_SWITCH_EXT(sw);
    pLaneExt  = GET_LANE_EXT(sw, serDes);

    if ( !switchExt->serdesSupportsKR )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {

        err = fm10000SerdesSpicoIntSBusWrite(sw, serDes, FM10000_SPICO_SERDES_INTR_0X04, 0);
        if (err == FM_OK)
        {
            err = fm10000SerdesSpicoIntSBusReadFast(sw, serDes, &result);
        }

        if (err != FM_OK || result != FM10000_SPICO_SERDES_INTR_0X04)
        {

            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                            "Serdes=0x%2.2x: Cannot stop KR training\n",
                             serDes);

            err = fm10000SerdesResetSpico(sw, serDes);

            waitSignalOk = FALSE;

            if (pLaneExt->fResetCnt < 0xffffffff)
            {
                pLaneExt->fResetCnt++;
            }
        }

        if (waitSignalOk == TRUE)
        {

            waitSignalOkCnt = 0;
            do
            {

                err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

                if (err != FM_OK || signalOk == TRUE)
                {
                    break;
                }


                fmDelay(0, FM10000_SERDES_KR_WAIT_SIGNAL_OK_CYCLE_DELAY);

            } while (++waitSignalOkCnt < FM10000_SERDES_KR_WAIT_SIGNAL_OK_MAX_CYCLES);

            if (err == FM_OK && waitSignalOkCnt >= FM10000_SERDES_KR_WAIT_SIGNAL_OK_MAX_CYCLES)
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Timeout waiting for signalOk after stopping KR\n");
                err = FM_FAIL;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SetKrPcalMode
 * \ingroup intSerDes
 *
 * \desc            Set the pCal mode to be run after KR is complete
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       dfeMode is the DFE mode, which is used to set the KR
 *                  pCal mode.
 *
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetKrPcalMode(fm_int  sw,
                               fm_int  serDes,
                               fm_int  dfeMode)
{
    fm_status       err;
    fm10000_lane   *pLaneExt;
    fm10000_switch *switchExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d serdes=%d\n",
                     sw, serDes);

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    if ( switchExt->serdesSupportsKR )
    {
        pLaneExt = GET_LANE_EXT(sw, serDes);

        switch ( dfeMode )
        {
            case FM_DFE_MODE_STATIC:
            case FM_DFE_MODE_ICAL_ONLY:
                pLaneExt->krExt.pCalMode = FM10000_SERDES_KR_PCAL_MODE_STATIC;
                break;
            case FM_DFE_MODE_CONTINUOUS:
                pLaneExt->krExt.pCalMode = FM10000_SERDES_KR_PCAL_MODE_CONTINUOUS;
                break;
            case FM_DFE_MODE_ONE_SHOT:
                pLaneExt->krExt.pCalMode = FM10000_SERDES_KR_PCAL_MODE_ONE_SHOT;
                break;
            default:
                err = FM_ERR_INVALID_ARGUMENT;
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesConfigureEeeInt
 * \ingroup intSerdes
 *
 * \desc            Enable EEE support at serdes level.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid value for the Serdes type
 *
 *****************************************************************************/
fm_status fm10000SerdesConfigureEeeInt(fm_int sw, fm_int serDes)
{
    fm_status err;
    fm_uint32 data;

    VALIDATE_SWITCH_INDEX(sw);
    VALIDATE_SERDES(serDes);

    err = FM_OK;


    if (GET_FM10000_PROPERTY()->enableEeeSpicoIntr == TRUE)
    {
        data = 0;
        FM_SET_BIT(data, FM10000_SPICO_SERDES_INTR_0X27, BIT_1, 1);

        err = fm10000SerdesSpicoWrOnlyInt(sw,
                                          serDes,
                                          FM10000_SPICO_SERDES_INTR_0X27,
                                          data);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetCapturedData
 * \ingroup intSerdes
 *
 * \desc            Get a 80 bit data sample from the specified serdes and
 *                  determine the number of level transitions ('0' to '1' and
 *                  vice-versa) in the sample.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes ID.
 *
 * \param[in]       pData10Bit points to the caller-allocated array of 10-bit
 *                  chunks of data (fm_uint32[8]) where this function will copy
 *                  the capture data. It may NULL.
 *
 * \param[in]       pNumTransitions points to the caller-allocated
 *                  storage where this function will place the number of
 *                  transitions in the captured data. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetCapturedData(fm_int       sw,
                                       fm_int       serDes,
                                       fm_uint32   *pData10Bit,
                                       fm_uint32   *pNumTransitions)
{
    fm_status   err;
    fm_int      cnt;
    fm_uint32   data10bit;
    fm_int      tr_cnt;
    fm_int      iloop;
    fm_uint32   lastsample;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pData10Bit=%p, pNumTransitions=%p\n",
                    sw,
                    serDes,
                    (void*) pData10Bit,
                    (void*) pNumTransitions);


    err = fm10000SerdesSpicoWrOnlyInt(sw,
                                      serDes,
                                      FM10000_SPICO_SERDES_INTR_0X1C,
                                      0);

    if (err == FM_OK)
    {
        err = fm10000SerdesSpicoWrOnlyInt(sw,
                                          serDes,
                                          FM10000_SPICO_SERDES_INTR_0X18,
                                          0x04);
    }

    if (err == FM_OK)
    {
        tr_cnt = 0;
        lastsample = 0;

        for (cnt = 0 ; cnt < 8; cnt++)
        {
            err = fm10000SerdesSpicoInt(sw,
                                        serDes,
                                        FM10000_SPICO_SERDES_INTR_0X1A,
                                        0,
                                        &data10bit);
            if (err == FM_OK)
            {
                if (pData10Bit)
                {
                    *pData10Bit++ = data10bit;
                }

                if (pNumTransitions != NULL)
                {
                    if (cnt == 0)
                    {
                        lastsample = data10bit & 0x01;
                    }

                    for (iloop = 0; iloop <10; iloop++)
                    {
                        if ( (data10bit ^ lastsample) & 0x01)
                        {
                            tr_cnt++;
                        }
                        lastsample = data10bit & 0x01;
                        data10bit >>= 1;
                    }
                }
            }
            else
            {
                break;
            }
        }

        if (pNumTransitions != NULL)
        {
            *pNumTransitions = tr_cnt;
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesValidateSignal
 * \ingroup intSerdes
 *
 * \desc            Validate a 80 bit data sample of the incoming signal
 *                  determining if the number of level transitions ('0' to '1'
 *                  and vice-versa) is big enough to perform DFE tuning on such
 *                  signal. Validation may be disabled setting the transition-
 *                  threshold to zero (0).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes ID.
 *
 * \param[in]       validSignal points to the caller-allocated storage that
 *                  this function will set to TRUE if the number of transitions
 *                  if bigger or equal than the threshold, or FALSE otherwise.
 *                  If the threshlod is zero (0), the signal validation is
 *                  disabled and valid signal is set to TRUE. Finally, if
 *                  a problem happens capturing data, validation is set to
 *                  TRUE.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesValidateSignal(fm_int              sw,
                                      fm_int              serDes,
                                      fm_bool            *validSignal)
{
    fm_status       err;
    fm_uint32       transitions;
    fm_int          transition_threshold;
    fm_laneAttr    *pLaneAttr;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, validSignal=%p\n",
                    sw,
                    serDes,
                    (void*) validSignal);

    err = FM_OK;

    if (validSignal)
    {
        *validSignal = TRUE;

        pLaneAttr = GET_LANE_ATTR( sw, serDes );
        transition_threshold = pLaneAttr->transitionThreshold;


        if (transition_threshold >= 0 &&  transition_threshold <= 70)
        {

            if (transition_threshold > 0)
            {
                err = fm10000SerdesGetCapturedData(sw, serDes, NULL, &transitions);

                if (err == FM_OK)
                {
                    if ((fm_int)transitions < transition_threshold)
                    {
                        *validSignal = FALSE;
                    }
                    FM_LOG_DEBUG2_V2(FM_LOG_CAT_SERDES,serDes,
                                     "Serdes=0x%2.2x: signal transitions=%2.2d, valid signal=%s\n",
                                     serDes,
                                     (fm_int)transitions,
                                     (*validSignal) ? "TRUE" : "FALSE");
                }
                else
                {


                    FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                                    "Serdes=0x%2.2x: cannot get captured data\n",
                                    serDes);
                    err = FM_OK;
                }
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,serDes,
                            "Serdes=0x%2.2x: Invalid transition threshold value=%d\n",
                            serDes,
                            transition_threshold);

            pLaneAttr->transitionThreshold = 0;
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesConfigDfeParam
 * \ingroup intSerdes
 *
 * \desc            Configures additional parameters before performing DFE
 *                  tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes ID.
 *
 * \param[in]       paramSelector is the selector of the parameter to be
 *                  configured
 *
 * \param[in]       paramValue is the value of the parameter to be set.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesConfigDfeParam(fm_int    sw,
                                      fm_int    serDes,
                                      fm_int    paramSelector,
                                      fm_int    paramValue)
{
    fm_status   err;


    err = FM_OK;

    if (paramSelector == 0)
    {
        if ( (curSerdesCodeVersionBuildId & 0xffff0000) >= 0x10550000 && paramValue)
        {

            err = fm10000SerdesSpicoWrOnlyInt(sw,
                                              serDes,
                                              FM10000_SPICO_SERDES_INTR_0X18,
                                              0x07);

            if (err == FM_OK)
            {

                err = fm10000SerdesSpicoWrOnlyInt(sw,
                                                  serDes,
                                                  FM10000_SPICO_SERDES_INTR_0X19,
                                                  paramValue & 0xffff);
                if (err == FM_OK)
                {

                    err = fm10000SerdesSpicoWrOnlyInt(sw,
                                                      serDes,
                                                      FM10000_SPICO_SERDES_INTR_0X19,
                                                      paramValue>>16);
                }
            }
        }
    }

    return err;

}

