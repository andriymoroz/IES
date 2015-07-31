/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_serdes_reg.c
 * Creation Date:   October 23, 2013
 * Description:     Provide debugging functions for accessing SERDES
 *                  register fields by name.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#include "fm_sdk_fm10000_int.h"

#include "debug/fm10000/fm10000_debug_regs_int.h"

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FIELD_FORMAT_B  "    %-20s: %d\n"
#define FIELD_FORMAT_F  "    %-20s: %d 0x%x\n"
#define FIELD_FORMAT_S  "    %-20s: %s\n"


#define MAX_STR_LEN     80

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_char invalidStr[MAX_STR_LEN]; /* Does not support multi thread */

/*****************************************************************************
 * Local Functions
 *****************************************************************************/



/*****************************************************************************/
/** fm10000SerdesGetRegName
 * \ingroup intRegs
 *
 * \desc            Return register name given register offset
 *
 * \param[in]       regOff is the register offset.
 *
 * \return          register name string.
 *
 *****************************************************************************/
fm_text fm10000SerdesGetRegName(fm_uint regOff)
{
    fm_int          sw;
    fm_text         regStr;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    

    sw = 0;
    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    regStr = "Register";


    if (serdesPtr->magicNumber == FM10000_SERDES_STRUCT_MAGIG_NUMBER &&
        serdesPtr->dbgGetRegName != NULL)
    {
        regStr = serdesPtr->dbgGetRegName(regOff);
    }

    return regStr;

}   /* end fm10000SerdesGetRegName */




/*****************************************************************************/
/** fm10000SerdesGetRegFields
 * \ingroup intRegs
 *
 * \desc            Return null-terminate fmRegisterField structure
 *                  containing all the fields of the given register.
 *
 * \param[in]       regOff is the register offset.
 *
 * \return          null-terminate fmRegisterField structure.
 *
 *****************************************************************************/
const fmRegisterField  *fm10000SerdesGetRegFields(fm_uint regOff)
{  
    fm_int           sw;
    fmRegisterField *pField;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    

    sw = 0;
    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    pField = NULL;


    if (serdesPtr->magicNumber == FM10000_SERDES_STRUCT_MAGIG_NUMBER &&
        serdesPtr->dbgGetRegFields != NULL)
    {
        pField = (fmRegisterField*)(serdesPtr->dbgGetRegFields(regOff));
    }

    return pField;

}   /* end fm10000SerdesGetRegFields */




/*****************************************************************************/
/** fm10000SerdesSpicoIntrGetRegName
 * \ingroup intRegs
 *
 * \desc            Return register name given register offset
 *
 * \param[in]       regOff is the register offset.
 *
 * \return          register name string.
 *
 *****************************************************************************/
fm_text fm10000SerdesSpicoIntrGetRegName(fm_uint regOff)
{
    fm_int          sw;
    fm_text         regStr;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    

    sw = 0;
    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    regStr = "Interrupt";


    if (serdesPtr->magicNumber == FM10000_SERDES_STRUCT_MAGIG_NUMBER &&
        serdesPtr->dbgGetSpicoIntrRegName != NULL)
    {
        regStr = serdesPtr->dbgGetSpicoIntrRegName(regOff);
    }

    return regStr;

}   /* end fm10000SerdesSpicoIntrGetRegName */




/*****************************************************************************/
/** fm10000SerdesSpicoIntrGetRegFields
 * \ingroup intRegs
 *
 * \desc            Return null-terminate fmRegisterField structure
 *                  containing all the fields of the given register.
 *
 * \param[in]       regOff is the register offset.
 *
 * \return          null-terminate fmRegisterField structure.
 *
 *****************************************************************************/
const fmRegisterField  *fm10000SerdesSpicoIntrGetRegFields(fm_uint regOff)
{
    fm_int           sw = 0;
    fmRegisterField *pField;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;

    sw = 0;
    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    pField = NULL;


    if (serdesPtr->magicNumber == FM10000_SERDES_STRUCT_MAGIG_NUMBER &&
        serdesPtr->dbgGetSpicoIntrRegFields != NULL)
    {
        pField = (fmRegisterField*)(serdesPtr->dbgGetSpicoIntrRegFields(regOff));
    }

    return pField;

}   /* end fm10000SerdesSpicoIntrGetRegFields */




/*****************************************************************************/
/** fm10000SpicoSerdesRegIsRead
 * \ingroup intSeredes
 *
 * \desc            Return TRUE if interrupt number is a read interrupt.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \return          TRUE is interrupt is a read interrupt.
 *
 *****************************************************************************/
fm_bool fm10000SpicoSerdesRegIsRead(fm_int intNum)
{

    switch (intNum)
    {
        case 0x00:
        case 0x3F:
            return TRUE;
    }

    return FALSE;

}   /* end fm10000SpicoSerdesRegIsRead */




/*****************************************************************************/
/** fm10000SBusGetIpIdCodeStr
 * \ingroup intSeredes
 *
 * \desc            Return TRUE if interrupt number is a read interrupt.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \return          TRUE is interrupt is a read interrupt.
 *
 *****************************************************************************/
static fm_text fm10000SBusGetIpIdCodeStr(fm_uint value)
{
    fm_int          sw;
    fm_text         regStr;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;
    

    sw = 0;
    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    FM_SNPRINTF_S(invalidStr, MAX_STR_LEN, "DEVICE(%d)", value);

    regStr = invalidStr;


    if (serdesPtr->magicNumber == FM10000_SERDES_STRUCT_MAGIG_NUMBER &&
        serdesPtr->dbgGetIpIdCodeStr != NULL)
    {
        regStr = serdesPtr->dbgGetIpIdCodeStr(value);
    }

    return regStr;

}   /* end fm10000SBusGetIpIdCodeStr */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/** fm10000DumpEplSerdesRegFields
 * \ingroup intSerdes
 *
 * \desc            Dump out each field of the given EPL SERDES register.
 *
 * \param[in]       regOff is the register offset.
 *
 * \param[out]      value is the register value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DumpEplSerdesRegFields(fm_int regOff, fm_uint32 value)
{
    fm_status       err;
    fm_int          i;
    fm_uint32       fieldVal;
    fm_int          size;

    const fmRegisterField *fields;

    err = FM_FAIL;

    FM_LOG_PRINT("%s[%d] = 0x%02x\n", fm10000SerdesGetRegName(regOff), regOff, value);

    fields = fm10000SerdesGetRegFields(regOff);

    if (fields == NULL)
    {
        /* just ignore field information */
        err = FM_OK;
    }
    else
    {
        for (i = 0 ; ; i++)
        {
            if (fields[i].name == NULL)
            {
                err = FM_OK;  /* end of list */
                break;
            }
            
            size = fields[i].size;
            if (size <= 32 && size > 0)
            {
                fieldVal = ( (value >> fields[i].start ) &  ( 0xffffffff >> (32-size)) );
    
                if (regOff == FM10000_SERDES_REG_FF &&
                    (strstr(fields[i].name, "IP_IDCODE") != NULL))
                {
                    FM_LOG_PRINT(FIELD_FORMAT_S, fields[i].name, fm10000SBusGetIpIdCodeStr(fieldVal));
                }
                else if (size == 1)
                {
                    FM_LOG_PRINT(FIELD_FORMAT_B, fields[i].name, (fm_int) fieldVal);
                }
                else
                {
                    FM_LOG_PRINT(FIELD_FORMAT_F, fields[i].name, fieldVal, fieldVal);
                }
            }
            else
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, fields[i].name, "No support for over 32-bit field");
            }
        }
    }

    return err;

}   /* end fm10000DumpEplSerdesRegFields */




/*****************************************************************************/
/** fm10000DumpPcieSerdesRegFields
 * \ingroup intSerdes
 *
 * \desc            Dump out each field of the given PCIe SERDES register.
 *
 * \param[in]       regOff is the register offset.
 *
 * \param[out]      value is the register value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DumpPcieSerdesRegFields(fm_int regOff, fm_uint32 value)
{
    fm_status       err;
    fm_int          i;
    fm_uint32       fieldVal;
    fm_int          size;

    const fmRegisterField *fields;

    err = FM_FAIL;


    FM_LOG_PRINT("PCIE: %s[%d] = 0x%02x\n", fm10000SerdesGetRegName(regOff), regOff, value);

    fields = fm10000SerdesGetRegFields(regOff);

    if (fields == NULL)
    {
        /* just ignore field information */
        err = FM_OK;
    }
    else
    {
    
        for (i = 0 ; ; i++)
        {
            if (fields[i].name == NULL)
            {
                err = FM_OK;  /* end of list */
                break;
            }
    
            size = fields[i].size;
            if (size <= 32 && size > 0)
            {
                fieldVal = ( (value >> fields[i].start ) &  ( 0xffffffff >> (32-size)) );
    
                if (regOff == FM10000_SERDES_REG_FF &&
                    (strstr(fields[i].name, "IP_IDCODE") != NULL))
                {
                    FM_LOG_PRINT(FIELD_FORMAT_S, fields[i].name, fm10000SBusGetIpIdCodeStr(fieldVal));
                }
                else if (size == 1)
                {
                    FM_LOG_PRINT(FIELD_FORMAT_B, fields[i].name, (fm_int) fieldVal);
                }
                else
                {
                    FM_LOG_PRINT(FIELD_FORMAT_F, fields[i].name, fieldVal, fieldVal);
                }
            }
            else
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, fields[i].name, "No support for over 32-bit field");
            }
        }
    }

    return err;

}   /* end fm10000DumpPcieSerdesRegFields */




/*****************************************************************************/
/** fm10000DumpSpicoSerdesRegFields
 * \ingroup intSeredes
 *
 * \desc            Dump out each field of the given SPICO SERDES register.
 *
 * \param[in]       intNum is the interrupt number.
 *
 * \param[out]      param is the input argument.
 *
 * \param[out]      value is the return value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000DumpSpicoSerdesRegFields(fm_int intNum, fm_uint32 param, fm_uint32 value)
{
    fm_status       err;
    fm_int          i;
    fm_uint32       fieldVal;
    fm_uint32       val;
    fm_int          size;

    const fmRegisterField *fields;
    err = FM_FAIL;

    FM_LOG_PRINT("%s[%d,0x%x] = 0x%02x\n", fm10000SerdesSpicoIntrGetRegName(intNum), intNum, param, value);

    fields = fm10000SerdesSpicoIntrGetRegFields(intNum);

    val = fm10000SpicoSerdesRegIsRead(intNum) ? value : param;

    if (fields == NULL)
    {
        /* just ignore field information */
        err = FM_OK;
    }
    else
    {
    
        for (i = 0 ; ; i++)
        {
            if (fields[i].name == NULL)
            {
                 /* end of list */
                err = FM_OK; 
                break;
            }
    
            size = fields[i].size;;
            if (size <= 32 && size > 0)
            {
                fieldVal = ( (val >> fields[i].start ) &  ( 0xffffffff >> (32-size)) );
    
                if (size == 1)
                {
                    FM_LOG_PRINT(FIELD_FORMAT_B, fields[i].name, (fm_int) fieldVal);
                }
                else
                {
                    FM_LOG_PRINT(FIELD_FORMAT_F, fields[i].name, fieldVal, fieldVal);
                }
            }
            else
            {
                FM_LOG_PRINT(FIELD_FORMAT_S, fields[i].name, "No support for over 32-bit field.");
            }
        }
    }

    return err;

}   /* end fm10000DumpSpicoSerdesRegFields */




/*****************************************************************************/
/**  fm10000SerdesInitXDebugServicesInt
 * \ingroup intSerdes
 *
 * \desc            Performs the initialization of extended debug services.
 *                  In particular, this function initializes the access to
 *                  the debug functions that provide the names of the serdes
 *                  related registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerdesInitXDebugServicesInt(fm_int sw)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_SERDES, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;
    err = FM_OK;

    serdesPtr->dbgGetRegName            = NULL;
    serdesPtr->dbgGetRegFields          = NULL;
    serdesPtr->dbgGetSpicoIntrRegName   = NULL;
    serdesPtr->dbgGetSpicoIntrRegFields = NULL;
    serdesPtr->dbgGetIpIdCodeStr        = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SerdesInitXDebugServicesInt */





