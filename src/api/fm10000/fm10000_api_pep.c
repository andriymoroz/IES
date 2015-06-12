/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_pep.c
 * Creation Date:   September, 2014
 * Description:     Functions related to PEPs
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

#include <fm_sdk_fm10000_int.h>

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
/** fm10000GetPepResetState
 * \ingroup intSwitch
 *
 * \desc            Retrieve the reset state of a PEP. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pepId is the PEP in which the register should be read.
 *
 * \param[out]      state points to storage where this function will store
 *                  the state of the PEP (0: In Reset, 1: Active)
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000GetPepResetState(fm_int sw, 
                                  fm_int pepId, 
                                  fm_bool *state)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  rv;

    switchPtr = GET_SWITCH_PTR(sw);

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_DEVICE_CFG(),
                                &rv);

    if (rv & (1 << (FM10000_DEVICE_CFG_b_PCIeEnable_0 + pepId) ) )
    {
        err = switchPtr->ReadUINT32(sw, 
                                    FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), pepId),
                                    &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        *state = FM_GET_BIT(rv, FM10000_PCIE_IP, NotInReset);
    }
    else
    {
        *state = 0;
    }
    
ABORT:
    return err;

}   /* end fm10000GetPepResetState */




/*****************************************************************************/
/** fm10000ReadPep
 * \ingroup intSwitch
 *
 * \desc            Read a PEP CSR register. This function will do a
 *                  sanity check on the PEP's reset state prior to the read.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read
 * 
 * \param[in]       pepId is the PEP in which the register should be read.
 *
 * \param[out]      value points to storage where this function will place
 *                  the read register value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_STATE if the PEP is in reset or is
 *                  not enabled.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000ReadPep(fm_int    sw, 
                         fm_uint32 addr, 
                         fm_int    pepId, 
                         fm_uint32 *value)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_bool    pepState;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, addr = 0x%08x, pepId = %d, value = %p\n",
                         sw,
                         addr,
                         pepId,
                         (void *) value);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fm10000GetPepResetState(sw, pepId, &pepState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (pepState == 0)
    {
        err = FM_ERR_INVALID_STATE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    err = switchPtr->ReadUINT32(sw, 
                                FM10000_PCIE_PF_ADDR(addr, pepId),
                                value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000GetPepResetState(sw, pepId, &pepState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (pepState == 0)
    {
        err = FM_ERR_INVALID_STATE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000ReadPep */




/*****************************************************************************/
/** fm10000WritePep
 * \ingroup intSwitch
 *
 * \desc            Write a PEP CSR register. This function will do a
 *                  sanity check on the PEP's reset state prior to the write.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       addr contains the CSR register address to read.
 * 
 * \param[in]       pepId is the PEP in which the register should be written.
 *
 * \param[in]       value is the data value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000WritePep(fm_int sw, 
                          fm_uint32 addr, 
                          fm_int pepId, 
                          fm_uint32 value)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_bool    pepState;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, addr = 0x%08x, pepId = %d, value = 0x%08x\n",
                         sw,
                         addr,
                         pepId,
                         value);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fm10000GetPepResetState(sw, pepId, &pepState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (pepState == 0)
    {
        err = FM_ERR_INVALID_STATE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    err = switchPtr->WriteUINT32(sw, 
                                 FM10000_PCIE_PF_ADDR(addr, pepId),
                                 value);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000GetPepResetState(sw, pepId, &pepState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (pepState == 0)
    {
        err = FM_ERR_INVALID_STATE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000WritePep */
