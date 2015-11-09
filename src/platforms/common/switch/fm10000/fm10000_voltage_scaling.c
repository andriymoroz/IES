/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_voltage_scaling.c
 * Creation Date:   April 17, 2015
 * Description:     Functions to access chip power supply voltage scaling.
 *
 * Copyright (c) 2015, Intel Corporation
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
#include <platforms/common/switch/fm10000/fm10000_voltage_scaling.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Fusebox word type VRM INFO */
#define FBWT_VRM_INFO         0x9

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


/*****************************************************************************/
/** fm10000DecodeVID
 *
 * \desc            Decodes voltage VR12 VID format to millivolts.
 *
 * \param[in]       vid is the voltage in VR12 VID format.
 *
 * \return          Voltage in millivolts.
 *
 *****************************************************************************/
static fm_uint fm10000DecodeVID(fm_uint vid)
{
    if (vid == 0)
    {
        return 0;
    }
    return 5 * (vid - 1) + 250;

}   /* end fm10000DecodeVID */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000GetNominalSwitchVoltages
 * \ingroup intPlatform
 *
 * \desc            Retrieve nominal switch voltages.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nominalVoltages is a pointer to a caller-allocated
 *                   ''fm_fm10000NominalVoltages'' structure to be filled in by
 *                   this function.
 *
 * \param[in]       readFunction is a pointer to function reading a 32-bit wide
 *                   register.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENTS if one of the argument pointers is
 *                   NULL.
 *
 *****************************************************************************/
fm_status fm10000GetNominalSwitchVoltages(fm_int                    sw,
                                          fm_fm10000NominalVoltages *nominalVoltages,
                                          fm_registerReadUINT32Func readFunction)
{
    fm_status      status;
    fm_uint32      regValue;
    fm_uint        vid;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    if ( (nominalVoltages == NULL) || (readFunction == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*nominalVoltages);

    status = readFunction(sw, FM10000_FUSE_DATA_0(), &regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    vid = FM_GET_UNNAMED_FIELD(regValue, 16, 8);
    if (vid == 0)
    {
        nominalVoltages->VDDS = 850;
        nominalVoltages->defValUsed = TRUE;
    }
    else
    {
        nominalVoltages->VDDS = fm10000DecodeVID(vid);
    }

    vid = FM_GET_UNNAMED_FIELD(regValue, 24, 8);
    if (vid == 0)
    {
        nominalVoltages->VDDF = 950;
        nominalVoltages->defValUsed = TRUE;
    }
    else
    {
        nominalVoltages->VDDF = fm10000DecodeVID(vid);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fm10000GetNominalSwitchVoltages */
