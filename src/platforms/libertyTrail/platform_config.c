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
    { "AUTODETECT",   FM_ETHMODE_AUTODETECT     },

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
    { "UNKNOW", FM_PLAT_PHY_UNKNOWN },
    { "GN2412", FM_PLAT_PHY_GN2412  },

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
 * \return          string representation of the value or UNKNOWN.
 *
 *****************************************************************************/
static fm_text GetStrMap(fm_int             value,
                         fm_platformStrMap *strMap,
                         fm_int             size,
                         fm_bool            hexVal)
{
    fm_int         cnt;
    static fm_int  strCnt = 0;
    static fm_char strBuf[8][MAX_BUF_SIZE+1]; /* No multi-thread nor
                                               * multi-calls in printf */
    strCnt = (strCnt + 1) % 8;  /* Not a good way to support multi-calls in printf */
    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value == strMap[cnt].value)
        {
            FM_SPRINTF_S(strBuf[strCnt],
                     sizeof(strBuf[strCnt]),
                     hexVal?"%s(0x%x)":"%s(%d)",
                     strMap[cnt].desc,
                     value);
            return strBuf[strCnt];
        }
    }

    FM_SNPRINTF_S(strBuf[strCnt], MAX_BUF_SIZE, "UNKNOWN(%d)", value);

    return strBuf[strCnt];

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
 * \return          string representation of the bit mask separated by commas.
 *
 *****************************************************************************/
static fm_text GetStrBitMap(fm_int             value,
                            fm_platformStrMap *strMap,
                            fm_int             size)
{
    fm_int         cnt;
    static fm_char strBuf[MAX_BUF_SIZE+1]; /* No multi-thread nor
                                            * multi-calls in printf */

    /* Empty string if no bit is set */
    strBuf[0] = '\0';

    for (cnt = 0 ; cnt < size ; cnt++)
    {
        if (value & strMap[cnt].value)
        {
            if (strBuf[0] != '\0')
            {
                FM_STRCAT_S(strBuf, sizeof(strBuf), ",");
            }

            FM_STRCAT_S(strBuf, sizeof(strBuf), strMap[cnt].desc);
        }
    }

    return strBuf;

}   /*  end GetStrBitMap */




/*****************************************************************************/
/* GetStringValue
 * \ingroup intPlatform
 *
 * \desc            Get integer equivalent of the string given the string
 *                  mapping.
 *
 * \param[in]       name is the name to find in the string mapping.
 *
 * \param[in]       strMap points to an array of fm_platformStrMap, where the
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
static fm_status GetStringValue(fm_text            name,
                                fm_platformStrMap *strMap,
                                fm_int             size,
                                fm_int *           value)
{
    fm_int  cnt;
    fm_int  lenName;
    fm_int  lenDesc;
    fm_text errStr;

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

    for (cnt = 0 ; cnt < size ; cnt++)
    {
        lenDesc = strlen(strMap[cnt].desc);
        if ( (lenName == lenDesc) &&
             ( (strncasecmp( name, strMap[cnt].desc, lenDesc)) == 0 ) )
        {
            *value = strMap[cnt].value;
            return FM_OK;
        }
    }

    return FM_ERR_NOT_FOUND;

}   /* end GetStringValue */




/*****************************************************************************/
/* GetConfigStrMap
 * \ingroup intPlatform
 *
 * \desc            Get integer configuration given a string mapping.
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
static fm_status GetConfigStrMap(fm_text            name,
                                 fm_int *           value,
                                 fm_int             defVal,
                                 fm_platformStrMap *strMap,
                                 fm_int             size)
{
    fm_text valText;

    valText = fmGetTextApiProperty(name, "UNDEF");

    if (strcmp(valText, "UNDEF") == 0)
    {
        /* The attribute is not found */
        *value = defVal;
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NOT_FOUND);
    }
    else
    {
        if (GetStringValue(valText, strMap, size, value) != FM_OK)
        {
            *value = defVal;
            FM_LOG_PRINT("Invalid value '%s' for '%s'. Defaulting to %s\n", 
                         valText,
                         name,
                         GetStrMap(defVal, strMap, size, TRUE));
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetConfigStrMap */




/*****************************************************************************/
/* GetConfigStrBitMap
 * \ingroup intPlatform
 *
 * \desc            Get bit mask configuration given a string mapping.
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
static fm_status GetConfigStrBitMap(fm_text            name,
                                    fm_int *           value,
                                    fm_int             defVal,
                                    fm_platformStrMap *strMap,
                                    fm_int             size)
{
    fm_text  valText;
    fm_int   valBit;
    fm_int   i;
    fm_char *token;
    fm_char *tokptr;
    fm_uint  strSize;
    fm_char  tmpText[MAX_BUF_SIZE+1];
    fm_int   strLen;

    valText = fmGetTextApiProperty(name, "UNDEF");

    if (strcmp(valText, "UNDEF") == 0)
    {
        *value = defVal;
        return FM_OK;
    }

    *value = 0;

    strLen = strlen(valText);

    if (strLen > MAX_BUF_SIZE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Size of buffer (%d) is too small for input string '%s'. "
                     "Length = %d.\n",
                     MAX_BUF_SIZE,
                     valText,
                     strLen);

        strLen = MAX_BUF_SIZE - 1;
    }

    FM_MEMCPY_S(tmpText, sizeof(tmpText), valText, strLen);
    tmpText[strLen] = '\0';

    /* Comma delimited values */
    strSize = MAX_BUF_SIZE;
    token   = FM_STRTOK_S(tmpText, &strSize, ", ", &tokptr);

    if (token == NULL)
    {
        return FM_OK;
    }

    if (GetStringValue(token, strMap, size, &valBit) == FM_OK)
    {
        *value |= valBit;
    }
    else
    {
        *value = defVal;
        FM_LOG_PRINT("Invalid value '%s' for '%s'. Defaulting to %s\n", 
                     token,
                     name,
                     GetStrBitMap(defVal, strMap, size));
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < size ; i++)
    {
        token = FM_STRTOK_S(NULL, &strSize, ", ", &tokptr);

        if (token == NULL)
        {
            break;
        }

        if (GetStringValue(token, strMap, size, &valBit) == FM_OK)
        {
            *value |= valBit;
        }
        else
        {
            *value = defVal;
            FM_LOG_PRINT("Invalid value '%s' for '%s'. Defaulting to %s\n", 
                         token,
                         name,
                         GetStrBitMap(defVal, strMap, size));
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetConfigStrBitMap */




/*****************************************************************************/
/* GetRequiredConfigInt
 * \ingroup intPlatform
 *
 * \desc            Get required integer configuration.
 *
 * \param[in]       name is the attribute name to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \param[in]       min is the mininum value to check the obtained value.
 *
 * \param[in]       max is the maximun value to check the obtained value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetRequiredConfigInt(fm_text name,
                                      fm_int *value,
                                      fm_int  min,
                                      fm_int  max)
{
    fm_int valInt;

    valInt = fmGetIntApiProperty(name, UNDEF_VAL);

    if (valInt == UNDEF_VAL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Required attribute '%s' is not found\n",
                     name);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if (valInt < min || valInt > max)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Required attribute '%s'=%d is out of range (%d,%d)\n",
                     name,
                     valInt,
                     min,
                     max);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *value = valInt;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetRequiredConfigInt */




/*****************************************************************************/
/* GetOptionalConfigInt
 * \ingroup intPlatform
 *
 * \desc            Get optional integer configuration.
 *
 * \param[in]       name is the attribute name to get.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \param[in]       defVal is the default value to return if the attribute
 *                  is not found.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetOptionalConfigInt(fm_text name,
                                      fm_int *value,
                                      fm_int  defVal)
{
    *value = fmGetIntApiProperty(name, defVal);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetOptionalConfigInt */




/*****************************************************************************/
/* GetAndValidateObjectValue
 * \ingroup intPlatform
 *
 * \desc            Get and validate the integer value following a
 *                  given string.
 *
 *                  For example: "LOG=1" would return 1.
 *
 * \param[in]       buf contains the text to search in.
 *
 * \param[in]       objStr the string to search in buf.
 *
 * \param[in]       errStr string to be printed upon error.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained value
 *
 * \param[in]       min is the mininum value to check the obtained value.
 *
 * \param[in]       max is the maximun value to check the obtained value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetAndValidateObjectValue(fm_text buf,
                                           fm_text objStr,
                                           fm_text errStr,
                                           fm_int  *value,
                                           fm_int  min,
                                           fm_int  max)
{
    fm_text pStr;
    fm_text endStr;
    fm_int  valInt;

    if ( (pStr = strstr(buf, objStr)) == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "String '%s' not found in property %s\n",
                     objStr,
                     buf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NOT_FOUND);
    }

    /* Extract object value */
    pStr += strlen(objStr);
    valInt = strtol(pStr, &endStr, 10);

    if (endStr == pStr)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Unrecognized %s number from property %s\n",
                     errStr,
                     buf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (valInt < min || valInt > max)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "%s%d is out of range (%d,%d) from property %s\n",
                     objStr,
                     valInt,
                     min,
                     max,
                     buf);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *value = valInt;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetAndValidateObjectValue */





/*****************************************************************************/
/* GetPortMapping
 * \ingroup intPlatform
 *
 * \desc            Parse the portMapping attribute for a given switch and
 *                  port and store the obtained values in the port structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetPortMapping(fm_int sw, fm_int port)
{
    fm_platformCfgPort *portCfg;
    fm_char             buf1[MAX_BUF_SIZE+1];
    fm_char             buf2[MAX_BUF_SIZE+1];
    fm_text             valText;
    fm_int              status;
    fm_int              lane;
    fm_int              value;
    fm_int              maxPhy;
    fm_bool             isLaneUsed[FM_PLAT_LANES_PER_EPL];

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    /* First see if the portIndex.%d.portMapping format is used */
    FM_SNPRINTF_S(buf1,
                  MAX_BUF_SIZE,
                  FM_AAK_API_PLATFORM_PORT_MAPPING,
                  sw,
                  port);

    valText = fmGetTextApiProperty(buf1, NULL);

    if (valText == NULL)
    {
        /* No, see if the portIndex.%d.lane.%d.portMapping format is used.
           This format requires all lanes to be defined. */

        FM_CLEAR(isLaneUsed);

        for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
        {
            FM_SNPRINTF_S(buf2,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_LANE_PORT_MAPPING,
                          sw,
                          port,
                          lane);

            valText = fmGetTextApiProperty(buf2, NULL);

            if (valText == NULL)
            {
                /* Mapping for each lane is required. */
                FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                             "Required portMapping attribute '%s' is missing\n",
                             (lane == 0) ? buf1 : buf2);
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
            }
            else
            {
                /* Parse the portMapping attribute line */

                /* Get logical port number*/
                status = GetAndValidateObjectValue(valText,
                                                   "LOG=",
                                                   "logical port",
                                                   &value,
                                                   0,
                                                   FM_PLAT_MAX_LOGICAL_PORT);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                if (lane == 0)
                {
                    portCfg->port = value;
                    portCfg->portType = FM_PLAT_PORT_TYPE_EPL;
                }
                else if (value != portCfg->port)
                {
                    /* All lanes must define the same logical port */
                    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                 "Wrong LOG=%d in portMapping attribute '%s'=%s"
                                 "All lanes must define the same "
                                 "LOG port number (LOG=%d).\n",
                                 value,
                                 buf2,
                                 valText,
                                 portCfg->port);
                    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
                }

                /* Get epl number */
                status = GetAndValidateObjectValue(valText,
                                                   "EPL=",
                                                   "epl",
                                                   &value,
                                                   0,
                                                   FM_PLAT_NUM_EPL-1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                if (lane == 0)
                {
                    portCfg->epl = value;
                }
                else if (value != portCfg->epl)
                {
                    /* All lanes must define the same epl number */
                    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                 "Wrong EPL=%d in portMapping attribute '%s'=%s"
                                 "All lanes must define the same "
                                 "EPL number (EPL=%d).\n",
                                 value,
                                 buf2,
                                 valText,
                                 portCfg->epl);
                    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
                }

                /* Get lane number */
                status = GetAndValidateObjectValue(valText,
                                                   "LANE=",
                                                   "lane",
                                                   &value,
                                                   0,
                                                   FM_PLAT_LANES_PER_EPL-1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                if (isLaneUsed[value])
                {
                    /* This lane is already used */
                    FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                 "Wrong LANE=%d in portMapping attribute '%s'=%s"
                                 "LANE=%d is already defined.\n",
                                 value,
                                 buf2,
                                 valText,
                                 value);
                    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
                }

                portCfg->lane[lane] = value;
                isLaneUsed[value] = TRUE;
            }
        }
    }
    else
    {
        /* Parse the portMapping attribute line */

#ifdef FM_SUPPORT_SWAG
        /* Look for 'SWAG=' string */
        if ( strstr(valText, "SWAG=") != NULL )
        {
            /* Get SWAG logical port number */
            status = GetAndValidateObjectValue(valText,
                                               "SWAG=",
                                               "logical port",
                                               &portCfg->port,
                                               0,
                                               FM_PLAT_MAX_LOGICAL_PORT);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Get physical switch number */
            status = GetAndValidateObjectValue(valText,
                                               "SW=",
                                               "switch",
                                               &portCfg->swagLink.swId,
                                               0,
                                               FM_PLAT_NUM_SW - 1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Get physical switch port number */
            status = GetAndValidateObjectValue(valText,
                                               "LOG=",
                                               "local port",
                                               &portCfg->swagLink.swPort,
                                               0,
                                               FM_PLAT_MAX_LOGICAL_PORT);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Internal port will be processed after */
            portCfg->swagLink.logicalPort = portCfg->port;
            portCfg->swagLink.partnerLogicalPort = -1;
            portCfg->swagLink.partnerSwitch = -1;
            portCfg->swagLink.partnerPort = -1;

            if (portCfg->swagLink.swPort == 0)
            {
                portCfg->swagLink.type = FM_SWAG_LINK_CPU;
            }
            else
            {
                portCfg->swagLink.type = FM_SWAG_LINK_EXTERNAL;
            }
        }
        else
        {
#endif

        /* Get logical port number */
        status = GetAndValidateObjectValue(valText,
                                           "LOG=",
                                           "logical port",
                                           &portCfg->port,
                                           0,
                                           FM_PLAT_MAX_LOGICAL_PORT);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Look for 'EPL=' string */
        if ( strstr(valText, "EPL=") != NULL )
        {
            portCfg->portType = FM_PLAT_PORT_TYPE_EPL;

            /* Get epl number */
            status = GetAndValidateObjectValue(valText,
                                               "EPL=",
                                               "epl",
                                               &portCfg->epl,
                                               0,
                                               FM_PLAT_NUM_EPL-1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Get lane number */
            status = GetAndValidateObjectValue(valText,
                                               "LANE=",
                                               "lane",
                                               &portCfg->lane[0],
                                               0,
                                               FM_PLAT_LANES_PER_EPL-1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /**************************************************
             * Get PHY/Retimer number and PHY port number 
             * associated to this port if any.
             **************************************************/

            /* Look for 'PHY=' string */
            if ( strstr(valText, "PHY=") != NULL )
            {
                maxPhy = FM_PLAT_GET_SWITCH_CFG(sw)->numPhys;

                /* Get PHY number */
                status = GetAndValidateObjectValue(valText,
                                                   "PHY=",
                                                   "phy",
                                                   &portCfg->phyNum,
                                                   0,
                                                   maxPhy-1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                /* Get PHY number if present */
                status = GetAndValidateObjectValue(valText,
                                                   "PORT=",
                                                   "phy port",
                                                   &portCfg->phyPort,
                                                   0,
                                                   FM_PLAT_PORT_PER_PHY-1);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }
        }
        /* Look for 'PCIE=' string */
        else if ( strstr(valText, "PCIE=") != NULL )
        {
            portCfg->portType = FM_PLAT_PORT_TYPE_PCIE;

            /* Get PCIE number */
            status = GetAndValidateObjectValue(valText,
                                               "PCIE=",
                                               "PCIE",
                                               &portCfg->pep,
                                               0,
                                               FM_PLAT_NUM_PEP-1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Look for 'TE=' string */
        else if ( strstr(valText, "TE=") != NULL )
        {
            portCfg->portType = FM_PLAT_PORT_TYPE_TUNNEL;

            /* Get TUNNEL number */
            status = GetAndValidateObjectValue(valText,
                                               "TE=",
                                               "TE",
                                               &portCfg->tunnel,
                                               0,
                                               FM_PLAT_NUM_TUNNEL-1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Look for 'LOOPBACK=' string */
        else if ( strstr(valText, "LPBK=") != NULL )
        {
            portCfg->portType = FM_PLAT_PORT_TYPE_LOOPBACK;

            /* Get LOOPBACK number */
            status = GetAndValidateObjectValue(valText,
                                               "LPBK=",
                                               "LPBK",
                                               &portCfg->loopback,
                                               0,
                                               FM_PLAT_NUM_LOOPBACK-1);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }
        /* Look for 'FIBM' string */
        else if ( strstr(valText, "FIBM") != NULL )
        {
            portCfg->portType = FM_PLAT_PORT_TYPE_FIBM;
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Unrecongnized portMapping attribute line '%s'="
                         "%s\n",
                         buf1,
                         valText);
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }
#ifdef FM_SUPPORT_SWAG
        }
#endif
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end GetPortMapping */


/*****************************************************************************/
/* GetPortLaneTerminationCfg
 * \ingroup intPlatform
 *
 * \desc            Get the lane termination of the given port on the given
 *                  switch. For QSFP ports all the lanes are read.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void GetPortLaneTerminationCfg(fm_int sw, fm_int port)
{
    fm_platformCfgPort *portCfg;
    fm_char             buf[MAX_BUF_SIZE+1];
    fm_int              valInt;
    fm_int              lane;
    fm_int              status;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    /**************************************************
     * Default value is not set
     ***************************************************/


    if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
    {
        /* For QSFP ports, the polarity is set using the port and lane
         * indexes. So get the polarity of all lanes. */

        for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
        {
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_PER_LANE_TERMINATION,
                          sw,
                          port,
                          lane);

            status = GetConfigStrMap(buf,
                                     &valInt,
                                     UNDEF_VAL,
                                     rxTerminationMap,
                                     FM_NENTRIES(rxTerminationMap));

            /* Save the termination in the proper lane cfg structure. */
            if (status == FM_OK)
            {
                FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->rxTermination = valInt;
            }
            else
            {
                FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->rxTermination = UNDEF_VAL;
            }
        }
    }
    else if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3)
    {
        /* For single lane port get the polatity using the port index only. */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PORT_TERMINATION,
                      sw,
                      port);

        status = GetConfigStrMap(buf,
                                 &valInt,
                                 UNDEF_VAL,
                                 rxTerminationMap,
                                 FM_NENTRIES(rxTerminationMap));

        if (status == FM_ERR_NOT_FOUND)
        {
            /* The polarity could be set using the port and lane 0 indexes.*/
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_PER_LANE_TERMINATION,
                          sw,
                          port,
                          0);

            GetConfigStrMap(buf,
                            &valInt,
                            UNDEF_VAL,
                            rxTerminationMap,
                            FM_NENTRIES(rxTerminationMap));
        }

        /* Save the polarity in the proper lane cfg structure. */
        lane = 0;
        if (status == FM_OK)
        {
            FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->rxTermination = valInt;
        }
        else
        {
            FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->rxTermination = UNDEF_VAL;
        }
    }

}   /*  end GetPortLaneTerminationCfg */




/*****************************************************************************/
/* GetPortLanePolarityCfg
 * \ingroup intPlatform
 *
 * \desc            Get the lane polarity of the given port on the given
 *                  switch. For QSFP ports all the lanes are read.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void GetPortLanePolarityCfg(fm_int sw, fm_int port)
{
    fm_platformCfgPort *portCfg;
    fm_char             buf[MAX_BUF_SIZE+1];
    fm_int              defVal;
    fm_int              valInt;
    fm_int              lane;
    fm_int              status;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    /**************************************************
     * First get the default lane polarity value.
     ***************************************************/

    GetStringValue(FM_AAD_API_PLATFORM_PORT_LANE_POLARITY_DEFAULT,
                   lanePolarityMap,
                   FM_NENTRIES(lanePolarityMap),
                   &defVal);

    FM_SNPRINTF_S(buf,
                  MAX_BUF_SIZE,
                  FM_AAK_API_PLATFORM_PORT_LANE_POLARITY_DEFAULT,
                  sw);

    GetConfigStrMap(buf,
                    &defVal,
                    defVal,
                    lanePolarityMap,
                    FM_NENTRIES(lanePolarityMap));


    if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
    {
        /* For QSFP ports, the polarity is set using the port and lane
         * indexes. So get the polarity of all lanes. */

        for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
        {
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_PER_LANE_POLARITY,
                          sw,
                          port,
                          lane);

            GetConfigStrMap(buf,
                            &valInt,
                            defVal,
                            lanePolarityMap,
                            FM_NENTRIES(lanePolarityMap));

            /* Save the polarity in the proper lane cfg structure. */
            FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->lanePolarity = valInt;
        }
    }
    else if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3)
    {
        /* For single lane port get the polatity using the port index only. */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PORT_LANE_POLARITY,
                      sw,
                      port);

        status = GetConfigStrMap(buf,
                                 &valInt,
                                 defVal,
                                 lanePolarityMap,
                                 FM_NENTRIES(lanePolarityMap));

        if (status == FM_ERR_NOT_FOUND)
        {
            /* The polarity could be set using the port and lane 0 indexes.*/
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_PER_LANE_POLARITY,
                          sw,
                          port,
                          0);

            GetConfigStrMap(buf,
                            &valInt,
                            defVal,
                            lanePolarityMap,
                            FM_NENTRIES(lanePolarityMap));
        }

        /* Save the polarity in the proper lane cfg structure. */
        lane = 0;
        FM_PLAT_GET_LANE_CFG(sw, portCfg, lane)->lanePolarity = valInt;
    }

}   /*  end GetPortLanePolarityCfg */




/*****************************************************************************/
/* GetPortLaneAttrCfg
 * \ingroup intPlatform
 *
 * \desc            Get the SerDes attribute configuration of the given port
 *                  on the given switch. For QSFP ports all the lanes are read.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \param[in]       perPortAttr pointer to per Port attribute name.
 *
 * \param[in]       perLaneAttr pointer to per Lane attribute name.
 *
 * \param[in]       perDefAttr pointer to default attribute name.
 *
 * \param[in]       defAttrValue default value associated to the perDefAttr.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the obtained values.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void GetPortLaneAttrCfg(fm_int  sw,
                               fm_int  port,
                               fm_text perPortAttr,
                               fm_text perLaneAttr,
                               fm_text portDefAttr,
                               fm_int  defAttrValue,
                               fm_int  value[])
{
    fm_platformCfgPort *portCfg;
    fm_char             buf[MAX_BUF_SIZE+1];
    fm_int              defVal;
    fm_int              lane;
    fm_int              status;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    /**************************************************
     * First get the default attribute value.
     ***************************************************/

    FM_SNPRINTF_S(buf, MAX_BUF_SIZE, portDefAttr, sw);

    defVal = fmGetIntApiProperty(buf, defAttrValue);

    /**************************************************
     * Get the attribute value for the port.
     ***************************************************/

    if (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0)
    {
        /* For QSFP ports, the attribute is set using the port and lane
         * indexes. So get the attribute value for all lanes. */

        for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
        {
            FM_SNPRINTF_S(buf, MAX_BUF_SIZE, perLaneAttr, sw, port, lane);

            value[lane] = fmGetIntApiProperty(buf, defVal);
            FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                         "Get %s : %d\n",
                         buf,
                         value[lane]);
        }
    }
    else if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3)
    {
        /* For single lane port get the attribute using the port index only. */
        FM_SNPRINTF_S(buf, MAX_BUF_SIZE, perPortAttr, sw, port);

        status = fmGetApiProperty(buf, FM_API_ATTR_INT, &value[0]);

        if (status != FM_OK)
        {
            /* The attribute could be set using the port and lane 0 indexes.*/
            FM_SNPRINTF_S(buf, MAX_BUF_SIZE, perLaneAttr, sw, port, 0);

            value[0] = fmGetIntApiProperty(buf, defVal);
        }
        FM_LOG_DEBUG(FM_LOG_CAT_PLATFORM,
                     "Get %s : %d\n",
                     buf,
                     value[0]);
    }

}   /*  end GetPortLaneAttrCfg */




/*****************************************************************************/
/* GetLaneCursor
 * \ingroup intPlatform
 *
 * \desc            Get the lane cusror property of the given port.
 *
 * \param[in]       sw is the switch index on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \param[in]       bitrate is the serdes bit rate on which to operate.
 *
 * \param[in]       cableType is the cable type (copper or optical).
 *
 * \param[in]       numLane is the number of lane.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static void GetLaneCursor(fm_int               sw,
                          fm_int               port,
                          fm_platSerdesBitRate bitRate,
                          fm_cableType         cableType,
                          fm_int               numLane)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgLane *laneCfg;
    fm_int              laneVal[FM_PLAT_LANES_PER_EPL];
    fm_int              lane;
    fm_text             aakPort;
    fm_text             aakLane;
    fm_text             aakDef;
    fm_int              aadDef;

    if (bitRate == FM_PLAT_SERDES_BITRATE_1G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = CURSOR(1G,_OPTICAL);
            aakLane = CURSOR_LANE(1G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = CURSOR(1G,_COPPER);
            aakLane = CURSOR_LANE(1G,_COPPER);
        }
    }
    else if (bitRate == FM_PLAT_SERDES_BITRATE_10G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = CURSOR(10G,_OPTICAL);
            aakLane = CURSOR_LANE(10G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = CURSOR(10G,_COPPER);
            aakLane = CURSOR_LANE(10G,_COPPER);
        }
    }
    else
    {
        /* 25G */
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = CURSOR(25G,_OPTICAL);
            aakLane = CURSOR_LANE(25G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = CURSOR(25G,_COPPER);
            aakLane = CURSOR_LANE(25G,_COPPER);
        }
    }

    if (cableType == FM_CABLE_TYPE_OPTICAL)
    {
        /* OPTICAL */
        aakDef = AAK_DEFAULT(CURSOR,_OPTICAL);
        aadDef = AAD_DEFAULT(CURSOR,_OPTICAL);
    }
    else
    {
        /* COPPER */
        aakDef = AAK_DEFAULT(CURSOR,_COPPER);
        aadDef = AAD_DEFAULT(CURSOR,_COPPER);
    }

    GetPortLaneAttrCfg(sw, port, aakPort, aakLane, aakDef, aadDef, laneVal);

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    for (lane = 0 ; lane < numLane ; lane++)
    {
        laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            laneCfg->optical[bitRate].cursor = laneVal[lane];
        }
        else
        {
            laneCfg->copper[bitRate].cursor = laneVal[lane];
        }
    }

}   /*  end GetLaneCursor */




/*****************************************************************************/
/* GetLanePreCursor
 * \ingroup intPlatform
 *
 * \desc            Get the lane preCusror property of the given port.
 *
 * \param[in]       sw  is the switch index on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \param[in]       bitrate is the serdes bit rate on which to operate.
 *
 * \param[in]       cableType is the cable type (copper or optical).
 *
 * \param[in]       numLane is the number of lane.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void GetLanePreCursor(fm_int               sw,
                             fm_int               port,
                             fm_platSerdesBitRate bitRate,
                             fm_cableType         cableType,
                             fm_int               numLane)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgLane *laneCfg;
    fm_int              laneVal[FM_PLAT_LANES_PER_EPL];
    fm_int              lane;
    fm_text             aakPort;
    fm_text             aakLane;
    fm_text             aakDef;
    fm_int              aadDef;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    if (bitRate == FM_PLAT_SERDES_BITRATE_1G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = PRECURSOR(1G,_OPTICAL);
            aakLane = PRECURSOR_LANE(1G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = PRECURSOR(1G,_COPPER);
            aakLane = PRECURSOR_LANE(1G,_COPPER);
        }
    }
    else if (bitRate == FM_PLAT_SERDES_BITRATE_10G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = PRECURSOR(10G,_OPTICAL);
            aakLane = PRECURSOR_LANE(10G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = PRECURSOR(10G,_COPPER);
            aakLane = PRECURSOR_LANE(10G,_COPPER);
        }
    }
    else
    {
        /* 25G */
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = PRECURSOR(25G,_OPTICAL);
            aakLane = PRECURSOR_LANE(25G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = PRECURSOR(25G,_COPPER);
            aakLane = PRECURSOR_LANE(25G,_COPPER);
        }
    }

    if (cableType == FM_CABLE_TYPE_OPTICAL)
    {
        /* OPTICAL */
        aakDef = AAK_DEFAULT(PRECURSOR,_OPTICAL);
        aadDef = AAD_DEFAULT(PRECURSOR,_OPTICAL);
    }
    else
    {
        /* COPPER */
        aakDef = AAK_DEFAULT(PRECURSOR,_COPPER);
        aadDef = AAD_DEFAULT(PRECURSOR,_COPPER);
    }

    GetPortLaneAttrCfg(sw, port, aakPort, aakLane, aakDef, aadDef, laneVal);

    for (lane = 0 ; lane < numLane ; lane++)
    {
        laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            laneCfg->optical[bitRate].preCursor = laneVal[lane];
        }
        else
        {
            laneCfg->copper[bitRate].preCursor = laneVal[lane];
        }
    }

}   /*  end GetLanePreCursor */




/*****************************************************************************/
/* GetLanePostCursor
 * \ingroup intPlatform
 *
 * \desc            Get the lane postCusror property of the given port.
 *
 * \param[in]       sw  is the switch index on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \param[in]       bitrate is the serdes bit rate on which to operate.
 *
 * \param[in]       cableType is the cable type (copper or optical).
 *
 * \param[in]       numLane is the number of lane.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void GetLanePostCursor(fm_int               sw,
                              fm_int               port,
                              fm_platSerdesBitRate bitRate,
                              fm_cableType         cableType,
                              fm_int               numLane)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgLane *laneCfg;
    fm_int              laneVal[FM_PLAT_LANES_PER_EPL];
    fm_int              lane;
    fm_text             aakPort;
    fm_text             aakLane;
    fm_text             aakDef;
    fm_int              aadDef;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    /**************************************************
     * Get SerDes preCursor for 1G and DA cable
     **************************************************/

    if (bitRate == FM_PLAT_SERDES_BITRATE_1G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = POSTCURSOR(1G,_OPTICAL);
            aakLane = POSTCURSOR_LANE(1G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = POSTCURSOR(1G,_COPPER);
            aakLane = POSTCURSOR_LANE(1G,_COPPER);
        }
    }
    else if (bitRate == FM_PLAT_SERDES_BITRATE_10G)
    {
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = POSTCURSOR(10G,_OPTICAL);
            aakLane = POSTCURSOR_LANE(10G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = POSTCURSOR(10G,_COPPER);
            aakLane = POSTCURSOR_LANE(10G,_COPPER);
        }
    }
    else
    {
        /* 25G */
        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            /* OPTICAL */
            aakPort = POSTCURSOR(25G,_OPTICAL);
            aakLane = POSTCURSOR_LANE(25G,_OPTICAL);
        }
        else
        {
            /* COPPER */
            aakPort = POSTCURSOR(25G,_COPPER);
            aakLane = POSTCURSOR_LANE(25G,_COPPER);
        }
    }

    if (cableType == FM_CABLE_TYPE_OPTICAL)
    {
        /* OPTICAL */
        aakDef  = AAK_DEFAULT(POSTCURSOR,_OPTICAL);
        aadDef  = AAD_DEFAULT(POSTCURSOR,_OPTICAL);
    }
    else
    {
        /* COPPER */
        aakDef = AAK_DEFAULT(POSTCURSOR,_COPPER);
        aadDef = AAD_DEFAULT(POSTCURSOR,_COPPER);
    }

    GetPortLaneAttrCfg(sw, port, aakPort, aakLane, aakDef, aadDef, laneVal);

    for (lane = 0 ; lane < numLane ; lane++)
    {
        laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

        if (cableType == FM_CABLE_TYPE_OPTICAL)
        {
            laneCfg->optical[bitRate].postCursor = laneVal[lane];
        }
        else
        {
            laneCfg->copper[bitRate].postCursor = laneVal[lane];
        }
    }

}   /*  end GetLanePostCursor */




/*****************************************************************************/
/* LoadEplProperties
 * \ingroup intPlatform
 *
 * \desc            Load all the per-EPL properties of the given port on the
 *                  given switch.
 *
 * \param[in]       sw is the switch index on which to operate.
 *
 * \param[in]       port is the port index on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LoadEplProperties(fm_int sw, fm_int port)
{
    fm_platformCfgPort *  portCfg;
    fm_platformCfgSwitch *swCfg;
    fm_platSerdesBitRate  bitRate;
    fm_cableType          cableType;
    fm_int                numLane;
    fm_int                lane;

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);

    if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
    {
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    switch (portCfg->intfType)
    {
        case FM_PLAT_INTF_TYPE_QSFP_LANE0:
        case FM_PLAT_INTF_TYPE_QSFP_LANE1:
        case FM_PLAT_INTF_TYPE_QSFP_LANE2:
        case FM_PLAT_INTF_TYPE_QSFP_LANE3:
            lane = portCfg->intfType - FM_PLAT_INTF_TYPE_QSFP_LANE0;

            swCfg = FM_PLAT_GET_SWITCH_CFG(sw);
            swCfg->epls[portCfg->epl].laneToPortIdx[lane] = portCfg->portIdx;

            /**************************************************
             * For QSFP ports, EPL properties are loaded using 
             * the QSFP_LANE0 only because all the epl/lane 
             * properties MUST be set on the portIndex 
             * associated to LANE0. 
             **************************************************/

            if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }

            numLane = FM_PLAT_LANES_PER_EPL;
            break;

        default:
            numLane = 1;
            break;
    }

    /**************************************************
     * Get lane termination
     **************************************************/

    GetPortLaneTerminationCfg(sw, port);

    /**************************************************
     * Get lane polarity
     **************************************************/

    GetPortLanePolarityCfg(sw, port);

    /**************************************************
     * Get SerDes cursor, preCursor and postCursor. 
     **************************************************/

    /* Do it for all supported SerDes bit rate: 1G, 10G and 25G */
    for (bitRate = 0 ; bitRate < FM_PLAT_SERDES_BITRATE_MAX ; bitRate++)
    {
        /* Do it for both DA cable and optical module. */
        for (cableType = 0 ; cableType < FM_CABLE_TYPE_MAX ; cableType++)
        {
            GetLaneCursor    (sw, port, bitRate, cableType, numLane);
            GetLanePreCursor (sw, port, bitRate, cableType, numLane);
            GetLanePostCursor(sw, port, bitRate, cableType, numLane);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /*  end LoadEplProperties */



/*****************************************************************************/
/* LoadGN2412Config
 * \ingroup intPlatform
 *
 * \desc            Load GN2412 retimer specific configuration.
 *
 * \param[in]       sw is the switch index on which to operate.
 * 
 * \param[in]       phyIdx is the retimer index on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LoadGN2412Config(fm_int sw, fm_int phyIdx)
{
    fm_gn2412LaneCfg *laneCfg;
    fm_status         status;
    fm_int            lane;
    fm_char           buf[MAX_BUF_SIZE+1];
    fm_text           valText;
    fm_int            crossConnect[FM_GN2412_NUM_LANES] = { 6,7,8,9,10,11,
                                                            0,1,2,3,4,5 };

    for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
    {
        laneCfg = &FM_PLAT_GET_PHY_CFG(sw, phyIdx)->gn2412Lane[lane];

        /* Get the TX equalization config for the given lane */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_TX_EQUALIZER,
                      sw,
                      phyIdx,
                      lane);

        valText = fmGetTextApiProperty(buf, NULL);

        if (valText == NULL)
        {
            /* Not defined, then used default values */
            laneCfg->polarity    = FM_GN2412_DEF_LANE_POLARITY;
            laneCfg->preTap      = FM_GN2412_DEF_LANE_PRE_TAP;
            laneCfg->attenuation = FM_GN2412_DEF_LANE_ATT;
            laneCfg->postTap     = FM_GN2412_DEF_LANE_POST_TAP;
            continue;
        }

        /* Get tap polarity value */
        status = GetAndValidateObjectValue(valText,
                                           "POL=",
                                           "polarity",
                                           &laneCfg->polarity,
                                           0,
                                           31);
        laneCfg->polarity = (status == FM_OK) ? laneCfg->polarity :
                                                FM_GN2412_DEF_LANE_POLARITY;

        /* Get pre-tap value */
        status = GetAndValidateObjectValue(valText,
                                           "PRE=",
                                           "pre-tap",
                                           &laneCfg->preTap,
                                           0,
                                           15);
        laneCfg->preTap = (status == FM_OK) ? laneCfg->preTap :
                                              FM_GN2412_DEF_LANE_PRE_TAP;

        /* Get attenuation value */
        status = GetAndValidateObjectValue(valText,
                                           "ATT=",
                                           "attenuation",
                                           &laneCfg->attenuation,
                                           0,
                                           15);
        laneCfg->attenuation = (status == FM_OK) ? laneCfg->attenuation :
                                                   FM_GN2412_DEF_LANE_ATT;

        /* Get post-tap value */
        status = GetAndValidateObjectValue(valText,
                                           "POST=",
                                           "post-tap",
                                           &laneCfg->postTap,
                                           0,
                                           31);
        laneCfg->postTap = (status == FM_OK) ? laneCfg->postTap :
                                               FM_GN2412_DEF_LANE_POST_TAP;

        /* Hard code the cross-connection. Will eventually be configurable. */
        laneCfg->rxPort = crossConnect[lane];

        /* Get the application mode for the given lane */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_APP_MODE,
                      sw,
                      phyIdx,
                      lane);
        GetOptionalConfigInt(buf, &laneCfg->appMode, 0x74);
    }

    return FM_OK;

}   /* end LoadGN2412Config */




/*****************************************************************************/
/* LoadPhyConfiguration
 * \ingroup intPlatform
 *
 * \desc            Load PHYs configuration.
 *
 * \param[in]       sw is the switch index on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status LoadPhyConfiguration(fm_int sw)
{
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPhy *   phyCfg;
    fm_status             status;
    fm_int                phyIdx;
    fm_int                valInt;
    fm_char               buf[MAX_BUF_SIZE+1];

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    /* Get the number of PHYs connected to this switch */
    FM_SNPRINTF_S(buf,
                  MAX_BUF_SIZE,
                  FM_AAK_API_PLATFORM_NUM_PHYS,
                  sw);

    GetOptionalConfigInt(buf, &valInt, 0);
    swCfg->numPhys = valInt;

    if ( swCfg->numPhys > FM_PLAT_MAX_NUM_PHYS )
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Number of PHYs %d is bigger than %d for '%s'\n",
                     swCfg->numPhys,
                     FM_PLAT_MAX_NUM_PHYS,
                     buf);
        return FM_ERR_INVALID_ARGUMENT;
    }
    else if ( swCfg->numPhys < 0 )
    {
        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "Missing valid value for '%s'\n", 
                     buf);
        return FM_ERR_INVALID_ARGUMENT;
    }
    else if ( swCfg->numPhys == 0 )
    {
        /* No PHY configuration to load */
        return FM_OK;
    }

    /* Allocate the PHY configuration structures */
    swCfg->phys = fmAlloc( swCfg->numPhys * sizeof(fm_platformCfgPhy) );

    if (swCfg->phys == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    for ( phyIdx = 0 ; phyIdx < swCfg->numPhys ; phyIdx++ )
    {
        phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);

        /* Get the PHY model */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_MODEL,
                      sw,
                      phyIdx);

        GetConfigStrMap(buf,
                        &valInt,
                        FM_PLAT_PHY_UNKNOWN,
                        phyModelMap,
                        FM_NENTRIES(phyModelMap));

        phyCfg->model = valInt;
        if ( phyCfg->model == FM_PLAT_PHY_UNKNOWN )
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Missing valid value for '%s'\n", 
                         buf);
            return FM_ERR_INVALID_ARGUMENT;
        }
        else if ( phyCfg->model == FM_PLAT_PHY_GN2412 )
        {
            /* Get GN2412 specific properties */
            status = LoadGN2412Config(sw, phyIdx);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        /* Get PHY I2C address */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_ADDR,
                      sw,
                      phyIdx);
        GetOptionalConfigInt(buf, &valInt, 0);

        phyCfg->addr = valInt;
        if ( phyCfg->addr == 0 )
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Missing valid value for '%s'\n", 
                         buf);
            return FM_ERR_INVALID_ARGUMENT;
        }

        /* Get PHY HW Resource ID */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_HW_RESOURCE_ID,
                      sw,
                      phyIdx);
        GetOptionalConfigInt(buf, &valInt, 0xFFFFFFFF);

        phyCfg->hwResourceId = valInt;
        if ( phyCfg->hwResourceId == 0xFFFFFFFF )
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Missing valid value for '%s'\n", 
                         buf);
            return FM_ERR_INVALID_ARGUMENT;
        }
    }

    return FM_OK;

}   /* end LoadPhyConfiguration */


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
    return GetStrMap( mode, ethModeMap, FM_NENTRIES(ethModeMap), TRUE);

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
                            FALSE ) );
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
                                FALSE ) );
        PRINT_VALUE(" xcvrPollPeriodMsec", swCfg->xcvrPollPeriodMsec);
        PRINT_VALUE(" intrPollPeriodMsec", swCfg->intrPollPeriodMsec);
        PRINT_STRING(" uioDevName", swCfg->uioDevName);
        PRINT_STRING(" netDevName", swCfg->netDevName);
        PRINT_STRING(" devMemOffset", swCfg->devMemOffset);
        PRINT_VALUE(" gpioPortIntr", swCfg->gpioPortIntr);
        PRINT_VALUE(" gpioI2cReset", swCfg->gpioI2cReset);
        PRINT_VALUE(" gpioFlashWP", swCfg->gpioFlashWP);
#ifdef FM_SUPPORT_SWAG
        PRINT_STRING("  switchRole",
                     GetStrMap( swCfg->switchRole,
                                swagRole,
                                FM_NENTRIES(swagRole),
                                FALSE ) );
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
            PRINT_STRING( "  ethMode", fmPlatformGetEthModeStr(portCfg->ethMode) );
            PRINT_STRING( "  portType",
                         GetStrMap( portCfg->portType,
                                    portTypeMap,
                                    FM_NENTRIES(portTypeMap),
                                    FALSE ) );
            PRINT_STRING( "  intfType",
                         GetStrMap( portCfg->intfType,
                                    intfTypeMap,
                                    FM_NENTRIES(intfTypeMap),
                                    FALSE ) );
            PRINT_STRING( "  capability",
                         GetStrBitMap( portCfg->cap,
                                       portCapMap,
                                       FM_NENTRIES(portCapMap) ) );
            PRINT_STRING("  dfeMode",
                         GetStrMap(portCfg->dfeMode, 
                                   dfeModeMap, 
                                   FM_NENTRIES(dfeModeMap), 
                                   FALSE));
            PRINT_STRING( "  an73Ability",
                         GetStrBitMap( portCfg->an73Ability,
                                       an73AbilityMap,
                                       FM_NENTRIES(an73AbilityMap) ) );
            PRINT_VALUE("  phyNum", portCfg->phyNum);
            PRINT_VALUE("  phyPort", portCfg->phyPort);

#ifdef FM_SUPPORT_SWAG
            PRINT_STRING("   swagLinkType",
                         GetStrMap( portCfg->swagLink.type,
                                    swagLinkType,
                                    FM_NENTRIES(swagLinkType),
                                    FALSE ) );
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
                                       FALSE));
                PRINT_STRING("  rxTermination",
                             GetStrMap(laneCfg->rxTermination, 
                                       rxTerminationMap, 
                                       FM_NENTRIES(rxTerminationMap), 
                                       FALSE));
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
                                    FALSE ) );
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
                     GetStrMap( libCfg->disableFuncIntf,
                                disableFuncIntfMap,
                                FM_NENTRIES(disableFuncIntfMap),
                                FALSE ) );

        FM_LOG_PRINT("#######################################################################\n\n");
    }

    return;

}   /* end fmPlatformCfgDump */




/*****************************************************************************/
/* fmPlatformCfgLoad
 * \ingroup intPlatform
 *
 * \desc            Load platform configuration file.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformCfgLoad(void)
{
    fm_status             status;
    fm_int                swIdx;
    fm_platformCfg *      platCfg;
    fm_platformCfgSwitch *swCfg;
    fm_int                portIdx;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgLib *   libCfg;
    fm_char               buf[MAX_BUF_SIZE+1];
    fm_int                defVal;
    fm_int                valInt;
    fm_text               valText;
    fm_int                len;
    fm_int                epl;
    fm_int                lane;
    fm_text               devName;
    fm_uint64             categoryMask;
#ifdef FM_SUPPORT_SWAG
    fm_int                internalPortIdx;
    fm_int                valInt2;
    fm_platformCfgPort   *portCfg2;
#endif

    /* Global configuration */
    platCfg = FM_PLAT_GET_CFG;

    /* Get the debug attribute */
    status = GetConfigStrBitMap(FM_AAK_API_PLATFORM_CONFIG_DEBUG,
                                &valInt,
                                0,
                                debugMap,
                                FM_NENTRIES(debugMap));
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    platCfg->debug = valInt;

    if (platCfg->debug & CFG_DBG_CONFIG)
    {
        /* Dump all API properties */
        fmDbgDumpApiProperties();
    }

    categoryMask = 0;

    if (platCfg->debug & CFG_DBG_ENABLE_PLAT_LOG)
    {
        FM_LOG_PRINT("Enabling platform debug logging\n");
        categoryMask |= FM_LOG_CAT_PLATFORM;
    }

    if (platCfg->debug & CFG_DBG_ENABLE_PORT_LOG)
    {
        FM_LOG_PRINT("Enabling port debug logging\n");
        categoryMask |= FM_LOG_CAT_PORT;
    }

    if (platCfg->debug & CFG_DBG_ENABLE_INTR_LOG)
    {
        FM_LOG_PRINT("Enabling interrupt debug logging\n");
        categoryMask |= FM_LOG_CAT_EVENT_INTR;
    }

    if (categoryMask != 0)
    {
        fmSetLoggingFilter(categoryMask, FM_LOG_LEVEL_ALL, NULL, NULL);
    }

    /* Get the number of switches in the system */
    status = GetRequiredConfigInt(FM_AAK_API_PLATFORM_NUM_SWITCHES,
                                  &valInt,
                                  0,
                                  256);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    platCfg->numSwitches = valInt;

    /* Get the platform name */
    valText = fmGetTextApiProperty(FM_AAK_API_PLATFORM_NAME,
                                   FM_AAD_API_PLATFORM_NAME);
    FM_STRNCPY_S(platCfg->name,
                 FM_PLAT_MAX_CFG_STR_LEN,
                 valText,
                 strlen(valText) + 1);

    /* Get the file lock filename */
    valText = fmGetTextApiProperty(FM_AAK_API_PLATFORM_FILE_LOCK_NAME,
                                   "");
    FM_STRNCPY_S(platCfg->fileLockName,
                 FM_PLAT_MAX_CFG_STR_LEN,
                 valText,
                 strlen(valText) + 1);

#ifdef FM_SUPPORT_SWAG
    /* Get the topology */
    GetConfigStrMap(FM_AAK_API_PLATFORM_TOPOLOGY,
                    &valInt,
                    FM_SWAG_TOPOLOGY_UNDEFINED,
                    swagTopology,
                    FM_NENTRIES(swagTopology));
    platCfg->topology = valInt;
#endif

    /* Allocate the switch configuration structures */
    fmRootPlatform->cfg.switches =
        fmAlloc( platCfg->numSwitches * sizeof(fm_platformCfgSwitch) );

    if (fmRootPlatform->cfg.switches == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }


    /************************************************** 
     *  
     * SWITCH CONFIGURATION 
     *  
     **************************************************/

    swIdx = 0;

    for (swIdx = 0 ; swIdx < platCfg->numSwitches ; swIdx++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);

        /* logical switch is the same as switch index */
        swCfg->swIdx = swIdx;

        FM_SNPRINTF_S(buf, MAX_BUF_SIZE, FM_AAK_API_PLATFORM_SW_NUM, swIdx);
        status = GetRequiredConfigInt(buf, &valInt, 0, 256);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        swCfg->swNum = valInt;

        /* Get the switch boot mode */

        GetStringValue(FM_AAD_API_PLATFORM_BOOT_MODE,
                       switchBootMode,
                       FM_NENTRIES(switchBootMode),
                       &defVal);
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_BOOT_MODE,
                      swIdx);
        GetConfigStrMap(buf,
                        &valInt,
                        defVal,
                        switchBootMode,
                        FM_NENTRIES(switchBootMode));
        swCfg->bootMode = valInt;

        /* Get the register access mode */

        GetStringValue(FM_AAD_API_PLATFORM_REGISTER_ACCESS,
                       regAccessMode,
                       FM_NENTRIES(regAccessMode),
                       &defVal);
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_REGISTER_ACCESS,
                      swIdx);
        GetConfigStrMap(buf,
                        &valInt,
                        defVal,
                        regAccessMode,
                        FM_NENTRIES(regAccessMode));
        swCfg->regAccess = valInt;

        /* Get the PCIe ISR mode */

        GetStringValue(FM_AAD_API_PLATFORM_PCIE_ISR,
                       pcieIsrMode,
                       FM_NENTRIES(pcieIsrMode),
                       &defVal);
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PCIE_ISR,
                      swIdx);
        GetConfigStrMap(buf,
                        &valInt,
                        defVal,
                        pcieIsrMode,
                        FM_NENTRIES(pcieIsrMode));
        swCfg->pcieISR = valInt;

        if (swCfg->pcieISR == FM_PLAT_PCIE_ISR_AUTO)
        {
            /* In auto mode, use the boot mode to determine pcie isr mode */
            if (swCfg->bootMode == FM_PLAT_BOOT_MODE_SPI)
            {
                swCfg->pcieISR = FM_PLAT_PCIE_ISR_SPI;
            }
            else
            {
                swCfg->pcieISR = FM_PLAT_PCIE_ISR_SW;
            }
        }

        /* Get the LED management polling period */

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_LED_POLL_MSEC,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_AAD_API_PLATFORM_LED_POLL_MSEC);
        swCfg->ledPollPeriodMsec = valInt;

        /* Get the LED blinking mode */

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_LED_BLINK_MODE,
                      swIdx);
        GetConfigStrMap(buf,
                        &valInt,
                        FM_LED_BLINK_MODE_NO_BLINK,
                        ledBlinkModeMap,
                        FM_NENTRIES(ledBlinkModeMap));
        swCfg->ledBlinkMode = valInt;

        /* Get the Transceiver management polling period */

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_XCVR_POLL_MSEC,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_AAD_API_PLATFORM_XCVR_POLL_MSEC);
        swCfg->xcvrPollPeriodMsec = valInt;

        /* Get interrupt polling */

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_INT_POLL_MSEC,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_AAD_API_PLATFORM_INT_POLL_MSEC);
        swCfg->intrPollPeriodMsec = valInt;

        /* Get the CPU port number */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_CPU_PORT,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_AAD_API_PLATFORM_CPU_PORT);
        swCfg->cpuPort = valInt;

        /* Get the FM10000 GPIO number for SFP/QSFP module interrupt */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PORT_INTERRUPT_GPIO,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_PLAT_UNDEFINED);
        swCfg->gpioPortIntr = valInt;

        /* Get the FM10000 GPIO number used for the I2C reset Control */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_I2C_RESET_GPIO,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_PLAT_UNDEFINED);
        swCfg->gpioI2cReset = valInt;

        /* Get the FM10000 GPIO number used to control the write protect pin on
           the SPI flash (switch's NVM). */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_FLASH_WP_GPIO,
                      swIdx);
        GetOptionalConfigInt(buf, &valInt, FM_PLAT_UNDEFINED);
        swCfg->gpioFlashWP = valInt;

        /* Get the number of ports */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_SW_NUMPORTS,
                      swIdx);
        status = GetRequiredConfigInt(buf,
                                      &valInt,
                                      0,
                                      platCfg->numSwitches * (FM10000_MAX_PORT+1));
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        swCfg->numPorts = valInt;

#ifdef FM_SUPPORT_SWAG
        /* Get the SWAG role */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_ROLE,
                      swIdx);
        GetConfigStrMap(buf,
                        &valInt,
                        FM_SWITCH_ROLE_UNDEFINED,
                        swagRole,
                        FM_NENTRIES(swagRole));
        swCfg->switchRole = valInt;
#endif

        /* Get the configure de-emphasis enable/disable flag */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_PHY_ENABLE_DEEMPHASIS,
                      swIdx);
        GetOptionalConfigInt(buf, 
                             &valInt,
                             FM_AAD_API_PLATFORM_PHY_ENABLE_DEEMPHASIS);
        swCfg->enablePhyDeEmphasis = valInt;

        /* PHY configuration must be loaded before port configuration as the
           number of PHYs (numPhys) is used below */
        status = LoadPhyConfiguration(swIdx);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);


        /* Initialize some of the EPL fields */
        for ( epl = 0 ; epl < FM_PLAT_NUM_EPL ; epl++ )
        {
            FM_MEMSET_S( swCfg->epls[epl].laneToPortIdx,
                         sizeof(swCfg->epls[epl].laneToPortIdx),
                         FM_PLAT_UNDEFINED,
                         sizeof(swCfg->epls[epl].laneToPortIdx) );
        }

        /**************************************************
         *
         * PORT CONFIGURATION
         *
         **************************************************/

        fmRootPlatform->cfg.switches[swIdx].ports =
            fmAlloc( swCfg->numPorts * sizeof(fm_platformCfgPort) );

        if (fmRootPlatform->cfg.switches[swIdx].ports == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
        }

        for (portIdx = 0 ; portIdx < swCfg->numPorts ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);

            portCfg->portIdx = portIdx;
            portCfg->portType = FM_PLAT_PORT_TYPE_NONE;
            portCfg->epl = FM_PLAT_UNDEFINED;
            portCfg->pep = FM_PLAT_UNDEFINED;
            portCfg->tunnel = FM_PLAT_UNDEFINED;
            portCfg->loopback = FM_PLAT_UNDEFINED;
            portCfg->phyNum = FM_PLAT_UNDEFINED;
            portCfg->phyPort = FM_PLAT_UNDEFINED;

            /* The physical port is the same as portIndex */
            portCfg->physPort = portIdx;

            for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
            {
                /* Default the lane ordering to one-to-one between
                   the QSFP lanes and the EPL lanes. */
                portCfg->lane[lane] = lane;
            }

            /**************************************************
             * Get the port mapping
             **************************************************/

            status = GetPortMapping(swIdx, portIdx);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            if (portCfg->port > swCfg->maxLogicalPortValue)
            {
                swCfg->maxLogicalPortValue = portCfg->port;
            }

            /* Make sure the CPU port is mapped to a PCIE port if the register
             * access needs it. */
            if (portCfg->port == swCfg->cpuPort)
            {
#ifndef FM_LT_WHITE_MODEL_SUPPORT
                if (portCfg->portType != FM_PLAT_PORT_TYPE_PCIE
#ifdef FM_SUPPORT_SWAG
                    && portCfg->swagLink.type != FM_SWAG_LINK_CPU
#endif
                   )
                {
                    if (swCfg->regAccess == FM_PLAT_REG_ACCESS_PCIE)
                    {
                        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                     "CPU port %d is not mapped to a PCIE port\n",
                                     swCfg->cpuPort);
                        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
                    }
                }
#endif
                swCfg->mgmtPep = portCfg->pep;
            }

            /**************************************************
             * Get the interface type
             **************************************************/

            GetStringValue(FM_AAD_API_PLATFORM_PORT_INTF_TYPE_DEFAULT,
                           intfTypeMap,
                           FM_NENTRIES(intfTypeMap),
                           &defVal);

            /* Get the default interface type */
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_INTF_TYPE_DEFAULT,
                          swIdx);
            GetConfigStrMap(buf,
                            &defVal,
                            defVal,
                            intfTypeMap,
                            FM_NENTRIES(intfTypeMap));

            /* Get this port interface type if it exists */
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_INTF_TYPE,
                          swIdx,
                          portIdx);
            GetConfigStrMap(buf,
                            &valInt,
                            defVal,
                            intfTypeMap,
                            FM_NENTRIES(intfTypeMap));

            portCfg->intfType = valInt;

            /**************************************************
             * Get resource ID
             **************************************************/

            FM_SNPRINTF_S(buf, 
                          MAX_BUF_SIZE, 
                          FM_AAK_API_PLATFORM_HW_RESOURCE_ID_DEFAULT, 
                          swIdx);
            defVal = portIdx;
            defVal = fmGetIntApiProperty(buf, defVal);

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_HW_RESOURCE_ID,
                          swIdx,
                          portIdx);
            GetOptionalConfigInt(buf, &valInt, defVal);
            portCfg->hwResourceId = valInt;

            /**************************************************
             * Get ethernet mode
             **************************************************/

            GetStringValue(FM_AAD_API_PLATFORM_PORT_ETHMODE_DEFAULT,
                           ethModeMap,
                           FM_NENTRIES(ethModeMap),
                           &defVal);
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_ETHMODE_DEFAULT,
                          swIdx);
            GetConfigStrMap(buf,
                            &defVal,
                            defVal,
                            ethModeMap,
                            FM_NENTRIES(ethModeMap) );
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_ETHMODE,
                          swIdx,
                          portIdx);
            GetConfigStrMap(buf,
                            &valInt,
                            defVal,
                            ethModeMap,
                            FM_NENTRIES(ethModeMap) );

            if (valInt == FM_ETHMODE_AUTODETECT)
            {
                portCfg->autodetect = TRUE;
                portCfg->ethMode = FM_ETH_MODE_DISABLED;
            }
            else
            {
                portCfg->autodetect = FALSE;
                portCfg->ethMode = valInt;
            }

            /**************************************************
             * Get port speed for scheduler BW allocation
             **************************************************/

            switch (portCfg->portType)
            {
                case FM_PLAT_PORT_TYPE_EPL:
                    /* Defaulting to 2500, but should always be equal
                     * or higher than default ethernet mode */
                    defVal = 2500;
                    break;

                case FM_PLAT_PORT_TYPE_PCIE:
                    /* Default to 50000, should be set to a maximum of 25G on
                     * a x4 PEP and 10G on a x1 PEP. */
                    defVal = 50000;
                    break;
                case FM_PLAT_PORT_TYPE_TUNNEL:
                    defVal = 100000;
                    break;
                case FM_PLAT_PORT_TYPE_LOOPBACK:
                    defVal = 25000;
                    break;
                case FM_PLAT_PORT_TYPE_FIBM:
                    defVal = 2500;
                    break;
                default:
                    defVal = 0;
            }

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_HW_PORT_SPEED,
                          swIdx,
                          portIdx);
            GetOptionalConfigInt(buf, &valInt, defVal);
            portCfg->speed = valInt;

            /**************************************************
             * Get port capabilities
             **************************************************/

            GetStringValue(FM_AAD_API_PLATFORM_PORT_CAPABILITY_DEFAULT,
                           portCapMap,
                           FM_NENTRIES(portCapMap),
                           &defVal);

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_CAPABILITY_DEFAULT,
                          swIdx);

            GetConfigStrBitMap(buf,
                               &defVal,
                               defVal,
                               portCapMap,
                               FM_NENTRIES(portCapMap));

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_CAPABILITY,
                          swIdx,
                          portIdx);

            GetConfigStrBitMap(buf,
                               &valInt,
                               defVal,
                               portCapMap,
                               FM_NENTRIES(portCapMap));
            portCfg->cap = valInt;

            /**************************************************
             * Get port DFE mode
             **************************************************/

            GetStringValue(FM_AAD_API_PLATFORM_PORT_DFE_MODE_DEFAULT,
                           dfeModeMap,
                           FM_NENTRIES(dfeModeMap),
                           &defVal);

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_DFE_MODE_DEFAULT,
                          swIdx);

            GetConfigStrMap(buf,
                            &defVal,
                            defVal,
                            dfeModeMap,
                            FM_NENTRIES(dfeModeMap));
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_DFE_MODE,
                          swIdx,
                          portIdx);

            GetConfigStrMap(buf,
                            &valInt,
                            defVal,
                            dfeModeMap,
                            FM_NENTRIES(dfeModeMap));
            portCfg->dfeMode = valInt;

            /**************************************************
             * Get AN73 abilities
             **************************************************/

            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_PORT_AN73_ABILITY,
                          swIdx,
                          portIdx);

            GetConfigStrBitMap(buf,
                               &valInt,
                               FM_PLAT_AN73_DEFAULT_ABILITIES,
                               an73AbilityMap,
                               FM_NENTRIES(an73AbilityMap));
            portCfg->an73Ability = valInt;
            portCfg->an73AbilityCfg = valInt;

            /**************************************************
             * Read the configuration related to EPL/lanes. 
             **************************************************/

            if (portCfg->portType == FM_PLAT_PORT_TYPE_EPL)
            {
                status = LoadEplProperties(swIdx, portIdx);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
            }

        }   /* end for (portIdx = 0 ; portIdx < swCfg->numPorts ; portIdx++) */

#ifdef FM_SUPPORT_SWAG
        for (internalPortIdx = 0 ; internalPortIdx < swCfg->numPorts ; internalPortIdx++)
        {
            FM_SNPRINTF_S(buf,
                          MAX_BUF_SIZE,
                          FM_AAK_API_PLATFORM_INTERNAL_PORT_MAPPING,
                          swIdx,
                          internalPortIdx);

            valText = fmGetTextApiProperty(buf, NULL);
            if (valText == NULL)
            {
                break;
            }

            /* Get first SWAG logical port number*/
            status = GetAndValidateObjectValue(valText,
                                               "SWAG=",
                                               "logical port",
                                               &valInt,
                                               0,
                                               FM_PLAT_MAX_LOGICAL_PORT);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Get second SWAG logical port number*/
            status = GetAndValidateObjectValue(valText + 5,
                                               "SWAG=",
                                               "logical port",
                                               &valInt2,
                                               0,
                                               FM_PLAT_MAX_LOGICAL_PORT);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            portCfg  = FM_PLAT_GET_PORT_CFG(swIdx, valInt);
            portCfg2 = FM_PLAT_GET_PORT_CFG(swIdx, valInt2);

            portCfg->swagLink.type = FM_SWAG_LINK_INTERNAL;
            portCfg->swagLink.partnerLogicalPort = valInt2;
            portCfg->swagLink.partnerSwitch = portCfg2->swagLink.swId;
            portCfg->swagLink.partnerPort = portCfg2->swagLink.swPort;

            portCfg2->swagLink.type = FM_SWAG_LINK_INTERNAL;
            portCfg2->swagLink.partnerLogicalPort = valInt;
            portCfg2->swagLink.partnerSwitch = portCfg->swagLink.swId;
            portCfg2->swagLink.partnerPort = portCfg->swagLink.swPort;
        }
#endif

        libCfg = FM_PLAT_GET_LIBS_CFG(swIdx);
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_SHARED_LIB_NAME,
                      swIdx);
        valText = fmGetTextApiProperty(buf, "");
        len     = (valText == NULL) ? 0 : strlen(valText);

        if (len)
        {
            FM_STRNCPY_S(libCfg->libName,
                         FM_PLAT_MAX_CFG_STR_LEN,
                         valText,
                         len + 1);
        }

        /* Optional features supported by shared library */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_SHARED_LIB_DISABLE,
                      swIdx);

        status = GetConfigStrBitMap(buf,
                                    &valInt,
                                    0,
                                    disableFuncIntfMap,
                                    FM_NENTRIES(disableFuncIntfMap));
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        libCfg->disableFuncIntf = valInt;

        /* Load the cold boot config properties */
        status = fm10000LoadBootCfg(swIdx, &swCfg->bootCfg);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);


        /* Get the /dev/mem offset */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_DEVMEM_OFFSET,
                      swIdx);

        valText = fmGetTextApiProperty(buf, 
                                       FM_AAD_API_PLATFORM_DEVMEM_OFFSET);

        FM_STRNCPY_S(swCfg->devMemOffset,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     valText,
                     FM_PLAT_MAX_CFG_STR_LEN);

        /* Get the raw socket netDev name */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_NETDEV_NAME,
                      swIdx);

        devName = fmGetTextApiProperty(buf, 
                                       FM_AAD_API_PLATFORM_NETDEV_NAME);

        FM_STRNCPY_S(swCfg->netDevName,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     devName,
                     FM_PLAT_MAX_CFG_STR_LEN);

        /* Get the uio device name */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_UIO_DEV_NAME,
                      swIdx);

        devName = fmGetTextApiProperty(buf, 
                                       FM_AAD_API_PLATFORM_UIO_DEV_NAME);

        FM_STRNCPY_S(swCfg->uioDevName,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     devName,
                     FM_PLAT_MAX_CFG_STR_LEN);

        /* Get the voltage regulator hardware resource ID */
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_VRM_HW_RESOURCE_ID,
                      swIdx,
                      "VDDS");
        GetOptionalConfigInt(buf, &valInt, -1);
        swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDS] = valInt;

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_VRM_HW_RESOURCE_ID,
                      swIdx,
                      "VDDF");
        GetOptionalConfigInt(buf, &valInt, -1);
        swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDF] = valInt;

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_VRM_HW_RESOURCE_ID,
                      swIdx,
                      "AVDD");
        GetOptionalConfigInt(buf, &valInt, -1);
        swCfg->vrm.hwResourceId[FM_PLAT_VRM_AVDD] = valInt;
    }   /* end  for (swIdx = 0 ; swIdx < platCfg->numSwitches ; swIdx++) */

    if (platCfg->debug & CFG_DBG_CONFIG)
    {
        fmPlatformCfgDump();
    }

    return FM_OK;

}   /* end fmPlatformCfgLoad */




/*****************************************************************************/
/* fmPlatformCfgReloadSpeed
 * \ingroup intPlatform
 *
 * \desc            Reload the per-port speed properties. The per-port
 *                  speed can change at run time to be reloaded during
 *                  a switch soft reset. This can be useful for updating the
 *                  scheduler bandwidth for a given set of ports. 
 *                                                                      \lb\lb
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformCfgReloadSpeed(fm_int sw)
{
    fm_status           err = FM_OK;
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_char             buf[MAX_BUF_SIZE+1];
    fm_int              defVal;
    fm_int              valInt;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (fmRootPlatform->cfg.switches[sw].ports == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

        /**************************************************
         * Get port speed for scheduler BW allocation
         **************************************************/

        switch (portCfg->portType)
        {
            case FM_PLAT_PORT_TYPE_EPL:
                /* Defaulting to 2500, but should always be equal
                 * or higher than default ethernet mode */
                defVal = 2500;
                break;

            case FM_PLAT_PORT_TYPE_PCIE:
                /* Default to 50000, should be set to a maximum of 25G on
                 * a x4 PEP and 10G on a x1 PEP. */
                defVal = 50000;
                break;
            case FM_PLAT_PORT_TYPE_TUNNEL:
                defVal = 100000;
                break;
            case FM_PLAT_PORT_TYPE_LOOPBACK:
                defVal = 25000;
                break;
            case FM_PLAT_PORT_TYPE_FIBM:
                defVal = 2500;
                break;
            default:
                defVal = 0;
        }

        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_HW_PORT_SPEED,
                      sw,
                      portIdx);
        GetOptionalConfigInt(buf, &valInt, defVal);
        portCfg->speed = valInt;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformCfgReloadSpeed */




/*****************************************************************************/
/* fmPlatformCfgSwitchGet
 * \ingroup intPlatform
 *
 * \desc            Return the fm_platformCfgSwitch structure of the given
 *                  switch.
 *                                                                      \lb\lb
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
 *                                                                      \lb\lb
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
 *                                                                      \lb\lb
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
 *                                                                      \lb\lb
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
 *                                                                      \lb\lb
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          the scheduler configuration mode for the specified switch.
 *
 *****************************************************************************/
fm_schedulerConfigMode fmPlatformGetSchedulerConfigMode(fm_int sw)
{
    fm_char        buf[MAX_BUF_SIZE+1];
    fm_int         valInt;


    FM_SNPRINTF_S(buf, MAX_BUF_SIZE, FM_AAK_API_PLATFORM_SCHED_LIST_SELECT, sw);
    GetOptionalConfigInt(buf, &valInt, FM_AAD_API_PLATFORM_SCHED_LIST_SELECT);

    if (valInt == FM_AAD_API_PLATFORM_SCHED_LIST_SELECT)
    {
        return (FM_SCHED_INIT_MODE_AUTOMATIC); 
    }
    else 
    {
        return (FM_SCHED_INIT_MODE_MANUAL);
    }

}   /* end fmPlatformGetSchedulerConfigMode */



/*****************************************************************************/
/* fmPlatformCfgSchedulerGetTokenList
 * \ingroup intPlatform
 *
 * \desc            Configure the scheduler token list for manual mode based
 *                  on the given switch.
 *                                                                      \lb\lb
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
    fm_char        buf[MAX_BUF_SIZE+1];
    fm_int         listNbr;
    fm_int         listLength;
    fm_int         count = 0;
    fm_int         valInt;
    fm_int         slotIdx;
    fm_text        valText;
    fm_status      status;


    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if (schedTokenList == NULL)
    {
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }


    /* Get the scheduler list number */
    FM_SNPRINTF_S(buf, MAX_BUF_SIZE, FM_AAK_API_PLATFORM_SCHED_LIST_SELECT, sw);
    status = GetRequiredConfigInt(buf, &listNbr, 0, 15);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);


    /* Look for the scheduler list length.  Note that it has to fit in the
       switch scheduler array registers.  This array consist of 2 port lists,
       an active one and a standby one.  This is why we divide the entries
       count by 2.  We also have to add 1 for the implicit idle entry, which
       will be present in the token list but not in the switch scheduler array
       registers */
    FM_SNPRINTF_S(buf, MAX_BUF_SIZE, FM_AAK_API_PLATFORM_SCHED_LIST_LENGTH, sw, listNbr);
    status = GetRequiredConfigInt(buf,
                                  &listLength,
                                  1,
                                  (FM10000_SCHED_RX_SCHEDULE_ENTRIES / 2) + 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);    


    /* Scan all slot entry and generate appropriate scheduler tokens */
    for (slotIdx = 0; slotIdx < listLength; slotIdx++)
    {
        FM_SNPRINTF_S(buf,
                      MAX_BUF_SIZE,
                      FM_AAK_API_PLATFORM_SCHED_TOKEN,
                      sw,
                      listNbr,
                      slotIdx);

        valText = fmGetTextApiProperty(buf, NULL);
        if (valText == NULL)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Required text property for '%s' is missing\n",
                         buf);
            FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }

        /* Parse the scheduler list slot property line,
           start by the port index */
        status = GetAndValidateObjectValue(valText,
                                           "PINDEX=",
                                           "port index",
                                           &schedTokenList[slotIdx].port,
                                           0,
                                           FM10000_MAX_PORT);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Parse the quad port flag */
        status = GetAndValidateObjectValue(valText,
                                           "Q=",
                                           "quad port flag",
                                           &valInt,
                                           0,
                                           1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        if (valInt == 0)
        {
            schedTokenList[slotIdx].quad = FALSE;
        }
        else
        {
            schedTokenList[slotIdx].quad = TRUE;
        }

        /* Parse the idle flag */
        status = GetAndValidateObjectValue(valText,
                                           "I=",
                                           "index flag",
                                           &valInt,
                                           0,
                                           1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status); 

        if (valInt == 0)
        {
            schedTokenList[slotIdx].idle = FALSE;
        }
        else
        {
            schedTokenList[slotIdx].idle = TRUE;
        }


        /* Look for 'EPL=' string */
        if ( strstr(valText, "EPL=") != NULL )
        {
            /* Get epl number */
            status = GetAndValidateObjectValue(valText,
                                               "EPL=",
                                               "epl",
                                               &schedTokenList[slotIdx].fabricPort,
                                               0,
                                               FM10000_MAX_EPL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* Get lane number */
            status = GetAndValidateObjectValue(valText,
                                               "LANE=",
                                               "lane",
                                               &valInt,
                                               0,
                                               FM_PLAT_LANES_PER_EPL-1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            schedTokenList[slotIdx].fabricPort =
                FM10000_EPL_LANE_TO_FABRIC_PORT(
                   (schedTokenList[slotIdx].fabricPort),
                   valInt);
        }

        /* Look for 'PCIE=' string */
        else if ( strstr(valText, "PCIE=") != NULL )
        {
            /* Get PCIE number */
            status = GetAndValidateObjectValue(valText,
                                               "PCIE=",
                                               "PCIE",
                                               &valInt,
                                               0,
                                               FM10000_MAX_PEP);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            if (valInt == FM10000_MAX_PEP)
            {
                schedTokenList[slotIdx].fabricPort =
                    FM10000_PCIE8_TO_FABRIC_PORT;
            }
            else
            {
                schedTokenList[slotIdx].fabricPort =
                    FM10000_PCIE_TO_FABRIC_PORT(valInt);
            }
        }

        /* Look for 'TE=' string */
        else if ( strstr(valText, "TE=") != NULL )
        {
            /* Get TUNNEL number */
            status = GetAndValidateObjectValue(valText,
                                               "TE=",
                                               "TE",
                                               &valInt,
                                               0,
                                               FM_PLAT_NUM_TUNNEL-1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            schedTokenList[slotIdx].fabricPort =
                FM10000_TE_TO_FABRIC_PORT(valInt);
        }

        /* Look for 'LOOPBACK=' string */
        else if ( strstr(valText, "LPBK=") != NULL )
        {
            /* Get LOOPBACK number */
            status = GetAndValidateObjectValue(valText,
                                               "LPBK=",
                                               "LPBK",
                                               &valInt,
                                               0,
                                               FM_PLAT_NUM_LOOPBACK-1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            schedTokenList[slotIdx].fabricPort =
                FM10000_LOOPBACK_TO_FABRIC_PORT(valInt);
        }

        /* Look for 'FIBM' string */
        else if ( strstr(valText, "FIBM") != NULL )
        {
            schedTokenList[slotIdx].fabricPort =
                FM10000_FIBM_TO_FABRIC_PORT;
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Unrecongnized scheduler token"
                         "type property line '%s'=%s\n",
                         buf,
                         valText);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
        }
    }

    count = listLength;


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, count);

}   /* end fmPlatformCfgSchedulerGetTokenList */

