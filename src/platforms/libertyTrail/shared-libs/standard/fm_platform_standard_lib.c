/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_platform_standard_lib.c
 * Creation Date:   April 2014
 * Description:     Platform library for the standard Liberty Trail platform.
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

#include <fm_sdk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <api/internal/fm10000/fm10000_api_regs_int.h>
#include <platforms/libertyTrail/platform_lib_api.h>
#include <platforms/libertyTrail/platform_app_api.h>
#include <platforms/libertyTrail/platform_attr.h>
#include <platforms/common/lib/i2c/ipmi/fm_platform_ipmi.h>
#include <platforms/common/lib/i2c/smbus/fm_platform_smbus.h>
#include <platforms/util/fm_util_config_tlv.h>
#include <platforms/util/fm_util.h>
#include <platforms/util/fm_util_pca.h>
#include <platforms/util/fm_util_device_lock.h>
#ifdef FM_SUPPORT_FTDI_RRC
#include <platforms/util/ftdi/rrc/fm_util_ftdi_rrc.h>
#endif

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define UNDEF_VAL          -1

#define ATTR_PREFIX        "api.platform.lib.config."

#define FIELD_FORMAT_V     "    %-42s: %d 0x%x\n"
#define FIELD_FORMAT_S     "    %-42s: %s\n"

#define PRINT_VALUE(name, value) \
    FM_LOG_PRINT(FIELD_FORMAT_V, name, value, value)

#define PRINT_STRING(name, string) \
    FM_LOG_PRINT(FIELD_FORMAT_S, name, string)

#define LedTypeToStr(type) \
    GetStrMap(type, ledTypeMap, FM_NENTRIES(ledTypeMap), FALSE, tmpStr, sizeof(tmpStr))

#define LedUsageToStr(usage) \
    GetStrMap(usage, ledUsageMap, FM_NENTRIES(ledUsageMap), FALSE, tmpStr, sizeof(tmpStr))

#define BusSelTypeToStr(type) \
    GetStrMap(type, busSelTypeMap, FM_NENTRIES(busSelTypeMap), FALSE, tmpStr, sizeof(tmpStr))

#define PcaMuxModelToStr(type) \
    GetStrMap(type, pcaMuxModelMap, FM_NENTRIES(pcaMuxModelMap), FALSE, tmpStr, sizeof(tmpStr))

#define PcaIoModelToStr(type) \
    GetStrMap(type, pcaIoModelMap, FM_NENTRIES(pcaIoModelMap), FALSE, tmpStr, sizeof(tmpStr))

#define VrmModelToStr(type) \
    GetStrMap(type, vrmModelMap, FM_NENTRIES(vrmModelMap), FALSE, tmpStr, sizeof(tmpStr))

#define IntfTypeToStr(type) \
    GetStrMap(type, intfTypeMap, FM_NENTRIES(intfTypeMap), FALSE, tmpStr, sizeof(tmpStr))

#define HwResourceToStr(type) \
    GetStrMap(type, hwResourceTypeMap, FM_NENTRIES(hwResourceTypeMap), FALSE, tmpStr, sizeof(tmpStr))

#define SET_BIT(lvalue, bit, bitvalue)    \
    ( lvalue = ( lvalue & ~(1 << bit) ) | \
               ( (bitvalue & 1) << bit ) )

#define GET_BIT(lvalue, bit)  (((lvalue) >> bit) & 0x1)

/* FM10000_CHIP_VERSION_A0 */
#define CHIP_VERSION_A0             0

#define LIB_MAX_STR_LEN             128

/* Max number of I2C buses supported */
/* Should be at least equal to the number of switch */
#define NUM_I2C_BUS                 12

/* Max number of PCA mux supported */
#define NUM_PCA_MUX                 16

/* Max number of PCA IO supported */
#define NUM_PCA_IO                  16

/* Max number of PCA pins supported */
#define NUM_PCA_PINS                40

/* Max number of pins per IO port */
#define NUM_PINS_PER_IO_PORT        8

/* Max number of LED per port supported. */
#define NUM_LED_PER_PORT            4
#define NUM_SUB_LED                 4

/* Specify value is not applicable or used */
#define UINT_NOT_USED               0xFFFFFFFF

/* Input Output configuration register value */
#define IOC_OUTPUT                  0
#define IOC_INPUT                   1

#define DBG_I2C                     (1<<0)
#define DBG_SKIP_SEL_BUS            (1<<1)
#define DBG_FORCE_MODPRES           (1<<2)
#define DBG_I2C_MUX                 (1<<3)
#define DBG_PORT_LED                (1<<4)
#define DBG_DUMP_CFG                (1<<5)

#define LED_USAGE_LINK              (1<<0)
#define LED_USAGE_TRAFFIC           (1<<1)
#define LED_USAGE_LINK_TRAFFIC      (1<<2) /* Keep for backward compatibility */
#define LED_USAGE_1G                (1<<3)
#define LED_USAGE_2PT5G             (1<<4)
#define LED_USAGE_10G               (1<<5)
#define LED_USAGE_25G               (1<<6)
#define LED_USAGE_40G               (1<<7)
#define LED_USAGE_100G              (1<<8)
#define LED_USAGE_ALLSPEED          (1<<9)

#define LED_USAGE_DEFAULT           (LED_USAGE_LINK | LED_USAGE_ALLSPEED)

#define LED_STATE_OFF               0
#define LED_STATE_ON                1
#define LED_STATE_BLINK_ON          2
#define LED_STATE_BLINK_OFF         3

/* Pin type supported */
typedef enum
{
    /* No LED */
    LED_TYPE_NONE = 0,

    /* PCA IO */
    LED_TYPE_PCA = 1,

    /* FPGA Control */
    LED_TYPE_FPGA = 2,

} fm_ledType;


/* Pin type supported */
typedef enum
{
    /* PCA MUX */
    BUS_SEL_TYPE_PCA_MUX,

    /* PCA IO Expander */
    BUS_SEL_TYPE_PCA_IO,

} fm_busSelType;


/* PCA mux device configuration */
typedef struct
{
    /* PCA device type */
    fm_pcaMuxModel   model;

    /* i2c bus where the device is located */
    fm_uint         bus;

    /* if this mux is behind another mux */
    fm_uint         parentMuxIdx;

    /* mux value of the parent to enable to this mux */
    fm_uint         parentMuxValue;

    /* I2C address of the device */
    fm_uint         addr;

    /* default or disabled value */
    fm_byte         initVal;

} fm_pcaMux;


/* PCA I/O device configuration */
typedef struct
{
    /* PCA device context */
    fm_pcaIoDevice  dev;

    /* if this device is behind a mux */
    fm_uint         parentMuxIdx;

    /* mux value of the parent to enable to this device */
    fm_uint         parentMuxValue;

} fm_pcaIo;


/* Mux select or pin enable configuration */
typedef struct
{
    /* I2C bus select type */
    fm_busSelType   busSelType;

    /* mux to the transceiver I2C bus, if pin type is PCAMUX */
    fm_uint         parentMuxIdx;

    /* mux value to enable to this device */
    fm_uint         parentMuxValue;

    /* IO to select transceiver I2C, if pin type is PCAIO */
    fm_uint         ioIdx;

    /* IO pin to select transceiver I2C, if pin type is PCAIO */
    fm_uint         ioPin;

    /* IO pin value to select transceiver I2C, if pin type is PCAIO */
    fm_uint         ioPinPolarity;

} fm_busSel;


/* SFP+ pin pattern configuration */
typedef struct
{
    /* MODPRES_N offset from base pin */
    fm_uint         modPresN;

    /* RX_LOS offset from base pin */
    fm_uint         rxLos;

    /* TXFAULT offset from base pin */
    fm_uint         txFault;

    /* TXDISABLE offset from base pin */
    fm_uint         txDisable;

} fm_sfppPin;


/* QSFP pin pattern configuration */
typedef struct
{
    /* MODPRES_N offset from base pin */
    fm_uint         modPresN;

    /* INT_N offset from base pin */
    fm_uint         intrN;

    /* LPMODE offset from base pin */
    fm_uint         lpMode;

    /* RESET_N offset from base pin */
    fm_uint         resetN;

} fm_qsfpPin;

typedef union
{
    fm_sfppPin sfppPin;
    fm_qsfpPin qsfpPin;

} fm_xcvrPin;


/* Interface type supported */
typedef enum
{
    /* None or not applicable */
    INTF_TYPE_NONE,

    /* SFP+ */
    INTF_TYPE_SFPP,

    /* QSFP */
    INTF_TYPE_QSFP,

    /* PCIE */
    INTF_TYPE_PCIE,

} fm_intfType;


/* Transceiver status/control configuration */
typedef struct
{
    /* Interface type (SFP+, QSFP, ...) */
    fm_intfType     intfType;

    /* index to pcaIo */
    fm_uint         ioIdx;

    /* base pin number, if using SFP+/QSFP pattern input */
    fm_uint         basePin;

    /* Transceiver pins */
    fm_xcvrPin      u;

} fm_xcvrIo;


/* Sub LED configuration */
typedef struct
{
    /* pin number if the LED is attached to a PCA IO expander or
       led number if the LED is attached to a PCA_LED driver */
    fm_uint pin;

    /* pin usage (LINK, TRAFFIC, speed) */
    fm_uint usage;

} fm_subLed;


/* Port LED configuration */
typedef struct
{
    /* LED type (PCA or FPGA) */
    fm_ledType type;

    /* index to pcaIo, if LED type is PCA */
    fm_uint ioIdx;

    /* sub LED configuration */
    fm_subLed subLed[NUM_SUB_LED];

} fm_portLed;


/* HwResource type supported */
typedef enum
{
    /* PORT */
    HWRESOURCE_TYPE_PORT = 0,

    /* Voltage regulator module */
    HWRESOURCE_TYPE_VRM = 1,

    /* PHY device */
    HWRESOURCE_TYPE_PHY = 2,

} fm_hmResourceType;


/* PHY I2C configuration */
typedef struct
{
    /* i2c bus where the device is located */
    fm_uint         bus;

    /* I2C bus select type */
    fm_busSelType   busSelType;

    /* if this mux is behind another mux */
    fm_uint         parentMuxIdx;

    /* mux value of the parent to enable to this mux */
    fm_uint         parentMuxValue;

} fm_phyI2C;


/* Hardware resource ID I2C voltage regulator module  configuration */
typedef struct
{
    /* device model */
    fm_vrmModel     model;

    /* i2c bus where the device is located */
    fm_uint         bus;

    /* I2C bus select type */
    fm_busSelType   busSelType;

    /* if this mux is behind another mux */
    fm_uint         parentMuxIdx;

    /* mux value of the parent to enable to this mux */
    fm_uint         parentMuxValue;

    /* I2C address of the device */
    fm_uint         addr;

} fm_vrmI2C;


/* Hardware resource ID configuration */
typedef struct
{
    /* Hardware resource type */
    fm_hmResourceType type;

    /* Bus select configuration */
    fm_busSel xcvrI2cBusSel;

    /* Transceiver status/control */
    fm_xcvrIo xcvrStateIo;

    /* Port LED */
    fm_portLed portLed[NUM_LED_PER_PORT];

    /* PHY I2C device configuration */
    fm_phyI2C phy;

    /* VRM I2C device configuration */
    fm_vrmI2C vrm;

} fm_hwResId;


/* I2C bus configuration */
typedef struct
{
    /* I2C Device name to open */
    fm_char devName[128];

    /* I2C file handle */
    fm_uintptr handle;

    /* Indicate if that bus uses FM10000 as master */
    fm_int isSwitchMaster;

    /* FM10000 chip version */
    fm_uint32  chipVersion;

    /* Indicates if the I2C_SMBUS_I2C_BLOCK_DATA transaction type is
     * supported by the linux SMBus driver associated to the i2c file desc.
     */
    fm_int i2cBlockSupported;

    /* I2C read/write function pointer */
    fm_utilI2cWriteReadHdnlFunc writeReadFunc;

    /* I2C debug function pointer */
    fm_setDebugFunc setDebugFunc;

    /* Indicates if the write-read operation is enabled on the switch */
    fm_int i2cWrRdEn;

} fm_i2cCfg;


typedef struct
{
    /* Current I2C bus selected by fmPlatformLibSelectBus */
    fm_uint         selectedBus;

    /* I2C Device config */
    fm_i2cCfg       i2c[NUM_I2C_BUS];

    /* PCA mux configuration */
    fm_uint         numPcaMux;
    fm_pcaMux       pcaMux[NUM_PCA_MUX];

    /* PCA IO configuration */
    fm_uint         numPcaIo;
    fm_pcaIo        pcaIo[NUM_PCA_IO];

    /* Number of ports supported */
    fm_uint         numResId;

    /* Hardware resource ID configuration */
    fm_hwResId      hwResId[FM_NUM_HW_RES_ID];

    /* Default bus selection type */
    fm_busSelType   defBusSelType;

    /* Pattern of the SFP+ pins, if applicable */
    fm_sfppPin      defSfppPat;

    /* Pattern of the QSFP pins, if applicable */
    fm_qsfpPin      defQsfpPat;

    /* File locking to share resources with other applications */
    fm_char         fileLockName[128];
    int             fileLock;

    /* debug control */
    fm_uint         debug;

} fm_hwCfg;


typedef struct
{
    /* String description of the value */
    fm_text desc;

    /* Value for the corresponding string */
    fm_int  value;

} fm_platformStrMap;


typedef union
{
    struct
    {
        fm_int m: 11; /* mantissa */
        fm_int e: 5;  /* exponent */
    } bits;

    fm_uint16 value;

} expman;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_hwCfg hwCfg;

static fm_platformStrMap ledTypeMap[] =
{
    { "NONE", LED_TYPE_NONE },
    { "PCA",  LED_TYPE_PCA  },
    { "FPGA", LED_TYPE_FPGA },
};

static fm_platformStrMap ledUsageMap[] =
{
    { "LINK",         LED_USAGE_LINK         },
    { "TRAFFIC",      LED_USAGE_TRAFFIC      },
    { "LINK_TRAFFIC", LED_USAGE_LINK_TRAFFIC },
    { "1G",           LED_USAGE_1G           },
    { "2.5G",         LED_USAGE_2PT5G        },
    { "10G",          LED_USAGE_10G          },
    { "25G",          LED_USAGE_25G          },
    { "40G",          LED_USAGE_40G          },
    { "100G",         LED_USAGE_100G         },
    { "ALLSPEED",     LED_USAGE_ALLSPEED     },
};

static fm_platformStrMap busSelTypeMap[] =
{
    { "PCAMUX", BUS_SEL_TYPE_PCA_MUX },
    { "PCAIO",  BUS_SEL_TYPE_PCA_IO  },
};

static fm_platformStrMap pcaMuxModelMap[] =
{
    { "PCA_UNKNOWN", PCA_MUX_UNKNOWN },
    { "PCA9541",     PCA_MUX_9541    },
    { "PCA9545",     PCA_MUX_9545    },
    { "PCA9546",     PCA_MUX_9546    },
    { "PCA9548",     PCA_MUX_9548    },
};

static fm_platformStrMap pcaIoModelMap[] =
{
    { "PCA_UNKNOWN", PCA_IO_UNKNOWN },
    { "PCA9505",     PCA_IO_9505    },
    { "PCA9506",     PCA_IO_9506    },
    { "PCA9538",     PCA_IO_9538    },
    { "PCA9539",     PCA_IO_9539    },
    { "PCA9551",     PCA_IO_9551    },
    { "PCA9554",     PCA_IO_9554    },
    { "PCA9555",     PCA_IO_9555    },
    { "PCA9634",     PCA_IO_9634    },
    { "PCA9635",     PCA_IO_9635    },
    { "PCA9698",     PCA_IO_9698    },
};

static fm_platformStrMap vrmModelMap[] = {
	{ "VRM_UNKNOWN", VRM_UNKNOWN  },
	{ "TPS40425",    VRM_TPS40425 },
	{ "PX8847",      VRM_PX8847   },
};

static fm_platformStrMap intfTypeMap[] =
{
    { "NONE", INTF_TYPE_NONE },
    { "SFPP", INTF_TYPE_SFPP },
    { "QSFP", INTF_TYPE_QSFP },
    { "PCIE", INTF_TYPE_PCIE },
};

static fm_platformStrMap debugMap[] =
{
    { "NONE",           0                 },
    { "I2C_RW",         DBG_I2C           },
    { "SKIP_SEL_BUS",   DBG_SKIP_SEL_BUS  },
    { "FORCE_MOD_PRES", DBG_FORCE_MODPRES },
    { "I2C_MUX",        DBG_I2C_MUX       },
    { "PORT_LED",       DBG_PORT_LED      },
    { "DUMP_CFG",       DBG_DUMP_CFG      },
};

static fm_platformStrMap hwResourceTypeMap[] =
{
    { "PORT",   HWRESOURCE_TYPE_PORT },
    { "VRM",    HWRESOURCE_TYPE_VRM  },
    { "PHY",    HWRESOURCE_TYPE_PHY  },
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
fm_status fmPlatformLibI2cWriteRead(fm_int   fd,
                                    fm_int   device,
                                    fm_byte *data,
                                    fm_int   wl,
                                    fm_int   rl);

fm_status fmPlatformLibGetVrmVoltage(fm_int     sw,
                                     fm_uint32  hwResourceId,
                                     fm_uint32 *mVolt);


/************************************************************************
 ****                                                                ****
 ****            BEGIN DOCUMENTED SHARED-LIB PROPERTIES              ****
 ****                                                                ****
 ************************************************************************/

/**
 * (Optional) Specifies what interface is used as I2C master to access
 * the port logic devices.
 *                                                                      \lb\lb
 * To indicate that the switch I2C is used as I2C master this property
 * must be set to switchI2C.
 *                                                                      \lb\lb
 * The available values are:
 *                                                                      \lb\lb
 *
 * switchI2C - Use FM10000 as master (default)
 *                                                                      \lb
 * /dev/i2c-x - Use CPU i2c/SMBus bus
 *                                                                      \lb
 * /dev/ipmi-x - Use ipmi library
 *                                                                      \lb\lb
 * If not defined, api.platform.lib.config.bus0.i2cDevName is
 * set to switchI2C
 *                                                                      \lb
 * Up to 12 buses can be defined.
 */
#define FM_AAK_LIB_BUS_I2C_DEV_NAME    "api.platform.lib.config.bus%d.i2cDevName"
#define FM_AAT_LIB_BUS_I2C_DEV_NAME    FM_API_ATTR_TEXT

/**
 * (Required) Number of PCA muxes present on the systems.
 *                                                                      \lb\lb
 * This includes only the PCA muxes attached to an FM10000 I2C bus tree.
 */
#define FM_AAK_LIB_PCAMUX_COUNT         "api.platform.lib.config.pcaMux.count"
#define FM_AAT_LIB_PCAMUX_COUNT         FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA mux model.
 *                                                                      \lb\lb
 * Supported PCA models: 9541, 9545, 9546 and 9548.
 */
#define FM_AAK_LIB_PCAMUX_MODEL         "api.platform.lib.config.pcaMux.%d.model"
#define FM_AAT_LIB_PCAMUX_MODEL         FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the bus number the mux is attached to.
 *                                                                      \lb\lb
 * If api.platform.lib.config.bus0.i2cDevName is set to switchI2C,
 * the bus number is the switch number the mux is attached to.
 *                                                                      \lb\lb
 * The default is set to bus 0.
 */
#define FM_AAK_LIB_PCAMUX_BUS           "api.platform.lib.config.pcaMux.%d.bus"
#define FM_AAT_LIB_PCAMUX_BUS           FM_API_ATTR_INT

/**
 * (Required) Specifies the mux I2C address.
 */
#define FM_AAK_LIB_PCAMUX_ADDR          "api.platform.lib.config.pcaMux.%d.addr"
#define FM_AAT_LIB_PCAMUX_ADDR          FM_API_ATTR_INT

/**
 * (Optional) If the PCA mux is behind another PCA mux, this property
 * specifies its parent PCA mux index.
 */
#define FM_AAK_LIB_PCAMUX_PARENT_INDEX  "api.platform.lib.config.pcaMux.%d.parent.index"
#define FM_AAT_LIB_PCAMUX_PARENT_INDEX   FM_API_ATTR_INT

/**
 * (Optional) If the PCA mux is behind another PCA mux, this property
 * specifies the value to write to its parent PCA mux to enable this mux.
 */
#define FM_AAK_LIB_PCAMUX_PARENT_VALUE  "api.platform.lib.config.pcaMux.%d.parent.value"
#define FM_AAT_LIB_PCAMUX_PARENT_VALUE  FM_API_ATTR_INT

/**
 * (Required) Number of PCA IO expanders present on the systems.
 *                                                                      \lb\lb
 * This includes only the IO expander attached to an FM10000 I2C bus
 * tree.
 */
#define FM_AAK_LIB_PCAIO_COUNT          "api.platform.lib.config.pcaIo.count"
#define FM_AAT_LIB_PCAIO_COUNT          FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA IO expander model.
 *                                                                      \lb\lb
 * Supported PCA models: 9505, 9506, 9551, 9554, 9555, 9634, 9635 and 9698
 */
#define FM_AAK_LIB_PCAIO_MODEL          "api.platform.lib.config.pcaIo.%d.model"
#define FM_AAT_LIB_PCAIO_MODEL          FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the bus number the PCA IO expander is attached to.
 *                                                                      \lb\lb
 * If api.platform.lib.config.bus0.i2cDevName is set to switchI2C,
 * the bus number is the switch number the mux is attached to.
 *                                                                      \lb\lb
 * The default is set to bus 0.
 */
#define FM_AAK_LIB_PCAIO_BUS            "api.platform.lib.config.pcaIo.%d.bus"
#define FM_AAT_LIB_PCAIO_BUS            FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA IO expander I2C address.
 */
#define FM_AAK_LIB_PCAIO_ADDR           "api.platform.lib.config.pcaIo.%d.addr"
#define FM_AAT_LIB_PCAIO_ADDR           FM_API_ATTR_INT

/**
 * (Optional) If the PCA IO expander is behind a PCA mux, this property
 * specifies its parent PCA mux index.
 */
#define FM_AAK_LIB_PCAIO_PARENT_INDEX   "api.platform.lib.config.pcaIo.%d.parent.index"
#define FM_AAT_LIB_PCAIO_PARENT_INDEX   FM_API_ATTR_INT

/**
 * (Optional) If the PCA IO expander is behind a PCA mux, this property
 * specifies the value to write to its parent PCA mux to reach this PCA IO.
 */
#define FM_AAK_LIB_PCAIO_PARENT_VALUE   "api.platform.lib.config.pcaIo.%d.parent.value"
#define FM_AAT_LIB_PCAIO_PARENT_VALUE   FM_API_ATTR_INT

/**
 * (Optional) Specifies the global blinking period for PCA9551 or PCA9634/35
 * LED driver.
 *                                                                      \lb\lb
 * For the PCA9634/35, that property defines the 8-bits GFRQ variable in the
 * following formula. Applies to all LEDs connected to that LED driver.
 *                                                                      \lb\lb
 * Blinking period is controlled through 256 linear steps from
 * 00h (41 ms, frequency 24 Hz) to FFh (10.73 s).
 *                                                                      \lb\lb
 *                           (GFRQ[7:0] + 1)
 *  Global blinking period = --------------- (sec)
 *                                 24
 *                                                                      \lb\lb
 * The default is set to 5 for 1/4 sec period.
 *                                                                      \lb\lb
 * For the PCA9551, that property defines the 8-bits PSC variable in the
 * following formula. Applies to all LEDs connected to that LED driver.
 *                                                                      \lb\lb
 * Blinking period is controlled through 256 linear steps from
 * 00h (26 ms, frequency 24 Hz) to FFh (6.74 s).
 *                                                                      \lb\lb
 *                     (PSC[7:0] + 1)
 *  Blinking period = --------------- (sec)
 *                           38
 *                                                                      \lb\lb
 * The default is set to 9 for ~1/4 sec period.
 */
#define FM_AAK_LIB_PCAIO_LED_BLINK_PERIOD       "api.platform.lib.config.pcaIo.%d.ledBlinkPeriod"
#define FM_AAT_LIB_PCAIO_LED_BLINK_PERIOD       FM_API_ATTR_INT

/**
 * (Optional) Specifies the global brightness value for PCA9634/35 LED driver.
 *                                                                      \lb\lb
 * In fact that property defines the 8-bits IDC variable in the following
 * formula. Applies to all LEDs connected to that LED driver.
 *                                                                      \lb\lb
 * Duty cycle is controlled through 256 linear steps from
 * 00h (0 % duty cycle = LED output off) to
 * FFh (99.6 % duty cycle = LED output at maximum brightness)
 *                                                                      \lb\lb
 *  duty cycle = IDC[7:0] / 256
 *                                                                      \lb\lb
 * The default is set to FFh for maximum brightness.
 */
#define FM_AAK_LIB_PCAIO_LED_BRIGHTNESS         "api.platform.lib.config.pcaIo.%d.ledBrightness"
#define FM_AAT_LIB_PCAIO_LED_BRIGHTNESS         FM_API_ATTR_INT

/**
 * (Optional) Mod_ABS default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all SFPP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_MODABS     "api.platform.lib.config.xcvrState.default.modAbs.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_MODABS     FM_API_ATTR_INT

/**
 * (Optional) Rx_LOS default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all SFPP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_RXLOS      "api.platform.lib.config.xcvrState.default.rxLos.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_RXLOS      FM_API_ATTR_INT

/**
 * (Optional) Tx_Disable default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all SFPP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_TXDISABLE  "api.platform.lib.config.xcvrState.default.txDisable.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_TXDISABLE  FM_API_ATTR_INT

/**
 * (Optional) Tx_Fault default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all SFPP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_TXFAULT    "api.platform.lib.config.xcvrState.default.txFault.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_TXFAULT    FM_API_ATTR_INT

/**
 * (Optional) ModPrsl default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all QSFP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_MODPRSL    "api.platform.lib.config.xcvrState.default.modPrsL.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_MODPRSL    FM_API_ATTR_INT

/**
 * (Optional) IntL default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all QSFP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_INTL       "api.platform.lib.config.xcvrState.default.intL.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_INTL       FM_API_ATTR_INT

/**
 * (Optional) LPMode default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all QSFP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_LPMODE     "api.platform.lib.config.xcvrState.default.lpMode.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_LPMODE     FM_API_ATTR_INT

/**
 * (Optional) resetL default pin offset from the PCA IO base pin.
 *                                                                      \lb\lb
 * Applies to all QSFP ports.
 *                                                                      \lb\lb
 * The default is set to UNUSED (0xffffffff).
 */
#define FM_AAK_LIB_XCVRSTATE_DEFAULT_RESETL     "api.platform.lib.config.xcvrState.default.resetL.pin"
#define FM_AAT_LIB_XCVRSTATE_DEFAULT_RESETL     FM_API_ATTR_INT

/**
 * (Required) Number of hardware resource ID used by the platform shared
 * library.
 *                                                                      \lb\lb
 * This number is the sum of the ports with the
 * switch.%d.portIndex.%d.interfaceType property set different than NONE.
 *                                                                      \lb\lb
 * See switch.%d.portIndex.%d.hwResourceId for details.
 */
#define FM_AAK_LIB_HWRESOURCE_ID_COUNT          "api.platform.lib.config.hwResourceId.count"
#define FM_AAT_LIB_HWRESOURCE_ID_COUNT          FM_API_ATTR_INT

/**
 * (Required) Specifies the interface type for this port.
 *                                                                      \lb\lb
 * Value is one of the following: NONE, SFPP or QSFP.
 *
 */
#define FM_AAK_LIB_HWRES_INTERFACE_TYPE             "api.platform.lib.config.hwResourceId.%d.interfaceType"
#define FM_AAT_LIB_HWRES_INTERFACE_TYPE             FM_API_ATTR_TEXT

/**
 * (Required) Specifies the PCA IO expander index the port is attached to.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_INDEX      "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.index"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_INDEX      FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA expander IO base pin number corresponding to
 * the first pin of the IO byte the port is attached to.
 *                                                                      \lb\lb
 * For example, the PCA9505 has 5 IO bytes. Each IO byte has 8 pins for
 * a total of 5 X 8 = 40 pins. Pin range for each IO byte is:
 * IO0: 0..7
 * IO1: 8..15
 * IO2: 16..23
 * IO3: 24..31
 * IO4: 32..39
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_BASEPIN    "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.basePin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_BASEPIN    FM_API_ATTR_INT

/**
 * (Optional) Mod_ABS pin offset from the PCA IO base pin for the given
 * SFPP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.modAbs.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_MODABS     "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.modAbs.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_MODABS     FM_API_ATTR_INT

/**
 * (Optional) Rx_LOS pin offset from the PCA IO base pin for the given
 * SFPP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.rxLos.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_RXLOS      "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.rxLos.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_RXLOS      FM_API_ATTR_INT

/**
 * (Optional) Tx_Disable pin offset from the PCA IO base pin for the given
 * SFPP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.txDisable.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_TXDISABLE  "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.txDisable.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_TXDISABLE  FM_API_ATTR_INT

/**
 * (Optional) Tx_Fault pin offset from the PCA IO base pin for the given
 * SFPP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.txFault.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_TXFAULT    "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.txFault.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_TXFAULT    FM_API_ATTR_INT

/**
 * (Optional) ModPrsl pin offset from the PCA IO base pin for the given
 * QSFP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.modPrsl.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_MODPRSL    "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.modPrsl.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_MODPRSL    FM_API_ATTR_INT

/**
 * (Optional) intL pin offset from the PCA IO base pin for the given
 * QSFP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.intL.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_INTL       "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.intL.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_INTL       FM_API_ATTR_INT

/**
 * (Optional) LPMode pin offset from the PCA IO base pin for the given
 * QSFP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.lpMode.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_LPMODE     "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.lpMode.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_LPMODE     FM_API_ATTR_INT

/**
 * (Optional) ResetL pin offset from the PCA IO base pin for the given
 * QSFP port.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrState.default.resetL.pin property.
 */
#define FM_AAK_LIB_HWRES_XCVRSTATE_PCAIO_RESETL     "api.platform.lib.config.hwResourceId.%d.xcvrState.pcaIo.resetL.pin"
#define FM_AAT_LIB_HWRES_XCVRSTATE_PCAIO_RESETL     FM_API_ATTR_INT

/**
 * (Optional) Specifies the default device type used to select the front
 * panel transceiver I2C bus.
 *                                                                      \lb\lb
 * Value is one of the following: PCAMUX, PCAIO.
 *                                                                      \lb\lb
 * Currently the platform shared library support PCAMUX type only.
 *                                                                      \lb\lb
 * The default value is set to PCAMUX.
 */
#define FM_AAK_LIB_XCVRI2C_DEFAULT_BUSSELTYPE       "api.platform.lib.config.xcvrI2C.default.busSelType"
#define FM_AAT_LIB_XCVRI2C_DEFAULT_BUSSELTYPE       FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the device type used to select the front panel
 * transceiver I2C bus.
 *                                                                      \lb\lb
 * Value is one of the following: PCAMUX, PCAIO.
 *                                                                      \lb\lb
 * Currently the platform shared library support PCAMUX type only.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrI2C.default.busSelType property.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_BUSSELTYPE         "api.platform.lib.config.hwResourceId.%d.xcvrI2C.busSelType"
#define FM_AAT_LIB_HWRES_XCVRI2C_BUSSELTYPE         FM_API_ATTR_TEXT

/**
 * (Required) Specifies the PCA mux index the front panel transceiver
 * is attached to.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_PCAMUX_INDEX       "api.platform.lib.config.hwResourceId.%d.xcvrI2C.pcaMux.index"
#define FM_AAT_LIB_HWRES_XCVRI2C_PCAMUX_INDEX       FM_API_ATTR_INT

/**
 * (Required) Specifies the value to write in the PCA mux to select the front
 *  panel transceiver I2C bus.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_PCAMUX_VALUE       "api.platform.lib.config.hwResourceId.%d.xcvrI2C.pcaMux.value"
#define FM_AAT_LIB_HWRES_XCVRI2C_PCAMUX_VALUE       FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA IO index the front panel transceiver
 * is attached to.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_PCAIO_INDEX       "api.platform.lib.config.hwResourceId.%d.xcvrI2C.pcaIo.index"
#define FM_AAT_LIB_HWRES_XCVRI2C_PCAIO_INDEX       FM_API_ATTR_INT

/**
 * (Required) Specifies the PCAIO pin to select the front
 *  panel transceiver I2C bus.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_PCAIO_PIN        "api.platform.lib.config.hwResourceId.%d.xcvrI2C.pcaIo.pin"
#define FM_AAT_LIB_HWRES_XCVRI2C_PCAIO_PIN        FM_API_ATTR_INT

/**
 * (Optional) Specifies the PCAIO pin value to select the front
 *  panel transceiver I2C bus.
 *
 * Default is 0 to enable.
 */
#define FM_AAK_LIB_HWRES_XCVRI2C_PCAIO_PIN_POLARITY  "api.platform.lib.config.hwResourceId.%d.xcvrI2C.pcaIo.pin.polarity"
#define FM_AAT_LIB_HWRES_XCVRI2C_PCAIO_PIN_POLARITY  FM_API_ATTR_INT

/**
 * (Optional) Specifies the device type this port LED is attached to.
 *                                                                      \lb\lb
 * Value is one of the following: NONE, PCA, FPGA.
 *                                                                      \lb\lb
 * FPGA support is not yet implemented.
 *                                                                      \lb\lb
 * Default value is NONE.
 */
#define FM_AAK_LIB_HWRES_PORTLED_TYPE               "api.platform.lib.config.hwResourceId.%d.portLed.%d.type"
#define FM_AAT_LIB_HWRES_PORTLED_TYPE               FM_API_ATTR_TEXT

/**
 * (Required) Specifies the PCA IO expander index this port LED is attached to.
 */
#define FM_AAK_LIB_HWRES_PORTLED_PCAIO_INDEX        "api.platform.lib.config.hwResourceId.%d.portLed.%d.pcaIo.index"
#define FM_AAT_LIB_HWRES_PORTLED_PCAIO_INDEX        FM_API_ATTR_INT

/**
 * (Required) Specifies the PCA expander IO pin number controlling this port LED.
 */
#define FM_AAK_LIB_HWRES_PORTLED_PCAIO_PIN          "api.platform.lib.config.hwResourceId.%d.portLed.%d.pcaIo.pin"
#define FM_AAT_LIB_HWRES_PORTLED_PCAIO_PIN          FM_API_ATTR_INT

#define FM_AAK_LIB_HWRES_PORT_SUBLED_PCAIO_PIN      "api.platform.lib.config.hwResourceId.%d.portLed.%d.%d.pcaIo.pin"
#define FM_AAT_LIB_HWRES_PORT_SUBLED_PCAIO_PIN      FM_API_ATTR_INT

/**
 * (Optional) Specifies the LED usage.
 *                                                                      \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following:
 *                                                                      \lb\lb
 * LINK     : Link UP (solid ON), Link DOWN (OFF)
 * TRAFFIC  : Blink on presence of tx or rx traffic
 * 1G       : LED applies to 1G link (includes 10/100M link speed)
 * 2.5G     : LED applies to 2.5G link (2500BASE-X)
 * 10G      : LED applies to 10G link
 * 25G      : LED applies to 25G link
 * 40G      : LED applies to 40G link
 * 100G     : LED applies to 100G link
 * ALLSPEED : LED applies to all speeds
 *                                                                      \lb\lb
 * The default value is set to LINK,ALLSPEED.
 */
#define FM_AAK_LIB_HWRES_PORTLED_PCAIO_USAGE        "api.platform.lib.config.hwResourceId.%d.portLed.%d.pcaIo.usage"
#define FM_AAT_LIB_HWRES_PORTLED_PCAIO_USAGE        FM_API_ATTR_TEXT

#define FM_AAK_LIB_HWRES_PORT_SUBLED_PCAIO_USAGE    "api.platform.lib.config.hwResourceId.%d.portLed.%d.%d.pcaIo.usage"
#define FM_AAT_LIB_HWRES_PORT_SUBLED_PCAIO_USAGE    FM_API_ATTR_TEXT

/**
 * (Optional) Shared-Lib debug options.
 *                                                                     \lb\lb
 * Value is a comma-separated string (with no spaces), consisting of
 * one or more of the following:
 * NONE, I2C_RW, I2C_MUX, SKIP_SEL_BUS, FORCE_MODPRES.
 *                                                                     \lb\lb
 * The default value is NONE.
 */

#define FM_AAK_LIB_CONFIG_DEBUG        "api.platform.lib.config.debug"
#define FM_AAT_LIB_CONFIG_DEBUG        FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the hardware resource type.
 *                                                                     \lb\lb
 * Value is one of the following: PORT or PHY.
 *                                                                     \lb\lb
 * The default value is PORT.
 */
#define FM_AAK_LIB_HWRES_TYPE                       "api.platform.lib.config.hwResourceId.%d.type"
#define FM_AAT_LIB_HWRES_TYPE                       FM_API_ATTR_TEXT

/**
 * (Optional) Specifies the device type used to select the PHY I2C bus.
 *                                                                      \lb\lb
 * Value is one of the following: PCAMUX, PCAIO.
 *                                                                      \lb\lb
 * Currently the platform shared library support PCAMUX type only.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrI2C.default.busSelType property.
 */
#define FM_AAK_LIB_HWRES_PHY_BUSSELTYPE             "api.platform.lib.config.hwResourceId.%d.phy.busSelType"
#define FM_AAT_LIB_HWRES_PHY_BUSSELTYPE             FM_API_ATTR_TEXT

/**
 * (Required) Specifies the PCA mux index the PHY device is attached to.
 */
#define FM_AAK_LIB_HWRES_PHY_PCAMUX_INDEX           "api.platform.lib.config.hwResourceId.%d.phy.pcaMux.index"
#define FM_AAT_LIB_HWRES_PHY_PCAMUX_INDEX           FM_API_ATTR_INT

/**
 * (Required) Specifies the value to write in the PCA mux to select the
 * PHY device I2C bus.
 */
#define FM_AAK_LIB_HWRES_PHY_PCAMUX_VALUE           "api.platform.lib.config.hwResourceId.%d.phy.pcaMux.value"
#define FM_AAT_LIB_HWRES_PHY_PCAMUX_VALUE           FM_API_ATTR_INT

/**
 * (Optional) Specifies the I2C bus number the PHY device is attached to.
 *                                                                      \lb\lb
 * The default is set to bus 0.
 */
#define FM_AAK_LIB_HWRES_PHY_BUS                    "api.platform.lib.config.hwResourceId.%d.phy.bus"
#define FM_AAT_LIB_HWRES_PHY_BUS                    FM_API_ATTR_INT


/**
 * (Optional) Specifies the device type used to select the
 * VRM I2C bus.
 *                                                                      \lb\lb
 * Value is one of the following: PCAMUX, PCAIO.
 *                                                                      \lb\lb
 * Currently the platform shared library support PCAMUX type only.
 *                                                                      \lb\lb
 * Default value is specified by the xcvrI2C.default.busSelType property.
 */
#define FM_AAK_LIB_HWRES_VRM_BUSSELTYPE             "api.platform.lib.config.hwResourceId.%d.vrm.busSelType"
#define FM_AAT_LIB_HWRES_VRM_BUSSELTYPE             FM_API_ATTR_TEXT

/**
 * (Required) Specifies the device model.
 *                                                                      \lb\lb
 * Device models: TPS40425, etc...
 */
#define FM_AAK_LIB_HWRES_VRM_MODEL                  "api.platform.lib.config.hwResourceId.%d.vrm.model"
#define FM_AAT_LIB_HWRES_VRM_MODEL                  FM_API_ATTR_TEXT

/**
 * (Required) Specifies the PCA mux index the VRM device is
 * attached to.
 */
#define FM_AAK_LIB_HWRES_VRM_PCAMUX_INDEX           "api.platform.lib.config.hwResourceId.%d.vrm.pcaMux.index"
#define FM_AAT_LIB_HWRES_VRM_PCAMUX_INDEX           FM_API_ATTR_INT

/**
 * (Required) Specifies the value to write in the PCA mux to
 * select the VRM device I2C bus.
 */
#define FM_AAK_LIB_HWRES_VRM_PCAMUX_VALUE           "api.platform.lib.config.hwResourceId.%d.vrm.pcaMux.value"
#define FM_AAT_LIB_HWRES_VRM_PCAMUX_VALUE           FM_API_ATTR_INT

/**
 * (Required) Specifies the VRM device I2C address.
 */
#define FM_AAK_LIB_HWRES_VRM_ADDR                   "api.platform.lib.config.hwResourceId.%d.vrm.addr"
#define FM_AAT_LIB_HWRES_VRM_ADDR                   FM_API_ATTR_INT

/**
 * (Optional) Specifies the bus number the VRM device is
 * attached to.
 *                                                                      \lb\lb
 * If api.platform.lib.config.bus0.i2cDevName is set to switchI2C,
 * the bus number is the switch number the device is attached
 * to.
 *                                                                      \lb\lb
 * The default is set to bus 0.
 */
#define FM_AAK_LIB_HWRES_VRM_BUS                    "api.platform.lib.config.hwResourceId.%d.vrm.bus"
#define FM_AAT_LIB_HWRES_VRM_BUS                    FM_API_ATTR_INT


/************************************************************************
 ****                                                                ****
 ****            END DOCUMENTED SHARED-LIB PROPERTIES                ****
 ****                                                                ****
 ****            BEGIN UNDOCUMENTED SHARED-LIB PROPERTIES            ****
 ****                                                                ****
 ************************************************************************/

/************************************************************
 * The following properties are not documented in the
 * Liberty Trail Software Specification as they are intended
 * for Intel Internal Use only.
 ************************************************************/

/**
 * Enable(1) or disable(0) the switch I2C write-read operation.
 *                                                                      \lb\lb
 * if disabled then the write-read operation is performed by doing first
 * the write operation and second by doing the read operation.
 */
#define FM_AAK_LIB_BUS_I2C_WR_RD_ENABLE         "api.platform.lib.config.bus%d.i2cWrRdEn"
#define FM_AAT_LIB_BUS_I2C_WR_RD_ENABLE         FM_API_ATTR_INT


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static fm_status SwitchI2cWriteRead(fm_uintptr handle,
                                    fm_uint    device,
                                    fm_byte   *data,
                                    fm_uint    wl,
                                    fm_uint    rl)
{
    fm_status  status;
    fm_int     i;
    fm_int     len;
    fm_int     sw;
    fm_i2cCfg *i2c;
    fm_uint32  rv;

    sw = (fm_int) handle;

    if (hwCfg.debug & DBG_I2C)
    {
        FM_LOG_PRINT("device 0x%x, wl %d, rl %d\n", device, wl, rl);

        if (wl > 0)
        {
            i = 0;
            len = (wl > 8) ? 8 : wl;
            len = wl;
            FM_LOG_PRINT("wr-data: ");
            while (len--)
            {
                FM_LOG_PRINT("%02x", data[i++]);
            }
        }
    }

    i2c = &hwCfg.i2c[sw];

    /* Read chip version: FM10000_CHIP_VERSION => register: 0x452 */
    status = fmReadUINT32(sw, 0x452, &rv);
    if (status == FM_OK)
    {
        i2c->chipVersion = rv & 0x7F;
        if ( i2c->chipVersion == CHIP_VERSION_A0 )
        {
            i2c->i2cWrRdEn = FALSE;
        }

        if (i2c->i2cWrRdEn)
        {
            status = fmI2cWriteRead(sw, device, data, wl, rl);
        }
        else
        {
            status = fmI2cWriteRead(sw, device, data, wl, 0);
            if (rl && status == FM_OK)
            {
                status = fmI2cWriteRead(sw, device, data, 0, rl);
            }
        }
    }

    if (status == FM_OK)
    {
        if (hwCfg.debug & DBG_I2C)
        {
            if (rl > 0)
            {
                i = 0;
                len = (rl > 8) ? 8 : rl;
                FM_LOG_PRINT("  rd-data: ");
                while (len--)
                {
                    FM_LOG_PRINT("%02x", data[i++]);
                }
            }
            FM_LOG_PRINT("\n");

        }
    }
    else
    {
        FM_LOG_PRINT("i2c error=%d (sw=%d devAddr %02x)\n", status, sw, device);
    }

    return status;

}   /* end SwitchI2cWriteRead */




/*****************************************************************************/
/* TakeLock
 * \ingroup intPlatform
 *
 * \desc            Take proper lock before accessing hardware.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status TakeLock(void)
{
    fm_status status = FM_OK;

    if (hwCfg.fileLock >= 0)
    {
        status = fmUtilDeviceLockTake(hwCfg.fileLock);
    }

    return status;

}   /* end TakeLock */




/*****************************************************************************/
/* DropLock
 * \ingroup intPlatform
 *
 * \desc            Drop lock taken.
 *
 * \return          none
 *
 *****************************************************************************/
static void DropLock(void)
{
    if (hwCfg.fileLock >= 0)
    {
        fmUtilDeviceLockDrop(hwCfg.fileLock);
    }

}   /* end DropLock */




/*****************************************************************************/
/* GetStrMap
 * \ingroup intPlatform
 *
 * \desc            Get string equivalent of the specified value given
 *                  the string mapping.
 *
 * \param[in]       strMap points to an array of fm_platformStrMap, where the
 *                  mapping of a string representation of a specific value.
 *
 * \param[in]       size is the size of the strMap array.
 *
 * \param[in]       hexVal is whether to display value in hex format.
 *
 * \param[out]      strBuf is the pointer to the buffer where the out is stored.
 *
 * \param[in]       strLen is the length of the strBuf.
 *
 * \return          strBuf, string representation of the value or UNKNOWN.
 *
 *****************************************************************************/
static fm_text GetStrMap(fm_int             value,
                         fm_platformStrMap *strMap,
                         fm_int             size,
                         fm_bool            hexVal,
                         fm_text            strBuf,
                         fm_int             strLen)
{
    fm_int         cnt;

    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value == strMap[cnt].value)
        {
#if 0
            FM_SNPRINTF_S(strBuf, strLen, hexVal?"%s(0x%x)":"%s(%d)",
                          strMap[cnt].desc, value);
#else
            FM_SNPRINTF_S(strBuf, strLen, "%s", strMap[cnt].desc);
#endif
            return strBuf;
        }
    }

    FM_SNPRINTF_S(strBuf, strLen, "UNKNOWN(%d)", value);
    return strBuf;

}   /* end GetStrMap */




/*****************************************************************************/
/* GetStrBitMap
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
 * \param[out]      strBuf is the pointer to the buffer where the out is stored.
 *
 * \param[in]       strLen is the length of the strBuf.
 *
 * \return          string representation of the bit mask separated by commas.
 *
 *****************************************************************************/
static fm_text GetStrBitMap(fm_int             value,
                            fm_platformStrMap *strMap,
                            fm_int             size,
                            fm_text            strBuf,
                            fm_int             strLen)
{
    fm_int         cnt;
    fm_int         len;

    /* Empty string if no bit is set */
    strBuf[0] = '\0';

    len = strLen;
    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value & strMap[cnt].value)
        {
            if (strBuf[0] != '\0')
            {
                FM_STRCAT_S(strBuf, len, ",");
                len = strLen - strlen(strBuf);
            }

            FM_STRCAT_S(strBuf, len, strMap[cnt].desc);
            len = strLen - strlen(strBuf);
        }
    }

    return strBuf;

}   /*  end GetStrBitMap */




/*****************************************************************************/
/* ValidateBusNumber
 * \ingroup intPlatform
 *
 * \desc            Verify if the bus number is valid
 *
 * \param[in]       bus is the bus number to validate
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ValidateBusNumber(fm_int bus)
{
    fm_status status;

    /* Is that bus number in the valid range */
    if ( bus >= 0 && bus < NUM_I2C_BUS )
    {
        /* Yes, make sure that bus has been defined */
        if ( !strlen(hwCfg.i2c[bus].devName) )
        {
            /* This bus is not defined */
            status = FM_ERR_NOT_FOUND;
        }
        else
        {
            status = FM_OK;
        }
    }
    else
    {
        /* No, bus number is out of range */
        status = FM_ERR_INVALID_ARGUMENT;
    }

    return status;

}   /*  end ValidateBusNumber */




/*****************************************************************************/
/* IsPcaIoLedDriver
 * \ingroup intPlatform
 *
 * \desc             This function determines if the given model
 *                   support LED driver.
 *
 * \param[in]        model is the PCA IO model
 *
 * \return           true if the model support LED driver
 *
 *****************************************************************************/
static fm_bool IsPcaIoLedDriver(fm_pcaIoModel model)
{
    switch (model)
    {
        case PCA_IO_9634:
        case PCA_IO_9635:
        case PCA_IO_9551:
            return TRUE;
        default:
            return FALSE;
    }
}




/*****************************************************************************/
/* IsPortSpeedSupportedByLed
 * \ingroup intPlatform
 *
 * \desc            This function determines if the given port speed applies
 *                  to the given LED.
 *
 * \param[in]       speed is the port speed
 *
 * \param[in]       usage is the LED usage options
 *
 * \return          true if the speed is supported by the given LED
 *
 *****************************************************************************/
static fm_bool IsPortSpeedSupportedByLed(fm_platPortLedSpeed speed,
                                         fm_uint             usage)
{
    fm_bool supported;

    if (usage & LED_USAGE_ALLSPEED)
    {
        return TRUE;
    }

    switch (speed)
    {
        case FM_PLAT_PORT_LED_SPEED_100G:
            supported = (usage & LED_USAGE_100G) ? TRUE : FALSE;
            break;

        case FM_PLAT_PORT_LED_SPEED_40G:
            supported = (usage & LED_USAGE_40G) ? TRUE : FALSE;
            break;

        case FM_PLAT_PORT_LED_SPEED_25G:
            supported = (usage & LED_USAGE_25G) ? TRUE : FALSE;
            break;

        case FM_PLAT_PORT_LED_SPEED_10G:
            supported = (usage & LED_USAGE_10G) ? TRUE : FALSE;
            break;

        case FM_PLAT_PORT_LED_SPEED_2P5G:
            supported = (usage & LED_USAGE_2PT5G) ? TRUE : FALSE;
            break;

        case FM_PLAT_PORT_LED_SPEED_1G:
        case FM_PLAT_PORT_LED_SPEED_100M:
        case FM_PLAT_PORT_LED_SPEED_10M:
            supported = (usage & LED_USAGE_1G) ? TRUE : FALSE;
            break;

        default:
            supported = FALSE;
            break;
    }

    return supported;

}   /* end IsPortSpeedSupportedByLed */




/*****************************************************************************/
/* GetLedState
 * \ingroup intPlatform
 *
 * \desc            This function returns the final LED state value based on
 *                  the requested led state, the speed and the LED usage mode.
 *
 * \param[in]       reqLedState is the requested LED state provided by the
 *                  LT platform code.
 *
 * \param[in]       usage is the LED usage options
 *
 * \return          One of the LED_STATE_XXX led state
 *
 *****************************************************************************/
static fm_int GetLedState(fm_uint32 reqLedState, fm_uint usage)
{
    fm_platPortLedState state;
    fm_platPortLedSpeed speed;
    fm_int              ledState;

    state = HW_LED_STATE_TO_STATE(reqLedState);
    speed = HW_LED_STATE_TO_SPEED(reqLedState);

    /* Turn OFF the LED by default. */
    ledState = LED_STATE_OFF;

    if ( state == FM_PLAT_PORT_LED_LINK_UP )
    {
        /* Is that LED used to report LINK state for the given speed? */
        if ( ( (usage & LED_USAGE_LINK) ||
               (usage & LED_USAGE_LINK_TRAFFIC) ) &&
             IsPortSpeedSupportedByLed(speed, usage) )
        {
            /* Yes, turn ON that LED */
            ledState = LED_STATE_ON;
        }
    }
    else if ( state == FM_PLAT_PORT_LED_BLINK_ON ||
              state == FM_PLAT_PORT_LED_BLINK_OFF )
    {
        /* Is the speed supported by that LED? */
        if ( IsPortSpeedSupportedByLed(speed, usage) )
        {
            /* Yes, is that LED used to report TRAFFIC state */
            if ( ( (usage & LED_USAGE_TRAFFIC) ||
                   (usage & LED_USAGE_LINK_TRAFFIC) ) )
            {
                /* Yes, BLINK that LED */
                if ( state == FM_PLAT_PORT_LED_BLINK_ON )
                {
                    ledState = LED_STATE_BLINK_ON;
                }
                else
                {
                    ledState = LED_STATE_BLINK_OFF;
                }
            }
            /* Is that LED used to report LINK state */
            else if ( usage & LED_USAGE_LINK )
            {
                /* Yes, turn ON that LED */
                ledState = LED_STATE_ON;
            }
        }
    }
    else if ( state == FM_PLAT_PORT_LED_LINK_DOWN )
    {
        /* The port is down, turn OFF that LED no matter the usage mode. */
    }

    if (hwCfg.debug & DBG_PORT_LED)
    {
        FM_LOG_PRINT("GetLedState: reqLedState 0x%x usage 0x%x "
                     "state %d speed %d return -> ledState %d\n",
                     reqLedState,
                     usage,
                     state,
                     speed,
                     ledState);
    }

    return ledState;

}   /* end GetLedState */




/*****************************************************************************/
/* CopyTlvStr
 * \ingroup intPlatform
 *
 * \desc            Copy TLV bytes into string buffer.
 *
 * \param[in]       dest is the pointer to the string buffer.
 *
 * \param[in]       destSize is the size of the dest buffer.
 *
 * \param[in]       src is the pointer to the TLV buffer.
 *
 * \param[in]       srcSize is the size of the TLV buffer.
 *
 * \return          Integer equivalent of the TLV bytes.
 *
 *****************************************************************************/
void CopyTlvStr(fm_text dest, fm_int destSize, fm_byte *src, fm_int srcSize)
{
    fm_int len;

    /* Reserve one for null terminated character */
    len = destSize - 1;
    if (len > srcSize)
    {
        len = srcSize;
    }

    FM_MEMCPY_S(dest, destSize, src, len);
    dest[len] = '\0';

} /* CopyTlvStr */




/*****************************************************************************/
/* GetTlvInt
 * \ingroup intPlatform
 *
 * \desc            Get up to 32-bit integer from TLV bytes.
 *                  Note: All encoding less than 32-bit will be assumed
 *                  as unsigned integer.
 *
 * \param[in]       tlv is an array of bytes.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \return          Integer equivalent of the TLV bytes.
 *
 *****************************************************************************/
static fm_int GetTlvInt(fm_byte *tlv, fm_int tlvLen)
{
    fm_int j;
    fm_int value;

    if (tlvLen > 4)
    {
        tlvLen = 4;
    }

    value = tlv[0];
    for (j = 1; j < tlvLen; j++)
    {
        value <<= 8;
        value  |= (tlv[j] & 0xFF);
    }

    return value;

}   /* end GetTlvInt */




/*****************************************************************************/
/* GetTlvBool
 * \ingroup intPlatform
 *
 * \desc            Get boolean from TLV byte.
 *
 * \param[in]       tlv is the pointer to a byte.
 *
 * \return          Integer equivalent of the TLV bytes.
 *
 *****************************************************************************/
static fm_bool GetTlvBool(fm_byte *tlv)
{

    return  ((*tlv) ? TRUE : FALSE);

}   /* end GetTlvBool */




/*****************************************************************************/
/* LoadConfig
 * \ingroup intPlatform
 *
 * \desc            Load shared library configuration.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LoadConfig(void)
{
    fm_status   status;
    fm_int      swIdx;
	fm_portLed *portLed;
	fm_phyI2C  *phyI2c;
	fm_vrmI2C  *vrmI2c;
	fm_busSel  *xcvrI2cBus;
	fm_xcvrIo  *xcvrIo;
	fm_uint     tlvType;
	fm_uint     tlvLen;
	fm_uint     bus;
	fm_uint     idx;
	fm_uint     led;
	fm_uint     subLed;
    fm_byte    *tlvCfg;
    fm_uint     tlvCfgLen;
    fm_byte    *tlv;
    fm_uint     processedLen;

    swIdx = 0;
    status = fmPlatformRequestLibTlvCfg(swIdx, &tlvCfg, &tlvCfgLen);
    if (status)
    {
        return status;
    }

    processedLen = 0;
    while (processedLen < tlvCfgLen)
    {
        tlv = tlvCfg + processedLen;
        tlvType = (tlv[0] << 8) | tlv[1];
        tlvLen = tlv[2];
        processedLen += (tlvLen + 3);

        switch (tlvType) {
        case FM_TLV_PLAT_FILE_LOCK_NAME: /* Shared with platform config */
            CopyTlvStr(hwCfg.fileLockName,
                    sizeof(hwCfg.fileLockName),
                    tlv + 3, tlvLen);
            break;
        case FM_TLV_PLAT_LIB_I2C_DEVNAME:
            bus = GetTlvInt(tlv + 3, 1);
            if (bus >= NUM_I2C_BUS)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            CopyTlvStr(hwCfg.i2c[bus].devName,
                    sizeof(hwCfg.i2c[bus].devName),
                    tlv + 4, tlvLen - 1);
            break;
        case FM_TLV_PLAT_LIB_MUX_COUNT:
            hwCfg.numPcaMux = GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_MUX_MODEL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaMux || idx >= NUM_PCA_MUX)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaMux[idx].model = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_MUX_BUS:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaMux || idx >= NUM_PCA_MUX)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaMux[idx].bus = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_MUX_ADDR:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaMux || idx >= NUM_PCA_MUX)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaMux[idx].addr = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_LIB_MUX_PARENT_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaMux || idx >= NUM_PCA_MUX)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaMux[idx].parentMuxIdx =
                GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_MUX_PARENT_VAL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaMux || idx >= NUM_PCA_MUX)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaMux[idx].parentMuxValue =
                GetTlvInt(tlv + 4, 4);
            break;

        case FM_TLV_PLAT_LIB_IO_COUNT:
            hwCfg.numPcaIo = GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_IO_MODEL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].dev.model = GetTlvInt(tlv + 4, 1);
            switch (hwCfg.pcaIo[idx].dev.model)
            {
                case PCA_IO_9634:
                case PCA_IO_9635:
                    hwCfg.pcaIo[idx].dev.ledBlinkPeriod = 5;
                    hwCfg.pcaIo[idx].dev.ledBrightness = 255;
                    break;
                case PCA_IO_9551:
                   hwCfg.pcaIo[idx].dev.ledBlinkPeriod  = 9;
                    break;
                default:
                    break;
            }
            break;
        case FM_TLV_PLAT_LIB_IO_BUS:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].dev.bus = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_IO_ADDR:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].dev.addr = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_LIB_IO_PARENT_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].parentMuxIdx =
                GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_IO_PARENT_VAL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].parentMuxValue =
                GetTlvInt(tlv + 4, 4);
            break;

        case FM_TLV_PLAT_LIB_IO_LED_BLINK_PER:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].dev.ledBlinkPeriod =
                GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_IO_LED_BRIGHTNESS:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numPcaIo || idx >= NUM_PCA_IO)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.pcaIo[idx].dev.ledBrightness =
                GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_MODABS_PIN:
            hwCfg.defSfppPat.modPresN =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_RXLOS_PIN:
            hwCfg.defSfppPat.rxLos =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_TXDISABLE_PIN:
            hwCfg.defSfppPat.txDisable =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_TXFAULT_PIN:
            hwCfg.defSfppPat.txFault =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_MODPRESL_PIN:
            hwCfg.defQsfpPat.modPresN =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_INTL_PIN:
            hwCfg.defQsfpPat.intrN =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_LPMODE_PIN:
            hwCfg.defQsfpPat.lpMode =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_DEF_RESETL_PIN:
            hwCfg.defQsfpPat.resetN =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_HWRESID_COUNT:
            hwCfg.numResId =
                GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_LIB_HWRESID_INTF_TYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->intfType = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->ioIdx = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_BASE_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->basePin = GetTlvInt(tlv + 4, 1);

            /* Now calculate pin offsets */
            if (xcvrIo->intfType == INTF_TYPE_SFPP) {
                xcvrIo->u.sfppPin.modPresN = xcvrIo->basePin +
                    hwCfg.defSfppPat.modPresN;
                xcvrIo->u.sfppPin.rxLos = xcvrIo->basePin +
                    hwCfg.defSfppPat.rxLos;
                xcvrIo->u.sfppPin.txFault = xcvrIo->basePin +
                    hwCfg.defSfppPat.txFault;
                xcvrIo->u.sfppPin.txDisable = xcvrIo->basePin +
                    hwCfg.defSfppPat.txDisable;
            } else if (xcvrIo->intfType == INTF_TYPE_QSFP) {
                xcvrIo->u.qsfpPin.modPresN = xcvrIo->basePin +
                    hwCfg.defQsfpPat.modPresN;
                xcvrIo->u.qsfpPin.intrN = xcvrIo->basePin +
                    hwCfg.defQsfpPat.intrN;
                xcvrIo->u.qsfpPin.lpMode = xcvrIo->basePin +
                    hwCfg.defQsfpPat.lpMode;
                xcvrIo->u.qsfpPin.resetN = xcvrIo->basePin +
                    hwCfg.defQsfpPat.resetN;
            }
            break;
        case FM_TLV_PLAT_LIB_XCVR_MODABS_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.sfppPin.modPresN = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_RXLOS_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.sfppPin.rxLos = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_TXDISABLE_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.sfppPin.txDisable = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_TXFAULT_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.sfppPin.txFault = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_MODPRESL_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.qsfpPin.modPresN = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_INTL_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.qsfpPin.intrN = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_LPMODE_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.qsfpPin.lpMode = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_RESETL_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrIo = &hwCfg.hwResId[idx].xcvrStateIo;
            xcvrIo->u.qsfpPin.resetN = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_I2C_DEF_BUSSELTYPE:
            for (idx = 0; idx < hwCfg.numResId; idx++) {
                xcvrI2cBus = &hwCfg.hwResId[idx].xcvrI2cBusSel;
                xcvrI2cBus->busSelType =
                    GetTlvInt(tlv + 3, 1);
            }
            break;
        case FM_TLV_PLAT_LIB_XCVR_I2_BUSSELTYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrI2cBus = &hwCfg.hwResId[idx].xcvrI2cBusSel;
            xcvrI2cBus->busSelType = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_MUX_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrI2cBus = &hwCfg.hwResId[idx].xcvrI2cBusSel;
            xcvrI2cBus->parentMuxIdx = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_XCVR_MUX_VAL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            xcvrI2cBus = &hwCfg.hwResId[idx].xcvrI2cBusSel;
            xcvrI2cBus->parentMuxValue = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_LIB_PORTLED_TYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            portLed->type = GetTlvInt(tlv + 5, 1);
        break;
        case FM_TLV_PLAT_LIB_PORTLED_IO_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            portLed->ioIdx = GetTlvInt(tlv + 5, 1);
        break;
        case FM_TLV_PLAT_LIB_PORTLED_IO_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            portLed->subLed[0].pin = GetTlvInt(tlv + 5, 1);
        break;
        case FM_TLV_PLAT_LIB_PORTLED_IO_LANE_PIN:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            subLed = GetTlvInt(tlv + 5, 1);
            if (subLed >= NUM_SUB_LED)
                return FM_ERR_INVALID_ARGUMENT;
            portLed->subLed[subLed].pin = GetTlvInt(tlv + 6, 1);
        break;
        case FM_TLV_PLAT_LIB_PORTLED_IO_USAGE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            portLed->subLed[0].usage = GetTlvInt(tlv + 5, 4);
        break;
        case FM_TLV_PLAT_LIB_PORTLED_IO_LANE_USAGE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            led = GetTlvInt(tlv + 4, 1);
            if (led >= NUM_LED_PER_PORT)
                return FM_ERR_INVALID_ARGUMENT;
            portLed = &hwCfg.hwResId[idx].portLed[led];
            subLed = GetTlvInt(tlv + 5, 1);
            if (subLed >= NUM_SUB_LED)
                return FM_ERR_INVALID_ARGUMENT;
            portLed->subLed[subLed].usage = GetTlvInt(tlv + 6, 4);
        break;
        case FM_TLV_PLAT_LIB_HWRESID_TYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            hwCfg.hwResId[idx].type = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_DEBUG:
            hwCfg.debug = GetTlvInt(tlv + 3, 4);
            break;
        case FM_TLV_PLAT_LIB_PHY_BUSSELTYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyI2c = &hwCfg.hwResId[idx].phy;
            phyI2c->busSelType = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_PHY_MUX_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyI2c = &hwCfg.hwResId[idx].phy;
            phyI2c->parentMuxIdx = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_PHY_MUX_VAL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyI2c = &hwCfg.hwResId[idx].phy;
            phyI2c->parentMuxValue = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_LIB_PHY_BUS:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyI2c = &hwCfg.hwResId[idx].phy;
            phyI2c->bus = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_VRM_BUSSELTYPE:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->busSelType = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_VRM_MODEL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->model = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_VRM_MUX_IDX:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->parentMuxIdx = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_VRM_MUX_VAL:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->parentMuxValue = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_LIB_VRM_MUX_ADDR:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->addr = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_LIB_VRM_MUX_BUS:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= hwCfg.numResId || idx >= FM_NUM_HW_RES_ID)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            vrmI2c = &hwCfg.hwResId[idx].vrm;
            vrmI2c->bus = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_LIB_BUS_I2C_EN_WR_RD:
            idx = GetTlvInt(tlv + 3, 1);
            if (idx >= NUM_I2C_BUS)
                return FM_ERR_INVALID_ARGUMENT;
            hwCfg.i2c[idx].i2cWrRdEn = GetTlvBool(tlv + 4);
            break;
        }
    }

    /* Release lib config to free memory */
    fmPlatformReleaseLibTlvCfg(swIdx);

    return 0;
}  /* end LoadConfig */




/*****************************************************************************/
/* DumpConfig
 * \ingroup intPlatform
 *
 * \desc            Dump configuration.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void DumpConfig(void)
{
    fm_portLed *portLed;
    fm_hwResId *hwResId;
    fm_pcaIo *  pcaIo;
    fm_xcvrIo * xcvrIo;
    fm_pcaMux * pcaMux;
    fm_busSel * xcvrI2cBus;
    fm_uint     cnt;
    fm_uint     led;
    fm_uint     subLed;
    fm_char     buf[LIB_MAX_STR_LEN];
    fm_char     tmpStr[LIB_MAX_STR_LEN];

#define PRINT_STR(name, string) \
    FM_SNPRINTF_S(buf, LIB_MAX_STR_LEN, name, cnt); \
    PRINT_STRING(buf, string);

#define PRINT_VAL(name, val) \
    FM_SNPRINTF_S(buf, LIB_MAX_STR_LEN, name, cnt); \
    PRINT_VALUE(buf, val);

#define PRINT_LED_STR(name, string) \
    FM_SNPRINTF_S(buf, LIB_MAX_STR_LEN, name, cnt, led); \
    PRINT_STRING(buf, string);

#define PRINT_SUBLED_STR(name, string) \
    FM_SNPRINTF_S(buf, LIB_MAX_STR_LEN, name, cnt, led, subLed); \
    PRINT_STRING(buf, string);

#define PRINT_LED_VAL(name, val) \
    FM_SNPRINTF_S(buf,LIB_MAX_STR_LEN, name, cnt, led); \
    PRINT_VALUE(buf, val);

#define PRINT_SUBLED_VAL(name, val) \
    FM_SNPRINTF_S(buf,LIB_MAX_STR_LEN, name, cnt, led, subLed); \
    PRINT_VALUE(buf, val);

    PRINT_VALUE("debug", hwCfg.debug);

    for (cnt = 0 ; cnt < NUM_I2C_BUS; cnt++)
    {
        PRINT_STR("i2cDevName[%d]", hwCfg.i2c[cnt].devName);
    }

    PRINT_STRING("fileLockName",  hwCfg.fileLockName);
    FM_LOG_PRINT("\n");

    /* PCA mux configuration */
    PRINT_VALUE("pcaMux.count", hwCfg.numPcaMux);
    for (cnt = 0 ; cnt < hwCfg.numPcaMux; cnt++)
    {
        pcaMux = &hwCfg.pcaMux[cnt];
        PRINT_STR("  pcaMux[%d].model", PcaMuxModelToStr(pcaMux->model));
        PRINT_VAL("  pcaMux[%d].bus", pcaMux->bus);
        PRINT_VAL("  pcaMux[%d].addr", pcaMux->addr);
        PRINT_VAL("  pcaMux[%d].parent.index", pcaMux->parentMuxIdx);
        PRINT_VAL("  pcaMux[%d].parent.value", pcaMux->parentMuxValue);
    }
    FM_LOG_PRINT("\n");

    /* PCA IO configuration */
    PRINT_VALUE("pcaIo.count", hwCfg.numPcaIo);
    for (cnt = 0 ; cnt < hwCfg.numPcaIo; cnt++)
    {
        pcaIo = &hwCfg.pcaIo[cnt];
        PRINT_STR("  pcaIo[%d].model", PcaIoModelToStr(pcaIo->dev.model));
        PRINT_VAL("  pcaIo[%d].bus", pcaIo->dev.bus);
        PRINT_VAL("  pcaIo[%d].addr", pcaIo->dev.addr);
        PRINT_VAL("  pcaIo[%d].parent.index", pcaIo->parentMuxIdx);
        PRINT_VAL("  pcaIo[%d].parent.value", pcaIo->parentMuxValue);
        PRINT_VAL("  pcaIo[%d].ledBlinkPeriod", pcaIo->dev.ledBlinkPeriod);
        PRINT_VAL("  pcaIo[%d].ledBrightness", pcaIo->dev.ledBrightness);
        PRINT_VAL("  pcaIo[%d].devCap.numBytes", pcaIo->dev.devCap.numBytes);
        PRINT_VAL("  pcaIo[%d].devCap.numBits", pcaIo->dev.devCap.numBits);
        PRINT_VAL("  pcaIo[%d].devCap.cap", pcaIo->dev.devCap.cap);
    }
    FM_LOG_PRINT("\n");

    PRINT_VALUE("defSfppPat.modPresN", hwCfg.defSfppPat.modPresN);
    PRINT_VALUE("defSfppPat.rxLos",    hwCfg.defSfppPat.rxLos);
    PRINT_VALUE("defSfppPat.txDisable",hwCfg.defSfppPat.txDisable);
    PRINT_VALUE("defSfppPat.txFault",  hwCfg.defSfppPat.txFault);
    PRINT_VALUE("defQsfpPat.modPresN", hwCfg.defQsfpPat.modPresN);
    PRINT_VALUE("defQsfpPat.intrN",    hwCfg.defQsfpPat.intrN);
    PRINT_VALUE("defQsfpPat.resetN",   hwCfg.defQsfpPat.resetN);
    PRINT_VALUE("defQsfpPat.lpMode",   hwCfg.defQsfpPat.lpMode);
    FM_LOG_PRINT("\n");

    /* Number of hwResource ports supported */
    PRINT_VALUE("hwResId.count", hwCfg.numResId);
    for (cnt = 0 ; cnt < hwCfg.numResId; cnt++)
    {
        hwResId = &hwCfg.hwResId[cnt];

        if (hwCfg.hwResId[cnt].type == HWRESOURCE_TYPE_PORT)
        {
            xcvrIo = &hwResId->xcvrStateIo;
            xcvrI2cBus = &hwResId->xcvrI2cBusSel;

            /* Mux select configuration */
            PRINT_STR("  hwResId.%d.xcvrI2C.busSelType",
                      BusSelTypeToStr(xcvrI2cBus->busSelType));

            if (xcvrI2cBus->busSelType == BUS_SEL_TYPE_PCA_MUX)
            {
                PRINT_VAL("  hwResId.%d.xcvrI2C.pcaMux.index",
                          xcvrI2cBus->parentMuxIdx);
                PRINT_VAL("  hwResId.%d.xcvrI2C.pcaMux.value",
                          xcvrI2cBus->parentMuxValue);
            }
            else if (xcvrI2cBus->busSelType == BUS_SEL_TYPE_PCA_IO)
            {
                PRINT_VAL("  hwResId.%d.xcvrI2C.pcaIo.index",
                          xcvrI2cBus->ioIdx);
                PRINT_VAL("  hwResId.%d.xcvrI2C.pcaIo.pin",
                          xcvrI2cBus->ioPin);
                PRINT_VAL("  hwResId.%d.xcvrI2C.pcaIo.pin.polarity",
                          xcvrI2cBus->ioPinPolarity);
            }

            /* Transceiver status/control */

            PRINT_STR("  hwResId.%d.xcvrState.intfType",
                      IntfTypeToStr(xcvrIo->intfType));
            PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.index", xcvrIo->ioIdx);
            PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.basePin", xcvrIo->basePin);

            if (xcvrIo->intfType == INTF_TYPE_SFPP)
            {
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.modPresN.pin",
                          xcvrIo->u.sfppPin.modPresN);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.rxLos.pin",
                          xcvrIo->u.sfppPin.rxLos);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.txFault.pin",
                          xcvrIo->u.sfppPin.txFault);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.txDisable.pin",
                          xcvrIo->u.sfppPin.txDisable);
            }
            else if (xcvrIo->intfType == INTF_TYPE_QSFP)
            {
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.modPresN.pin",
                          xcvrIo->u.qsfpPin.modPresN);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.intrN.pin",
                          xcvrIo->u.qsfpPin.intrN);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.lpMode.pin",
                          xcvrIo->u.qsfpPin.lpMode);
                PRINT_VAL("  hwResId.%d.xcvrState.pcaIo.resetN.pin",
                          xcvrIo->u.qsfpPin.resetN);
            }

            for (led = 0 ; led < NUM_LED_PER_PORT ; led++)
            {
                /* Port LED */
                portLed = &hwResId->portLed[led];
                PRINT_LED_STR("  hwResId.%d.led.%d.type",
                              LedTypeToStr(portLed->type));

                if (portLed->type == LED_TYPE_PCA)
                {
                    PRINT_LED_VAL("  hwResId.%d.led.%d.pcaio.index",
                                  portLed->ioIdx);
                    for (subLed = 0 ; subLed < NUM_SUB_LED ; subLed++)
                    {
                        PRINT_SUBLED_VAL("  hwResId.%d.led.%d.%d.pcaio.pin",
                                         portLed->subLed[subLed].pin);
                        PRINT_SUBLED_STR(
                           "  hwResId.%d.led.%d.%d.usage",
                           GetStrBitMap( portLed->subLed[subLed].usage,
                                      ledUsageMap,
                                      FM_NENTRIES(ledUsageMap),
                                      tmpStr,
                                      sizeof(tmpStr) ) );

                    }
                }
            }
            FM_LOG_PRINT("\n");
        }
        else if (hwCfg.hwResId[cnt].type == HWRESOURCE_TYPE_VRM)
        {
            PRINT_STR("  hwResId.%d.vrm.busSelType",
                      BusSelTypeToStr(hwCfg.hwResId[cnt].vrm.busSelType));

            /* Mux select configuration */
            if (hwCfg.hwResId[cnt].vrm.busSelType == BUS_SEL_TYPE_PCA_MUX)
            {
                PRINT_VAL("  hwResId.%d.vrm.pcaMux.index",
                          hwCfg.hwResId[cnt].vrm.parentMuxIdx);
                PRINT_VAL("  hwResId.%d.vrm.pcaMux.value",
                          hwCfg.hwResId[cnt].vrm.parentMuxValue);
            }

            PRINT_VAL("  hwResId.%d.vrm.bus",
                      hwCfg.hwResId[cnt].vrm.bus);
            PRINT_VAL("  hwResId.%d.vrm.addr",
                      hwCfg.hwResId[cnt].vrm.addr);
            PRINT_STR("  hwResId.%d.vrm.model",
                      VrmModelToStr(hwCfg.hwResId[cnt].vrm.model));
        }
        else if (hwCfg.hwResId[cnt].type == HWRESOURCE_TYPE_PHY)
        {
        }
    }

}   /* end DumpConfig */




/*****************************************************************************/
/* SetupMuxPathRcrsv
 * \ingroup intPlatform
 *
 * \desc            Setup the given mux and any parent mux recursively
 *
 * \param[in]       muxIdx is the index to the pca mux structure.
 *
 * \param[in]       muxValue is the value to set for the given mux.
 *
 * \param[in]       disable indicates that other muxes sharing the same
 *                  parent mux must be disabled.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetupMuxPathRcrsv(fm_uint muxIdx,
                                   fm_uint muxValue,
                                   fm_bool disable)
{
    fm_i2cCfg *i2c;
    fm_pcaMux *pcaMux;
    fm_pcaMux *mux;
    fm_status  status;
    fm_byte    data[1];
    fm_uint    cnt;

    if ( muxIdx == UINT_NOT_USED)
    {
        return FM_OK;
    }

    pcaMux = &hwCfg.pcaMux[muxIdx];

    if (pcaMux->parentMuxIdx == muxIdx)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Parent mux index %d is the same\n", muxIdx);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    status = SetupMuxPathRcrsv(pcaMux->parentMuxIdx,
                               pcaMux->parentMuxValue,
                               FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    i2c = &hwCfg.i2c[pcaMux->bus];
    if (pcaMux->model == PCA_MUX_9541)
    {
        status = fmUtilPca9541TakeBusControl(i2c->handle,
                                             i2c->writeReadFunc,
                                             pcaMux->addr);
    }
    else
    {
        if (disable && pcaMux->parentMuxIdx != UINT_NOT_USED)
        {
            /* Disable other muxes sharing the same parent mux. */
            for (cnt = 0 ; cnt < hwCfg.numPcaMux; cnt++)
            {
                if (cnt == muxIdx)
                {
                    continue;
                }

                mux = &hwCfg.pcaMux[cnt];
                if ( mux->parentMuxIdx == pcaMux->parentMuxIdx &&
                     mux->parentMuxValue == pcaMux->parentMuxValue &&
                     mux->model != PCA_MUX_9541 )
                {
                    /* Disable that mux */
                    data[0] = 0;
                    if (hwCfg.debug & DBG_I2C)
                    {
                        FM_LOG_PRINT("Clear Mux 0x%x\n", mux->addr);
                    }
                    status = i2c->writeReadFunc(i2c->handle,
                                                mux->addr,
                                                data,
                                                1,
                                                0);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
                }
            }
        }

        data[0] = muxValue;
        if (hwCfg.debug & DBG_I2C)
        {
            FM_LOG_PRINT("Set Mux 0x%x => 0x%x\n", pcaMux->addr, muxValue);
        }
        status = i2c->writeReadFunc(i2c->handle, pcaMux->addr, data, 1, 0);
    }

    return status;

}   /* end SetupMuxPathRcrsv */




/*****************************************************************************/
/* SetupMuxPath
 * \ingroup intPlatform
 *
 * \desc            Setup the given mux and any parent mux.
 *
 * \param[in]       muxIdx is the index to the pca mux structure.
 *
 * \param[in]       muxValue is the value to set for the given mux.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetupMuxPath(fm_uint muxIdx, fm_uint muxValue)
{
    fm_status  status;
    fm_i2cCfg *i2c;
    fm_pcaMux *pcaMux;
    fm_byte    data[1];
    fm_uint    cnt;

    if (muxIdx == UINT_NOT_USED)
    {
        return FM_OK;
    }

    /* Disable all top level mux first */
    for (cnt = 0 ; cnt < hwCfg.numPcaMux; cnt++)
    {
        if (cnt == muxIdx)
        {
            continue;
        }

        pcaMux = &hwCfg.pcaMux[cnt];

        /* Only disable MUX on the same I2C bus */
        if (pcaMux->parentMuxIdx == UINT_NOT_USED &&
            pcaMux->bus == hwCfg.pcaMux[muxIdx].bus)
        {
            if (pcaMux->model != PCA_MUX_9541)
            {
                i2c = &hwCfg.i2c[pcaMux->bus];
                data[0] = 0;
                if (hwCfg.debug & DBG_I2C)
                {
                    FM_LOG_PRINT("Clear Mux 0x%x\n", pcaMux->addr);
                }
                status = i2c->writeReadFunc(i2c->handle, pcaMux->addr, data, 1, 0);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }
        }
    }

    return SetupMuxPathRcrsv(muxIdx, muxValue, TRUE);

} /* end SetupMuxPath */




/*****************************************************************************/
/* InitPca
 * \ingroup intPlatform
 *
 * \desc            Initialize PCA devices.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitPca(void)
{
    fm_status       status = FM_OK;
    fm_uint         cnt;
    fm_uint         led;
    fm_uint         subLed;
    fm_uint         offset;
    fm_uint         byteIdx;
    fm_uint         bitIdx;
    fm_xcvrIo *     xcvrIo;
    fm_pcaIoRegs *  ioRegs;
    fm_pcaIoDevice *ioDev;
    fm_i2cCfg *     i2c;
    fm_hwResId *    hwResId;
    fm_portLed *    portLed;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    for (cnt = 0 ; cnt < hwCfg.numPcaMux; cnt++)
    {
        switch (hwCfg.pcaMux[cnt].model)
        {
            case PCA_MUX_9541:
            case PCA_MUX_9545:
            case PCA_MUX_9546:
            case PCA_MUX_9548:
                hwCfg.pcaMux[cnt].initVal = 0x0;
            break;
            case PCA_MUX_UNKNOWN:
            break;
        }
    }

    /* Write to hardware */
    /* Do we need to init mux? */

    /* Initialize PCA IO cached registers */
    for (cnt = 0 ; cnt < hwCfg.numPcaIo; cnt++)
    {
        ioDev = &hwCfg.pcaIo[cnt].dev;
        i2c = &hwCfg.i2c[ioDev->bus];

        ioDev->fd = i2c->handle;
        ioDev->i2cBlockSupported = i2c->i2cBlockSupported;
        ioDev->func = i2c->writeReadFunc;

        if (ioDev->model == PCA_IO_9634 || ioDev->model == PCA_IO_9635)
        {
            /* Mode1 register: Normal mode, auto-increment enabled,
                               all subaddresses and AllCall address disabled */
            ioDev->ledRegs.mode[0] = 0x80;

            /* Mode2 register: Blinking, totem-pole structure */
            ioDev->ledRegs.mode[1] = 0x25;

            /* GRPPWM: Set 50% duty cycle (range 00h-FFh) */
            ioDev->ledRegs.group[0] = 0x7f;

            /* GRPFREQ: Set blink period */
            ioDev->ledRegs.group[1] = ioDev->ledBlinkPeriod;

            for (led = 0 ; led < (ioDev->devCap.numBits) ; led++)
            {
                /* Set LED output brightness */
                ioDev->ledRegs.pwm[led] = ioDev->ledBrightness;
            }

            for (led = 0 ; led < (ioDev->devCap.numBytes) ; led++)
            {
                /* All LEDs OFF */
                ioDev->ledRegs.ledout[led] = 0;
            }

            ioDev->ledRegs.ledOff   = FM_PCA_LEDOUT_OFF;
            ioDev->ledRegs.ledOn    = FM_PCA_LEDOUT_ON;
            ioDev->ledRegs.ledBlink = FM_PCA_LEDOUT_BLINK;
        }
        else if (ioDev->model == PCA_IO_9551)
        {
            /* PSC0/PSC1: Set blink period */
            ioDev->ledRegs.psc0 = ioDev->ledBlinkPeriod;
            ioDev->ledRegs.psc1 = ioDev->ledBlinkPeriod;

            /* PWM0/PWM1: duty cycle = (256 - PWM)/256 */
            /* Set to 50% --> 0.5 = (256 - 128)/256    */
            ioDev->ledRegs.pwm0 = 128;
            ioDev->ledRegs.pwm1 = 128;

            /* All LEDs OFF */
            ioDev->ledRegs.ledout[0] = 0x55;
            ioDev->ledRegs.ledout[1] = 0x55;

            ioDev->ledRegs.ledOff   = FM_PCA_LS_OFF;
            ioDev->ledRegs.ledOn    = FM_PCA_LS_ON;
            ioDev->ledRegs.ledBlink = FM_PCA_LS_BLINK0;
        }
        else
        {
            for (byteIdx = 0 ; byteIdx < (ioDev->devCap.numBytes) ; byteIdx++)
            {
                /* Default all pins as inputs */
                ioDev->cachedRegs.ioc[byteIdx] = 0xFF;

                /* Enable all interrupts by default */
                ioDev->cachedRegs.intr[byteIdx] = 0x00;
            }
        }
    }

    /* Initialize Transceiver and LED pins */
    for (cnt = 0 ; cnt < hwCfg.numResId; cnt++)
    {
        hwResId = &hwCfg.hwResId[cnt];
        if (hwResId->xcvrI2cBusSel.busSelType == BUS_SEL_TYPE_PCA_IO)
        {
            ioRegs = &hwCfg.pcaIo[hwResId->xcvrI2cBusSel.ioIdx].dev.cachedRegs;
            offset = hwResId->xcvrI2cBusSel.ioPin;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_OUTPUT);
                SET_BIT(ioRegs->output[byteIdx], bitIdx,
                    !hwResId->xcvrI2cBusSel.ioPinPolarity);
            }
        }

        /* Transceiver status/control */
        xcvrIo = &hwResId->xcvrStateIo;
        ioRegs = &hwCfg.pcaIo[xcvrIo->ioIdx].dev.cachedRegs;

        if (xcvrIo->intfType == INTF_TYPE_SFPP)
        {
            offset = xcvrIo->u.sfppPin.modPresN;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_INPUT);
            }

            offset = xcvrIo->u.sfppPin.rxLos;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_INPUT);
            }

            offset = xcvrIo->u.sfppPin.txFault;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_INPUT);
            }

            offset = xcvrIo->u.sfppPin.txDisable;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_OUTPUT);
            }
        }
        else if (xcvrIo->intfType == INTF_TYPE_QSFP)
        {
            offset = xcvrIo->u.qsfpPin.modPresN;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_INPUT);
            }

            offset = xcvrIo->u.qsfpPin.intrN;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_INPUT);
            }

            offset = xcvrIo->u.qsfpPin.lpMode;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                /* Default to LPMODE off */
                SET_BIT(ioRegs->output[byteIdx], bitIdx, 0);
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_OUTPUT);
            }

            offset = xcvrIo->u.qsfpPin.resetN;
            byteIdx = offset / 8;
            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
            {
                bitIdx  = offset % 8;
                /* Default to ENABLED */
                SET_BIT(ioRegs->output[byteIdx], bitIdx, 1);
                SET_BIT(ioRegs->ioc[byteIdx], bitIdx, IOC_OUTPUT);
            }
        }

        /* Port LED */
        for (led = 0 ; led < NUM_LED_PER_PORT ; led++)
        {
            portLed = &hwResId->portLed[led];

            if (portLed->type == LED_TYPE_PCA)
            {
                ioDev = &hwCfg.pcaIo[portLed->ioIdx].dev;
                if ( !(IsPcaIoLedDriver(ioDev->model)) )
                {
                    for (subLed = 0 ; subLed < NUM_SUB_LED ; subLed++)
                    {
                        offset  = portLed->subLed[subLed].pin;
                        if (offset != UINT_NOT_USED)
                        {
                            byteIdx = offset / 8;
                            bitIdx  = offset % 8;
                            if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
                            {
                                SET_BIT(ioDev->cachedRegs.ioc[byteIdx],
                                        bitIdx,
                                        IOC_OUTPUT);
                            }
                        }
                    }
                }
            }

        }   /* end for (led = 0 ; led < NUM_LED_PER_PORT ; led++) */

    }   /* end for (cnt = 0 ; cnt < hwCfg.numResId; cnt++) */


    /* Init the PCA IO hardware registers for devices connected to I2C bus
       other than switch i2c bus, for example /dev/i2c-0. The devices connected
       to a switch i2c bus will be initialize later in InitSwitchPcaDevice */
    for (cnt = 0 ; cnt < hwCfg.numPcaIo; cnt++)
    {
        i2c = &hwCfg.i2c[hwCfg.pcaIo[cnt].dev.bus];

        if (i2c->isSwitchMaster)
        {
            if (hwCfg.debug & DBG_I2C)
            {
                FM_LOG_PRINT("skip pcaio=%d bus=%d\n",
                             cnt,
                             hwCfg.pcaIo[cnt].dev.bus);
            }
            continue;
        }

        status = SetupMuxPath(hwCfg.pcaIo[cnt].parentMuxIdx,
                              hwCfg.pcaIo[cnt].parentMuxValue);
        if (status)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Error to init PCA mux\n");
            break;
        }

        status = fmUtilPcaIoInit(&hwCfg.pcaIo[cnt].dev);
        if (status)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Error to init PCA IO (%d) device\n", cnt);
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end InitPca */



/*****************************************************************************/
/* InitSwitchPcaDevice
 * \ingroup intPlatform
 *
 * \desc            Initialize PCA devices attached the given switch.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitSwitchPcaDevice(fm_int sw)
{
    fm_status       status = FM_OK;
    fm_uint         cnt;
    fm_int          bus;
    fm_i2cCfg *     i2c;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    /* Init the PCA IO hardware registers for devices connected to this switch
       i2c bus. */
    for (cnt = 0 ; cnt < hwCfg.numPcaIo; cnt++)
    {
        bus = hwCfg.pcaIo[cnt].dev.bus;
        i2c = &hwCfg.i2c[bus];

        if (i2c->isSwitchMaster && bus == sw )
        {
            status = SetupMuxPath(hwCfg.pcaIo[cnt].parentMuxIdx,
                                  hwCfg.pcaIo[cnt].parentMuxValue);
            if (status)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Error to init PCA mux\n");
                break;
            }

            status = fmUtilPcaIoInit(&hwCfg.pcaIo[cnt].dev);
            if (status)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Error to init PCA IO (%d) device\n", cnt);
                break;
            }
        }
        else if (hwCfg.debug & DBG_I2C)
        {
            FM_LOG_PRINT("skip pcaio=%d bus=%d\n", cnt, bus);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end InitSwitchPcaDevice */




/*****************************************************************************/
/* DumpPca
 * \ingroup intPlatform
 *
 * \desc            Dump PCA devices.
 *
 * \return          NONE.
 *
 *****************************************************************************/
static void DumpPca(void)
{
    fm_uint cnt;

    for (cnt = 0 ; cnt < hwCfg.numPcaIo; cnt++)
    {
        FM_LOG_PRINT("##### PCA IO #%d at 0x%x #####\n",
                     cnt,
                     hwCfg.pcaIo[cnt].dev.addr);

        fmUtilPcaIoDump(&hwCfg.pcaIo[cnt].dev);

        FM_LOG_PRINT("\n");
    }

}   /* end DumpPca */




/*****************************************************************************/
/* GetVrmVoltageInt
 * \ingroup platformLib
 *
 * \desc            Get the actual voltage from the voltage regulator.
 *
 * \note            Assumes that the caller has taken the file locks.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id associated
 *                  with the VRM.
 *                  api.platform.config.switch.n.vrm.hwResourceId.
 *
 *                  The sub-channel number is embedded in the resourceId.
 *
 * \param[in]       mVolt is the caller allocated storage where the
 *                  function will placeis the voltage in milli-volt.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status GetVrmVoltageInt(fm_int     sw,
                           fm_uint32  hwResourceId,
                           fm_uint32 *mVolt)
{
    fm_status    status = FM_OK;
    fm_i2cCfg *  i2c;
    fm_hwResId * hwResId;
    fm_vrmI2C *  vrmI2c;
    fm_byte      data[3];
    fm_uint      channel;
    fm_uint      reg;
    fm_byte      page;
    fm_int       e;
    expman       s;

    FM_NOT_USED(sw);

    if ( HW_RESOURCE_ID_TO_IDX(hwResourceId) >= FM_NUM_HW_RES_ID )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    hwResId = &hwCfg.hwResId[HW_RESOURCE_ID_TO_IDX(hwResourceId)];
    if (hwResId == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    i2c = &hwCfg.i2c[hwResId->vrm.bus];
    if (i2c == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    if ( i2c->isSwitchMaster && i2c->chipVersion == CHIP_VERSION_A0 )
    {
        /* Because of the i2c write-read issue with FM10000 A0, voltage
           scaling is not supported. */
        FM_LOG_PRINT("Voltage scaling not supported on FM10000 A0 version\n");
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    vrmI2c = &hwResId->vrm;
    if (vrmI2c == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    channel = HW_RESOURCE_ID_TO_VRMSUBCHAN(hwResourceId);

    if (hwResId->type != HWRESOURCE_TYPE_VRM)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = SetupMuxPath(hwResId->vrm.parentMuxIdx,
                          hwResId->vrm.parentMuxValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (hwResId->vrm.model && (vrmI2c->model == VRM_TPS40425))
    {
        if (channel == 0)
        {
            page = 0;
        }
        else if (channel == 1)
        {
            page = 1;
        }
        else
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Select Page */
        data[0] = 0x00;
        data[1] = page;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Get READ_VOUT (0x8b)*/
        data[0] = 0x8b;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 1, 2);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        s.value = (data[1] << 8) | data[0];

        e = abs(s.bits.e);

        /* Unit is 1.953 mV (from VOUT_MODE instead of e)*/
        *mVolt = ((s.bits.m) * 1953) / 1000;
    }
    else if (hwResId->vrm.model && (vrmI2c->model == VRM_PX8847))
    {
        if (channel == 0)
        {
            page = 0x21;
            reg = 0x8f;
        }
        else if (channel == 1)
        {
            page = 0x21;
            reg = 0x9b;
        }
        else
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Select Page */
        data[0] = 0x00;
        data[1] = page;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Read Output Voltage */
        data[0] = reg;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 1, 2); /* password1 */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Unit is 0.625mv (from spec) */
        *mVolt = (((data[1] << 8) | data[0]) * 625) / 1000;

        if (page)
        {
            /* Set back to page 0 */
            data[0] = 0x00;
            data[1] = 0x00;

            status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

ABORT:
    return status;

}   /* end GetVrmVoltageInt */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformLibInit
 * \ingroup platformLib
 *
 * \desc            Initializes platform library.
 *                  This will be called per process in shared process mode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibInit(void)
{
    fm_i2cCfg *i2c;
    fm_status  status;
    fm_int     bus;
    fm_uint    idx;
    fm_char    tmpStr[32];
    fm_portLed *portLed;
    fm_uint     led;
    fm_uint     subLed;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_PLATFORM);

    memset(&hwCfg, 0, sizeof(hwCfg));

    hwCfg.fileLock = -1;

    for ( idx = 0 ; idx < NUM_I2C_BUS ; idx++ )
    {
        hwCfg.i2c[idx].i2cWrRdEn = TRUE;
    }

    for (idx = 0 ; idx < NUM_PCA_MUX ; idx++)
    {
        hwCfg.pcaMux[idx].parentMuxIdx = UINT_NOT_USED;
    }

    for (idx = 0 ; idx < NUM_PCA_IO ; idx++)
    {
        hwCfg.pcaIo[idx].parentMuxIdx = UINT_NOT_USED;
    }

    for (idx = 0 ; idx < FM_NUM_HW_RES_ID ; idx++)
    {
        hwCfg.hwResId[idx].xcvrI2cBusSel.parentMuxIdx =
            UINT_NOT_USED;
        hwCfg.hwResId[idx].phy.parentMuxIdx = UINT_NOT_USED;
        hwCfg.hwResId[idx].vrm.parentMuxIdx = UINT_NOT_USED;
        for (led = 0 ; led < NUM_LED_PER_PORT ; led++)
        {
            portLed = &hwCfg.hwResId[idx].portLed[led];
            portLed->ioIdx = UINT_NOT_USED;
            for (subLed = 0 ; subLed < NUM_SUB_LED ; subLed++)
            {
                portLed->subLed[subLed].pin   = UINT_NOT_USED;
                portLed->subLed[subLed].usage = UINT_NOT_USED;
            }
        }
    }

    hwCfg.defSfppPat.modPresN = UINT_NOT_USED;
    hwCfg.defSfppPat.rxLos = UINT_NOT_USED;
    hwCfg.defSfppPat.txDisable = UINT_NOT_USED;
    hwCfg.defSfppPat.txFault = UINT_NOT_USED;
    hwCfg.defQsfpPat.modPresN = UINT_NOT_USED;
    hwCfg.defQsfpPat.intrN = UINT_NOT_USED;
    hwCfg.defQsfpPat.lpMode = UINT_NOT_USED;
    hwCfg.defQsfpPat.resetN = UINT_NOT_USED;

    status = LoadConfig();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (strlen(hwCfg.i2c[0].devName) == 0)
    {
        /* Bus 0 is not defined, do we have another bus number (1..NUM_I2C_BUS)
           defined. */
        for ( idx = 1 ; idx < NUM_I2C_BUS ; idx++ )
        {
            if ( strlen(hwCfg.i2c[idx].devName) )
            {
                /* Found another I2C bus defined. This is the SWAG case,
                   where the swag switch is sw 0. Set bus 0 devName to "swag"
                   for fmPlatformLibInit. */
                FM_STRCPY_S(hwCfg.i2c[0].devName,
                    sizeof(hwCfg.i2c[0].devName),
                    "swag");
                break;
            }
        }

        if (idx >= NUM_I2C_BUS)
        {
            /* No other I2C bus found, then default bus 0 to "switchI2C" */
            FM_STRCPY_S(hwCfg.i2c[0].devName,
                sizeof(hwCfg.i2c[0].devName),
                "switchI2C");
        }
    }

    /* Update the device capabilities */
    for (idx = 0 ; idx < hwCfg.numPcaIo; idx++)
    {
        status = fmUtilPcaIoGetCap(hwCfg.pcaIo[idx].dev.model,
                                   &hwCfg.pcaIo[idx].dev.devCap);
        if (status)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "%s: Unable to get device capabilities.\n",
                PcaIoModelToStr(hwCfg.pcaIo[idx].dev.model));
            return status;
        }
    }

    if (hwCfg.debug & DBG_DUMP_CFG)
    {
        DumpConfig();
    }

    if (strlen(hwCfg.fileLockName) > 0)
    {
        hwCfg.fileLock = 
            open(hwCfg.fileLockName, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);

        if (hwCfg.fileLock < 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to open filelock name: [%s].\n",
                         hwCfg.fileLockName);
        }
    }

    for (bus = 0 ; bus < NUM_I2C_BUS ; bus++)
    {
        i2c = &hwCfg.i2c[bus];
        i2c->handle = 0;
        i2c->i2cBlockSupported = 0;
        i2c->isSwitchMaster = 0;

        /* Subsequent bus could be unused */
        if ((bus > 0) && !strlen(i2c->devName))
        {
            continue;
        }

        if (strstr(i2c->devName, "switchI2C") != NULL)
        {
            i2c->writeReadFunc = SwitchI2cWriteRead;
            i2c->setDebugFunc  = NULL;
            i2c->handle = bus; /* bus is the same as switch number */
            i2c->i2cBlockSupported = 0;
            i2c->isSwitchMaster = 1;
        }
        else if (strstr(i2c->devName, "/dev/i2c") != NULL)
        {
            status = fmPlatformSMBusI2cInit(i2c->devName, (int *)&i2c->handle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            i2c->writeReadFunc = fmPlatformSMBusI2cWriteReadHandle;
            i2c->setDebugFunc  = NULL;

            fmPlatformSMBusIsI2cBlockSupported((int )i2c->handle,
                                               &i2c->i2cBlockSupported);
        }
        else if (strstr(i2c->devName, "/dev/ipmi") != NULL)
        {
            status = fmPlatformIpmiInit(i2c->devName, (int *)&i2c->handle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            i2c->writeReadFunc = fmPlatformIpmiWriteReadHandle;
            i2c->setDebugFunc  = fmPlatformIpmiSetDebug;
        }
#ifdef FM_SUPPORT_FTDI_RRC
        else if (strstr(i2c->devName, "ttyUSB") != NULL)
        {
            status = fmUtilFtdiRrcInit(i2c->devName, &i2c->handle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            i2c->writeReadFunc = fmUtilFtdiRrcI2cWriteRead;
            i2c->setDebugFunc  = fmUtilFtdiRrcSetDebug;
        }
#endif
        else
        {
            if ((bus == 0) && strstr(i2c->devName, "swag") != NULL)
            {
               /* In SWAG, there is no bus associated to bus 0 (switch 0). */
               continue;
            }

            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unknown device name: [%s]\n",
                         i2c->devName);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }

    status = InitPca();

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibInit */




/*****************************************************************************/
/* fmPlatformLibInitSwitch
 * \ingroup platformLib
 *
 * \desc            Performs hardware initialization for the given switch.
 *                  This will be only called once from the first process.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibInitSwitch(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    status = InitSwitchPcaDevice(sw);

    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibInitSwitch */




/*****************************************************************************/
/* fmPlatformLibPostInit
 * \ingroup platformLib
 *
 * \desc            Called by the switch post init code
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       upStatus is the current switch state
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibPostInit(fm_int sw,
                                fm_int upStatus)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d upStatus=%d\n",
                 sw, upStatus);

    FM_NOT_USED(sw);
    FM_NOT_USED(upStatus);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLibPostInit */




/*****************************************************************************/
/* fmPlatformLibDebugDump
 * \ingroup platformLib
 *
 * \desc            Sends debug commands to platform library.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id to identify
 *                  a port, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibDebugDump(fm_int    sw,
                                 fm_uint32 hwResourceId,
                                 fm_text   action,
                                 fm_text   args)
{
    fm_status status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d resId=%d action=%s\n",
                 sw, hwResourceId, action);

    FM_NOT_USED(sw);
    FM_NOT_USED(hwResourceId);
    FM_NOT_USED(args);

    if (strncasecmp(action, "dumpPca", 7) == 0)
    {
        status = TakeLock();
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        DumpPca();
        DropLock();
    }
    else if (strncasecmp(action, "dumpConfig", 10) == 0)
    {
        DumpConfig();
    }
    else if (strncasecmp(action, "i2cDebug", 8) == 0)
    {
        if (strncasecmp(args, "on", 2) == 0)
            hwCfg.debug |= DBG_I2C;
        else
            hwCfg.debug &= ~DBG_I2C;
    }
    else if (strncasecmp(action, "i2cMuxDebug", 10) == 0)
    {
        if (strncasecmp(args, "on", 2) == 0)
            hwCfg.debug |= DBG_I2C_MUX;
        else
            hwCfg.debug &= ~DBG_I2C_MUX;
    }
    else if (strncasecmp(action, "skipSelBus", 10) == 0)
    {
        if (strncasecmp(args, "on", 2) == 0)
            hwCfg.debug |= DBG_SKIP_SEL_BUS;
        else
            hwCfg.debug &= ~DBG_SKIP_SEL_BUS;
    }
    else if (strncasecmp(action, "forceModPres", 12) == 0)
    {
        if (strncasecmp(args, "on", 2) == 0)
            hwCfg.debug |= DBG_FORCE_MODPRES;
        else
            hwCfg.debug &= ~DBG_FORCE_MODPRES;
    }
    else if (strncasecmp(action, "help", 4) == 0)
    {
        printf("Available Commands:\n");
        printf("    dumpPca             - Dump PCA registers\n");
        printf("    dumpConfig          - Dump configuration\n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* fmPlatformLibDebugDump */




/*****************************************************************************/
/* fmPlatformLibResetSwitch
 * \ingroup platformLib
 *
 * \desc            Controls switch reset.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       reset indicates whether to place switch in reset or not.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibResetSwitch(fm_int sw, fm_bool reset)
{
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d reset=%d\n", sw, reset);

    FM_NOT_USED(sw);
    FM_NOT_USED(reset);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLibResetSwitch */




/*****************************************************************************/
/* fmPlatformLibSelectBus
 * \ingroup platformLib
 *
 * \desc            Selects specified bus before accessing the hardware.
 *                  Locking is handled by the caller.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       busType is the bus type. See fm_platBusType.
 *
 * \param[in]       hwResourceId is the hardware resource id to identify
 *                  a port, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibSelectBus(fm_int    sw,
                                 fm_int    busType,
                                 fm_uint32 hwResourceId)
{
    fm_status   status = FM_OK;
    fm_uint     hwIdx;
    fm_uint     idx;
    fm_byte     data[2];
    fm_hwResId *hwResId;
    fm_uint     muxIdx;
    fm_uint     muxValue;
    fm_uint     ioIdx;
    fm_uint     lastIoIdx;
    fm_uint     offset;
    fm_uint     byteIdx;
    fm_uint     bitIdx;
    fm_uint     cnt;
    fm_busSel  *xcvrI2cBus;
    fm_uint     lastId;
    fm_uint     id;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d busType=%d resId=%d\n",
                 sw, busType, hwResourceId);

    if (hwCfg.debug & DBG_SKIP_SEL_BUS)
    {
        FM_LOG_PRINT("Skipping busType=%d resId=%d\n",
                     busType, hwResourceId);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    FM_NOT_USED(sw);

    /* Default to 0 */
    hwCfg.selectedBus = 0;

    if ( busType == FM_PLAT_BUS_NUMBER )
    {
        /* The hw ID contains the bus number to select. */
        status = ValidateBusNumber(hwResourceId);

        if ( status == FM_OK )
        {
            hwCfg.selectedBus = hwResourceId;
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Invalid bus number %d \n",
                         hwResourceId);
        }
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    hwIdx = HW_RESOURCE_ID_TO_IDX(hwResourceId);

    if (hwIdx >= hwCfg.numResId)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "hwResourceId %u out of range %u\n",
                     hwIdx,
                     hwCfg.numResId);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    hwResId = &hwCfg.hwResId[hwIdx];

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    muxIdx = UINT_NOT_USED;

    if (busType == FM_PLAT_BUS_XCVR_STATE)
    {
        idx = hwResId->xcvrStateIo.ioIdx;
        muxIdx = hwCfg.pcaIo[idx].parentMuxIdx;
        muxValue = hwCfg.pcaIo[idx].parentMuxValue;

        if (muxIdx == UINT_NOT_USED)
        {
            /* Means the IO is not behind a MUX and located directly on the
               main I2C branch. So use the IO bus number for bus selection */
            hwCfg.selectedBus = hwCfg.pcaIo[idx].dev.bus;
        }
    }
    else if (busType == FM_PLAT_BUS_XCVR_EEPROM)
    {
        if (hwResId->xcvrI2cBusSel.busSelType == BUS_SEL_TYPE_PCA_MUX)
        {
            muxIdx = hwResId->xcvrI2cBusSel.parentMuxIdx;
            muxValue = hwResId->xcvrI2cBusSel.parentMuxValue;
        }
        else if (hwResId->xcvrI2cBusSel.busSelType == BUS_SEL_TYPE_PCA_IO)
        {
            idx = hwResId->xcvrI2cBusSel.ioIdx;
            muxIdx = hwCfg.pcaIo[idx].parentMuxIdx;
            muxValue = hwCfg.pcaIo[idx].parentMuxValue;

            if (muxIdx == UINT_NOT_USED)
            {
                /* Means the IO is not behind a MUX and located directly on the
                   main I2C branch. So use the IO bus number for bus selection */
                hwCfg.selectedBus = hwCfg.pcaIo[idx].dev.bus;
            }

            /* Enable I2C select bit only on the one selected
             * and disable the rest */
            lastId = UINT_NOT_USED;
            lastIoIdx = 0;
            for (cnt = 0 ; cnt < hwCfg.numResId; cnt++)
            {
                if (hwCfg.hwResId[cnt].type == HWRESOURCE_TYPE_PORT)
                {
                    xcvrI2cBus = &hwCfg.hwResId[cnt].xcvrI2cBusSel;

                    if (xcvrI2cBus->busSelType == BUS_SEL_TYPE_PCA_IO)
                    {
                        offset = xcvrI2cBus->ioPin;
                        byteIdx = offset / 8;
                        if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
                        {
                            bitIdx  = offset % 8;
                            ioIdx = xcvrI2cBus->ioIdx;
                            SET_BIT(hwCfg.pcaIo[ioIdx].dev.cachedRegs.output[byteIdx],
                                    bitIdx,
                                    (cnt == hwIdx) ? xcvrI2cBus->ioPinPolarity :
                                        !xcvrI2cBus->ioPinPolarity);

                            id = byteIdx | (ioIdx << 8);
                            /* Only update HW only when change to different byte */
                            if ((lastId != UINT_NOT_USED && (id != lastId)) ||
                                 (cnt == (hwCfg.numResId - 1)))
                            {
                                /* because the pcaIos could be on different mux paths*/
                                /* we need to set the mux path here before we try to */
                                /* set the IO pins state */
                                status = SetupMuxPath(hwCfg.pcaIo[lastIoIdx].parentMuxIdx,
                                                      hwCfg.pcaIo[lastIoIdx].parentMuxValue);

                                status = fmUtilPcaIoWriteRegs(&hwCfg.pcaIo[lastIoIdx].dev,
                                                              PCA_IO_REG_TYPE_OUTPUT,
                                                              byteIdx,
                                                              1);
                            }
                            lastIoIdx = ioIdx;
                            lastId = id;
                        }
                    }
                }
            }
        }
    }
    else if (busType == FM_PLAT_BUS_PHY)
    {
        if (hwResId->phy.busSelType == BUS_SEL_TYPE_PCA_MUX)
        {
            muxIdx = hwResId->phy.parentMuxIdx;
            muxValue = hwResId->phy.parentMuxValue;

            if (muxIdx == UINT_NOT_USED)
            {
                /* Means the PHY is not behind a MUX and located directly on
                   the main I2C branch. So use the PHY bus number for bus
                   selection */
                hwCfg.selectedBus = hwResId->phy.bus;
            }
        }
        else if (hwResId->phy.busSelType == BUS_SEL_TYPE_PCA_IO)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Need support\n");
        }
    }
    else if (busType == FM_PLAT_BUS_DISABLED)
    {
        ; /* Maybe put all the mux into disabled */
    }

    if ( muxIdx != UINT_NOT_USED && muxIdx < NUM_PCA_MUX )
    {
        hwCfg.selectedBus = hwCfg.pcaMux[muxIdx].bus;
        status = SetupMuxPath(muxIdx, muxValue);
    }

    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibSelectBus */




/*****************************************************************************/
/* fmPlatformLibI2cWriteRead
 * \ingroup platformLib
 *
 * \desc            Performs I2C read write commands. For each type of access,
 *                  SelectBus function will be called first before multiple
 *                  calls of this function are invoked. Locking is handled
 *                  by the caller.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       i2cAddr is the I2C address of the device.
 *
 * \param[in,out]   data points to caller-allocated storage where data is written
 *                  to the device or data is read from the device.
 *
 * \param[in]       writeLen is the number of bytes to write.
 *
 * \param[in]       readLen is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibI2cWriteRead(fm_int   sw,
                                    fm_int   i2cAddr,
                                    fm_byte *data,
                                    fm_int   writeLen,
                                    fm_int   readLen)
{
    fm_status  status;
    fm_i2cCfg *i2c;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d i2cAddr=0x%x writeLen=%d readLen=%d\n",
                 sw, i2cAddr, writeLen, readLen);

    if (hwCfg.selectedBus < NUM_I2C_BUS)
    {
        i2c = &hwCfg.i2c[hwCfg.selectedBus];

        status = TakeLock();
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        status = i2c->writeReadFunc(i2c->handle,
                                    i2cAddr,
                                    data,
                                    writeLen,
                                    readLen);
        DropLock();
    }
    else
    {
        status = FM_ERR_UNINITIALIZED;
    }


    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibI2cWriteRead */




/*****************************************************************************/
/* fmPlatformLibGetPortXcvrState
 * \ingroup platformLib
 *
 * \desc            Returns the port transceiver module state.
 *                  NOTE: The caller can only call SelectBus function for
 *                  the first port in the list before calling this function.
 *                  This function must handle bus select if there is a change
 *                  in bus select for subsequent ports.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceIdList is the hardware resource id associated with
 *                  a port, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \param[in]       numPorts is the size of hwResourceIdList.
 *
 * \param[out]      xcvrStateValid points to caller-allocated storage where
 *                  which valid tranceiver state is stored. See platform
 *                  transceiver (FM_PLAT_XCVR_XXX) state definitions.
 *
 * \param[out]      xcvrState points to caller-allocated storage where
 *                  corresponding transceiver state is stored.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibGetPortXcvrState(fm_int     sw,
                                        fm_uint32 *hwResourceIdList,
                                        fm_int     numPorts,
                                        fm_uint32 *xcvrStateValid,
                                        fm_uint32 *xcvrState)
{
    fm_xcvrIo *     xcvrIo;
    fm_pcaIoDevice *ioDev;
    fm_status       status = FM_OK;
    fm_uint         cnt;
    fm_uint         hwIdx;
    fm_uint         offset;
    fm_uint         byteIdx;
    fm_uint         bitIdx;
    fm_bool         regSynced[NUM_PCA_IO];

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d numPorts=0x%x\n", sw, numPorts);

    if (hwCfg.debug & DBG_FORCE_MODPRES)
    {
        for (cnt = 0 ; cnt < (fm_uint)numPorts ; cnt++)
        {
            xcvrStateValid[cnt] = 0;
            xcvrState[cnt] = 0;

            xcvrStateValid[cnt] |= FM_PLAT_XCVR_PRESENT;
            xcvrState[cnt]      |= FM_PLAT_XCVR_PRESENT;
            xcvrStateValid[cnt] |= FM_PLAT_XCVR_ENABLE;
            xcvrState[cnt]      |= FM_PLAT_XCVR_ENABLE;
#if 0
            if (xcvrIo->intfType == INTF_TYPE_SFPP)
            {
                xcvrState[cnt] |= FM_PLAT_XCVR_RXLOS;
                xcvrState[cnt] |= FM_PLAT_XCVR_TXFAULT;
            }
            else if (xcvrIo->intfType == INTF_TYPE_QSFP)
            {
                xcvrState[cnt] |= FM_PLAT_XCVR_INTR;
                xcvrState[cnt] |= FM_PLAT_XCVR_LPMODE;
            }
#endif
        }
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    /* Mark all the PCA IO input regs out of date for the code
     * below to update the input registers only when needed */
    FM_CLEAR(regSynced);

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    for (cnt = 0 ; cnt < (fm_uint)numPorts ; cnt++)
    {
        hwIdx = HW_RESOURCE_ID_TO_IDX(hwResourceIdList[cnt]);
        if (hwIdx >= hwCfg.numResId)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "hwResourceId %u out of range %u\n",
                         hwIdx,
                         hwCfg.numResId);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        xcvrStateValid[cnt] = 0;
        xcvrState[cnt] = 0;

        /* The caller already selected the bus for the first port,
         * so select bus is required for subsequent ports only.  */
        if (cnt > 0)
        {
            status = fmPlatformLibSelectBus(sw, FM_PLAT_BUS_XCVR_STATE, hwIdx);

            if (status != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                             "Failed to select xcvr bus (hwResourceId %u)\n",
                             hwIdx);
                continue;
            }
        }

        /* Transceiver status/control */
        xcvrIo = &hwCfg.hwResId[hwIdx].xcvrStateIo;
        ioDev  = &hwCfg.pcaIo[xcvrIo->ioIdx].dev;

        /* Only read from HW when input registers are not in sync */
        if (regSynced[xcvrIo->ioIdx] == FALSE)
        {
            /* Read all the input registers from that PCA IO */
            status = fmUtilPcaIoReadRegs(ioDev,
                                         PCA_IO_REG_TYPE_INPUT,
                                         0,
                                         ioDev->devCap.numBytes);
            if (status != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                             "Failed to read PCA IO regs (hwResourceId %u)\n",
                             hwIdx);
                continue;
            }

            /* Indicates the INPUT reg has been read */
            regSynced[xcvrIo->ioIdx] = TRUE;
        }

        if (xcvrIo->intfType == INTF_TYPE_SFPP)
        {
            offset = xcvrIo->u.sfppPin.modPresN;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_PRESENT;
                if (!GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_PRESENT;
                }
            }

            offset = xcvrIo->u.sfppPin.rxLos;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_RXLOS;
                if (GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_RXLOS;
                }
            }

            offset = xcvrIo->u.sfppPin.txFault;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_TXFAULT;
                if (GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_TXFAULT;
                }
            }

            offset = xcvrIo->u.sfppPin.txDisable;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_ENABLE;
                if (!GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_ENABLE;
                }
            }
        }
        else if (xcvrIo->intfType == INTF_TYPE_QSFP)
        {
            offset = xcvrIo->u.qsfpPin.modPresN;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_PRESENT;
                if (!GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_PRESENT;
                }
            }

            offset = xcvrIo->u.qsfpPin.intrN;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_INTR;
                if (!GET_BIT(ioDev->cachedRegs.input[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_INTR;
                }
            }

            /* Reading output cached reg for lpMode and enable bits because of
               input/output mismatch seen with specific cables
               (Vendor : TE Connectivity : PN 2231368-1).*/
            offset = xcvrIo->u.qsfpPin.lpMode;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_LPMODE;
                if (GET_BIT(ioDev->cachedRegs.output[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_LPMODE;
                }
            }

            offset = xcvrIo->u.qsfpPin.resetN;
            if ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS))
            {
                byteIdx = offset / 8;
                bitIdx  = offset % 8;
                xcvrStateValid[cnt] |= FM_PLAT_XCVR_ENABLE;
                if (GET_BIT(ioDev->cachedRegs.output[byteIdx], bitIdx))
                {
                    xcvrState[cnt] |= FM_PLAT_XCVR_ENABLE;
                }
            }
        }
    }

ABORT:
    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibGetPortXcvrState */




/*****************************************************************************/
/* fmPlatformLibSetPortXcvrState
 * \ingroup platformLib
 *
 * \desc            Set port transceiver module state.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id associated with
 *                  a port, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \param[out]      xcvrStateValid is a bitmap indicating which tranceiver
 *                  state to be set. See platform transceiver state
 *                  (FM_PLAT_XCVR_XXX) definitions.
 *
 * \param[out]      xcvrState is the corresponding transceiver state to be set.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibSetPortXcvrState(fm_int    sw,
                                        fm_uint32 hwResourceId,
                                        fm_uint32 xcvrStateValid,
                                        fm_uint32 xcvrState)
{
    fm_xcvrIo *     xcvrIo;
    fm_pcaIoDevice *ioDev;
    fm_status       status = FM_OK;
    fm_uint         hwIdx;
    fm_uint         offset;
    fm_uint         byteIdx;
    fm_uint         bitIdx;
    fm_bool         isRead;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d hwResId=%d xcvrStateValid=%x xcvrState=%x\n",
                 sw, hwResourceId, xcvrStateValid, xcvrState);

    FM_NOT_USED(sw);

    hwIdx = HW_RESOURCE_ID_TO_IDX(hwResourceId);
    if (hwIdx >= hwCfg.numResId)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "hwResourceId %u out of range %u\n",
                     hwIdx,
                     hwCfg.numResId);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    xcvrIo = &hwCfg.hwResId[hwIdx].xcvrStateIo;
    ioDev = &hwCfg.pcaIo[xcvrIo->ioIdx].dev;

    if (xcvrIo->intfType == INTF_TYPE_SFPP)
    {
        offset = xcvrIo->u.sfppPin.txDisable;
        if ( ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS)) &&
             (xcvrStateValid & FM_PLAT_XCVR_ENABLE) )
        {
            byteIdx = offset / 8;
            bitIdx  = offset % 8;

            /* Read the output register from that PCA IO */
            status = fmUtilPcaIoReadRegs(ioDev,
                                         PCA_IO_REG_TYPE_OUTPUT,
                                         byteIdx,
                                         1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            SET_BIT(ioDev->cachedRegs.output[byteIdx],
                    bitIdx,
                    (xcvrState & FM_PLAT_XCVR_ENABLE) ? 0 : 1);

            /* Write the new value */
            status = fmUtilPcaIoWriteRegs(ioDev,
                                          PCA_IO_REG_TYPE_OUTPUT,
                                          byteIdx,
                                          1);
        }
    }
    else if (xcvrIo->intfType == INTF_TYPE_QSFP)
    {
        isRead = FALSE;
        byteIdx = UINT_NOT_USED;
        offset = xcvrIo->u.qsfpPin.lpMode;
        if ( ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS)) &&
             (xcvrStateValid & FM_PLAT_XCVR_LPMODE) )
        {
            byteIdx = offset / 8;
            bitIdx  = offset % 8;

            /* Read the output register from that PCA IO */
            status = fmUtilPcaIoReadRegs(ioDev,
                                         PCA_IO_REG_TYPE_OUTPUT,
                                         byteIdx,
                                         1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            SET_BIT(ioDev->cachedRegs.output[byteIdx],
                    bitIdx,
                    (xcvrState & FM_PLAT_XCVR_LPMODE) ? 1 : 0);

            /* Indicates the OUTPUT reg has been read */
            isRead = TRUE;
        }
        offset = xcvrIo->u.qsfpPin.resetN;
        if ( ((offset != UINT_NOT_USED) && (offset < NUM_PCA_PINS)) &&
             (xcvrStateValid & FM_PLAT_XCVR_ENABLE) )
        {
            byteIdx = offset / 8;
            bitIdx  = offset % 8;

            /* Read the output register from that PCA IO
               if not already read above */
            if (!isRead)
            {
                status = fmUtilPcaIoReadRegs(ioDev,
                                             PCA_IO_REG_TYPE_OUTPUT,
                                             byteIdx,
                                             1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }
            SET_BIT(ioDev->cachedRegs.output[byteIdx],
                    bitIdx,
                    (xcvrState & FM_PLAT_XCVR_ENABLE) ? 1 : 0);
        }
        if (byteIdx != UINT_NOT_USED)
        {
            /* Write the new value */
            status = fmUtilPcaIoWriteRegs(ioDev,
                                          PCA_IO_REG_TYPE_OUTPUT,
                                          byteIdx,
                                          1);
        }
    }

ABORT:
    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibSetPortXcvrState */




/*****************************************************************************/
/* fmPlatformLibSetPortLed
 * \ingroup platformLib
 *
 * \desc            Set port LEDs for the given port list.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceIdList is the hardware resource id associated
 *                  with list of ports, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 *                  The LED number is embedded in the resourceId.
 *
 * \param[in]       numHwId is the number of elements in the hwResourceIdList.
 *
 * \param[in]       ledState is corresponding LED state to set.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibSetPortLed(fm_int     sw,
                                  fm_uint32 *hwResourceIdList,
                                  fm_int     numHwId,
                                  fm_uint32 *ledStateList)
{
    fm_portLed *    portLed;
    fm_pcaIo *      pcaIo;
    fm_pcaIoDevice *ioDev;
    fm_status       status = FM_OK;
    fm_uint         hwIdx;
    fm_uint         cnt;
    fm_uint         led;
    fm_uint         subLed;
    fm_uint         pcaIdx;
    fm_uint         pin;
    fm_uint         usage;
    fm_uint         byteIdx;
    fm_uint         bitIdx;
    fm_int          ledState;
    fm_byte         ledPcaState;
    fm_byte         ledout;
    fm_bool         regSynced[NUM_PCA_IO];

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d numHwId=%d\n", sw, numHwId);

    FM_NOT_USED(sw);

    /* Mark all the PCA IO output regs out of date for the code
     * below to update the registers only when needed */
    FM_CLEAR(regSynced);

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    for (cnt = 0 ; cnt < (fm_uint)numHwId ; cnt++)
    {
        hwIdx = HW_RESOURCE_ID_TO_IDX(hwResourceIdList[cnt]);
        if (hwIdx >= hwCfg.numResId)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "hwResourceId %u out of range %u\n",
                         hwIdx,
                         hwCfg.numResId);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        led = HW_RESOURCE_ID_TO_LEDNUM(hwResourceIdList[cnt]);
        if (led >= NUM_LED_PER_PORT)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "led number %u embedded in hwResourceId is "
                         "out of range %u\n",
                         led,
                         NUM_LED_PER_PORT);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (hwCfg.debug & DBG_PORT_LED)
        {
            FM_LOG_PRINT("SetLed: hwId=%d ledNum=%d ledState=0x%x \n",
                         hwIdx,
                         led,
                         ledStateList[cnt]);
        }

        portLed = &hwCfg.hwResId[hwIdx].portLed[led];

        if (portLed->type == LED_TYPE_PCA)
        {
            pcaIdx = portLed->ioIdx;
            pcaIo  = &hwCfg.pcaIo[pcaIdx];
            ioDev  = &hwCfg.pcaIo[pcaIdx].dev;

            for (subLed = 0 ; subLed < NUM_SUB_LED ; subLed++)
            {
                pin   = portLed->subLed[subLed].pin;
                usage = portLed->subLed[subLed].usage;

                if ( pin != UINT_NOT_USED )
                {
                    /* Only read from HW when OUTPUT registers are not in sync */
                    if (regSynced[pcaIdx] == FALSE)
                    {
                        status = SetupMuxPath(pcaIo->parentMuxIdx,
                                              pcaIo->parentMuxValue);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                        /* Read all the output registers from that PCA IO */
                        status = fmUtilPcaIoReadRegs(ioDev,
                                                     PCA_IO_REG_TYPE_OUTPUT,
                                                     0,
                                                     ioDev->devCap.numBytes);
                        if (status != FM_OK)
                        {
                            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                                         "Error reading PCA IO regs "
                                         "(hwResId %u)\n",
                                         hwIdx);
                            continue;
                        }

                        /* Indicates the OUTPUT reg has been read */
                        regSynced[pcaIdx] = TRUE;
                    }

                    ledState = GetLedState(ledStateList[cnt], usage);

                    if (IsPcaIoLedDriver(ioDev->model))
                    {
                        if ( ledState == LED_STATE_ON )
                        {
                            /* Turn ON the LED */
                            ledPcaState = ioDev->ledRegs.ledOn;
                        }
                        else if ( ledState == LED_STATE_BLINK_ON ||
                                  ledState == LED_STATE_BLINK_OFF )
                        {
                            /* Enable LED blinking */
                            ledPcaState = ioDev->ledRegs.ledBlink;
                        }
                        else
                        {
                            /* Turn OFF the LED */
                            ledPcaState = ioDev->ledRegs.ledOff;
                        }

                        byteIdx = pin / FM_PCA_LED_PER_LEDOUT;
                        ledout = ioDev->ledRegs.ledout[byteIdx];
                        if (hwCfg.debug & DBG_PORT_LED)
                        {
                            FM_LOG_PRINT("ledout: 0x%x pin=%d "
                                         "byteIdx=%d ledPcaState %d\n",
                                         ledout,
                                         pin,
                                         byteIdx,
                                         ledPcaState);
                        }

                        pin = pin % FM_PCA_LED_PER_LEDOUT;
                        ledout &= ~(0x3 << (pin * 2));
                        ledout |= (ledPcaState << (pin * 2));
                        if (hwCfg.debug & DBG_PORT_LED)
                        {
                            FM_LOG_PRINT("ledout: 0x%x pin=%d\n", ledout, pin);
                        }
                        ioDev->ledRegs.ledout[byteIdx] = ledout;
                    }
                    else
                    {
                        if ( ledState == LED_STATE_ON ||
                             ledState == LED_STATE_BLINK_ON )
                        {
                            /* Turn ON the LED */
                            ledPcaState = 1;
                        }
                        else
                        {
                            /* Turn OFF the LED */
                            ledPcaState = 0;
                        }

                        byteIdx = pin / 8;
                        bitIdx  = pin % 8;
                        if (byteIdx < FM_PCA_IO_REG_MAX_SIZE)
                        {
                            SET_BIT(ioDev->cachedRegs.output[byteIdx],
                                    bitIdx,
                                    ledPcaState);

                            if (hwCfg.debug & DBG_PORT_LED)
                            {
                                FM_LOG_PRINT("pin=%d byteIdx %d bitIdx %d "
                                             "ledPcaState %d\n",
                                             pin,
                                             byteIdx,
                                             bitIdx,
                                             ledPcaState);
                            }
                        }
                    }

                }   /* end if ( pin != UINT_NOT_USED ) */

            }  /* for (subLed = 0 ; subLed < NUM_SUB_LED ; subLed++) */

        }
        else if (portLed->type == LED_TYPE_NONE)
        {
            /* Nothing to do */
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Need support\n");
        }
    }

    /* Now update the PCA IO that have been modified */
    for (pcaIdx = 0 ; pcaIdx < hwCfg.numPcaIo; pcaIdx++)
    {
        if (regSynced[pcaIdx])
        {
            pcaIo  = &hwCfg.pcaIo[pcaIdx];
            ioDev  = &hwCfg.pcaIo[pcaIdx].dev;
            status = SetupMuxPath(pcaIo->parentMuxIdx,
                                  pcaIo->parentMuxValue);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            status = fmUtilPcaIoWriteRegs(ioDev,
                                          PCA_IO_REG_TYPE_OUTPUT,
                                          0,
                                          ioDev->devCap.numBytes);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }

ABORT:
    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibSetPortLed */




/*****************************************************************************/
/* fmPlatformLibEnablePortIntr
 * \ingroup platformLib
 *
 * \desc            Enable/Disable port interrupt for the given port list.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceIdList is the hardware resource id associated with
 *                  a list of ports, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \param[in]       numPorts is the number of ports in the hwResourceIdList.
 *
 * \param[in]       enable is corresponding flag to enable/disable interrupt.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibEnablePortIntr(fm_int     sw,
                                      fm_uint32 *hwResourceIdList,
                                      fm_int     numPorts,
                                      fm_bool   *enableList)
{
    /* All PCA interrupts are enabled in IniPca() */
    FM_NOT_USED(sw);
    FM_NOT_USED(hwResourceIdList);
    FM_NOT_USED(numPorts);
    FM_NOT_USED(enableList);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLibEnablePortIntr */




/*****************************************************************************/
/* fmPlatformLibGetPortIntrPending
 * \ingroup platformLib
 *
 * \desc            Return list of ports with pending interrupt.
 *                  NOTE: If the platform doesn't have support to identify
 *                  which ports are interruting, then the platform can
 *                  choose not to support this feature.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id associated with
 *                  a port, as specified by property
 *                  api.platform.config.switch.n.portIndex.port.hwResourceId.
 *
 * \param[out]      hwResourceIdList points to an array of uint32, where
 *                  this function will write the port hardware resource id that are
 *                  interrupting.
 *
 * \param[in]       listSize is the size of hwResourceIdList.
 *
 * \param[out]      numPorts points to caller-allocated storage where this
 *                  function should place the number of entries returned in
 *                  hwResourceIdList.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibGetPortIntrPending(fm_int     sw,
                                          fm_uint32 *hwResourceIdList,
                                          fm_int     listSize,
                                          fm_int    *numPorts)
{

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d listSize=%d\n",
                 sw, listSize);

    FM_NOT_USED(sw);
    FM_NOT_USED(hwResourceIdList);
    FM_NOT_USED(listSize);

    /* Just return 0 so fmPlatformXcvrStateUpdate will poll all ports */
    *numPorts = 0;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLibGetPortIntrPending */




/*****************************************************************************/
/* fmPlatformLibSetVrmVoltage
 * \ingroup platformLib
 *
 * \desc            Set voltage regulator.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id associated
 *                  with the VRM.
 *                  api.platform.config.switch.n.vrm.hwResourceId.
 *
 *                  The sub-channel number is embedded in the resourceId.
 *
 * \param[in]       mVolt is the voltage to set in milli-volt.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibSetVrmVoltage(fm_int     sw,
                                     fm_uint32  hwResourceId,
                                     fm_uint32  mVolt)
{
    fm_status    status = FM_OK;
    fm_i2cCfg *  i2c;
    fm_hwResId * hwResId;
    fm_vrmI2C *  vrmI2c;
    fm_byte      data[4];
    fm_uint      channel;
    fm_uint      regVidSet;
    fm_uint      regVidChange;
    fm_byte      page;
    fm_uint16    vid;
    fm_uint16    val;
    fm_int16     newTrimVal;
    fm_int16     newRefVolt;
    fm_int16     newTrimVoltDelta;
    fm_int16     actualRefVolt;
    fm_int16     actualTrimVolt;
    fm_uint32    actualVolt;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d hwResourceId=%d mVolt=%d\n",
                                       sw,
                                       hwResourceId,
                                       mVolt);

    FM_NOT_USED(sw);

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if ( HW_RESOURCE_ID_TO_IDX(hwResourceId) >= FM_NUM_HW_RES_ID )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    hwResId = &hwCfg.hwResId[HW_RESOURCE_ID_TO_IDX(hwResourceId)];
    if (hwResId == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    i2c = &hwCfg.i2c[hwResId->vrm.bus];
    if (i2c == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    if ( i2c->isSwitchMaster && i2c->chipVersion == CHIP_VERSION_A0 )
    {
        /* Because of the i2c write-read issue with FM10000 A0, voltage
           scaling is not supported. */
        FM_LOG_PRINT("Voltage scaling not supported on FM10000 A0 version\n");
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    vrmI2c = &hwResId->vrm;
    if (vrmI2c == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    channel = HW_RESOURCE_ID_TO_VRMSUBCHAN(hwResourceId);

    if (hwResId->type != HWRESOURCE_TYPE_VRM)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = SetupMuxPath(hwResId->vrm.parentMuxIdx,
                          hwResId->vrm.parentMuxValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (hwResId->vrm.model && (vrmI2c->model == VRM_TPS40425))
    {
        if (channel == 0)
        {
            page = 0;
        }
        else if (channel == 1)
        {
            page = 1;
        }
        else
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (mVolt > 1100 || mVolt < 800)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Set voltage (%d) should be "
                         "between 800 and 1100 mVolt\n", mVolt);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Select Page */
        data[0] = 0x00;
        data[1] = page;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Get actual voltage */
        status = GetVrmVoltageInt(sw, hwResourceId, &actualVolt);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Get VREF_TRIM (0xd4) */
        data[0] = 0xd4;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 1, 2);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        val = (data[1] << 8) | data[0];

        /* Get the trim voltage (2mv increment) */
        actualTrimVolt = (fm_int16)val * 2;

        /* Get the reference voltage (Vref + trim) */
        actualRefVolt = 600 + actualTrimVolt;

        /* Get the new reference voltage */
        newRefVolt = (mVolt * actualRefVolt) / actualVolt;

        /* Get the new trim voltage delta */
        newTrimVoltDelta = newRefVolt - actualRefVolt;

        /* Get the required trim adjustement and convert to 2mv increment */
        newTrimVal = (actualTrimVolt + newTrimVoltDelta) >> 1;

        /* Validate that trim value to set is in range */
        if (newTrimVal > 30)
        {
            newTrimVal = 30;
        }
        else if (newTrimVal < -60)
        {
            newTrimVal = -60;
        }

        /* Set VREF_TRIM (0xd4) */
        data[0] = 0xd4;
        data[1] = newTrimVal & 0xff;
        data[2] = (newTrimVal >> 8) & 0xff;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 3, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
    else if (hwResId->vrm.model && (hwResId->vrm.model == VRM_PX8847))
    {
        page = 0x20;

        if (channel == 0)
        {
            regVidSet = 0xc4;
            regVidChange = 0xcd;
        }
        else if (channel == 1)
        {
            regVidSet = 0xc8;
            regVidChange = 0xce;
        }
        else
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (mVolt > 1100 || mVolt < 800)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "Set voltage (%d) should be "
                         "between 800 and 1100 mVolt\n", mVolt);
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Convert mVoltage to VID */
        vid = (mVolt - 250)/5 + 1;

        /* Select Page */
        data[0] = 0x00;
        data[1] = page;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Set register passwords */
        data[0] = 0xfd;
        data[1] = 0x7c;
        data[2] = 0xb3;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 3, 0); /* password1 */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        data[0] = 0xfe;  /* PX8848 - extended I2C write protocol */
        data[1] = 0xcf;
        data[2] = 0xb2;
        data[3] = 0x8a;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 4, 0); /* password2 */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Set Output Voltage */
        data[0] = 0xfe;  /* PX8848 - extended I2C write protocol */
        data[1] = regVidSet;
        data[2] = vid & 0x00ff;
        data[3] = (vid & 0xff00) >> 8;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 4, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Enable Output Voltage changed */
        data[0] = 0xfe;  /* PX8848 - extended I2C write protocol */
        data[1] = regVidChange;
        data[2] = 0x00;
        data[3] = 0x01;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 4, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Reset register passwords */
        data[0] = 0xfe;  /* PX8848 - extended I2C write protocol */
        data[1] = 0xcf;
        data[2] = 0x00;
        data[3] = 0x00;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 4, 0); /* password2 */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        data[0] = 0xfd;
        data[1] = 0x00;
        data[2] = 0x00;

        status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 3, 0); /* password1 */
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        if (page)
        {
            /* Set back to page 0 */
            data[0] = 0x00;
            data[1] = 0x00;

            status = i2c->writeReadFunc(i2c->handle, vrmI2c->addr, data, 2, 0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
    }
    else
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

ABORT:
    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibSetVrmVoltage */




/*****************************************************************************/
/* fmPlatformLibGetVrmVoltage
 * \ingroup platformLib
 *
 * \desc            Get the actual voltage from the voltage regulator.
 *
 * \param[in]       sw is the switch number, as specified by property
 *                  api.platform.config.switch.n.switchNumber.
 *
 * \param[in]       hwResourceId is the hardware resource id associated
 *                  with the VRM.
 *                  api.platform.config.switch.n.vrm.hwResourceId.
 *
 *                  The sub-channel number is embedded in the resourceId.
 *
 * \param[in]       mVolt is the caller allocated storage where the
 *                  function will placeis the voltage in milli-volt.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLibGetVrmVoltage(fm_int     sw,
                                     fm_uint32  hwResourceId,
                                     fm_uint32 *mVolt)
{
    fm_status    status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d hwResourceId=%d\n",
                                       sw,
                                       hwResourceId);

    status = TakeLock();
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    status = GetVrmVoltageInt(sw, hwResourceId, mVolt);

    DropLock();

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLibGetVrmVoltage */

