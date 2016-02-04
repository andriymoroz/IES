/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_regs_access_memmap.c
 * Creation Date:   March 8, 2012
 * Description:     Functions to access chip registers using memmap.
 *
 * Copyright (c) 2012 - 2013, Intel Corporation
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
#include <platforms/common/instrument/platform_instrument.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
/* Intel-internal test instrumentation */
#ifndef INSTRUMENT_REG_WRITE
#define INSTRUMENT_REG_WRITE(sw, addr, val)
#endif

/* Enable this if logging for CSR access is needed */
#if 0
#define CSR_LOG_ENTRY FM_LOG_ENTRY_VERBOSE
#define CSR_LOG_EXIT  FM_LOG_EXIT_VERBOSE
#else
#define CSR_LOG_ENTRY(...)
#define CSR_LOG_EXIT(cat, status) return (status)
#endif

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
/** fmPlatformReadCSR
 * \ingroup intPlatform
 *
 * \desc            Read a CSR register.
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
fm_status fmPlatformReadCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    *value = GET_PLAT_MEMMAP_CSR(sw)[addr];

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformReadCSR */




/*****************************************************************************/
/** fmPlatformWriteCSR
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
fm_status fmPlatformWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = 0x%08x\n",
                  sw,
                  addr,
                  value);

    if (GET_PLAT_STATE(sw)->bypassEnable && BYPASS_ADDR_CHECK(addr) )
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    INSTRUMENT_REG_WRITE(sw, addr, value);
    GET_PLAT_MEMMAP_CSR(sw)[addr] = value;

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformWriteCSR */




/*****************************************************************************/
/** fmPlatformMaskCSR
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
 *                   or FALSE to clear the masked bits in the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMaskCSR(fm_int sw, fm_uint reg, fm_uint32 mask, fm_bool on)
{
    fm_uint32 value;

    CSR_LOG_ENTRY( FM_LOG_CAT_PLATFORM,
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

    value = GET_PLAT_MEMMAP_CSR(sw)[reg];

    if (on)
    {
        value |= mask;
    }
    else
    {
        value &= ~mask;
    }

    GET_PLAT_MEMMAP_CSR(sw)[reg] = value;

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformMaskCSR */




/*****************************************************************************/
/** fmPlatformReadCSRMult
 * \ingroup intPlatform
 *
 * \desc            Read multiple CSR registers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the starting CSR register address to read.
 *
 * \param[in]       n contains the number of consecutive register addresses
 *                  to read.
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
fm_status fmPlatformReadCSRMult(fm_int     sw,
                                fm_uint32  addr,
                                fm_int     n,
                                fm_uint32 *value)
{
    fm_int i;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    for (i = 0 ; i < n ; i++)
    {
        value[i] = GET_PLAT_MEMMAP_CSR(sw)[addr + i];
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformReadCSRMult */




/*****************************************************************************/
/** fmPlatformWriteCSRMult
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
fm_status fmPlatformWriteCSRMult(fm_int     sw,
                                 fm_uint32  addr,
                                 fm_int     n,
                                 fm_uint32 *value)
{
    fm_int i;

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
        INSTRUMENT_REG_WRITE(sw, addr + i, value[i]);
        GET_PLAT_MEMMAP_CSR(sw)[addr + i] = value[i];
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformWriteCSRMult */




/*****************************************************************************/
/** fmPlatformReadCSR64
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
fm_status fmPlatformReadCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value)
{
    fm_uint64 lo, hi;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, value = %p\n",
                  sw,
                  addr,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    lo = GET_PLAT_MEMMAP_CSR(sw)[addr + 0];
    hi = GET_PLAT_MEMMAP_CSR(sw)[addr + 1];
    *value = (hi << 32) | lo;

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformReadCSR64 */




/*****************************************************************************/
/** fmPlatformWriteCSR64
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
fm_status fmPlatformWriteCSR64(fm_int sw, fm_uint32 addr, fm_uint64 value)
{
    fm_uint32 lo;
    fm_uint32 hi;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, "
                  "value = 0x%016" FM_FORMAT_64 "x\n",
                  sw,
                  addr,
                  value);

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    lo = value; /* low 32-bit */
    hi = (value >> 32);

    INSTRUMENT_REG_WRITE(sw, addr + 0, lo);
    INSTRUMENT_REG_WRITE(sw, addr + 1, hi);

    GET_PLAT_MEMMAP_CSR(sw)[addr + 0] = lo;
    GET_PLAT_MEMMAP_CSR(sw)[addr + 1] = hi;

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformWriteCSR64 */




/*****************************************************************************/
/** fmPlatformReadCSRMult64
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
fm_status fmPlatformReadCSRMult64(fm_int     sw,
                                  fm_uint32  addr,
                                  fm_int     n,
                                  fm_uint64 *value)
{
    fm_int    i;
    fm_uint64 lo, hi;

    CSR_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                  "sw = %d, addr = 0x%08x, n = %d, value = %p\n",
                  sw,
                  addr,
                  n,
                  (void *) value);

    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    for (i = 0 ; i < n ; i++)
    {
        lo = GET_PLAT_MEMMAP_CSR(sw)[addr + 0 + i*2];
        hi = GET_PLAT_MEMMAP_CSR(sw)[addr + 1 + i*2];
        value[i] = (hi << 32) | lo;
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformReadCSRMult64 */




/*****************************************************************************/
/** fmPlatformWriteCSRMult64
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
fm_status fmPlatformWriteCSRMult64(fm_int     sw,
                                   fm_uint32  addr,
                                   fm_int     n,
                                   fm_uint64 *value)
{
    fm_int    i;
    fm_uint32 lo, hi;

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
        lo = value[i];
        hi = (value[i] >> 32);

        INSTRUMENT_REG_WRITE(sw, addr + 0 + (i * 2), lo);
        INSTRUMENT_REG_WRITE(sw, addr + 1 + (i * 2), hi);

        GET_PLAT_MEMMAP_CSR(sw)[addr + 0 + i*2] = lo;
        GET_PLAT_MEMMAP_CSR(sw)[addr + 1 + i*2] = hi;
    }

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    CSR_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformWriteCSRMult64 */




/*****************************************************************************/
/** fmPlatformReadRawCSR
 * \ingroup intPlatform
 *
 * \desc            Read a CSR register without taking the platform lock.
 *                                                                      \lb\lb
 *                  The caller must have taken the platform lock, or a write
 *                  lock on the entire switch, prior to calling this function
 *                  and is responsible for releasing the lock.
 *                                                                      \lb\lb
 *                  This function may be called multiple times consecutively
 *                  for multiple word width registers since it is assumed that
 *                  the caller has taken any locks necessary to assure
 *                  atomicity.
 *
 * \note            Implementation of this function in the platform layer is
 *                  optional. If not implemented, the calling function
 *                  will automatically use the normal register read function,
 *                  which will cause it to just execute much more slowly than 
 *                  if this "raw" function is implemented.
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
fm_status fmPlatformReadRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    *value = GET_PLAT_MEMMAP_CSR(sw)[addr];

    return FM_OK;
    
}   /* end fmPlatformReadRawCSR */




/*****************************************************************************/
/** fmPlatformWriteRawCSR
 * \ingroup intPlatform
 *
 * \desc            Write a CSR register without taking the platform lock.
 *                                                                      \lb\lb
 *                  The caller must have taken the platform lock, or a write
 *                  lock on the entire switch, prior to calling this function
 *                  and is responsible for releasing the lock.
 *                                                                      \lb\lb
 *                  This function may be called multiple times consecutively
 *                  for multiple word width registers since it is assumed that
 *                  the caller has taken any locks necessary to assure
 *                  atomicity.
 *
 * \note            Implementation of this function in the platform layer is
 *                  optional. If not implemented, the calling function
 *                  will automatically use the normal register write function,
 *                  which will cause it to just execute much more slowly than 
 *                  if this "raw" function is implemented.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to write.
 *
 * \param[in]       value is the data value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformWriteRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        return FM_OK;
    }

    INSTRUMENT_REG_WRITE(sw, addr, value);
    GET_PLAT_MEMMAP_CSR(sw)[addr] = value;

    return FM_OK;

}   /* end fmPlatformWriteRawCSR */




/*****************************************************************************/
/** fmPlatformWriteRawCSRSeq
 * \ingroup intPlatform
 *
 * \desc            Write a sequence of CSR register without taking the
 *                  platform lock.
 *                                                                      \lb\lb
 *                  The caller must have taken the platform lock, or a write
 *                  lock on the entire switch, prior to calling this function
 *                  and is responsible for releasing the lock.
 *
 * \note            Implementation of this function in the platform layer is
 *                  optional. If not implemented, the calling function
 *                  will automatically use the normal register write function,
 *                  which will cause it to just execute much more slowly than 
 *                  if this "raw" function is implemented.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr points to an array of register address to be written.
 *                  The array must be n elements in length.
 *
 * \param[in]       value points to an array of values to be written. The
 *                  array must be n elements in length.
 * 
 * \param[in]       n contains the number of register addresses to write.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformWriteRawCSRSeq(fm_int     sw,
                                   fm_uint32 *addr,
                                   fm_uint32 *value,
                                   fm_int     n)
{
    fm_int i;

    if (GET_PLAT_STATE(sw)->bypassEnable)
    {
        return FM_OK;
    }

    for (i = 0 ; i < n ; i++)
    {
        INSTRUMENT_REG_WRITE(sw, addr[i], value[i]);
        GET_PLAT_MEMMAP_CSR(sw)[addr[i]] = value[i];
    }

    return FM_OK;

}   /* end fmPlatformWriteRawCSRSeq */


