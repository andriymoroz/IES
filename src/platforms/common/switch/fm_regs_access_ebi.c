/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_regs_access_ebi.c
 * Creation Date:   August 2014
 * Description:     Functions to access chip registers using EBI.
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
/** fmPlatformEbiInit
 * \ingroup intPlatform
 *
 * \desc            Initialize EBI device, if applicable.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       devName is the name to the EBI device.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformEbiInit(fm_int sw, fm_text devName)
{

    return FM_OK;

} /* end fmPlatformEbiInit */




/*****************************************************************************/
/** fmPlatformEbiReadCSR
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
fm_status fmPlatformEbiReadCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiReadCSR */




/*****************************************************************************/
/** fmPlatformEbiWriteCSR
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
fm_status fmPlatformEbiWriteCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteCSR */




/*****************************************************************************/
/** fmPlatformEbiMaskCSR
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
fm_status fmPlatformEbiMaskCSR(fm_int    sw,
                               fm_uint   reg,
                               fm_uint32 mask,
                               fm_bool   on)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(reg);
    FM_NOT_USED(mask);
    FM_NOT_USED(on);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiMaskCSR */




/*****************************************************************************/
/** fmPlatformEbiReadCSRMult
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
fm_status fmPlatformEbiReadCSRMult(fm_int     sw,
                                   fm_uint32  addr,
                                   fm_int     n,
                                   fm_uint32 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(n);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiReadCSRMult */




/*****************************************************************************/
/** fmPlatformEbiWriteCSRMult
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
fm_status fmPlatformEbiWriteCSRMult(fm_int     sw,
                                    fm_uint32  addr,
                                    fm_int     n,
                                    fm_uint32 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(n);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteCSRMult */




/*****************************************************************************/
/** fmPlatformEbiReadCSR64
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
fm_status fmPlatformEbiReadCSR64(fm_int sw, fm_uint32 addr, fm_uint64 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiReadCSR64 */




/*****************************************************************************/
/** fmPlatformEbiWriteCSR64
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
fm_status fmPlatformEbiWriteCSR64(fm_int sw, fm_uint32 addr, fm_uint64 value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteCSR64 */




/*****************************************************************************/
/** fmPlatformEbiReadCSRMult64
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
fm_status fmPlatformEbiReadCSRMult64(fm_int     sw,
                                     fm_uint32  addr,
                                     fm_int     n,
                                     fm_uint64 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(n);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiReadCSRMult64 */




/*****************************************************************************/
/** fmPlatformEbiWriteCSRMult64
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
fm_status fmPlatformEbiWriteCSRMult64(fm_int     sw,
                                      fm_uint32  addr,
                                      fm_int     n,
                                      fm_uint64 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(n);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteCSRMult64 */




/*****************************************************************************/
/** fmPlatformEbiReadRawCSR
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
fm_status fmPlatformEbiReadRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiReadRawCSR */




/*****************************************************************************/
/** fmPlatformEbiWriteRawCSR
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
fm_status fmPlatformEbiWriteRawCSR(fm_int sw, fm_uint32 addr, fm_uint32 value)
{

    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteRawCSR */




/*****************************************************************************/
/** fmPlatformEbiWriteRawCSRSeq
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
fm_status fmPlatformEbiWriteRawCSRSeq(fm_int     sw,
                                      fm_uint32 *addr,
                                      fm_uint32 *value,
                                      fm_int     n)
{
    /* EBI access not yet implemented */
    FM_NOT_USED(sw);
    FM_NOT_USED(addr);
    FM_NOT_USED(n);
    FM_NOT_USED(value);

    return FM_ERR_UNSUPPORTED;;

}   /* end fmPlatformEbiWriteRawCSRSeq */


