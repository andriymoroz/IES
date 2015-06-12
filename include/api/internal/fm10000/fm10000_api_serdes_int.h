/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_serdes_int.h
 * Creation Date:   October 21, 2013
 * Description:     function to manage SERDES.
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

#define FM10000_SERDES_STRUCT_MAGIG_NUMBER      0xaa55abcd

 
#define FM10000_EPL_RING_SERDES_SBUS_BASE_ADDR  0x02
#define FM10000_PCIE_RING_SERDES_SBUS_BASE_ADDR 0x02

#define FM10000_SBUS_SERDES_BCAST_ADDR          0xFF
#define FM10000_SBUS_CONTROLLER_ADDR            0xFE
#define FM10000_SBUS_SPICO_BCAST_ADDR           0xFD

#define FM10000_SBM_EPL_RING_SPICO_ADDRESS      0x27
#define FM10000_SBM_PCIE_RING_SPICO_ADDRESS     0x25

 
#define FM10000_SERDES_EPL_BCAST                0xFF
#define FM10000_SERDES_PCIE_BCAST               0xFF

 
#define FM10000_EPL_RING_SERDES_NUM             36

 
#define FM10000_SERDES_SAI_ACCESS_DLY_THRESH    5
#define FM10000_SERDES_SAI_ACCESS_LOOP_DELAY    1000

 
#define FM10000_SERDES_TIMESTAMPMS_MAX          1000000

 
 
#define FM10000_SERDES_SHORT_TIMEOUT            70000

 
 
#define FM10000_SERDES_LONG_TIMEOUT             700000

 
 
#define FM10000_SERDES_XLONG_TIMEOUT           2000000

 
 
#define FM10000_SERDES_DEFERRED_KR_COMPLETE    500000

 
#define FM10000_SERDES_RESET_DELAY             20000

 
#define FM10000_SERDES_PLL_CAL_CNT_THRESHOLD    200

 
#define FM10000_SERDES_CTRL_TX_ENA              (1 << 0)
#define FM10000_SERDES_CTRL_RX_ENA              (1 << 1)
#define FM10000_SERDES_CTRL_OUTPUT_ENA          (1 << 2)
#define FM10000_SERDES_CTRL_TX_ENA_MASK         (1 << 3)
#define FM10000_SERDES_CTRL_RX_ENA_MASK         (1 << 4)
#define FM10000_SERDES_CTRL_OUTPUT_ENA_MASK     (1 << 5)

 
#define FM10000_SBM_INTERRUPT_TIMEOUT_MSEC      5000
#define FM10000_SERDES_INTERRUPT_TIMEOUT_MSEC   3000
#define FM10000_SERDES_BIST_TIMEOUT_MSEC        5000
#define FM10000_SERDES_INT02_TIMEOUT_MSEC       5000

 
#define FM10000_SERDES_INT_FAST_TIMEOUT_CYCLES  100

 
#define FM10000_SERDES_DFE_DFAULT_HF            0x00;
#define FM10000_SERDES_DFE_DFAULT_LF            0x0c;
#define FM10000_SERDES_DFE_DFAULT_DC            0x38;
#define FM10000_SERDES_DFE_DFAULT_BW            0x0f;

 
 
#define FM10000_SERDES_SIGNALOK_DEBOUNCE_THRESHOLD  3
  
 
#define FM10000_EYE_DIAGRAM_SAMPLE_ARRAY_NUM    8

 
 
 
#define FM10000_SERDES_EYE_SAMPLES              (64*64)

 
 
#define FM10000_SERDES_DFE_SHORT_TIMEOUT        100000

 
 
#define FM10000_SERDES_DFE_STOP_TUNING_TIMEOUT  200000

 
#define FM10000_SERDES_DFE_DFAULT_DEBOUNCE_TIME 100

 
 
 
#define FM10000_SERDES_DFE_ADAPTIVE_TIMEOUT_MIN 2
#define FM10000_SERDES_DFE_ADAPTIVE_TIMEOUT_MAX 60

 
#define FM10000_SERDES_DFE_TUNING_MAX_RETRIES   5

 
 
 
#define FM10000_SERDES_ICAL_TUNING_MAX_CYCLES   100

 
 
 
#define FM10000_SERDES_DFE_TUNING_MAX_CYCLES    100


 
 
#define FM10000_SERDES_DFE_DATA_LEVEL0_THRESHLD 10

 
#define FM10000_SERDES_DFE_STOP_CYCLE_DELAY     200000

 
#define FM10000_SERDES_ICAL_STOP_MAX_CYCLES     10

 
#define FM10000_SERDES_PCAL_STOP_MAX_CYCLES     500

 
#define FM10000_SERDES_DFE_PAUSE_TUNING_DELAY   100000000

 
#define FM10000_SERDES_DFE_PAUSE_TUNING_LOOPS   75

 
#define FM10000_SERDES_KR_TRAINING_MAX_WAIT     (512000 / FM10000_SERDES_SHORT_TIMEOUT + 1)

 
#define FM10000_SERDES_KR_WAIT_SIGNAL_OK_CYCLE_DELAY    500000

 
#define FM10000_SERDES_KR_WAIT_SIGNAL_OK_MAX_CYCLES     500

 
 
 
 

#define FM10000_SERDES_DIVIDER_ETHMODE_1000X        0x08
#define FM10000_SERDES_DIVIDER_ETHMODE_2500X        0x14
#define FM10000_SERDES_DIVIDER_ETHMODE_6G           0x27
#define FM10000_SERDES_DIVIDER_ETHMODE_10G          0x42
#define FM10000_SERDES_DIVIDER_ETHMODE_25G          0xa5

 
#define FM10000_SERDES_KR_OP_MODE_START_TRNG_ENABLD 0x02

 
#define FM10000_SERDES_BIST_PATTERN_IDLECHAR        0x27c
#define FM10000_SERDES_BIST_PATTERN_LOWFREQ         0x07c
#define FM10000_SERDES_BIST_PATTERN_HIGHFREQ        0x155
#define FM10000_SERDES_BIST_PATTERN_MIXEDFREQ       0x0fac14
#define FM10000_SERDES_BIST_PATTERN_SQUARE8_0       0x00FF00FF00
#define FM10000_SERDES_BIST_PATTERN_SQUARE8_1       0xFF00FF00FF
#define FM10000_SERDES_BIST_PATTERN_SQUARE10        0xFFC00
#define FM10000_SERDES_BIST_USER_PATTERN_DEFAULT    0x3333333333


 
typedef enum
{
    FM10000_PMD_CONTROL_FV16G           = 0,         

    FM10000_PMD_CONTROL_CLAUSE72,                    

    FM10000_PMD_CONTROL_CLAUSE84,                    

    FM10000_PMD_CONTROL_CLAUSE92,                    

    FM10000_PMD_CONTROL_MAX

} fm10000_pmdClauseConfig;


typedef enum
{
    FM10000_LANE_BITRATE_UNKNOWN = -1,

     
    FM10000_LANE_BITRATE_25GBPS = 0,

     
    FM10000_LANE_BITRATE_10GBPS,

     
    FM10000_LANE_BITRATE_6GBPS,

     
    FM10000_LANE_BITRATE_2500MBPS,

     
    FM10000_LANE_BITRATE_1000MBPS,

     
    FM10000_LANE_BITRATE_100MBPS,

     
    FM10000_LANE_BITRATE_10MBPS,

     
    FM10000_LANE_BITRATE_MAX,

} fm10000_laneBitrate;


typedef enum
{
    FM10000_SERDES_WIDTH_10 = 0,
    FM10000_SERDES_WIDTH_20 = 1,
    FM10000_SERDES_WIDTH_40 = 3,

} fm_serdesWidthMode;

typedef enum
{
    FM10000_SERDES_DMA_TYPE_ESB,
    FM10000_SERDES_DMA_TYPE_LSB,
    FM10000_SERDES_DMA_TYPE_LSB_DIRECT,
    FM10000_SERDES_DMA_TYPE_DMEM,
    FM10000_SERDES_DMA_TYPE_DMAREG
} fm10000SerdesDmaType;


 
typedef enum
{
    FM10000_SPICO_BIST_CMD_START = 1,
    FM10000_SPICO_BIST_CMD_CHECK = 2,
    FM10000_SPICO_BIST_CMD_ALL   = 3,            

} fm10000SerdesSpicoBistCmdType;


 
typedef enum
{
    FM10000_SERDES_LB_OFF,                   
    FM10000_SERDES_LB_INTERNAL_ON,           
    FM10000_SERDES_LB_INTERNAL_OFF,          
    FM10000_SERDES_LB_PARALLEL_ON_REFCLK,    
    FM10000_SERDES_LB_PARALLEL_ON_RX_CLK,    
    FM10000_SERDES_LB_PARALLEL_OFF,          

} fm10000SerdesLbMode;

 
typedef enum
{
    FM10000_SERDES_POLARITY_NONE,
    FM10000_SERDES_POLARITY_INVERT_TX,
    FM10000_SERDES_POLARITY_INVERT_RX,
    FM10000_SERDES_POLARITY_INVERT_TX_RX,

} fm10000SerdesPolarity;



 
typedef enum
{
    FM10000_SERDES_RX_CMP_DATA_OFF,
    FM10000_SERDES_RX_CMP_DATA_PRBS7,
    FM10000_SERDES_RX_CMP_DATA_PRBS9,
    FM10000_SERDES_RX_CMP_DATA_PRBS11,
    FM10000_SERDES_RX_CMP_DATA_PRBS15,
    FM10000_SERDES_RX_CMP_DATA_PRBS23,
    FM10000_SERDES_RX_CMP_DATA_PRBS31,
    FM10000_SERDES_RX_CMP_DATA_SELF_SEED,

} fm10000SerdesRxCmpData;


 
typedef enum
{
    FM10000_SERDES_TX_DATA_SEL_CORE,
    FM10000_SERDES_TX_DATA_SEL_PRBS7,
    FM10000_SERDES_TX_DATA_SEL_PRBS9,
    FM10000_SERDES_TX_DATA_SEL_PRBS11,
    FM10000_SERDES_TX_DATA_SEL_PRBS15,
    FM10000_SERDES_TX_DATA_SEL_PRBS23,
    FM10000_SERDES_TX_DATA_SEL_PRBS31,
    FM10000_SERDES_TX_DATA_SEL_USER,
    FM10000_SERDES_TX_DATA_SEL_LOOPBACK,

} fm10000SerdesTxDataSelect;



 
typedef enum
{
    FM10000_SERDES_SEL_TX,
    FM10000_SERDES_SEL_RX,
    FM10000_SERDES_SEL_TX_RX,

} fm10000SerdesSelect;


 
typedef enum
{
    FM10000_SERDES_EQ_SEL_PRECUR,
    FM10000_SERDES_EQ_SEL_ATTEN,
    FM10000_SERDES_EQ_SEL_POSTCUR,
     
    FM10000_SERDES_EQ_SEL_ALL
} fm10000SerdesEqSelect;


 
typedef enum
{
    FM10000_SERDES_RX_TERM_AGND,
    FM10000_SERDES_RX_TERM_AVDD,
    FM10000_SERDES_RX_TERM_FLOAT,

} fm10000SerdesRxTerm;


 
typedef enum
{
     
    FM10000_SERDES_PLL_CALIBRATION_DISABLED,

 
 
 
 
 
    FM10000_SERDES_PLL_CALIBRATION_ENABLED,

 
 
 
    FM10000_SERDES_PLL_CALIBRATION_ADAPTIVE,

     
    FM10000_SERDES_PLL_CALIBRATION_MAX

} fm10000SerdesPllCal;



 
typedef enum
{
     
    FM10000_SERDES_KR_PCAL_MODE_STATIC,

     
    FM10000_SERDES_KR_PCAL_MODE_ONE_SHOT,

     
    FM10000_SERDES_KR_PCAL_MODE_CONTINUOUS,

     
    FM10000_SERDES_KR_PCAL_MODE_MAX

} fm10000SerdesKrPcalMode;



 
 
 
 
#define FM10000_SERDES_DFE_PARAM_DFE_STATUS_b_CoarseInProgress  0
#define FM10000_SERDES_DFE_PARAM_DFE_STATUS_b_FineInProgress    1
#define FM10000_SERDES_DFE_PARAM_DFE_STATUS_b_AdaptiveEnabled   6


 
typedef enum
{
    FM10000_SERDES_DGB_DISABLED,
    FM10000_SERDES_DBG_TEST_BOARD_LVL_1,
    FM10000_SERDES_DBG_MAX
} fm10000_serDesDbgLvl;


 
 
 
 
typedef enum
{
     
    FM10000_SERDES_USE_BASIC_STATE_MACHINE,

     
    FM10000_SERDES_USE_STUB_STATE_MACHINE,

 
 
 
    FM10000_SERDES_USE_TEST_MODE_STATE_MACHINE,

     
    FM10000_SERDES_USE_STATE_MACHINE_MAX

} fm10000_serDesSmMode;



#define FM10000_SERDES_SM_HISTORY_SIZE      16
#define FM10000_SERDES_SM_RECORD_SIZE       sizeof(int)

#define FM10000_SERDES_DFE_SM_HISTORY_SIZE  16
#define FM10000_SERDES_DFE_SM_RECORD_SIZE   sizeof(int)

 
#define FM10000_SERDES_PWRUP_INTR_MASK            \
    ( (1U << FM10000_SERDES_IP_b_TxRdy)      |    \
      (1U << FM10000_SERDES_IP_b_RxRdy) )

 
#define FM10000_SERDES_OPSTATE_INTR_MASK          \
    ( (1U << FM10000_SERDES_IP_b_RxSignalOk)  )

 
#define FM10000_SERDES_INT_MASK             (FM10000_SERDES_PWRUP_INTR_MASK | FM10000_SERDES_OPSTATE_INTR_MASK)

 
extern fm_text fm10000LaneBitRatesMap[FM10000_LANE_BITRATE_MAX];

#define VALIDATE_SERDES(serdes)                              \
    if ( ((serdes < 0) || (serdes >= FM10000_NUM_SERDES)) )  \
    {                                                        \
        FM_LOG_ERROR( FM_LOG_CAT_SERDES,                     \
                      "Invalid SERDES number: %d\n",         \
                      serdes );                              \
        return FM_ERR_INVALID_ARGUMENT;                      \
    }

 
typedef struct _fm10000_serdes              fm10000_serdes;
typedef struct _fm10000_lane                fm10000_lane;
typedef struct _fm10000_serDesSmEventInfo   fm10000_serDesSmEventInfo;
typedef struct _fm10000_laneDfe             fm10000_laneDfe;
typedef struct _fm10000_dfeSmEventInfo      fm10000_dfeSmEventInfo;
typedef struct _fm10000_laneKr              fm10000_laneKr;

typedef struct
{
    fm_serdesRing    ring;

    fm_uint          sbusAddr;

    fm_int           fabricPort;

    fm10000_endpoint endpoint;

    fm_int           physLane;

} fm10000_serdesMap;

 
 
 
struct _fm10000_serdes
{
    fm_uint32                       magicNumber;

 
 
 
 
 
 
 
 
    fm_status                       (*SerdesGetEyeHeight)(fm_int sw, fm_int serdes, fm_int *pEyeScore, fm_int *pHeightmV);

    fm_status                       (*SerdesGetEyeScore)(fm_int sw, fm_int serdes, fm_int *pHeight, fm_int *pWidth);

    fm_status                       (*SerdesGetEyeDiagram)(fm_int sw, fm_int serdes, fm_eyeDiagramSample *pSampleTable);

 
 
 
 
 
 
    fm_status                       (*dbgDump)(fm_int sw, fm_int serdes, fm_bool detailed);

    fm_status                       (*dbgDumpStatus)(fm_int sw, fm_int serdes);

    fm_status                       (*dbgDumpDfeStatus)(fm_int sw, fm_int serdes, fm_bool detailed);

    fm_status                       (*dbgDumpKrStatus)(fm_int sw, fm_int serdes);

    fm_status                       (*dbgDumpSpicoSbmVersions)(fm_int sw, fm_int serdes);

    fm_status                       (*dbgDumpRegisters)(fm_int sw, fm_int serdes);

    fm_status                       (*dbgResetStats)(fm_int sw, fm_int serdes);

    fm_status                       (*dbgInit)(fm_int sw, fm_int serdes, fm_uint dataWidth, fm_uint rateSel);

    fm_status                       (*dbgResetSerdes)(fm_int sw, fm_int port);

    fm_status                       (*dbgReadRegister)(fm_int sw, fm_int serdes, fm_uint regAddr, fm_uint32 *pValue);

    fm_status                       (*dbgWriteRegister)(fm_int sw, fm_int serdes, fm_uint regAddr, fm_uint32 value);

    fm_status                       (*dbgSetTxPattern)(fm_int sw, fm_int serdes, fm_text patternStr);

    fm_status                       (*dbgSetRxPattern)(fm_int sw, fm_int serdes, fm_text patternStr);

    fm_status                       (*dbgSetPolarity)(fm_int sw, fm_int serdes, fm_text polarityStr);

    fm_status                       (*dbgSetLoopback)(fm_int sw, fm_int serdes, fm_text loopbackStr);

    fm_status                       (*dbgInjectErrors)(fm_int sw, fm_int serdes, fm10000SerdesSelect serdesSel, fm_uint numErrors);

    fm_status                       (*dbgReadSbusRegister)(fm_int sw, fm_int serdes, fm_int devRegId, fm_uint32 *pValue);

    fm_status                       (*dbgWriteSbusRegister)(fm_int sw, fm_int serdes, fm_int devRegId, fm_uint32 value);

    fm_status                       (*dbgInterruptSpico)(fm_int sw, fm_int cmd, fm_int param, fm_int timeout, fm_uint32 *pResult);

    fm_status                       (*dbgRunDfeTuning)(fm_int sw, fm_int serdes, fm_dfeMode dfeMode, fm_int dfeHf, fm_int  dfeLf, fm_int dfeDc, fm_int dfeBw);

    fm_text                         (*dbgGetRegName)(fm_uint regOff);

    const void                     *(*dbgGetRegFields)(fm_uint regOff);

    fm_text                         (*dbgGetSpicoIntrRegName)(fm_uint regOff);

    const void                     *(*dbgGetSpicoIntrRegFields)(fm_uint regOff);

    fm_text                         (*dbgGetIpIdCodeStr)(fm_uint value);

};

 
struct _fm10000_dfeSmEventInfo
{
 
 
 

    fm_switch        *switchPtr;      
    fm_lane          *lanePtr;        
    fm10000_lane     *laneExt;        
    fm10000_laneDfe  *laneDfe;        
    fm_laneAttr      *laneAttr;       

 
 
 


 
 
 
};



 
 
 
struct  _fm10000_laneDfe
{
     
    fm10000_lane               *pLaneExt;

     
    fm_smHandle                 smHandle;

     
    fm_int                      smType;

     
    fm_int                      transitionHistorySize;

     
    fm_timerHandle              timerHandle;

     
    fm10000_dfeSmEventInfo      eventInfo;

 
 
 
 
 
 
 
    fm_uint32                   dfeTuningStat;

     
    fm_int                      retryCntr;

     
    fm_int                      cycleCntr;

     
    fm_bool                     dfeAdaptive;

     
    fm_bool                     sendDfeComplete;

     
    fm_bool                     pCalKrMode;

 
 
 
    fm_int                      dfeDebounceTime;

     
    fm_uint32                   dfeDataLevThreshold;

     
    fm_bool                     pause;

     
    fm_int                      dfe_HF;

     
    fm_int                      dfe_LF;

     
    fm_int                      dfe_DC;

     
    fm_int                      dfe_BW;

     
    fm_int                      eyeScoreHeight;

     
    fm_int                      eyeScoreHeightmV;

 
 
 
    fm_uint32                   stopCycleCnt;

     
    fm_uint32                   forcedStopCycleCnt;

 
 
    fm_uint32                   stopCoarseDelayAvg;

 
 
    fm_uint32                   stopCoarseDelayMax;

 
 
    fm_uint32                   stopFineDelayAvg;

 
 
    fm_uint32                   stopFineDelayMax;

 
 
    fm_uint32                   stopTuningDelayLstMs;

 
 
    fm_uint32                   stopTuningDelayAvgMs;

 
 
    fm_uint32                   stopTuningDelayMaxMs;

 
 
 
    fm_uint32                   startCycleCnt;

 
 
    fm_uint32                   iCalDelayAvg;

 
 
    fm_uint32                   iCalDelayMax;

 
 
    fm_uint32                   pCalDelayAvg;

 
 
    fm_uint32                   pCalDelayMax;

     
    fm_uint32                   refTimeMs;

 
 
    fm_uint32                   iCalDelayLastMs;

 
 
    fm_uint32                   iCalDelayAvgMs;

 
 
    fm_uint32                   iCalDelayMaxMs;

 
 
    fm_uint32                   pCalDelayLastMs;

 
 
    fm_uint32                   pCalDelayAvgMs;

 
 
    fm_uint32                   pCalDelayMaxMs;
};


 
 
 
struct  _fm10000_laneKr
{
     
    fm10000_lane               *pLaneExt;

     
    fm_bool                     invrtAdjPolarity;

     
    fm_bool                     disaTimeout;

     
    fm_bool                     disaTxEqAdjReq;

     
    fm_bool                     resetParameters;

     
    fm10000SerdesKrPcalMode     pCalMode;

     
    fm_int                      krTrainingCtrlCnt;

     
    fm_int                      relLane;

     
    fm_uint32                   seed;

     
    fm10000_pmdClauseConfig     clause;

     
    fm_bool                     opt_TT_FECreq;      

     
    fm_bool                     opt_TT_FECcap;      

     
    fm_bool                     opt_TT_TF;          

     
    fm_int                      eyeScoreHeight;

 
 
 
    fm_uint32                   startKrCycleCnt;

     
    fm_uint32                   krErrorCnt;

     
    fm_uint32                   refTimeMs;

     
    fm_uint32                   krTrainingDelayLastMs;

     
    fm_uint32                   krTrainingDelayAvgMs;

     
    fm_uint32                   krTrainingDelayMaxMs;

};




 
struct _fm10000_serDesSmEventInfo
{
 
 
 

    fm_switch        *switchPtr;      
    fm_lane          *lanePtr;        
    fm10000_lane     *laneExt;        
    fm_laneAttr      *laneAttr;       

 
 
 

    union
    {
        fm_dfeMode          dfeMode;
        fm10000_laneBitrate bitRate;
        fm_int              bistSubmode;

    } info;

 
 
 

};



 
 
 
struct  _fm10000_lane
{
      
    fm_lane                    *base;

     
    fm_int                      lane;

     
    fm_int                      physLane;

     
    fm_int                      epl;

     
    fm_int                      channel;

     
    fm_int                      serDes;

     
    fm_smHandle                 smHandle;

     
    fm_int                      smType;

     
    fm_int                      transitionHistorySize;

     
    fm10000_laneBitrate         bitRate;

     
    fm10000_laneBitrate         prevBitRate;

     
    fm_serdesWidthMode          widthMode; 

     
    fm_uint                     rateSel;

     
    fm_dfeMode                  dfeMode;

 
 
 
 
    fm_uint                     eeeModeActive;

     
    fm10000_laneDfe             dfeExt;

     
    fm10000_laneKr              krExt;

     
    fm_timerHandle              timerHandle;

     
    fm_uint32                   serdesInterruptMask;

     
    fm10000_serDesSmEventInfo   eventInfo;

     
    fm10000_serDesDbgLvl        dbgLvl;

     
    fm_int                      signalOkDebounce;

     
    fm_bool                     krTrainingEn;

     
    fm_bool                     serDesActive;

     
    fm_uint32                   serDesEnableCache;

     
    fm_uint32                   serDesEnableRetryCtrl;

 
 
 
 
    fm_uint32                   serDesPllStatus;

 
 
 
    fm_bool                     nearLoopbackEn;

 
 
 
    fm_uint32                   farLoopbackStatus;

     
    fm_bool                     bistActive;

     
    fm_int                      bistTxSubMode;

     
    fm_int                      bistRxSubMode;

 
 
 
 
    fm_uint64                   bistCustomData0;

 
 
    fm_uint64                   bistCustomData1;

     
     
    fm_byte                     pllMask;

 
 
 
 
 
 
 
    fm10000SerdesPllCal         pllCalibrationMode;

     
    fm_uint32                   pllCalibrationCycleCnt ;

     
    fm_uint32                   txPhaseSlip;

     
    fm_uint32                   rxPhaseSlip;

     
    fm10000SerdesRxTerm         rxTermination;

 
 
    void                       *pEyeDiagExt;

     
    fm10000_port               *nativePortExt;

     
    fm10000_port               *parentPortExt;

     
    FM_DLL_DEFINE_NODE( _fm10000_lane, nextLane, prevLane );

     
    fm_uint32                   fResetCnt;

     
    fm_uint32                   serdesRestoredCnt;
};

fm_status fm10000SerdesInitMappingTable(fm_int sw);
fm_bool fm10000SerdesCheckIfIsActive(fm_int sw,
                                     fm_int serDes);
void fm10000SerDesSaiSetDebug(fm_int debug);
void fm10000SerDesPcieSetDebug(fm_int debug);
fm_status fm10000InitSwSerdes(fm_int sw);
fm_status fm10000SerdesInitXServicesInt(fm_int sw);
fm_status fm10000GetSerdesWidthModeRateSel(fm_int              serDes,
                                           fm_int              bitRate,
                                           fm_serdesWidthMode *pWidthMode,
                                           fm_uint            *pRateSel);
fm_status fm10000SetPcslCfgWidthMode(fm_int             sw,
                                     fm_int             serDes,
                                     fm_serdesWidthMode widthMode);
fm_status fm10000ConfigurePcslBitSlip(fm_int    sw,
                                      fm_int    serDes);
fm_status fm10000SerdesConfigurePhaseSlip(fm_int    sw,
                                          fm_int    serDes);
fm_status fm10000SpicoRamBist(fm_int                        sw,
                              fm_serdesRing                 ring,
                              fm_uint                       sbusAddr,
                              fm10000SerdesSpicoBistCmdType cmd);
fm_status fm10000SerdesWrite(fm_int sw, fm_int serdes, fm_uint regAddr, fm_uint32 value);
fm_status fm10000SerdesRead(fm_int sw, fm_int serdes, fm_uint regAddr, fm_uint32 *value);

fm_status fm10000SbmSpicoIntWrite(fm_int sw, fm_serdesRing ring, fm_uint sbusAddr, fm_uint intNum, fm_uint32 param);
fm_status fm10000SbmSpicoIntRead(fm_int sw, fm_serdesRing ring, fm_uint sbusAddr, fm_uint32 *value);
fm_status fm10000SerdesSpicoInt(fm_int     sw,
                                fm_int     serdes,
                                fm_uint    intNum, 
                                fm_uint32  param, 
                                fm_uint32 *pResult);
fm_status fm10000SerdesPcieSpicoInt(fm_int     sw,
                                    fm_int     serDes,
                                    fm_uint    intNum,
                                    fm_uint32  param,
                                    fm_uint32 *pResult);
fm_status fm10000SerdesSpicoWrOnlyInt(fm_int      sw,
                                      fm_int      serdes,
                                      fm_uint     intNum,
                                      fm_uint32   param);
fm_status fm10000SbmSpicoInt(fm_int         sw,
                             fm_serdesRing  ring,
                             fm_int         sbusAddr,
                             fm_uint        intNum,
                             fm_uint32      param,
                             fm_uint32     *value);
fm_bool   fm10000SerdesSpicoIsRunning(fm_int sw,
                                      fm_int serDes);
fm_bool   fm10000SbmSpicoIsRunning(fm_int sw,
                                   fm_int ring);
fm_status fm10000SerdesDmaRead(fm_int               sw,
                              fm_int               serDes,
                              fm10000SerdesDmaType type,
                              fm_uint              addr,
                              fm_uint32           *value);
fm_status fm10000SerdesDmaWrite(fm_int               sw,
                               fm_int               serDes,
                               fm10000SerdesDmaType type,
                               fm_uint              addr,
                               fm_uint32            data);
fm_status fm10000SerdesDmaReadModifyWrite(fm_int                sw,
                                         fm_int                serDes,
                                         fm10000SerdesDmaType  type,
                                         fm_uint               regAddr,
                                         fm_uint32             data,
                                         fm_uint32             mask,
                                         fm_uint32            *pReadValue);
fm_status fm10000SerdesReadModifyWrite(fm_int     sw,
                                      fm_int     serDes,
                                      fm_uint    regAddr,
                                      fm_uint32  data,
                                      fm_uint32  mask,
                                      fm_uint32 *pReadValue);
fm_status fm10000SerdesSpicoDoCrc(fm_int    sw,
                                  fm_int    serdes);
fm_status fm10000SbmSpicoDoCrc(fm_int           sw,
                               fm_serdesRing    ring,
                               fm_uint          sbusAddr);
fm_status fm10000SwapImageDoCrc(fm_int          sw,
                                fm_serdesRing   ring,
                                fm_uint         sbusAddr,
                                fm_int          swapCrcCode);
fm_status fm10000SerdesResetSpico(fm_int sw,
                                  fm_int serDes);
fm_status fm10000SerdesSpicoSetup(fm_int sw,
                                  fm_int serDes);
fm_status fm10000SerdesSpicoSaveImageParam(const fm_uint16 *pRomImg,
                                           fm_int           numWords);
fm_status fm10000SerdesCheckId(fm_int   sw,
                               fm_int   serDes,
                               fm_bool *pValidId);
fm_status fm10000SerdesEnableSerDesInterrupts(fm_int   sw,
                                              fm_int   serDes);
fm_status fm10000SerdesDisableSerDesInterrupts(fm_int   sw,
                                               fm_int   serDes);
fm_status fm10000SerdesTxRxEnaCtrl(fm_int    sw,
                                   fm_int    serDes,
                                   fm_uint32 enaCtrl);
fm_status fm10000SerdesDisable(fm_int    sw,
                               fm_int    serDes);
fm_status fm10000SerdesSetBitRate(fm_int  sw,
                                  fm_int  serDes,
                                  fm_uint ratSel);
fm_status fm10000SerdesSetWidthMode(fm_int             sw,
                                    fm_int             serDes,
                                    fm_serdesWidthMode widthMode);
fm_status fm10000SerdesSetPllCalibrationMode(fm_int    sw,
                                             fm_int    serDes);
fm_status fm10000SerdesSpicoIntSBusWrite(fm_int     sw,
                                         fm_int     serdes,
                                         fm_uint    intNum,
                                         fm_uint32  param);
fm_status fm10000SerdesSpicoIntSBusRead(fm_int      sw,
                                        fm_int      serdes,
                                        fm_uint32  *pValue);
fm_status fm10000SerdesSpicoIntSBusReadFast(fm_int      sw,
                                            fm_int      serdes,
                                            fm_uint32  *pValue);
fm_status fm10000SerdesInitLaneControlStructures(fm_int sw,
                                                 fm_int serDes);
fm_status fm10000SerdesInitOpMode(fm_int sw);
fm_status fm10000SerdesClearAllSerDesInterrupts(fm_int sw);
fm_status fm10000SerdesGetOpMode(fm_int                 sw,
                                 fm_int                 serDes,
                                 fm_serDesOpMode       *pSerdesOpMode,
                                 fm10000_serDesSmMode  *pSerdesSmMode,
                                 fm_bool               *pSerdesUseLaneSai);
fm_status fm10000SerdesSpicoUploadImage(fm_int           sw,
                                        fm_serdesRing    ring,
                                        fm_int           serdesAddr,
                                        const fm_uint16 *pRomImg,
                                        fm_int           numWords);
fm_status fm10000SbmSpicoUploadImage(fm_int           sw,
                                     fm_serdesRing    ring,
                                     fm_uint          sbusAddr,
                                     const fm_uint16 *pSbmRomImg,
                                     fm_int           sbmNumWords);
fm_status fm10000SerdesSwapUploadImage(fm_int           sw,
                                       fm_serdesRing    ring,
                                       fm_uint          sbusAddr,
                                       const fm_uint16 *pSwapRomImg,
                                       fm_int           swapNumWords);
fm_status fm10000SerdesSwapAltUploadImage(fm_int         sw,
                                          fm_serdesRing    ring,
                                          fm_uint          sbusAddr,
                                          const fm_uint16 *pSwapRomImg,
                                          fm_int           swapNumWords);
fm_status fm10000SbmGetBuildRevisionId(fm_int        sw,
                                       fm_serdesRing ring,
                                       fm_uint      *pValue);

fm_status fm10000SerdesChckCrcVersionBuildId(fm_int           sw,
                                             fm_int           firstSerdes,
                                             fm_int           lastSerdes,
                                             fm_uint32        expectedCodeVersionBuildId);
fm_status fm10000SbmChckCrcVersionBuildId(fm_int     sw,
                                          fm_int     ring,
                                          fm_uint32  expectedCodeVersionBuildId);
fm_status fm10000SwapImageCheckCrc(fm_int     sw,
                                   fm_int     ring,
                                   fm_int     swapCrcCode);
fm_status fm10000SerdesGetTxRxReadyStatus(fm_int   sw,
                                          fm_uint  serdes,
                                          fm_bool *txRdy,
                                          fm_bool *rxRdy);
fm_status fm10000SerdesInitSignalOk(fm_int sw,
                                    fm_int serdes,
                                    fm_int threshold);
fm_status fm10000SerdesGetSignalOk(fm_int   sw,
                                   fm_int   serdes,
                                   fm_bool *pSignalOk);
fm_status fm10000SerdesGetKrTrainingStatus(fm_int   sw,
                                           fm_int   serDes,
                                           fm_bool *pKrSignalOk,
                                           fm_bool *pKrTrainingFailure);
fm_status fm10000SerdesSetLoopbackMode(fm_int              sw,
                                       fm_int              serdes,
                                       fm10000SerdesLbMode mode);
fm_status fm10000SerdesGetLoopbackMode(fm_int               sw,
                                       fm_int               serdes,
                                       fm10000SerdesLbMode *mode);
fm_status fm10000SerdesSetPolarity(fm_int                sw,
                                   fm_int                serdes,
                                   fm10000SerdesPolarity mode);
fm_status fm10000SerdesGetPolarity(fm_int                 sw,
                                   fm_int                 serdes,
                                   fm10000SerdesPolarity *mode);
fm_status fm10000SerdesGetTxEq(fm_int                sw,
                               fm_int                serdes,
                               fm10000SerdesEqSelect select,
                               fm_int               *txEq);
fm_status fm10000SerdesSetTxEq(fm_int                sw,
                               fm_int                serdes,
                               fm10000SerdesEqSelect select,
                               fm_int                txEq);
fm_status fm10000SerdesGetRxTerm(fm_int               sw,
                                 fm_int               serdes,
                                 fm10000SerdesRxTerm *rxTerm);
fm_status fm10000SerdesSetRxTerm(fm_int              sw,
                                 fm_int              serdes,
                                 fm10000SerdesRxTerm rxTerm);
fm_status fm10000SerDesGetBuildRevisionId(fm_int    sw,
                                          fm_int    serdes,
                                          fm_uint  *pValue);
fm_status fm10000SerdesSetBasicCmpMode(fm_int   sw,
                                       fm_int   serdes);
fm_status fm10000SerdesGetTxDataSelect(fm_int                   sw,
                                       fm_int                   serdes,
                                       fm10000SerdesTxDataSelect *dataSel);
fm_status fm10000SerdesGetRxCmpData(fm_int                   sw,
                                    fm_int                   serdes,
                                    fm10000SerdesRxCmpData *cmpData);
fm_status fm10000SerdesSetDataCoreSource(fm_int                  sw,
                                         fm_int                  serdes,
                                         fm10000SerdesSelect     serdesSel);
fm_status fm10000SerdesSetTxDataSelect(fm_int                  sw,
                                       fm_int                  serdes,
                                       fm10000SerdesTxDataSelect dataSel);
fm_status fm10000SerdesSetRxCmpData(fm_int                  sw,
                                    fm_int                  serdes,
                                    fm10000SerdesRxCmpData cmpData);
fm_status fm10000SerdesDisablePrbsGen(fm_int                  sw,
                                      fm_int                  serdes,
                                      fm10000SerdesSelect     serdesSel);
fm_status fm10000SerdesSetUserDataPattern(fm_int sw,
                                          fm_int serdes,
                                          fm10000SerdesSelect serdesSel,
                                          fm_uint32 *pattern10Bit,
                                          fm_int patternSize);
fm_status fm10000SerdesGetErrors(fm_int               sw,
                                 fm_int               serdes,
                                 fm10000SerdesDmaType type,
                                 fm_uint             *counter,
                                 fm_bool              clearCounter);
fm_status fm10000MapSerdesToSbus(fm_int         sw,
                                 fm_int         serdes,
                                 fm_uint       *sbusAddr,
                                 fm_serdesRing *ring );

fm_status fm10000MapPortLaneToSerdes(fm_int sw,
                                     fm_int port,
                                     fm_int laneNum,
                                     fm_int *serdes);
fm_status fm10000MapLogicalPortToSerdes(fm_int  sw,
                                        fm_int  port,
                                        fm_int *serdes);
fm_status fm10000MapFabricPortToSerdes( fm_int         sw,
                                        fm_int         fabricPort,
                                        fm_int        *serdes,
                                        fm_serdesRing *serdesRing );
fm_status fm10000MapPhysicalPortToSerdes( fm_int         sw,
                                          fm_int         physPort,
                                          fm_int        *serdes,
                                          fm_serdesRing *ring );
fm_status fm10000MapPhysicalPortToPepSerDes( fm_int  sw,
                                             fm_int  physPort,
                                             fm_int *pep,
                                             fm_int *serdes );
fm_status fm10000MapFabricPortToPepSerDes( fm_int  sw,
                                           fm_int  fabricPort,
                                           fm_int *pep,
                                           fm_int *serDes );

fm_status fm10000MapPepToLogicalPort( fm_int sw, fm_int pep, fm_int *port );

fm_status fm10000MapLogicalPortToPep( fm_int sw, fm_int port, fm_int *pep );

fm_status fm10000DbgSerdesInit(fm_int sw,
                               fm_int serDes,
                               fm_uint dataWidth,
                               fm_uint rateSel);
fm_status fm10000DbgSerdesRunDfeTuning(fm_int        sw,
                                       fm_int        serDes,
                                       fm_dfeMode    dfeMode,
                                       fm_int        dfeHf,
                                       fm_int        dfeLf,
                                       fm_int        dfeDc,
                                       fm_int        dfeBw);
fm_status fm10000DbgDumpSerDes(fm_int  sw,
                                fm_int  serDes,
                                fm_text cmd);
fm_status fm10000DbgSetSerDesTxPattern(fm_int  sw,
                                       fm_int  serDes,
                                       fm_text pattern);
fm_status fm10000DbgSetSerDesRxPattern(fm_int  sw,
                                       fm_int  serDes,
                                       fm_text pattern);
fm_status fm10000DbgSetSerdesPolarity(fm_int  sw,
                                      fm_int  serDes,
                                      fm_text polarityStr);
fm_status fm10000DbgSetSerdesLoopback(fm_int  sw,
                                      fm_int  serDes,
                                      fm_text loopbackStr);
fm_status fm10000DbgSerdesInjectErrors(fm_int sw,
                                       fm_int serdes,
                                       fm_int serdesSel,
                                       fm_uint numErrors);
fm_status fm10000DbgReadSBusRegister(fm_int     sw,
                                     fm_int     sbusDevID,
                                     fm_int     devRegID,
                                     fm_bool    writeReg,
                                     fm_uint32 *value);
fm_status fm10000DbgWriteSBusRegister(fm_int     sw,
                                      fm_int     sbusDevID,
                                      fm_int     devRegID,
                                      fm_uint32  value);
fm_status fm10000DbgReadSerDesRegister(fm_int     sw,
                                       fm_int     serDes,
                                       fm_uint    regAddr,
                                       fm_uint32 *value);
fm_status fm10000DbgWriteSerDesRegister(fm_int     sw,
                                        fm_int     serDes,
                                        fm_uint    regAddr,
                                        fm_uint32  value);
fm_status fm10000DbgInterruptSpico(fm_int      sw,
                                   fm_int      cmd,
                                   fm_int      param,
                                   fm_int      timeout,
                                   fm_uint32  *result);
fm_status fm10000SetSerdesTxPattern(fm_int    sw,
                                    fm_int    serdes,
                                    fm_int    submode,
                                    fm_uint64 customData0,
                                    fm_uint64 customData1);
fm_status fm10000SetSerdesRxPattern(fm_int    sw,
                                    fm_int    serdes,
                                    fm_int    submode,
                                    fm_uint64 customData0,
                                    fm_uint64 customData1);
fm_status fm10000ClearSerdesTxPattern(fm_int sw,
                                      fm_int serdes);
fm_status fm10000ClearSerdesRxPattern(fm_int sw,
                                      fm_int serdes);
fm_status fm10000ResetSerdesErrorCounter(fm_int sw,
                                         fm_int serdes);
fm_status fm10000GetSerdesErrorCounter(fm_int     sw, 
                                       fm_int     serdes, 
                                       fm_uint32 *counter);
fm_status fm10000SerdesInjectErrors(fm_int              sw,
                                    fm_int              serdes,
                                    fm10000SerdesSelect serdesSel,
                                    fm_uint             numErrors);
fm_status fm10000SetSerdesCursor(fm_int    sw,
                                 fm_int    serdes,
                                 fm_int    cursor);
fm_status fm10000SetSerdesPreCursor(fm_int    sw,
                                    fm_int    serdes,
                                    fm_int    preCursor);
fm_status fm10000SetSerdesPostCursor(fm_int    sw,
                                     fm_int    serdes,
                                     fm_int    postCursor);
fm_status fm10000SetSerdesLanePolarity(fm_int sw,
                                       fm_int serdes,
                                       fm_bool invertTx,
                                       fm_bool invertRx);
fm_status fm10000SerdesDfeTuningStartICal (fm_int     sw,
                                           fm_int     serDes);
fm_status fm10000SerdesDfeTuningStartPCalSingleExec(fm_int     sw,
                                                    fm_int     serDes);
fm_status fm10000SerdesDfeTuningStartPCalContinuous(fm_int     sw,
                                                    fm_int     serDes);
fm_status fm10000SerdesDfeTuningGetStatus(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint32 *pDfeStatus);
fm_status fm10000SerdesDfeTuningGetICalStatus(fm_int     sw,
                                              fm_int     serDes,
                                              fm_bool   *pICalStatus);
fm_status fm10000SerdesDfeTuningGetPCalStatus(fm_int     sw,
                                              fm_int     serDes,
                                              fm_bool   *pPCalStatus);
fm_status fm10000SerdesDfeTuningCheckICalConvergence(fm_int     sw,
                                                     fm_int     serDes,
                                                     fm_bool   *pICalSuccessful);
fm_status fm10000SerdesDfeTuningStop(fm_int     sw,
                                     fm_int     serDes);
fm_status fm10000SerdesForcedDfeStop(fm_int       sw,
                                     fm_int       serDes);
fm_status fm10000SerdesDfeTuningConfig(fm_int     sw,
                                       fm_int     serDes);
fm_status fm10000SerdesDfeTuningReset(fm_int     sw,
                                      fm_int     serDes);
fm_status fm10000StopKrTraining(fm_int  sw,
                                fm_int  serdes,
                                fm_bool waitSignalOk);
fm_status fm10000StartKrTraining(fm_int  sw,
                                 fm_int  serDes);
fm_status fm10000SetKrPcalMode(fm_int   sw,
                               fm_int   serDes,
                               fm_int   dfeMode);
fm_status fm10000SerdesGetTuningState(fm_int     sw,
                                      fm_int     serDes,
                                      fm_uint32 *pDfeTuningState);
fm_status fm10000SerdesIsDfeTuningActive(fm_int     sw,
                                         fm_int     serDes,
                                         fm_bool   *pDfeTuningIsActive);
fm_status fm10000SerdesGetEyeHeight(fm_int  sw,
                                    fm_int  serDes,
                                    fm_int *pEyeScore,
                                    fm_int *pHeightmV);
fm_status fm10000SerdesGetEyeScore(fm_int     sw,
                                   fm_int     serDes,
                                   fm_uint32 *pEyeScore);
fm_status fm10000SerdesGetBistUserPattern(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint64 *pSerDesBistUserPatternLow,
                                          fm_uint64 *pSerDesBistUserPatternHigh);
fm_status fm10000SerdesSetBistUserPattern(fm_int     sw,
                                          fm_int     serDes,
                                          fm_uint64 *pSerDesBistUserPatternLow,
                                          fm_uint64 *pSerDesBistUserPatternHigh);
fm_status fm10000SerdesSaveKrTrainingDelayInfo(fm_int       sw,
                                               fm_int       serDes);
fm_status fm10000SerdesIncrKrStatsCounter(fm_int       sw,
                                          fm_int       serDes,
                                          fm_int       counterSpec);
fm_status fm10000SerdesSaveStopTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    stopCoarseDelay,
                                               fm_uint32    stopFineDelay);
fm_status fm10000SerdesSaveICalTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    iCalDelay);
fm_status fm10000SerdesSavePCalTuningStatsInfo(fm_int       sw,
                                               fm_int       serDes,
                                               fm_uint32    pCalDelay);
fm_status fm10000SerdesSaveICalTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes);
fm_status fm10000SerdesSavePCalTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes);
fm_status fm10000SerdesSaveStopTuningDelayInfo(fm_int       sw,
                                               fm_int       serDes);
fm_uint32 fm10000SerdesGetTimestampMs(void);
fm_bool fm10000SerdesGetTimestampDiffMs(fm_uint32  start,
                                        fm_uint32  stop,
                                        fm_uint32 *pDiff);
fm_status fm10000MapEplChannelToLane(fm_int     sw,
                                     fm_int     epl,
                                     fm_int     channel,
                                     fm_int *   lane );
fm_status fm10000SerdesIncrStatsCounter(fm_int       sw,
                                        fm_int       serDes,
                                        fm_int       counterSpec);
fm_status fm10000MapEplLaneToChannel(fm_int     sw,
                                     fm_int     epl,
                                     fm_int     lane,
                                     fm_int *   channel );
fm_status fm10000MapEplLaneToSerdes (fm_int     sw,
                                     fm_int     epl,
                                     fm_int     lane,
                                     fm_int *   serDes );
fm_status fm10000MapSerdesToLogicalPort(fm_int  sw,
                                        fm_int  serdes,
                                        fm_int *port);
fm_status fm10000MapSerdesToEplLane (fm_int     sw,
                                     fm_int     serDes,
                                     fm_int *   epl,
                                     fm_int *   lane );
fm_status fm10000MapSerdesToPepLane(fm_int  sw,
                                    fm_int  serdes,
                                    fm_int *pep,
                                    fm_int *lane );
fm_status fm10000SerdesSetupKrConfigClause92(fm_int     sw,
                                             fm_int     serDes,
                                             fm_int     relLane,
                                             fm_uint32 *pSeed);
fm_status fm10000SerdesSetupKrConfigClause84(fm_int     sw,
                                             fm_int     serDes,
                                             fm_int     relLane,
                                             fm_uint32 *pSeed);
fm_status fm10000SerdesSetupKrConfig(fm_int       sw,
                                     fm_int       serDes,
                                     fm_bool     *pKrIsRunning);
fm_status fm10000SerDesEventHandler( fm_int    sw,
                                     fm_int    epl,
                                     fm_int    lane,
                                     fm_uint32 serDesIp );
fm_status fm10000SerDesGetEyeHeightWidth(fm_int     sw,
                                         fm_int     serDes,
                                         fm_int     *pHeigth,
                                         fm_int     *pWidth);
fm_status fm10000SerDesGetEyeDiagram(fm_int                 sw,
                                     fm_int                 serDes,
                                     fm_eyeDiagramSample   *pSampleTable);
fm_status fm10000SerdesSendDfeEventReq(fm_int    sw,
                                       fm_int    serDes,
                                       fm_int    eventId);

fm_status fm10000SerdesConfigureEeeInt(fm_int sw, fm_int serDes);
fm_int    fm10000SerdesGetPepFromMap(fm_int serDes);

 
 

 
extern const fm_uint16 fm10000_sbus_master_code_prd[];
extern const fm_uint32 fm10000_sbus_master_code_size_prd;
extern const fm_uint32 fm10000_sbus_master_code_versionBuildId_prd;

extern const fm_uint16 fm10000_serdes_spico_code_prd1[];
extern const fm_uint32 fm10000_serdes_spico_code_size_prd1;
extern const fm_uint32 fm10000_serdes_spico_code_versionBuildId_prd1;

extern const fm_uint16 fm10000_serdes_swap_code_prd1[];
extern const fm_uint32 fm10000_serdes_swap_code_size_prd1;
extern const fm_uint32 fm10000_serdes_swap_code_versionBuildId_prd1;

extern const fm_uint16 fm10000_serdes_spico_code_prd2[];
extern const fm_uint32 fm10000_serdes_spico_code_size_prd2;
extern const fm_uint32 fm10000_serdes_spico_code_versionBuildId_prd2;

extern const fm_uint16 fm10000_serdes_swap_code_prd2[];
extern const fm_uint32 fm10000_serdes_swap_code_size_prd2;
extern const fm_uint32 fm10000_serdes_swap_code_versionBuildId_prd2;

 
extern const fm_uint16 fm10000_sbus_master_code_dev[];
extern const fm_uint32 fm10000_sbus_master_code_size_dev;
extern const fm_uint32 fm10000_sbus_master_code_versionBuildId_dev;

extern const fm_uint16 fm10000_serdes_spico_code_dev1[];
extern const fm_uint32 fm10000_serdes_spico_code_size_dev1;
extern const fm_uint32 fm10000_serdes_spico_code_versionBuildId_dev1;

extern const fm_uint16 fm10000_serdes_swap_code_dev1[];
extern const fm_uint32 fm10000_serdes_swap_code_size_dev1;
extern const fm_uint32 fm10000_serdes_swap_code_versionBuildId_dev1;

extern const fm_uint16 fm10000_serdes_spico_code_dev2[];
extern const fm_uint32 fm10000_serdes_spico_code_size_dev2;
extern const fm_uint32 fm10000_serdes_spico_code_versionBuildId_dev2;

extern const fm_uint16 fm10000_serdes_swap_code_dev2[];
extern const fm_uint32 fm10000_serdes_swap_code_size_dev2;
extern const fm_uint32 fm10000_serdes_swap_code_versionBuildId_dev2;


