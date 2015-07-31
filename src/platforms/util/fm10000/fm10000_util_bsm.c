/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_util_bsm.c
 * Creation Date:   April 2015
 * Description:     BSM utility shared functions.
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

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <fm_sdk_fm10000_int.h>
#include <platforms/util/fm10000/fm10000_util_bsm.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/* dbg BSM_SCRATCH nvm version supported */ 
#define NVM_VERSION_MAX     0xFFFF

/* BSM_SCRATCH registers definition */
#define BSM_STATUS                  400
#define EEPROM_IMAGE_VERSION        401
#define MASTER_FW_VERSION           402
#define SERDES_FW_VERSION           403
#define SERDES_STATUS_1             404
#define SERDES_STATUS_2             405
#define SERDES_STATUS_3             406
#define SERDES_STATUS_4             407
#define PCIE_MASTER_STATUS          408
#define PCIE_SERDES_STATUS          409
#define DE_COLD_RESET_STATUS        430
#define SBUS_RESET_STATUS           431
#define MEMORY_REPAIR_STATUS        432
#define MEMORY_INIT_STATUS          433
#define PCIE_PCS_DIS_STATUS         434
#define PCIE_MASTER_FW_DL_STATUS    435
#define PCIE_FW_CHECK_STATUS        436
#define PCIE_SERDES_FW_DL_STATUS    437
#define PCIE_SERDES_INIT_STATUS     438
#define PCIE_PCS_EN_STATUS          439
#define PCIE_DE_WARM_RESET_STATUS   440
#define PCIE_ISR_STATUS_0           441
#define PCIE_ISR_STATUS_1           442
#define PCIE_ISR_STATUS_2           443
#define PCIE_ISR_STATUS_3           444
#define PCIE_ISR_STATUS_4           445
#define PCIE_ISR_STATUS_5           446
#define PCIE_ISR_STATUS_6           447
#define PCIE_ISR_STATUS_7           448
#define PCIE_ISR_STATUS_8           449
#define SERDES_OOR_STATUS_PASS_1    450
#define SERDES_OOR_STATUS_PASS_2    451

/* BSM_STATUS step bits */
#define FM10000_BSM_STATUS_l_Step   0
#define FM10000_BSM_STATUS_h_Step   7

#define TIME_PASSED(a, b)            \
    (((a.sec < b.sec) || ((a.sec == b.sec) && (a.usec < b.usec))) ? 0 : 1)

#define LtssmToStr(value, regData) \
    RegValueToStr(value, ltssmRegMap, FM_NENTRIES(ltssmRegMap), regData)
#define BsmStatusToStr(value, regData) \
    RegValueToStr(value, bsmStatusMap, FM_NENTRIES(bsmStatusMap), regData)

/* structure which holds the register value to msg mapping */
typedef struct _regStrMap {
    fm_uint32   value;
    fm_uint32   mask;
    fm_text     msg;
} regStrMap;

/* structure holds dbg functions NVM versions up to maxVersionSupported */
typedef struct _nvmVersionRegAccess
{
    fm_uint   maxVersionSupported;
    fm_status (*funcBsmScratchDump)(fm_int                    sw,
                                    fm_registerReadUINT32Func readFunc,
                                    fm_uint32                 regMask);
    fm_status (*funcBsmStatusPoll)(fm_int                    sw,
                                   fm_registerReadUINT32Func readFunc,
                                   fm_uint32                 miliSec);
    fm_status (*funcLtssmPoll)(fm_int                    sw,
                               fm_registerReadUINT32Func readFunc,
                               fm_int                    pep, 
                               fm_uint32                 miliSec);
} nvmVersionRegAccess;

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
/* ltssm register values mapping */ 
static const regStrMap ltssmRegMap[] =
{
        {0x00000000, 0xFFFFFFFF, "DETECT_QUIET(0x00)"},
        {0x00000001, 0xFFFFFFFF, "DETECT_ACT(0x01)"},
        {0x00000002, 0xFFFFFFFF, "POLL_ACTIVE(0x02)"},
        {0x00000003, 0xFFFFFFFF, "POLL_COMPLIANCE(0x03)"},
        {0x00000004, 0xFFFFFFFF, "POLL_CONFIG(0x04)"},
        {0x00000005, 0xFFFFFFFF, "PRE_DETECT_QUIET(0x05)"},
        {0x00000006, 0xFFFFFFFF, "DETECT_WAIT(0x06)"},
        {0x00000007, 0xFFFFFFFF, "CFG_LINKWD_START(0x07)"},
        {0x00000008, 0xFFFFFFFF, "CFG_LINKWD_ACEPT(0x08)"},
        {0x00000009, 0xFFFFFFFF, "CFG_LANENUM_WAIT(0x09)"},
        {0x0000000A, 0xFFFFFFFF, "CFG_LANENUM_ACEPT(0x0A)"},
        {0x0000000B, 0xFFFFFFFF, "CFG_COMPLETE(0x0B)"},
        {0x0000000C, 0xFFFFFFFF, "CFG_IDLE(0x0C)"},
        {0x0000000D, 0xFFFFFFFF, "RCVRY_LOCK(0x0D)"},
        {0x0000000E, 0xFFFFFFFF, "RCVRY_SPEED(0x0E)"},
        {0x0000000F, 0xFFFFFFFF, "RCVRY_RCVRCFG(0x0F)"},
        {0x00000010, 0xFFFFFFFF, "RCVRY_IDLE(0x10)"},
        {0x00000020, 0xFFFFFFFF, "RCVRY_EQ0(0x20)"},
        {0x00000021, 0xFFFFFFFF, "RCVRY_EQ1(0x21)"},
        {0x00000022, 0xFFFFFFFF, "RCVRY_EQ2(0x22)"},
        {0x00000023, 0xFFFFFFFF, "RCVRY_EQ3(0x23)"},
        {0x00000011, 0xFFFFFFFF, "L0(0x11)"},
        {0x00000012, 0xFFFFFFFF, "L0S(0x12)"},
        {0x00000013, 0xFFFFFFFF, "L123_SEND_EIDLE(0x13)"},
        {0x00000014, 0xFFFFFFFF, "L1_IDLE(0x14)"},
        {0x00000015, 0xFFFFFFFF, "L2_IDLE(0x15)"},
        {0x00000016, 0xFFFFFFFF, "L2_WAKE(0x16)"},
        {0x00000017, 0xFFFFFFFF, "DISABLED_ENTRY(0x17)"},
        {0x00000018, 0xFFFFFFFF, "DISABLED_IDLE(0x18)"},
        {0x00000019, 0xFFFFFFFF, "DISABLED(0x19)"},
        {0x0000001A, 0xFFFFFFFF, "LPBK_ENTRY(0x1A)"},
        {0x0000001B, 0xFFFFFFFF, "LPBK_ACTIVE(0x1B)"},
        {0x0000001C, 0xFFFFFFFF, "LPBK_EXIT(0x1C)"},
        {0x0000001D, 0xFFFFFFFF, "LPBK_EXIT_TIMEOUT(0x1D)"},
        {0x0000001E, 0xFFFFFFFF, "HOT_RESET_ENTRY(0x1E)"},
        {0x0000001F, 0xFFFFFFFF, "HOT_RESET(0x1F)"},
        {0x00000000, 0x00000000, "(Unrecognized)"} /* must be the last one */
};

/* BSM_STATUS register values mapping */
const regStrMap bsmStatusMap[] =
{
        {0x0000000A, 0xFFFFFFFF, "Cold Reset De-Asserted"},
        {0x0000000B, 0xFFFFFFFF, "EPL/PCIe SBUS Controller Reset (Starting)"},
        {0x0001000B, 0xFFFFFFFF, "EPL/PCIe SBUS Controller Reset (Complete)"},
        {0x0000000C, 0xFFFFFFFF, "Memory repair step starting"},
        {0x0001000C, 0xFFFFFFFF, "Memory repair is running"},
        {0x0011000C, 0xFFFFFFFF, "Memory repair completed: pass"},
        {0x0021000C, 0xFFFFFFFF, "Memory repair completed: fail"},
        {0x0031000C, 0xFFFFFFFF, "Memory repair did not complete"},
        {0x0041000C, 0xFFFFFFFF, "Memory repair skipped (config option)"},
        {0x0000000D, 0xFFFFFFFF, "Memory init starting"},
        {0x0010000D, 0xFFFFFFFF, "Memory init running"},
        {0x0110000D, 0xFFFFFFFF, "Memory init stopped"},
        {0x0009000D, 0xFFFFFFFF, "Disable PCIe PCS interrupts starting"},
        {0x0009000D, 0x000FFFFF, "PCIe PCS interrupts disabled,"},
        {0x0101000E, 0xFFFFFFFF, "PCIe Master SPICO Pre-firmware download"},
        {0x0102000E, 0xFFFFFFFF, "PCIe Master SPICO firmware download"},
        {0x0103000E, 0xFFFFFFFF, "PCIe Master SPICO Post-firmware download"},
        {0x0108000E, 0xFFFFFFFF, "PCIe Master SPICO firmware download complete and successful"},
        {0x0108FFFE, 0x0000FFFF, "PCIe Master SPICO Firmware download error: "},
        {0x0001000E, 0xFFFFFFFF, "PCIe SerDes Pre-firmware download"},
        {0x0002000E, 0xFFFFFFFF, "PCIe SerDes firmware download"},
        {0x0003000E, 0xFFFFFFFF, "PCIe SerDes Post-firmware download"},
        {0x0004000E, 0xFFFFFFFF, "PCIe SerDes firmware Verification (Version and CRC)"},
        {0x0004000E, 0xF00FFFFF, "PCIe SerDes Firmware Verification Status"},
        {0x0008000E, 0xFFFFFFFF, "PCIe SerDes Firmware download complete and successful"},
        {0x0008FFFE, 0x000FFFFF, "PCIe SerDes Firmware download error."},
        {0x0000000F, 0xFFFFFFFF, "PCIe SerDes Initialization starting"},
        {0x0001000F, 0xFFFFFFFF, "PCIe SerDes RX & TX reflck ratio"},
        {0x0002000F, 0xFFFFFFFF, "PCIe SerDes Equalization seed"},
        {0x0003000F, 0xFFFFFFFF, "PCIe SerDes Configure iCal to run on first equalization"},
        {0x0004000F, 0xFFFFFFFF, "PCIe SerDes Complete and successful"},
        {0x0009000F, 0xFFFFFFFF, "Enable PCIe PCS interrupts starting"},
        {0x0009000F, 0x000FFFFF, "PCIe PCS interrupts enabled for core"},
        {0x0001FFFF, 0xFFFFFFFF, "PCIe SerDes RX & TX reflck ratio, ERROR"},
        {0x0002FFFF, 0xFFFFFFFF, "PCIe SerDes Equalization seed, ERROR"},
        {0x0003FFFF, 0xFFFFFFFF, "PCIe SerDes Configure iCal to run on first equalization, ERROR"},
        {0x00000010, 0xFFFFFFFF, "PCIe warm reset deassert starting"},
        {0x00010010, 0xFFFFFFFF, "PCIe warm reset deassert completed"},
        {0x000F0011, 0xFFFFFFFF, "ISR starting"},
        {0x00000011, 0xFFF0FFFF, "Interrupt Host Y"},
        {0x01000011, 0xFFF0FFFF, "Interrupt Processing, OutOfReset"},
        {0x02000011, 0xFFF0FFFF, "Interrupt Processing, VPD"},
        {0x04000011, 0xFFF0FFFF, "Interrupt Processing, HotReset"},
        {0x08000011, 0xFFF0FFFF, "Interrupt Processing, DeviceStateChange"},
        {0x10000011, 0xFFF0FFFF, "Interrupt Processing, DataPathReset"},
        {0x20000011, 0xFFF0FFFF, "Interrupt Processing, PFLR"},
        {0x00000111, 0x0000FFFF, "Interrupt Complete, OutOfReset"},
        {0x00000211, 0x0000FFFF, "Interrupt Complete, VPD"},
        {0x00000411, 0x0000FFFF, "Interrupt Complete, HotReset"},
        {0x00000811, 0x0000FFFF, "Interrupt Complete, DeviceStateChange"},
        {0x00001011, 0x0000FFFF, "Interrupt Complete, DataPathReset"},
        {0x00002011, 0x0000FFFF, "Interrupt Complete, PFLR"},
        {0x00000000, 0x00000000, "(Unrecognized)"}   /* must be the last one */
};

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static fm_status DbgDumpBsmScratchDefault(fm_int                    sw,
                                          fm_registerReadUINT32Func readFunc,
                                          fm_uint32                 regMask);
static fm_status DbgBsmStatusRegPollDefault(fm_int                    sw,
                                            fm_registerReadUINT32Func readFunc,
                                            fm_uint32                 miliSec);
static fm_status DbgLtssmRegPollDefault(fm_int                    sw,
                                        fm_registerReadUINT32Func readFunc,
                                        fm_int                    pep,
                                        fm_uint32                 miliSec);

/* nvm image dbg functions entries array*/
static const nvmVersionRegAccess fm10000NvmAccess[] =
{
        {
                .maxVersionSupported    = NVM_VERSION_MAX - 1,
                .funcBsmScratchDump     = DbgDumpBsmScratchDefault,
                .funcBsmStatusPoll      = DbgBsmStatusRegPollDefault,
                .funcLtssmPoll          = DbgLtssmRegPollDefault,
        },
        {
                .maxVersionSupported    = NVM_VERSION_MAX,
                .funcBsmScratchDump     = NULL,
                .funcBsmStatusPoll      = NULL,
                .funcLtssmPoll          = NULL,
        },
};

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** DumpBsmSerdesStatusRegister
 * \ingroup intDiagReg
 *
 * \desc            Function dumps detailed serdes status info based on two 
 *                  register values SERDES_STATUS_1, SERDES_STATUS_2
 *
 * \param[in]       value 32 bits SERDES_STATUS_1 register value.
 * 
 * \param[in]       value2 32 bits SERDES_STATUS_2 register value.
 *
 * \return          None.
 *
 *****************************************************************************/
static void DumpBsmSerdesStatusRegister(fm_uint32 value, fm_uint32 value2)
{
    fm_int i;
    fm_int lane;
    
    for (i = 0 ; i < 4 ; i++)
    {
        for (lane = 0 ; lane < 8 ; lane++)
        {
            FM_LOG_PRINT("\n      PCIE%d.LANE[%d] : %s   ",
                         i,
                         lane,
                         ((value >> (8 * i + lane)) & 0x01) ? "pass" : "fail");
        }
    }
    FM_LOG_PRINT("\n      PCIE4.LANE[0] : %s   ",
                 (value2 & 0x01) ? "pass" : "fail");
    FM_LOG_PRINT("\n");
    
}   /* end DumpBsmSerdesStatusRegister */




/*****************************************************************************/
/** RegValueToStr
 * \ingroup intDiagReg
 *
 * \desc            function returns the register value data entry matching 
 *                  the register value or register masked value.
 *
 * \param[in]       value register value to be searched in the regMap array.
 * 
 * \param[in]       regMap register mapping entries array.
 * 
 * \param[in]       size of the regMap array.
 * 
 * \param[out]      regData register data structure pointer.
 *
 * \return          FM_OK if successfuly found mapped value.
 * \return          FM_FAIL if matching entry could not be find .
 *
 *****************************************************************************/
static fm_status RegValueToStr(fm_uint32       value,
                               const regStrMap regMap[],
                               fm_int          size,
                               regStrMap *     regData)
{
    fm_status status;
    fm_int    cnt;

    status = FM_FAIL;
    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if ( (value == regMap[cnt].value) ||
             ((value & regMap[cnt].mask) == regMap[cnt].value) )
        {
            regData->value = regMap[cnt].value;
            regData->mask  = regMap[cnt].mask;
            regData->msg   = regMap[cnt].msg;
            return FM_OK;
        }
    }
    return FM_FAIL;
    
}   /* end RegValueToStr */




/*****************************************************************************/
/** DumpBsmStatusRegister
 * \ingroup intDiagReg
 *
 * \desc            dumps detailed BSM_STATUS info based on the register value.
 *
 * \param[in]       msg register name.
 *
 * \param[in]       reg register number.
 * 
 * \param[in]       bsmStatus BSM_STATUS register value to be translated.
 *
 * \return          None.
 *
 *****************************************************************************/
static void DumpBsmStatusRegister(fm_text   msg,
                                  fm_uint32 reg,
                                  fm_uint32 bsmStatus)
{
    fm_uint32 bsmBootStep;
    fm_uint   core;
    fm_uint   host;
    fm_int    i;
    regStrMap bsmRegData;
    
    FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x ", msg, reg, bsmStatus);
    
    bsmBootStep    = FM_GET_FIELD(bsmStatus, FM10000_BSM_STATUS, Step);
    
    if (BsmStatusToStr(bsmStatus, &bsmRegData) == FM_OK)
    {
        FM_LOG_PRINT("\n    0x%08x : %s", bsmStatus, bsmRegData.msg);
        if (bsmRegData.value != bsmStatus)
        {
            /* must extract additional info from bsmStatus value*/
            switch (bsmBootStep)
            {
                case 0x0D:
                    core = (bsmStatus >> 20 ) & 0xFFF;
                    FM_LOG_PRINT("core bitmap  0x%03x", core);
                    for (i = 0 ; i < 8 ; i++)
                    {
                        FM_LOG_PRINT("\n\tcore[%d] %s",
                                    i,
                                    ((core >> i) & 0x01) ? "enabled" : "disabled"); 
                    }
                    break;
    
                case 0x0E:
                    FM_LOG_PRINT("\n    version ");
                    switch ((bsmStatus >> 24 ) & 0xF)
                    {
                        case 1:
                            FM_LOG_PRINT("Master OK, ");
                            break;
                        case 2:
                            FM_LOG_PRINT("All Serdes OK, ");
                            break;
                        default:
                            FM_LOG_PRINT("<unspecified>, ");
                    }
                    switch ((bsmStatus >> 20 ) & 0xF)
                    {
                        case 1:
                            FM_LOG_PRINT("Master OK, ");
                            break;
                        case 2:
                            FM_LOG_PRINT("All Serdes OK, ");
                            break;
                        default:
                            FM_LOG_PRINT("<unspecified>, ");
                    }
                    break;
    
                case 0x0F:
                    core = (bsmStatus >> 20 ) & 0xFFF;
                    for (i = 0 ; i < 8 ; i++)
                    {
                        FM_LOG_PRINT("\n\tPEP[%d] %s",
                                    i,
                                    (((core >> i) & 0x01) == 1) ? "enabled" : "disabled"); 
                    }
                    break;
    
                case 0x11:
                    host = (bsmStatus >> 16 ) & 0xF;
                    FM_LOG_PRINT(", host %d", host);
                    if (host > 8)
                    {
                        FM_LOG_PRINT("\nWARNING unexpected host number %d\n", host); 
                    }
                    break;
    
                default: 
                    break;
    
            }
        }
    }
    else
    {
        /* register status value unrecognized */ 
        FM_LOG_PRINT(" (Unrecognized)");
    }
    
}   /* end DumpBsmStatusRegister */




/*****************************************************************************/
/** DbgLtssmRegPollDefault
 * \ingroup intDiagReg
 *
 * \desc            NVM default function version for dbg LTSSM registers 
 *                  polling.
 *                  Function examines the register value for duration 
 *                  defined in miliSec.
 *                  Register value change detection displays the corresponding
 *                  mapped message.
 *                  For upper NVM versions, corresponding functions could be
 *                  defined in fm10000NvmAccess array.
 *
 * \param[in]       sw is the switch the ltssm will be polled.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 * 
 * \param[in]       pep is the PEP to poll LTSSM for, use -1 to poll all PEPs.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DbgLtssmRegPollDefault(fm_int                    sw,
                                        fm_registerReadUINT32Func readFunc,
                                        fm_int                    pep, 
                                        fm_uint32                 miliSec)
{
    fm_status    err;
    fm_uint      nvmVer;
    fm_uint32    ltssmReg;
    fm_uint32    linkCtrlReg;
    fm_timestamp tTresh;
    fm_timestamp tNow;
    fm_timestamp tDiff;
    fm_timestamp tStart;
    fm_int       i;
    fm_uint32    iter;
    fm_uint32    ltssm;
    fm_uint32    genWidth;
    fm_uint32    ltssmValues[10];
    fm_uint32    genWidthValues[10];
    regStrMap    ltssmRegDataNew;
    regStrMap    ltssmRegDataOld;
    fm_uint32    devCfg;
    fm_int       start;
    fm_int       end;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }
    
    iter = 0;
    tTresh.sec = miliSec / 1000;
    tTresh.usec = (miliSec % 1000) * 1000;
    
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nLTSSM status registers poll");
    
    /* get nvmVersion */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_LOG_PRINT("\n%-25s[%d]      : %02x.%02x (0x%08x)",
                 "EEPROM VERSION",
                 EEPROM_IMAGE_VERSION,
                 (nvmVer & 0xFF00) >> 8,
                 (nvmVer & 0x00FF),
                 nvmVer);

    FM_LOG_PRINT("\n");

    FM_CLEAR(ltssmValues);
    FM_CLEAR(genWidthValues);
    
    err = fmGetTime(&tStart);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    fmSubTimestamps(&tStart, &tStart, &tDiff);
    FM_LOG_PRINT("\n%-10s %-4s %-25s %-25s",
                 "t(s.us)",
                 "pep",
                 "last state",
                 "new state");
    FM_LOG_PRINT("\n%-10s %-4s %-25s %-25s",
                  "---------",
                  "---",
                  "----------------------",
                  "----------------------");

    err = readFunc(sw, FM10000_DEVICE_CFG(), &devCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Determine the if all PEPs need to be scanned or just one */
    if (pep < 0 || pep > 8)
    {
        start = 1;
        end = 9;
    }
    else
    {
        start = pep+1;
        end = start;
    }

    do
    {
        err = fmGetTime(&tNow);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        for ( i = start ; i <= end ; i++ )
        {
            /* Execute on enabled PEPs only */
            if ( (devCfg & (1 << (i-1 + FM10000_DEVICE_CFG_b_PCIeEnable_0))) == 0)
            {
                continue;
            }

            ltssmReg    = 0x00201CA | (i << 20);
            linkCtrlReg = 0x0020020 | (i << 20);
            
            err = readFunc(sw, ltssmReg, &ltssm);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            
            err = readFunc(sw, linkCtrlReg, &genWidth);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            
            ltssm    &= 0x3F; 
            genWidth = (genWidth >> 16) & 0x1FF;
            
            if (ltssmValues[i - 1] != ltssm)
            {
                LtssmToStr(ltssm, &ltssmRegDataNew);
                LtssmToStr(ltssmValues[i - 1], &ltssmRegDataOld);
                FM_LOG_PRINT("\n%02lld.%06lld  %3d  %-22s -> %-25s",
                             tDiff.sec,
                             tDiff.usec,
                             (i - 1),
                             ltssmRegDataOld.msg,
                             ltssmRegDataNew.msg);
            }
            if (genWidthValues[i - 1] != genWidth)
            {
                FM_LOG_PRINT("\n%02lld.%06lld  %3d  Gen%d x%d %-14s -> Gen%d x%d",
                             tDiff.sec,
                             tDiff.usec,
                             (i - 1),
                             genWidthValues[i - 1] & 0xF,
                             (genWidthValues[i - 1] >> 4) & 0x1F,
                             " ",
                             genWidth & 0xF,
                             (genWidth >> 4) & 0x1F);
            }
            
            ltssmValues[i - 1]    = ltssm;
            genWidthValues[i - 1] = genWidth;
            
        }
        fmSubTimestamps(&tNow, &tStart, &tDiff);
        
        iter++;

    } while (TIME_PASSED(tDiff, tTresh) == 0);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Report completed iterations %d in %lld.%06lld, seconds",
                 iter,
                 tDiff.sec,
                 tDiff.usec);
    FM_LOG_PRINT("\n\n");

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end DbgLtssmRegPollDefault */




/*****************************************************************************/
/** DbgBsmStatusRegPollDefault
 * \ingroup intDiagReg
 *
 * \desc            NVM default function version for BSM_STATUS register polling.
 *                  Function examines the register value for duration defined 
 *                  in miliSec.
 *                  Register value change detection displays the corresponding
 *                  mapped message.
 *                  For upper NVM versions, corresponding functions could be
 *                  defined in fm10000NvmAccess array.
 *
 * \param[in]       sw is the switch BSM_STATUS will be examined.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DbgBsmStatusRegPollDefault(fm_int                    sw,
                                            fm_registerReadUINT32Func readFunc,
                                            fm_uint32                 miliSec)
{
    fm_status    err;
    fm_uint32    nvmVer;
    fm_uint32    bsmStatus;
    fm_uint32    bsmStatusOld;
    fm_uint32    iter;
    fm_timestamp tTresh;
    fm_timestamp tNow;
    fm_timestamp tDiff;
    fm_timestamp tStart;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }
    
    iter = 0;
    tTresh.sec = miliSec / 1000;
    tTresh.usec = (miliSec % 1000) * 1000;

    /* get nvmVersion */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nPolling BSM_STATUS register for %lld.%06lld seconds",
                   tTresh.sec,
                   tTresh.usec);
    
    FM_LOG_PRINT("\n%-25s[%d]      : %02x.%02x (0x%08x)",
                 "EEPROM VERSION",
                 EEPROM_IMAGE_VERSION,
                 (nvmVer & 0xFF00) >> 8,
                 (nvmVer & 0x00FF),
                 nvmVer);

    FM_LOG_PRINT("\n");
    
    err = fmGetTime(&tStart);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    fmSubTimestamps(&tStart, &tStart, &tDiff);
    
    bsmStatusOld = 0x00;
    do
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(BSM_STATUS), &bsmStatus);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        if (bsmStatusOld != bsmStatus)
        {
            FM_LOG_PRINT("%02lld.%06lld ", tDiff.sec, tDiff.usec);
            DumpBsmStatusRegister("BSM_STATUS", BSM_STATUS, bsmStatus);
        }

        err = fmGetTime(&tNow);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        fmSubTimestamps(&tNow, &tStart, &tDiff);
        
        bsmStatusOld = bsmStatus;
        iter++;
    } while (TIME_PASSED(tDiff, tTresh) == 0);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Report completed iterations %d in %lld.%06lld, seconds",
                 iter,
                 tDiff.sec,
                 tDiff.usec);
    FM_LOG_PRINT("\n");

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end DbgBsmStatusRegPollDefault */




/*****************************************************************************/
/** DbgDumpBsmScratchDefault
 * \ingroup intDiagPorts
 *
 * \desc            NVM default function version for BSM_SCRATCH registers
 *                  detailed registers dump. 
 *                  For upper NVM versions, corresponding functions could be
 *                  defined in fm10000NvmAccess array
 *
 * \param[in]       sw is the switch BSM_SCRATCH will be dumped.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 *
 * \param[in]       regMask used to filter out the registers dumped.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static fm_status DbgDumpBsmScratchDefault(fm_int                    sw,
                                          fm_registerReadUINT32Func readFunc,
                                          fm_uint32                 regMask)
{
    fm_status err;
    fm_uint32 nvmVer;
    fm_uint32 value;
    fm_uint32 value2;
    fm_uint32 rv;
    fm_uint32 pcieEnable;
    fm_uint32 pcieRegMask;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);
    
    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    /* get nvmVersion */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("\nBSM_SCRATCH registers dump\n");
    FM_LOG_PRINT("\n%-25s[%d]      : %02x.%02x (0x%08x)",
                 "EEPROM VERSION",
                 EEPROM_IMAGE_VERSION,
                 (nvmVer & 0xFF00) >> 8,
                 (nvmVer & 0x00FF),
                 nvmVer);

    /* dump BSM_STATUS registers */
    if (regMask & REG_MASK_BSM_INIT_STATUS)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(BSM_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("BSM_STATUS", BSM_STATUS, value);
    
        err = readFunc(sw, FM10000_BSM_SCRATCH(MASTER_FW_VERSION), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "MASTER_FW_VERSION",
                     MASTER_FW_VERSION,
                     value);    /* [402] */
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_FW_VERSION), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_FW_VERSION",
                     SERDES_FW_VERSION,
                     value);    /* [403] */
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_STATUS_1), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_STATUS_2), &value2);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_STATUS_1",
                     SERDES_STATUS_1,
                     value);      /* [404] */
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_STATUS_2",
                     SERDES_STATUS_2,
                     value2);      /* [405] */
        FM_LOG_PRINT("\n    PCIe firmware CRC check status");
        DumpBsmSerdesStatusRegister(value, value2);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_STATUS_3), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_STATUS_4), &value2);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_STATUS_3",
                     SERDES_STATUS_3,
                     value);      /* [406] */
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_STATUS_4",
                      SERDES_STATUS_4,
                      value2);      /* [407] */
        FM_LOG_PRINT("\n  PCIe firmware version check status");
        DumpBsmSerdesStatusRegister(value, value2);
    
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_MASTER_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "PCIE_MASTER_STATUS",
                      PCIE_MASTER_STATUS,
                      value);   /* [408] */
        FM_LOG_PRINT("\n    %s: %s",
                     "PCIe Master SPICO firmware download status",
                     (value == 0) ? "pass" : "fail");
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_SERDES_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "PCIE_SERDES_STATUS",
                     PCIE_SERDES_STATUS,
                     value);   /* [409] */
        FM_LOG_PRINT("\n    %s: %s",
                     "PCIe SerDes firmware download and initialization status",
                     (value == 0) ? "pass" : "fail");
    }

    /* dump BSM_STATUS archive registers */
    if (regMask & REG_MASK_BSM_INIT_STATUS_ARCHIVE)
    {
        FM_LOG_PRINT("\n\nArchive registers");
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(DE_COLD_RESET_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("DE_COLD_RESET_STATUS",
                               DE_COLD_RESET_STATUS,
                               value);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(SBUS_RESET_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("SBUS_RESET_STATUS",
                               SBUS_RESET_STATUS,
                               value);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(MEMORY_REPAIR_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("MEMORY_REPAIR_STATUS",
                               MEMORY_REPAIR_STATUS,
                               value);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(MEMORY_INIT_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("MEMORY_INIT_STATUS",
                               MEMORY_INIT_STATUS,
                               value);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_PCS_DIS_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_PCS_DIS_STATUS",
                               PCIE_PCS_DIS_STATUS,
                               value);
        
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_MASTER_FW_DL_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_MASTER_FW_DL_STATUS",
                               PCIE_MASTER_FW_DL_STATUS,
                               value);

        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_FW_CHECK_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_FW_CHECK_STATUS",
                               PCIE_FW_CHECK_STATUS,
                               value);

        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_SERDES_FW_DL_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_SERDES_FW_DL_STATUS",
                               PCIE_SERDES_FW_DL_STATUS,
                               value);

        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_SERDES_INIT_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_SERDES_INIT_STATUS",
                               PCIE_SERDES_INIT_STATUS,
                               value);

        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_PCS_EN_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_PCS_EN_STATUS",
                               PCIE_PCS_EN_STATUS,
                               value);

        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_DE_WARM_RESET_STATUS), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_DE_WARM_RESET_STATUS",
                               PCIE_DE_WARM_RESET_STATUS,
                               value);
    }
    
    /* dump BSM_ISR_PCIE registers */
    pcieRegMask = 0x0;

    /* dump only enabled PCIEs */
    if (regMask & REG_MASK_BSM_ISR_PCIE_ENABLED)
    {
        /* retrieve the enabled PCIEs */
        err = readFunc(sw, FM10000_DEVICE_CFG(),  &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        pcieEnable = FM_GET_FIELD( rv, FM10000_DEVICE_CFG, PCIeEnable );
        pcieRegMask = (pcieEnable & ( 1 << 0)) ? REG_MASK_BSM_ISR_STATUS_PCIE0 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 1)) ? REG_MASK_BSM_ISR_STATUS_PCIE1 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 2)) ? REG_MASK_BSM_ISR_STATUS_PCIE2 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 3)) ? REG_MASK_BSM_ISR_STATUS_PCIE3 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 4)) ? REG_MASK_BSM_ISR_STATUS_PCIE4 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 5)) ? REG_MASK_BSM_ISR_STATUS_PCIE5 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 6)) ? REG_MASK_BSM_ISR_STATUS_PCIE6 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 7)) ? REG_MASK_BSM_ISR_STATUS_PCIE7 : 0;
        pcieRegMask |= (pcieEnable & ( 1 << 8)) ? REG_MASK_BSM_ISR_STATUS_PCIE8 : 0;
    }
    else if (regMask & REG_MASK_BSM_PCIE_ISR_STATUS)
    {
        pcieRegMask = regMask & REG_MASK_BSM_PCIE_ISR_STATUS;
    }
    
    /* dump ISR STATUS */
    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE0)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_0), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_0",
                               PCIE_ISR_STATUS_0,
                               value);
    }

    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE1)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_1), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_1",
                               PCIE_ISR_STATUS_1,
                               value);
    }

    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE2)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_2),  &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_2",
                               PCIE_ISR_STATUS_2,
                               value);
    }

    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE3)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_3), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_3",
                               PCIE_ISR_STATUS_3,
                               value);
    }
    
    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE4)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_4), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_4",
                              PCIE_ISR_STATUS_4,
                              value);
    }

    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE5)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_5), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_5",
                               PCIE_ISR_STATUS_5,
                               value);
    }

    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE6)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_6), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_6",
                               PCIE_ISR_STATUS_6,
                               value);
    }
    
    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE7)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_7), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_7",
                               PCIE_ISR_STATUS_7,
                               value);
    }
    
    if (pcieRegMask & REG_MASK_BSM_ISR_STATUS_PCIE8)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(PCIE_ISR_STATUS_8), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        DumpBsmStatusRegister("PCIE_ISR_STATUS_8",
                               PCIE_ISR_STATUS_8,
                               value);
    }
    
    if (regMask & REG_MASK_BSM_INIT_OOR)
    {
        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_OOR_STATUS_PASS_1), &value);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        err = readFunc(sw, FM10000_BSM_SCRATCH(SERDES_OOR_STATUS_PASS_2), &value2);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_OOR_STATUS_PASS_1",
                     SERDES_OOR_STATUS_PASS_1,
                     value);
        FM_LOG_PRINT("\n%-25s[%d]      : 0x%08x",
                     "SERDES_OOR_STATUS_PASS_2",
                     SERDES_OOR_STATUS_PASS_2,
                     value2);
        FM_LOG_PRINT("\n  Serdes on OutOfReset pass/fail status");
        DumpBsmSerdesStatusRegister(value, value2);
        
    }
    FM_LOG_PRINT("\n");
    
ABORT:
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end DbgDumpBsmScratchDefault */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000DbgPollLtssm
 *
 * \desc            LTSSM polling function.
 *                  Examines the LTSSM and LinkCtrl registers.
 *                  Registers value change detection dumps the corresponding
 *                  dbg message. 
 *
 * \param[in]       sw switch BSM_SCRATCH will be examined.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 * 
 * \param[in]       pep is the PEP to poll LTSSM for, use -1 to poll all PEPs.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DbgPollLtssm(fm_int                    sw,
                              fm_registerReadUINT32Func readFunc,
                              fm_int                    pep,
                              fm_uint32                 miliSec)
{
    fm_status err;
    fm_uint   nvmVer;
    fm_int    size;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);
   
    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }
    
    /* get nvmVersion */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    /* find the funcLtssmPoll function supporting nvm version */
    size = FM_NENTRIES(fm10000NvmAccess);
    for ( i = 0 ; i < size ; i++)
    {
        if (fm10000NvmAccess[i].maxVersionSupported >= nvmVer)
        {
            if (!fm10000NvmAccess[i].funcLtssmPoll)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            }
            
            /* got valid function version */
            err = fm10000NvmAccess[i].funcLtssmPoll(sw,
                                                    readFunc,
                                                    pep,
                                                    miliSec);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            
            break;
        }
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end fm10000DbgPollLtssm */




/*****************************************************************************/
/** fm10000DbgPollBsmStatus
 *
 * \desc            BSM_STATUS register polling function.
 *                  Examines the BSM_STATUS register, value change detection
 *                  dumps the corresponding dbg message.
 *
 * \param[in]       sw is the switch BSM_STATUS will be examined.
 * 
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DbgPollBsmStatus(fm_int                    sw,
                                  fm_registerReadUINT32Func readFunc,
                                  fm_uint32                 miliSec)
{
    fm_status err;
    fm_int    i;
    fm_int    size;
    fm_uint   nvmVer;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);
    
    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    /* get nvmVersion */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    /* find the BsmStatusPoll function supporting nvm version */
    size = FM_NENTRIES(fm10000NvmAccess);
    for ( i = 0 ; i < size ; i++)
    {
        if (fm10000NvmAccess[i].maxVersionSupported >= nvmVer)
        {
            if (!fm10000NvmAccess[i].funcBsmStatusPoll)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            }
            
            /* got valid function version */
            err = fm10000NvmAccess[i].funcBsmStatusPoll(sw,
                                                        readFunc,
                                                        miliSec);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            
            break;
        }
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end fm10000DbgPollBsmStatus */




/*****************************************************************************/
/** fm10000DbgDumpBsmScratch
 *
 * \desc            Dbg dump detailed info for BSM_SCRATCH registers.
 *
 * \param[in]       sw is the switch BSM_SCRATCH will be dumped.
 *
 * \param[in]       readFunc is the function pointer to read 32-bit registers.
 * 
 * \param[in]       regMask used to filter out the registers dumps.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpBsmScratch(fm_int                    sw,
                                   fm_registerReadUINT32Func readFunc,
                                   fm_uint32                 regMask)
{
    fm_status err;
    fm_int    i;
    fm_int    size;
    fm_uint   nvmVer;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (!readFunc)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }
    
    /* get nvmVersion, call the register readFunc */
    err = readFunc(sw, FM10000_BSM_SCRATCH(EEPROM_IMAGE_VERSION), &nvmVer);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    
    /* find the BsmScratchDump function supporting this nvm version */ 
    size = FM_NENTRIES(fm10000NvmAccess);
    for ( i = 0 ; i < size ; i++)
    {
        if (fm10000NvmAccess[i].maxVersionSupported >= nvmVer)
        {
            if (!fm10000NvmAccess[i].funcBsmScratchDump)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            }
            
            /* got valid function version */
            err = fm10000NvmAccess[i].funcBsmScratchDump(sw,
                                                         readFunc,
                                                         regMask);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            break;
        }
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fm10000DbgDumpBsmScratch */

