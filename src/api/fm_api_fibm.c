/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_fibm.c
 * Creation Date:   June 20, 2008
 * Description:     Helper Functions for FIBM
 *
 * Copyright (c) 2006 - 2011, Intel Corporation
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
/** fmFibmSlaveGetMasterSwitch
 * \ingroup intFibm
 *
 * \desc            A helper function to return the master switch
 *                  given a slave switch.
 *
 * \param[in]       slaveSw is switch number.
 *
 * \return          master switch or the same input value if FIBM not valid.
 *
 *****************************************************************************/
fm_int fmFibmSlaveGetMasterSwitch(fm_int slaveSw)
{
    fm_switch *         switchPtr;
    fm_status           err;
    fm_fibmSwitchConfig cfg;

    switchPtr = GET_SWITCH_PTR(slaveSw);

    if (switchPtr == NULL || !switchPtr->GetFibmSwitchConfig)
    {
        return slaveSw;
    }

    err = switchPtr->GetFibmSwitchConfig(slaveSw, &cfg);

    if (err != FM_OK)
    {
        return slaveSw;
    }

    /* For standalone NIC, it will be -1, but we just return
     * -1 and let the caller handle appropriately
     */

    return cfg.masterSwitch;

} /* end fmFibmSlaveGetMasterSwitch */




/*****************************************************************************/
/** fmFibmSlaveIsLogicalPortMgmt
 * \ingroup intFibm
 *
 * \desc            A helper function to return whether a logical port on
 *                  the slave switch is used for FIBM mgmt traffic.
 *
 * \param[in]       slaveSw is switch number.
 *
 * \param[in]       logicalPort is logical port number.
 *
 * \return          TRUE if the port is used for FIBM mgmt traffic.
 * \return          FALSE if the port is not used for FIBM mgmt traffic.
 *
 *****************************************************************************/
fm_bool fmFibmSlaveIsLogicalPortMgmt(fm_int slaveSw, fm_int logicalPort)
{
    fm_switch *         switchPtr;
    fm_status           err;
    fm_fibmSwitchConfig cfg;

    switchPtr = GET_SWITCH_PTR(slaveSw);

    if (switchPtr == NULL || !switchPtr->GetFibmSwitchConfig)
    {
        return FALSE;
    }

    err = switchPtr->GetFibmSwitchConfig(slaveSw, &cfg);

    if (err != FM_OK)
    {
        return FALSE;
    }

    if (cfg.slaveMgmtPort == logicalPort)
    {
        return TRUE;
    }

    return FALSE;
} /* end fmFibmSlaveIsLogicalPortMgmt */




/*****************************************************************************/
/** fmFibmSlaveIsPortMgmt
 * \ingroup intFibm
 *
 * \desc            A helper function to return whether a physical port on
 *                  the slave switch is used for FIBM mgmt traffic.
 *
 * \param[in]       slaveSw is switch number.
 *
 * \param[in]       physPort is physical port number.
 *
 * \return          TRUE if the port is used for FIBM mgmt traffic.
 * \return          FALSE if the port is not used for FIBM mgmt traffic.
 *
 *****************************************************************************/
fm_bool fmFibmSlaveIsPortMgmt(fm_int slaveSw, fm_int physPort)
{
    fm_switch *         switchPtr;
    fm_status           err;
    fm_fibmSwitchConfig cfg;
    fm_int              port;

    switchPtr = GET_SWITCH_PTR(slaveSw);

    if (switchPtr == NULL || !switchPtr->GetFibmSwitchConfig)
    {
        return FALSE;
    }

    err = switchPtr->GetFibmSwitchConfig(slaveSw, &cfg);

    if (err != FM_OK)
    {
        return FALSE;
    }

    if ( FM_OK != fmMapLogicalPortToPhysical(switchPtr, 
                                             cfg.slaveMgmtPort, 
                                             &port) )
    {
        return FALSE;
    }

    if (physPort == port)
    {
        return TRUE;
    }

    return FALSE;
} /* end fmFibmSlaveIsPortMgmt */

/*****************************************************************************/
/** fmFibmStartBatching
 * \ingroup intFibm
 *
 * \desc            Start batching of write commands. Specify TRUE to start
 *                  batching of write commands, and FALSE to stop batching
 *                  write commands and flush the pending buffer.
 *
 * \note            It is assumed that the caller has already validated and
 *                  protected the switch prior to calling this function.
 *
 * \param[in]       sw is switch number.
 *
 * \param[in]       start specifies where to TRUE or FALSE
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the platform does not support FIBM
 *                  batching.
 * \return          FM_ERR_UNINITIALIZED if switch is not initialized as an
 *                  FIBM slave.
 * \return          FM_ERR_FIBM_TIMEOUT if no response from remote switch.
 * \return          FM_ERR_NO_MEM if FIBM initialization error.
 * \return          FM_FAIL if FIBM internal processing error.
 *****************************************************************************/
fm_status fmFibmStartBatching(fm_int sw, fm_bool start)
{
    fm_status   err;
    fm_switch  *switchPtr;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_FIBM, "sw = %d start %d\n", sw, start);

    switchPtr = GET_SWITCH_PTR(sw);

    if (fmRootApi->isSwitchFibmSlave[sw])
    {
        FM_API_CALL_FAMILY(err, switchPtr->EnableFibmBatching, sw, start);
    }
    else
    {
        err = FM_ERR_UNINITIALIZED;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_FIBM, err);

} /* end fmFibmStartBatching */
