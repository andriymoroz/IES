/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_regs.c
 * Creation Date:   2007
 * Description:     Contains wrappers for all the register access
 *                  functions.  Most simply call the chip's specific version.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

/* NOTE: VALIDATE_AND_PROTECT_SW is similar to VALIDATE_AND_PROTECT_SWITCH
 *  but does not validate the switch state. This is required for the Testpoint
 *  command "reset switch eeprom no-exit".
 *  
 *  This command is used to reset and release the switch and have it booted
 *  from a local EEPROM (monaco). But prior to reset the switch a call to
 *  fmSetSwitchState(sw,FALSE) is performed, which set the switch state to DOWN.
 *  
 *  The no-exit option keeps Testpoint running and in order to be able to read
 *  or write switch registers or other devices like CPLD or PHY the switch state
 *  must not be verified.
 *  
 *  Code that calls this macro MUST release the lock before returning!
 *  The easiest way to do this is to invoke the UNPROTECT_SWITCH macro */

#define VALIDATE_AND_PROTECT_SW(sw)                                         \
    fm_bool swProtected = FALSE;                                            \
    if ( (sw) < 0 || (sw) >= FM_MAX_NUM_SWITCHES )                          \
    {                                                                       \
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,                                     \
                     "VALIDATE_SWITCH_INDEX: %d not in [0,%d]\n",           \
                     (sw),                                                  \
                     FM_MAX_NUM_SWITCHES);                                  \
        return FM_ERR_INVALID_SWITCH;                                       \
    }                                                                       \
    VALIDATE_SWITCH_LOCK(sw);                                               \
    /* Take read access to the switch lock */                               \
    PROTECT_SWITCH(sw);                                                     \
    if (!fmRootApi->fmSwitchStateTable[(sw)])                               \
    {                                                                       \
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Switch not allocated\n");          \
        UNPROTECT_SWITCH(sw);                                               \
        return FM_ERR_SWITCH_NOT_UP;                                        \
    }                                                                       \
    swProtected = TRUE

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
/** fmReadUINT32
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read a single 32-bit register. If the register is cached,
 *                  this function will return the cached value.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated storage where this
 *                  function should place the contents of the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUINT32(fm_int sw, fm_uint reg, fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, reg, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUINT32 */




/*****************************************************************************/
/** fmReadUncachedUINT32
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read a single 32-bit register. If the register is cached,
 *                  this function will bypass the cache, reading directly
 *                  from the hardware.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated storage where this
 *                  function should place the contents of the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUncachedUINT32(fm_int sw, fm_uint reg, fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT32)
    {
        err = switchPtr->ReadUncacheUINT32(sw, reg, ptr);
    }
    else
    {
        err = switchPtr->ReadUINT32(sw, reg, ptr);
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUncachedUINT32 */




/*****************************************************************************/
/** fmReadUINT32Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read multiple 32-bit word registers. If the registers are 
 *                  cached, this function will return the cached values.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 32-bit words to read from
 *                  the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated array where this
 *                  function should place the words read from the switch.
 *                  The length of the array must be at least as long as
 *                  wordCount.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUINT32Mult(fm_int sw, fm_uint reg, fm_int wordCount, fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32Mult(sw, reg, wordCount, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUINT32Mult */




/*****************************************************************************/
/** fmReadUncachedUINT32Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read multiple 32-bit word registers. If the registers are 
 *                  cached, this function will bypass the cache, reading 
 *                  directly from the hardware.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 32-bit words to read from
 *                  the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated array where this
 *                  function should place the words read from the switch.
 *                  The length of the array must be at least as long as
 *                  wordCount.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUncachedUINT32Mult(fm_int sw, 
                                   fm_uint reg, 
                                   fm_int wordCount, 
                                   fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT32Mult)
    {
        err = switchPtr->ReadUncacheUINT32Mult(sw, reg, wordCount, ptr);
    }
    else
    {
        err = switchPtr->ReadUINT32Mult(sw, reg, wordCount, ptr);
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUncachedUINT32Mult */




/*****************************************************************************/
/** fmReadUINT64
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read a single 64-bit register. If the register is cached,
 *                  this function will return the cached value.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register
 *                  file.
 *
 * \param[out]      ptr points to caller-allocated storage where this
 *                  function should place the contents of the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUINT64(fm_int sw, fm_uint reg, fm_uint64 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT64(sw, reg, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUINT64 */




/*****************************************************************************/
/** fmReadUncachedUINT64
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read a single 64-bit register. If the register is cached,
 *                  this function will bypass the cache, reading directly
 *                  from the hardware.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated storage where this
 *                  function should place the contents of the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUncachedUINT64(fm_int sw, fm_uint reg, fm_uint64 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT64)
    {
        err = switchPtr->ReadUncacheUINT64(sw, reg, ptr);
    }
    else
    {
        err = switchPtr->ReadUINT64(sw, reg, ptr);
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUncachedUINT64 */




/*****************************************************************************/
/** fmReadUINT64Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read multiple 64-bit word registers. If the registers are 
 *                  cached, this function will return the cached values.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 64-bit words to read from
 *                  the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated array where this
 *                  function should place the words read from the switch.
 *                  The length of the array must be at least as long as
 *                  wordCount.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUINT64Mult(fm_int sw, fm_uint reg, fm_int wordCount, fm_uint64 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err =  switchPtr->ReadUINT64Mult(sw, reg, wordCount, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUINT64Mult */




/*****************************************************************************/
/** fmReadUncachedUINT64Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Read multiple 64-bit word registers. If the registers are 
 *                  cached, this function will bypass the cache, reading 
 *                  directly from the hardware.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 64-bit words to read from
 *                  the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated array where this
 *                  function should place the words read from the switch.
 *                  The length of the array must be at least as long as
 *                  wordCount.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmReadUncachedUINT64Mult(fm_int sw, 
                                   fm_uint reg, 
                                   fm_int wordCount, 
                                   fm_uint64 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->ReadUncacheUINT64Mult)
    {
        err = switchPtr->ReadUncacheUINT64Mult(sw, reg, wordCount, ptr);
    }
    else
    {
        err = switchPtr->ReadUINT64Mult(sw, reg, wordCount, ptr);
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadUncachedUINT64Mult */




/*****************************************************************************/
/** fmReadScatterGather
 * \ingroup intSwitch
 *
 * \desc            Reads multiple discontiguous regions of register space.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of scatter-gather entries
 *                  in sgList.
 *
 * \param[in]       sgList is an array of length nEntries, which
 *                  describes each region to be read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL a driver error occurs.
 *
 *****************************************************************************/
fm_status fmReadScatterGather(fm_int                     sw,
                              fm_int                     nEntries,
                              fm_scatterGatherListEntry *sgList)
{
    fm_status  err = FM_OK;
    fm_int     i;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SW(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);

    /* If supported then use it */
    if (switchPtr->ReadScatterGather)
    {
        err = switchPtr->ReadScatterGather(sw, nEntries, sgList);
    }
    else
    {
        for (i = 0 ; err == FM_OK && i < nEntries ; i++)
        {
            fm_uint32  addr  = sgList[i].addr;
            fm_uint32  count = sgList[i].count;
            fm_uint32 *data  = sgList[i].data;
    
            while (err == FM_OK && count > 0)
            {
                fm_uint32 nWords = count;
                err    = switchPtr->ReadUINT32Mult(sw, addr, nWords, data);
                addr  += nWords;
                count -= nWords;
                data  += nWords;
            }
        }
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmReadScatterGather */




/*****************************************************************************/
/** fmWriteUINT32
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Write a single 32-bit register.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[in]       value is the 32-bit value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmWriteUINT32(fm_int sw, fm_uint reg, fm_uint32 value)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->WriteUINT32(sw, reg, value);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmWriteUINT32 */




/*****************************************************************************/
/** fmWriteUINT32Field
 * \ingroup intSwitch
 *
 * \desc            This function writes a field in a 32bit register.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       addr is the register address.
 *
 * \param[in]       startingBit is the bit to start writing at.
 *
 * \param[in]       length defines the number of bits written (and masked out).
 *
 * \param[in]       value is the value to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmWriteUINT32Field(fm_int sw,
                             fm_int addr,
                             fm_int startingBit,
                             fm_int length,
                             fm_int value)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  mask = (1 << length) - 1;
    fm_uint32  regVal;

    /* assume the caller has done all necessary checks */
    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, addr, &regVal);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    regVal &= ~(mask << startingBit);
    regVal |= ( (value & mask) << startingBit );

    err = switchPtr->WriteUINT32(sw, addr, regVal);

    if (err != FM_OK)
    {
        goto ABORT;
    }

ABORT:
    DROP_REG_LOCK(sw);

    return err;

}   /* end fmWriteUINT32Field */




/*****************************************************************************/
/** fmWriteUINT32Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Write multiple 32-bit word registers.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 32-bit words to write to
 *                  the switch's register file.
 *
 * \param[in]       ptr points to an array of values to be written to the
 *                  switch.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmWriteUINT32Mult(fm_int     sw,
                            fm_uint    reg,
                            fm_int     wordCount,
                            fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->WriteUINT32Mult(sw, reg, wordCount, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmWriteUINT32Mult */




/*****************************************************************************/
/** fmWriteUINT64
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Write a single 64-bit register.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the 32-bit word offset into the switch's register
 *                  file.
 *
 * \param[in]       value is the 64-bit value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmWriteUINT64(fm_int sw, fm_uint reg, fm_uint64 value)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->WriteUINT64(sw, reg, value);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmWriteUINT64 */




/*****************************************************************************/
/** fmWriteUINT64Mult
 * \ingroup diagAccess 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Write multiple 64-bit word registers.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 64-bit words to write to
 *                  the switch's register file.
 *
 * \param[in]       ptr points to an array of values to be written to the
 *                  switch.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmWriteUINT64Mult(fm_int sw, fm_uint reg, fm_int wordCount, fm_uint64 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->WriteUINT64Mult(sw, reg, wordCount, ptr);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmWriteUINT64Mult */




/*****************************************************************************/
/** fmWriteScatterGather
 * \ingroup intSwitch
 *
 * \desc            Writes multiple discontiguous regions of register space.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       nEntries is the number of scatter-gather entries
 *                  in sgList.
 *
 * \param[in]       sgList is an array of length nEntries, which
 *                  describes each region to be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL a driver error occurs.
 *
 *****************************************************************************/
fm_status fmWriteScatterGather(fm_int                     sw,
                               fm_int                     nEntries,
                               fm_scatterGatherListEntry *sgList)
{
    fm_status err = FM_OK;
    fm_int    i;
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* If supported then use it */
    if (switchPtr->WriteScatterGather)
    {
        err = switchPtr->WriteScatterGather(sw, nEntries, sgList);
    }
    else
    {
        for (i = 0 ; err == FM_OK && i < nEntries ; i++)
        {
            fm_uint32  addr  = sgList[i].addr;
            fm_uint32  count = sgList[i].count;
            fm_uint32 *data  = sgList[i].data;
    
            while (err == FM_OK && count > 0)
            {
                fm_uint32 nWords = count;
                err    = switchPtr->WriteUINT32Mult(sw, addr, nWords, data);
                addr  += nWords;
                count -= nWords;
                data  += nWords;
            }
        }
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmWriteScatterGather */




/*****************************************************************************/
/** fmMaskUINT32
 * \ingroup intSwitch
 *
 * \desc        Sets or clears bits in a register
 *
 * \param[in]   sw is the switch number.
 *
 * \param[in]   reg is the register to mask.
 *
 * \param[in]   mask is the mask to be applied
 *
 * \param[in]   on indicates whether the
 *
 * \return      FM_ERR_UNSUPPORTED
 *
 *****************************************************************************/
fm_status fmMaskUINT32(fm_int sw, fm_uint reg, fm_uint32 mask, fm_bool on)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->MaskUINT32(sw, reg, mask, on);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmMaskUINT32 */




/*****************************************************************************/
/** fmI2cWriteUINT32
 * \ingroup diagAccess
 *
 * \desc            Write a single 32-bit register through I2C.
 *
 * \note            The platform layer must provide a corresponding I2C 
 *                  access function.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[in]       value is the 32-bit value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmI2cWriteUINT32(fm_int sw, fm_uint reg, fm_uint32 value)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->I2cWriteUINT32 != NULL)
    {
        err = switchPtr->I2cWriteUINT32(sw, reg, value);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmI2cWriteUINT32 */




/*****************************************************************************/
/** fmI2cWriteUINT32Mult
 * \ingroup diagAccess
 *
 * \desc            Write multiple 32-bit word registers through I2C.
 *
 * \note            The platform layer must provide a corresponding I2C 
 *                  access function.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Writing registers used by the
 *                  API or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 32-bit words to write to
 *                  the switch's register file.
 *
 * \param[in]       ptr points to an array of values to be written to the
 *                  switch.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmI2cWriteUINT32Mult(fm_int     sw,
                               fm_uint    reg,
                               fm_int     wordCount,
                               fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->I2cWriteUINT32Mult)
    {
        err = switchPtr->I2cWriteUINT32Mult(sw, reg, wordCount, ptr);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmI2cWriteUINT32Mult */




/*****************************************************************************/
/** fmI2cReadUINT32
 * \ingroup diagAccess
 *
 * \desc            Read a single 32-bit register through I2C.
 *
 * \note            The platform layer must provide a corresponding I2C 
 *                  access function.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated storage where this
 *                  function should place the contents of the register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmI2cReadUINT32(fm_int sw, fm_uint reg, fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->I2cReadUINT32 != NULL)
    {
        err = switchPtr->I2cReadUINT32(sw, reg, ptr);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmI2cReadUINT32 */




/*****************************************************************************/
/** fmI2cReadUINT32Mult
 * \ingroup diagAccess
 *
 * \desc            Read multiple 32-bit word registers through I2C.
 *
 * \note            The platform layer must provide a corresponding I2C 
 *                  access function.
 *
 * \note            This function is provided for diagnostic purposes and
 *                  should be used with care. Use of this function on certain
 *                  registers or while the switch device is being booted
 *                  may adversely affect operation of the device or the API.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the starting word offset into the switch's register
 *                  file.
 *
 * \param[in]       wordCount is the number of 32-bit words to read from
 *                  the switch's register file.
 *
 * \param[out]      ptr points to caller-allocated array where this
 *                  function should place the words read from the switch.
 *                  The length of the array must be at least as long as
 *                  wordCount.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if wordCount is out of range.
 * \return          FM_ERR_BAD_IOCTL if reg is invalid.
 *
 *****************************************************************************/
fm_status fmI2cReadUINT32Mult(fm_int     sw, 
                              fm_uint    reg, 
                              fm_int     wordCount, 
                              fm_uint32 *ptr)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->I2cReadUINT32Mult != NULL)
    {
        err = switchPtr->I2cReadUINT32Mult(sw, reg, wordCount, ptr);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmI2cReadUINT32Mult */




/*****************************************************************************/
/** fmEmulateWriteRawUINT32Seq
 * \ingroup intSwitch
 *
 * \desc            Writes a sequence of UINT32 registers without taking the
 *                  platform lock. This function emulates the WriteRawUINT32Seq
 *                  function on platforms that do not implement it. Caller
 *                  of this function must already have taken the switch and 
 *                  platform locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr points to an array of register addresses to be written.
 *                  The array must be n elements in length.
 *
 * \param[in]       value points to an array of values to be written. The
 *                  array must be n elements in length.
 * 
 * \param[in]       n is the number of register addresses to write.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmEmulateWriteRawUINT32Seq(fm_int     sw,
                                     fm_uint32 *addr,
                                     fm_uint32 *value,
                                     fm_int     n)
{
    fm_switch * switchPtr = GET_SWITCH_PTR(sw);
    fm_int      i;
    fm_status   err = FM_OK;

    for (i = 0 ; i < n ; i++)
    {
        err = switchPtr->WriteUINT32(sw, addr[i], value[i]);
        if (err != FM_OK)
        {
            return err;
        }
    }

    return FM_OK;

}   /* end fmEmulateWriteRawUINT32Seq */




/*****************************************************************************/
/** fmI2cWriteRead
 * \ingroup intSwitch
 *
 * \desc            Use switch master I2C support to access I2C devices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to an array from which data is written and
 *                  into which data is read.
 *
 * \param[in]       wl is the number of bytes to write.
 *
 * \param[in]       rl is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmI2cWriteRead(fm_int     sw,
                         fm_uint    device,
                         fm_byte   *data,
                         fm_uint    wl,
                         fm_uint    rl)
{
    fm_switch *switchPtr;
    fm_status  err;

    VALIDATE_AND_PROTECT_SW(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->I2cWriteRead)
    {
        err = switchPtr->I2cWriteRead(sw, device, data, wl, rl);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmI2cWriteRead */
