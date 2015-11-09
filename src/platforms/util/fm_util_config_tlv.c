/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_config_tlv.c
 * Creation Date:   May 15, 2015
 * Description:     Functions to encode/decode API/platform
 *                  properties to TLVs.
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

#include <fm_sdk_fm10000_int.h>
#include <platforms/util/fm_util_config_tlv.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define TLV_MAX_LEN                 255

#define MAX_PROP_LEN                1024
#define MAX_PROP_TOK_SIZE           5

#define TLV_LEN_OFF                 2
#define TLV_VALUE_OFF               3
#define TLV_SET_TYPE(_tlv, _val)    _tlv[0] = (_val >> 8) & 0xFF;     \
                                    _tlv[1] = _val & 0xFF;
#define TLV_SET_LEN(_tlv, _len)     _tlv[2] = _len;


#define API_CFG_PREFIX "api."
#define DEBUG_CFG_PREFIX "debug."
#define FM10K_CFG_PREFIX "api.FM10000."

#define PLAT_CFG_PREFIX "api.platform.config."
#define PLAT_CFG_SW_PREFIX "api.platform.config.switch."
#define PLAT_LIB_CFG_PREFIX "api.platform.lib.config."
#define PLAT_LIB_CFG_BUS_PREFIX "api.platform.lib.config.bus"
#define PLAT_LIB_CFG_PCAMUX_PREFIX "api.platform.lib.config.pcaMux."
#define PLAT_LIB_CFG_PCAIO_PREFIX "api.platform.lib.config.pcaIo."
#define PLAT_LIB_CFG_HWRESID_PREFIX "api.platform.lib.config.hwResourceId."

typedef enum
{
    PROP_TEXT,
    PROP_INT,
    PROP_INT_H, /* Output is displayed in hex */
    PROP_UINT,  /* Encoding is unsigned */
    PROP_UINT_H, /* Output is displayed in hex */
    PROP_UINT64,
    PROP_BOOL
} fm_propType;

typedef enum
{
    ENCODE,
    DECODE
} fm_utilEnDecode;

typedef fm_status (*fm_enDecodeValueFunc)(fm_utilEnDecode encode,
                                          void           *propMap,
                                          fm_byte        *tlvVal,
                                          fm_int          tlvValSize,
                                          fm_char        *propTxt,
                                          fm_int          propTxtSize,
                                          fm_int         *len);


typedef struct
{
    /* String description of the value */
    fm_text desc;

    /* Value for the corresponding string */
    fm_int  value;

} fm_utilStrMap;


typedef struct
{
    /* Matching string */
    fm_text key;

    /* Prop type */
    fm_propType propType;

    /* TLV Type */
    fm_uint tlvId;

    /* Length of value property, not including arguments */
    fm_int  valLen;

    /* Encode/Decode function pointer if used */
    fm_enDecodeValueFunc enDecFunc;

    /* String mapping structure if used */
    fm_utilStrMap *strMap;

    /* Size of string mapping structure if used */
    fm_int strMapSize;

} fm_utilPropMap;

/* Forward declaration for propMap initialization */
static fm_status EnDecodePortMapping(fm_utilEnDecode encode,
                                     void    *propMap,
                                     fm_byte *tlvVal,
                                     fm_int   tlvValSize,
                                     fm_char *propTxt,
                                     fm_int   propTxtSize,
                                     fm_int  *len);
static fm_status EnDecodeIntPortMapping(fm_utilEnDecode encode,
                                        void    *propMap,
                                        fm_byte *tlvVal,
                                        fm_int   tlvValSize,
                                        fm_char *propTxt,
                                        fm_int   propTxtSize,
                                        fm_int  *len);
static fm_status EnDecodePhyTxEqualizer(fm_utilEnDecode encode,
                                        void    *propMap,
                                        fm_byte *tlvVal,
                                        fm_int   tlvValSize,
                                        fm_char *propTxt,
                                        fm_int   propTxtSize,
                                        fm_int  *len);
static fm_status EnDecodeDevMemOff(fm_utilEnDecode endec,
                                   void    *propMap,
                                   fm_byte *tlvVal,
                                   fm_int   tlvValSize,
                                   fm_char *propTxt,
                                   fm_int   propTxtSize,
                                   fm_int  *len);
static fm_status EnDecodeByBitMap(fm_utilEnDecode endec,
                                  void           *propMap,
                                  fm_byte        *tlvVal,
                                  fm_int          tlvValSize,
                                  fm_char        *propTxt,
                                  fm_int          propTxtSize,
                                  fm_int         *len);
static fm_status EnDecodeByMap(fm_utilEnDecode endec,
                               void           *propMap,
                               fm_byte        *tlvVal,
                               fm_int          tlvValSize,
                               fm_char        *propTxt,
                               fm_int          propTxtSize,
                               fm_int         *len);

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_utilStrMap schedModeMap[] =
{
    { "static",  0},
    { "dynamic", 1},
};



static fm_utilStrMap polarityMap[] =
{
    { "INVERT_NONE",  0},
    { "INVERT_RX",    1},
    { "INVERT_TX",    2},
    { "INVERT_RX_TX", 3},
};

static fm_utilStrMap rxTermMap[] =
{
    { "TERM_HIGH",    0  },
    { "TERM_LOW",     1  },
    { "TERM_FLOAT",   2  },
};

static fm_utilStrMap topologyMap[] =
{
    { "UNDEFINED",      0},
    { "RING",           1},
    { "FAT_TREE",       2},
    { "MESH",           3},
};

static fm_utilStrMap swRoleMap[] =
{
    { "UNDEFINED",      0},
    { "LEAF",           1},
    { "SPINE",          2},
    { "SPINE_LEAF",     3},
    { "SWAG",           5},
};


static fm_utilStrMap ethModeMap[] =
{
    { "disabled",     0},
    { "1000Base-KX",  1},
    { "1000Base-X",   2},
    { "2500Base-X",   3},
    { "6GBase-CR",    4},
    { "10GBase-KX4",  5},
    { "10GBase-CX4",  6},
    { "10GBase-CR",   7},
    { "10GBase-SR",   8},
    { "24GBase-CR4",  9},
    { "25GBase-SR",   10},
    { "40GBase-SR4",  11},
    { "100GBase-SR4", 12},
    { "AN-73",        13},
    { "SGMII",        14},
    { "AUTODETECT",   15},
};

static fm_utilStrMap intfTypeMap[] =
{
    { "NONE",        0},
    { "SFPP",        1},
    { "QSFP_LANE0",  2},
    { "QSFP_LANE1",  3},
    { "QSFP_LANE2",  4},
    { "QSFP_LANE3",  5},
    { "PCIE",        6},
};

static fm_utilStrMap portCapMap[] =
{
    { "NONE",   0 },
    { "LAG",    (1 << 0)},
    { "ROUTE",  (1 << 1)},
    { "10M",    (1 << 2)},
    { "100M",   (1 << 3)},
    { "1G",     (1 << 4)},
    { "2PT5G",  (1 << 5)},
    { "5G",     (1 << 6)},
    { "10G",    (1 << 7)},
    { "20G",    (1 << 8)},
    { "25G",    (1 << 9)},
    { "40G",    (1 << 10)},
    { "100G",   (1 << 11)},
    { "SW_LED", (1 << 12)},

};

static fm_utilStrMap an73AbilityMap[] =
{
    { "RF",           (1 << 13)},
    { "NP",           (1 << 15)},
    { "1GBase-KX",    (1 << 21)},
    { "10GBase-KR",   (1 << 23)},
    { "40GBase-KR4",  (1 << 24)},
    { "40GBase-CR4",  (1 << 25)},
    { "100GBase-KR4", (1 << 28)},
    { "100GBase-CR4", (1 << 29) },
};

static fm_utilStrMap dfeModeMap[] =
{
    { "STATIC",     0},
    { "ONE_SHOT",   1},
    { "CONTINUOUS", 2},
    { "KR",         3},
    { "ICAL_ONLY",  4},
};

static fm_utilStrMap disableFuncIntfMap[] =
{
    { "NONE",               (1 << 0)},
    { "ResetSwitch",        (1 << 1)},
    { "I2cWriteRead",       (1 << 2)},
    { "DebugDump",          (1 << 3)},
    { "InitSwitch",         (1 << 4)},
    { "SelectBus",          (1 << 5)},
    { "GetPortXcvrState",   (1 << 6)},
    { "SetPortXcvrState",   (1 << 7)},
    { "SetPortLed",         (1 << 8)},
    { "EnablePortIntr",     (1 << 9)},
    { "GetPortIntrPending", (1 << 10)},
    { "PostInit",           (1 << 11)},
    { "SetVrmVoltage",      (1 << 12)},
    { "GetVrmVoltage",      (1 << 12)},

};

static fm_utilStrMap debugMap[] =
{
    { "NONE",      0},
    { "CONFIG",    (1 << 0)},
    { "MOD_STATE", (1 << 1)},
    { "MOD_INTR",  (1 << 2)},
    { "MOD_TYPE",  (1 << 3)},
    { "MOD_LED",   (1 << 4)},
    { "PLAT_LOG",  (1 << 5)},
    { "PORT_LOG",  (1 << 6)},
    { "INTR_LOG",  (1 << 7)},

};

static fm_utilStrMap ledBlkModeMap[] =
{
    { "NO_BLINK",     0},
    { "SW_CONTROL",   1},
    { "HW_ASSISTED",  2},
};

static fm_utilStrMap bootModeMap[] =
{
    { "SPI", 0},
    { "EBI", 1},
    { "I2C", 2},
};

static fm_utilStrMap regModeMap[] =
{
    { "PCIE", 0},
    { "EBI", 1},
    { "I2C", 2},
};

static fm_utilStrMap isrModeMap[] =
{
    { "AUTO", 0},
    { "SW", 1},
    { "SPI", 2},
};

static fm_utilStrMap libIntfTypeMap[] =
{
    { "NONE",  0},
    { "SFPP",  1},
    { "QSFP",  2},
    { "PCIE",  3},
};

static fm_utilStrMap busSelTypeMap[] =
{
    { "PCAMUX",  0},
    { "PCAIO",   1},
};

static fm_utilStrMap portLedTypeMap[] =
{
    { "NONE",  0},
    { "PCA",   1},
    { "FPGA",  2},
};

static fm_utilStrMap libDebugMap[] =
{
    { "NONE",            0       },
    { "I2C_RW",          (1 << 0)},
    { "SKIP_BUS_SEL",    (1 << 1)},
    { "FORCE_MOD_PRES",  (1 << 2)},
    { "I2C_MUX",         (1 << 3)},
    { "PORT_LED",        (1 << 4)},
    { "DUMP_CFG",        (1 << 5)},


};

static fm_utilStrMap portLedUsageMap[] =
{
    { "LINK",         (1 << 0)},
    { "TRAFFIC",      (1 << 1)},
    { "LINK_TRAFFIC", (1 << 2)},
    { "1G",           (1 << 3)},
    { "2.5G",         (1 << 4)},
    { "10G",          (1 << 5)},
    { "25G",          (1 << 6)},
    { "40G",          (1 << 7)},
    { "100G",         (1 << 8)},
    { "ALLSPEED",     (1 << 9)},
};

static fm_utilStrMap pcaMuxModelMap[] =
{
    { "PCA_UNKNOWN", 0 },
    { "PCA9541",     1 },
    { "PCA9545",     2 },
    { "PCA9546",     3 },
    { "PCA9548",     4 },
};

static fm_utilStrMap pcaIoModelMap[] =
{
    { "PCA_UNKNOWN", 0 },
    { "PCA9505",     1 },
    { "PCA9506",     2 },
    { "PCA9538",     3 },
    { "PCA9539",     4 },
    { "PCA9551",     5 },
    { "PCA9554",     6 },
    { "PCA9555",     7 },
    { "PCA9634",     8 },
    { "PCA9635",     9 },
    { "PCA9698",     10},
};

static fm_utilStrMap vrmModelMap[] =
{
    { "VRM_UNKNOWN", 0 },
    { "TPS40425",    1 },
    { "PX8847",      2 },
};


static fm_utilStrMap hwResTypeMap[] =
{
    { "PORT",  0},
    { "VRM",   1},
    { "PHY",   2},
};

static fm_utilStrMap phyModelMap[] =
{
    { "PHY_UNKNOWN", 0 },
    { "GN2412",      1  },

};


/* API properties */
static fm_utilPropMap apiProp[] = {
    {"api.system.ebi.clock",
        PROP_INT, FM_TLV_API_EBI_CLOCK, 4, NULL, NULL, 0},
    {"api.system.fhRef.clock",
        PROP_INT, FM_TLV_API_FH_REF_CLK, 4, NULL, NULL, 0},
    {"api.stp.defaultState.vlanMember",
        PROP_INT, FM_TLV_API_STP_DEF_VLAN_MEMBER, 1, NULL, NULL, 0},
    {"api.allowDirectSendToCpu",
        PROP_BOOL, FM_TLV_API_DIR_SEND_TO_CPU, 1, NULL, NULL, 0},
    {"api.stp.defaultState.vlanNonMember",
        PROP_INT, FM_TLV_API_STP_DEF_VLAN_NON_MEMBER, 1, NULL, NULL, 0},
    {"debug.boot.identifySwitch",
        PROP_BOOL, FM_TLV_API_IDENTIFY_SWITCH, 1, NULL, NULL, 0},
    {"debug.boot.reset",
        PROP_BOOL, FM_TLV_API_BOOT_RESET, 1, NULL, NULL, 0},
    {"debug.boot.autoInsertSwitches",
        PROP_BOOL, FM_TLV_API_AUTO_INSERT_SW, 1, NULL, NULL, 0},
    {"api.boot.deviceResetTime",
        PROP_INT, FM_TLV_API_DEV_RESET_TIME, 4, NULL, NULL, 0},
    {"api.swag.autoEnableLinks",
        PROP_BOOL, FM_TLV_API_SWAG_EN_LINK, 1, NULL, NULL, 0},
    {"api.event.blockThreshold",
        PROP_INT, FM_TLV_API_EVENT_BLK_THRESHOLD, 2, NULL, NULL, 0},
    {"api.event.unblockThreshold",
        PROP_INT, FM_TLV_API_EVENT_UNBLK_THRESHOLD, 2, NULL, NULL, 0},
    {"api.event.semaphoreTimeout",
        PROP_INT, FM_TLV_API_EVENT_SEM_TIMEOUT, 2, NULL, NULL, 0},
    {"api.packet.rxDirectEnqueueing",
        PROP_BOOL, FM_TLV_API_RX_DIRECTED_ENQ, 1, NULL, NULL, 0},
    {"api.packet.rxDriverDestinations",
        PROP_INT, FM_TLV_API_RX_DRV_DEST, 1, NULL, NULL, 0},
#if 0
    {"api.boot.portOutOfResetMask",
        PROP_INT_H, FM_TLV_API_BOOT_PORT_RESET_MASK, 1, NULL, NULL, 0},
#endif
    {"api.lag.asyncDeletion",
        PROP_BOOL, FM_TLV_API_LAG_ASYNC_DEL, 1, NULL, NULL, 0},
    {"api.ma.eventOnStaticAddr",
        PROP_BOOL, FM_TLV_API_EVENT_ON_STATIC_ADDR, 1, NULL, NULL, 0},
    {"api.ma.eventOnDynamicAddr",
        PROP_BOOL, FM_TLV_API_EVENT_ON_DYN_ADDR, 1, NULL, NULL, 0},
    {"api.ma.flushOnPortDown",
        PROP_BOOL, FM_TLV_API_FLUSH_ON_PORT_DOWN, 1, NULL, NULL, 0},
    {"api.ma.flushOnVlanChange",
        PROP_BOOL, FM_TLV_API_FLUSH_ON_VLAN_CHG, 1, NULL, NULL, 0},
    {"api.ma.flushOnLagChange",
        PROP_BOOL, FM_TLV_API_FLUSH_ON_LAG_CHG, 1, NULL, NULL, 0},
    {"api.ma.tcnFifoBurstSize",
        PROP_INT, FM_TLV_API_TCN_FIFO_BURST_SIZE, 2, NULL, NULL, 0},
    {"api.swag.internalPort.vlanStats",
        PROP_BOOL, FM_TLV_API_SWAG_INT_VLAN_STATS, 1, NULL, NULL, 0},
    {"api.perLagManagement",
        PROP_BOOL, FM_TLV_API_PER_LAG_MGMT, 1, NULL, NULL, 0},
    {"api.parityRepair.enable",
        PROP_BOOL, FM_TLV_API_PARITY_REPAIR_EN, 1, NULL, NULL, 0},
    {"api.SWAG.maxACLPortSets",
        PROP_INT, FM_TLV_API_SWAG_MAX_ACL_PORT_SET, 2, NULL, NULL, 0},
    {"api.portSets.maxPortSets",
        PROP_INT, FM_TLV_API_MAX_PORT_SETS, 2, NULL, NULL, 0},
    {"api.packetReceive.enable",
        PROP_BOOL, FM_TLV_API_PKT_RX_EN, 1, NULL, NULL, 0},
    {"api.multicast.singleAddress",
        PROP_BOOL, FM_TLV_API_MC_SINGLE_ADDR, 1, NULL, NULL, 0},
    {"api.platform.model.position",
        PROP_INT, FM_TLV_API_PLAT_MODEL_POS, 1, NULL, NULL, 0},
    {"api.vn.numNextHops",
        PROP_INT, FM_TLV_API_VN_NUM_NH, 2, NULL, NULL, 0},
    {"api.vn.encapProtocol",
        PROP_INT_H, FM_TLV_API_VN_ENCAP_PROTOCOL, 2, NULL, NULL, 0},
    {"api.vn.encapVersion",
        PROP_INT, FM_TLV_API_VN_ENCAP_VER, 1, NULL, NULL, 0},
    {"api.routing.supportRouteLookups",
        PROP_BOOL, FM_TLV_API_SUP_ROUTE_LOOKUP, 1, NULL, NULL, 0},
    {"api.routing.maintenance.enable",
        PROP_BOOL, FM_TLV_API_RT_MAINT_EN, 1, NULL, NULL, 0},
    {"api.vlan.autoVlan2Tagging",
        PROP_BOOL, FM_TLV_API_AUTO_VLAN2_TAG, 1, NULL, NULL, 0},
    {"api.ma.eventOnAddrChange",
        PROP_BOOL, FM_TLV_API_EVENT_ON_ADDR_CHG, 1, NULL, NULL, 0},

    /* Start of undocumented properties */
    {"debug.boot.interruptHandler.disable",
        PROP_BOOL, FM_TLV_API_INTR_HANDLER_DIS, 1, NULL, NULL, 0},
    {"api.maTableMaintenance.enable",
        PROP_BOOL, FM_TLV_API_MA_TABLE_MAINT_EN, 1, NULL, NULL, 0},
    {"api.fastMaintenance.enable",
        PROP_BOOL, FM_TLV_API_FAST_MAINT_EN, 1, NULL, NULL, 0},
    {"api.fastMaintenance.period",
        PROP_INT, FM_TLV_API_FAST_MAINT_PER, 4, NULL, NULL, 0},
#if 0
    {"api.vegas.uplinkPortsFirst",
        PROP_BOOL, FM_TLV_API_VEGAS_UPLNK_FIRST, 1, NULL, NULL, 0},
#endif
    {"api.strict.glortPhysical",
        PROP_BOOL, FM_TLV_API_STRICT_GLORT, 1, NULL, NULL, 0},
    {"api.autoPauseOff.resetWatermark",
        PROP_BOOL, FM_TLV_API_AUTO_RESET_WM, 1, NULL, NULL, 0},
    {"api.swag.autoSubSwitches",
        PROP_BOOL, FM_TLV_API_AUTO_SUB_SW, 1, NULL, NULL, 0},
    {"api.swag.autoInternalPorts",
        PROP_BOOL, FM_TLV_API_SWAG_AUTO_INT_PORT, 1, NULL, NULL, 0},
    {"api.swag.autoVNVsi",
        PROP_BOOL, FM_TLV_API_SWAG_AUTO_NVVSI, 1, NULL, NULL, 0},
    {"api.FM4000.FIBM.testRig",
        PROP_INT, FM_TLV_API_FIBM_TEST_RIG, 2, NULL, NULL, 0},
    {"api.FM4000.FIBM.testPort",
        PROP_INT, FM_TLV_API_FIBM_TEST_PORT, 2, NULL, NULL, 0},
    {"api.portRemapTable",
        PROP_TEXT, FM_TLV_API_PORT_REMAP_TABLE, 0, NULL, NULL, 0},
    {"api.platform.bypassEnable",
        PROP_BOOL, FM_TLV_API_PLAT_BYPASS_EN, 1, NULL, NULL, 0},
    {"api.lag.delSemaphore.timeout",
        PROP_INT, FM_TLV_API_LAG_DEL_SEM_TIMEOUT, 2, NULL, NULL, 0},
    {"api.stp.enableInternalPortControl",
        PROP_BOOL, FM_TLV_API_STP_EN_INT_PORT_CTRL, 1, NULL, NULL, 0},
    {"api.generateEepromMode",
        PROP_INT, FM_TLV_API_GEN_EEPROM_MODE, 1, NULL, NULL, 0},
    {"api.platform.model.portMapType",
        PROP_INT, FM_TLV_API_MODEL_PORT_MAP_TYPE, 1, NULL, NULL, 0},
    {"api.platform.model.switchType",
        PROP_TEXT, FM_TLV_API_MODEL_SW_TYPE, 0, NULL, NULL, 0},
    {"api.platform.model.sendEOT",
        PROP_BOOL, FM_TLV_API_MODEL_TX_EOT, 1, NULL, NULL, 0},
    {"api.platform.model.logEgressInfo",
        PROP_BOOL, FM_TLV_API_MODEL_LOG_EGR_INFO, 1, NULL, NULL, 0},
    {"api.platform.enableRefClock",
        PROP_BOOL, FM_TLV_API_PLAT_EN_REF_CLK, 1, NULL, NULL, 0},
    {"api.platform.setRefClock",
        PROP_BOOL, FM_TLV_API_PLAT_SET_REF_CLK, 1, NULL, NULL, 0},
    {"api.platform.i2cDevName",
        PROP_TEXT, FM_TLV_API_PLAT_I2C_DEVNAME, 0, NULL, NULL, 0},
    {"api.platform.priorityBufferQueues",
        PROP_INT, FM_TLV_API_PLAT_PRI_BUF_Q, 1, NULL, NULL, 0},
    {"api.platform.pktSchedType",
        PROP_INT, FM_TLV_API_PLAT_PKT_SCHED_TYPE, 1, NULL, NULL, 0},
    {"api.platform.separateBufferPoolEnable",
        PROP_BOOL, FM_TLV_API_PLAT_SEP_BUF_POOL, 1, NULL, NULL, 0},
    {"api.platform.numBuffersRx",
        PROP_INT, FM_TLV_API_PLAT_NUM_BUF_RX, 4, NULL, NULL, 0},
    {"api.platform.numBuffersTx",
        PROP_INT, FM_TLV_API_PLAT_NUM_BUF_TX, 4, NULL, NULL, 0},
    {"api.platform.model.pktInterface",
        PROP_TEXT, FM_TLV_API_PLAT_MODEL_PKT_INTF, 0, NULL, NULL, 0},
    {"api.platform.pktInterface",
        PROP_TEXT, FM_TLV_API_PLAT_PKT_INTF, 0, NULL, NULL, 0},
    {"api.platform.model.topology.name",
        PROP_TEXT, FM_TLV_API_PLAT_MODEL_TOPO, 0, NULL, NULL, 0},
    {"api.platform.model.topology.file.useModelPath",
        PROP_BOOL, FM_TLV_API_PLAT_MODEL_USE_PATH, 1, NULL, NULL, 0},
    {"api.platform.model.devBoardIp",
        PROP_TEXT, FM_TLV_API_PLAT_DEV_BOARD_IP, 0, NULL, NULL, 0},
    {"api.platform.model.devBoardPort",
        PROP_INT, FM_TLV_API_PLAT_DEV_BOARD_PORT, 2, NULL, NULL, 0},
    {"api.platform.model.deviceCfg",
        PROP_INT_H, FM_TLV_API_PLAT_MODEL_DEVICE_CFG, 4, NULL, NULL, 0},
    {"api.platform.model.chipVersion",
        PROP_INT_H, FM_TLV_API_PLAT_MODEL_CHIP_VERSION, 4, NULL, NULL, 0},
    {"api.platform.sbusServer.tcpPort",
        PROP_INT, FM_TLV_API_PLAT_SBUS_SERVER_PORT, 2, NULL, NULL, 0},
    {"api.platform.isWhiteModel",
        PROP_BOOL, FM_TLV_API_PLAT_IS_WHITE_MODEL, 1, NULL, NULL, 0},
    {"api.platform.TswitchOffset",
        PROP_INT, FM_TLV_API_PLAT_TSW_OFF, 1, NULL, NULL, 0},
    {"api.platform.portLedBlinkEnable",
        PROP_BOOL, FM_TLV_API_PLAT_LED_BLINK_EN, 1, NULL, NULL, 0},
    {"api.debug.initLoggingCategories",
        PROP_TEXT, FM_TLV_API_DBG_INT_LOG_CAT, 0, NULL, NULL, 0},
    {"api.port.addPepsToFlooding",
        PROP_BOOL, FM_TLV_API_ADD_PEP_FLOOD, 1, NULL, NULL, 0},
    {"api.port.allowFtagVlanTagging",
        PROP_BOOL, FM_TLV_API_ADD_ALLOW_FTAG_VLAN_TAG, 1, NULL, NULL, 0},
    {"api.scheduler.ignoreBwViolationNoWarning",
        PROP_BOOL, FM_TLV_API_IGNORE_BW_VIOLATION_NW, 1, NULL, NULL, 0},
    {"api.scheduler.ignoreBwViolation",
        PROP_BOOL, FM_TLV_API_IGNORE_BW_VIOLATION, 1, NULL, NULL, 0},
    {"api.dfe.allowEarlyLinkUp",
        PROP_BOOL, FM_TLV_API_DFE_EARLY_LNK_UP, 1, NULL, NULL, 0},
    {"api.dfe.allowKrPcal",
        PROP_BOOL, FM_TLV_API_DFE_ALLOW_KR_PCAL, 1, NULL, NULL, 0},
    {"api.gsme.timestampMode",
        PROP_INT, FM_TLV_API_GSME_TS_MODE, 1, NULL, NULL, 0},
    {"api.multicast.hni.flooding",
        PROP_BOOL, FM_TLV_API_MC_HNI_FLOODING, 1, NULL, NULL, 0},
    {"api.hni.macEntriesPerPep",
        PROP_INT, FM_TLV_API_HNI_MAC_ENTRIES_PER_PEP, 2, NULL, NULL, 0},
    {"api.hni.macEntriesPerPort",
        PROP_INT, FM_TLV_API_HNI_MAC_ENTRIES_PER_PORT, 2, NULL, NULL, 0},
    {"api.hni.innOutEntriesPerPep",
        PROP_INT, FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PEP, 2, NULL, NULL, 0},
    {"api.hni.innOutEntriesPerPort",
        PROP_INT, FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PORT, 2, NULL, NULL, 0},
    {"api.port.enableStatusPolling",
        PROP_BOOL, FM_TLV_API_PORT_ENABLE_STATUS_POLLING, 1, NULL, NULL, 0},
    {"api.dfe.enableSigOkDebounce",
        PROP_BOOL, FM_TLV_API_DFE_ENABLE_SIGNALOK_DEBOUNCING, 1, NULL, NULL, 0},
    {"api.an.timerAllowOutSpec",
        PROP_BOOL, FM_TLV_API_AN_TIMER_ALLOW_OUT_SPEC, 1, NULL, NULL, 0},

};

/* FM10K properties starting with api.FM10000. */
static fm_utilPropMap fm10kProp[] = {
    {"wmSelect", PROP_TEXT, FM_TLV_FM10K_WMSELECT, 0,
        NULL, NULL, 0},
    {"cmRxSmpPrivateBytes", PROP_INT, FM_TLV_FM10K_CM_RX_SMP_PRIVATE_BYTES, 4,
        NULL, NULL, 0},
    {"cmTxTcHogBytes", PROP_INT, FM_TLV_FM10K_CM_TX_TC_HOG_BYTES, 4,
        NULL, NULL, 0},
    {"cmSmpSdVsHogPercent", PROP_INT, FM_TLV_FM10K_CM_SMP_SD_VS_HOG_PERCENT, 1,
        NULL, NULL, 0},
    {"cmSmpSdJitterBits", PROP_INT, FM_TLV_FM10K_CM_SMP_SD_JITTER_BITS, 1,
        NULL, NULL, 0},
    {"cmTxSdOnPrivate", PROP_BOOL, FM_TLV_FM10K_CM_TX_SD_ON_PRIVATE, 1,
        NULL, NULL, 0},
    {"cmTxSdOnSmpFree", PROP_BOOL, FM_TLV_FM10K_CM_TX_SD_ON_SMP_FREE, 1,
        NULL, NULL, 0},
    {"cmPauseBufferBytes", PROP_INT, FM_TLV_FM10K_CM_PAUSE_BUFFER_BYTES, 4,
        NULL, NULL, 0},
    {"mcastMaxEntriesPerCam", PROP_INT, FM_TLV_FM10K_MCAST_MAX_ENTRIES_PER_CAM, 2,
        NULL, NULL, 0},
    {"ffu.unicastSliceRangeFirst", PROP_INT, FM_TLV_FM10K_FFU_UNICAST_SLICE_1ST, 1,
        NULL, NULL, 0},
    {"ffu.unicastSliceRangeLast", PROP_INT, FM_TLV_FM10K_FFU_UNICAST_SLICE_LAST, 1,
        NULL, NULL, 0},
    {"ffu.multicastSliceRangeFirst", PROP_INT, FM_TLV_FM10K_FFU_MULTICAST_SLICE_1ST, 1,
        NULL, NULL, 0},
    {"ffu.multicastSliceRangeLast", PROP_INT, FM_TLV_FM10K_FFU_MULTICAST_SLICE_LAST, 1,
        NULL, NULL, 0},
    {"ffu.aclSliceRangeFirst", PROP_INT, FM_TLV_FM10K_FFU_ACL_SLICE_1ST, 1,
        NULL, NULL, 0},
    {"ffu.aclSliceRangeLast", PROP_INT, FM_TLV_FM10K_FFU_ACL_SLICE_LAST, 1,
        NULL, NULL, 0},
    {"ffu.mapMAC.reservedForRouting", PROP_INT, FM_TLV_FM10K_FFU_MAPMAC_ROUTING, 1,
        NULL, NULL, 0},
    {"ffu.unicastPrecedenceMin", PROP_INT, FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MIN, 1,
        NULL, NULL, 0},
    {"ffu.unicastPrecedenceMax", PROP_INT, FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MAX, 1,
        NULL, NULL, 0},
     {"ffu.multicastPrecedenceMin", PROP_INT, FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MIN, 1,
        NULL, NULL, 0},
    {"ffu.multicastPrecedenceMax", PROP_INT, FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MAX, 1,
        NULL, NULL, 0},
    {"ffu.aclPrecedenceMin", PROP_INT, FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MIN, 1,
        NULL, NULL, 0},
    {"ffu.aclPrecedenceMax", PROP_INT, FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MAX, 1,
        NULL, NULL, 0},
    {"ffu.ACLStrictCountPolice", PROP_BOOL, FM_TLV_FM10K_FFU_ACL_STRICT_COUNT_POLICE, 1,
        NULL, NULL, 0},
    {"initUcastFloodingTriggers", PROP_BOOL, FM_TLV_FM10K_INIT_UCAST_FLOODING_TRIGGERS, 1,
        NULL, NULL, 0},
    {"initMcastFloodingTriggers", PROP_BOOL, FM_TLV_FM10K_INIT_MCAST_FLOODING_TRIGGERS, 1,
        NULL, NULL, 0},
    {"initBcastFloodingTriggers", PROP_BOOL, FM_TLV_FM10K_INIT_BCAST_FLOODING_TRIGGERS, 1,
        NULL, NULL, 0},
    {"initReservedMacTriggers", PROP_BOOL, FM_TLV_FM10K_INIT_RESERVED_MAC_TRIGGERS, 1,
        NULL, NULL, 0},
    {"floodingTrapPriority", PROP_INT, FM_TLV_FM10K_FLOODING_TRAP_PRIORITY, 1,
        NULL, NULL, 0},
    {"autoNeg.generateEvents", PROP_BOOL, FM_TLV_FM10K_AUTONEG_GENERATE_EVENTS, 1,
        NULL, NULL, 0},
    {"linkDependsOnDfe", PROP_BOOL, FM_TLV_FM10K_LINK_DEPENDS_ON_DFE, 1,
        NULL, NULL, 0},
    {"vn.useSharedEncapFlows", PROP_BOOL, FM_TLV_FM10K_VN_USE_SHARED_ENCAP_FLOWS, 1,
        NULL, NULL, 0},
    {"vn.maxRemoteAddresses", PROP_INT, FM_TLV_FM10K_VN_MAX_TUNNEL_RULES, 4,
        NULL, NULL, 0},
    {"vn.tunnelGroupHashSize", PROP_INT, FM_TLV_FM10K_VN_TUNNEL_GROUP_HASH_SIZE, 4,
        NULL, NULL, 0},
    {"vn.teVID", PROP_INT, FM_TLV_FM10K_VN_TE_VID, 2,
        NULL, NULL, 0},
    {"vn.encapAclNumber", PROP_INT, FM_TLV_FM10K_VN_ENCAP_ACL_NUM, 4,
        NULL, NULL, 0},
    {"vn.decapAclNumber", PROP_INT, FM_TLV_FM10K_VN_DECAP_ACL_NUM, 4,
        NULL, NULL, 0},
    {"mcast.numStackGroups", PROP_INT, FM_TLV_FM10K_MCAST_NUM_STACK_GROUPS, 2,
        NULL, NULL, 0},
    {"vn.TunnelOnlyOnIngress", PROP_BOOL, FM_TLV_FM10K_VN_TUNNEL_ONLY_IN_INGRESS, 1,
        NULL, NULL, 0},
    {"mtable.cleanupWatermark", PROP_INT, FM_TLV_FM10K_MTABLE_CLEANUP_WATERMARK, 1,
        NULL, NULL, 0},
    {"schedMode", PROP_TEXT, FM_TLV_FM10K_SCHED_MODE, 0, NULL, NULL, 0},
    {"updateSchedOnLinkChange", PROP_BOOL, FM_TLV_FM10K_UPD_SCHED_ON_LNK_CHANGE, 1,
        NULL, NULL, 0},
    {"parity.crmTimeout", PROP_INT, FM_TLV_FM10K_PARITY_CRM_TIMEOUT, 1,
        NULL, NULL, 0},

    {"createRemoteLogicalPorts", PROP_BOOL, FM_TLV_FM10K_CREATE_REMOTE_LOGICAL_PORTS, 1,
        NULL, NULL, 0},
    {"autoNeg.clause37.timeout", PROP_INT, FM_TLV_FM10K_AUTONEG_CLAUSE_37_TIMEOUT, 2,
        NULL, NULL, 0},
    {"autoNeg.SGMII.timeout", PROP_INT, FM_TLV_FM10K_AUTONEG_SGMII_TIMEOUT, 1,
        NULL, NULL, 0},
    {"useHniServicesLoopback", PROP_BOOL, FM_TLV_FM10K_HNI_SERVICES_LOOPBACK, 1,
        NULL, NULL, 0},
    {"antiBubbleWm", PROP_INT, FM_TLV_FM10K_ANTI_BUBBLE_WM, 1,
        NULL, NULL, 0},
    {"serdesOpMode", PROP_INT, FM_TLV_FM10K_SERDES_OP_MODE, 1,
        NULL, NULL, 0},
    {"serdesDbgLevel", PROP_INT, FM_TLV_FM10K_SERDES_DBG_LVL, 1,
        NULL, NULL, 0},
    {"parity.enableInterrupts", PROP_BOOL, FM_TLV_FM10K_PARITY_INTERRUPTS, 1,
        NULL, NULL, 0},
    {"parity.startTcamMonitors", PROP_BOOL, FM_TLV_FM10K_PARITY_TCAM_MONITOR, 1,
        NULL, NULL, 0},
    {"schedOverspeed", PROP_INT, FM_TLV_FM10K_SCHED_OVERSPEED, 4,
        NULL, NULL, 0},
    {"intr.linkIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_LINK_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.autoNegIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_AUTONEG_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.serdesIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_SERDES_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.pcieIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_PCIE_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.maTcnIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_MATCN_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.fhTailIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_FHTAIL_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.swIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_SW_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"intr.teIgnoreMask", PROP_INT_H, FM_TLV_FM10K_INTR_TE_IGNORE_MASK, 4,
        NULL, NULL, 0},
    {"enable.eeeSpicoIntr", PROP_BOOL, FM_TLV_FM10K_EEE_SPICO_INTR, 1,
        NULL, NULL, 0},
    {"useAlternateSpicoFw", PROP_BOOL, FM_TLV_FM10K_USE_ALTERNATE_SPICO_FW, 1,
        NULL, NULL, 0},
    {"allowKrPcalOnEee", PROP_BOOL, FM_TLV_FM10K_ALLOW_KRPCAL_ON_EEE, 1,
        NULL, NULL, 0},
};


/* Property starting with api.platform.config. */
static fm_utilPropMap platConfig[] = {
    {"numSwitches", PROP_UINT, FM_TLV_PLAT_NUM_SW, 1, NULL, NULL, 0},
    {"platformName", PROP_TEXT, FM_TLV_PLAT_NAME, 0, NULL, NULL, 0},
    {"fileLockName", PROP_TEXT, FM_TLV_PLAT_FILE_LOCK_NAME, 0, NULL, NULL, 0},
    {"switch.topology", PROP_TEXT, FM_TLV_PLAT_SW_TOPOLOGY, 1,
        EnDecodeByMap, topologyMap, FM_NENTRIES(topologyMap)},
    {"ebiDevName", PROP_TEXT, FM_TLV_PLAT_EBI_DEVNAME, 0, NULL, NULL, 0},
    {"debug", PROP_TEXT, FM_TLV_PLAT_DEBUG, 4,
        EnDecodeByBitMap, debugMap, FM_NENTRIES(debugMap)},

};

/* Property starting with api.platform.config.switch.%d */
static fm_utilPropMap platConfigSw[] = {
    {"switchNumber", PROP_UINT, FM_TLV_PLAT_SW_NUMBER, 1, NULL, NULL, 0},
    {"numPorts", PROP_UINT, FM_TLV_PLAT_SW_NUM_PORTS, 1, NULL, NULL, 0},
    {"ledPollPeriodMsec",
        PROP_UINT, FM_TLV_PLAT_SW_LED_POLL_PER, 2, NULL, NULL, 0},
    {"ledBlinkMode",
        PROP_TEXT, FM_TLV_PLAT_SW_LED_BLINK_MODE, 1,
        EnDecodeByMap, ledBlkModeMap, FM_NENTRIES(ledBlkModeMap)},
    {"xcvrPollPeriodMsec",
        PROP_UINT, FM_TLV_PLAT_SW_XCVR_POLL_PER, 2, NULL, NULL, 0},
    {"port.default.hwResourceId",
        PROP_INT_H, FM_TLV_PLAT_SW_PORT_DEF_HW_ID, 4, NULL, NULL, 0},
    {"role", PROP_TEXT, FM_TLV_PLAT_SW_ROLE, 1,
        EnDecodeByMap, swRoleMap, FM_NENTRIES(swRoleMap)},
    {"port.default.ethernetMode",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_DEF_ETHMODE, 1,
        EnDecodeByMap, ethModeMap, FM_NENTRIES(ethModeMap)},
    {"port.default.lanePolarity",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_DEF_LANE_POL, 1,
        EnDecodeByMap, polarityMap, FM_NENTRIES(polarityMap)},
    {"port.default.preCursorCopper", 
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_CU, 1, NULL, NULL, 0},
    {"port.default.preCursorOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_OPT, 1, NULL, NULL, 0},
    {"port.default.cursorCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_CU, 1, NULL, NULL, 0},
    {"port.default.cursorOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_OPT, 1, NULL, NULL, 0},
    {"port.default.postCursorCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_CU, 1, NULL, NULL, 0},
    {"port.default.postCursorOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_OPT, 1, NULL, NULL, 0},
    {"port.default.interfaceType",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_DEF_INTF_TYPE, 1,
        EnDecodeByMap, intfTypeMap, FM_NENTRIES(intfTypeMap)},
    {"port.default.capability",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_DEF_CAP, 4,
        EnDecodeByBitMap, portCapMap, FM_NENTRIES(portCapMap)},
    {"port.default.dfeMode",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_DEF_DFE_MODE, 1,
        EnDecodeByMap, dfeModeMap, FM_NENTRIES(dfeModeMap)},
    {"sharedLibraryName",
        PROP_TEXT, FM_TLV_PLAT_SW_SHARED_LIB_NAME, 0, NULL, NULL, 0},
    {"sharedLibrary.disable",
        PROP_TEXT, FM_TLV_PLAT_SW_SHARED_LIB_DISABLE, 4, 
        EnDecodeByBitMap, disableFuncIntfMap, FM_NENTRIES(disableFuncIntfMap)},
    {"portIntrGpio",
        PROP_UINT, FM_TLV_PLAT_SW_PORT_INTR_GPIO, 1, NULL, NULL, 0},
    {"i2cResetGpio",
        PROP_UINT, FM_TLV_PLAT_SW_I2C_RESET_GPIO, 1, NULL, NULL, 0},
    {"flashWriteProtectGpio",
        PROP_UINT, FM_TLV_PLAT_SW_FLASH_WP_GPIO, 1, NULL, NULL, 0},
    {"keepSerdesCfg",
        PROP_BOOL, FM_TLV_PLAT_SW_KEEP_SERDES_CFG, 1, NULL, NULL, 0},
    {"numPhys", PROP_UINT, FM_TLV_PLAT_SW_NUM_PHYS, 1, NULL, NULL, 0},

    {"devMemOffset",
        PROP_TEXT, FM_TLV_PLAT_DEV_MEM_OFF, 8, EnDecodeDevMemOff, NULL, 0},
    {"netDevName", PROP_TEXT, FM_TLV_PLAT_NET_DEVNAME, 0, NULL, NULL, 0},
    {"uioDevName", PROP_TEXT, FM_TLV_PLAT_UIO_DEVNAME, 0, NULL, NULL, 0},
    {"intrTimeoutCnt", PROP_UINT, FM_TLV_PLAT_INTR_TIMEOUT_CNT, 2, NULL, NULL, 0},
    {"bootMode", PROP_TEXT, FM_TLV_PLAT_BOOT_MODE, 1, 
        EnDecodeByMap, bootModeMap, FM_NENTRIES(bootModeMap)},
    {"regAccess", PROP_TEXT, FM_TLV_PLAT_REG_ACCESS, 1,
        EnDecodeByMap, regModeMap, FM_NENTRIES(regModeMap)},
    {"pcieIsr", PROP_TEXT, FM_TLV_PLAT_PCIE_ISR, 1,
        EnDecodeByMap, isrModeMap, FM_NENTRIES(isrModeMap)},
    {"cpuPort", PROP_UINT, FM_TLV_PLAT_CPU_PORT, 2, NULL, NULL, 0},
    {"intPollMsec", PROP_UINT, FM_TLV_PLAT_INTR_POLL_PER, 2, NULL, NULL, 0},
    {"phyEnableDeemphasis",
        PROP_BOOL, FM_TLV_PLAT_PHY_EN_DEEMPHASIS, 1, NULL, NULL, 0},
    {"msiEnabled", PROP_BOOL, FM_TLV_PLAT_SW_MSI_ENABLE, 1, NULL, NULL, 0},
    {"fhClock", PROP_INT, FM_TLV_PLAT_SW_FH_CLOCK, 4, NULL, NULL, 0},
    {"useDefVoltageScaling", PROP_INT, FM_TLV_PLAT_SW_VRM_USE_DEF_VOLTAGE, 4, NULL, NULL, 0},
    {"VDDS.hwResourceId", PROP_INT_H, FM_TLV_PLAT_SW_VDDS_USE_HW_RESOURCE_ID, 4, NULL, NULL, 0},
    {"VDDF.hwResourceId", PROP_INT_H, FM_TLV_PLAT_SW_VDDF_USE_HW_RESOURCE_ID, 4, NULL, NULL, 0},
    {"AVDD.hwResourceId", PROP_INT_H, FM_TLV_PLAT_SW_AVDD_USE_HW_RESOURCE_ID, 4, NULL, NULL, 0},
    {"i2cClkDivider", PROP_UINT, FM_TLV_PLAT_SW_I2C_CLKDIVIDER, 1, NULL, NULL, 0},
};

/* Property starting with api.platform.config.switch.%d.internalPortIndex.%d */
static fm_utilPropMap platConfigSwIntPortIdx[] = {
    {"portMapping", PROP_TEXT, FM_TLV_PLAT_SW_INT_PORT_MAPPING, 6,
        EnDecodeIntPortMapping, NULL, 0},
};

/* Property starting with api.platform.config.switch.%d.portIndex.%d */
static fm_utilPropMap platConfigSwPortIdx[] = {
    {"speed", PROP_INT, FM_TLV_PLAT_SW_PORTIDX_SPEED, 4, NULL, NULL, 0},
    {"portMapping",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_MAPPING, 6, EnDecodePortMapping, NULL, 0},
    {"hwResourceId",
        PROP_INT_H, FM_TLV_PLAT_SW_PORTIDX_HW_ID, 4, NULL, NULL, 0},
    {"ethernetMode",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_ETHMODE, 1,
        EnDecodeByMap, ethModeMap, FM_NENTRIES(ethModeMap)},
    {"lanePolarity",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_POL, 1,
        EnDecodeByMap, polarityMap, FM_NENTRIES(polarityMap)},
    {"rxTermination",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_RX_TERM, 1,
        EnDecodeByMap, rxTermMap, FM_NENTRIES(rxTermMap)},
    {"preCursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_1G, 1, NULL, NULL, 0},
    {"preCursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_10G, 1, NULL, NULL, 0},
    {"preCursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_25G, 1, NULL, NULL, 0},
    {"preCursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_1G, 1, NULL, NULL, 0},
    {"preCursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_10G, 1, NULL, NULL, 0},
    {"preCursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_25G, 1, NULL, NULL, 0},
    {"cursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_1G, 1, NULL, NULL, 0},
    {"cursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_10G, 1, NULL, NULL, 0},
    {"cursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_25G, 1, NULL, NULL, 0},
    {"cursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_1G, 1, NULL, NULL, 0},
    {"cursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_10G, 1, NULL, NULL, 0},
    {"cursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_25G, 1, NULL, NULL, 0},
    {"postCursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_1G, 1, NULL, NULL, 0},
    {"postCursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_10G, 1, NULL, NULL, 0},
    {"postCursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_25G, 1, NULL, NULL, 0},
    {"postCursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_1G, 1, NULL, NULL, 0},
    {"postCursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_10G, 1, NULL, NULL, 0},
    {"postCursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_25G, 1, NULL, NULL, 0},
    {"interfaceType",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_INTF_TYPE, 1,
        EnDecodeByMap, intfTypeMap, FM_NENTRIES(intfTypeMap)},
    {"capability",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_CAP, 4,
        EnDecodeByBitMap, portCapMap, FM_NENTRIES(portCapMap)},
    {"dfeMode",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_DFE_MODE, 1,
        EnDecodeByMap, dfeModeMap, FM_NENTRIES(dfeModeMap)},
    {"an73Ability",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_AN73_ABILITY, 4,
        EnDecodeByBitMap, an73AbilityMap, FM_NENTRIES(an73AbilityMap)},
};

/* Property starting with api.platform.config.switch.%d.portIndex.%d.lane.%d */
static fm_utilPropMap platConfigSwPortIdxLane[] = {
    {"lanePolarity",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_LANE_POL, 1,
        EnDecodeByMap, polarityMap, FM_NENTRIES(polarityMap)},
    {"rxTermination",
        PROP_TEXT, FM_TLV_PLAT_SW_PORTIDX_LANE_RX_TERM, 1,
        EnDecodeByMap, rxTermMap, FM_NENTRIES(rxTermMap)},
    {"portMapping",
        PROP_TEXT, FM_TLV_PLAT_SW_PORT_LANE_MAPPING, 6, EnDecodePortMapping, NULL, 0},
    {"preCursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_1G, 1, NULL, NULL, 0},
    {"preCursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_10G, 1, NULL, NULL, 0},
    {"preCursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_25G, 1, NULL, NULL, 0},
    {"preCursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_1G, 1, NULL, NULL, 0},
    {"preCursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_10G, 1, NULL, NULL, 0},
    {"preCursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_25G, 1, NULL, NULL, 0},
    {"cursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_1G, 1, NULL, NULL, 0},
    {"cursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_10G, 1, NULL, NULL, 0},
    {"cursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_25G, 1, NULL, NULL, 0},
    {"cursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_1G, 1, NULL, NULL, 0},
    {"cursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_10G, 1, NULL, NULL, 0},
    {"cursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_25G, 1, NULL, NULL, 0},
    {"postCursor1GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_1G, 1, NULL, NULL, 0},
    {"postCursor10GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_10G, 1, NULL, NULL, 0},
    {"postCursor25GCopper",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_25G, 1, NULL, NULL, 0},
    {"postCursor1GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_1G, 1, NULL, NULL, 0},
    {"postCursor10GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_10G, 1, NULL, NULL, 0},
    {"postCursor25GOptical",
        PROP_UINT, FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_25G, 1, NULL, NULL, 0},
};

/* Property starting with api.platform.config.switch.%d.phy.%d */
static fm_utilPropMap platConfigSwPhy[] = {
    {"model", PROP_TEXT, FM_TLV_PLAT_SW_PHY_MODEL, 1,
        EnDecodeByMap, phyModelMap, FM_NENTRIES(phyModelMap)},
    {"addr", PROP_INT_H, FM_TLV_PLAT_SW_PHY_ADDR, 2, NULL, NULL, 0},
    {"hwResourceId", PROP_INT_H, FM_TLV_PLAT_SW_PHY_HW_ID, 4, NULL, NULL, 0},
};


/* Property starting with api.platform.config.switch.%d.phy.%d.lane.%d */
static fm_utilPropMap platConfigSwPhyLane[] = {
    {"txEqualizer", PROP_TEXT, FM_TLV_PLAT_SW_PHY_LANE_TXEQ, 7,
        EnDecodePhyTxEqualizer, NULL, 0},
    {"appMode", PROP_UINT, FM_TLV_PLAT_SW_PHY_APP_MODE, 1, NULL, NULL, 0},
};



/* Property starting with api.platform.lib.config. */
static fm_utilPropMap platLibConfig[] = {
    {"pcaIo.count", PROP_UINT, FM_TLV_PLAT_LIB_IO_COUNT, 1, NULL, NULL, 0},
    {"pcaMux.count", PROP_UINT, FM_TLV_PLAT_LIB_MUX_COUNT, 1, NULL, NULL, 0},
    {"xcvrState.default.modAbs.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_MODABS_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.rxLos.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_RXLOS_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.txDisable.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_TXDISABLE_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.txFault.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_TXFAULT_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.modPrsL.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_MODPRESL_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.intL.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_INTL_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.lpMode.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_LPMODE_PIN, 1, NULL, NULL, 0},
    {"xcvrState.default.resetL.pin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_DEF_RESETL_PIN, 1, NULL, NULL, 0},
    {"hwResourceId.count",
        PROP_UINT, FM_TLV_PLAT_LIB_HWRESID_COUNT, 1, NULL, NULL, 0},
    {"xcvrI2C.default.busSelType",
        PROP_TEXT, FM_TLV_PLAT_LIB_XCVR_I2C_DEF_BUSSELTYPE, 1, NULL, NULL, 0},
    {"debug", PROP_TEXT, FM_TLV_PLAT_LIB_DEBUG, 4,
        EnDecodeByBitMap, libDebugMap, FM_NENTRIES(libDebugMap)},
};

/* Property starting with api.platform.lib.config.pcaMux.%d */
static fm_utilPropMap platLibConfigPcaMux[] = {
    {"model", PROP_TEXT, FM_TLV_PLAT_LIB_MUX_MODEL, 1,
        EnDecodeByMap, pcaMuxModelMap, FM_NENTRIES(pcaMuxModelMap)},
    {"bus", PROP_UINT, FM_TLV_PLAT_LIB_MUX_BUS, 1, NULL, NULL, 0},
    {"addr", PROP_UINT_H, FM_TLV_PLAT_LIB_MUX_ADDR, 2, NULL, NULL, 0},
    {"parent.index", PROP_UINT, FM_TLV_PLAT_LIB_MUX_PARENT_IDX, 1, NULL, NULL, 0},
    {"parent.value", PROP_INT_H, FM_TLV_PLAT_LIB_MUX_PARENT_VAL, 4, NULL, NULL, 0},
};

/* Property starting with api.platform.lib.config.pcaIo.%d */
static fm_utilPropMap platLibConfigPcaIo[] = {
    {"model", PROP_TEXT, FM_TLV_PLAT_LIB_IO_MODEL, 1,
        EnDecodeByMap, pcaIoModelMap, FM_NENTRIES(pcaIoModelMap)},
    {"bus", PROP_UINT, FM_TLV_PLAT_LIB_IO_BUS, 1, NULL, NULL, 0},
    {"addr", PROP_UINT_H, FM_TLV_PLAT_LIB_IO_ADDR, 2, NULL, NULL, 0},
    {"parent.index", PROP_UINT, FM_TLV_PLAT_LIB_IO_PARENT_IDX, 1, NULL, NULL, 0},
    {"parent.value", PROP_INT_H, FM_TLV_PLAT_LIB_IO_PARENT_VAL, 4, NULL, NULL, 0},
    {"ledBlinkPeriod", PROP_UINT, FM_TLV_PLAT_LIB_IO_LED_BLINK_PER, 2, NULL, NULL, 0},
    {"ledBrightness", PROP_UINT, FM_TLV_PLAT_LIB_IO_LED_BRIGHTNESS, 1, NULL, NULL, 0},
};


/* Property starting with api.platform.lib.config.hwResourceId.%d */
static fm_utilPropMap platLibConfigHwResId[] = {
    {"interfaceType", PROP_TEXT, FM_TLV_PLAT_LIB_HWRESID_INTF_TYPE, 1,
        EnDecodeByMap, libIntfTypeMap, FM_NENTRIES(libIntfTypeMap)},
    {"xcvrState.pcaIo.index",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_IDX, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.basePin",
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_BASE_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.modAbs.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_MODABS_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.rxLos.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_RXLOS_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.txDisable.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_TXDISABLE_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.txFault.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_TXFAULT_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.modPresL.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_MODPRESL_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.intL.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_INTL_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.lpMode.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_LPMODE_PIN, 1, NULL, NULL, 0},
    {"xcvrState.pcaIo.resetL.pin", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_RESETL_PIN, 1, NULL, NULL, 0},
    {"xcvrI2C.busSelType", 
        PROP_TEXT, FM_TLV_PLAT_LIB_XCVR_I2_BUSSELTYPE, 1,
        EnDecodeByMap, busSelTypeMap, FM_NENTRIES(busSelTypeMap)},
    {"xcvrI2C.pcaMux.index", 
        PROP_UINT, FM_TLV_PLAT_LIB_XCVR_MUX_IDX, 1, NULL, NULL, 0},
    {"xcvrI2C.pcaMux.value", 
        PROP_UINT_H, FM_TLV_PLAT_LIB_XCVR_MUX_VAL, 4, NULL, NULL, 0},
    {"type", 
        PROP_TEXT, FM_TLV_PLAT_LIB_HWRESID_TYPE, 1,
        EnDecodeByMap, hwResTypeMap, FM_NENTRIES(hwResTypeMap)},
    {"phy.busSelType", 
        PROP_TEXT, FM_TLV_PLAT_LIB_PHY_BUSSELTYPE, 1,
        EnDecodeByMap, busSelTypeMap, FM_NENTRIES(busSelTypeMap)},
    {"phy.pcaMux.index", 
        PROP_UINT, FM_TLV_PLAT_LIB_PHY_MUX_IDX, 1, NULL, NULL, 0},
    {"phy.pcaMux.value", 
        PROP_INT_H, FM_TLV_PLAT_LIB_PHY_MUX_VAL, 4, NULL, NULL, 0},
    {"phy.bus", 
        PROP_UINT, FM_TLV_PLAT_LIB_PHY_BUS, 1, NULL, NULL, 0},
    {"vrm.busSelType",
        PROP_TEXT, FM_TLV_PLAT_LIB_VRM_BUSSELTYPE, 1,
        EnDecodeByMap, busSelTypeMap, FM_NENTRIES(busSelTypeMap)},
    {"vrm.model",
        PROP_TEXT, FM_TLV_PLAT_LIB_VRM_MODEL, 1,
         EnDecodeByMap, vrmModelMap, FM_NENTRIES(vrmModelMap)},
    {"vrm.pcaMux.index",
        PROP_UINT, FM_TLV_PLAT_LIB_VRM_MUX_IDX, 1, NULL, NULL, 0},
    {"vrm.pcaMux.value",
        PROP_UINT_H, FM_TLV_PLAT_LIB_VRM_MUX_VAL, 4, NULL, NULL, 0},
    {"vrm.addr",
        PROP_UINT_H, FM_TLV_PLAT_LIB_VRM_MUX_ADDR, 2, NULL, NULL, 0},
    {"vrm.bus",
        PROP_UINT, FM_TLV_PLAT_LIB_VRM_MUX_BUS, 1, NULL, NULL, 0},
};


/* Property starting with api.platform.lib.config.hwResourceId.%d.portLed.%d */
static fm_utilPropMap platLibConfigHwResIdPortLed[] = {
    {"type", PROP_TEXT, FM_TLV_PLAT_LIB_PORTLED_TYPE, 1,
        EnDecodeByMap, portLedTypeMap, FM_NENTRIES(portLedTypeMap)},
    {"pcaIo.index", PROP_UINT, FM_TLV_PLAT_LIB_PORTLED_IO_IDX, 1, NULL, NULL, 0},
    {"pcaIo.pin", PROP_UINT, FM_TLV_PLAT_LIB_PORTLED_IO_PIN, 1, NULL, NULL, 0},
    {"pcaIo.usage", PROP_TEXT, FM_TLV_PLAT_LIB_PORTLED_IO_USAGE, 4,
        EnDecodeByBitMap, portLedUsageMap, FM_NENTRIES(portLedUsageMap)},
};

/* Property starting with api.platform.lib.config.hwResourceId.%d.portLed.%d.%d */
static fm_utilPropMap platLibConfigHwResIdPortLedLane[] = {
    {"pcaIo.pin", PROP_UINT, FM_TLV_PLAT_LIB_PORTLED_IO_LANE_PIN, 1, NULL, NULL, 0},
    {"pcaIo.usage", PROP_TEXT, FM_TLV_PLAT_LIB_PORTLED_IO_LANE_USAGE, 4,
        EnDecodeByBitMap, portLedUsageMap, FM_NENTRIES(portLedUsageMap)},
};


/* Property starting with api.platform.lib.config.bus%d */
static fm_utilPropMap platLibConfigBus[] = {
    {"i2cWrRdEn", PROP_UINT, FM_TLV_PLAT_LIB_BUS_I2C_EN_WR_RD, 1, NULL, NULL, 0},
    {"i2cDevName", PROP_TEXT, FM_TLV_PLAT_LIB_I2C_DEVNAME, 0, NULL, NULL, 0},
};



/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/* GetMapStr
 * \ingroup intPlatform
 *
 * \desc            Get string equivalent of the specified value given
 *                  the string mapping.
 *
 * \param[in]       value is the value on which to operate.
 *
 * \param[in]       strMap points to an array of fm_utilStrMap, where the
 *                  mapping of a string representation of a specific value.
 *
 * \param[in]       size is the size of the strMap array.
 *
 * \param[out]      strBuf is the caller allocated buffer where the function
 *                  will place the output text equivalent.
 *
 * \param[in]       strBufSize is the size of strBuf.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetMapStr(fm_int             value,
                           fm_utilStrMap     *strMap,
                           fm_int             size,
                           fm_text            strBuf,
                           fm_int             strBufSize)
{
    fm_int         cnt;
    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value == strMap[cnt].value)
        {
            FM_SPRINTF_S(strBuf,
                         strBufSize,
                         "%s",
                         strMap[cnt].desc,
                         value);
            return FM_OK;
        }
    }

    FM_SNPRINTF_S(strBuf, strBufSize, "UNKNOWN(%d)", value);

    return FM_OK;

}   /* end GetMapStr */




/*****************************************************************************/
/* GetMapValue
 * \ingroup intPlatform
 *
 * \desc            Get integer equivalent of the string given the string
 *                  mapping.
 *
 * \param[in]       name is the name to find in the string mapping.
 *
 * \param[in]       strMap points to an array of fm_utilStrMap, where the
 *                  mapping of a string representation of a specific value.
 *
 * \param[in]       size is the size of the strMap array.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetMapValue(fm_text         name,
                             fm_utilStrMap  *strMap,
                             fm_int          size,
                             fm_int *        value)
{
    fm_int  cnt;
    fm_int  lenName;
    fm_int  lenDesc;
    fm_text errStr;
    fm_char tempStr[64];

    /* Alow numeric value */
    if (isdigit(name[0]))
    {
        if ( (size > 2) && (name[0] == '0') && (name[1] == 'x') )
        {
            *value = strtol(name + 2, &errStr, 16);
        }
        else
        {
            *value = strtol(name, &errStr, 10);
        }

        /* Input is a number */
        if (strlen(errStr) == 0)
        {
            return FM_OK;
        }
    }

    lenName = strlen(name);
    if (lenName >= (fm_int)sizeof(tempStr))
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Name '%s' is too long\n",
                     name);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    /* Remove trailing spaces or quotation*/
    for (cnt = 0; cnt < lenName; cnt++)
    {
        if (isascii(name[cnt]) && 
            !(isspace(name[cnt]) || name[cnt] == '"'))
        {
            tempStr[cnt] = name[cnt];
        }
        else
        {
            break;
        }
    }
    tempStr[cnt] = '\0';
    lenName = cnt;


    for (cnt = 0 ; cnt < size ; cnt++)
    {
        lenDesc = strlen(strMap[cnt].desc);
        if ( (lenName == lenDesc) &&
             ( (strncasecmp( tempStr, strMap[cnt].desc, lenDesc)) == 0 ) )
        {
            *value = strMap[cnt].value;
            return FM_OK;
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end GetMapValue */



/*****************************************************************************/
/* GetBitMapStr
 * \ingroup intPlatform
 *
 * \desc            Get string equivalent of the specified bit mask value
 *                  given the string mapping.
 *
 * \param[in]       value is the value on which to operate.
 *
 * \param[in]       strMap points to an array of fm_platformStrMap, specifying
 *                  a set of strings and their corresponding integer values.
 *
 * \param[in]       size is the size of the strMap array.
 *
 * \param[out]      strBuf is the caller allocated buffer where the function
 *                  will place the output text equivalent.
 *
 * \param[in]       strBufSize is the size of strBuf.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetBitMapStr(fm_int             value,
                              fm_utilStrMap     *strMap,
                              fm_int             size,
                              fm_text            strBuf,
                              fm_int             strBufSize)
{
    fm_int         cnt;

    /* Empty string if no bit is set */
    strBuf[0] = '\0';

    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value & strMap[cnt].value)
        {
            if (strBuf[0] != '\0')
            {
                FM_STRCAT_S(strBuf, strBufSize, ",");
            }

            FM_STRCAT_S(strBuf, strBufSize, strMap[cnt].desc);
        }
    }

    return FM_OK;

}   /*  end GetBitMapStr */




/*****************************************************************************/
/* GetBitMapValue
 * \ingroup intPlatform
 *
 * \desc            Get bit mask value given a string mapping.
 *
 * \param[in]       name is the attribute name to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \param[in]       defVal is the default value to return if the attribute
 *                  is not found.
 *
 * \param[in]       strMap points to an array of fm_platformStrMap, where the
 *                  mapping of a string representation of a specific value.
 *
 * \param[in]       size is the size of the strMap array.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetBitMapValue(fm_text            name,
                                fm_utilStrMap     *strMap,
                                fm_int             size,
                                fm_int *           value)
{
    fm_int   valBit;
    fm_int   i;
    fm_char *token;
    fm_char *tokptr;
    fm_uint  strSize;
    fm_char  tmpText[512+1];
    fm_int   strLen;

    *value = 0;

    strLen = strlen(name);
    if (strLen >= (fm_int)sizeof(tmpText))
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Name '%s' is too long\n",
                     name);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    /* Remove trailing spaces or quotation*/
    for (i = 0; i < strLen; i++)
    {
        if (isascii(name[i]) && 
            !(isspace(name[i]) || name[i] == '"'))
        {
            tmpText[i] = name[i];
        }
        else
        {
            break;
        }
    }
    strLen = i;
    tmpText[strLen] = '\0';

    /* Comma delimited values */
    strSize = sizeof(tmpText) - 1;
    token   = FM_STRTOK_S(tmpText, &strSize, ", ", &tokptr);

    if (token == NULL)
    {
        return FM_OK;
    }

    if (GetMapValue(token, strMap, size, &valBit) == FM_OK)
    {
        *value |= valBit;
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Invalid value '%s' for '%s'\n", 
                     token,
                     name);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < size ; i++)
    {
        token = FM_STRTOK_S(NULL, &strSize, ", ", &tokptr);

        if (token == NULL)
        {
            break;
        }

        if (GetMapValue(token, strMap, size, &valBit) == FM_OK)
        {
            *value |= valBit;
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Invalid value '%s' for '%s'\n", 
                         token,
                         name);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }
    }

    return FM_OK;

}   /*  end GetBitMapValue */





/*****************************************************************************/
/* GetStringValue
 * \ingroup intPlatform
 *
 * \desc            Get integer equivalent of the string given the string
 *                  mapping.
 *
 * \param[in]       name is the name to find in the string mapping.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetStringValue(fm_text name, fm_int *value)
{
    fm_uint cnt;
    fm_uint size;
    fm_text errStr;
    fm_char tempStr[32];

    /* Alow numeric value or -1 */
    if (isdigit(name[0]) || (name[0] == '-'))
    {
        size = strlen(name);
        if (size >= sizeof(tempStr))
        {
            return FM_ERR_INVALID_ARGUMENT;
        }

        /* Remove trailing spaces or quotation*/
        for (cnt = 0; cnt < size; cnt++)
        {
            if (isascii(name[cnt]) && 
                !(isspace(name[cnt]) || name[cnt] == '"'))
            {
                tempStr[cnt] = name[cnt];
            }
            else
            {
                break;
            }
        }
        tempStr[cnt] = '\0';

        if ( (size > 2) && (tempStr[0] == '0') && (tempStr[1] == 'x') )
        {
            *value = strtol(tempStr + 2, &errStr, 16);
        }
        else
        {
            *value = strtol(tempStr, &errStr, 10);
        }

        /* Input is a number */
        if (strlen(errStr) == 0)
        {
            return FM_OK;
        }
    }
    if (strncasecmp(name, "true", 4) == 0)
    {
        *value = 1;
        return FM_OK;
    }
    if (strncasecmp(name, "false", 5) == 0)
    {
        *value = 0;
        return FM_OK;
    }

    return FM_ERR_NOT_FOUND;

}   /* end GetStringValue */



/*****************************************************************************/
/* GetStringValueU64
 * \ingroup intPlatform
 *
 * \desc            Get unsigned 64-bit integer equivalent of the string given
 *                  the string mapping.
 *
 * \param[in]       name is the name to find in the string mapping.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetStringValueU64(fm_text            name,
                                   fm_uint64 *        value)
{
    fm_uint cnt;
    fm_uint size;
    fm_text errStr;
    fm_char tempStr[32];

    /* Alow numeric value */
    if (isdigit(name[0]))
    {
        size = strlen(name);
        if (size >= sizeof(tempStr))
        {
            return FM_ERR_INVALID_ARGUMENT;
        }

        /* Remove trailing spaces or quotation*/
        for (cnt = 0; cnt < size; cnt++)
        {
            if (isascii(name[cnt]) && 
                !(isspace(name[cnt]) || name[cnt] == '"'))
            {
                tempStr[cnt] = name[cnt];
            }
            else
            {
                break;
            }
        }
        tempStr[cnt] = '\0';

        if ( (size > 2) && (tempStr[0] == '0') && (tempStr[1] == 'x') )
        {
            *value = strtoull(tempStr + 2, &errStr, 16);
        }
        else
        {
            *value = strtoull(tempStr, &errStr, 10);
        }

        /* Input is a number */
        if (strlen(errStr) == 0)
        {
            return FM_OK;
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end GetStringValueU64 */



/*****************************************************************************/
/* GetTlvValue
 * \ingroup intPlatform
 *
 * \desc            Get up to 32-bit integer from TLV bytes.
 *
 * \param[in]       tlv is an array of bytes.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetTlvValue(fm_byte *tlv,
                             fm_int   tlvLen,
                             fm_int *  value)
{
    fm_int j;

    if (tlvLen > 4)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *value = tlv[0];
    for (j = 1; j < tlvLen; j++)
    {
        *value <<= 8;
        *value  |= (tlv[j] & 0xFF);
    }

    return FM_OK;

}   /* end GetTlvValue */



/*****************************************************************************/
/* GetTlvValueU64
 * \ingroup intPlatform
 *
 * \desc            Get up to 64-bit unsigned integer from TLV bytes.
 *
 * \param[in]       tlv is an array of bytes.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetTlvValueU64(fm_byte *  tlv,
                                fm_int     tlvLen,
                                fm_uint64 *value)
{
    fm_int j;

    if (tlvLen > 8)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *value = tlv[0];
    for (j = 1; j < tlvLen; j++)
    {
        *value <<= 8;
        *value  |= (tlv[j] & 0xFFUL);
    }

    return FM_OK;

}   /* end GetTlvValueU64 */



/*****************************************************************************/
/* SetTlvValue
 * \ingroup intPlatform
 *
 * \desc            Set up to 32-bit integer to TLV bytes.
 *
 * \param[in]       value is hte 64-bit unsigned integer to set.
 *
 * \param[in]       tlv is an array of bytes where the function will place
 *                  the integer value.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetTlvValue(fm_int   value,
                             fm_byte *tlv,
                             fm_int   tlvLen)
                             
{
    fm_int j;

    if (tlvLen > 4)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    for (j = 0; j < tlvLen; j++)
    {
        tlv[tlvLen -1 - j] = (value >> (j * 8)) & 0xFF;
    }

    return FM_OK;

}   /* end SetTlvValue */



/*****************************************************************************/
/* SetTlvValueU64
 * \ingroup intPlatform
 *
 * \desc            Set up to 64-bit unsigned integer to TLV bytes.
 *
 * \param[in]       value is hte 64-bit unsigned integer to set.
 *
 * \param[in]       tlv is an array of bytes where the function will place
 *                  the integer value.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetTlvValueU64(fm_uint64  value,
                                fm_byte *  tlv,
                                fm_int     tlvLen)
                             
{
    fm_int j;

    if (tlvLen > 8)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    for (j = 0; j < tlvLen; j++)
    {
        tlv[tlvLen -1 - j] = (value >> (j * 8)) & 0xFF;
    }

    return FM_OK;

}   /* end SetTlvValueU64 */



/*****************************************************************************/
/** ParseProperty
 * \ingroup intPlatform
 *
 * \desc            Parse text property into tokens delimited by integer
 *                  argument.
 *
 * \param[in]       propText is the property string to parse.
 *
 * \param[out]      propNameTok is an array of string pointer where the
 *                  function will place the pointer to the start of the
 *                  text string after each integer argument. This assumes
 *                  the size of this array is at least MAX_PROP_LEN.
 *
 * \param[out]      propArgsList is an array of integer where the function
 *                  will place the argument value. This assumes the size
 *                  of this array is at least MAX_PROP_LEN.
 *
 * \param[out]      propArgsSize is the caller allocated memory where the
 *                  function will place the number of argument found.
 *
 * \param[out]      propType is the caller allocated memory where the
 *                  function will place the pointer to the property type.
 *
 * \param[out]      propVal is the caller allocated memory where the
 *                  function will place the pointer to the property value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status ParseProperty(fm_text  propText,
                               fm_text *propNameTok,
                               fm_int  *propArgsList,
                               fm_int  *propArgsSize,
                               fm_text *propType,
                               fm_text *propVal)
{
    fm_int  i;
    fm_text startArg;
    fm_int  argVal;
    fm_uint argLen;
    fm_char tempStr[16];

    *propArgsSize              = 0;
    propNameTok[*propArgsSize] = propText;
    *propType                   = NULL;
    *propVal                    = NULL;
    startArg                    = NULL;

    for (i = 1; i < MAX_PROP_LEN; i++)
    {
        if (!isascii(propText[i]))
        {
            /* Error: value token not found yet but got non ASCII characters  */
            return FM_ERR_INVALID_ARGUMENT;
        }
        if (isspace(propText[i-1]) && !isspace(propText[i]))
        {
            if (*propType == NULL)
            {
                /* Found the first type token */
                *propType = &propText[i];
            }
            else
            {
                /* Found the value token */
                *propVal = &propText[i];
                return FM_OK;
            }
        }
        /* Still parsing prop name, find arguments in prop name */
        if (*propType == NULL && isdigit(propText[i]))
        {
            if (propText[i-1] == '.')
            {
                startArg = &propText[i];
            }
            /* bus argument is bus0, instead of bus.0. */
            if ((i > 3) &&
                (propText[i-1] == 's') &&
                (propText[i-2] == 'u') && 
                (propText[i-3] == 'b'))
            {
                startArg = &propText[i];
            }
        }
        else if (startArg && !isdigit(propText[i]))
        {
            argLen = (&propText[i] - startArg);
            if (propText[i] == '.' && argLen < sizeof(tempStr))
            {
                FM_MEMCPY_S(tempStr, sizeof(tempStr), startArg, argLen);
                tempStr[argLen] = '\0';
                /* Found a argument within property */
                if (GetStringValue(tempStr, &argVal) == FM_OK)
                {
                    propArgsList[*propArgsSize] = argVal;
                    /* First index is the start of the property */
                    propNameTok[(*propArgsSize)+1] = &propText[i+1];
                    (*propArgsSize)++;
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unable to convert value [%s]\n", tempStr); 
                }
            }
            startArg = NULL;
        }

    }

    return FM_ERR_INVALID_ARGUMENT;

} /* end ParseProperty */



/*****************************************************************************/
/** EncodeGenericConfig
 * \ingroup intPlatform
 *
 * \desc            Encode platform config text property into corresponding TLV.
 *
 * \param[in]       propMap is an array of fm_utilPropMap structure describing the
 *                  property to be encoded.
 *
 * \param[in]       propMapLen is the length of the propMap array.
 *
 * \param[in]       propNameTok is an array of string pointer to the start of
 *                  the text string after each integer argument.
 *
 * \param[in]       propArgsList is an array of integers of argument value.
 *
 * \param[in]       propArgsSize is the size of propArgsList as well as propNameTok.
 *
 * \param[in]       propType is the pointer to the property type.
 *
 * \param[in]       propVal is the pointer to the property value.
 *
 * \param[out]      tlv points to an array of bytes, where the function
 *                  will store the encoded TLV.
 *
 * \param[in]       length is the size of the tlv array.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EncodeGenericConfig(fm_utilPropMap *propMap,
                                     fm_int      propMapLen,
                                     fm_text    *propNameTok,
                                     fm_int     *propArgsList,
                                     fm_int      propArgsSize,
                                     fm_text     propType,
                                     fm_text     propVal,
                                     fm_byte    *tlv,
                                     fm_int      length)
{
    fm_status   status;
    fm_int      i;
    fm_int      j;
    fm_int      keyLen;
    fm_int      valLen;
    fm_int      valOff;
    fm_int      val;
    fm_int      maxVal;
    fm_uint64   valU64;

    for (i = 0; i < propMapLen; i++)
    {
        valOff = propArgsSize; /* Number of byte arguments */
        valLen = 0;
        keyLen = strlen(propMap[i].key);
        if (propNameTok[propArgsSize] == NULL)
        {
            return FM_ERR_INVALID_ARGUMENT;
        }

        if (strncmp(propMap[i].key, propNameTok[propArgsSize], keyLen)  == 0)
        {
            switch (propMap[i].propType)
            {
                case PROP_TEXT:
                    valLen = strlen(propVal);
                    if (propMap[i].enDecFunc)
                    {
                        status = propMap[i].enDecFunc(ENCODE,
                                                      &propMap[i],
                                                      tlv + TLV_VALUE_OFF + valOff,
                                                      length - TLV_VALUE_OFF - valOff,
                                                      propVal,
                                                      valLen,
                                                      &valLen);
                        if (status)
                        {
                            return status;
                        }
                    }
                    else
                    {
                        /* Remove trailing special characters */
                        for (j = valLen - 1 ; j > 0 ; j--)
                        {
                            if (isprint(propVal[j]) && !(propVal[j] == '"'))
                            {
                                break;
                            }
                        }
                        valLen = j + 1;

                        if ((valLen <= 0) || (valLen >= TLV_MAX_LEN))
                        {
                            return FM_ERR_INVALID_ARGUMENT;
                        }
                        FM_MEMCPY_S(tlv + TLV_VALUE_OFF + valOff,
                                    length - TLV_VALUE_OFF - valOff,
                                    propVal,
                                    valLen);
                    }
                    break;
                case PROP_BOOL:
                case PROP_INT:
                case PROP_INT_H:
                case PROP_UINT:
                case PROP_UINT_H:
                    status = GetStringValue(propVal, &val);
                    if (status)
                    {
                        return status;
                    }

                    valLen = propMap[i].valLen;
                    for (j = 0; j < valLen; j++)
                    {
                        tlv[TLV_VALUE_OFF + valOff + j] = 
                            (val >> ((valLen - j - 1) * 8)) & 0xFF;
                    }
                    /* Check for value exceed the size of storage */
                    if (valLen < 4)
                    {
                        if (propMap[i].propType == PROP_UINT ||
                            propMap[i].propType == PROP_UINT_H)
                        {
                            /* Unsigned encoding */
                            maxVal = (1 << (8*valLen)) - 1;
                            if (val < 0 || val > maxVal)
                            {
                                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                    "%s: Value %d overflows for unsigned integer "
                                    "size of %d byte. Valid range (0,%d)\n",
                                    propMap[i].key, val, valLen, maxVal);
                            }
                        }
                        else
                        {
                            /* Signed encoding */
                            maxVal = (1 << (8*valLen - 1)) - 1;
                            if (val > maxVal || val < -maxVal)
                            {
                                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                    "%s: Value %d overflows for integer size of "
                                    "%d byte. Valid range (-%d,%d)\n",
                                    propMap[i].key, val, valLen, maxVal, maxVal);
                            }
                        }
                    }
                    break;
                case PROP_UINT64:
                    status = GetStringValueU64(propVal, &valU64);
                    if (status)
                    {
                        return status;
                    }

                    valLen = propMap[i].valLen;
                    for (j = 0; j < valLen; j++)
                    {
                        tlv[TLV_VALUE_OFF + valOff + j] = 
                            (valU64 >> (j * 8)) & 0xFF;
                    }
                    break;
            }

            /* Add arguments before value */
            switch (propArgsSize)
            {
                case 3:
                    tlv[TLV_VALUE_OFF]   = propArgsList[0] & 0xFF;
                    tlv[TLV_VALUE_OFF+1] = propArgsList[1] & 0xFF;
                    tlv[TLV_VALUE_OFF+2] = propArgsList[2] & 0xFF;
                    break;
                case 2:
                    tlv[TLV_VALUE_OFF]   = propArgsList[0] & 0xFF;
                    tlv[TLV_VALUE_OFF+1] = propArgsList[1] & 0xFF;
                    break;
                case 1:
                    tlv[TLV_VALUE_OFF] = propArgsList[0] & 0xFF;
                    break;
                case 0:
                    /* Nothing */
                    break;
                default:
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unsupported %d arguments \n", propArgsSize);
                    return FM_ERR_INVALID_ARGUMENT;
            }

            TLV_SET_TYPE(tlv, propMap[i].tlvId);
            /* One byte per argument */
            TLV_SET_LEN(tlv, valLen + propArgsSize);
            return FM_OK;
        }
    }


    return FM_ERR_NOT_FOUND;

}   /* end EncodeGenericConfig */



/*****************************************************************************/
/** FindTlvMap
 * \ingroup intPlatform
 *
 * \desc            Find TLV index to the structure given the value if found.
 *
 * \param[in]       propMap is an array of fm_utilPropMap structure describing the
 *                  properties.
 *
 * \param[in]       propMapLen is the length of the propMap array.
 *
 * \param[in]       tlvType is TLV type to match.
 *
 * \return          pointer to coresponding fm_utilPropMap if found.
 * \return          NULL if not found.
 *
 *****************************************************************************/
static fm_utilPropMap * FindTlvMap(fm_utilPropMap *propMap,
                               fm_int      propMapLen,
                               fm_uint     tlvType)
{
    fm_int      i;

    for (i = 0; i < propMapLen; i++)
    {
        if (propMap[i].tlvId == tlvType)
        {
            return &propMap[i];
        }
    }


    return NULL;

}   /* end FindTlvMap */



/*****************************************************************************/
/** PrintPropTypeValue
 * \ingroup intPlatform
 *
 * \desc            Convert TLV value into property text string.
 *
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure describing the
 *                  property.
 *
 * \param[in]       tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValLen is the length of the tlv array.
 *
 * \param[out]      strBuf is the caller allocated buffer where the function
 *                  will write the decoded property text type and value.
 *
 * \param[in]       bufSize is the size of strBuf.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_int PrintPropTypeValue(fm_utilPropMap *propMap,
                                 fm_byte *tlvVal,
                                 fm_int tlvValLen,
                                 fm_text strBuf,
                                 fm_int bufSize)
{
    fm_int valInt;
    fm_int len;
    fm_int cnt;

    switch (propMap->propType)
    {
        case PROP_BOOL:
            return FM_SNPRINTF_S(strBuf, bufSize, " bool %s", tlvVal[0] ? "true" : "false");
        case PROP_INT:
        case PROP_INT_H:
        case PROP_UINT:
        case PROP_UINT_H:
            if (propMap->valLen != tlvValLen)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Property value length is %d but got %d\n",
                    propMap->valLen, tlvValLen);
            }
            if (tlvValLen > 4)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Invalid integer tlv value length %d\n",
                    tlvValLen);
                tlvValLen = 4;
            }
            GetTlvValue(tlvVal, tlvValLen, &valInt);
            return FM_SNPRINTF_S(strBuf,
                                 bufSize,
                                 (propMap->propType==PROP_INT ||
                                  propMap->propType==PROP_UINT) ?
                                     " int %d" : " int 0x%x",
                                 valInt);
        default:
            if (propMap->enDecFunc)
            {
                propMap->enDecFunc(DECODE,
                                   propMap,
                                   tlvVal,
                                   tlvValLen,
                                   strBuf,
                                   bufSize,
                                   &cnt);
            }
            else
            {
                len = tlvValLen;
                if (len > bufSize)
                {
                    len = bufSize;
                }

                /* including fm_text type */
                len = len + 7;
                cnt = FM_SNPRINTF_S(strBuf, len, " text %s", tlvVal);
                if (cnt > len)
                {
                    cnt = len;
                }
                strBuf[cnt] = '\0';
            }
            return cnt;
    }
} /* end PrintPropTypeValue */



/*****************************************************************************/
/** EnDecodeDevMemOff
 * \ingroup intPlatform
 *
 * \desc            Encode/Decode devMemOffset property.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array.
 *
 * \param[in/out]   propTxt is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       propTxtSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodeDevMemOff(fm_utilEnDecode endec,
                                   void    *propMap,
                                   fm_byte *tlvVal,
                                   fm_int   tlvValSize,
                                   fm_char *propTxt,
                                   fm_int   propTxtSize,
                                   fm_int  *len)
{
    fm_status   status;
    fm_uint64   valU64;

    if (endec == ENCODE)
    {
        status = GetStringValueU64(propTxt, &valU64);
        if (status)
        {
            return status;
        }
        SetTlvValueU64(valU64, tlvVal, 8);

        if (len)
        {
            *len = 8;
        }
    }
    else
    {
        if (tlvValSize != 8)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Expect size of TLV value is 8 but got %d\n",
                tlvValSize);
        }
        GetTlvValueU64(tlvVal, 8, &valU64);
        
        FM_SNPRINTF_S(propTxt, propTxtSize, " text 0x%llx", valU64);
        if (len)
        {
            *len = strlen(propTxt);
        }
    }

    return FM_OK;

} /* end EnDecodeDevMemOff */



/*****************************************************************************/
/** EnDecodePortMapping
 * \ingroup intPlatform
 *
 * \desc            Encode/Decode portMapping property.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array.
 *
 * \param[in/out]   valText is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       valTextSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodePortMapping(fm_utilEnDecode endec,
                                     void    *propMap,
                                     fm_byte *tlvVal,
                                     fm_int   tlvValSize,
                                     fm_char *valText,
                                     fm_int   valTextSize,
                                     fm_int  *len)
{
    fm_status   status;
    fm_text     pTxt;
    fm_text     pTxt2;
    fm_int      port;
    fm_int      phy;
    fm_int      phyPort;
    fm_int      swPort;
    fm_int      lane;
    fm_int      val;
    fm_int      valLen;
    fm_int      off;
    fm_bool     isSwag;

    if (endec == ENCODE)
    {
        isSwag = FALSE;
        if ( (pTxt = strstr(valText, "PCIE=")) != NULL )
        {
            tlvVal[0] = 0;
            off       = 5;
            valLen    = 4;
        }
        else if ( (pTxt = strstr(valText, "EPL=")) != NULL )
        {
            if ( (pTxt2 = strstr(valText, "PHY=")) != NULL )
            {
                status = GetStringValue(pTxt2 + 4, &val);
                if (status)
                {
                    return status;
                }
                SetTlvValue(val, tlvVal + 4, 1);
                if ( (pTxt2 = strstr(valText, "PORT=")) != NULL )
                {
                    status = GetStringValue(pTxt2 + 5, &val);
                    if (status)
                    {
                        return status;
                    }
                    SetTlvValue(val, tlvVal + 5, 1);
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unable to find PHY port number key word in [%s]\n",
                        valText);
                    return FM_ERR_INVALID_ARGUMENT;
                }
                if ( (pTxt2 = strstr(valText, "LANE=")) != NULL )
                {
                    status = GetStringValue(pTxt2 + 5, &lane);
                    if (status)
                    {
                        return status;
                    }
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unable to find LANE port number key word in [%s]\n",
                        valText);
                    return FM_ERR_INVALID_ARGUMENT;
                }
                tlvVal[0] = 6 | (lane << 4);
                off       = 4;
                valLen    = 6;
            }
            else
            {
                if ( (pTxt2 = strstr(valText, "LANE=")) != NULL )
                {
                    status = GetStringValue(pTxt2 + 5, &lane);
                    if (status)
                    {
                        return status;
                    }
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unable to find LANE port number key word in [%s]\n",
                        valText);
                    return FM_ERR_INVALID_ARGUMENT;
                }
                tlvVal[0] = 1 | (lane << 4);
                off       = 4;
                valLen    = 4;
            }
        }
        else if ( (pTxt = strstr(valText, "LPBK=")) != NULL )
        {
            tlvVal[0] = 2;
            off       = 5;
            valLen    = 4;
        }
        else if ( (pTxt = strstr(valText, "TE=")) != NULL )
        {
            tlvVal[0] = 3;
            off       = 3;
            valLen    = 4;
        }
        else if ( (pTxt = strstr(valText, "FIBM=")) != NULL )
        {
            tlvVal[0] = 4;
            off       = 5;
            valLen    = 4;
        }
        else if ( (pTxt = strstr(valText, "SWAG=")) != NULL )
        {
            isSwag    = TRUE;
            tlvVal[0] = 5;
            off       = 3;
            valLen    = 6;
            if ( (pTxt2 = strstr(valText, "LOG=")) != NULL )
            {
                status = GetStringValue(pTxt2 + 4, &port);
                if (status)
                {
                    return status;
                }
                SetTlvValue(port, tlvVal + 4, 2);
            }
            /* offset at 3 is for SW= and not the same for other types */
            pTxt = strstr(valText, "SW=");
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find PORT type key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        status = GetStringValue(pTxt + off, &val);
        if (status)
        {
            return status;
        }
        tlvVal[3] = val;

        if ( (pTxt = strstr(valText, isSwag ? "SWAG=" : "LOG=")) != NULL )
        {
            status = GetStringValue(pTxt + (isSwag ? 5 : 4), &port);
            if (status)
            {
                return status;
            }
            SetTlvValue(port, tlvVal + 1, 2);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find LOG key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }


        if (len)
        {
            *len = valLen;
        }
    }
    else
    {
        GetTlvValue(tlvVal + 3, 1, &val);
        GetTlvValue(tlvVal + 1, 2, &port);
        switch (tlvVal[0] & 0xF)
        {
            case 0:
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d PCIE=%d\"",
                              port, val);
                break;
            case 1:
                lane = (tlvVal[0] >> 4) & 0xF;
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d EPL=%d LANE=%d\"",
                              port, val, (tlvVal[0] >> 4) & 0xF);
                break;
            case 2:
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d LPBK=%d\"",
                              port, val);
                break;
            case 3:
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d TE=%d\"",
                              port, val);
                break;
            case 4:
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d FIBM=%d\"",
                              port, val);
                break;
            case 5:
                GetTlvValue(tlvVal + 4, 2, &swPort);
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"SWAG=%d SW=%d LOG=%d\"",
                              port, val, swPort);
                break;
            case 6:
                GetTlvValue(tlvVal + 4, 1, &phy);
                GetTlvValue(tlvVal + 5, 1, &phyPort);
                lane = (tlvVal[0] >> 4) & 0xF;
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d EPL=%d LANE=%d PHY=%d PORT=%d\"",
                              port, val, lane, phy, phyPort);
                break;
            default:
                FM_SNPRINTF_S(valText, valTextSize,
                              " text \"LOG=%d UNKNOWN=%d\"",
                              port, val);
                break;
        }

        if (len)
        {
            *len = strlen(valText);
        }
    }

    return FM_OK;

} /* end EnDecodePortMapping */



/*****************************************************************************/
/** EnDecodeIntPortMapping
 * \ingroup intPlatform
 *
 * \desc            Encode/Decode internalPortMapping property.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array.
 *
 * \param[in/out]   valText is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       valTextSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodeIntPortMapping(fm_utilEnDecode endec,
                                        void    *propMap,
                                        fm_byte *tlvVal,
                                        fm_int   tlvValSize,
                                        fm_char *valText,
                                        fm_int   valTextSize,
                                        fm_int  *len)
{
    fm_status   status;
    fm_text     pTxt;
    fm_int      val1;
    fm_int      val2;

    if (endec == ENCODE)
    {
        if ((pTxt = strstr(valText, "SWAG=")) != NULL )
        {
            status = GetStringValue(pTxt + 5, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 0, 2);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find first SWAG key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        /* Find next SWAG keyword */
        if ((pTxt = strstr(pTxt + 5, "SWAG=")) != NULL )
        {
            status = GetStringValue(pTxt + 5, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 2, 2);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find second SWAG key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }


        if (len)
        {
            *len = 4;
        }
    }
    else
    {
        GetTlvValue(tlvVal + 0, 2, &val1);
        GetTlvValue(tlvVal + 2, 2, &val2);
        FM_SNPRINTF_S(valText, valTextSize,
                      " text \"SWAG=%d SWAG=%d\"",
                      val1, val2);

        if (len)
        {
            *len = strlen(valText);
        }
    }

    return FM_OK;

} /* end EnDecodeIntPortMapping */





/*****************************************************************************/
/** EnDecodePhyTxEqualizer
 * \ingroup intPlatform
 *
 * \desc            Encode/Decode PHY Tx Equalizer property.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array.
 *
 * \param[in/out]   valText is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       valTextSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodePhyTxEqualizer(fm_utilEnDecode endec,
                                        void    *propMap,
                                        fm_byte *tlvVal,
                                        fm_int   tlvValSize,
                                        fm_char *valText,
                                        fm_int   valTextSize,
                                        fm_int  *len)
{
    fm_status   status;
    fm_text     pTxt;
    fm_int      val1;
    fm_int      val2;
    fm_int      val3;
    fm_int      val4;

    if (endec == ENCODE)
    {
        if ((pTxt = strstr(valText, "PRE=")) != NULL )
        {
            status = GetStringValue(pTxt + 4, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 0, 1);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find PRE= key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        if ((pTxt = strstr(valText, "ATT=")) != NULL )
        {
            status = GetStringValue(pTxt + 4, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 1, 1);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find ATT= key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        if ((pTxt = strstr(valText, "POST=")) != NULL )
        {
            status = GetStringValue(pTxt + 5, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 2, 1);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find POST= key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        if ((pTxt = strstr(valText, "POL=")) != NULL )
        {
            status = GetStringValue(pTxt + 4, &val1);
            if (status)
            {
                return status;
            }
            SetTlvValue(val1, tlvVal + 3, 1);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to find POL= key word in [%s]\n",
                valText);
            return FM_ERR_INVALID_ARGUMENT;
        }

        if (len)
        {
            *len = 4;
        }
    }
    else
    {
        GetTlvValue(tlvVal + 0, 1, &val1);
        GetTlvValue(tlvVal + 1, 1, &val2);
        GetTlvValue(tlvVal + 2, 1, &val3);
        GetTlvValue(tlvVal + 3, 1, &val4);
        FM_SNPRINTF_S(valText, valTextSize,
                      " text \"PRE=%d ATT=%d POST=%d POL=%d\"",
                      val1, val2, val3, val4);

        if (len)
        {
            *len = strlen(valText);
        }
    }

    return FM_OK;

} /* end EnDecodePhyTxEqualizer */




/*****************************************************************************/
/** EnDecodeByMap
 * \ingroup intPlatform
 *
 * \desc            Generic encode/decode property using mapping structure.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array. For decoding, this
 *                  is also the size of the TLV expected to return.
 *
 * \param[in/out]   propTxt is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       propTxtSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodeByMap(fm_utilEnDecode endec,
                               void           *propMapV,
                               fm_byte        *tlvVal,
                               fm_int          tlvValSize,
                               fm_char        *propTxt,
                               fm_int          propTxtSize,
                               fm_int         *len)
{
    fm_status   status;
    fm_int      value;
    fm_utilPropMap *propMap = (fm_utilPropMap*)propMapV;

    if (propMap->strMap == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (endec == ENCODE)
    {
        status = GetMapValue(propTxt,
                             propMap->strMap,
                             propMap->strMapSize,
                             &value);
        if (status)
        {
            return status;
        }

        if (propMap->valLen == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Property value TLV is incorrectly at 0. Set to 1.\n");
            propMap->valLen = 1;
        }
        if (propMap->valLen > tlvValSize)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Property value TLV is %d but TLV size of only %d\n",
                propMap->valLen,
                tlvValSize);
            return FM_ERR_INVALID_ARGUMENT;
        }
        SetTlvValue(value, tlvVal, propMap->valLen);

        if (len)
        {
            *len = propMap->valLen;
        }
    }
    else
    {
        GetTlvValue(tlvVal, tlvValSize, &value);
        FM_SNPRINTF_S(propTxt, propTxtSize, " text ", "");
        status = GetMapStr(value,
                           propMap->strMap,
                           propMap->strMapSize,
                           propTxt + 6,
                           propTxtSize - 6);
        if (status)
        {
            return status;
        }
        if (len)
        {
            *len = strlen(propTxt);
        }
    }

    return FM_OK;

} /* end EnDecodeByMap */



/*****************************************************************************/
/** EnDecodeByBitMap
 * \ingroup intPlatform
 *
 * \desc            Generic encode/decode property using mapping structure.
 *
 *
 * \param[in]       endec specifies whether to encode or decode.
 *
 * \param[in]       propMap is the pointer to fm_utilPropMap structure
 *                  describing the property to be encoded/decoded.
 *
 * \param[in/out]   tlvVal points to an array of bytes, storing the property value.
 *
 * \param[in]       tlvValSize is the size of the tlv array. For decoding, this
 *                  is also the size of the TLV expected to return.
 *
 * \param[in/out]   propTxt is the caller allocated buffer storing the property
 *                  text type and value.
 *
 * \param[in]       propTxtSize is the size of propTxt.
 *
 * \param[in]       len is the caller allocated memory where the function
 *                  will place the length of output tlvVal, if encoding, or
 *                  the length of output propTxt, if decoding. len can
 *                  be NULL if the return value is not required.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnDecodeByBitMap(fm_utilEnDecode endec,
                                  void           *propMapV,
                                  fm_byte        *tlvVal,
                                  fm_int          tlvValSize,
                                  fm_char        *propTxt,
                                  fm_int          propTxtSize,
                                  fm_int         *len)
{
    fm_status   status;
    fm_int      value;
    fm_utilPropMap *propMap = (fm_utilPropMap*)propMapV;

    if (propMap->strMap == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (endec == ENCODE)
    {
        status = GetBitMapValue(propTxt,
                                propMap->strMap,
                                propMap->strMapSize,
                                &value);
        if (status)
        {
            return status;
        }

        if (propMap->valLen == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Property value TLV is incorrectly at 0. Set to 1.\n");
            propMap->valLen = 1;
        }
        if (propMap->valLen > tlvValSize)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Property value TLV is %d but TLV size of only %d\n",
                propMap->valLen,
                tlvValSize);
            return FM_ERR_INVALID_ARGUMENT;
        }
        SetTlvValue(value, tlvVal, propMap->valLen);

        if (len)
        {
            *len = propMap->valLen;
        }
    }
    else
    {
        GetTlvValue(tlvVal, tlvValSize, &value);
        FM_SNPRINTF_S(propTxt, propTxtSize, " text ", "");
        status = GetBitMapStr(value,
                              propMap->strMap,
                              propMap->strMapSize,
                              propTxt + 6,
                              propTxtSize - 6);
        if (status)
        {
            return status;
        }
        if (len)
        {
            *len = strlen(propTxt);
        }
    }

    return FM_OK;

} /* end EnDecodeByBitMap */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

    

/*****************************************************************************/
/** fmUtilConfigPropertyEncodeTlv
 * \ingroup intPlatform
 *
 * \desc            Encode text property into corresponding TLV.
 *
 * \param[in]       property is the property to encode.
 *
 * \param[out]      tlv points to an array of bytes, where the function
 *                  will store the encoded TLV.
 *
 * \param[in]       length is the length of the tlv array.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilConfigPropertyEncodeTlv(fm_text property,
                                        fm_byte *tlv,
                                        fm_int length)
{
    fm_status   status;
    fm_text     propNameTok[MAX_PROP_TOK_SIZE];
    fm_int      propArgsList[MAX_PROP_TOK_SIZE];
    fm_int      propArgsSize;
    fm_text     propType;
    fm_text     propVal;



    TLV_SET_TYPE(tlv, 0xFFFF);
    TLV_SET_LEN(tlv, 0);

    status = ParseProperty(property,
                           propNameTok,
                           propArgsList,
                           &propArgsSize,
                           &propType,
                           &propVal);
    if (status)
    {
        return status;
    }

    status = FM_ERR_NOT_FOUND;

    /* NOTE: check for more specific pattern first */
    if ((strncasecmp(property,
                     PLAT_CFG_SW_PREFIX, 
                     sizeof(PLAT_CFG_SW_PREFIX) -1 ) == 0) &&
        (propArgsSize > 0) )
    {
        if (propArgsSize == 1)
        {
            /* Properties with only switch argument */
            status = EncodeGenericConfig(platConfigSw,
                                         sizeof(platConfigSw)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 2) &&
                (strncasecmp(propNameTok[1], "portIndex", 8) == 0))
        {
            /* Properties with switch and portIndex */
            status = EncodeGenericConfig(platConfigSwPortIdx,
                                         sizeof(platConfigSwPortIdx)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 2) &&
                (strncasecmp(propNameTok[1], "internalPortIndex", 17) == 0))
        {
            /* Properties with switch and internalPortIndex */
            status = EncodeGenericConfig(platConfigSwIntPortIdx,
                                         sizeof(platConfigSwIntPortIdx)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 2) &&
                (strncasecmp(propNameTok[1], "phy", 3) == 0))
        {
            /* Properties with switch and phy */
            status = EncodeGenericConfig(platConfigSwPhy,
                                         sizeof(platConfigSwPhy)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 3) &&
                (strncasecmp(propNameTok[1], "portIndex", 8) == 0) &&
                (strncasecmp(propNameTok[2], "lane", 4) == 0))
        {
            /* Properties with switch and portIndex and lane */
            status = EncodeGenericConfig(platConfigSwPortIdxLane,
                                         sizeof(platConfigSwPortIdxLane)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 3) &&
                (strncasecmp(propNameTok[1], "phy", 3) == 0) &&
                (strncasecmp(propNameTok[2], "lane", 4) == 0))
        {
            /* Properties with switch and portIndex and lane */
            status = EncodeGenericConfig(platConfigSwPhyLane,
                                         sizeof(platConfigSwPhyLane)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
    }
    else if ((propArgsSize > 0) &&
             (strncasecmp(property,
                     PLAT_LIB_CFG_BUS_PREFIX, 
                     sizeof(PLAT_LIB_CFG_BUS_PREFIX) -1 ) == 0))
    {
        if (propArgsSize == 1)
        {
            /* Properties with only bus argument */
            status = EncodeGenericConfig(platLibConfigBus,
                                         sizeof(platLibConfigBus)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
    }
    else if ((propArgsSize > 0) &&
             (strncasecmp(property,
                     PLAT_LIB_CFG_PCAMUX_PREFIX, 
                     sizeof(PLAT_LIB_CFG_PCAMUX_PREFIX) -1 ) == 0))
    {
        if (propArgsSize == 1)
        {
            /* Properties with only pcaMux argument */
            status = EncodeGenericConfig(platLibConfigPcaMux,
                                         sizeof(platLibConfigPcaMux)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
    }
    else if ((propArgsSize > 0) &&
             (strncasecmp(property,
                     PLAT_LIB_CFG_PCAIO_PREFIX, 
                     sizeof(PLAT_LIB_CFG_PCAIO_PREFIX) -1 ) == 0))
    {
        if (propArgsSize == 1)
        {
            /* Properties with only pcaIo argument */
            status = EncodeGenericConfig(platLibConfigPcaIo,
                                         sizeof(platLibConfigPcaIo)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
    }
    else if ((propArgsSize > 0) &&
             (strncasecmp(property,
                     PLAT_LIB_CFG_HWRESID_PREFIX, 
                     sizeof(PLAT_LIB_CFG_HWRESID_PREFIX) -1 ) == 0))
    {
        if (propArgsSize == 1)
        {
            /* Properties with only hwResourceId argument */
            status = EncodeGenericConfig(platLibConfigHwResId,
                                         sizeof(platLibConfigHwResId)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 2) &&
                (strncasecmp(propNameTok[1], "portLed", 7) == 0))
        {
            /* Properties with hwResourceId and portLed argument */
            status = EncodeGenericConfig(platLibConfigHwResIdPortLed,
                                         sizeof(platLibConfigHwResIdPortLed)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
        else if ((propArgsSize == 3) &&
                (strncasecmp(propNameTok[1], "portLed", 7) == 0))
        {
            /* Properties with hwResourceId, portLed and lane arguments */
            status = EncodeGenericConfig(platLibConfigHwResIdPortLedLane,
                                         sizeof(platLibConfigHwResIdPortLedLane)/
                                         sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
        }
    }
    else if ((propArgsSize == 0) &&
             (strncasecmp(property,
                          PLAT_LIB_CFG_PREFIX, 
                          sizeof(PLAT_LIB_CFG_PREFIX) -1 ) == 0))
    {
            /* Properties with no argument, start after prefix */
            propNameTok[0] = property + sizeof(PLAT_LIB_CFG_PREFIX) - 1;
            status = EncodeGenericConfig(platLibConfig,
                                         sizeof(platLibConfig)/sizeof(fm_utilPropMap),
                                         propNameTok,
                                         propArgsList,
                                         propArgsSize,
                                         propType,
                                         propVal,
                                         tlv,
                                         length);
    }
    else if ((propArgsSize == 0) &&
             (strncasecmp(property,
                          PLAT_CFG_PREFIX, 
                          sizeof(PLAT_CFG_PREFIX)-1) == 0))
    {
        /* Properties with no argument, start after prefix */
        propNameTok[0] = property + sizeof(PLAT_CFG_PREFIX) - 1;
        status = EncodeGenericConfig(platConfig,
                                     sizeof(platConfig)/sizeof(fm_utilPropMap),
                                     propNameTok,
                                     propArgsList,
                                     propArgsSize,
                                     propType,
                                     propVal,
                                     tlv,
                                     length);
    }
    else if ((propArgsSize == 0) &&
             ((strncasecmp(property, FM10K_CFG_PREFIX, sizeof(FM10K_CFG_PREFIX)-1) == 0)) )
    {
        /* FM10K properties */
        propNameTok[0] =  property + sizeof(FM10K_CFG_PREFIX) - 1;
        status = EncodeGenericConfig(fm10kProp,
                                     sizeof(fm10kProp)/sizeof(fm_utilPropMap),
                                     propNameTok,
                                     propArgsList,
                                     propArgsSize,
                                     propType,
                                     propVal,
                                     tlv,
                                     length);
    }
    else if ((propArgsSize == 0) &&
             ((strncasecmp(property, API_CFG_PREFIX, sizeof(API_CFG_PREFIX)-1) == 0) || 
              (strncasecmp(property, DEBUG_CFG_PREFIX, sizeof(DEBUG_CFG_PREFIX)-1) == 0)) )
    {
        /* API properties, start from first character */
        propNameTok[0] = property;
        status = EncodeGenericConfig(apiProp,
                                     sizeof(apiProp)/sizeof(fm_utilPropMap),
                                     propNameTok,
                                     propArgsList,
                                     propArgsSize,
                                     propType,
                                     propVal,
                                     tlv,
                                     length);
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "Unhandled property: [%s]\n", property);
        return FM_ERR_INVALID_ARGUMENT;
    }

    return status;

}   /* end fmUtilConfigPropertyEncodeTlv */




/*****************************************************************************/
/** fmUtilConfigPropertyDecodeTlv
 * \ingroup intPlatform
 *
 * \desc            Encode TLV property into corresponding text.
 *
 * \param[out]      tlv points to an array of bytes of single encode TLV.
 *,
 * \param[in]       propBuf is the caller allocated buffer where the function
 *                  will write the corresponding text property.
 *
 * \param[in]       bufSize is the size of propBuf.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmUtilConfigPropertyDecodeTlv(fm_byte *tlv,
                                        fm_text propBuf,
                                        fm_int bufSize)
{
    fm_status           status;
    fm_uint             tlvType;
    fm_uint             tlvLen;
    fm_utilPropMap          *propMap;
    fm_int              bufLen;

    status = FM_ERR_NOT_FOUND;

    tlvType = (tlv[0] << 8) | tlv[1];
    tlvLen = tlv[2];


    if (tlvType < 0x2000)
    {
        /* API properties */
        propMap = FindTlvMap(apiProp,
                             sizeof(apiProp)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s",
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 3, tlvLen, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }
    }
    else if (tlvType < 0x3000)
    {
        /* FM10K properties */
        propMap = FindTlvMap(fm10kProp,
                             sizeof(fm10kProp)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%s",
                        FM10K_CFG_PREFIX,
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 3, tlvLen, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }
    }
    else if (tlvType < 0x5000)
    {
        /* Platform LT config */

        /* Properties with no argument, start after prefix */
        propMap = FindTlvMap(platConfig,
                             sizeof(platConfig)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%s",
                        PLAT_CFG_PREFIX,
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 3, tlvLen, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch */
        propMap = FindTlvMap(platConfigSw,
                             sizeof(platConfigSw)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.%s",
                        PLAT_CFG_SW_PREFIX,
                        tlv[3], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 4, tlvLen - 1, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch and portIndex */
        propMap = FindTlvMap(platConfigSwPortIdx,
                             sizeof(platConfigSwPortIdx)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.portIndex.%d.%s",
                        PLAT_CFG_SW_PREFIX, 
                        tlv[3], 
                        tlv[4], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 5, tlvLen - 2, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch and internalPortIndex */
        propMap = FindTlvMap(platConfigSwIntPortIdx,
                             sizeof(platConfigSwIntPortIdx)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.internalPortIndex.%d.%s",
                        PLAT_CFG_SW_PREFIX, 
                        tlv[3], 
                        tlv[4], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 5, tlvLen - 2, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch and phy */
        propMap = FindTlvMap(platConfigSwPhy,
                             sizeof(platConfigSwPhy)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.phy.%d.%s",
                        PLAT_CFG_SW_PREFIX, 
                        tlv[3], 
                        tlv[4], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 5, tlvLen - 2, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch and portIndex and lane */
        propMap = FindTlvMap(platConfigSwPortIdxLane,
                             sizeof(platConfigSwPortIdxLane)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.portIndex.%d.lane.%d.%s",
                        PLAT_CFG_SW_PREFIX, 
                        tlv[3], 
                        tlv[4], 
                        tlv[5], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 6, tlvLen - 3, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with switch and portIndex and lane */
        propMap = FindTlvMap(platConfigSwPhyLane,
                             sizeof(platConfigSwPhyLane)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.phy.%d.lane.%d.%s",
                        PLAT_CFG_SW_PREFIX, 
                        tlv[3], 
                        tlv[4], 
                        tlv[5], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 6, tlvLen - 3, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }
    }
    else if (tlvType < 0x6000)
    {
        /* Platform LT lib config */

        /* Properties with only bus argument */
        propMap = FindTlvMap(platLibConfigBus,
                             sizeof(platLibConfigBus)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.%s ",
                        PLAT_LIB_CFG_BUS_PREFIX,
                        tlv[3], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 4, tlvLen - 1, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with only pcaMux argument */
        propMap = FindTlvMap(platLibConfigPcaMux,
                             sizeof(platLibConfigPcaMux)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.%s",
                        PLAT_LIB_CFG_PCAMUX_PREFIX,
                        tlv[3], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 4, tlvLen - 1, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with only pcaIo argument */
        propMap = FindTlvMap(platLibConfigPcaIo,
                             sizeof(platLibConfigPcaIo)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.%s",
                        PLAT_LIB_CFG_PCAIO_PREFIX,
                        tlv[3], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 4, tlvLen - 1, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with only hwResourceId argument */
        propMap = FindTlvMap(platLibConfigHwResId,
                             sizeof(platLibConfigHwResId)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.%s",
                        PLAT_LIB_CFG_HWRESID_PREFIX,
                        tlv[3], 
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 4, tlvLen - 1, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with hwResourceId and portLed argument */
        propMap = FindTlvMap(platLibConfigHwResIdPortLed,
                             sizeof(platLibConfigHwResIdPortLed)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.portLed.%d.%s",
                        PLAT_LIB_CFG_HWRESID_PREFIX,
                        tlv[3],
                        tlv[4],
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 5, tlvLen - 2, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with hwResourceId, portLed and lane arguments */
        propMap = FindTlvMap(platLibConfigHwResIdPortLedLane,
                             sizeof(platLibConfigHwResIdPortLedLane)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%d.portLed.%d.%d.%s",
                        PLAT_LIB_CFG_HWRESID_PREFIX,
                        tlv[3],
                        tlv[4],
                        tlv[5],
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 6, tlvLen - 3, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }

        /* Properties with no argument, start after prefix */
        propMap = FindTlvMap(platLibConfig,
                             sizeof(platLibConfig)/
                             sizeof(fm_utilPropMap),
                             tlvType);
        if (propMap)
        {
            bufLen = FM_SNPRINTF_S(propBuf, bufSize, "%s%s",
                        PLAT_LIB_CFG_PREFIX,
                        propMap->key);
            PrintPropTypeValue(propMap, tlv + 3, tlvLen, propBuf + bufLen, bufSize - bufLen);
            return FM_OK;
        }
    }
    else
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    return status;

}   /* end fmUtilConfigPropertyDecodeTlv */

