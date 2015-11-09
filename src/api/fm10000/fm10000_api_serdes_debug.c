/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_serdes_debug.c
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


/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000DbgDumpSerDes
 * \ingroup intSerdes
 *
 * \desc            Dump debug info for a given SERDES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       cmd is the command to execude..
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpSerDes(fm_int  sw,
                               fm_int  serDes,
                               fm_text cmd)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (cmd == NULL)
    {
        err =  FM_ERR_INVALID_ARGUMENT;
    }
    else if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (STR_EQ(cmd, "status"))
    {
        if (serdesPtr->dbgDumpStatus == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpStatus(sw,serDes);
    }
    else if (STR_EQ(cmd, "imageVersion"))
    {
        if (serdesPtr->dbgDumpSpicoSbmVersions == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpSpicoSbmVersions(sw,serDes);
    }
    else if (STR_EQ(cmd, "serdesRegs"))
    {
        if (serdesPtr->dbgDumpRegisters == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpRegisters(sw,serDes);
    }
    else if (STR_EQ(cmd, "krStatus"))
    {
        if (serdesPtr->dbgDumpKrStatus == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpKrStatus(sw,serDes);
    }
    else if (STR_EQ(cmd, "dfeStatus"))
    {
        if (serdesPtr->dbgDumpDfeStatus == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpDfeStatus(sw,serDes, FALSE);
    }
    else if (STR_EQ(cmd, "dfeStatusDetailed"))
    {
        if (serdesPtr->dbgDumpDfeStatus == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgDumpDfeStatus(sw,serDes, TRUE);
    }
    else if (STR_EQ(cmd, "resetStats"))
    {
        if (serdesPtr->dbgResetStats == NULL)
        {
            return FM_ERR_UNSUPPORTED;
        }
        err = serdesPtr->dbgResetStats(sw,serDes);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_PRINT("Valid serdes dump commands:\n"
                    "    status, imageVersion, serdesRegs, krStatus, and dfeStatus\n");
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerdesDump
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
fm_status fm10000SerdesDump(fm_int sw, fm_int serdes, fm_bool detailed)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgDump != NULL)
    {
        err = serdesPtr->dbgDump(sw,serdes,detailed);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSerdesInit
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
fm_status fm10000DbgSerdesInit(fm_int  sw,
                               fm_int  serDes,
                               fm_uint dataWidth,
                               fm_uint rateSel)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgInit != NULL)
    {
        err = serdesPtr->dbgInit(sw, serDes, dataWidth, rateSel);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSerdesRunDfeTuning
 * \ingroup intSerdes
 *
 * \desc            Run DFE tuning on the serdes, only for debug or test mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       dfeMode is DFE mode, see fm_dfeMode.
 *
 * \param[in]       dfeHf is the HF parameter, set to -1 to use the default
 *                  value.
 *
 * \param[in]       dfeLf is the LF parameter, set to -1 to use the default
 *                  value.
 *
 * \param[in]       dfeDc is the DC parameter, set to -1 to use the default
 *                  value.
 *
 * \param[in]       dfeBw is the BW parameter, set to -1 to use the default
 *                  value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSerdesRunDfeTuning(fm_int        sw,
                                       fm_int        serDes,
                                       fm_dfeMode    dfeMode,
                                       fm_int        dfeHf,
                                       fm_int        dfeLf,
                                       fm_int        dfeDc,
                                       fm_int        dfeBw)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgRunDfeTuning != NULL)
    {
        err = serdesPtr->dbgRunDfeTuning(sw, serDes, dfeMode, dfeHf, dfeLf, dfeDc, dfeBw);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgResetSerDes
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
fm_status fm10000DbgResetSerDes(fm_int sw,
                                fm_int port)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgResetSerdes != NULL)
    {
        err = serdesPtr->dbgResetSerdes(sw,port);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgReadSerDesRegister
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
fm_status fm10000DbgReadSerDesRegister(fm_int     sw,
                                       fm_int     serDes,
                                       fm_uint    regAddr,
                                       fm_uint32 *pValue)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgReadRegister != NULL)
    {
        err = serdesPtr->dbgReadRegister(sw, serDes, regAddr, pValue);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgWriteSerDesRegister
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
fm_status fm10000DbgWriteSerDesRegister(fm_int     sw,
                                        fm_int     serDes,
                                        fm_uint    regAddr,
                                        fm_uint32  value)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgWriteRegister != NULL)
    {
        err = serdesPtr->dbgWriteRegister(sw, serDes, regAddr, value);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSetSerDesTxPattern
 * \ingroup intSerDes
 *
 * \desc            Configure SerDes Tx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       patternStr is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSetSerDesTxPattern(fm_int  sw,
                                       fm_int  serDes,
                                       fm_text patternStr)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetTxPattern != NULL)
    {
        err = serdesPtr->dbgSetTxPattern(sw, serDes, patternStr);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSetSerDesRxPattern
 * \ingroup intSerDes
 *
 * \desc            Configure SerDes Rx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       patternStr is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSetSerDesRxPattern(fm_int  sw,
                                       fm_int  serDes,
                                       fm_text patternStr)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetRxPattern != NULL)
    {
        err = serdesPtr->dbgSetRxPattern(sw,serDes,patternStr);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSetSerdesPolarity
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
fm_status fm10000DbgSetSerdesPolarity(fm_int  sw,
                                      fm_int  serDes,
                                      fm_text polarityStr)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetPolarity != NULL)
    {
        err = serdesPtr->dbgSetPolarity(sw,serDes,polarityStr);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSetSerdesLoopback
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
fm_status fm10000DbgSetSerdesLoopback(fm_int  sw,
                                      fm_int  serDes,
                                      fm_text loopbackStr)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;


    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetLoopback != NULL)
    {
        err = serdesPtr->dbgSetLoopback(sw,serDes,loopbackStr);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSerdesInjectErrors
 * \ingroup intSerdes
 *
 * \desc            Inject the given number of errors in the specified serdes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       serdesSel specified the where to inject errors: Tx, Rx or
 *                  TxRx. See 'fm10000SerdesSelect'.
 *
 * \param[in]       numErrors the number of errors to be injected.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSerdesInjectErrors(fm_int   sw,
                                       fm_int   serDes,
                                       fm_int   serdesSel,
                                       fm_uint  numErrors)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;


    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgInjectErrors != NULL)
    {
        err = serdesPtr->dbgInjectErrors(sw,serDes,serdesSel,numErrors);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgReadSBusRegister
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
 * \param[in]       writeReg is not applicable for this chip family.
 *
 * \param[out]      pValue is a pointer to a caller-allocated area where this
 *                  function will return the content of the register
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgReadSBusRegister(fm_int     sw,
                                     fm_int     sbusDevID,
                                     fm_int     devRegID,
                                     fm_bool    writeReg,
                                     fm_uint32 *pValue)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;


    FM_NOT_USED(writeReg);

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgReadSbusRegister != NULL)
    {
        err = serdesPtr->dbgReadSbusRegister(sw,sbusDevID,devRegID,pValue);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgWriteSBusRegister
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
fm_status fm10000DbgWriteSBusRegister(fm_int     sw,
                                      fm_int     sbusDevID,
                                      fm_int     devRegID,
                                      fm_uint32  value)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgWriteSbusRegister != NULL)
    {
        err = serdesPtr->dbgWriteSbusRegister(sw,sbusDevID,devRegID,value);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgInterruptSpico
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
fm_status fm10000DbgInterruptSpico(fm_int      sw,
                                   fm_int      cmd,
                                   fm_int      param,
                                   fm_int      timeout,
                                   fm_uint32  *pResult)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgInterruptSpico != NULL)
    {
        err = serdesPtr->dbgInterruptSpico(sw,cmd,param,timeout,pResult);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSerDesSetDfeParameter
 * \ingroup intSerDes
 *
 * \desc            Set, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector to paramValue
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       paramSelector is the selector of the parameter to be set
 *
 * \param[in]       paramValue is the value to be set on the selected parameter
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSerDesSetDfeParameter(fm_int      sw,
                                          fm_int      serDes,
                                          fm_uint32   paramSelector,
                                          fm_uint32   paramValue)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetDfeParameter != NULL)
    {
        err = serdesPtr->dbgSetDfeParameter(sw, serDes, paramSelector, paramValue);
    }

    return err;

}




/*****************************************************************************/
/** fm10000DbgSerDesGetDfeParameter
 * \ingroup intSerDes
 *
 * \desc            Get, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       paramSelector is the selector of the parameter to be get
 *
 * \param[in]       pParamValue is a pointer to a caller-allocated area where
 *                  this function will return the value of the selected
 *                  parameter.If NULL, the parameter will be just printed.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DbgSerDesGetDfeParameter(fm_int      sw,
                                          fm_int      serDes,
                                          fm_uint32   paramSelector,
                                          fm_uint32 * pParamValue)
{
    fm_status        err;
    fm10000_serdes  *serdesPtr;
    fm10000_switch  *switchExt;


    switchExt = GET_SWITCH_EXT(sw);
    serdesPtr = &switchExt->serdesXServices;

    err = FM_ERR_UNSUPPORTED;

    if (serdesPtr->magicNumber != FM10000_SERDES_STRUCT_MAGIG_NUMBER)
    {

        err = FM_ERR_UNINITIALIZED;
    }
    else if (serdesPtr->dbgSetDfeParameter != NULL)
    {
        err = serdesPtr->dbgGetDfeParameter(sw, serDes, paramSelector, pParamValue);
    }

    return err;

}


