/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_bsm.c
 * Creation Date:   April 24, 2015
 * Description:     BSM utils debugging functions.
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
/** fmDbgPollLtssm
 * \ingroup diagBsm
 * 
 * \chips           FM10000
 *
 * \desc            LTSSM polling function. 
 *                  Examines the switch LTSSM and LinkCtrl registers.
 *                  Registers value change detection dumps the corresponding
 *                  dbg message. 
 *
 * \param[in]       sw switch the LTSSM will be examined.
 * 
 * \param[in]       pep is the PEP to poll LTSSM for, use -1 to poll all PEPs.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmDbgPollLtssm(fm_int    sw,
                         fm_int    pep,
                         fm_uint32 miliSec)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);
    
    PROTECT_SWITCH( sw );
    
    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }
    
    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgPollLtssm,
                        sw,
                        switchPtr->ReadUINT32,
                        pep,
                        miliSec )
    
ABORT:
    UNPROTECT_SWITCH( sw );
    
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end fmDbgPollLtssm */




/*****************************************************************************/
/** fmDbgPollReset
 * \ingroup diagBsm
 * 
 * \chips           FM10000
 *
 * \desc            Reset polling function.
 *                  Examines the PCIE_RESET_N and LTSSM to determine how
 *                  long it took from reset to start of link training.
 *                  Registers value change detection dumps the corresponding
 *                  dbg message. 
 *
 * \param[in]       sw switch the LTSSM will be examined.
 * 
 * \param[in]       pep is the PEP to poll LTSSM for, use -1 to poll all PEPs.
 * 
 * \param[in]       miliSec polling time in milliseconds.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmDbgPollReset(fm_int    sw,
                         fm_int    pep,
                         fm_uint32 miliSec)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);
    
    PROTECT_SWITCH( sw );
    
    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }
    
    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgPollReset,
                        sw,
                        switchPtr->ReadUINT32,
                        pep,
                        miliSec )
    
ABORT:
    UNPROTECT_SWITCH( sw );
    
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end fmDbgPollReset */




/*****************************************************************************/
/** fmDbgPollBsmStatus
 * \ingroup diagBsm
 * 
 * \chips           FM10000
 *
 * \desc            Switch BSM_STATUS register polling function.
 *                  Examines the BSM_STATUS register, value change detection
 *                  dumps the corresponding dbg message.
 *
 * \param[in]       sw is the switch BSM_STATUS will be examined.
 *
 * \param[in]       miliSec polling time in milliseconds.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmDbgPollBsmStatus(fm_int    sw,
                             fm_uint32 miliSec)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    PROTECT_SWITCH( sw );
    
    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }
    
    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgPollBsmStatus,
                        sw,
                        switchPtr->ReadUINT32,
                        miliSec )
    
ABORT:
    UNPROTECT_SWITCH( sw );

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmDbgPollBsmStatus */





/*****************************************************************************/
/** fmDbgDumpBsmScratch
 * \ingroup diagBsm
 * 
 * \chips           FM10000
 *
 * \desc            Dbg dump detailed BSM_SCRATCH registers for the switch.
 *
 * \param[in]       sw is the switch BSM_SCRATCH will be dumped.
 *
 * \param[in]       regMask used to filter out the registers dumps used as
 *                  a bitmap combination out of the following values. 
 *                  
 *                  REG_MASK_BSM_LOCKS                  registers 0..1
 *                  REG_MASK_BSM_CONFIG                 registers 10..199
 *                  REG_MASK_BSM_INIT_STATUS            registers 400..409
 *                  REG_MASK_BSM_INIT_STATUS_ARCHIVE    registers 430..440
 *                  REG_MASK_BSM_INIT_OOR               registers 450..451
 *                  REG_MASK_BSM_ISR_STATUS_PCIE0       register 441
 *                  REG_MASK_BSM_ISR_STATUS_PCIE1       register 442
 *                  REG_MASK_BSM_ISR_STATUS_PCIE2       register 443
 *                  REG_MASK_BSM_ISR_STATUS_PCIE3       register 444
 *                  REG_MASK_BSM_ISR_STATUS_PCIE4       register 445
 *                  REG_MASK_BSM_ISR_STATUS_PCIE5       register 446
 *                  REG_MASK_BSM_ISR_STATUS_PCIE6       register 447
 *                  REG_MASK_BSM_ISR_STATUS_PCIE7       register 448
 *                  REG_MASK_BSM_ISR_STATUS_PCIE8       register 449
 *                  REG_MASK_BSM_ISR_PCIE_ENABLED       all enabled PCIEs
 *                  REG_MASK_BSM_PCIE_ISR_STATUS        all PCIEs, 441..449
 *                  REG_MASK_BSM_INIT_ALL               all BSM_SCRATCH init 
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmDbgDumpBsmScratch(fm_int    sw,
                              fm_uint32 regMask)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, err);
    }

    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgDumpBsmScratch,
                        sw,
                        switchPtr->ReadUINT32,
                        regMask );
    
ABORT:
    UNPROTECT_SWITCH( sw );

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    
}   /* end fmDbgDumpBsmScratch */
