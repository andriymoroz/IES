/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_regs.c
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#define MAX_REG_NAME_LENGTH         200


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/* Set to 1 to read the FM_PORT_MASK attribute instead of reading the
 * PORT_CFG_2 register from the switch. This is used for EEPROM image
 * generation. */
fm_int fmDbgUsePortMaskAttribute = 0;

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
/** fmDbgMonitorRegister
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Enable or Disable monitoring of writes to a 
 *                  switch register.
 *
 * \param[in]       sw is the switch to monitor. Set to -1 to monitor all
 *                  switches.
 *
 * \param[in]       regOffset is the register offset.
 *
 * \param[in]       monitor if 0, disable monitoring, if 1 enable monitoring.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmDbgMonitorRegister(fm_int sw, fm_uint32 regOffset, fm_bool monitor)
{
    int reg;
    fmDbgMonitoredRegInfo *regInfo;

    /* first see if this register is on the list */
    for (reg = 0 ; reg < fmRootDebug->fmDbgMonitoredRegCount ; reg++)
    {
        if ( (regOffset == fmRootDebug->fmDbgMonitoredRegs[reg].regOffset) &&
            (sw == fmRootDebug->fmDbgMonitoredRegs[reg].sw) )
        {
            break;
        }
    }

    if (monitor)
    {
        /* Adding register to list if not already present */
        if (reg >= fmRootDebug->fmDbgMonitoredRegCount
            && fmRootDebug->fmDbgMonitoredRegCount < FM_DBG_MAX_MONITORED_REGS)
        {
            regInfo = &fmRootDebug->fmDbgMonitoredRegs[fmRootDebug->fmDbgMonitoredRegCount];

            regInfo->sw        = sw;
            regInfo->regOffset = regOffset;

            ++fmRootDebug->fmDbgMonitoredRegCount;
        }
    }
    else
    {
        /* Removing register from list */
        if (reg < fmRootDebug->fmDbgMonitoredRegCount)
        {
            --fmRootDebug->fmDbgMonitoredRegCount;

            for ( ; reg < fmRootDebug->fmDbgMonitoredRegCount ; reg++)
            {
                fmRootDebug->fmDbgMonitoredRegs[reg] = fmRootDebug->
                                                           fmDbgMonitoredRegs[
                    reg + 1];
            }

            fmRootDebug->
                fmDbgMonitoredRegs[fmRootDebug->
                                       fmDbgMonitoredRegCount].sw = 0;
            fmRootDebug->
                fmDbgMonitoredRegs[fmRootDebug->
                                       fmDbgMonitoredRegCount].regOffset = 0;
        }
    }

    return FM_OK;

}   /* end fmDbgMonitorRegister */




/**********************************************************************
 * fmDbgRegisterUpdate
 *
 * Description: Called when a switch device register is being updated.
 *              Scans the monitor list to see if this register update
 *              should be reported
 *
 * Arguments:   sw              switch number
 *              regOffset       register offset
 *              regValue        new register value
 *
 * Returns:     None.
 *
 **********************************************************************/
void fmDbgRegisterUpdate(fm_int sw, fm_uint32 regOffset, fm_uint32 regValue)
{
    int reg;

    FM_NOT_USED(regValue);

    for (reg = 0 ; reg < fmRootDebug->fmDbgMonitoredRegCount ; reg++)
    {
        if ( (fmRootDebug->fmDbgMonitoredRegs[reg].regOffset == regOffset) &&
            ( (fmRootDebug->fmDbgMonitoredRegs[reg].sw == sw) ||
             (fmRootDebug->fmDbgMonitoredRegs[reg].sw == -1) ) )
        {
            FM_LOG_PRINT("Register Update: %08X of switch %d set to %08X\n",
                         regOffset, sw, regValue);
            break;
        }
    }

}   /* end fmDbgRegisterUpdate */




/*****************************************************************************/
/** fmDbgListRegisters
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display a list of switch registers that may be displayed
 *                  by calling ''fmDbgDumpRegister'', ''fmDbgDumpRegisterV2''
 *                  or ''fmDbgDumpRegisterV3''.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       showGlobals should be TRUE to include all global registers
 *                  in the list.
 *
 * \param[in]       showPorts should be TRUE to include all port registers
 *                  in the list.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgListRegisters(fm_int sw, fm_bool showGlobals, fm_bool showPorts)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    switchPtr->DbgListRegisters(sw, showGlobals, showPorts);

    UNPROTECT_SWITCH(sw);
    return;

}   /* end fmDbgListRegisters */




/*********************************************************************
 * fmDbgPrintRegValue
 *
 * Description: callback function to dump register contents to stdout
 *              for debugging purposes
 *
 * Arguments:   sw                  switch number
 *              regId               Index into register table
 *              regAddress          address of register
 *              regSize             number of words in register
 *              isStatReg           TRUE if register is a statistics register
 *              regValue1           low-order 64 bits of register
 *              regValue2           high-order 64 bits of register
 *              callbackInfo        not used
 *
 * Returns:     FALSE to prevent further dumping (if in loop)
 *              TRUE to continue further dumping if in loop
 *
 *********************************************************************/
fm_bool fmDbgPrintRegValue(fm_int    sw,
                           fm_int    regId,
                           fm_uint   regAddress,
                           fm_int    regSize,
                           fm_bool   isStatReg,
                           fm_uint64 regValue1,
                           fm_uint64 regValue2,
                           fm_voidptr callbackInfo)
{
    fm_char regName[MAX_REG_NAME_LENGTH];
    fm_bool isPort;
    fm_int  index0Ptr;
    fm_int  index1Ptr;
    fm_int  index2Ptr;
    fm_char regTitle[MAX_REG_NAME_LENGTH * 2];

    FM_NOT_USED(regId);
    FM_NOT_USED(callbackInfo);
    FM_NOT_USED(regValue1);
    FM_NOT_USED(regValue2);

    FM_LOG_DEBUG(FM_LOG_CAT_DEBUG,
                 "fmDbgPrintRegValue: sw=%d, "
                 "regAddress=0x%x, regSize=%d, isStatReg=%d, val1=0x%"
                 FM_FORMAT_64 "x, val2=0x%" FM_FORMAT_64 "x\n",
                 sw,
                 regAddress,
                 regSize,
                 isStatReg,
                 regValue1,
                 regValue2);

    fmDbgGetRegisterName(sw,
                         regId,
                         regAddress,
                         regName,
                         MAX_REG_NAME_LENGTH,
                         &isPort,
                         &index0Ptr,
                         &index1Ptr,
                         &index2Ptr,
                         TRUE,
                         FALSE);

    FM_SNPRINTF_S(regTitle, sizeof(regTitle),
                  "%-50s (%08X) :   ", regName, regAddress);

    switch (regSize)
    {
        case 1:
            FM_LOG_PRINT("%s %08X\n", regTitle, (fm_uint32) regValue1);
            break;

        case 2:

            if (isStatReg)
            {
                FM_LOG_PRINT("%s %" FM_FORMAT_64 "u\n", regTitle, regValue1);
            }
            else
            {
                FM_LOG_PRINT("%s %08X %08X\n", 
                             regTitle, 
                             (fm_uint32) (regValue1 >> 32),
                             (fm_uint32) regValue1);
            }

            break;

        case 3:
            FM_LOG_PRINT("%s %08X %08X %08X\n", 
                         regTitle, 
                         (fm_uint32) regValue2, 
                         (fm_uint32) (regValue1 >> 32),
                         (fm_uint32) regValue1);
            break;

        case 4:
            FM_LOG_PRINT("%s %08X %08X %08X %08X\n", 
                         regTitle, 
                         (fm_uint32) (regValue2 >> 32),
                         (fm_uint32) regValue2,
                         (fm_uint32) (regValue1 >> 32),
                         (fm_uint32) regValue1);
            break;

        default:
            FM_LOG_PRINT("Unexpected register size %d for register %s (%08X)\n",
                         regSize, regName, regAddress);
            break;

    }   /* end switch (regSize) */

    return TRUE;

}   /* end fmDbgPrintRegValue */




/*****************************************************************************/
/** fmDbgDumpRegister
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \note            Some registers (on devices other than FM2000) are doubly 
 *                  or triply indexed. If dumped with this function, the 
 *                  second and third indexes will always be zero. To specify a 
 *                  value for the second or third index, use 
 *                  ''fmDbgDumpRegisterV2'' or ''fmDbgDumpRegisterV3''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number (for port registers) or
 *                  the index (for non-port indexed registers).
 *
 * \param[in]       regname is the name of the register or group of registers 
 *                  to be displayed.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL 
 *                  and channel number, but an EPL-channel tuple represents 
 *                  a port. These registers have an alias by the same name, 
 *                  but with a "_P" suffix. When referred to by the alias 
 *                  name, port may be specified using the logical port number. 
 *                  This function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpRegister(fm_int sw, fm_int port, fm_text regname)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpRegister, sw, port, regname);

    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpRegister */




/*****************************************************************************/
/** fmDbgDumpRegisterV2
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \note            This function is provided for devices other than FM2000
 *                  that have doubly indexed registers, but it can be 
 *                  successfully called for the FM2000 as well. Singly 
 *                  indexed and non-indexed registers can also be accessed 
 *                  with ''fmDbgDumpRegister''. Triply indexed registers
 *                  require ''fmDbgDumpRegisterV3''.
 *                                                                      \lb\lb
 *                  This function provides two index arguments. Some registers
 *                  are not indexed. For these registers, all index arguments
 *                  are ignored and may be specified as zero. Some registers
 *                  take a single index. In these cases, the first index
 *                  argument, indexA, is taken as the register index. The
 *                  second index argument, indexB, is ignored:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA]
 *                                                                      \lb\lb
 *                  For doubly indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB]
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the switch device datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number.
 *
 * \param[in]       regname is the name of the register or group of registers 
 *                  to be displayed.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL and 
 *                  channel number, but an EPL-channel tuple represents a port. 
 *                  These registers have an alias by the same name, but with 
 *                  a "_P" suffix. When referred to by the alias name, port 
 *                  may be specified using the logical port number. This 
 *                  function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INDEX if indexA or indexB is out of
 *                  range for the specified register.
 * \return          FM_ERR_UNKNOWN_REGISTER if regname is not recognized.
 *
 *****************************************************************************/
fm_status fmDbgDumpRegisterV2(fm_int  sw,
                              fm_int  indexA,
                              fm_int  indexB,
                              fm_text regname)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgDumpRegisterV2,
                       sw,
                       indexA,
                       indexB,
                       regname);

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgDumpRegisterV2 */




/*****************************************************************************/
/** fmDbgDumpRegisterV3
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the contents of a switch register, or a group
 *                  of related registers.
 *
 * \note            This function is provided for FM6000 devices (which have 
 *                  triply indexed registers), but it can be successfully 
 *                  called for the FM2000, FM3000, and FM4000 devices as well. 
 *                  Singly indexed and non-indexed registers can also be 
 *                  accessed with ''fmDbgDumpRegister''. Doubly indexed 
 *                  registers can also be accessed with ''fmDbgDumpRegisterV2''.
 *                                                                      \lb\lb
 *                  This function provides three index arguments. For 
 *                  non-indexed registers, all index arguments are ignored and 
 *                  may be specified as zero. For singly indexed registers, 
 *                  the first index argument, indexA, is taken as the register 
 *                  index. The second and third index arguments, indexB and 
 *                  indexC, are ignored:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA]
 *                                                                      \lb\lb
 *                  For doubly indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB]
 *                                                                      \lb\lb
 *                  For triply indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB][indexC]
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly or
 *                  triply indexed, this argument specifies the left-most 
 *                  index as the register is depicted in the switch device 
 *                  datasheet. For some indexed registers, this index may 
 *                  represent a port number. In those cases, this argument 
 *                  should specify the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number. Ignored for FM2000 devices.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers. Ignored for FM2000, FM3000 and 
 *                  FM4000 devices.
 *
 * \param[in]       regname is the name of the register or group of registers 
 *                  to be displayed.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL 
 *                  and channel number, but an EPL-channel tuple represents 
 *                  a port. These registers have an alias by the same name, 
 *                  but with a "_P" suffix. When referred to by the alias 
 *                  name, port may be specified using the logical port number. 
 *                  This function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is out of
 *                  range for the specified register.
 * \return          FM_ERR_UNKNOWN_REGISTER if regname is not recognized.
 *
 *****************************************************************************/
fm_status fmDbgDumpRegisterV3(fm_int  sw,
                              fm_int  indexA,
                              fm_int  indexB,
                              fm_int  indexC,
                              fm_text regname)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgDumpRegisterV3,
                       sw,
                       indexA,
                       indexB,
                       indexC,
                       regname);

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgDumpRegisterV3 */




/*****************************************************************************/
/** fmDbgWriteRegister
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Write the contents of a switch register.
 *
 * \note            Some registers (on devices other than FM2000) are doubly 
 *                  or triply indexed. If written with this function, the 
 *                  second and third indexes will always be zero. To specify a 
 *                  value for the second or third index, use 
 *                  ''fmDbgWriteRegisterV2'' or ''fmDbgWriteRegisterV3''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number (for port registers) or
 *                  the index (for non-port indexed registers).
 *
 * \param[in]       regname is the name of the register to be written.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL and 
 *                  channel number, but an EPL-channel tuple represents a port. 
 *                  These registers have an alias by the same name, but with 
 *                  a "_P" suffix. When referred to by the alias name, port 
 *                  may be specified using the logical port number. This 
 *                  function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \param[in]       value is the value to write to the register.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgWriteRegister(fm_int sw, fm_int port, fm_text regname, fm_int value)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgWriteRegister,
                            sw,
                            port,
                            regname, 
                            value);

    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgWriteRegister */




/*****************************************************************************/
/** fmDbgWriteRegisterV2
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Write the contents of a switch register. Provides
 *                  support for multi-word and doubly indexed registers
 *                  (for devices other than FM2000).
 *
 * \note            This function is provided for devices other than FM2000
 *                  that have doubly indexed registers, but it can be 
 *                  successfully called for the FM2000 as well. Singly 
 *                  indexed and non-indexed registers can also be accessed 
 *                  with ''fmDbgWriteRegister''. Triply indexed registers
 *                  require ''fmDbgWriteRegisterV3''.
 *                                                                      \lb\lb
 *                  This function provides two index arguments. Some registers
 *                  are not indexed. For these registers, all index arguments
 *                  are ignored and may be specified as zero. Some registers
 *                  take a single index. In these cases, the first index
 *                  argument, indexA, is taken as the register index. The
 *                  second index argument, indexB, is ignored:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA]
 *                                                                      \lb\lb
 *                  For doubly indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB]
 *
 * \note            Multi-word registers generally require that all words of
 *                  the register be written before the write will become
 *                  effective and the words must be written in a particular 
 *                  order. This function does nothing to guarantee atomicity 
 *                  between writing one word and writing the next word and does
 *                  not enforce the required order that the words are written
 *                  in.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wordOffset is used for all multi-word registers and 
 *                  ignored for single-word (and single-word indexed) 
 *                  registers.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly
 *                  indexed, this argument specifies the left index as the
 *                  register is described in the switch device datasheet. For
 *                  some indexed registers, this index may represent a port
 *                  number. In those cases, this argument should specify the
 *                  logical port number.
 *
 * \param[in]       indexB is used to index into doubly indexed registers
 *                  and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number. Ignored for FM2000 devices.
 *
 * \param[in]       regName is the name of the register to write.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL and 
 *                  channel number, but an EPL-channel tuple represents a port. 
 *                  These registers have an alias by the same name, but with 
 *                  a "_P" suffix. When referred to by the alias name, port 
 *                  may be specified using the logical port number. This 
 *                  function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \param[in]       value is the register value to be written.
 *
 * \return          FM_OK if successful. For FM2000 devices, this is the only
 *                  value ever returned regardless of what arguments are
 *                  provided.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA or indexB is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fmDbgWriteRegisterV2(fm_int    sw, 
                               fm_int    wordOffset, 
                               fm_int    indexA, 
                               fm_int    indexB, 
                               fm_text   regName, 
                               fm_uint32 value)
{
    fm_status err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgWriteRegisterV2,
                       sw, 
                       wordOffset, 
                       indexA, 
                       indexB, 
                       regName, 
                       value);
    
    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteRegisterV2 */




/*****************************************************************************/
/** fmDbgWriteRegisterV3
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Write the contents of a switch register. Provides
 *                  support for multi-word and doubly and triply indexed 
 *                  registers.
 *
 * \note            This function is provided for FM6000 devices (which have 
 *                  triply indexed registers), but it can be successfully 
 *                  called for the FM2000 and FM3000/FM4000 devices as well. 
 *                  Singly indexed and non-indexed registers can also be 
 *                  accessed with ''fmDbgWriteRegister''. Doubly indexed 
 *                  registers can also be accessed with ''fmDbgWriteRegisterV2''.
 *                                                                      \lb\lb
 *                  This function provides three index arguments. For 
 *                  non-indexed registers, all index arguments are ignored and 
 *                  may be specified as zero. For singly indexed registers, 
 *                  the first index argument, indexA, is taken as the register 
 *                  index. The second and third index arguments, indexB and 
 *                  indexC, are ignored:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA]
 *                                                                      \lb\lb
 *                  For doubly indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB]
 *                                                                      \lb\lb
 *                  For triply indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB][indexC]
 *
 * \note            Multi-word registers generally require that all words of
 *                  the register be written before the write will become
 *                  effective and the words must be written in a particular 
 *                  order. This function does nothing to guarantee atomicity 
 *                  between writing one word and writing the next word and does
 *                  not enforce the required order that the words are written
 *                  in.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       wordOffset is used for all multi-word registers and 
 *                  ignored for single-word (and single-word indexed) 
 *                  registers.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly or
 *                  triply indexed, this argument specifies the left-most 
 *                  index as the register is depicted in the switch device 
 *                  datasheet. For some indexed registers, this index may 
 *                  represent a port number. In those cases, this argument 
 *                  should specify the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number. Ignored for FM2000 devices.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers. Ignored for FM2000, FM3000 and 
 *                  FM4000 devices.
 *
 * \param[in]       regName is the name of the register to write.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL and 
 *                  channel number, but an EPL-channel tuple represents a port. 
 *                  These registers have an alias by the same name, but with 
 *                  a "_P" suffix. When referred to by the alias name, port 
 *                  may be specified using the logical port number. This 
 *                  function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \param[in]       value is the register value to be written.
 *
 * \return          FM_OK if successful. For FM2000 devices, this is the only
 *                  value ever returned regardless of what arguments are
 *                  provided.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if wordOffset is invalid for the
 *                  specified register.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fmDbgWriteRegisterV3(fm_int    sw, 
                               fm_int    wordOffset, 
                               fm_int    indexA, 
                               fm_int    indexB, 
                               fm_int    indexC, 
                               fm_text   regName, 
                               fm_uint32 value)
{
    fm_status err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgWriteRegisterV3,
                       sw, 
                       wordOffset, 
                       indexA, 
                       indexB, 
                       indexC, 
                       regName, 
                       value);
    
    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteRegisterV3 */




/*****************************************************************************/
/** fmDbgWriteRegisterField
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Write the field contents of a switch register. Provides
 *                  support for multi-word and doubly and triply indexed 
 *                  registers.
 *
 * \note            This function provides three index arguments. For 
 *                  non-indexed registers, all index arguments are ignored and 
 *                  may be specified as zero. For singly indexed registers, 
 *                  the first index argument, indexA, is taken as the register 
 *                  index. The second and third index arguments, indexB and 
 *                  indexC, are ignored:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA]
 *                                                                      \lb\lb
 *                  For doubly indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB]
 *                                                                      \lb\lb
 *                  For triply indexed registers, the arguments are
 *                  ordered as they appear in the datasheet:
 *                                                                      \lb\lb
 *                          REGISTER_NAME[indexA][indexB][indexC]
 *
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       indexA is used for all indexed registers and ignored
 *                  for non-indexed registers. If the register is doubly or
 *                  triply indexed, this argument specifies the left-most 
 *                  index as the register is depicted in the switch device 
 *                  datasheet. For some indexed registers, this index may 
 *                  represent a port number. In those cases, this argument 
 *                  should specify the logical port number.
 *
 * \param[in]       indexB is used to index into doubly and triply indexed 
 *                  registers and is ignored for singly indexed and non-indexed
 *                  registers. For some registers, this index may represent a
 *                  port number. In those cases, this argument should specify
 *                  the logical port number. Ignored for FM2000 devices.
 *
 * \param[in]       indexC is used to index into triply indexed registers and 
 *                  is ignored for singly indexed, doubly indexed and 
 *                  non-indexed registers. Ignored for FM2000, FM3000 and 
 *                  FM4000 devices.
 *
 * \param[in]       regName is the name of the register to write.
 *                                                                      \lb\lb
 *                  On FM6000 devices, some registers are indexed by EPL and 
 *                  channel number, but an EPL-channel tuple represents a port. 
 *                  These registers have an alias by the same name, but with 
 *                  a "_P" suffix. When referred to by the alias name, port 
 *                  may be specified using the logical port number. This 
 *                  function will automatically convert the logical port 
 *                  number into the corresponding EPL and channel indexes.
 *
 * \param[in]       fieldName is the field name of the register to write.
 *
 * \param[in]       value is the field value to be written.
 *
 * \return          FM_OK if successful. For FM2000 devices, this is the only
 *                  value ever returned regardless of what arguments are
 *                  provided.
 * \return          FM_ERR_UNKNOWN_REGISTER if regName is not recognized.
 * \return          FM_ERR_INVALID_ARGUMENT if fieldName is not recognized.
 * \return          FM_ERR_INVALID_INDEX if indexA, indexB or indexC is
 *                  invalid for the specified register.
 * \return          FM_ERR_INVALID_REGISTER if a write operation cannot
 *                  be performed on the specified register (possibly because
 *                  it is a pseudo-register).
 *
 *****************************************************************************/
fm_status fmDbgWriteRegisterField(fm_int    sw, 
                                  fm_int    indexA, 
                                  fm_int    indexB, 
                                  fm_int    indexC, 
                                  fm_text   regName, 
                                  fm_text   fieldName, 
                                  fm_uint64 value)
{
    fm_status err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgWriteRegisterField,
                       sw, 
                       indexA, 
                       indexB, 
                       indexC, 
                       regName, 
                       fieldName, 
                       value);
    
    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteRegisterField */




/*****************************************************************************/
/** fmDbgGetRegisterName
 * \ingroup intDiagReg
 *
 * \desc            Given a register address, return the register name and
 *                  information about the register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regId is the index into the register table.
 *
 * \param[in]       regAddress is the register address to look up.
 *
 * \param[out]      regName is a pointer to caller-allocated memory where
 *                  this function may write the register name.  (100 bytes)
 *
 * \param[in]       regNameLength is the length of regName.
 *
 * \param[in,out]   isPort points to caller-allocated storage where this
 *                  function will write TRUE if the register is a port
 *                  register. If NULL on entry, this argument will be
 *                  ignored by the function.
 *
 * \param[in,out]   index0Ptr points to caller-allocated storage where this
 *                  function will write the offset within the register array
 *                  for indexed registers.  If the register is a port register,
 *                  this will be set to the logical port number, not the
 *                  physical port. If NULL on entry, this argument will be
 *                  ignored by the function.
 *
 * \param[in,out]   index1Ptr points to caller-allocated storage where this
 *                  function will write the secondary offset within the
 *                  register array for doubly or triply indexed registers.  
 *                  If the register is a port register, and the secondary index
 *                  represents the port, this will be set to the logical port
 *                  number, not the physical port. For non-doubly or triply 
 *                  indexed registers, this will be set to zero. If NULL on 
 *                  entry, this argument will be ignored by the function.
 *
 * \param[in,out]   index2Ptr points to caller-allocated storage where this
 *                  function will write the tertiary offset within the
 *                  register array for triply indexed registers.  For 
 *                  non-triply indexed registers, this will be set to zero. 
 *                  If NULL on entry, this argument will be ignored by the 
 *                  function.
 *
 * \param[in]       logicalPorts indicates whether port registers will have
 *                  the port number converted from physical to logical
 *                  before it is added to the register name.
 *
 * \param[in]       partialLongRegs indicates if the name should be built
 *                  with the expectation that registers longer than 1 word
 *                  will be dumped as separate words. This causes a subscript
 *                  for the word offset to be added to the register name.
 *                  FALSE causes the register name to be generated without
 *                  that subscript.
 *
 * \return          None.
 *
 *********************************************************************/
void fmDbgGetRegisterName(fm_int   sw,
                          fm_int   regId,
                          fm_uint  regAddress,
                          fm_text  regName,
                          fm_uint  regNameLength,
                          fm_bool *isPort,
                          fm_int * index0Ptr,
                          fm_int * index1Ptr,
                          fm_int * index2Ptr,
                          fm_bool  logicalPorts,
                          fm_bool  partialLongRegs)
{
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgGetRegisterName,
                            sw,
                            regId,
                            regAddress,
                            regName,
                            regNameLength,
                            isPort,
                            index0Ptr,
                            index1Ptr,
                            index2Ptr,
                            logicalPorts,
                            partialLongRegs);

}   /* end fmDbgGetRegisterName */




/*****************************************************************************/
/** fmDbgReadRegister
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Reads a register by name instead of by address.  The given
 *                  indices do not include the word index for > 32-bit registers.
 *                  These are handled internally and it is assumed that the
 *                  appropriately sized data is passed into the argument.  All
 *                  32-bit words are read back into 64-bit words for convenience.
 *
 * \param[in]       sw identifies the switch for which to read the register.
 *
 * \param[in]       firstIndex is the first index for the register.
 *
 * \param[in]       secondIndex is the second index for the register.
 *
 * \param[in]       registerName is the register name string.
 *
 * \param[out]      values points to caller allocated storage where the read values
 *                  will be written.   It is assumed that there are enough entries
 *                  in this storage space for the given register.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgReadRegister(fm_int  sw,
                       fm_int  firstIndex,
                       fm_int  secondIndex,
                       fm_text registerName,
                       void *  values)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgReadRegister,
                            sw,
                            firstIndex,
                            secondIndex,
                            registerName,
                            values);

    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgReadRegister */




/*****************************************************************************/
/** fmDbgVerifyRegisterCache
 * \ingroup diagReg 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000
 *
 * \desc            Verifies cached switch register values against the
 *                  hardware for any registers that are cached.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if all cached entries are in sync.
 * \return          FM_FAIL if some entries are not in sync.
 *
 *****************************************************************************/
fm_status fmDbgVerifyRegisterCache(fm_int  sw)
{
    fm_status err;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->RegCacheVerify, sw);

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgVerifyRegisterCache */




/*****************************************************************************
 * fmDbgWriteRegisterBits
 *
 * Description: Update specified bits in a register.  The function retrieves
 *              the current value of the register, ands the complement of the
 *              mask into the register value, then ors the supplied value
 *              into the register contents before writing the final result
 *              back to the register.
 *
 * Arguments:   sw is the switch number
 *              reg is the register address
 *              mask is the mask of the bits to be cleared
 *              value is the new value to be ored into the register
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgWriteRegisterBits(fm_int    sw,
                            fm_uint   reg,
                            fm_uint32 mask,
                            fm_uint32 value)
{
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgWriteRegisterBits,
                            sw,
                            reg,
                            mask,
                            value);

}   /* end fmDbgWriteRegisterBits */




/*****************************************************************************/
/** fmTestReadRegister
 * \ingroup intSwitch
 *
 * \desc            Read a 32-bit register (testing)
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       offset is the offset within the switch to read
 *
 * \param[out]      pVal contains the value read
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmTestReadRegister(fm_int sw, fm_uint offset, fm_uint32 *pVal)
{
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT32)
    {
        /* When caching is enabled, use this to read uncached value */
        return (switchPtr->ReadUncacheUINT32(sw, offset, pVal));
    }
    
    return (switchPtr->ReadUINT32(sw, offset, pVal));


}   /* end fmTestReadRegister */




/*****************************************************************************/
/** fmTestRead64BitRegister
 * \ingroup intSwitch
 *
 * \desc            Read a 64-bit register (testing)
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       offset is the offset within the switch to read
 *
 * \param[out]      pVal contains the value read
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmTestRead64BitRegister(fm_int sw, fm_uint offset, fm_uint64 *pVal)
{
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT64)
    {
        /* When caching is enabled, use this to read uncached value */
        return (switchPtr->ReadUncacheUINT64(sw, offset, pVal));
    }

    return (switchPtr->ReadUINT64(sw, offset, pVal));

}   /* end fmTestRead64BitRegister */




/*****************************************************************************/
/** fmTrace
 * \ingroup intSwitch
 *
 * \desc            Write an event trace to the trace file
 *
 * \param[in]       event is the event code
 *
 * \param[in]       arg1 is the first event argument
 *
 * \param[in]       arg2 is the second event argument
 *
 * \param[in]       arg3 is the third event argument
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
void fmTrace(fm_uint32 event, fm_uint32 arg1, fm_uint32 arg2, fm_uint32 arg3)
{
#ifdef FM_DO_TRACE
    fm_uint32 eventBuf[4];

    eventBuf[0] = event;
    eventBuf[1] = arg1;
    eventBuf[2] = arg2;
    eventBuf[3] = arg3;
    write( fmTraceFH, &eventBuf, sizeof(eventBuf) );
#else
    FM_NOT_USED(event);
    FM_NOT_USED(arg1);
    FM_NOT_USED(arg2);
    FM_NOT_USED(arg3);
#endif

}   /* end fmTrace */




/*****************************************************************************/
/** fmDbgDumpFFU
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the contents of the FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       validSlicesOnly set to TRUE to skip disabled slices.
 *
 * \param[in]       validRulesOnly set to TRUE to skip disabled rules.
 * 
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpFFU(fm_int sw, fm_bool validSlicesOnly, fm_bool validRulesOnly)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpFFU,
                            sw,
                            validSlicesOnly,
                            validRulesOnly );
    
    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpFFU */




/*****************************************************************************/
/** fmDbgDumpMapper
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display the contents of the Mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpMapper(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMapper, sw );
    
    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpMapper */




/*****************************************************************************/
/** fmDbgSetMiscAttribute
 * \ingroup intSwitch
 *
 * \desc            Set a given attribute to a given value.
 *
 * \param[in]       sw identifies the switch for which to set attribute.
 *
 * \param[in]       attr attribute to set
 *
 * \param[in]       value set the attribute to this value
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status  fmDbgSetMiscAttribute(fm_int sw, fm_uint attr, fm_int value)
{
    FM_NOT_USED(sw);

    switch (attr)
    {
        case FM_DBG_ATTR_PORT_CFG2:
            fmDbgUsePortMaskAttribute = value;
            break;

        default:
            return FM_FAIL;
    }

    return FM_OK;

}   /* end fmDbgSetMiscAttribute */




/*****************************************************************************/
/** fmDbgInitSwitchRegisterTable
 * \ingroup intSwitch
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            Initialize the switch register table
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgInitSwitchRegisterTable(fm_int sw)
{
    fm_switch *switchPtr;
    
    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgInitSwitchRegisterTable, sw);
    
    return;
    
}   /* end fmDbgInitSwitchRegisterTable */




/*****************************************************************************/
/** fmDbgDumpL2LSweepers
 * \ingroup diagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Display the Layer 2 Lookup Sweeper configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       regs should be set to TRUE to include raw registers.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpL2LSweepers(fm_int sw, fm_bool regs)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Unable to lock switch %d.\n", sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpL2LSweepers, sw, regs);
    UNPROTECT_SWITCH(sw);
    
}   /* end fmDbgDumpL2LSweepers */




/*****************************************************************************/
/** fmDbgDumpSBusRegister
 * \ingroup intDiagPorts 
 *
 * \chips           FM6000
 *
 * \desc            Display the contents of SBus registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       sbusDevID is the device ID on the SBus
 *
 * \param[in]       devRegID is the register ID within the device
 *
 * \param[in]       writeReg needs to be set to TRUE if the register is a
 *                  WRITE only register, FALSE if it's a READ only register
 * 
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpSBusRegister( fm_int sw, 
                            fm_int sbusDevID, 
                            fm_int devRegID, 
                            fm_bool writeReg )
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID( switchPtr->DbgDumpSBusRegister,
                             sw,
                             sbusDevID,
                             devRegID,
                             writeReg );
    
    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpSBusRegister */




/*****************************************************************************/
/** fmDbgReadSBusRegister 
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            Reads the contents of SBus registers.
 *
 * \param[in]       sw is the switch on which to operate.
 * \param[in]       sbusDevID is the device ID on the SBus
 * \param[in]       devRegID is the register ID within the device
 * \param[in]       writeReg needs to be set to TRUE if the register is a
 *                  WRITE SerDes register, FALSE if it's a READ SerDes register
 * \param[out]      value is a pointer to a caller-allocated area where this
 *                  function will return the content of the register 
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not running
 * \return          FM_ERR_INVALID_INDEX is sbusDevID/devRegID are out of range
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL
 * \return          error codes from ''fm6000ReadSBus'' otherwise
 *
 *****************************************************************************/
fm_status fmDbgReadSBusRegister( fm_int     sw, 
                                 fm_int     sbusDevID, 
                                 fm_int     devRegID, 
                                 fm_bool    writeReg,
                                 fm_uint32 *value )
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

    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgReadSBusRegister,
                        sw,
                        sbusDevID,
                        devRegID,
                        writeReg,
                        value );

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgReadSBusRegister */




/*****************************************************************************/
/** fmDbgWriteSBusRegister 
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            Writes to SBus registers.
 *
 * \param[in]       sw is the switch on which to operate.
 * \param[in]       sbusDevID is the device ID on the SBus
 * \param[in]       devRegID is the register ID within the device
 * \param[in]       value is the value to be written to the register
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not running
 * \return          FM_ERR_INVALID_INDEX is sbusDevID/devRegID are out of range
 * \return          error codes from ''fm6000WriteSBus'' otherwise
 *
 *****************************************************************************/
fm_status fmDbgWriteSBusRegister( fm_int     sw, 
                                  fm_int     sbusDevID, 
                                  fm_int     devRegID, 
                                  fm_uint32  value )
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
                        switchPtr->DbgWriteSBusRegister,
                        sw,
                        sbusDevID,
                        devRegID,
                        value );

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteSBusRegister */




/*****************************************************************************/
/** fmDbgDumpEthSerDesRegister
 * \ingroup intDiagPorts 
 *
 * \chips           FM6000
 *
 * \desc            Display the contents of an Ethernet SerDes registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the Ethernet logical port ID.
 *
 * \param[in]       devRegID is the register ID within the device.
 *
 * \param[in]       writeReg needs to be set to TRUE if the register is a
 *                  WRITE only register, FALSE if it's a READ only register.
 * 
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpEthSerDesRegister(fm_int  sw, 
                                fm_int  port,
                                fm_int  devRegID, 
                                fm_bool writeReg)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID( switchPtr->DbgDumpEthSerDesRegister,
                             sw,
                             port,
                             devRegID,
                             writeReg );
    
    UNPROTECT_SWITCH(sw);
    return;
    
}   /* end fmDbgDumpEthSerDesRegister */




/*****************************************************************************/
/** fmDbgReadEthSerDesRegister 
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            Display the contents of EthSerDes registers.
 *
 * \param[in]       sw is the switch on which to operate.
 * \param[in]       port is the Ethernet logical port ID
 * \param[in]       devRegID is the register ID within the device
 * \param[in]       writeReg needs to be set to TRUE if the register is a
 *                  WRITE SerDes register, FALSE if it's a READ SerDes register
 * \param[out]      value is a pointer to a caller-allocated area where this
 *                  function will return the content of the register 
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not running
 * \return          FM_ERR_INVALID_PORT is the port ID is invalid
 * \return          FM_ERR_INVALID_INDEX is devRegID are out of range
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL
 * \return          error codes from ''fm6000ReadSBus'' otherwise
 *
 *****************************************************************************/
fm_status fmDbgReadEthSerDesRegister( fm_int     sw, 
                                      fm_int     port,
                                      fm_int     devRegID, 
                                      fm_bool    writeReg,
                                      fm_uint32 *value )
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

    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgReadEthSerDesRegister,
                        sw,
                        port,
                        devRegID,
                        writeReg,
                        value );

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgReadEthSerDesRegister */




/*****************************************************************************/
/** fmDbgWriteEthSerDesRegister 
 * \ingroup intDiag
 *
 * \chips           FM6000
 *
 * \desc            Display the contents of EthSerDes registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the device ID on the EthSerDes
 *
 * \param[in]       devRegID is the register ID within the device
 *
 * \param[in]       value is the value to be written to the register
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not running
 * \return          FM_ERR_INVALID_PORT is the port ID is invalid
 * \return          FM_ERR_INVALID_INDEX is devRegID are out of range
 * \return          error codes from ''fm6000WriteEthSerDes'' otherwise
 *
 *****************************************************************************/
fm_status fmDbgWriteEthSerDesRegister( fm_int     sw, 
                                       fm_int     port, 
                                       fm_int     devRegID, 
                                       fm_uint32  value )
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
                        switchPtr->DbgWriteEthSerDesRegister,
                        sw,
                        port,
                        devRegID,
                        value );

    UNPROTECT_SWITCH(sw);
    return err;

}   /* end fmDbgWriteEthSerDesRegister */




/*****************************************************************************/
/** fmDbgInterruptSpico
 * \ingroup intDiagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Sends the specified SPICO interrupt command to the
 *                  SPICO controller, waits for the command to complete,
 *                  and returns the command's data result.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       cmd is the 10-bit SPICO interrupt command.
 * 
 * \param[in]       arg is the 10-bit SPICO command argument.
 * 
 * \param[in]       timeout specifies the maximum length of time to wait
 *                  for the interrupt command to complete. (nanoseconds)
 * 
 * \param[out]      result points to a caller-supplied location to receive
 *                  the 10-bit data result.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not running
 * \return          Other ''Status Codes'' as appropriate
 * 
 *****************************************************************************/
fm_status fmDbgInterruptSpico( fm_int      sw,
                               fm_int      cmd,
                               fm_int      arg,
                               fm_int      timeout,
                               fm_uint32 * result )
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

    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgInterruptSpico,
                        sw,
                        cmd,
                        arg,
                        timeout,
                        result)

    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgInterruptSpico */



