/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_reg_access_i2c.c
 * Creation Date:   March 8, 2012
 * Description:     Functions to access switch registers via I2c.
 *
 * Copyright (c) 2012 - 2015, Intel Corporation
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

/* Enable this if logging for CSR access is needed */
#if 0
#define CSR_LOG_ENTRY FM_LOG_ENTRY_VERBOSE
#define CSR_LOG_EXIT  FM_LOG_EXIT_VERBOSE
#else
#define CSR_LOG_ENTRY(...)
#define CSR_LOG_EXIT(cat, status) return (status)
#endif

#define FM6000_I2C_ADDR 0x40

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
/** fmPlatformI2cReadCSR
 * \ingroup intPlatform
 *
 * \desc            Read a CSR register via I2C slave.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read
 *
 * \param[out]      value points to storage where this function will place
 *                  the read register value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    fm_status err = FM_OK;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    *value = addr;
    err    = fmPlatformI2cWriteRead(sw,    /*  sw              */
                                    0,     /*  bus             */
                                    FM6000_I2C_ADDR,  /* I2C address */
                                    value, /*  pass reg address, will get reg value */
                                    3,     /*  write length    */
                                    4);    /*  read length     */

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadCSR */




/*****************************************************************************/
/** fmPlatformI2cWriteCSR
 * \ingroup intPlatform
 *
 * \desc            Write a CSR register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read.
 *
 * \param[in]       value is the data value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{
    fm_status          err = FM_OK;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = 0x%08x\n",
                  sw,
                  addr,
                  value);

    if ( GET_PLAT_STATE(sw)->bypassEnable && BYPASS_ADDR_CHECK(addr) )
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    data = ( ( (fm_uint64) addr ) << 32) | value;

    err = fmPlatformI2cWriteLong(sw,    /*  sw              */
                                 0,     /*  bus             */
                                 FM6000_I2C_ADDR,  /* I2C address */
                                 data,  /*  reg_addr and reg_data */
                                 7);    /*  write length    */

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cWriteCSR */




/*****************************************************************************/
/** fmPlatformI2cMaskCSR
 * \ingroup intPlatform
 *
 * \desc            Mask on or off the bits in a single 32-bit register.
 *
 * \note            This function is not called by the API directly, but by
 *                  platform layer code that is commonly available to all
 *                  platforms.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       reg is the word offset into the switch's register file.
 *
 * \param[in]       mask is the bit mask to turn on or off.
 *
 * \param[in]       on should be TRUE to set the masked bits in the register
 *                  or FALSE to clear the masked bits in the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cMaskCSR(fm_int    sw,
                                   fm_uint   reg,
                                   fm_uint32 mask,
                                   fm_bool   on)
{
    fm_status          err;
    fm_uint32          value;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, reg = %u, mask = 0x%08x, on = %s\n",
                  sw,
                  reg,
                  mask,
                  FM_BOOLSTRING(on) );

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C accesses */
    value = reg;
    err   = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &value, 3, 4);

    if (err == FM_OK)
    {
        if (on)
        {
            value |= mask;
        }
        else
        {
            value &= ~mask;
        }

        data = ( ( (fm_uint64) (reg) ) << 32) | value;
        err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
}   /* end fmPlatformI2cMaskCSR */




/*****************************************************************************/
/** fmPlatformI2cReadCSRMult
 * \ingroup intPlatform
 *
 * \desc            Read multiple CSR registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       n contains the number of consecutive register addresses
 *                  to read.
 *
 * \param[in]       addr contains the starting CSR register address to read.
 *
 * \param[out]      value points to an array to be filled in with
 *                  the register data read. The array must be n elements in
 *                  length.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadCSRMult(fm_int     sw,
                                       fm_uint32  addr,
                                       fm_int     n,
                                       fm_uint32 *value)
{
    fm_status err = FM_OK;
    fm_int    i;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    for (i = 0 ; i < n ; i++)
    {
        value[i] = addr + i;
        err      = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &value[i], 3, 4);

        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadCSRMult */




/*****************************************************************************/
/** fmPlatformI2cWriteCSRMult
 * \ingroup intPlatform
 *
 * \desc            Write multiple CSR registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the starting CSR register address to write.
 *
 * \param[in]       n contains the number of consecutive register addresses
 *                  to write.
 *
 * \param[in]       value points to an array of values to be written. The
 *                  array must be n elements in length.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWriteCSRMult(fm_int     sw,
                                    fm_uint32  addr,
                                    fm_int     n,
                                    fm_uint32 *value)
{
    fm_status          err = FM_OK;
    fm_int             i;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    for (i = 0 ; i < n ; i++)
    {
        data = ( ( ((fm_uint64)addr + i) ) << 32) | value[i];
        err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);

        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cWriteCSRMult */




/*****************************************************************************/
/** fmPlatformI2cReadCSR64
 * \ingroup intPlatform
 *
 * \desc            Read a 64-bit CSR register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read.
 *
 * \param[out]      value points to storage where the 64-bit read data value
 *                  will be stored by this function.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value)
{
    fm_status err = FM_OK;
    fm_uint32 lo, hi;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    lo  = addr + 0;
    err = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &lo, 3, 4);

    if (err == FM_OK)
    {
        hi  = addr + 1;
        err = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &hi, 3, 4);

        *value = ( (fm_uint64) hi << 32 ) | lo;
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadCSR64 */




/*****************************************************************************/
/** fmPlatformI2cWriteCSR64
 * \ingroup intPlatform
 *
 * \desc            Writes a 64-bit CSR register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to write.
 *
 * \param[in]       value is the 64-bit data value to write.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWriteCSR64(fm_int sw, fm_uint32 addr, fm_uint64 value)
{
    fm_status          err = FM_OK;
    fm_uint32          lo, hi;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = 0x%016" FM_FORMAT_64 "x\n",
                  sw,
                  addr,
                  value);

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    /* Perform I2C access */
    lo = value;
    hi = (value >> 32);

    data = ( ( ((fm_uint64)addr + 0) ) << 32 ) | lo;
    err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);

    if (err == FM_OK)
    {
        data = ( ( ((fm_uint64)addr + 1) ) << 32 ) | hi;
        err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cWriteCSR64 */




/*****************************************************************************/
/** fmPlatformI2cReadCSRMult64
 * \ingroup intPlatform
 *
 * \desc            Read multiple 64-bit CSR registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the starting CSR register address to read.
 *
 * \param[in]       n contains the number of register addresses to read.
 *
 * \param[out]      value points to an array of to be filled in with
 *                  the 64-bit register data read. The array must be n elements
 *                  in length.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadCSRMult64(fm_int     sw,
                                         fm_uint32  addr,
                                         fm_int     n,
                                         fm_uint64 *value)
{
    fm_int    i;
    fm_uint32 lo, hi;
    fm_status err = FM_OK;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    for (i = 0 ; i < n ; i++)
    {
        /* Perform I2C access */
        lo  = addr + (i * 2) + 0;
        err = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &lo, 3, 4);

        if (err == FM_OK)
        {
            hi  = addr + (i * 2) + 1;
            err = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &hi, 3, 4);

            value[i] = ( (fm_uint64) hi << 32 ) | lo;
        }

        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadCSRMult64 */




/*****************************************************************************/
/** fmPlatformI2cWriteCSRMult64
 * \ingroup intPlatform
 *
 * \desc            Write multiple 64-bit CSR registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the starting CSR register address to write.
 *
 * \param[in]       n contains the number of register addresses to write.
 *
 * \param[in]       value points to an array of 64-bit values to write. The
 *                  array must be n elements in length.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWriteCSRMult64(fm_int     sw,
                                          fm_uint32  addr,
                                          fm_int     n,
                                          fm_uint64 *value)
{
    fm_int             i;
    fm_uint32          lo, hi;
    fm_status          err = FM_OK;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    for (i = 0 ; i < n ; i++)
    {
        /* Perform I2C access */
        lo = (value[i] & 0xffffffffL);
        hi = (value[i] >> 32);

        data = ( ( (fm_uint64) addr + (i * 2) + 0) << 32 ) | lo;
        err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);

        if (err == FM_OK)
        {
            data = ( ( (fm_uint64) addr + (i * 2) + 1) << 32 ) | hi;
            err  = fmPlatformI2cWriteLong(sw, 0, FM6000_I2C_ADDR, data, 7);
        }

        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cWriteCSRMult64 */




/*****************************************************************************/
/** fmPlatformI2cReadPreBootCSR
 * \ingroup intPlatform
 *
 * \desc            Read a CSR register via I2C slave before the switch is
 *                  even created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read
 *
 * \param[out]      value points to storage where this function will place
 *                  the read register value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadPreBootCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    fm_status err = FM_OK;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    /* Perform I2C access */
    *value = addr;
    err    = fmPlatformI2cWriteRead(sw,    /*  sw              */
                                    0,     /*  bus             */
                                    FM6000_I2C_ADDR,  /* I2C address */
                                    value, /*  pass reg address, will get reg value */
                                    3,     /*  write length    */
                                    4);    /*  read length     */

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadPreBootCSR */




/*****************************************************************************/
/** fmPlatformI2cReadPreBootCSR64
 * \ingroup intPlatform
 *
 * \desc            Read a 64-bit CSR register before the switch is even
 *                  created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read.
 *
 * \param[out]      value points to storage where the 64-bit read data value
 *                  will be stored by this function.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cReadPreBootCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value)
{
    fm_status err = FM_OK;
    fm_uint32 lo, hi;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    /* Perform I2C access */
    lo  = addr + 0;
    err = fmPlatformI2cWriteRead(sw, 0, 0x40, &lo, 3, 4);

    if (err == FM_OK)
    {
        hi  = addr + 1;
        err = fmPlatformI2cWriteRead(sw, 0, FM6000_I2C_ADDR, &hi, 3, 4);

        *value = ( (fm_uint64) hi << 32 ) | lo;
    }

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cReadPreBootCSR64 */




/*****************************************************************************/
/** fmPlatformI2cWritePreBootCSR
 * \ingroup intPlatform
 *
 * \desc            Write a CSR register without taking any locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read.
 *
 * \param[in]       value is the data value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWritePreBootCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{
    fm_status          err = FM_OK;
    fm_uint64          data;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = 0x%08x\n",
                  sw,
                  addr,
                  value);

    /* Perform I2C access */
    data = ( ( (fm_uint64) addr ) << 32) | value;

    err = fmPlatformI2cWriteLong(sw,    /*  sw              */
                                 0,     /*  bus             */
                                 FM6000_I2C_ADDR,  /* I2C address */
                                 data,  /*  reg_addr and reg_data */
                                 7);    /*  write length    */

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformI2cWritePreBootCSR */
