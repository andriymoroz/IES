/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_serdes.c
 * Creation Date:   May 29, 2014
 * Description:     Provide SERDES debugging functions.
 *
 * Copyright (c) 2014, Intel Corporation
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmDbgInitSerDes
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Initialize specified SERDES, only for debug or test mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number.
 *
 * \param[in]       dataWidth is the data with mode of either 10, 20, or 40.
 *
 * \param[in]       rateSel is the SERDES rate and ratio.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgInitSerDes(fm_int sw,
                          fm_int serDes,
                          fm_uint dataWidth,
                          fm_uint rateSel)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgInitSerDes,
                       sw,
                       serDes,
                       dataWidth,
                       rateSel);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgInitSerDes */




/*****************************************************************************/
/** fmDbgRunSerDesDfeTuning
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Initialize specified SERDES, only for debug or test mode.
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
fm_status fmDbgRunSerDesDfeTuning(fm_int        sw,
                                  fm_int        serDes,
                                  fm_dfeMode    dfeMode,
                                  fm_int        dfeHf,
                                  fm_int        dfeLf,
                                  fm_int        dfeDc,
                                  fm_int        dfeBw)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgRunSerDesDfeTuning,
                       sw,
                       serDes,
                       dfeMode,
                       dfeHf,
                       dfeLf,
                       dfeDc,
                       dfeBw);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgRunSerDesDfeTuning */



/*****************************************************************************/
/** fmDbgReadSerDesRegister 
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Read a value from a SerDes register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       reg is the register address
 *
 * \param[out]      value is a pointer to a caller-allocated area where this
 *                  function will return the content of the register 
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmDbgReadSerDesRegister(fm_int     sw, 
                                  fm_int     serDes,
                                  fm_int     reg, 
                                  fm_uint32 *value)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);

    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgReadSerDesRegister,
                       sw,
                       serDes,
                       reg,
                       value);

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgReadSerDesRegister */




/*****************************************************************************/
/** fmDbgWriteEthSerDesRegister 
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Write a value to a SerDes register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       reg is the register address
 *
 * \param[in]       value is the value to be written to the register
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgWriteSerDesRegister(fm_int     sw, 
                                   fm_int     serDes, 
                                   fm_uint    reg, 
                                   fm_uint32  value)
{
    fm_status  err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);

    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",  sw);
        return err;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgWriteSerDesRegister,
                        sw,
                        serDes,
                        reg,
                        value);

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteSerDesRegister */







/*****************************************************************************/
/** fmDbgDumpSerDes
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Dump debug information on a SerDes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       cmd is chip specific command to execute.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgDumpSerDes(fm_int  sw, 
                          fm_int  serDes,
                          fm_text cmd)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgDumpSerDes,
                       sw,
                       serDes,
                       cmd);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgDumpSerDes */





/*****************************************************************************/
/** fmDbgSetSerDesTxPattern
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Configure SerDes Tx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       pattern is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesTxPattern(fm_int  sw,
                                  fm_int  serDes,
                                  fm_text pattern)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesTxPattern,
                       sw,
                       serDes,
                       pattern);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesTxPattern */



/*****************************************************************************/
/** fmDbgSetSerDesRxPattern
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Configure SerDes Rx pattern.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       pattern is the pattern string to configure.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesRxPattern(fm_int  sw,
                                  fm_int  serDes,
                                  fm_text pattern)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesRxPattern,
                       sw,
                       serDes,
                       pattern);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesRxPattern */



/*****************************************************************************/
/** fmDbgResetSerDesErrorCounter
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Clear SerDes error counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgResetSerDesErrorCounter(fm_int sw, 
                                       fm_int serDes)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgResetSerDesErrorCounter,
                       sw,
                       serDes);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgResetSerDesErrorCounter */




/*****************************************************************************/
/** fmDbgGetSerDesErrorCounter
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Return SERDES error counter.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[out]      pCounter pointer to a caller-defined memory location where
 *                  this function returns the value of the error counter.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgGetSerDesErrorCounter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32 *pCounter)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgGetSerDesErrorCounter,
                       sw,
                       serDes,
                       pCounter);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgGetSerDesErrorCounter */


/*****************************************************************************/
/** fmDbgInjectSerDesErrors
 * \ingroup intDiagSerdes
 *
 * \chips           FM10000
 *
 * \desc            Inject errors into SERDES TX or RX stream.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       serDesDir is SERDES direction. 0 - TX, 1 - RX.
 *
 * \param[in]       numErrors is the number of errors to inject.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgInjectSerDesErrors(fm_int sw,
                                  fm_int serDes,
                                  fm_int serDesDir, 
                                  fm_uint numErrors)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgInjectSerDesErrors,
                       sw,
                       serDes,
                       serDesDir,
                       numErrors);
    
    UNPROTECT_SWITCH(sw);

    return err;

} /* fmDbgInjectSerDesErrors */




/*****************************************************************************/
/** fmDbgSetSerDesCursor
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Set SERDES cursor or attenuation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       cursor is the SERDES cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesCursor(fm_int     sw,
                               fm_int     serDes,
                               fm_int     cursor)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesCursor,
                       sw,
                       serDes,
                       cursor);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesCursor */




/*****************************************************************************/
/** fmDbgSetSerDesPreCursor
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Set SERDES pre-cursor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       preCursor is the SERDES pre-cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesPreCursor(fm_int    sw,
                                  fm_int    serDes,
                                  fm_int    preCursor)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesPreCursor,
                       sw,
                       serDes,
                       preCursor);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesPreCursor */




/*****************************************************************************/
/** fmDbgSetSerDesPostCursor
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Set SERDES post-cursor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       postCursor is the SERDES post-cursor to set.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesPostCursor(fm_int    sw,
                                   fm_int    serDes,
                                   fm_int    postCursor)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesPostCursor,
                       sw,
                       serDes,
                       postCursor);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesPostCursor */







/*****************************************************************************/
/** fmDbgSetSerDesPolarity
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Set SERDES polarity.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       polarityStr is a string of values: tx, rx, or txrx.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesPolarity(fm_int  sw, 
                                 fm_int  serDes,
                                 fm_text polarityStr)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesPolarity,
                       sw,
                       serDes,
                       polarityStr);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgSetSerDesPolarity */





/*****************************************************************************/
/** fmDbgSetSerDesLoopback
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Set SERDES loopback mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       loopbackStr is a string of values: off, on, or rx2tx.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgSetSerDesLoopback(fm_int  sw, 
                                fm_int  serDes,
                                fm_text loopbackStr)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesLoopback,
                       sw,
                       serDes,
                       loopbackStr);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgSetSerDesLoopback */



/*****************************************************************************/
/** fmDbgSetSerDesDfeParameter
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
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
fm_status fmDbgSetSerDesDfeParameter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32  paramSelector,
                                     fm_uint32  paramValue)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgSetSerDesDfeParam,
                       sw,
                       serDes,
                       paramSelector,
                       paramValue);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgSetSerDesDfeParameter */




/*****************************************************************************/
/** fmDbgGetSerDesDfeParameter
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Get, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       paramSelector is the selector of the parameter to be get
 *
 * \param[in]       pParamValue is a pointer to a caller-allocated area where
 *                  this function will return the value of the selected
 *                  parameter. If NULL, the parameter will be just printed.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgGetSerDesDfeParameter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32  paramSelector,
                                     fm_uint32 *pParamValue)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgGetSerDesDfeParam,
                       sw,
                       serDes,
                       paramSelector,
                       pParamValue);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgGetSerDesDfeParameter */




/*****************************************************************************/
/** fmDbgDumpSerDesDfeParameter
 * \ingroup intDiagSerDes
 *
 * \chips           FM10000
 *
 * \desc            Dump, for the given serDes, the value of the DFE parameter
 *                  indicated by paramSelector
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       serDes is the SERDES number on which to operate.
 *
 * \param[in]       paramSelector is the selector of the parameter to be get
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmDbgDumpSerDesDfeParameter(fm_int     sw,
                                      fm_int     serDes,
                                      fm_uint32  paramSelector)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return err;
    }

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgGetSerDesDfeParam,
                       sw,
                       serDes,
                       paramSelector,
                       NULL);
    
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpSerDesDfeParameter */



