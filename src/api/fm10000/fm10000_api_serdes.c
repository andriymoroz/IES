/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_serdes.c
 * Creation Date:   October 17, 2013
 * Description:     File containing a number of utilities functions to manage
 *                  the FM10000 PCIe and Ethernet Serdes
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


#define PEP_OFFSET_X8  0x8
#define PEP_OFFSET_X4  0x4

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_status fm10000SerdesSpicoSaiInt(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint    intNum,
                                          fm_uint32  param,
                                          fm_uint32 *pResult);
static fm_status fm10000SerdesInitGenericOptions(fm_int sw);
static fm_status fm10000SerdesInitXServices(fm_int sw);


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

fm_status __attribute__((weak)) fm10000SerdesInitXServicesExt(fm_int sw);


fm_text fm10000LaneBitRatesMap[FM10000_LANE_BITRATE_MAX] =
{
    "25 Gbps",
    "10 Gbps",
    "6.25 Gbps",
    "2.5 Gbps",
    "1.0 Gbps",
    "100 Mbps",
    "10 Mbps"
};


fm10000_serdesMap fm10000SerdesMap[FM10000_NUM_SERDES] =
{

    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x00,     0,          { 0 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x01,     1,          { 0 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x02,     2,          { 0 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x03,     3,          { 0 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x04,     4,          { 1 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x05,     5,          { 1 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x06,     6,          { 1 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x07,     7,          { 1 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x08,     8,          { 2 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x09,     9,          { 2 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0a,     10,         { 2 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0b,     11,         { 2 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0c,     12,         { 3 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0d,     13,         { 3 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0e,     14,         { 3 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x0f,     15,         { 3 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x10,     16,         { 4 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x11,     17,         { 4 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x12,     18,         { 4 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x13,     19,         { 4 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x14,     20,         { 5 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x15,     21,         { 5 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x16,     22,         { 5 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x17,     23,         { 5 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x18,     24,         { 6 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x19,     25,         { 6 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1a,     26,         { 6 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1b,     27,         { 6 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1c,     28,         { 7 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1d,     29,         { 7 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1e,     30,         { 7 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x1f,     31,         { 7 },    3    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x20,     32,         { 8 },    0    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x21,     33,         { 8 },    1    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x22,     34,         { 8 },    2    },
    {  FM10000_SERDES_RING_EPL,  FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR + 0x23,     35,         { 8 },    3    },


    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x00,    36,         { 0 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x02,    36,         { 0 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x04,    36,         { 0 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x06,    36,         { 0 },    3    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x08,    38,         { 1 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x0a,    38,         { 1 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x0c,    38,         { 1 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x0e,    38,         { 1 },    3    },

    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x10,    40,         { 2 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x12,    40,         { 2 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x14,    40,         { 2 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x16,    40,         { 2 },    3    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x18,    42,         { 3 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x1a,    42,         { 3 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x1c,    42,         { 3 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x1e,    42,         { 3 },    3    },

    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x20,    44,         { 4 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x22,    44,         { 4 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x24,    44,         { 4 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x26,    44,         { 4 },    3    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x28,    46,         { 5 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x2a,    46,         { 5 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x2c,    46,         { 5 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x2e,    46,         { 5 },    3    },

    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x30,    48,         { 6 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x32,    48,         { 6 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x34,    48,         { 6 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x36,    48,         { 6 },    3    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x38,    50,         { 7 },    0    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x3a,    50,         { 7 },    1    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x3c,    50,         { 7 },    2    },
    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x3e,    50,         { 7 },    3    },

    {  FM10000_SERDES_RING_PCIE, FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR + 0x40,    63,         { 8 },    0    },
};

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_text widthModeStr[] =
{
    "10b",
    "20b",
    "40b"
};


static fm_text dfeCtleTypeStr[] =
{
    "SERDES_DFE_CTLE_HF",
    "SERDES_DFE_CTLE_LF",
    "SERDES_DFE_CTLE_DC",
    "SERDES_DFE_CTLE_BW",
    "SERDES_DFE_CTLE_LB",
    "SERDES_DFE_CTLE_MAX"
};




static fm_int serdesSaiDebug = 0;


static fm_int serdesPcieDebug = 0;


static fm_bool eplUseSbusIntf  = FALSE;
static fm_bool pcieUseSbusIntf = FALSE;

/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** GetSerdesValidId
 * \ingroup intSerdes
 *
 * \desc            Get SERDES valid ID status.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          TRUE if serDes has a valid ID code.
 *
 *****************************************************************************/
static fm_bool GetSerdesValidId(fm_int sw,
                                fm_int serDes)
{
    fm_status       err;
    fm_bool         validIdCode;


    validIdCode = FALSE;
    err  = fm10000SerdesCheckId(sw, serDes, &validIdCode);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Cannot read Id for serdes=%d\n", serDes);
    }

    return validIdCode;

}




/*****************************************************************************/
/** fm10000SerdesSpicoSaiInt
 * \ingroup intSerdes
 *
 * \desc            Perform a fast SERDES SPICO interrupt access using
 *                  LANE_SAI_CFG and LANE_SAI_STATUS registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \param[out]      pResult points to the caller-allocated storage where this
 *                  function will place the result. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoSaiInt(fm_int     sw,
                                   fm_int     serDes,
                                   fm_uint    intNum,
                                   fm_uint32  param,
                                   fm_uint32 *pResult)
{
    fm_status     err;
    fm_switch    *switchPtr;
    fm_uint       laneSaiCfgRegAddr;
    fm_uint       laneSaiStatusRegAddr;
    fm_uint64     laneSaiCfg;
    fm_uint32     laneSaiStatus;
    fm_int        epl;
    fm_int        lane;
    fm_uint       sbusAddr;
    fm_serdesRing ring;
    fm_timestamp  start;
    fm_timestamp  end;
    fm_timestamp  diff;
    fm_uint       delTime;
    fm_uint       loopCnt;

    switchPtr = GET_SWITCH_PTR(sw);
    sbusAddr  = 0;
    ring      = 0;


    if (pResult != NULL)
    {
        *pResult = 0;
    }


    err = fm10000MapSerdesToEplLane( sw, serDes, &epl, &lane);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    laneSaiCfgRegAddr = FM10000_LANE_SAI_CFG(epl, lane, 0);
    laneSaiStatusRegAddr = FM10000_LANE_SAI_STATUS(epl, lane);


    delTime = 0;
    loopCnt = 0;
    fmGetTime(&start);

    while (delTime < FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        loopCnt++;
        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        err = switchPtr->ReadUINT32(sw, laneSaiStatusRegAddr, &laneSaiStatus);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

        if (!FM_GET_BIT(laneSaiStatus, FM10000_LANE_SAI_STATUS, Busy) )
        {

            break;
        }
        else
        {




            if (delTime > FM10000_SERDES_SAI_ACCESS_DLY_THRESH)
            {
                fmDelay(0, loopCnt * FM10000_SERDES_SAI_ACCESS_LOOP_DELAY);
            }
        }
    }

    if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = FM_FAIL;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt still busy in %u msec %u interations, "
                     "serdes %d. LANE_SAI_STATUS = 0x%8.8x (int=0x%2.2x, param=0x%4.4x)\n",
                     delTime,
                     loopCnt,
                     serDes,
                     laneSaiStatus,
                     intNum,
                     param);

        fm10000VerifySwitchAliveStatus(sw);
        FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);
    }

    laneSaiCfg = 0;
    err = switchPtr->WriteUINT64(sw, laneSaiCfgRegAddr, laneSaiCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);


    FM_SET_FIELD64(laneSaiCfg, FM10000_LANE_SAI_CFG, Code, intNum);
    FM_SET_FIELD64(laneSaiCfg, FM10000_LANE_SAI_CFG, Data, param);

    FM_SET_FIELD64(laneSaiCfg, FM10000_LANE_SAI_CFG, ResultMode, 1);
    FM_SET_BIT64(laneSaiCfg, FM10000_LANE_SAI_CFG, Request, 1);


    err = switchPtr->WriteUINT64(sw, laneSaiCfgRegAddr, laneSaiCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    if (serdesSaiDebug)
    {
        fm10000MapSerdesToSbus(sw, serDes, &sbusAddr, &ring);


        FM_LOG_PRINT("sw=%d addr=0x%2.2x reg=0x03 <= 0x%8.8x  t=%4.4d.%3.3d (SAI)\n",
                     sw, sbusAddr, (intNum<<16)|(param& 0xffff), (fm_int)(start.sec%10000), (fm_int)(start.usec/1000));
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
        err = switchPtr->ReadUINT32(sw, laneSaiStatusRegAddr, &laneSaiStatus);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

        if (FM_GET_BIT(laneSaiStatus,FM10000_LANE_SAI_STATUS,Complete))
        {

            break;
        }
        else
        {


            if (delTime > FM10000_SERDES_SAI_ACCESS_DLY_THRESH)
            {
                if (delTime > 1000)
                {

                    err = fm10000VerifySwitchAliveStatus(sw);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);
                }
                fmDelay(0, loopCnt * FM10000_SERDES_SAI_ACCESS_LOOP_DELAY);
            }
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_SERDES,
                 "Serdes %d Interrupt 0x%x param=0x%4.4x done in %llu usec %u interations\n",
                 serDes, intNum, param, diff.sec*1000000 + diff.usec, loopCnt );

    if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = FM_FAIL;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt 0x%x param=0x%4.4x timeout %u msec %u interations, "
                     "serdes %d. LANE_SAI_STATUS = 0x%8.8x\n",
                     intNum, param, delTime, loopCnt, serDes, laneSaiStatus);
        fm10000VerifySwitchAliveStatus(sw);
    }
    else if (err == FM_OK)
    {
        if (pResult != NULL)
        {
            *pResult = laneSaiStatus & 0x0000ffff;
        }

        if (serdesSaiDebug)
        {

            FM_LOG_PRINT("sw=%d addr=0x%2.2x reg=0x04 => 0x%8.8x  t=%4.4d.%3.3d (SAI)\n",
                         sw, sbusAddr, laneSaiStatus & 0x0000ffff,
                         (fm_int)(start.sec%10000), (fm_int)(start.usec/1000));
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/** fm10000SerdesPcieSpicoInt
 * \ingroup intSerdes
 *
 * \desc            Perform a fast SERDES SPICO interrupt access using
 *                  PCIE_SERDES_CTRL registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \param[out]      pResult points to the caller-allocated storage where this
 *                  function will place the result. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesPcieSpicoInt(fm_int     sw,
                                    fm_int     serDes,
                                    fm_uint    intNum,
                                    fm_uint32  param,
                                    fm_uint32 *pResult)
{
    fm_status     err;
    fm_switch    *switchPtr;
    fm_uint64     val;
    fm_uint       addr;
    fm_int        pep;
    fm_int        lane;
    fm_uint       sbusAddr;
    fm_serdesRing ring;
    fm_timestamp  start;
    fm_timestamp  end;
    fm_timestamp  diff;
    fm_uint       delTime;
    fm_uint       loopCnt;


    switchPtr = GET_SWITCH_PTR(sw);

    if (pResult != NULL)
    {
        *pResult = 0;
    }


    err = fm10000MapSerdesToPepLane( sw, serDes, &pep, &lane);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);


    if (pep & 1)
    {
        lane = lane + 4;
    }


    val = 0;
    addr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_SERDES_CTRL(lane, 0), pep);
    FM_SET_FIELD64(val, FM10000_PCIE_SERDES_CTRL, DataWrite, param);


    err = switchPtr->WriteUINT64(sw, addr, val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);



    FM_SET_FIELD64(val, FM10000_PCIE_SERDES_CTRL, InterruptCode, intNum);
    FM_SET_BIT(val, FM10000_PCIE_SERDES_CTRL, Interrupt, 1);
    err = switchPtr->WriteUINT64(sw, addr, val);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

    if (serdesPcieDebug)
    {
        fm10000MapSerdesToSbus(sw, serDes, &sbusAddr, &ring);


        FM_LOG_PRINT("sw=%d pep %d.%d addr=0x%2.2x PCIE_SERDES_CTRL "
                     "InterruptCode=0x%4.4x DataWrite=0x%4.4x\n",
                     sw, pep, lane, sbusAddr, (intNum & 0xFFFF), (param & 0xFFFF));
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
        err = switchPtr->ReadUINT64(sw, addr, &val);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);

        if (!FM_GET_BIT(val, FM10000_PCIE_SERDES_CTRL, InProgess))
        {
            break;
        }
        else
        {


            if (delTime > FM10000_SERDES_SAI_ACCESS_DLY_THRESH)
            {
                if (delTime > 1000)
                {

                    err = fm10000VerifySwitchAliveStatus(sw);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, err);
                }
                fmDelay(0, loopCnt * FM10000_SERDES_SAI_ACCESS_LOOP_DELAY);
            }
        }
    }

    if (serdesPcieDebug)
    {
        FM_LOG_PRINT("Serdes %d Interrupt 0x%x param=0x%4.4x done in %llu usec %u interations\n",
                      serDes, intNum, param, diff.sec*1000000 + diff.usec, loopCnt );
    }

    if (delTime >= FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC)
    {
        err = FM_FAIL;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Serdes Interrupt 0x%x timeout %u msec %u iterations, "
                     "serdes %d. PCIE_SERDES_CTRL = 0x%llx\n",
                     intNum, delTime, loopCnt, serDes, val);
        fm10000VerifySwitchAliveStatus(sw);
    }
    else if (err == FM_OK)
    {
        if (pResult != NULL)
        {
            *pResult = FM_GET_FIELD64(val, FM10000_PCIE_SERDES_CTRL, DataRead);
        }

        if (serdesPcieDebug)
        {

            FM_LOG_PRINT("sw=%d addr=0x%2.2x PCIE_SERDES_CTRL DataRead=0x%4.4x t=%u\n",
                         sw, sbusAddr,
                         (fm_uint16)((FM_GET_FIELD64(val, FM10000_PCIE_SERDES_CTRL, DataRead) & 0xFFFF)),
                         delTime);
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/**  fm10000SerdesInitXServices
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
static fm_status fm10000SerdesInitXServices(fm_int sw)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    fm_status        (*initServices)(fm_int sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    FM_CLEAR(*serdesPtr);
    serdesPtr->magicNumber = FM10000_SERDES_STRUCT_MAGIG_NUMBER;

    err = fm10000SerdesInitXServicesInt(sw);

    if (err == FM_OK)
    {

        err = fm10000SerdesInitXDebugServicesInt(sw);
    }

    if (err == FM_OK)
    {
        initServices = fm10000SerdesInitXServicesExt;
        if (initServices)
        {
            err = (*initServices)(sw);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/**  fm10000SerdesInitGenericOptions
 * \ingroup intSerdes
 *
 * \desc            Performs the initialization of several serdes general
 *                  options.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status fm10000SerdesInitGenericOptions(fm_int sw)
{
    fm_status       err;
    fm10000_switch *switchExt;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/**  fm10000SerdesGetPepId
 * \ingroup intSerdes
 *
 * \desc            Return the pepId given the serdes number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \return          PEP number based on x4/x8 mode.
 *
 *****************************************************************************/
static fm_int fm10000SerdesGetPepId(fm_int sw, fm_int serdes)
{
    fm_switch   *switchPtr;
    fm_laneAttr *laneAttr;


    if ((fm10000SerdesMap[serdes].endpoint.pep & 1) == 0)
    {
        return (fm10000SerdesMap[serdes].endpoint.pep);
    }



    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr->laneTableSize == 0)
    {

        FM_LOG_FATAL(FM_LOG_CAT_SERDES,
                    "Switch %d lane table has not been initialized\n",
                    sw);
        return (fm10000SerdesMap[serdes].endpoint.pep);
    }

    laneAttr = GET_LANE_ATTR(sw, serdes);
    if (laneAttr->pepOffset == 0)
    {

        fm10000SerdesInitMappingTable(sw);
    }

    switch (laneAttr->pepOffset)
    {
        case PEP_OFFSET_X8:
            if (fm10000SerdesMap[serdes].endpoint.pep & 1)
            {

                return (fm10000SerdesMap[serdes].endpoint.pep - 1);
            }
            return (fm10000SerdesMap[serdes].endpoint.pep);
        case PEP_OFFSET_X4:
           return (fm10000SerdesMap[serdes].endpoint.pep);
        default:
            FM_LOG_FATAL(FM_LOG_CAT_SERDES,
                        "Switch %d Serdes %d PEP offset has not been initialized\n",
                        sw, serdes);
            return (fm10000SerdesMap[serdes].endpoint.pep);
    }

}




/*****************************************************************************/
/**  fm10000SerdesGetPepFromMap
 * \ingroup intSerdes
 *
 * \desc            Reads the PEP number assigned to the serDes
 *
 * \param[in]       serDes is the serDes number.
 *
 * \return          PEP number from the serDes map
 *
 *****************************************************************************/
fm_int fm10000SerdesGetPepFromMap(fm_int serDes)
{
    return (fm10000SerdesMap[serDes].endpoint.pep);

}




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/**  fm10000SerdesInitMappingTable
 * \ingroup intSerdes
 *
 * \desc            Performs the initialization of the mapping table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitMappingTable(fm_int sw)
{
    fm_status       err;
    fm_switch      *switchPtr;
    fm_uint32       deviceCfg;
    fm_uint32       pcieMode;
    fm_int          serdes;
    fm_int          startSerdes;
    fm_int          pep;
    fm_bool         x8;
    fm_laneAttr *   laneAttr;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR( sw );

    err = switchPtr->ReadUINT32( sw, FM10000_DEVICE_CFG(), &deviceCfg );
    FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SERDES, err );

    pcieMode = FM_GET_FIELD(deviceCfg, FM10000_DEVICE_CFG, PCIeMode );

    for (serdes = 36; serdes <= 68 ; serdes++)
    {
        laneAttr = GET_LANE_ATTR(sw, serdes);
        laneAttr->pepOffset = PEP_OFFSET_X8;
    }

    for (pep = 1; pep <= 7; pep = pep + 2)
    {
        startSerdes = 40 + (pep - 1)*4;
        if ( ( pcieMode & ( 1 << (pep/2) ) ) != 0 )
        {

            x8 = FALSE;
        }
        else
        {

            x8 = TRUE;
        }

        for (serdes = startSerdes; serdes < (startSerdes + 4); serdes++)
        {
            laneAttr = GET_LANE_ATTR(sw, serdes);
            laneAttr->pepOffset = x8?PEP_OFFSET_X8:PEP_OFFSET_X4;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/** fm10000SerdesCheckIfIsActive
 * \ingroup intSerdes
 *
 * \desc            Check to see if port is mapped to an active SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          TRUE if serDes has a valid ID code.
 *
 *****************************************************************************/
fm_bool fm10000SerdesCheckIfIsActive(fm_int sw,
                                     fm_int serDes)
{
    fm_status       err;
    fm10000_lane   *pLaneExt;
    fm_bool         validIdCode;

    validIdCode = FALSE;
    err = FM_OK;

    if ( serDes > 0 && serDes < FM10000_NUM_SERDES )
    {
        if (GET_SWITCH_PTR(sw) == NULL ||
            GET_SWITCH_PTR(sw)->laneTable == NULL)
        {
            err  = fm10000SerdesCheckId(sw, serDes, &validIdCode);
        }
        else
        {
            pLaneExt  = GET_LANE_EXT(sw, serDes);

            if (pLaneExt == NULL)
            {
                err  = fm10000SerdesCheckId(sw, serDes, &validIdCode);
            }
            else
            {
                validIdCode = pLaneExt->serDesActive;
            }
        }

        if (err != FM_OK)
        {
            validIdCode = FALSE;
        }
    }

    return validIdCode;

}




/*****************************************************************************/
/** fm10000SerDesSaiSetDebug
 * \ingroup intSBus
 *
 * \desc            Set debug flag to log SAI transactions. SBus transactions
 *                  are also logged.
 *
 * \param[in]       debug at SAI serdes access level.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fm10000SerDesSaiSetDebug(fm_int debug)
{

    if (debug == 0xFF)
    {
        FM_LOG_PRINT("Force EPL to use SAI. Previous = %s.\n", eplUseSbusIntf?"SBUS":"SAI");
        eplUseSbusIntf = FALSE;
        return;
    }
    if (debug == 0xFE)
    {
        FM_LOG_PRINT("Force EPL to use SBUS. Previous = %s.\n", eplUseSbusIntf?"SBUS":"SAI");
        eplUseSbusIntf = TRUE;
        return;
    }

    serdesSaiDebug = debug;


    fm10000SbusSetDebug(debug);

}




/*****************************************************************************/
/** fm10000SerDesPcieSetDebug
 * \ingroup intSBus
 *
 * \desc            Set debug flag to log PCIe transactions. SBus transactions
 *                  are also logged.
 *
 * \param[in]       debug at PCIe serdes access level.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fm10000SerDesPcieSetDebug(fm_int debug)
{
    if (debug == 0xFF)
    {
        FM_LOG_PRINT("Force PCIE to use Parallel interface. Previous = %s.\n",
                     pcieUseSbusIntf?"SBUS":"PARALLEL");
        pcieUseSbusIntf = FALSE;
        return;
    }
    if (debug == 0xFE)
    {
        FM_LOG_PRINT("Force PCIE to use SBUS. Previous = %s.\n",
                      pcieUseSbusIntf?"SBUS":"PARALLEL");
        pcieUseSbusIntf = TRUE;
        return;
    }

    serdesPcieDebug = debug;


    fm10000SbusSetDebug(debug);

}




/*****************************************************************************/
/**  fm10000LoadSpicoCode
 * \ingroup intSerdes
 *
 * \desc            Load SPICO image to the SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000LoadSpicoCode(fm_int sw)
{
    fm_status       err;
    fm_int          useProductionSpicoCodeVersions;
    fm_int          imageOption;
    fm_int          serdesImageSelector;
    fm10000_switch *switchExt;
    fm_uint16      *pSbmCodeImage;
    fm_uint32       sbmCodeSize;
    fm_uint32       sbmCodeVersionBuildId;
    fm_uint16      *pSerdesCodeImage;
    fm_uint32       serdesCodeSize;
    fm_uint32       serdesCodeVersionBuildId;
    fm_uint16      *pSwapCodeImage;
    fm_uint32       swapCodeSize;
    fm_uint32       swapCodeVersionBuildId;
    fm_int          swapCrcCode;
    fm_int          firstSerdes;
    fm_int          lastSerdes;
    fm_serDesOpMode serdesOpMode;



    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    err = FM_OK;

    switchExt = GET_SWITCH_EXT(sw);


    err = fm10000SerdesGetOpMode(sw, 0, &serdesOpMode, NULL, NULL);

    useProductionSpicoCodeVersions = (serdesOpMode == FM_SERDES_OPMODE_TEST_BOARD) ? 1 : 0;


    imageOption = FM10000_SERDES_DEFAULT_SPICO_FW;

    if (imageOption < 0 || imageOption > 1)
    {
        imageOption = 0;
    }


    if (GET_FM10000_PROPERTY()->useAlternateSpicoFw)
    {

        imageOption ^= 1;
    }

    serdesImageSelector = (useProductionSpicoCodeVersions << 1) | imageOption;


    swapCodeSize = 0;
    pSwapCodeImage = NULL;
    serdesCodeVersionBuildId = 0;
    serdesCodeSize = 0;
    pSerdesCodeImage = NULL;
    sbmCodeVersionBuildId = 0;

    switch (serdesImageSelector)
    {
        case 0:
            pSbmCodeImage            = (fm_uint16 *) fm10000_sbus_master_code_prd;
            sbmCodeSize              = fm10000_sbus_master_code_size_prd;
            sbmCodeVersionBuildId    = fm10000_sbus_master_code_versionBuildId_prd;

            pSerdesCodeImage         = (fm_uint16 *) fm10000_serdes_spico_code_prd1;
            serdesCodeSize           = fm10000_serdes_spico_code_size_prd1;
            serdesCodeVersionBuildId = fm10000_serdes_spico_code_versionBuildId_prd1;
            firstSerdes              = 0;
            lastSerdes               = FM10000_EPL_RING_SERDES_NUM-1;

            pSwapCodeImage           = (fm_uint16 *) fm10000_serdes_swap_code_prd1;
            swapCodeSize             = fm10000_serdes_swap_code_size_prd1;
            swapCodeVersionBuildId   = fm10000_serdes_swap_code_versionBuildId_prd1;
            break;
        case 1:
            pSbmCodeImage            = (fm_uint16 *) fm10000_sbus_master_code_prd;
            sbmCodeSize              = fm10000_sbus_master_code_size_prd;
            sbmCodeVersionBuildId    = fm10000_sbus_master_code_versionBuildId_prd;

            pSerdesCodeImage         = (fm_uint16 *) fm10000_serdes_spico_code_prd2;
            serdesCodeSize           = fm10000_serdes_spico_code_size_prd2;
            serdesCodeVersionBuildId = fm10000_serdes_spico_code_versionBuildId_prd2;
            firstSerdes              = 0;
            lastSerdes               = FM10000_EPL_RING_SERDES_NUM-1;

            pSwapCodeImage           = (fm_uint16 *) fm10000_serdes_swap_code_prd2;
            swapCodeSize             = fm10000_serdes_swap_code_size_prd2;
            swapCodeVersionBuildId   = fm10000_serdes_swap_code_versionBuildId_prd2;
            break;
        case 2:
            pSbmCodeImage            = (fm_uint16 *) fm10000_sbus_master_code_dev;
            sbmCodeSize              = fm10000_sbus_master_code_size_dev;
            sbmCodeVersionBuildId    = fm10000_sbus_master_code_versionBuildId_dev;

            pSerdesCodeImage         = (fm_uint16 *) fm10000_serdes_spico_code_dev1;
            serdesCodeSize           = fm10000_serdes_spico_code_size_dev1;
            serdesCodeVersionBuildId = fm10000_serdes_spico_code_versionBuildId_dev1;
            firstSerdes              = 0;
            lastSerdes               = FM10000_EPL_RING_SERDES_NUM-1;

            pSwapCodeImage           = (fm_uint16 *) fm10000_serdes_swap_code_dev1;
            swapCodeSize             = fm10000_serdes_swap_code_size_dev1;
            swapCodeVersionBuildId   = fm10000_serdes_swap_code_versionBuildId_dev1;
            break;
        case 3:
            pSbmCodeImage            = (fm_uint16 *) fm10000_sbus_master_code_dev;
            sbmCodeSize              = fm10000_sbus_master_code_size_dev;
            sbmCodeVersionBuildId    = fm10000_sbus_master_code_versionBuildId_dev;

            pSerdesCodeImage         = (fm_uint16 *) fm10000_serdes_spico_code_dev2;
            serdesCodeSize           = fm10000_serdes_spico_code_size_dev2;
            serdesCodeVersionBuildId = fm10000_serdes_spico_code_versionBuildId_dev2;
            firstSerdes              = 0;
            lastSerdes               = FM10000_EPL_RING_SERDES_NUM-1;

            pSwapCodeImage           = (fm_uint16 *) fm10000_serdes_swap_code_dev2;
            swapCodeSize             = fm10000_serdes_swap_code_size_dev2;
            swapCodeVersionBuildId   = fm10000_serdes_swap_code_versionBuildId_dev2;
            break;

        default:
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_SERDES, "Invalid SerDes image selector: %d\n", serdesImageSelector);
    }

    if (err == FM_OK)
    {

        switchExt->serdesSupportsKR = (serdesCodeVersionBuildId & 0x04) ? TRUE : FALSE;

        if (GET_FM10000_PROPERTY()->serdesDbgLevel > 0)
        {
            FM_LOG_PRINT("Support for KR: %s\n", switchExt->serdesSupportsKR? "YES" : "NO");
        }


        err = fm10000SbmSpicoUploadImage(sw,
                                         FM10000_SERDES_RING_EPL,
                                         FM10000_SBUS_SPICO_BCAST_ADDR,
                                         pSbmCodeImage,
                                         sbmCodeSize);
    }

    if (err == FM_OK)
    {

        err = fm10000SbmChckCrcVersionBuildId(sw, FM10000_SERDES_RING_EPL, sbmCodeVersionBuildId);
    }



    if (err == FM_OK && swapCodeSize > 0)
    {
        if ((sbmCodeVersionBuildId & 0x00008000) == 0)
        {


            err = fm10000SerdesSwapUploadImage(sw,
                                               FM10000_SERDES_RING_EPL,
                                               FM10000_SBUS_SPICO_BCAST_ADDR,
                                               pSwapCodeImage,
                                               swapCodeSize);
            swapCrcCode = 0x1a;
        }
        else
        {
            err = fm10000SerdesSwapAltUploadImage(sw,
                                                  FM10000_SERDES_RING_EPL,
                                                  FM10000_SBUS_SPICO_BCAST_ADDR,
                                                  pSwapCodeImage,
                                                  swapCodeSize);
            swapCrcCode = 0x04;
        }

        if (err == FM_OK)
        {

            err = fm10000SwapImageCheckCrc(sw, FM10000_SERDES_RING_EPL, swapCrcCode);
        }
    }

    if (err == FM_OK)
    {




        err = fm10000SerdesSpicoUploadImage(sw,
                                            FM10000_SERDES_RING_EPL,
                                            FM10000_SERDES_EPL_BCAST,
                                            pSerdesCodeImage,
                                            serdesCodeSize);
        if (err == FM_OK)
        {


            err = fm10000SerdesChckCrcVersionBuildId(sw,
                                                     firstSerdes,
                                                     lastSerdes,
                                                     serdesCodeVersionBuildId);

            fm10000SerdesSpicoSaveImageParamV2(pSerdesCodeImage,
                                               serdesCodeSize,
                                               serdesCodeVersionBuildId);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/**  fm10000InitSwSerdes
 * \ingroup intSerdes
 *
 * \desc            Perform any SERDES intialization at switch initialization.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000InitSwSerdes(fm_int sw)
{
    fm_status       err;
    fm_serDesOpMode serdesOpMode;
    fm10000_switch *switchExt;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    err = fm10000SerdesInitXServices(sw);



    if (err == FM_OK && switchExt->serdesBypassSbus == FALSE)
    {
        err = fm10000SerdesGetOpMode(sw, 0, &serdesOpMode, NULL, NULL);

        if (err == FM_OK)
        {
            if (serdesOpMode != FM_SERDES_OPMODE_STUB_SM)
            {

                err = fm10000SpicoRamBist(sw,
                                          FM10000_SERDES_RING_EPL,
                                          FM10000_SBUS_SPICO_BCAST_ADDR,
                                          FM10000_SPICO_BIST_CMD_ALL);
                if (err != FM_OK)
                {

                    FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                                 "Spico Ram BIST Failed: %s\n",
                                 fmErrorMsg(err));
                }


                err = fm10000LoadSpicoCode(sw);


                if (err == FM_OK)
                {
                    err = fm10000SerdesInitGenericOptions(sw);
                }

            }
        }
    }


    switchExt->serdesIntAccssCtrlEna = TRUE;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}




/*****************************************************************************/
/**  fm10000SerdesWrite
 * \ingroup intSerdes
 *
 * \desc            Write to a SERDES register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SERDES number.
 *
 * \param[in]       regAddr is the SERDES register address to write to.
 *
 * \param[in]       value is the value to write to the register.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesWrite(fm_int     sw,
                             fm_int     serdes,
                             fm_uint    regAddr,
                             fm_uint32  value)
{
    fm_status       err;
    fm_uint         sbusAddr;
    fm_serdesRing   ring;

    err = fm10000MapSerdesToSbus(sw, serdes, &sbusAddr, &ring);

    if (err == FM_OK)
    {
        err = fm10000SbusWrite(sw, (ring == FM10000_SERDES_RING_EPL), sbusAddr, regAddr, value);
    }

    return err;

}




/*****************************************************************************/
/**  fm10000SerdesRead
 * \ingroup intSerdes
 *
 * \desc            Read a SerDes register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serdes is the SerDes number.
 *
 * \param[in]       regAddr is the SerDes register address to read from.
 *
 * \param[in]       pValue is caller allocated storage where the register
 *                  value will be written.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesRead(fm_int     sw,
                            fm_int     serdes,
                            fm_uint    regAddr,
                            fm_uint32 *pValue)
{
    fm_status       err;
    fm_uint         sbusAddr;
    fm_serdesRing   ring;


    err = fm10000MapSerdesToSbus(sw, serdes, &sbusAddr, &ring);

    if (err == FM_OK)
    {
        err = fm10000SbusRead(sw, (ring == FM10000_SERDES_RING_EPL), sbusAddr, regAddr, pValue);
    }

    return err;

}




/*****************************************************************************/
/**  fm10000SerdesReadModifyWrite
 * \ingroup intSerdes
 *
 * \desc            Read and modify a SERDES register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number.
 *
 * \param[in]       regAddr is the SerDes register address to write to.
 *
 * \param[in]       data is the value to write to the register.
 *
 * \param[in]       mask is the mask to modify the register value.
 *
 * \param[in]       pReadValue points to the caller-allocated storage where
 *                  the function will return the read value before
 *                  modification. It may be NULL.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesReadModifyWrite(fm_int     sw,
                                       fm_int     serDes,
                                       fm_uint    regAddr,
                                       fm_uint32  data,
                                       fm_uint32  mask,
                                       fm_uint32 *pReadValue)
{
    fm_status       err;
    fm_uint32       readVal;
    fm10000_switch *switchExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, regAddr=0x%2.2x, data=0x%4.4x, mask=0x%4.4x, pReadValue=%p\n",
                    sw,
                    serDes,
                    regAddr,
                    data,
                    mask,
                    (void*)pReadValue);

    switchExt = GET_SWITCH_EXT(sw);

    if (switchExt->serdesBypassSbus == TRUE)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fm10000SerdesRead(sw, serDes, regAddr, &readVal);

        if (err == FM_OK)
        {
            err = fm10000SerdesWrite(sw, serDes, regAddr, (data & mask) | (readVal & ~mask));

            if (err == FM_OK && pReadValue != NULL)
            {
                *pReadValue = readVal;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSpicoWrOnlyInt
 * \ingroup intSerdes
 *
 * \desc            Perform a SERDES SPICO write only interrupt.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoWrOnlyInt(fm_int      sw,
                                      fm_int      serDes,
                                      fm_uint     intNum,
                                      fm_uint32   param)
{
    fm_status   err;
    fm_uint32   retVal;

    FM_LOG_ENTRY_VERBOSE_V2(FM_LOG_CAT_SWITCH, serDes,
                            "sw=%d, serDes=%d, intNum 0x%4.4x, param=0x%4.4x\n",
                            sw,
                            serDes,
                            intNum,
                            param);

    err = fm10000SerdesSpicoInt(sw,serDes,intNum,param,&retVal);

    if (err == FM_OK && retVal != intNum)
    {
        err = FM_FAIL;
        FM_LOG_ERROR( FM_LOG_CAT_SERDES,
                      "Bad value returned by SPICO interrupt: serdes=0x%2.2x, "
                      "intNum=0x%2.2x, param=0x%8.8x, retVal=0x%2.2x expected retVal=0x%2.2x\n",
                      serDes,
                      intNum,
                      param,
                      retVal,
                      intNum);
    }

    FM_LOG_EXIT_VERBOSE_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSpicoInt
 * \ingroup intSerdes
 *
 * \desc            Perform SERDES SPICO interrupt.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \param[out]      pValue points to the caller-allocated storage where this
 *                  function will place the result. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSpicoInt(fm_int      sw,
                                fm_int      serDes,
                                fm_uint     intNum,
                                fm_uint32   param,
                                fm_uint32  *pValue)
{
    fm_status           err;
    fm10000_switch     *switchExt;
    fm_bool             isEplRing;
    fm_bool             useParallelIntf;

    FM_LOG_ENTRY_VERBOSE_V2(FM_LOG_CAT_SWITCH, serDes,
                            "sw=%d, serDes=%d, intNum 0x%4.4x, param=0x%4.4x, pValue=%p\n",
                            sw,
                            serDes,
                            intNum,
                            param,
                            (void *) pValue);

    switchExt = GET_SWITCH_EXT(sw);


    if (pValue != NULL)
    {
        *pValue = 0;
    }

    isEplRing = (serDes < FM10000_EPL_RING_SERDES_NUM);

    useParallelIntf = FALSE;

    if (switchExt->serdesIntUseLaneSai && switchExt->serdesIntAccssCtrlEna)
    {
        useParallelIntf = !eplUseSbusIntf;
    }
    if (!isEplRing)
    {
        useParallelIntf = !pcieUseSbusIntf;
    }

    if (useParallelIntf)
    {
        if (isEplRing)
        {



            err = fm10000SerdesSpicoSaiInt(sw, serDes, intNum, param, pValue);
        }
        else
        {

            err = fm10000SerdesPcieSpicoInt(sw, serDes, intNum, param, pValue);
        }
    }
    else
    {

        err = fm10000SerdesSpicoIntSBusWrite(sw, serDes, intNum, param);
        if (err == FM_OK)
        {


            err = fm10000SerdesSpicoIntSBusRead(sw, serDes, pValue);
        }
    }

    FM_LOG_EXIT_VERBOSE_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SbmSpicoInt
 * \ingroup intSerdes
 *
 * \desc            Perform a SBUS master SPICO interrupt.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ring specifies the target ring: EPL or PCIE.
 *
 * \param[in]       sbusAddr is the SPICO SBUS address.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[in]       param is the interrupt parameter.
 *
 * \param[out]      pValue points to the caller-allocated storage where this
 *                  function will place the result.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SbmSpicoInt(fm_int         sw,
                             fm_serdesRing  ring,
                             fm_int         sbusAddr,
                             fm_uint        intNum,
                             fm_uint32      param,
                             fm_uint32     *pValue)
{
    fm_status err;


    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, ring=%d, sbusAddr=0x%2.2x, intNum= %d, param=0x%4.4x, pValue=%p\n",
                         sw,
                         ring,
                         sbusAddr,
                         intNum,
                         param,
                         (void *) pValue);

    if (pValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {

        err = fm10000SbmSpicoIntWrite(sw, ring, sbusAddr, intNum, param);

        if (err == FM_OK)
        {

            err = fm10000SbmSpicoIntRead(sw, ring, sbusAddr, pValue);
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesInitLaneControlStructures
 * \ingroup intSerdes
 *
 * \desc            Initialize lane control structures associated to the
 *                  specified serDes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the serdes ID number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitLaneControlStructures(fm_int sw,
                                                 fm_int serDes)
{
    fm_status            err;
    fm_switch           *switchPtr;
    fm_lane             *pLane;
    fm_laneAttr         *pLaneAttr;
    fm10000_lane        *pLaneExt;
    fm10000_laneDfe     *pLaneDfe;
    fm10000_laneKr      *pLaneKr;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d serDes=0x%2.2x\n", sw, serDes);

    err = FM_OK;

    if ( serDes < 0 || serDes >= FM10000_NUM_SERDES )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                         serDes,
                         "Invalid serDes ID number: %d\n",
                         serDes );
    }
    else
    {
        switchPtr = GET_SWITCH_PTR(sw);
        pLane     = GET_LANE_PTR( sw, serDes );
        pLaneExt  = GET_LANE_EXT(sw, serDes);
        pLaneAttr = GET_LANE_ATTR( sw, serDes );
        pLaneDfe  = &pLaneExt->dfeExt;
        pLaneKr   = &pLaneExt->krExt;




        pLaneExt->smType                = FM_SMTYPE_UNSPECIFIED;
        pLaneExt->transitionHistorySize = FM10000_SERDES_SM_HISTORY_SIZE;
        pLaneExt->bitRate               = FM10000_LANE_BITRATE_UNKNOWN;
        pLaneExt->dfeMode               = FM_DFE_MODE_STATIC;

        pLaneExt->serDesActive          = GetSerdesValidId(sw, serDes);
        pLaneExt->serDesEnableCache     = 0;
        pLaneExt->serDes                = serDes;


        pLaneExt->rxTermination         = FM10000_SERDES_RX_TERM_AVDD;
        pLaneExt->pllCalibrationMode    = FM10000_SERDES_PLL_CALIBRATION_ENABLED;

        pLaneExt->pllCalibrationCycleCnt = (((fm_uint32)rand()) % FM10000_SERDES_PLL_CAL_CNT_THRESHOLD) / 10;



        pLaneAttr->preCursor            = 0;
        pLaneAttr->cursor               = 0;
        pLaneAttr->postCursor           = 0;
        pLaneAttr->initializePreCursor  = 4;
        pLaneAttr->initializeCursor     = 2;
        pLaneAttr->initializePostCursor = 18;
        pLaneAttr->preCursorDecOnPreset = 0;
        pLaneAttr->postCursorDecOnPreset= 0;
        pLaneAttr->txLaneEnableConfigKrInit = FM_DISABLED;
        pLaneAttr->transitionThreshold  = FM10000_SERDES_SIGNAL_TRANSITION_THRESHOLD_DFV;



        pLaneExt->lane                  = FM_PORT_LANE_NA;


        pLaneExt->eeeModeActive         = 0;


        FM_DLL_INIT_NODE( pLaneExt, nextLane, prevLane );


        pLaneExt->eventInfo.switchPtr   = switchPtr;
        pLaneExt->eventInfo.lanePtr     = pLane;
        pLaneExt->eventInfo.laneExt     = pLaneExt;
        pLaneExt->eventInfo.laneAttr    = pLaneAttr;


        pLaneExt->bistCustomData0       = FM10000_SERDES_BIST_USER_PATTERN_DEFAULT;
        pLaneExt->bistCustomData1       = FM10000_SERDES_BIST_USER_PATTERN_DEFAULT;


        pLaneDfe->smType                = FM_SMTYPE_UNSPECIFIED;
        pLaneDfe->transitionHistorySize = FM10000_SERDES_DFE_SM_HISTORY_SIZE;
        pLaneDfe->dfeAdaptive           = TRUE;
        pLaneDfe->dfe_HF                = FM10000_SERDES_DFE_DFAULT_HF;
        pLaneDfe->dfe_LF                = FM10000_SERDES_DFE_DFAULT_LF;
        pLaneDfe->dfe_DC                = FM10000_SERDES_DFE_DFAULT_DC;
        pLaneDfe->dfe_BW                = FM10000_SERDES_DFE_DFAULT_BW;


        pLaneDfe->dfeDebounceTime       = -1;


        pLaneDfe->eventInfo.switchPtr   = switchPtr;
        pLaneDfe->eventInfo.lanePtr     = pLane;
        pLaneDfe->eventInfo.laneExt     = pLaneExt;
        pLaneDfe->eventInfo.laneDfe     = pLaneDfe;
        pLaneDfe->eventInfo.laneAttr    = pLaneAttr;


        pLaneKr->pLaneExt               = pLaneExt;
        pLaneKr->invrtAdjPolarity       = FALSE;
        pLaneKr->disaTimeout            = TRUE;
        pLaneKr->disaTxEqAdjReq         = FALSE;
        pLaneKr->resetParameters        = TRUE;
        pLaneKr->relLane                = 0;
        pLaneKr->seed                   = 0;
        pLaneKr->clause                 = FM10000_PMD_CONTROL_CLAUSE72;
        pLaneKr->opt_TT_FECreq          = FALSE;
        pLaneKr->opt_TT_TF              = FALSE;
        pLaneKr->opt_TT_FECcap          = FALSE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesClearAllSerDesInterrupts
 * \ingroup intSerdes
 *
 * \desc            Clear the interrupts for all serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesClearAllSerDesInterrupts(fm_int sw)
{
   fm_status    err;
   fm_int       eplNdx;
   fm_int       laneNdx;
   fm_uint32    val;
   fm_switch *  switchPtr;

   FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

   err = FM_OK;
   switchPtr = GET_SWITCH_PTR(sw);

   val = ~0;

   TAKE_REG_LOCK( sw );
   for (eplNdx = 0; eplNdx < FM10000_NUM_EPLS && err == FM_OK; eplNdx++)
   {
       for (laneNdx=0; laneNdx < FM10000_PORTS_PER_EPL && err == FM_OK; laneNdx++)
       {
           err = switchPtr->WriteUINT32( sw,
                                         FM10000_SERDES_IM(eplNdx, laneNdx),
                                         val);
           if (err == FM_OK)
           {
               err = switchPtr->WriteUINT32( sw,
                                             FM10000_SERDES_IP(eplNdx, laneNdx),
                                             val);
           }
       }
   }
   DROP_REG_LOCK( sw );

   FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesInitOpMode
 * \ingroup intSerdes
 *
 * \desc            Initializes serdes operational mode according to the value
 *                  of api.FM10000.serdesOpMode API attribute and the
 *                  api.platform.model.devBoardIp platform attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitOpMode(fm_int sw)
{
    fm_status           err;
    fm10000_switch     *switchExt;
    fm_int              serdesOpModeTmp;
    fm_text             boardIp;
    fm_int              boardPort;
    fm_int              serdesDbgLvl;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    err = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);


    switchExt->serdesBypassSbus = FALSE;




    boardIp = GET_PROPERTY()->modelDevBoardIp;

    boardPort = GET_PROPERTY()->modelDevBoardPort;

    if (strlen(boardIp) != 0 && boardPort != 0)
    {

        switchExt->serdesOpMode        = FM_SERDES_OPMODE_TEST_BOARD;
        switchExt->serdesSmMode        = FM10000_SERDES_USE_TEST_MODE_STATE_MACHINE;
        switchExt->serdesIntUseLaneSai = FALSE;
    }
    else
    {


        switchExt->serdesIntAccssCtrlEna = FALSE;


        serdesOpModeTmp = GET_FM10000_PROPERTY()->serdesOpMode;

        serdesDbgLvl = GET_FM10000_PROPERTY()->serdesDbgLevel;

        if (serdesDbgLvl > 0)
        {
            FM_LOG_PRINT("Required SerDes Op Mode= %d\n",serdesOpModeTmp);
        }



        if (GET_PROPERTY()->isWhiteModel)
        {
            if (serdesOpModeTmp == FM_SERDES_OPMODE_CHIP_VIA_SAI  ||
                serdesOpModeTmp == FM_SERDES_OPMODE_CHIP_VIA_SBUS)
            {
                FM_LOG_PRINT("Forced SerDes Stub State Machine Mode\n");
                serdesOpModeTmp = FM_SERDES_OPMODE_STUB_SM;
            }
        }



        switchExt->serdesOpMode  = serdesOpModeTmp;

        switch (serdesOpModeTmp)
        {
            case FM_SERDES_OPMODE_CHIP_VIA_SAI:
            {
                switchExt->serdesSmMode        = FM10000_SERDES_USE_BASIC_STATE_MACHINE;
                switchExt->serdesIntUseLaneSai = TRUE;
                if (serdesDbgLvl > 0)
                {
                    FM_LOG_PRINT("SerDes API configured for silicon:\n");
                    FM_LOG_PRINT(" * Basic SerDes state machine\n");
                    FM_LOG_PRINT(" * SerDes interrupts via LANE_SAI_ registers\n");
                }
                break;
            }

            case FM_SERDES_OPMODE_CHIP_VIA_SBUS:
            {
                switchExt->serdesSmMode        = FM10000_SERDES_USE_BASIC_STATE_MACHINE;
                switchExt->serdesIntUseLaneSai = FALSE;
                if (serdesDbgLvl > 0)
                {
                    FM_LOG_PRINT("SerDes API configured for silicon:\n");
                    FM_LOG_PRINT(" * Basic SerDes state machine\n");
                    FM_LOG_PRINT(" * SerDes interrupts via SBus\n");
                }
                break;
            }

            case FM_SERDES_OPMODE_TEST_BENCH:
            {
                switchExt->serdesSmMode          = FM10000_SERDES_USE_BASIC_STATE_MACHINE;

                switchExt->serdesIntUseLaneSai   = TRUE;

                switchExt->serdesBypassSbus      = TRUE;
                FM_LOG_PRINT("SerDes API configured for Test Bench mode:\n");
                FM_LOG_PRINT(" * Basic SerDes state machine\n");
                FM_LOG_PRINT(" * SerDes interrupts via LANE_SAI_ registers\n");
                FM_LOG_PRINT(" * SBus bypassed\n");
                break;
            }

            case FM_SERDES_OPMODE_TEST_BOARD:
            {
                switchExt->serdesSmMode        = FM10000_SERDES_USE_TEST_MODE_STATE_MACHINE;
                switchExt->serdesIntUseLaneSai = FALSE;
                FM_LOG_PRINT("SerDes API configured for Test Board mode:\n");
                FM_LOG_PRINT(" * SerDes [0..5] : Basic SerDes state machine\n");
                FM_LOG_PRINT(" * SerDes [6..up]: Stub  SerDes state machine\n");
                FM_LOG_PRINT(" * SerDes interrupts via SBus\n");

                break;
            }

            case FM_SERDES_OPMODE_STUB_SM:
            {
                switchExt->serdesSmMode        = FM10000_SERDES_USE_STUB_STATE_MACHINE;
                switchExt->serdesIntUseLaneSai = FALSE;
                switchExt->serdesBypassSbus    = TRUE;
                FM_LOG_PRINT("SerDes API running Stub SerDes state machines\n");
                break;
            }

            default:
            {

                switchExt->serdesSmMode        = FM10000_SERDES_USE_STUB_STATE_MACHINE;
                switchExt->serdesIntUseLaneSai = TRUE;
                err = FM_FAIL;

                FM_LOG_ERROR(FM_LOG_CAT_SERDES,"Invalid serdes opMode=%d\n",serdesOpModeTmp);
                break;
            }

        }

        if (err == FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SERDES,"Serdes opMode=%d, stateMachineMode=%d, useLaneSai=%s\n",
                         switchExt->serdesOpMode,
                         switchExt->serdesSmMode,
                         (switchExt->serdesIntUseLaneSai == FALSE) ? "FALSE" : "TRUE");
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000SerdesGetOpMode
 * \ingroup intSerdes
 *
 * \desc            Returns the serdes operational mode, including the opMode,
 *                  the kind of state machine being used and if serdes register
 *                  accesses are via LANE_SAI_XXX registers of via SBus.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate. It only
 *                  used for some serdes opModes.
 *
 * \param[out]      pSerdesOpMode points to the caller-allocated storage where
 *                  this function will return the serdes opMode.It may be NULL.
 *
 * \param[out]      pSerdesSmMode points to the caller-allocated storage where
 *                  this function will return the serdes state machine mode.
 *                  It may be NULL.
 *
 * \param[out]      pSerdesUseLaneSai points to the caller-allocated storage
 *                  where this function will return the if the specified serdes
 *                  should use the SBus to access the serdes (TRUE) or the
 *                  LANE_PAI_XXX registers instead. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetOpMode(fm_int                 sw,
                                 fm_int                 serDes,
                                 fm_serDesOpMode       *pSerdesOpMode,
                                 fm10000_serDesSmMode  *pSerdesSmMode,
                                 fm_bool               *pSerdesUseLaneSai)
{
    fm_status               err;
    fm10000_switch         *switchExt;
    fm_serDesOpMode         serdesOpModeTmp;
    fm10000_serDesSmMode    serdesSmModeTmp;
    fm_bool                 serdesUseLaneSaiTmp;


    FM_LOG_ENTRY(FM_LOG_CAT_SERDES,
                 "sw=%d, serDes=%d, pSerdesOpMode=%p, pSerdesSmMode=%p, "
                 "pSerdesUseLaneSai=%p\n",
                 sw,
                 serDes,
                 (void *) pSerdesOpMode,
                 (void *) pSerdesSmMode,
                 (void *) pSerdesUseLaneSai);

    err = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);

    serdesOpModeTmp     = switchExt->serdesOpMode;
    serdesSmModeTmp     = switchExt->serdesSmMode;
    serdesUseLaneSaiTmp = switchExt->serdesIntUseLaneSai;



    if (serdesSmModeTmp == FM10000_SERDES_USE_TEST_MODE_STATE_MACHINE)
    {

        if ( fm10000SerdesCheckIfIsActive(sw, serDes) )
        {

            serdesSmModeTmp     = FM10000_SERDES_USE_BASIC_STATE_MACHINE;
            serdesUseLaneSaiTmp = TRUE;
        }
        else
        {
            serdesSmModeTmp     = FM10000_SERDES_USE_STUB_STATE_MACHINE;
            serdesUseLaneSaiTmp = FALSE;
        }
    }
    else if (serdesSmModeTmp >= FM10000_SERDES_USE_STATE_MACHINE_MAX)
    {

        err = FM_FAIL;
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Invalid serdes state machine mode=%d\n",
                     serdesSmModeTmp);
    }

    if (err == FM_OK)
    {
        if (pSerdesOpMode != NULL)
        {
            *pSerdesOpMode = serdesOpModeTmp;
        }

        if (pSerdesSmMode != NULL)
        {
            *pSerdesSmMode = serdesSmModeTmp;
        }

        if (pSerdesUseLaneSai != NULL)
        {
            *pSerdesUseLaneSai = serdesUseLaneSaiTmp;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_SERDES, err);

}





/*****************************************************************************/
/** fm10000SetPcslCfgWidthMode
 * \ingroup intSerdes
 *
 * \desc            Configures PCS data width according to the port speed.
 *                  Only valid for 10G and 25G ethernet modes. Only applicable
 *                  to EPL serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the target SerDes.
 *
 * \param[out]      widthMode is the whidth mode being used in the serdes.
 *                  See ''fm_serdesWidthMode'' enumeration.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if serDes and/or widthMode are
 *                   invalid.
 *
 *****************************************************************************/
fm_status fm10000SetPcslCfgWidthMode(fm_int             sw,
                                     fm_int             serDes,
                                     fm_serdesWidthMode widthMode)
{
    fm_status   err;
    fm_switch * switchPtr;
    fm_int      epl;
    fm_int      lane;
    fm_uint32   pcsl_cfg;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, widthMode=%d\n",
                     sw,
                     serDes,
                     widthMode);


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
    else if (widthMode != FM10000_SERDES_WIDTH_10 &&
             widthMode != FM10000_SERDES_WIDTH_20 &&
             widthMode != FM10000_SERDES_WIDTH_40)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Invalid data width specification; serDes=%d, widtMode=%d\n",
                        serDes,
                        widthMode);
    }
    else
    {

        err = fm10000MapSerdesToEplLane(sw, serDes, &epl, &lane);



        if (err == FM_OK)
        {

            err = switchPtr->ReadUINT32(sw, FM10000_PCSL_CFG(epl, lane), &pcsl_cfg);

            if (err == FM_OK)
            {
                FM_SET_BIT(pcsl_cfg, FM10000_PCSL_CFG, TxGbNarrow, (widthMode == FM10000_SERDES_WIDTH_40) ? 0 : 1);
                FM_SET_BIT(pcsl_cfg, FM10000_PCSL_CFG, RxGbNarrow, (widthMode == FM10000_SERDES_WIDTH_40) ? 0 : 1);
                FM_SET_BIT(pcsl_cfg, FM10000_PCSL_CFG, RxBitSlipEnable, 0x00);

                err = switchPtr->WriteUINT32(sw, FM10000_PCSL_CFG(epl, lane), pcsl_cfg);
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSaveKrTrainingDelayInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  KR training.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveKrTrainingDelayInfo(fm_int       sw,
                                               fm_int       serDes)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneKr     *pLaneKr;
    fm_uint32           timeStamp;
    fm_uint32           timeDiff;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneKr  = &pLaneExt->krExt;
    err = FM_OK;

    if (pLaneKr->startKrCycleCnt)
    {
        timeStamp = fm10000SerdesGetTimestampMs();

        err = fm10000SerdesGetTimestampDiffMs(pLaneKr->refTimeMs,timeStamp,&timeDiff);

        if (err == FM_OK)
        {

            pLaneKr->krTrainingDelayLastMs = timeDiff;


            if (pLaneKr->krTrainingDelayMaxMs < timeDiff)
            {
                pLaneKr->krTrainingDelayMaxMs = timeDiff;
            }


            if (pLaneKr->startKrCycleCnt < 32)
            {
                pLaneKr->krTrainingDelayAvgMs = (pLaneKr->krTrainingDelayAvgMs * (pLaneKr->startKrCycleCnt - 1)
                                                 + 1000 * timeDiff) / pLaneKr->startKrCycleCnt;
            }
            else
            {
                pLaneKr->krTrainingDelayAvgMs =   pLaneKr->krTrainingDelayAvgMs
                                                - (pLaneKr->krTrainingDelayAvgMs / 32)
                                                + (1000 * timeDiff) / 32;
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Serdes #%d DFE delays: statistical value out of range\n", serDes);
        }
    }
    else
    {
        err = FM_ERR_INVALID_STATE;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSaveKrTrainingTimeoutInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  KR training.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveKrTrainingTimeoutInfo(fm_int       sw,
                                                 fm_int       serDes)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneKr     *pLaneKr;
    fm_uint32           timeStamp;
    fm_uint32           timeDiff;
    fm_uint             linkTimeoutThreshold;
    fm_int              port;
    fm10000_portAttr   *portAttrExt;
    fm_bool             signalOk;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneKr  = &pLaneExt->krExt;
    err = FM_OK;

    if (pLaneKr->startKrCycleCnt)
    {
        timeStamp = fm10000SerdesGetTimestampMs();

        err = fm10000SerdesGetTimestampDiffMs(pLaneKr->refTimeMs,timeStamp,&timeDiff);

        if (err == FM_OK)
        {
            err = fm10000MapSerdesToLogicalPort(sw, serDes, &port);
            if (err == FM_OK)
            {
                portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

                linkTimeoutThreshold = (portAttrExt->autoNegLinkInhbTimer - (portAttrExt->autoNegLinkInhbTimer >> 5)) / 1000;

                err = fm10000SerdesGetSignalOk(sw,serDes, &signalOk);
            }

            if (err == FM_OK                            &&
                timeDiff >= linkTimeoutThreshold        &&
                timeDiff < (2 * linkTimeoutThreshold)   &&
                signalOk == FALSE)
            {
                pLaneKr->krTrainingDelayLastMs = timeDiff;
                if (pLaneKr->krTrainingDelayMaxMs < timeDiff)
                {
                    pLaneKr->krTrainingDelayMaxMs = timeDiff;
                }


                if (pLaneKr->startKrCycleCnt < 32)
                {
                    pLaneKr->krTrainingDelayAvgMs = (pLaneKr->krTrainingDelayAvgMs * (pLaneKr->startKrCycleCnt - 1)
                                                     + 1000 * timeDiff) / pLaneKr->startKrCycleCnt;
                }
                else
                {
                    pLaneKr->krTrainingDelayAvgMs =   pLaneKr->krTrainingDelayAvgMs
                                                    - (pLaneKr->krTrainingDelayAvgMs / 32)
                                                    + (1000 * timeDiff) / 32;
                }

                err = fm10000SerdesIncrKrStatsCounter(sw,serDes,2);
            }
        }

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Serdes #%d KR delays: cannot compute timeout statistical value out of range\n", serDes);
        }
    }
    else
    {
        err = FM_ERR_INVALID_STATE;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesIncrKrStatsCounter
 * \ingroup intSerdes
 *
 * \desc            Function that increments one of the KR statistical counters:
 *                  the start KR training counter or the KR training failed.
 *                  These counters saturates at 0xffffffff.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       counterSpec: 0 indicates increment KR-start-training
 *                  counter, 1 indicates incr KR-error counter; 2 indicates
 *                  incr KR-timeout counter.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesIncrKrStatsCounter(fm_int       sw,
                                          fm_int       serDes,
                                          fm_int       counterSpec)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneKr     *pLaneKr;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneKr = &pLaneExt->krExt;
    err = FM_OK;


    switch (counterSpec)
    {
        case 0:

            if (pLaneKr->startKrCycleCnt < 0xffffffff)
            {
                (pLaneKr->startKrCycleCnt)++;
            }
            break;
        case 1:

            if (pLaneKr->krErrorCnt < 0xffffffff)
            {
                (pLaneKr->krErrorCnt)++;
            }
            break;
         case 2:

            if (pLaneKr->krTimeoutCnt < 0xffffffff)
            {
                (pLaneKr->krTimeoutCnt)++;
            }
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;

    }

    return err;

}





/*****************************************************************************/
/** fm10000SerdesSaveStopTuningStatsInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when stopping
 *                  DFE tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       stopCoarseDelay stop coarse tuning delay. See
 *                  ''fm10000_laneDfe'' to see measure units.
 *
 * \param[in]       stopFineDelay stop fine tuning delay. See
 *                  ''fm10000_laneDfe'' to see measure units.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveStopTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    stopCoarseDelay,
                                               fm_uint32    stopFineDelay)
{
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;



    if (pLaneDfe->stopCoarseDelayMax < stopCoarseDelay)
    {
        pLaneDfe->stopCoarseDelayMax = stopCoarseDelay;
    }

    if (pLaneDfe->stopFineDelayMax < stopFineDelay)
    {
        pLaneDfe->stopFineDelayMax = stopFineDelay;
    }


    if (pLaneDfe->stopCycleCnt < 32)
    {
        pLaneDfe->stopCoarseDelayAvg = (pLaneDfe->stopCoarseDelayAvg * (pLaneDfe->stopCycleCnt -1)
                                      + 1000 * stopCoarseDelay) / pLaneDfe->stopCycleCnt;
        pLaneDfe->stopFineDelayAvg   = (pLaneDfe->stopFineDelayAvg * (pLaneDfe->stopCycleCnt -1)
                                      + 1000 * stopFineDelay) / pLaneDfe->stopCycleCnt;
    }
    else
    {
        pLaneDfe->stopCoarseDelayAvg =   pLaneDfe->stopCoarseDelayAvg
                                       - (pLaneDfe->stopCoarseDelayAvg / 32)
                                       + (1000 * stopCoarseDelay) / 32;

        pLaneDfe->stopFineDelayAvg =   pLaneDfe->stopFineDelayAvg
                                       - (pLaneDfe->stopFineDelayAvg / 32)
                                       + (1000 * stopFineDelay) / 32;
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesSaveICalTuningStatsInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  iCal DFE (coarse) tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       iCalDelay iCal (coarse) tuning delay. See ''fm10000_laneDfe''
 *                  to see measure units.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveICalTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    iCalDelay)
{
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;



    if (pLaneDfe->iCalDelayMax < iCalDelay)
    {
        pLaneDfe->iCalDelayMax = iCalDelay;
    }


    if (pLaneDfe->startCycleCnt < 32)
    {
        pLaneDfe->iCalDelayAvg = (pLaneDfe->iCalDelayAvg * (pLaneDfe->startCycleCnt - 1)
                                + 1000 * iCalDelay) / pLaneDfe->startCycleCnt;
    }
    else
    {
        pLaneDfe->iCalDelayAvg =   pLaneDfe->iCalDelayAvg
                                 - (pLaneDfe->iCalDelayAvg / 32)
                                 + (1000 * iCalDelay) / 32;
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesSavePCalTuningStatsInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  pCal DFE (fine) tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       pCalDelay iCal (fine) tuning delay. See ''fm10000_laneDfe''
 *                  to see measure units.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSavePCalTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    pCalDelay)
{
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;


    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;


    if (pLaneDfe->pCalDelayMax < pCalDelay)
    {
        pLaneDfe->pCalDelayMax = pCalDelay;
    }


    if (pLaneDfe->startCycleCnt < 32)
    {
        pLaneDfe->pCalDelayAvg =  (pLaneDfe->pCalDelayAvg * (pLaneDfe->startCycleCnt - 1)
                                 + 1000 * pCalDelay) / pLaneDfe->startCycleCnt;
    }
    else
    {
        pLaneDfe->pCalDelayAvg =   pLaneDfe->pCalDelayAvg
                                 - (pLaneDfe->pCalDelayAvg / 32)
                                 + (1000 * pCalDelay) / 32;
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerdesSaveICalTuningDelayInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  iCal DFE (coarse) tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveICalTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;
    fm_uint32           timeStamp;
    fm_uint32           timeDiff;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;
    err = FM_OK;

    if (pLaneDfe->startCycleCnt)
    {
        timeStamp = fm10000SerdesGetTimestampMs();

        err = fm10000SerdesGetTimestampDiffMs(pLaneDfe->refTimeMs,
                                              timeStamp,&timeDiff);

        if (err == FM_OK)
        {

            pLaneDfe->iCalDelayLastMs = timeDiff;


            if (pLaneDfe->iCalDelayMaxMs < timeDiff)
            {
                pLaneDfe->iCalDelayMaxMs = timeDiff;
            }


            if (pLaneDfe->startCycleCnt < 32)
            {
                pLaneDfe->iCalDelayAvgMs = (pLaneDfe->iCalDelayAvgMs * (pLaneDfe->startCycleCnt - 1)
                                           + 1000 * timeDiff) / pLaneDfe->startCycleCnt;
            }
            else
            {
                pLaneDfe->iCalDelayAvgMs =   pLaneDfe->iCalDelayAvgMs
                                          - (pLaneDfe->iCalDelayAvgMs / 32)
                                          + (1000 * timeDiff) / 32;
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Serdes #%d DFE delays: "
                            "statistical value out of range\n",
                            serDes);
        }
    }
    else
    {
        err = FM_ERR_INVALID_STATE;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSavePCalTuningDelayInfo
 * \ingroup intSerdes
 *
 * \desc            Saves statistical information about delays when performing
 *                  iCal DFE (coarse) tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSavePCalTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;
    fm_uint32           timeStamp;
    fm_uint32           timeDiff;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;
    err = FM_OK;


    if (pLaneDfe->startCycleCnt)
    {
        timeStamp = fm10000SerdesGetTimestampMs();

        err = fm10000SerdesGetTimestampDiffMs(pLaneDfe->refTimeMs,timeStamp,&timeDiff);

        if (err == FM_OK)
        {
            pLaneDfe->pCalDelayLastMs = timeDiff;


            if (pLaneDfe->pCalDelayMaxMs < timeDiff)
            {
                pLaneDfe->pCalDelayMaxMs = timeDiff;
            }


            if (pLaneDfe->startCycleCnt < 32)
            {
                pLaneDfe->pCalDelayAvgMs = (pLaneDfe->pCalDelayAvgMs * (pLaneDfe->startCycleCnt - 1)
                                           + 1000 * timeDiff) / pLaneDfe->startCycleCnt;
            }
            else
            {
                pLaneDfe->pCalDelayAvgMs =   pLaneDfe->pCalDelayAvgMs
                                          - (pLaneDfe->pCalDelayAvgMs / 32)
                                          + (1000 * timeDiff) / 32;
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Serdes #%d DFE delays: statistical value out of range\n", serDes);
        }
    }
    else
    {
        err = FM_ERR_INVALID_STATE;
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesSaveStopTuningDelayInfo
 * \ingroup intSerdes
 *
 * \desc            Calculates and saves statistical information about delays
 *                  when stopping DFE tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSaveStopTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;
    fm_uint32           timeStamp;
    fm_uint32           timeDiff;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;
    err = FM_OK;


    if (pLaneDfe->stopCycleCnt)
    {
        timeStamp = fm10000SerdesGetTimestampMs();

        err = fm10000SerdesGetTimestampDiffMs(pLaneDfe->refTimeMs,timeStamp,&timeDiff);

        if (err == FM_OK)
        {

            pLaneDfe->stopTuningDelayLstMs = timeDiff;


            if (pLaneDfe->stopTuningDelayMaxMs < timeDiff)
            {
                pLaneDfe->stopTuningDelayMaxMs = timeDiff;
            }


            if (pLaneDfe->startCycleCnt < 32)
            {
                pLaneDfe->stopTuningDelayAvgMs = (pLaneDfe->stopTuningDelayAvgMs * (pLaneDfe->stopCycleCnt - 1)
                                                + 1000 * timeDiff) / pLaneDfe->stopCycleCnt;
            }
            else
            {
                pLaneDfe->stopTuningDelayAvgMs =    pLaneDfe->stopTuningDelayAvgMs
                                                 - (pLaneDfe->stopTuningDelayAvgMs / 32)
                                                 + (1000 * timeDiff) / 32;
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Serdes #%d DFE delays: statistical value out of range\n", serDes);
        }
    }
    else
    {
        err = FM_ERR_INVALID_STATE;
    }

    return err;

}





/*****************************************************************************/
/** fm10000SerdesGetTimestampMs
 * \ingroup intSerdes
 *
 * \desc            Function that return a relative timestamp to be used
 *                  for delay calculations involving serdes operations.
 *
 * \return          the time stamp in Ms.
 *
 *****************************************************************************/
fm_uint32 fm10000SerdesGetTimestampMs(void)
{
    fm_timestamp  timeStamp;
    fm_uint32     timeStampMs;



    fmGetTime(&timeStamp);
    timeStampMs = (fm_uint32)(((timeStamp.sec*1000)+(timeStamp.usec/1000)) % FM10000_SERDES_TIMESTAMPMS_MAX);

    return timeStampMs;

}




/*****************************************************************************/
/** fm10000SerdesGetTimestampDiffMs
 * \ingroup intSerdes
 *
 * \desc            Function that return a the difference between two relative
 *                  timestamps (stop-start). This function is intented to be
 *                  used for delay calculations involving serdes operations.
 *                  Timestamps are expressed in Ms and the range is limited to
 *                  FM10000_SERDES_MSTIMESTAMP_MAX
 *
 * \param[in]       start is the initial or reference timestamp, must be in
 *                  the range [0..FM10000_SERDES_MSTIMESTAMP_MAX].
 *
 * \param[in]       stop is the final reference timestamp, must be in
 *                  the range [0..FM10000_SERDES_TIMESTAMPMS_MAX].
 *
 * \param[out]      pDiff points to the caller-allocated storage where this
 *                  function will place the result (stop-start).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pDiff is a NULL pointer or if
 *                  start or stop are out of range.
 *
 *****************************************************************************/
fm_bool fm10000SerdesGetTimestampDiffMs(fm_uint32  start,
                                        fm_uint32  stop,
                                        fm_uint32 *pDiff)
{
    fm_status   err;


    if (pDiff == NULL                           ||
        start >= FM10000_SERDES_TIMESTAMPMS_MAX ||
        stop  >= FM10000_SERDES_TIMESTAMPMS_MAX)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FM_OK;

        *pDiff = stop-start;

        if (*pDiff >= FM10000_SERDES_TIMESTAMPMS_MAX)
        {
            *pDiff -= FM10000_SERDES_TIMESTAMPMS_MAX;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesIncrStatsCounter
 * \ingroup intSerdes
 *
 * \desc            Function that increments one of the statistical counters:
 *                  the start-dfe counter and and the stop-dfe cycle counter.
 *                  These counters saturates at 0xffffffff.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       counterSpec: 0 indicates increment start-dfe cycle counter,
 *                  1 indicates incr stop-dfe cycle counter.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesIncrStatsCounter(fm_int       sw,
                                        fm_int       serDes,
                                        fm_int       counterSpec)
{
    fm_status           err;
    fm10000_lane       *pLaneExt;
    fm10000_laneDfe    *pLaneDfe;



    pLaneExt = GET_LANE_EXT(sw, serDes);
    pLaneDfe = &pLaneExt->dfeExt;
    err = FM_OK;


    switch (counterSpec)
    {
        case 0:

            if (pLaneDfe->startCycleCnt < 0xffffffff)
            {
                (pLaneDfe->startCycleCnt)++;
            }
            break;
        case 1:

            if (pLaneDfe->stopCycleCnt < 0xffffffff)
            {
                (pLaneDfe->stopCycleCnt)++;
            }
            break;
        default:
            err = FM_ERR_INVALID_ARGUMENT;

    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesGetTuningState
 * \ingroup intSerdes
 *
 * \desc            Returns the DFE tuning state for the specified serdes.
 *                  Bits 1:0 return the status of coarse tuning and bits 3:2
 *                  the status of fine tuning.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pDfeTuningState points to the caller-allocated storage
 *                  where this function will return the tuning state for
 *                  the specified serdes.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetTuningState(fm_int     sw,
                                      fm_int     serDes,
                                      fm_uint32 *pDfeTuningState)
{
    fm_status       err;
    fm10000_lane *  pLaneExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d serdes=%d pDfeTuningState=%p\n",
                     sw,
                     serDes,
                     (void*)pDfeTuningState);

    err = FM_OK;

    if (pDfeTuningState == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pLaneExt  = GET_LANE_EXT(sw, serDes);

        *pDfeTuningState = pLaneExt->dfeExt.dfeTuningStat & 0x0f;
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesIsDfeTuningActive
 * \ingroup intSerdes
 *
 * \desc            Returns TRUE if dfe tuning is active on the given serdes
 *                  or FALSE otherwise.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pDfeTuningIsActive points to the caller-allocated storage
 *                  where this function will return TRUE is dfe tuning is
 *                  active or FALSE otherwise.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesIsDfeTuningActive(fm_int     sw,
                                         fm_int     serDes,
                                         fm_bool   *pDfeTuningIsActive)
{
    fm_status       err;
    fm10000_lane *  pLaneExt;

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d serdes=%d pDfeTuningIsActive=%p\n",
                     sw,
                     serDes,
                     (void*)pDfeTuningIsActive);

    err = FM_OK;

    if (pDfeTuningIsActive == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pLaneExt  = GET_LANE_EXT(sw, serDes);

        *pDfeTuningIsActive = ((pLaneExt->dfeExt.dfeTuningStat & 0x0f) != 0) ||
                              (pLaneExt->dfeExt.retryCntr > 0);
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetEyeScore
 * \ingroup intSerdes
 *
 * \desc            Returns the eye height for the given serdes. The eye width
 *                  is not available for the time being and 0xff is returned
 *                  instead. Eye score is not available for static DFE modes
 *                  and 0xffff is returned in that case. The eye score range
 *                  is [0..64].
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pEyeScore points to the caller-allocated storage where this
 *                  function will return the eye height (bits[0..7]) and width
 *                  (bits[8..15])
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetEyeScore(fm_int     sw,
                                   fm_int     serDes,
                                   fm_uint32 *pEyeScore)
{
    fm_status      err;
    fm10000_lane * pLaneExt;
    fm_int         eyeScoreWidth;
    fm_int         eyeScoreHeight;
#if 0
    fm_bool        signalOk;
#endif

    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pEyeScore=%p\n",
                    sw,
                    serDes,
                    (void *)pEyeScore);


    if (pEyeScore == NULL           ||
        serDes < 0                  ||
        serDes >= FM10000_NUM_SERDES)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FM_OK;
        pLaneExt = GET_LANE_EXT(sw, serDes);

        if (pLaneExt->dfeMode == FM_DFE_MODE_STATIC)
        {
#if 0
            err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

            if (signalOk)
            {
                err = fm10000SerDesGetEyeHeightWidth(sw, serDes, &eyeScoreHeight, &eyeScoreWidth);
            }
            else
#endif
            {
                eyeScoreHeight = -1;
                eyeScoreWidth = -1;
                err = FM_OK;
            }

            *pEyeScore = (eyeScoreHeight & 0xFF) |  ((eyeScoreWidth & 0xff) << 8);
        }
        else if (pLaneExt->dfeMode == FM_DFE_MODE_KR)
        {

            *pEyeScore = ((pLaneExt->krExt.eyeScoreHeight) / 4 ) & 0xff;


            *pEyeScore |= (0xff << 8);
        }
        else
        {

            *pEyeScore = ((pLaneExt->dfeExt.eyeScoreHeight) / 4 ) & 0xff;


            *pEyeScore |= (0xff << 8);
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesGetBistUserPattern
 * \ingroup intSerdes
 *
 * \desc            Returns the BIST user pattern for the given serdes. The
 *                  whole pattern is split in two chunks. The lower part is
 *                  returned in pSerDesBistUserPatternLow (used by for the
 *                  custom10, custom20 and custom40 bist submodes and for the
 *                  lower 40 bits of custom80). The high part is returned by
 *                  pSerDesBistUserPatternHigh and it keeps the upper 40 bits
 *                  of the custom80 bist submode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pSerDesBistUserPatternLow points to the caller-allocated
 *                  storage where this function will return the low part of
 *                  the bist user pattern. It may be NULL.
 *
 * \param[out]      pSerDesBistUserPatternHigh points to the caller-allocated
 *                  storage where this function will return the high part of
 *                  the bist user pattern. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesGetBistUserPattern(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint64 *pSerDesBistUserPatternLow,
                                          fm_uint64 *pSerDesBistUserPatternHigh)
{
    fm_status      err;
    fm10000_lane * pLaneExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pSerDesBistUserPatternLow=%p, pSerDesBistUserPatternHigh=%p\n",
                    sw,
                    serDes,
                    (void *) pSerDesBistUserPatternLow,
                    (void *) pSerDesBistUserPatternHigh);


    if (serDes < 0                   ||
        serDes >= FM10000_NUM_SERDES)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FM_OK;
        pLaneExt = GET_LANE_EXT(sw, serDes);

        if (pSerDesBistUserPatternLow != NULL)
        {
            *pSerDesBistUserPatternLow = pLaneExt->bistCustomData0;
        }

        if (pSerDesBistUserPatternHigh != NULL)
        {
            *pSerDesBistUserPatternHigh = pLaneExt->bistCustomData1;
        }

    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerdesSetBistUserPattern
 * \ingroup intSerdes
 *
 * \desc            Sets the BIST user pattern for the given serdes. The lower
 *                  part is passed using pSerDesBistUserPatternLow (used by for the
 *                  custom10, custom20 and custom40 bist submodes and for the
 *                  lower 40 bits of custom80). The high part is passed by
 *                  pSerDesBistUserPatternHigh and it keeps the upper 40 bits
 *                  of the custom80 bist submode. If the value pointed by
 *                  pSerDesBistUserPatternLow is 0, then the whole user pattern
 *                  is set to its default value. If the value pointed by
 *                  pSerDesBistUserPatternHigh is 0, only the high part of the
 *                  then pattern is set to its default value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[out]      pSerDesBistUserPatternLow points to low part of
 *                  the bist user pattern. It may be NULL.
 *
 * \param[out]      pSerDesBistUserPatternHigh points to the high part of
 *                  the bist user pattern. It may be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesSetBistUserPattern(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint64 *pSerDesBistUserPatternLow,
                                          fm_uint64 *pSerDesBistUserPatternHigh)
{
    fm_status      err;
    fm10000_lane * pLaneExt;


    FM_LOG_ENTRY_V2(FM_LOG_CAT_SERDES, serDes,
                    "sw=%d, serDes=%d, pSerDesBistUserPatternLow=%p, pSerDesBistUserPatternHigh=%p\n",
                    sw,
                    serDes,
                    (void *) pSerDesBistUserPatternLow,
                    (void *) pSerDesBistUserPatternHigh);


    if (serDes < 0                   ||
        serDes >= FM10000_NUM_SERDES)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FM_OK;
        pLaneExt = GET_LANE_EXT(sw, serDes);

        if (pSerDesBistUserPatternLow != NULL)
        {
            if (*pSerDesBistUserPatternLow != 0)
            {
                pLaneExt->bistCustomData0 = *pSerDesBistUserPatternLow;
            }
            else
            {

                pLaneExt->bistCustomData0 = FM10000_SERDES_BIST_USER_PATTERN_DEFAULT;
                pLaneExt->bistCustomData1 = FM10000_SERDES_BIST_USER_PATTERN_DEFAULT;


                pSerDesBistUserPatternHigh = NULL;
            }

            *pSerDesBistUserPatternLow = pLaneExt->bistCustomData0;
        }

        if (pSerDesBistUserPatternHigh != NULL)
        {
            if (*pSerDesBistUserPatternHigh != 0)
            {
                pLaneExt->bistCustomData1 = *pSerDesBistUserPatternHigh;
            }
            else
            {

                pLaneExt->bistCustomData1 = FM10000_SERDES_BIST_USER_PATTERN_DEFAULT;
            }
        }
    }

    FM_LOG_EXIT_V2(FM_LOG_CAT_SWITCH, serDes, err);

}




/*****************************************************************************/
/** fm10000SerDesEventHandler
 * \ingroup intSerdes
 *
 * \desc            Function that processes serDes-level interrupts
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       epl is the ID of the EPL on which the event occurred
 *
 * \param[in]       lane is the ID of the lane on which the event occurred
 *
 * \param[in]       serDesIp is the interrupt pending mask for this EPL lane
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000SerDesEventHandler( fm_int    sw,
                                     fm_int    epl,
                                     fm_int    lane,
                                     fm_uint32 serDesIp )
{
    fm_status       err;
    fm_int          serDes;
    fm10000_lane *  pLaneExt;
    fm_smEventInfo  eventInfo;
    fm_bool         signalOk;
    fm_switch *     switchPtr;
    fm_bool         txRdyFlag;
    fm_bool         rxRdyFlag;


    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                         "sw=%d, epl=%d, lane=%d, serDesIp=0x%8.8x\n",
                         sw,
                         epl,
                         lane,
                         serDesIp);

    switchPtr = GET_SWITCH_PTR(sw);


    err = fm10000MapEplLaneToSerdes( sw, epl, lane, &serDes );

    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {

        pLaneExt = GET_LANE_EXT( sw, serDes );

        if (pLaneExt != NULL)
        {

            eventInfo.smType         = pLaneExt->smType;
            eventInfo.lock           = FM_GET_STATE_LOCK( sw );
            eventInfo.dontSaveRecord = FALSE;

            if (FM_GET_BIT( serDesIp, FM10000_SERDES_IP, TxRdy) ||
                FM_GET_BIT( serDesIp, FM10000_SERDES_IP, RxRdy) )
            {

                err = fm10000SerdesGetTxRxReadyStatus(sw, serDes, &txRdyFlag, &rxRdyFlag);

                if (err == FM_OK)
                {
                    eventInfo.eventId = -1;

                    if (txRdyFlag && rxRdyFlag)
                    {
                        eventInfo.eventId = FM10000_SERDES_EVENT_RXTX_PLLS_LOCKED_IND;
                    }
                    else if (txRdyFlag)
                    {
                        eventInfo.eventId = FM10000_SERDES_EVENT_TX_PLL_LOCKED_IND;
                    }
                    else if (rxRdyFlag)
                    {
                        eventInfo.eventId = FM10000_SERDES_EVENT_RX_PLL_LOCKED_IND;
                    }
                    if (eventInfo.eventId >= 0)
                    {
                        err = fmNotifyStateMachineEvent(pLaneExt->smHandle,
                                                &eventInfo,
                                                &pLaneExt->eventInfo,
                                                &pLaneExt->serDes);
                    }
                }
            }


            if ( FM_GET_BIT( serDesIp, FM10000_SERDES_IP, RxSignalOk ) )
            {
                err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

                if (err == FM_OK)
                {
                    if (signalOk == TRUE)
                    {

                        eventInfo.eventId = FM10000_SERDES_EVENT_SIGNALOK_ASSERTED_IND;
                    }
                    else
                    {

                        eventInfo.eventId = FM10000_SERDES_EVENT_SIGNALOK_DEASSERTED_IND;
                    }

                    err = fmNotifyStateMachineEvent(pLaneExt->smHandle,
                                                    &eventInfo,
                                                    &pLaneExt->eventInfo,
                                                    &pLaneExt->serDes);
                }
            }


            TAKE_REG_LOCK( sw );
            err = switchPtr->WriteUINT32( sw,
                                          FM10000_SERDES_IM(epl, lane),
                                          ~pLaneExt->serdesInterruptMask);
            DROP_REG_LOCK( sw );

        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, err);

}




/*****************************************************************************/
/** fm10000MapPhysicalPortToPepSerDes
 * \ingroup intSerdes
 *
 * \desc            Function that maps a physical port to a PEP and SerDes IDs
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       physPort is the ID of the physical port
 *
 * \param[in]       pep is the pointer to a caller-allocated variable where
 *                  this function will return the PEP ID
 *
 * \param[in]       serdes is the pointer to a caller-allocated variable where
 *                  this function will return the serDes ID
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if any of the pointers is NULL or
 *                  the fabricPort is invalid or not a PCIE port
 * \return          FM_ERR_INVALID_PORT if the physical port is not tied to a
 *                  fabric port.
 *
 *****************************************************************************/
fm_status fm10000MapPhysicalPortToPepSerDes( fm_int  sw,
                                             fm_int  physPort,
                                             fm_int *pep,
                                             fm_int *serdes )
{
    fm_int status;
    fm_int fabricPort;

    status = fm10000MapPhysicalPortToFabricPort( sw, physPort, &fabricPort );
    if ( status == FM_OK )
    {
        status =
            fm10000MapFabricPortToPepSerDes( sw, fabricPort, pep, serdes );
    }

    return status;

}




/*****************************************************************************/
/** fm10000MapFabricPortToPepSerDes
 * \ingroup intSerdes
 *
 * \desc            Function that maps a fabric port to a PEP and SerDes IDs
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       fabricPort is the ID of the fabricPort
 *
 * \param[in]       pep is the pointer to a caller-allocated variable where
 *                  this function will return the PEP ID
 *
 * \param[in]       serDes is the pointer to a caller-allocated variable where
 *                  this function will return the serDes ID
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_INVALID_ARGUMENT if any of the pointers is NULL or
 *                  the fabricPort is invalid or not a PCIE port
 *****************************************************************************/
fm_status fm10000MapFabricPortToPepSerDes( fm_int  sw,
                                           fm_int  fabricPort,
                                           fm_int *pep,
                                           fm_int *serDes )
{
    fm_int i;
    fm_int status;

    status = FM_ERR_INVALID_ARGUMENT;
    if ( pep == NULL || serDes == NULL )
    {
        return status;
    }

    for ( i = 0 ; i < FM10000_NUM_SERDES ; i++ )
    {
        if ( fm10000SerdesMap[i].fabricPort == fabricPort )
        {
            if ( fm10000SerdesMap[i].ring == FM10000_SERDES_RING_PCIE )
            {
                *serDes = i;
                *pep = fm10000SerdesGetPepId(sw, i);
                status = FM_OK;
            }
            break;

        }
    }

    return status;

}




/*****************************************************************************/
/** fm10000SerdesSendDfeEventReq
 * \ingroup intSerdes
 *
 * \desc            Send a notification to the dfe level state machine that
 *                  belongs to the specified serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SerDes number on which to operate.
 *
 * \param[in]       eventId is the event to be notified.
 *
 * \return          FM_OK if successful
 *
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *****************************************************************************/
fm_status fm10000SerdesSendDfeEventReq(fm_int    sw,
                                       fm_int    serDes,
                                       fm_int    eventId)
{
    fm_status        err;
    fm10000_lane    *pLaneExt;
    fm_smEventInfo   dfeEventInfo;


    pLaneExt = GET_LANE_EXT(sw,serDes);


    dfeEventInfo.smType  = pLaneExt->dfeExt.smType;
    dfeEventInfo.eventId = eventId;
    dfeEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    dfeEventInfo.dontSaveRecord = FALSE;

    err = fmNotifyStateMachineEvent( pLaneExt->dfeExt.smHandle,
                                     &dfeEventInfo,
                                     &pLaneExt->dfeExt.eventInfo,
                                     &pLaneExt->serDes );
    return err;

}





/*****************************************************************************/
/** fm10000MapEplChannelToLane
 * \ingroup intSerdes
 *
 * \desc            Return corresponding lane given (epl, channel) tuple.
 *
 * \param[in]       sw is the ID of the switch on which to operate.
 *
 * \param[in]       epl is the EPL number.
 *
 * \param[in]       channel is the EPL channel number.
 *
 * \param[out]      lane is a pointer to a caller-allocated area where the
 *                  function will return the lane number.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if epl or channel is out of range.
 * *
 *****************************************************************************/
fm_status fm10000MapEplChannelToLane(fm_int  sw,
                                     fm_int  epl,
                                     fm_int  channel,
                                     fm_int *lane)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                 "sw=%d epl=%d channel=%d lane=%p\n",
                 sw, epl, channel, (void *) lane);

    if ( ( ( epl < 0 ) || ( epl >= FM10000_NUM_EPLS ) ) ||
         ( ( channel < 0 ) || ( channel >= FM10000_PORTS_PER_EPL ) ) )
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
    }


    *lane = channel;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, FM_OK);

}




/*****************************************************************************/
/** fm10000MapEplLaneToChannel
 * \ingroup intSerdes
 *
 * \desc            Return corresponding channel given (epl, lane) tuple.
 *
 * \param[in]       sw is the ID of the switch on which to operate.
 *
 * \param[in]       epl is the EPL number.
 *
 * \param[in]       lane is the EPL lane number.
 *
 * \param[out]      channel is a pointer to a caller-allocated area where the
 *                  function will return the channel number.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if epl or lane is out of range.
 * *
 *****************************************************************************/
fm_status fm10000MapEplLaneToChannel(fm_int  sw,
                                     fm_int  epl,
                                     fm_int  lane,
                                     fm_int *channel)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SERDES,
                 "sw=%d epl=%d lane=%d channel=%p\n",
                 sw, epl, lane, (void *) channel);

    if ( ( ( epl < 0 ) || ( epl >= FM10000_NUM_EPLS ) ) ||
         ( ( lane < 0 ) || ( lane >= FM10000_PORTS_PER_EPL ) ) )
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, FM_ERR_INVALID_ARGUMENT);
    }

    *channel = lane;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SERDES, FM_OK);

}




/*****************************************************************************/
/** fm10000MapEplLaneToSerdes
 * \ingroup intSerdes
 *
 * \desc            Return serdes number given (epl, lane) tuple.
 *
 * \param[in]       sw is the ID of the switch on which to operate.
 *
 * \param[in]       epl is the EPL number.
 *
 * \param[in]       lane is the EPL lane number.
 *
 * \param[out]      serdes is a pointer to a caller-allocated area where the
 *                  function will return the serdes number.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if epl or lane is out of range.
 * *
 *****************************************************************************/
fm_status fm10000MapEplLaneToSerdes(fm_int  sw,
                                    fm_int  epl,
                                    fm_int  lane,
                                    fm_int *serdes)
{
    fm_int i;

    FM_NOT_USED(sw);

    for (i = 0 ; i < FM10000_NUM_SERDES ; i++)
    {
        if ( ( fm10000SerdesMap[i].ring         == FM10000_SERDES_RING_EPL ) &&
             ( fm10000SerdesMap[i].endpoint.epl == epl )                     &&
             ( fm10000SerdesMap[i].physLane     == lane ) )
        {
            *serdes= i;

            return FM_OK;
        }
    }

    return FM_ERR_INVALID_ARGUMENT;

}




/*****************************************************************************/
/** fm10000MapSerdesToSbus
 * \ingroup intSerdes
 *
 * \desc            Return SBUS address and ring given serdes number.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[out]      sbusAddr is a pointer to a caller-allocated area where the
 *                  function will return the SBUS address.
 *
 * \param[out]      ring is a pointer to a caller-allocated area where the
 *                  function will return the ring number.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is out of range
 *
 *****************************************************************************/
fm_status fm10000MapSerdesToSbus(fm_int         sw,
                                 fm_int         serdes,
                                 fm_uint       *sbusAddr,
                                 fm_serdesRing *ring )
{
    FM_NOT_USED(sw);

    if (serdes == FM10000_SERDES_EPL_BCAST)
    {
        *sbusAddr  = 0xFF;
        *ring      = FM10000_SERDES_RING_EPL;
    }
    else if (serdes == FM10000_SERDES_PCIE_BCAST)
    {
        *sbusAddr  = 0xFF;
        *ring      = FM10000_SERDES_RING_PCIE;
    }
    else
    {
        VALIDATE_SERDES(serdes);

        *sbusAddr  = fm10000SerdesMap[serdes].sbusAddr;
        *ring      = fm10000SerdesMap[serdes].ring;
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000MapSerdesToEplLane
 * \ingroup intSerdes
 *
 * \desc            Return (epl, lane) tuple given serdes number.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[out]      epl is a pointer to a caller-allocated area where the
 *                  function will return the EPL number. It may be NULL.
 *
 * \param[out]      lane is a pointer to a caller-allocated area where the
 *                  function will return the lane number. It may be NULL.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is out of range
 *
 *****************************************************************************/
fm_status fm10000MapSerdesToEplLane(fm_int  sw,
                                    fm_int  serdes,
                                    fm_int *epl,
                                    fm_int *lane )
{
    fm_status err;

    FM_NOT_USED(sw);


    err = FM_OK;

    if (serdes >= 0                     &&
        serdes < FM10000_NUM_SERDES     &&
        fm10000SerdesMap[serdes].ring == FM10000_SERDES_RING_EPL )
    {
        if (epl != NULL)
        {
            *epl  = fm10000SerdesMap[serdes].endpoint.epl;
        }
        if (lane != NULL)
        {
            *lane = fm10000SerdesMap[serdes].physLane;
        }
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    return err;

}




/*****************************************************************************/
/** fm10000MapSerdesToPepLane
 * \ingroup intSerdes
 *
 * \desc            Return (pep, lane) tuple given serdes number.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[out]      pep is a pointer to a caller-allocated area where the
 *                  function will return the PEP number. It may be NULL.
 *
 * \param[out]      lane is a pointer to a caller-allocated area where the
 *                  function will return the lane number. It may be NULL.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is out of range
 *
 *****************************************************************************/
fm_status fm10000MapSerdesToPepLane(fm_int  sw,
                                    fm_int  serdes,
                                    fm_int *pep,
                                    fm_int *lane )
{
    fm_status err;
    fm_int    pepId;
    fm_int    laneNum;

    FM_NOT_USED(sw);

    err = FM_OK;

    if (serdes >= 0                     &&
        serdes < FM10000_NUM_SERDES     &&
        fm10000SerdesMap[serdes].ring == FM10000_SERDES_RING_PCIE )
    {
        pepId  = fm10000SerdesGetPepId(sw, serdes);
        laneNum = fm10000SerdesMap[serdes].physLane;
    }
    else
    {
        pepId  = 0;
        laneNum = 0;
        err = FM_ERR_INVALID_ARGUMENT;
    }

    if (pep != NULL)
    {
        *pep = pepId;
    }
    if (lane != NULL)
    {
        *lane = laneNum;
    }

    return err;

}




/*****************************************************************************/
/** fm10000MapSerdesToLogicalPort
 * \ingroup intSerdes
 *
 * \desc            Return logical port given serdes number.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       serdes is the serdes number.
 *
 * \param[out]      port is a pointer to a caller-allocated area where the
 *                  function will return the logical port.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT serdes number is out of range
 *
 *****************************************************************************/
fm_status fm10000MapSerdesToLogicalPort(fm_int  sw,
                                        fm_int  serdes,
                                        fm_int *port)
{
    fm_status err;
    fm_int physPort;
    fm_int fabricPort;
    fm_int mappedSw;

    VALIDATE_SERDES(serdes);

    fabricPort = fm10000SerdesMap[serdes].fabricPort;

    err = fm10000MapFabricPortToPhysicalPort( sw, fabricPort, &physPort );
    if ( err == FM_OK )
    {
        err = fmPlatformMapPhysicalPortToLogical( sw, physPort, &mappedSw, port );
    }

    return err;

}




/*****************************************************************************/
/** fm10000MapPortLaneToSerdes
 * \ingroup intSerdes
 *
 * \desc            Return serdes number given (port, laneNum) tuple.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       laneNum is the relative number of the logical port for
 *                  multi-lane ports.
 *
 * \param[out]      serdes points to a caller-supplied location in which
 *                  the serdes number will be returned.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not UP yet
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT invalid pointer arguments
 * \return          FM_ERR_INVALID_PORT_MAC invalid MAC ID
 * \return          FM_ERR_INVALID_PORT_LANE invalid lane ID
 *
 *****************************************************************************/
fm_status fm10000MapPortLaneToSerdes( fm_int  sw,
                                      fm_int  port,
                                      fm_int  laneNum,
                                      fm_int *serdes )
{
    fm_status err;
    fm_int    phySwitch;
    fm_int    phyPort;
    fm_int    epl;
    fm_int    eplLane;
    fm_serdesRing ring;

    if ( serdes == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {

        err = fmPlatformMapLogicalPortToPhysical( sw,
                                                  port,
                                                  &phySwitch,
                                                  &phyPort );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, port, err );


        err = fm10000MapPhysicalPortToSerdes( phySwitch,
                                              phyPort,
                                              serdes,
                                              &ring );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, port, err );

        if ( ring == FM10000_SERDES_RING_EPL )
        {
            err = fm10000MapPhysicalPortToEplChannel(sw,
                                                     phyPort,
                                                     &epl,
                                                     &eplLane);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, port, err );

            err = fmPlatformMapPortLaneToEplLane( sw,
                                                  port,
                                                  laneNum,
                                                  &eplLane );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, port, err );

            err = fm10000MapEplLaneToSerdes(sw,
                                            epl,
                                            eplLane,
                                            serdes);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, port, err );

            FM_LOG_DEBUG(FM_LOG_CAT_SERDES,
                         "sw = %d, port = %d, laneNum = %d : phyport = %d "
                         "epl = %d, eplLane = %d, serdes = %d\n",
                         sw, port, laneNum, phyPort, epl, eplLane, *serdes);
        }
        else
        {
            *serdes += laneNum;
        }
    }

ABORT:

    return err;

}




/*****************************************************************************/
/** fm10000MapFabricPortToSerdes
 * \ingroup intSerdes
 *
 * \desc            Return serdes number given faabric port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       fabricPort is the fabric port number.
 *
 * \param[out]      serdes it the pointer to a caller-allocated area where
 *                  the function will return the Serdes ID.
 *
 * \param[out]      serdesRing it the pointer to a caller-allocated area where
 *                  the function will return the Serdes ring type (see
 *                  ''fm_serdesRing'')
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch is invalid
 *
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is NULL
 *****************************************************************************/
fm_status fm10000MapFabricPortToSerdes( fm_int         sw,
                                        fm_int         fabricPort,
                                        fm_int        *serdes,
                                        fm_serdesRing *serdesRing )
{
    fm_int    i;
    fm_status status;

    if (serdes == NULL || serdesRing == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status =  FM_ERR_INVALID_ARGUMENT;
    for (i = 0 ; i < FM10000_NUM_SERDES ; i++)
    {
        if ( fm10000SerdesMap[i].fabricPort == fabricPort )
        {
            *serdes = i;
            *serdesRing = fm10000SerdesMap[i].ring;
            return FM_OK;
        }
    }

    return status;

}




/*****************************************************************************/
/** fm10000MapPhysicalPortToSerdes
 * \ingroup intSerdes
 *
 * \desc            Return serdes number given physical port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       physPort is the physical port number.
 *
 * \param[out]      serdes it the pointer to a caller-allocated area where
 *                  the function will return the Serdes ID.
 *
 * \param[out]      serdesRing it the pointer to a caller-allocated area where
 *                  the function will return the Serdes ring type (see
 *                  ''fm_serdesRing'')
 *
 * \return          FM_OK if successful
 *
 * \return          FM_ERR_INVALID_SWITCH if the switch is invalid
 *
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is NULL
 *****************************************************************************/
fm_status fm10000MapPhysicalPortToSerdes( fm_int        sw,
                                          fm_int        physPort,
                                          fm_int        *serdes,
                                          fm_serdesRing *serdesRing )
{
    fm_int    i;
    fm_int    fabricPort;
    fm_status status;

    if (serdes == NULL || serdesRing == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status = fm10000MapPhysicalPortToFabricPort( sw, physPort, &fabricPort );

    if ( status == FM_OK )
    {
        for (i = 0 ; i < FM10000_NUM_SERDES ; i++)
        {
            if ( fm10000SerdesMap[i].fabricPort == fabricPort )
            {
                *serdes = i;
                *serdesRing = fm10000SerdesMap[i].ring;
                return FM_OK;
            }
        }

        status =  FM_ERR_INVALID_ARGUMENT;

    }

    return status;

}




/*****************************************************************************/
/** fm10000MapLogicalPortToSerdes
 * \ingroup intSerdes
 *
 * \desc            Return serdes number given logical port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      serdes is the pointer to a caller-allocated area where
 *                  the function will return the Serdes ID.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_INVALID_ARGUMENT if serdes is NULL
 *
 *****************************************************************************/
fm_status fm10000MapLogicalPortToSerdes(fm_int  sw,
                                        fm_int  port,
                                        fm_int *serdes)
{

    return fm10000MapPortLaneToSerdes(sw, port, 0, serdes);

}




/*****************************************************************************/
/** fm10000MapPepToLogicalPort
 * \ingroup intSwitch
 *
 * \desc            maps a PCIE Express endpoint ID to the associated logical
 *                  port.
 *                  Note well: the tables used by this function to perform
 *                  the pep to port mapping are constant, so no lock is
 *                  required to protect its execution.
 *
 * \param[in]       sw is the ID of the switch on which to operate
 *
 * \param[in]       pep os the ID of PCIE endpoint on which to operate
 *
 * \param[out]      port is the pointer to a caller-allocated area where this
 *                  function will return the logical port ID
 *
 * \return          FM_OK if request succeeded.
 *
 * \return          FM_ERR_NOT_FOUND if this PEP ID was not found in the port
 *                  map
 *
 * \return          FM_ERR_INVALID_ARGUMENT if the port pointer is invalid
 *****************************************************************************/
fm_status fm10000MapPepToLogicalPort(fm_int  sw,
                                     fm_int  pep,
                                     fm_int *port )
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     i;
    fm_int     fabricPort;

    if ( port == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switchPtr = GET_SWITCH_PTR( sw );




    status = FM_ERR_NOT_FOUND;
    for (i = 0 ; i < FM10000_NUM_SERDES ; i++)
    {
        if ( fm10000SerdesMap[i].ring == FM10000_SERDES_RING_PCIE &&
             fm10000SerdesMap[i].endpoint.pep  == pep )
        {
            fabricPort = fm10000SerdesMap[i].fabricPort;
            status = fm10000MapFabricPortToLogicalPort( sw, fabricPort, port);
            break;
        }
    }

    return status;

}




/*****************************************************************************/
/** fm10000MapLogicalPortToPep
 * \ingroup intSwitch
 *
 * \desc            maps logical port to the associated PCIE Express
 *                  endpoint ID.
 *
 * \param[in]       sw is the ID of the switch on which to operate
 *
 * \param[in]       port is the logical port on which to operate
 *
 * \param[out]      pep is the pointer to a caller-allocated area where this
 *                  function will return the PCIE Express endpoint ID.
 *
 * \return          FM_OK if request succeeded.
 *
 * \return          FM_ERR_NOT_FOUND if this port was not found in the
 *                  PCIE Express map.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if the pep pointer is invalid
 *****************************************************************************/
fm_status fm10000MapLogicalPortToPep( fm_int sw, fm_int port, fm_int *pep )
{
    fm_status status;
    fm_int    i;
    fm_int    fabricPort;
    fm_int    logicalPort;
    fm_bool   pepFound;

    if ( pep == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status   = FM_OK;
    pepFound = FALSE;

    for (i = 0 ; i < FM10000_NUM_SERDES ; i++)
    {
        if ( fm10000SerdesMap[i].ring == FM10000_SERDES_RING_PCIE)
        {
            fabricPort = fm10000SerdesMap[i].fabricPort;

            status = fm10000MapFabricPortToLogicalPort(sw,
                                                       fabricPort,
                                                       &logicalPort);
            if ( status == FM_ERR_INVALID_PORT )
            {


                status = FM_OK;
                continue;
            }

            if (logicalPort == port)
            {
                *pep = fm10000SerdesGetPepId(sw, i);
                pepFound = TRUE;

                break;
            }
        }
    }

    if (pepFound == FALSE)
    {
        status = FM_ERR_NOT_FOUND;
    }
    return status;

}




