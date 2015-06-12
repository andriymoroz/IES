/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_app_stubs.c
 * Creation Date:   January 11, 2013
 * Description:     Default implementations of Platform Application
 * 					service functions.
 *
 * Copyright (c) 2006 - 2014, Intel Corporation
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

static int dummyVariable;


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * Public Functions
 *****************************************************************************/




#if !defined(FM_HAVE_fmPlatformMdioRead)

/*****************************************************************************/
/** fmPlatformMdioRead
 * \ingroup platformApp
 *
 * \desc            Read 16 bits of data from the MDIO bus.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       type is a bit mask specifying MDIO access options.
 *                  See MDIO Management Options.
 *
 * \param[in]       addr is the MDIO address.
 *
 * \param[in]       dev is the device on the MDIO port. dev is only used if
 *                  type includes the FM_SMGMT_MDIO_10G bit, otherwise it
 *                  is ignored.
 *
 * \param[in]       reg is the register number on the MDIO device.
 *
 * \param[out]      data contains the value read from the MDIO device.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_int fmPlatformMdioRead(fm_int     sw,
                          fm_int     type,
                          fm_int     addr,
                          fm_int     dev,
                          fm_int     reg,
                          fm_uint16 *data)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(type);
    FM_NOT_USED(addr);
    FM_NOT_USED(dev);
    FM_NOT_USED(reg);
    FM_NOT_USED(data);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);

}   /* end fmPlatformMdioRead */

#endif




#if !defined(FM_HAVE_fmPlatformMdioWrite)

/*****************************************************************************/
/** fmPlatformMdioWrite
 * \ingroup platformApp
 *
 * \desc            Write 16 bits of data to the MDIO bus.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       type is a bit mask specifying MDIO access options.
 *                  See MDIO Management Options.
 *
 * \param[in]       addr is the MDIO physical address.
 *
 * \param[in]       dev is the device on the MDIO port. dev is only used if
 *                  type includes the FM_SMGMT_MDIO_10G bit, otherwise it
 *                  is ignored.
 *
 * \param[in]       reg is the register number on the MDIO device.
 *
 * \param[in]       data contains the value to write to the MDIO device.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_int fmPlatformMdioWrite(fm_int    sw,
                           fm_int    type,
                           fm_int    addr,
                           fm_int    dev,
                           fm_int    reg,
                           fm_uint16 data)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(type);
    FM_NOT_USED(addr);
    FM_NOT_USED(dev);
    FM_NOT_USED(reg);
    FM_NOT_USED(data);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);

}   /* end fmPlatformMdioWrite */

#endif

