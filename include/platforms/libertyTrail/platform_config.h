/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_config.h
 * Creation Date:   June 2, 2014
 * Description:     Platform functions to handle configurations.
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

#ifndef __FM_PLATFORM_CONFIG_H
#define __FM_PLATFORM_CONFIG_H

/*#include <net/if.h>*/
#include <api/internal/fm10000/fm10000_api_hw_int.h>
#include <platforms/util/fm_util.h>
#include <platforms/util/retimer/fm_util_gn2412.h>

#define FM_PLAT_UNDEFINED               -1
#define FM_ETHMODE_AUTODETECT           0x12345678

#define FM_PLAT_NUM_EPL                 FM10000_NUM_EPLS
#define FM_PLAT_LANES_PER_EPL           FM10000_PORTS_PER_EPL
#define FM_PLAT_NUM_PEP                 FM10000_NUM_PEPS
#define FM_PLAT_NUM_TUNNEL              2
#define FM_PLAT_NUM_LOOPBACK            2
#define FM_PLAT_NUM_GPIO                16

/* Maximum logical port number assigned to a physical port. */ 
#define FM_PLAT_MAX_LOGICAL_PORT        1024

/* Limit the number of PHY to the number EPL ports */
#define FM_PLAT_MAX_NUM_PHYS            (FM_PLAT_NUM_EPL * FM_PLAT_LANES_PER_EPL)

/* Limit the number of ports per PHY to 16 */
#define FM_PLAT_PORT_PER_PHY            16

#define CFG_DBG_CONFIG                  (1<<0)
#define CFG_DBG_MOD_STATE               (1<<1)
#define CFG_DBG_MOD_INTR                (1<<2)
#define CFG_DBG_MOD_TYPE                (1<<3)
#define CFG_DBG_MOD_LED                 (1<<4)

/* Special flags to enable various logging in the API */
#define CFG_DBG_ENABLE_PLAT_LOG         (1<<5)
#define CFG_DBG_ENABLE_PORT_LOG         (1<<6)
#define CFG_DBG_ENABLE_INTR_LOG         (1<<7)


#define CONFIG_DEBUG(fmt, ...)                          \
    if (fmRootPlatform->cfg.debug & CFG_DBG_CONFIG)     \
    {                                                   \
        FM_LOG_PRINT(fmt, ##__VA_ARGS__);               \
    }
#define MOD_STATE_DEBUG(fmt, ...)                       \
    if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_STATE)  \
    {                                                   \
        FM_LOG_PRINT(fmt, ##__VA_ARGS__);               \
    }
#define MOD_TYPE_DEBUG(fmt, ...)                        \
    if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_TYPE)   \
    {                                                   \
        FM_LOG_PRINT(fmt, ##__VA_ARGS__);               \
    }
#define MOD_INTR_DEBUG(fmt, ...)                        \
    if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_INTR)   \
    {                                                   \
        FM_LOG_PRINT(fmt, ##__VA_ARGS__);               \
    }
#define MOD_LED_DEBUG(fmt, ...)                         \
    if (fmRootPlatform->cfg.debug & CFG_DBG_MOD_LED)    \
    {                                                   \
        FM_LOG_PRINT(fmt, ##__VA_ARGS__);               \
    }

/* Flags for fm_platformCfgLib.disableFuncIntf */
#define FM_PLAT_DISABLE_NONE                     (1 << 0)
#define FM_PLAT_DISABLE_RESET_SWITCH_FUNC        (1 << 1)
#define FM_PLAT_DISABLE_I2C_FUNC                 (1 << 2)
#define FM_PLAT_DISABLE_DEBUG_FUNC               (1 << 3)
#define FM_PLAT_DISABLE_INIT_SW_FUNC             (1 << 4)
#define FM_PLAT_DISABLE_SEL_BUS_FUNC             (1 << 5)
#define FM_PLAT_DISABLE_GET_PORT_XCVR_STATE_FUNC (1 << 6)
#define FM_PLAT_DISABLE_SET_PORT_XCVR_STATE_FUNC (1 << 7)
#define FM_PLAT_DISABLE_SET_PORT_LED_FUNC        (1 << 8)
#define FM_PLAT_DISABLE_ENABLE_PORT_INTR_FUNC    (1 << 9)
#define FM_PLAT_DISABLE_GET_PORT_INTR_FUNC       (1 << 10)
#define FM_PLAT_DISABLE_POST_INIT_FUNC           (1 << 11)
#define FM_PLAT_DISABLE_SET_VRM_VOLTAGE_FUNC     (1 << 12)
#define FM_PLAT_DISABLE_GET_VRM_VOLTAGE_FUNC     (1 << 13)

/* Maximum number of voltage regulator module. */ 
#define FM_PLAT_MAX_VRM        3


typedef enum 
{
    FM_PLAT_PORT_TYPE_NONE,
    FM_PLAT_PORT_TYPE_EPL,
    FM_PLAT_PORT_TYPE_PCIE,
    FM_PLAT_PORT_TYPE_TUNNEL,
    FM_PLAT_PORT_TYPE_LOOPBACK,
    FM_PLAT_PORT_TYPE_FIBM,

} fm_platPortType;

typedef enum 
{
    FM_PLAT_INTF_TYPE_NONE       = 0,
    FM_PLAT_INTF_TYPE_SFPP       = 1,
    FM_PLAT_INTF_TYPE_QSFP_LANE0 = 2,     /* Primary QSFP port */
    FM_PLAT_INTF_TYPE_QSFP_LANE1 = 3,
    FM_PLAT_INTF_TYPE_QSFP_LANE2 = 4,
    FM_PLAT_INTF_TYPE_QSFP_LANE3 = 5,
    FM_PLAT_INTF_TYPE_PCIE       = 6,

} fm_platIntfType;

typedef enum 
{
    FM_PLAT_BOOT_MODE_SPI = 0,  /* Boot from SPI flash (EEPROM) */
    FM_PLAT_BOOT_MODE_EBI = 1,  /* Boot from EBI interface */
    FM_PLAT_BOOT_MODE_I2C = 2,  /* Boot from I2C interface */

} fm_platBootMode;

typedef enum 
{
    FM_PLAT_REG_ACCESS_PCIE = 0, /* Switch managed from PCIe interface */
    FM_PLAT_REG_ACCESS_EBI  = 1, /* Switch managed from EBI interface */
    FM_PLAT_REG_ACCESS_I2C  = 2, /* Switch managed from I2C interface */

} fm_platRegAccessMode;

typedef enum 
{
    FM_PLAT_PCIE_ISR_AUTO = 0,   /* PCIe ISR mode based on boot mode */
    FM_PLAT_PCIE_ISR_SW   = 1,   /* PCIe ISR done in software */
    FM_PLAT_PCIE_ISR_SPI  = 2,   /* PCIe ISR done in SPI flash */

} fm_platPcieIsrMode;

typedef enum 
{
    FM_PLAT_SERDES_BITRATE_1G,  /* 1.25Gbps bit rate */
    FM_PLAT_SERDES_BITRATE_10G, /* 10.3125Gbs bit rate */
    FM_PLAT_SERDES_BITRATE_25G, /* 25.78125Gbps bit rate */
    FM_PLAT_SERDES_BITRATE_MAX

} fm_platSerdesBitRate;

typedef enum 
{
    FM_LED_BLINK_MODE_NO_BLINK    = 0,
    FM_LED_BLINK_MODE_SW_CONTROL  = 1,
    FM_LED_BLINK_MODE_HW_ASSISTED = 2,

} fm_platLedBlinkMode;


typedef enum
{
    FM_PLAT_PHY_UNKNOWN = 0,
    FM_PLAT_PHY_GN2412  = 1,     /* Semtech KR retimer */

} fm_platPhyModel;


#define FM_PLAT_MAX_CFG_STR_LEN 32

#define FM_PLAT_NUM_SW                  fmRootPlatform->cfg.numSwitches
#define FM_PLAT_GET_CFG                 (&fmRootPlatform->cfg)
#define FM_PLAT_GET_SWITCH_CFG(sw)      (&fmRootPlatform->cfg.switches[sw])
#define FM_PLAT_GET_PHY_CFG(sw,phy)     (&fmRootPlatform->cfg.switches[sw].phys[phy])
#define FM_PLAT_NUM_PHY(sw)             fmRootPlatform->cfg.switches[sw].numPhys
#define FM_PLAT_NUM_PORT(sw)            fmRootPlatform->cfg.switches[sw].numPorts
#define FM_PLAT_GET_PORT_CFG(sw, port)  (&fmRootPlatform->cfg.switches[sw].ports[port])
#define FM_PLAT_GET_LIBS_CFG(sw)        (&fmRootPlatform->cfg.switches[sw].libs)

#define FM_PLAT_GET_LANE_CFG(sw, pCfg, laneNum)                                  \
    (&FM_PLAT_GET_SWITCH_CFG(sw)->epls[pCfg->epl].lane[pCfg->lane[laneNum]])

/* Port capabilities. See constPortCapabilities */
#define FM_PLAT_PORT_CAP_LAG_CAPABLE    (1 << 0)
#define FM_PLAT_PORT_IS_LAG_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_LAG_CAPABLE)
#define FM_PLAT_PORT_CAP_CAN_ROUTE      (1 << 1)
#define FM_PLAT_PORT_IS_ROUTE_CAP(pCfg) ((pCfg)->cap & FM_PLAT_PORT_CAP_CAN_ROUTE)
#define FM_PLAT_PORT_CAP_SPEED_10M      (1 << 2)
#define FM_PLAT_PORT_IS_10M_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_10M)
#define FM_PLAT_PORT_CAP_SPEED_100M     (1 << 3)
#define FM_PLAT_PORT_IS_100M_CAP(pCfg)  ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_100M)
#define FM_PLAT_PORT_CAP_SPEED_1G       (1 << 4)
#define FM_PLAT_PORT_IS_1G_CAP(pCfg)    ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_1G)
#define FM_PLAT_PORT_CAP_SPEED_2PT5G    (1 << 5)
#define FM_PLAT_PORT_IS_2PT5G_CAP(pCfg) ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_2PT5G)
#define FM_PLAT_PORT_CAP_SPEED_5G       (1 << 6)
#define FM_PLAT_PORT_IS_5G_CAP(pCfg)    ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_5G)
#define FM_PLAT_PORT_CAP_SPEED_10G      (1 << 7)
#define FM_PLAT_PORT_IS_10G_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_10G)
#define FM_PLAT_PORT_CAP_SPEED_20G      (1 << 8)
#define FM_PLAT_PORT_IS_20G_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_20G)
#define FM_PLAT_PORT_CAP_SPEED_25G      (1 << 9)
#define FM_PLAT_PORT_IS_25G_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_25G)
#define FM_PLAT_PORT_CAP_SPEED_40G      (1 << 10)
#define FM_PLAT_PORT_IS_40G_CAP(pCfg)   ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_40G)
#define FM_PLAT_PORT_CAP_SPEED_100G     (1 << 11)
#define FM_PLAT_PORT_IS_100G_CAP(pCfg)  ((pCfg)->cap & FM_PLAT_PORT_CAP_SPEED_100G)
#define FM_PLAT_PORT_CAP_SW_LED         (1 << 12)        /* SW driven LED */
#define FM_PLAT_PORT_IS_SW_LED(pCfg)    ((pCfg)->cap & FM_PLAT_PORT_CAP_SW_LED)

/* AN73 abilities.
   IMPORTANT: The bit position already represent the proper bit position
              in the AN73 base page register_*/
#define FM_PLAT_AN73_ABILITY_RF                 (1 << 13)
#define FM_PLAT_AN73_ABILITY_NP                 (1 << 15)
#define FM_PLAT_AN73_ABILITY_1000BASE_KX        (1 << 21)
#define FM_PLAT_AN73_ABILITY_10GBASE_KR         (1 << 23)
#define FM_PLAT_AN73_ABILITY_40GBASE_KR4        (1 << 24)
#define FM_PLAT_AN73_ABILITY_40GBASE_CR4        (1 << 25)
#define FM_PLAT_AN73_ABILITY_100GBASE_KR4       (1 << 28)
#define FM_PLAT_AN73_ABILITY_100GBASE_CR4       (1 << 29)
#define FM_PLAT_AN73_ABILITY_25GGBASE_CR_KR     (1 << 31) /* CR or KR */

/* Bits to indicate if the serdes value has been set. */
#define FM_STATE_SERDES_IN_PROGRESS     0x1
#define FM_STATE_SERDES_TX_CURSOR       0x2
#define FM_STATE_SERDES_TX_PRECURSOR    0x4
#define FM_STATE_SERDES_TX_POSTCURSOR   0x8

typedef struct
{
    /* PHY/retimer model */
    fm_platPhyModel model;

    /* I2C address of the device */
    fm_uint addr;

    /* Unique ID for the platform to identify the HW resource for this PHY */
    fm_uint32 hwResourceId;

    /* GN2412 lane configuration */
    fm_gn2412LaneCfg gn2412Lane[FM_GN2412_NUM_LANES];

} fm_platformCfgPhy;

typedef struct
{
    /* SERDES preCursor value */
    fm_int preCursor;

    /* SERDES cursor value */
    fm_int cursor;

    /* SERDES postCursor value */
    fm_int postCursor;

} fm_platCfgSerdes;

typedef struct
{
    /* Lane termination */
    fm_int rxTermination;

    /* Lane polarity setting */
    fm_int lanePolarity;

    /* SERDES settings for DA cables */
    fm_platCfgSerdes copper[FM_PLAT_SERDES_BITRATE_MAX];

    /* SERDES settings for optical modules */
    fm_platCfgSerdes optical[FM_PLAT_SERDES_BITRATE_MAX];

    /* SERDES settings set by the application through the API */
    fm_platCfgSerdes appCfg;

    /* bits field indicating whether the x-Cursor have been set by the app. */
    fm_int appCfgState;

} fm_platformCfgLane;


typedef struct
{
    /* Indexed with EPL lane number */
    fm_platformCfgLane lane[FM_PLAT_LANES_PER_EPL];

    /* Port index corresponding to QSFP_LANE0, LANE1, LANE2 and LANE3 */
    fm_int laneToPortIdx[FM_PLAT_LANES_PER_EPL];

} fm_platformCfgEpl;

typedef struct
{
    /* Flags for config loading status */
    fm_uint32           loadFlags;

    /* Port index. Same as the index to the structure */
    fm_int              portIdx;

    /* Unique logical port */
    fm_int              port;

    /* Port type (EPL, PCIE, TUNNEL, LOOPBACK) */
    fm_platPortType     portType;

    /* EPL number associated to this port */
    fm_int              epl;

    /* LANE number associated to this port */
    fm_int              lane[FM_PLAT_LANES_PER_EPL];

    /* PEP number associated to this port */
    fm_int              pep;

    /* TUNNEL number associated to this port */
    fm_int              tunnel;

    /* LOOPBACK number associated to this port */
    fm_int              loopback;

    /* Unique ID for the platform to identify the hardware resource
     * for the associated port */
    fm_uint32           hwResourceId;

    /* Unique physical port */
    fm_int              physPort;

    /* Cable autodetection mode enable/disable */
    fm_bool             autodetect;

    /* startup default ethMode */
    fm_ethMode          ethMode;

    /* Interface type */
    fm_platIntfType     intfType;

    /* Port Speed if cannot be determined from ethernet mode */
    fm_int              speed;

    /* Port capability. See constPortCapabilities */
    fm_uint32           cap;

    /* AN73 Abilities to set in the API */
    fm_uint32           an73Ability;

    /* AN73 Abilities as defined in the LT configuration file */
    fm_uint32           an73AbilityCfg;

    /* DFE mode */
    fm_byte             dfeMode;

    /* Associated PHY number, if any */
    fm_int              phyNum;

    /* Associated PHY port number, if any */
    fm_int              phyPort;

#ifdef FM_SUPPORT_SWAG
    /* Define the SWAG logical port to switch logical port mapping */
    fm_swagLink         swagLink;
#endif

} fm_platformCfgPort;


typedef struct
{
    /* Shared library name */
    fm_char libName[FM_PLAT_MAX_CFG_STR_LEN];

    /* Whether to disable certain function interface */
    fm_uint disableFuncIntf;

    /* Buffer to save shared lib TLVs for shared lib to load later */
    fm_byte *tlvCfgBuf;

    /* Total size of tlvCfg */
    fm_uint tlvCfgBufSize;

    /* Length of valid tlv */
    fm_uint tlvCfgLen;

} fm_platformCfgLib;


typedef struct
{
    /* Unique ID for the platform to identify the hardware resource
     * for the voltage regulator modules */
    fm_int hwResourceId[FM_PLAT_MAX_VRM];

    /* Indicate to program the VRM with the default voltage values when the
       fuse box is not programmed with the voltage scaling values (set to 0). */
    fm_int useDefVoltages;

} fm_platformCfgVrm;


typedef struct
{
    /* Switch index. Same as the index to the structure */
    fm_int                  swIdx;

    /* Switch number specific to hardware. This number will be passed
     * down to the shared library interfaces, not the sw value. */
    fm_int                  swNum;

    /* Number of ports on the switch */
    fm_int                  numPorts;

    /* CPU logical port */
    fm_int                  cpuPort;

    /* Management PEP */
    fm_int                  mgmtPep;

    /* MSI enabled */
    fm_bool                 msiEnabled
;
    /* Frame handler clock */
    fm_int                  fhClock;

    /* Switch boot mode (SPI FLASH, EBI, I2C) */
    fm_platBootMode         bootMode;

    /* Register access mode (PCIe, EBI, I2C) */
    fm_platRegAccessMode    regAccess;

    /* PCIE ISR mode (AUTO, SW, SPI) */
    fm_platPcieIsrMode      pcieISR;

    /* Port configuration on the switch */
    fm_platformCfgPort     *ports;

    /* EPL configuration on the switch */
    fm_platformCfgEpl       epls[FM_PLAT_NUM_EPL];

    /* Optional DLL library configurations */
    fm_platformCfgLib       libs;

    /* LED polling period for SW driven LED */
    fm_int                  ledPollPeriodMsec;

    /* LED blinking mode */
    fm_int                  ledBlinkMode;

    /* Transceiver Mgmt polling period */
    fm_int                  xcvrPollPeriodMsec;

    /* Highest logical port value representing the physical ports */
    fm_int                  maxLogicalPortValue;

    /* The boot configuration used for EBI/I2C boot */
    fm10000_bootCfg         bootCfg;

    /* Interrupt polling period in msec (0: polling disabled) */
    fm_int                  intrPollPeriodMsec;

    /* Interrupt timeout count */
    fm_int                  intrTimeoutCnt;

    /* SerDes preserve their configuration upon ethernet mode change */
    fm_int                  keepSerdesCfg;

#ifdef FM_SUPPORT_SWAG
    /* The switch role when managed by SWAG */
    fm_switchRole           switchRole;
#endif

    /* /dev/mem offset */
    fm_char                 devMemOffset[FM_PLAT_MAX_CFG_STR_LEN];

    /* Network device name */
    fm_char                 netDevName[FM_PLAT_MAX_CFG_STR_LEN];

    /* UIO device name */
    fm_char                 uioDevName[FM_PLAT_MAX_CFG_STR_LEN];

    /* Number of PHYs/retimers connected to the switch */
    fm_int                  numPhys;

    /* PHY configuration */
    fm_platformCfgPhy      *phys;

    /* Voltage regulator modules */
    fm_platformCfgVrm       vrm;

    /* FM10000 GPIO number for SFP/QSFP module interrupt. */
    fm_int                  gpioPortIntr;

    /* FM10000 GPIO number for the I2C reset Control. */
    fm_int                  gpioI2cReset;

    /* FM10000 GPIO number for the write protect pin on the FM10000 Flash. */
    fm_int                  gpioFlashWP;

    /* Indicate to enable the configuration of the PHY's de-emphasis. */
    fm_int                  enablePhyDeEmphasis;

    /* Scheduler list select */
    fm_int                  schedListSelect;

    /* I2C clock divider */
    fm_uint         i2cClkDivider;

} fm_platformCfgSwitch;


typedef struct
{
    /* platform name */
    fm_char                 name[FM_PLAT_MAX_CFG_STR_LEN];

    /* File locking to share resources with other applications */
    fm_char                 fileLockName[FM_PLAT_MAX_CFG_STR_LEN];

    /* Number of switches in the system */
    fm_int                  numSwitches;

    /* Switch configuration 
     * NOTE: This structure is indexed by API sw notion */
    fm_platformCfgSwitch   *switches;

    /* debug control */
    fm_uint                 debug;

#ifdef FM_SUPPORT_SWAG
    /* SWAG topology configured */
    fm_swagTopology         topology;
#endif

    /* EVI device name */
    fm_char                 ebiDevName[FM_PLAT_MAX_CFG_STR_LEN];

} fm_platformCfg;


fm_status fmPlatformCfgInit(void);
fm_status fmPlatformLoadPropertiesFromLine(fm_text line);
void fmPlatformCfgDump(void);
fm_platformCfgSwitch *fmPlatformCfgSwitchGet(fm_int sw);
fm_int fmPlatformCfgPortGetIndex(fm_int sw, fm_int port);
fm_platformCfgPort *fmPlatformCfgPortGet(fm_int sw, fm_int port);
fm_platformCfgLane *fmPlatformCfgLaneGet(fm_int sw, fm_int port, fm_int lane);
fm_text fmPlatformGetEthModeStr(fm_ethMode mode);
fm_schedulerConfigMode fmPlatformGetSchedulerConfigMode(fm_int sw);
fm_int fmPlatformCfgSchedulerGetTokenList(fm_int sw,
                                          fm_schedulerToken *schedTokenList);

#endif /* __FM_PLATFORM_CONFIG_H */
