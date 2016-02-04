/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_config.c
 * Creation Date:   June 2, 2014
 * Description:     Functions to handle configuration for platform.
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

#include <fm_sdk_fm10000_int.h>
#include <platforms/util/fm_util_config_tlv.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Re-defined to have shorter name */
#define BPS_1G      FM_PLAT_SERDES_BITRATE_1G
#define BPS_10G     FM_PLAT_SERDES_BITRATE_10G
#define BPS_25G     FM_PLAT_SERDES_BITRATE_25G

typedef enum 
{
    FM_CABLE_TYPE_COPPER,
    FM_CABLE_TYPE_OPTICAL,
    FM_CABLE_TYPE_MAX

} fm_cableType;

#define CURSOR(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_CURSOR_ ## bitrate ## type)

#define CURSOR_LANE(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_LANE_CURSOR_ ## bitrate ## type)

#define PRECURSOR(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_PRECURSOR_ ## bitrate ## type)

#define PRECURSOR_LANE(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_LANE_PRECURSOR_ ## bitrate ## type)

#define POSTCURSOR(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_POSTCURSOR_ ## bitrate ## type)

#define POSTCURSOR_LANE(bitrate, type) \
    (FM_AAK_API_PLATFORM_PORT_LANE_POSTCURSOR_ ## bitrate ## type)

#define AAK_DEFAULT(cursor, type) \
    (FM_AAK_API_PLATFORM_PORT_ ## cursor ## type ## _DEFAULT)

#define AAD_DEFAULT(cursor, type) \
    (FM_AAD_API_PLATFORM_PORT_ ## cursor ## type ## _DEFAULT)


#define FIELD_FORMAT_B     "    %-22s: %d\n"
#define FIELD_FORMAT_V     "    %-22s: %d 0x%x\n"
#define FIELD_FORMAT_S     "    %-22s: %s\n"

#define PRINT_BIT(name, value) \
    FM_LOG_PRINT(FIELD_FORMAT_B, name, value)

#define PRINT_VALUE(name, value) \
    FM_LOG_PRINT(FIELD_FORMAT_V, name, value, value)

#define PRINT_STRING(name, string) \
    FM_LOG_PRINT(FIELD_FORMAT_S, name, string)

#define UNDEF_VAL          -1
#define MAX_BUF_SIZE       256
#define MIN_PHYSICAL_PORT  0
#define MAX_PHYSICAL_PORT  76


typedef struct
{
    /* String description of the value */
    fm_text desc;

    /* Value for the corresponding string */
    fm_int  value;

} fm_platformStrMap;

/* Pair of switch number and port configuration on the switch */
typedef struct
{
    /* Switch number */
    fm_int swIdx;

    /* Pointer to the port configuration structure */
    fm_platformCfgPort *portCfg;

} fm_portCfgSwitch;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_platformStrMap debugMap[] =
{
    { "NONE",      0                       },
    { "CONFIG",    CFG_DBG_CONFIG          },
    { "MOD_STATE", CFG_DBG_MOD_STATE       },
    { "MOD_INTR",  CFG_DBG_MOD_INTR        },
    { "MOD_TYPE",  CFG_DBG_MOD_TYPE        },
    { "MOD_LED",   CFG_DBG_MOD_LED         },
    { "PLAT_LOG",  CFG_DBG_ENABLE_PLAT_LOG },
    { "PORT_LOG",  CFG_DBG_ENABLE_PORT_LOG },
    { "INTR_LOG",  CFG_DBG_ENABLE_INTR_LOG },

};

static fm_platformStrMap ethModeMap[] =
{
    { "1000Base-KX",  FM_ETH_MODE_1000BASE_KX  },
    { "1000Base-X",   FM_ETH_MODE_1000BASE_X   },
    { "2500Base-X",   FM_ETH_MODE_2500BASE_X   },
    { "6GBase-CR",    FM_ETH_MODE_6GBASE_CR    },
    { "6GBase-KR",    FM_ETH_MODE_6GBASE_KR    },
    { "10GBase-KX4",  FM_ETH_MODE_10GBASE_KX4  },
    { "10GBase-CX4",  FM_ETH_MODE_10GBASE_CX4  },
    { "10GBase-CR",   FM_ETH_MODE_10GBASE_CR   },
    { "10GBase-KR",   FM_ETH_MODE_10GBASE_KR   },
    { "10GBase-SR",   FM_ETH_MODE_10GBASE_SR   },
    { "24GBase-KR4",  FM_ETH_MODE_24GBASE_KR4  },
    { "24GBase-CR4",  FM_ETH_MODE_24GBASE_CR4  },
    { "25GBase-SR",   FM_ETH_MODE_25GBASE_SR   },
    { "40GBase-KR4",  FM_ETH_MODE_40GBASE_KR4  },
    { "40GBase-CR4",  FM_ETH_MODE_40GBASE_CR4  },
    { "40GBase-SR4",  FM_ETH_MODE_40GBASE_SR4  },
    { "100GBase-SR4", FM_ETH_MODE_100GBASE_SR4 },
    { "AN-73",        FM_ETH_MODE_AN_73        },
    { "SGMII",        FM_ETH_MODE_SGMII        },
    { "XAUI",         FM_ETH_MODE_XAUI         },
    { "XLAUI",        FM_ETH_MODE_XLAUI        },
    { "disabled",     FM_ETH_MODE_DISABLED     },
    { "AUTODETECT",   FM_ETHMODE_AUTODETECT    },

};

static fm_platformStrMap portTypeMap[] =
{
    { "NONE",     FM_PLAT_PORT_TYPE_NONE     },
    { "EPL",      FM_PLAT_PORT_TYPE_EPL      },
    { "PCIE",     FM_PLAT_PORT_TYPE_PCIE     },
    { "TE",       FM_PLAT_PORT_TYPE_TUNNEL   },
    { "LPBK",     FM_PLAT_PORT_TYPE_LOOPBACK },
    { "FIBM",     FM_PLAT_PORT_TYPE_FIBM     },

};

static fm_platformStrMap intfTypeMap[] =
{
    { "NONE",       FM_PLAT_INTF_TYPE_NONE       },
    { "SFPP",       FM_PLAT_INTF_TYPE_SFPP       },
    { "QSFP_LANE0", FM_PLAT_INTF_TYPE_QSFP_LANE0 },
    { "QSFP_LANE1", FM_PLAT_INTF_TYPE_QSFP_LANE1 },
    { "QSFP_LANE2", FM_PLAT_INTF_TYPE_QSFP_LANE2 },
    { "QSFP_LANE3", FM_PLAT_INTF_TYPE_QSFP_LANE3 },
    { "PCIE",       FM_PLAT_INTF_TYPE_PCIE       },

};

static fm_platformStrMap lanePolarityMap[] =
{
    { "INVERT_NONE",  FM_POLARITY_INVERT_NONE  },
    { "INVERT_RX",    FM_POLARITY_INVERT_RX    },
    { "INVERT_TX",    FM_POLARITY_INVERT_TX    },
    { "INVERT_RX_TX", FM_POLARITY_INVERT_RX_TX },

};

static fm_platformStrMap rxTerminationMap[] =
{
    { "TERM_HIGH",    FM_PORT_TERMINATION_HIGH  },
    { "TERM_LOW",     FM_PORT_TERMINATION_LOW   },
    { "TERM_FLOAT",   FM_PORT_TERMINATION_FLOAT },
    { "NOT_SET",      UNDEF_VAL},

};

static fm_platformStrMap portCapMap[] =
{
    { "NONE",   0                            },
    { "LAG",    FM_PLAT_PORT_CAP_LAG_CAPABLE },
    { "ROUTE",  FM_PLAT_PORT_CAP_CAN_ROUTE   },
    { "10M",    FM_PLAT_PORT_CAP_SPEED_10M   },
    { "100M",   FM_PLAT_PORT_CAP_SPEED_100M  },
    { "1G",     FM_PLAT_PORT_CAP_SPEED_1G    },
    { "2PT5G",  FM_PLAT_PORT_CAP_SPEED_2PT5G },
    { "5G",     FM_PLAT_PORT_CAP_SPEED_5G    },
    { "10G",    FM_PLAT_PORT_CAP_SPEED_10G   },
    { "20G",    FM_PLAT_PORT_CAP_SPEED_20G   },
    { "25G",    FM_PLAT_PORT_CAP_SPEED_25G   },
    { "40G",    FM_PLAT_PORT_CAP_SPEED_40G   },
    { "100G",   FM_PLAT_PORT_CAP_SPEED_100G  },
    { "SW_LED", FM_PLAT_PORT_CAP_SW_LED      },

};

static fm_platformStrMap an73AbilityMap[] =
{
    { "RF",           FM_PLAT_AN73_ABILITY_RF           },
    { "NP",           FM_PLAT_AN73_ABILITY_NP           },
    { "1GBase-KX",    FM_PLAT_AN73_ABILITY_1000BASE_KX  },
    { "10GBase-KR",   FM_PLAT_AN73_ABILITY_10GBASE_KR   },
    { "25GBase-CR",   FM_PLAT_AN73_ABILITY_25GGBASE_CR_KR},
    { "25GBase-KR",   FM_PLAT_AN73_ABILITY_25GGBASE_CR_KR},
    { "40GBase-KR4",  FM_PLAT_AN73_ABILITY_40GBASE_KR4  },
    { "40GBase-CR4",  FM_PLAT_AN73_ABILITY_40GBASE_CR4  },
    { "100GBase-KR4", FM_PLAT_AN73_ABILITY_100GBASE_KR4 },
    { "100GBase-CR4", FM_PLAT_AN73_ABILITY_100GBASE_CR4 },
};

static fm_platformStrMap dfeModeMap[] =
{
    { "STATIC",     FM_DFE_MODE_STATIC     },
    { "ONE_SHOT",   FM_DFE_MODE_ONE_SHOT   },
    { "CONTINUOUS", FM_DFE_MODE_CONTINUOUS },
    { "KR",         FM_DFE_MODE_KR         },
    { "ICAL_ONLY",  FM_DFE_MODE_ICAL_ONLY  },

};

static fm_platformStrMap ledBlinkModeMap[] =
{
    { "NO_BLINK",    FM_LED_BLINK_MODE_NO_BLINK    },
    { "SW_CONTROL",  FM_LED_BLINK_MODE_SW_CONTROL  },
    { "HW_ASSISTED", FM_LED_BLINK_MODE_HW_ASSISTED },
};

static fm_platformStrMap switchBootMode[] =
{
    { "SPI",    FM_PLAT_BOOT_MODE_SPI },
    { "EBI",    FM_PLAT_BOOT_MODE_EBI },
    { "I2C",    FM_PLAT_BOOT_MODE_I2C },

};

static fm_platformStrMap regAccessMode[] =
{
    { "PCIE",   FM_PLAT_REG_ACCESS_PCIE },
    { "EBI",    FM_PLAT_REG_ACCESS_EBI  },
    { "I2C",    FM_PLAT_REG_ACCESS_I2C  },

};

static fm_platformStrMap pcieIsrMode[] =
{
    { "AUTO",   FM_PLAT_PCIE_ISR_AUTO },
    { "SW",     FM_PLAT_PCIE_ISR_SW   },
    { "SPI",    FM_PLAT_PCIE_ISR_SPI  },

};

static fm_platformStrMap phyModelMap[] =
{
    { "UNKNOWN", FM_PLAT_PHY_UNKNOWN },
    { "GN2412",  FM_PLAT_PHY_GN2412  },

};

static fm_platformStrMap disableFuncIntfMap[] =
{
    { "NONE",                                      FM_PLAT_DISABLE_NONE                    },
    { FM_PLAT_RESET_SWITCH_FUNC_NAME_SHORT,        FM_PLAT_DISABLE_RESET_SWITCH_FUNC       },
    { FM_PLAT_I2C_RW_FUNC_NAME_SHORT,              FM_PLAT_DISABLE_I2C_FUNC                },
    { FM_PLAT_DEBUG_FUNC_NAME_SHORT,               FM_PLAT_DISABLE_DEBUG_FUNC              },
    { FM_PLAT_INIT_SW_FUNC_NAME_SHORT,             FM_PLAT_DISABLE_INIT_SW_FUNC            },
    { FM_PLAT_SEL_BUS_FUNC_NAME_SHORT,             FM_PLAT_DISABLE_SEL_BUS_FUNC            },
    { FM_PLAT_GET_PORT_XCVR_STATE_FUNC_NAME_SHORT, FM_PLAT_DISABLE_GET_PORT_XCVR_STATE_FUNC},
    { FM_PLAT_SET_PORT_XCVR_STATE_FUNC_NAME_SHORT, FM_PLAT_DISABLE_SET_PORT_XCVR_STATE_FUNC},
    { FM_PLAT_SET_PORT_LED_FUNC_NAME_SHORT,        FM_PLAT_DISABLE_SET_PORT_LED_FUNC       },
    { FM_PLAT_ENABLE_PORT_INTR_FUNC_NAME_SHORT,    FM_PLAT_DISABLE_ENABLE_PORT_INTR_FUNC   },
    { FM_PLAT_GET_PORT_INTR_PEND_FUNC_NAME_SHORT,  FM_PLAT_DISABLE_GET_PORT_INTR_FUNC      },
    { FM_PLAT_POST_INIT_FUNC_NAME_SHORT,           FM_PLAT_DISABLE_POST_INIT_FUNC          },
    { FM_PLAT_SET_VRM_VOLTAGE_FUNC_NAME_SHORT,     FM_PLAT_DISABLE_SET_VRM_VOLTAGE_FUNC    },
    { FM_PLAT_GET_VRM_VOLTAGE_FUNC_NAME_SHORT,     FM_PLAT_DISABLE_GET_VRM_VOLTAGE_FUNC    },

};

#ifdef FM_SUPPORT_SWAG
static fm_platformStrMap swagLinkType[] =
{
    { "UNDEFINED",   FM_SWAG_LINK_UNDEFINED },
    { "INTERNAL",    FM_SWAG_LINK_INTERNAL },
    { "EXTERNAL",    FM_SWAG_LINK_EXTERNAL },
    { "CPU",         FM_SWAG_LINK_CPU },

};

static fm_platformStrMap swagRole[] =
{
    { "UNDEFINED",   FM_SWITCH_ROLE_UNDEFINED },
    { "LEAF",        FM_SWITCH_ROLE_LEAF },
    { "SPINE",       FM_SWITCH_ROLE_SPINE },
    { "SPINE_LEAF",  FM_SWITCH_ROLE_SPINE_LEAF },
    { "SWAG",        FM_SWITCH_ROLE_SWAG },

};

static fm_platformStrMap swagTopology[] =
{
    { "UNDEFINED",   FM_SWAG_TOPOLOGY_UNDEFINED },
    { "RING",        FM_SWAG_TOPOLOGY_RING },
    { "FAT_TREE",    FM_SWAG_TOPOLOGY_FAT_TREE },
    { "MESH",        FM_SWAG_TOPOLOGY_MESH },

};
#endif


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/* GetStrMap
 * \ingroup intPlatform
 *
 * \desc            Get string equivalent of the specified value given
 *                  the string mapping.
 *
 * \param[in]       value is the value on which to operate.
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
 * \return          string representation of the value or UNKNOWN.
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
            if (strBuf == NULL)
            {
                return strMap[cnt].desc;
            }
            FM_SPRINTF_S(strBuf,
                     strLen,
                     hexVal?"%s(0x%x)":"%s(%d)",
                     strMap[cnt].desc,
                     value);
            return strBuf;
        }
    }

    if (strBuf == NULL)
    {
        return "UNKNOWN";
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

    /* Empty string if no bit is set */
    strBuf[0] = '\0';

    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value & strMap[cnt].value)
        {
            if (strBuf[0] != '\0')
            {
                FM_STRCAT_S(strBuf, strLen, ",");
            }

            FM_STRCAT_S(strBuf, strLen, strMap[cnt].desc);
        }
    }

    return strBuf;

}   /*  end GetStrBitMap */





/*****************************************************************************/
/* ApiPropertyLineLoad
 * \ingroup intPlatform
 *
 * \desc            Load API text configuration line.
 *                                                                      \lb\lb
 * \param[in]       line is the config line to load.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ApiPropertyLineLoad(fm_text line)
{
    fm_status status;
    fm_byte   tlv[FM_TLV_MAX_BUF_SIZE];

    status = fmUtilConfigPropertyEncodeTlv(line, tlv, sizeof(tlv));

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "Unable to encode API property: [%s]\n", line);
        return status;
    }

    return fmLoadApiPropertyTlv(tlv);

} /* end ApiPropertyLineLoad */



/*****************************************************************************/
/* LibCfgLineLoad
 * \ingroup intPlatform
 *
 * \desc            Save shared lib text configuration line for the
 *                  shared lib to load when it initializes.
 *                                                                      \lb\lb
 * \param[in]       line is the config line to load.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LibCfgLineLoad(fm_text line)
{
    fm_status status;
    fm_byte   tlv[FM_TLV_MAX_BUF_SIZE];

    status = fmUtilConfigPropertyEncodeTlv(line, tlv, sizeof(tlv));

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "Unable to encode LT config: [%s]\n", line);
        return status;
    }

    return fmPlatformLoadLibCfgTlv(tlv);

} /* end LibCfgLineLoad */



/*****************************************************************************/
/* LtCfgLineLoad
 * \ingroup intPlatform
 *
 * \desc            Load LT text configuration line.
 *                                                                      \lb\lb
 * \param[in]       line is the config line to load.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LtCfgLineLoad(fm_text line)
{
    fm_status status;
    fm_byte   tlv[FM_TLV_MAX_BUF_SIZE];
    fm_uint   tlvType;

    /* Don't support these properties here, only for NVM image generation */
    if (strstr(line, ".bootCfg.") != NULL)
    {
        return FM_OK;
    }

    status = fmUtilConfigPropertyEncodeTlv(line, tlv, sizeof(tlv));

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "Unable to encode LT config: [%s]\n", line);
        return status;
    }

    tlvType = (tlv[0] << 8) | tlv[1];
    if (tlvType == FM_TLV_PLAT_FILE_LOCK_NAME)
    {
         /* Shared with platform config */
        status = LibCfgLineLoad(line);
        if (status)
        {
            return status;
        }
    }

    return fmPlatformLoadLTCfgTlv(tlv);

} /* end LtCfgLineLoad */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformGetEthModeStr
 * \ingroup intPlatform
 *
 * \desc            Return string equivalent of the ethernet mode.
 *
 * \param[in]       mode is the ethernet mode value on which to operate.
 *
 * \return          string name of the ethernet mode.
 *
 *****************************************************************************/
fm_text fmPlatformGetEthModeStr(fm_ethMode mode)
{

    return GetStrMap(mode,
                     ethModeMap,
                     FM_NENTRIES(ethModeMap),
                     TRUE,
                     NULL,
                     0);

}   /* end fmPlatformGetEthModeStr */




/*****************************************************************************/
/* fmPlatformCfgDump
 * \ingroup intPlatform
 *
 * \desc            Dump platform configuration.
 *
 * \return          NONE.
 *
 *****************************************************************************/
void fmPlatformCfgDump(void)
{
    fm_platformCfg *      platCfg;
    fm_platformCfgLib *   libCfg;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgLane *  laneCfg;
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPhy *   phyCfg;
    fm_gn2412LaneCfg *    phyLaneCfg;
    fm_int                swIdx;
    fm_int                portIdx;
    fm_int                epl;
    fm_int                lane;
    fm_int                phyIdx;
    fm_char               tmpStr[MAX_BUF_SIZE+1];

    platCfg = FM_PLAT_GET_CFG;
    PRINT_VALUE("debug", platCfg->debug);
    PRINT_VALUE("numSwitches", FM_PLAT_NUM_SW);
    PRINT_STRING("platformName", platCfg->name);
    PRINT_STRING("fileLockName", platCfg->fileLockName);

#ifdef FM_SUPPORT_SWAG
    PRINT_STRING("topology",
                 GetStrMap( platCfg->topology,
                            swagTopology,
                            FM_NENTRIES(swagTopology),
                            FALSE,
                            tmpStr,
                            sizeof(tmpStr) ) );
#endif

    for (swIdx = 0 ; swIdx < FM_PLAT_NUM_SW ; swIdx++)
    {
        /* Logical switch is the same as swIdx */
        FM_LOG_PRINT("################################ SW#%d ################################\n", swIdx);
        swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
        PRINT_VALUE(" swIdx", swCfg->swIdx);
        PRINT_VALUE(" switchNumber", swCfg->swNum);
        PRINT_VALUE(" numPorts", swCfg->numPorts);
        PRINT_VALUE(" maxLogicalPortValue", swCfg->maxLogicalPortValue);
        PRINT_VALUE(" ledPollPeriodMsec", swCfg->ledPollPeriodMsec);
        PRINT_STRING("  ledBlinkMode",
                     GetStrMap( swCfg->ledBlinkMode,
                                ledBlinkModeMap,
                                FM_NENTRIES(ledBlinkModeMap),
                                FALSE,
                                tmpStr,
                                sizeof(tmpStr) ) );
        PRINT_VALUE(" xcvrPollPeriodMsec", swCfg->xcvrPollPeriodMsec);
        PRINT_VALUE(" intrPollPeriodMsec", swCfg->intrPollPeriodMsec);
        PRINT_STRING(" uioDevName", swCfg->uioDevName);
        PRINT_STRING(" netDevName", swCfg->netDevName);
        PRINT_STRING(" devMemOffset", swCfg->devMemOffset);
        PRINT_VALUE(" gpioPortIntr", swCfg->gpioPortIntr);
        PRINT_VALUE(" gpioI2cReset", swCfg->gpioI2cReset);
        PRINT_VALUE(" gpioFlashWP", swCfg->gpioFlashWP);
        PRINT_VALUE(" enablePhyDeEmphasis", swCfg->enablePhyDeEmphasis);
        PRINT_VALUE(" vrm.useDefVoltages", swCfg->vrm.useDefVoltages);
        PRINT_VALUE(" VDDS.hwResourceId",
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDS]);
        PRINT_VALUE(" VDDF.hwResourceId",
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDF]);
        PRINT_VALUE(" AVDD.hwResourceId",
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_AVDD]);
        PRINT_VALUE(" fhClock", swCfg->fhClock);
#ifdef FM_SUPPORT_SWAG
        PRINT_STRING("  switchRole",
                     GetStrMap( swCfg->switchRole,
                                swagRole,
                                FM_NENTRIES(swagRole),
                                FALSE,
                                tmpStr,
                                sizeof(tmpStr) ) );
#endif

        for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            FM_LOG_PRINT("==============Port Index %d============\n", portIdx);
            PRINT_VALUE("  logicalPort", portCfg->port);
            PRINT_VALUE("  hwResourceId", portCfg->hwResourceId);
            PRINT_VALUE("  physPort", portCfg->physPort);
            PRINT_VALUE("  epl", portCfg->epl);
            PRINT_VALUE("  lane[0]", portCfg->lane[0]);
            PRINT_VALUE("  lane[1]", portCfg->lane[1]);
            PRINT_VALUE("  lane[2]", portCfg->lane[2]);
            PRINT_VALUE("  lane[3]", portCfg->lane[3]);
            PRINT_VALUE("  pep", portCfg->pep);
            PRINT_VALUE("  tunnel", portCfg->tunnel);
            PRINT_VALUE("  loopback", portCfg->loopback);
            PRINT_VALUE("  autodetect", portCfg->autodetect);
            PRINT_VALUE("  speed", portCfg->speed);
            PRINT_STRING( "  ethMode",
                         GetStrMap( portCfg->ethMode,
                                    ethModeMap,
                                    FM_NENTRIES(ethModeMap),
                                    TRUE,
                                    tmpStr,
                                    sizeof(tmpStr) ) );
            PRINT_STRING( "  portType",
                         GetStrMap( portCfg->portType,
                                    portTypeMap,
                                    FM_NENTRIES(portTypeMap),
                                    FALSE,
                                    tmpStr,
                                    sizeof(tmpStr) ) );
            PRINT_STRING( "  intfType",
                         GetStrMap( portCfg->intfType,
                                    intfTypeMap,
                                    FM_NENTRIES(intfTypeMap),
                                    FALSE,
                                    tmpStr,
                                    sizeof(tmpStr)  ) );
            PRINT_STRING("  dfeMode",
                         GetStrMap(portCfg->dfeMode, 
                                   dfeModeMap, 
                                   FM_NENTRIES(dfeModeMap), 
                                   FALSE,
                                   tmpStr,
                                   sizeof(tmpStr) ) );
            PRINT_STRING( "  capability",
                         GetStrBitMap( portCfg->cap,
                                       portCapMap,
                                       FM_NENTRIES(portCapMap),
                                       tmpStr,
                                       sizeof(tmpStr) ) );
            PRINT_STRING( "  an73AbilityCfg",
                         GetStrBitMap( portCfg->an73AbilityCfg,
                                       an73AbilityMap,
                                       FM_NENTRIES(an73AbilityMap),
                                       tmpStr,
                                       sizeof(tmpStr) ) );
            PRINT_STRING( "  an73Ability",
                         GetStrBitMap( portCfg->an73Ability,
                                       an73AbilityMap,
                                       FM_NENTRIES(an73AbilityMap),
                                       tmpStr,
                                       sizeof(tmpStr) ) );
            PRINT_VALUE("  phyNum", portCfg->phyNum);
            PRINT_VALUE("  phyPort", portCfg->phyPort);

#ifdef FM_SUPPORT_SWAG
            PRINT_STRING("   swagLinkType",
                         GetStrMap( portCfg->swagLink.type,
                                    swagLinkType,
                                    FM_NENTRIES(swagLinkType),
                                    FALSE,
                                    tmpStr,
                                    sizeof(tmpStr) ) );
            if (portCfg->swagLink.type == FM_SWAG_LINK_INTERNAL)
            {
                FM_LOG_PRINT("   SWAG port %d <--> SWAG port %d\n",
                             portCfg->swagLink.logicalPort, portCfg->swagLink.partnerLogicalPort);
                FM_LOG_PRINT("   SW %d, port %d <--> SW %d, port %d\n",
                             portCfg->swagLink.swId, portCfg->swagLink.swPort,
                             portCfg->swagLink.partnerSwitch, portCfg->swagLink.partnerPort);
            }
            else if (portCfg->swagLink.type == FM_SWAG_LINK_EXTERNAL)
            {
                FM_LOG_PRINT("   SWAG port %d, SW %d, port %d\n",
                             portCfg->swagLink.logicalPort,
                             portCfg->swagLink.swId, portCfg->swagLink.swPort);
            }
#endif
            FM_LOG_PRINT("\n");
        }

        for (epl = 0 ; epl < FM_PLAT_NUM_EPL ; epl++)
        {
            FM_LOG_PRINT("#######################################\n");
            FM_LOG_PRINT("#                EPL %d                #\n", epl);
            FM_LOG_PRINT("#######################################\n");
            PRINT_VALUE("  laneToPortIdx[0]", swCfg->epls[epl].laneToPortIdx[0]);
            PRINT_VALUE("  laneToPortIdx[1]", swCfg->epls[epl].laneToPortIdx[1]);
            PRINT_VALUE("  laneToPortIdx[2]", swCfg->epls[epl].laneToPortIdx[2]);
            PRINT_VALUE("  laneToPortIdx[3]", swCfg->epls[epl].laneToPortIdx[3]);


            for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
            {
                FM_LOG_PRINT("============ EPL %d, lane %d ============\n", epl, lane);
                laneCfg = &swCfg->epls[epl].lane[lane];
                PRINT_STRING("  lanePolarity",
                             GetStrMap(laneCfg->lanePolarity, 
                                       lanePolarityMap, 
                                       FM_NENTRIES(lanePolarityMap), 
                                       FALSE,
                                    tmpStr,
                                    sizeof(tmpStr)));
                PRINT_STRING("  rxTermination",
                             GetStrMap(laneCfg->rxTermination, 
                                       rxTerminationMap, 
                                       FM_NENTRIES(rxTerminationMap), 
                                       FALSE,
                                    tmpStr,
                                    sizeof(tmpStr)));
                PRINT_VALUE("  preCursor1GCopper", 
                            laneCfg->copper[BPS_1G].preCursor);
                PRINT_VALUE("  preCursor10GCopper", 
                            laneCfg->copper[BPS_10G].preCursor);
                PRINT_VALUE("  preCursor25GCopper", 
                            laneCfg->copper[BPS_25G].preCursor);
                PRINT_VALUE("  preCursor1GOptical",
                            laneCfg->optical[BPS_1G].preCursor);
                PRINT_VALUE("  preCursor10GOptical",
                            laneCfg->optical[BPS_10G].preCursor);
                PRINT_VALUE("  preCursor25GOptical",
                            laneCfg->optical[BPS_25G].preCursor);

                PRINT_VALUE("  cursor1GCopper",
                            laneCfg->copper[BPS_1G].cursor);
                PRINT_VALUE("  cursor10GCopper",
                            laneCfg->copper[BPS_10G].cursor);
                PRINT_VALUE("  cursor25GCopper",
                            laneCfg->copper[BPS_25G].cursor);
                PRINT_VALUE("  cursor1GOptical",
                            laneCfg->optical[BPS_1G].cursor);
                PRINT_VALUE("  cursor10GOptical",
                            laneCfg->optical[BPS_10G].cursor);
                PRINT_VALUE("  cursor25GOptical",
                            laneCfg->optical[BPS_25G].cursor);

                PRINT_VALUE("  postCursor1GCopper",
                            laneCfg->copper[BPS_1G].postCursor);
                PRINT_VALUE("  postCursor10GCopper",
                            laneCfg->copper[BPS_10G].postCursor);
                PRINT_VALUE("  postCursor25GCopper",
                            laneCfg->copper[BPS_25G].postCursor);
                PRINT_VALUE("  postCursor1GOptical",
                            laneCfg->optical[BPS_1G].postCursor);
                PRINT_VALUE("  postCursor10GOptical",
                            laneCfg->optical[BPS_10G].postCursor);
                PRINT_VALUE("  postCursor25GOptical",
                            laneCfg->optical[BPS_25G].postCursor);

                FM_LOG_PRINT("\n");
            }
            FM_LOG_PRINT("\n");
        }

        for ( phyIdx = 0 ; phyIdx < FM_PLAT_NUM_PHY(swIdx) ; phyIdx++ )
        {
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            FM_LOG_PRINT("==============PHY Index %d============\n", phyIdx);

            PRINT_STRING("  model",
                         GetStrMap( phyCfg->model,
                                    phyModelMap,
                                    FM_NENTRIES(phyModelMap),
                                    FALSE,
                                    tmpStr,
                                    sizeof(tmpStr) ) );
            PRINT_VALUE("  addr", phyCfg->addr);
            PRINT_VALUE("  hwResourceId", phyCfg->hwResourceId);

            if ( phyCfg->model == FM_PLAT_PHY_GN2412 )
            {
                for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
                {
                    phyLaneCfg = &phyCfg->gn2412Lane[lane];
                    FM_LOG_PRINT("    ======== GN2412 %d, lane %d ========\n", 
                                 phyIdx, 
                                 lane);

                    PRINT_VALUE("  appMode", phyLaneCfg->appMode);
                    PRINT_VALUE("  polarity", phyLaneCfg->polarity);
                    PRINT_VALUE("  preTap", phyLaneCfg->preTap);
                    PRINT_VALUE("  attenuation", phyLaneCfg->attenuation);
                    PRINT_VALUE("  postTap", phyLaneCfg->postTap);
                }
            }

            FM_LOG_PRINT("\n");
        }

        FM_LOG_PRINT(" Shared Library Config:\n");
        libCfg = FM_PLAT_GET_LIBS_CFG(swIdx);
        PRINT_STRING("  sharedLibraryName", libCfg->libName);
        PRINT_STRING("  disableFuncIntf",
                     GetStrBitMap( libCfg->disableFuncIntf,
                                   disableFuncIntfMap,
                                   FM_NENTRIES(disableFuncIntfMap),
                                   tmpStr,
                                   sizeof(tmpStr) )  );
        PRINT_VALUE("  tlvCfgBufSize", libCfg->tlvCfgBufSize);
        PRINT_VALUE("  tlvCfgLen", libCfg->tlvCfgLen);

        FM_LOG_PRINT("#######################################################################\n\n");
    }

    return;

}   /* end fmPlatformCfgDump */




/*****************************************************************************/
/* fmPlatformCfgInit
 * \ingroup intPlatform
 *
 * \desc            Initialize platform configuration before being loaded.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformCfgInit(void)
{
    fm_status             status;
    fm_platformCfg *      platCfg;

    status = FM_OK;

    /* Global configuration */
    platCfg = FM_PLAT_GET_CFG;

    platCfg->debug = 0;
    platCfg->numSwitches = 0;

    FM_STRNCPY_S(platCfg->name,
                 sizeof(platCfg->name),
                 FM_AAD_API_PLATFORM_NAME,
                 sizeof(FM_AAD_API_PLATFORM_NAME) + 1);

    platCfg->fileLockName[0] = '\0';

    FM_STRNCPY_S(platCfg->ebiDevName,
                 sizeof(platCfg->ebiDevName),
                 FM_AAD_API_PLATFORM_EBI_DEV_NAME ,
                 sizeof(FM_AAD_API_PLATFORM_EBI_DEV_NAME) + 1);

#ifdef FM_SUPPORT_SWAG
    platCfg->topology = FM_SWAG_TOPOLOGY_UNDEFINED;
#endif

    /* The rest of the initialization is done at TLV loading time */

    return status;

}   /* end fmPlatformCfgInit  */




/*****************************************************************************/
/** fmPlatformLoadPropertiesFromLine
 * \ingroup intPlatform
 *
 * \desc            Loads an attribute from a text line
 *                                                                      \lb\lb
 *                  The expected file format for the database is as follows:
 *                                                                      \lb\lb
 *                  [key] [type] [value]
 *                                                                      \lb\lb
 *                  Where key is a dotted string, type is one of int, bool or
 *                  float, and value is the value to set. Space is the only
 *                  valid separator, but multiple spaces are allowed.
 *
 * \param[in]       line is the text line to load to load.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformLoadPropertiesFromLine(fm_text line)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "line=%s\n", line);

    if (strncmp(line, "api.platform.config.", 20) == 0)
    {
        status = LtCfgLineLoad(line);
    }
    else if (strncmp(line, "api.platform.lib.config.", 24) == 0)
    {
        status = LibCfgLineLoad(line);
    }
    else
    {
        status = ApiPropertyLineLoad(line);
    }

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "%s: Unable to load config: [%s]\n",
                 fmErrorMsg(status), line);

    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformLoadPropertiesFromLine  */



/*****************************************************************************/
/* fmPlatformRequestLibTlvCfg
 * \ingroup intPlatform
 *
 * \desc            Provide the shared lib tlv config.
 *                                                                      \lb\lb
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tlv is the caller-allocated storage where the function
 *                  will place the pointer to the tlv config buffer.
 *
 * \param[out]      tlvLen is the caller-allocated storage where the function
 *                  will place the length of the tlv config..
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformRequestLibTlvCfg(fm_int sw, fm_byte **tlv, fm_uint *tlvLen)
{
    fm_platformCfgLib *libCfg;

    if (sw < 0 || sw >= FM_PLAT_NUM_SW)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    libCfg = FM_PLAT_GET_LIBS_CFG(sw);

    *tlv    = libCfg->tlvCfgBuf;
    *tlvLen = libCfg->tlvCfgLen;

    return FM_OK;

}   /* end fmPlatformRequestLibTlvCfg */




/*****************************************************************************/
/* fmPlatformReleaseLibTlvCfg
 * \ingroup intPlatform
 *
 * \desc            Free the shared lib tlv config.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformReleaseLibTlvCfg(fm_int sw)
{
    fm_platformCfgLib *libCfg;

    if (sw < 0 || sw >= FM_PLAT_NUM_SW)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    libCfg = FM_PLAT_GET_LIBS_CFG(sw);

    if (libCfg->tlvCfgBuf)
    {
        fmFree(libCfg->tlvCfgBuf);
        libCfg->tlvCfgBuf = NULL;
    }
    libCfg->tlvCfgBufSize = 0;
    libCfg->tlvCfgLen     = 0;

    return FM_OK;

}   /* end fmPlatformReleaseLibTlvCfg */




/*****************************************************************************/
/* fmPlatformCfgSwitchGet
 * \ingroup intPlatform
 *
 * \desc            Return the fm_platformCfgSwitch structure of the given
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          pointer to the fm_platformCfgSwitch for given switch.
 * \return          NULL if not found.
 *
 *****************************************************************************/
fm_platformCfgSwitch *fmPlatformCfgSwitchGet(fm_int sw)
{
    if (sw < 0 || sw >= FM_PLAT_NUM_SW)
    {
        return NULL;
    }

    return FM_PLAT_GET_SWITCH_CFG(sw);

}   /* end fmPlatformCfgSwitchGet */




/*****************************************************************************/
/* fmPlatformCfgPortGetIndex
 * \ingroup intPlatform
 *
 * \desc            Return the index to the fm_platformCfgPort structure of
 *                  the given port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          index to the fm_platformPort for given port.
 * \return          -1 if not found.
 *
 *****************************************************************************/
fm_int fmPlatformCfgPortGetIndex(fm_int sw, fm_int port)
{
    fm_int portIdx;

    if ( port < 0 || port > FM_PLAT_GET_SWITCH_CFG(sw)->maxLogicalPortValue )
    {
        return -1;
    }

    portIdx = GET_PLAT_STATE(sw)->lportToPortTableIndex[port];

    if (portIdx >= 0 && FM_PLAT_GET_PORT_CFG(sw, portIdx)->port == port)
    {
        return portIdx;
    }

    return -1;

}   /* end fmPlatformCfgPortGetIndex */




/*****************************************************************************/
/* fmPlatformCfgPortGet
 * \ingroup intPlatform
 *
 * \desc            Return the fm_platformCfgPort structure of the given port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          pointer to the fm_platformPort for given port.
 * \return          NULL if not found.
 *
 *****************************************************************************/
fm_platformCfgPort *fmPlatformCfgPortGet(fm_int sw, fm_int port)
{
    fm_int portIdx;

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        return NULL;
    }

    return FM_PLAT_GET_PORT_CFG(sw, portIdx);

}   /* end fmPlatformCfgPortGet */




/*****************************************************************************/
/* fmPlatformCfgLaneGet
 * \ingroup intPlatform
 *
 * \desc            Return the fm_platformCfgPort structure of the given port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       lane is the lane number.
 *
 * \return          pointer to the fm_platformLaneCfg for given port.
 * \return          NULL if not found.
 *
 *****************************************************************************/
fm_platformCfgLane *fmPlatformCfgLaneGet(fm_int sw, fm_int port, fm_int lane)
{
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPort *  portCfg;
    fm_int portIdx;

    portIdx = fmPlatformCfgPortGetIndex(sw, port);

    if (portIdx < 0)
    {
        return NULL;
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
    portCfg = FM_PLAT_GET_PORT_CFG(sw,portIdx);

    return &swCfg->epls[portCfg->epl].lane[portCfg->lane[lane]];

}   /* end fmPlatformCfgLaneGet */




/*****************************************************************************/
/* fmPlatformGetSchedulerConfigMode
 * \ingroup intPlatform
 *
 * \desc            Get the scheduler configuration mode for the given switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          the scheduler configuration mode for the specified switch.
 *
 *****************************************************************************/
fm_schedulerConfigMode fmPlatformGetSchedulerConfigMode(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    if (swCfg->schedListSelect == FM_AAD_API_PLATFORM_SCHED_LIST_SELECT)
    {
        return (FM_SCHED_INIT_MODE_AUTOMATIC); 
    }
    else 
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "No support for scheduler manual mode\n");
        return (FM_SCHED_INIT_MODE_MANUAL);
    }

}   /* end fmPlatformGetSchedulerConfigMode */




/*****************************************************************************/
/* fmPlatformCfgSchedulerGetTokenList
 * \ingroup intPlatform
 *
 * \desc            Configure the scheduler token list for manual mode based
 *                  on the given switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      schedTokenList is a pointer to the caller-supplied
 *                  scheduler token list structure and should be filled
 *                  with the scheduler properties of the specified switch.
 *                  Refer to fm_schedulerToken for details on the limits
 *                  of certains structure fields and which fields
 *                  are required.
 *
 * \return          the number of tokens configured in the schedTokenList.
 * \return          0 if no configuration token were found in the properties.
 *
 *****************************************************************************/
fm_int fmPlatformCfgSchedulerGetTokenList(fm_int sw,
                                          fm_schedulerToken *schedTokenList)
{
    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                 "No support for scheduler manual mode\n");

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, 0);

}   /* end fmPlatformCfgSchedulerGetTokenList */

