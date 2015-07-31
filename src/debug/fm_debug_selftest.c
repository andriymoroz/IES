/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_selftest.c
 * Creation Date:   March 5, 2009
 * Description:     FM4000 self test
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

/*****************************************************************************/
/** fmDbgSwitchSelfTest
 * \ingroup diagMisc 
 *
 * \chips           FM3000, FM4000, FM6000
 *
 * \desc            Perform a memory test on the switch. This test is 
 *                  destructive of all prior configuration and disruptive
 *                  of traffic through the switch.
 *                                                                      \lb\lb
 *                  The state of the switch must be carefully managed before
 *                  and after the test is performed. See the description of
 *                  the ctrlState argument below for details.  
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ctrlState should normally be set TRUE, which will
 *                  cause this function to bring down the switch and bring it
 *                  up again prior to initiating the test. The purpose of this
 *                  cycle is to clear any prior switch configuration from the 
 *                  API soft-state, since the test wipes out configuration in 
 *                  the hardware. Following the test, the switch will be left 
 *                  in a DOWN state. To resume normal operation, the switch
 *                  must be brought up by calling ''fmSetSwitchState'' and any
 *                  desired configuration reapplied.
 *                                                                      \lb\lb
 *                  If ctrlState is set to FALSE, this function will not
 *                  affect the switch state. It is the caller's responsbility
 *                  to call ''fmSetSwitchState'' to set the switch down then
 *                  again to set it up prior to the first call to this function.
 *                  Following the test, the switch will be left in the UP 
 *                  state and subsequent calls may be made to this function 
 *                  without further manipulation of the switch's state. 
 *                  Following the last call to this function, it is 
 *                  recommended that ''fmSetSwitchState'' be called to bring 
 *                  the switch DOWN so it may be reset prior to any subsequent 
 *                  normal use.
 *
 * \return          FM_OK if the test passes.
 * \return          FM_FAIL if the test fails.
 * \return          FM_ERR_INVALID_SWITCH sw is not a valid switch number, not
 *                  a physical switch or a member of a switch aggregation 
 *                  (SWAG).
 *
 *****************************************************************************/
 
/* Some implementation note core test code on FM2000:
 *
 *      On FM2000, it may be necessary to disable LEDs at beginning of test 
 *      and reenable at end raw register accesses can be made without fear of 
 *      encountering Errata #10.
 */
 
fm_status fmDbgSwitchSelfTest(fm_int sw, fm_bool ctrlState)
{
    fm_status               err;
    fm_status               testResult = FM_FAIL;
    fm_switch               *switchPtr;
    fm_bool                 switchLocked = FALSE;
    fm_paritySweeperConfig  currentParitySweeper;
    fm_paritySweeperConfig  paritySweeper;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);
    VALIDATE_SWITCH_LOCK(sw);

    /* Take write access to the switch lock */
    err = LOCK_SWITCH(sw);
    
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to capture switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);

        goto ABORT;
    }

    switchLocked = TRUE;

    /* return error if switch is not found. */
    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    switchPtr = fmRootApi->fmSwitchStateTable[sw];
    
    /* Do not perform test on a SWAG or a member of a SWAG. */
    if (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG ||
        switchPtr->swag >= 0)
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (ctrlState)
    {
        /**************************************************
         * First bring the switch down. This will clear
         * all API soft state and delete VLANs, LAGs,
         * multicast groups, routes, etc.
         **************************************************/
        
        err = fmSetSwitchState(sw, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
        /**************************************************
         * Now bring the switch back up so we can perform
         * the memory test.
         **************************************************/
        
        err = fmSetSwitchState(sw, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
    }
    
    /**************************************************
     * Make Sure Parity Sweeper is disabled
     **************************************************/
    paritySweeper.enabled = FALSE;
    fmGetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &currentParitySweeper);
    fmSetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &paritySweeper);

    UNLOCK_SWITCH(sw);
    switchLocked = FALSE;

    /**************************************************
     * Now perform the test.
     **************************************************/
    FM_API_CALL_FAMILY(testResult, switchPtr->DbgSwitchSelfTest, sw);
     
    if (ctrlState)
    {
        /**************************************************
         * Bring switch down again. We have a test result,
         * so ignore the return code when calling 
         * fmSetSwitchState since we only care about the
         * test result now.
         **************************************************/
        
        fmSetSwitchState(sw, FALSE);
    }

    fmSetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &currentParitySweeper);
    
ABORT:

    if (switchLocked)
    {
        UNLOCK_SWITCH(sw);
    }
    
    if (err == FM_OK)
    {
        err = testResult;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmDbgSwitchSelfTest */




/*****************************************************************************/
/** fmDbgPolicerTest
  * \ingroup intDiagMisc 
 *
 * \chips           FM6000
 *
 * \desc            Perform a memory test on the switch to validates the
 *                  Policer DualSram 1K Bloc
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ctrlState should normally be set TRUE, which will
 *                  cause this function to bring down the switch and bring it
 *                  up again prior to initiating the test. The purpose of this
 *                  cycle is to clear any prior switch configuration from the 
 *                  API soft-state, since the test wipes out configuration in 
 *                  the hardware. Following the test, the switch will be left 
 *                  in a DOWN state. To resume normal operation, the switch
 *                  must be brought up by calling ''fmSetSwitchState'' and any
 *                  desired configuration reapplied.
 *                                                                      \lb\lb
 *                  If ctrlState is set to FALSE, this function will not
 *                  affect the switch state. It is the caller's responsbility
 *                  to call ''fmSetSwitchState'' to set the switch down then
 *                  again to set it up prior to the first call to this function.
 *                  Following the test, the switch will be left in the UP 
 *                  state and subsequent calls may be made to this function 
 *                  without further manipulation of the switch's state. 
 *                  Following the last call to this function, it is 
 *                  recommended that ''fmSetSwitchState'' be called to bring 
 *                  the switch DOWN so it may be reset prior to any subsequent 
 *                  normal use.
 *
 * \param[in]       portList refer to an array of logical ports used to perform
 *                  the test.
 *
 * \param[in]       portCnt is the size of the portList array.
 *
 * \param[in]       mrlLimiter define if mrl rate limiter must be set.
 *
 * \return          FM_OK if the test passes.
 * \return          FM_FAIL if the test fails.
 * \return          FM_ERR_INVALID_SWITCH sw is not a valid switch number, not
 *                  a physical switch or a member of a switch aggregation 
 *                  (SWAG).
 *
 *****************************************************************************/
fm_status fmDbgPolicerTest(fm_int  sw,
                           fm_bool ctrlState,
                           fm_int *portList,
                           fm_int  portCnt,
                           fm_bool mrlLimiter)
{
    fm_status               err;
    fm_status               testResult = FM_FAIL;
    fm_switch               *switchPtr;
    fm_bool                 switchLocked = FALSE;
    fm_paritySweeperConfig  currentParitySweeper;
    fm_paritySweeperConfig  paritySweeper;
    fm_int                  i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);
    VALIDATE_SWITCH_LOCK(sw);

    /* Take write access to the switch lock */
    err = LOCK_SWITCH(sw);
    
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to capture switch write lock %p for switch %d\n",
                     (void *) fmRootApi->fmSwitchLockTable[sw],
                     sw);

        goto ABORT;
    }

    switchLocked = TRUE;

    /* return error if switch is not found. */
    if (!fmRootApi->fmSwitchStateTable[sw])
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    switchPtr = fmRootApi->fmSwitchStateTable[sw];
    
    /* Do not perform test on a SWAG or a member of a SWAG. */
    if (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG ||
        switchPtr->swag >= 0)
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    if (ctrlState)
    {
        /**************************************************
         * First bring the switch down. This will clear
         * all API soft state and delete VLANs, LAGs,
         * multicast groups, routes, etc.
         **************************************************/
        
        err = fmSetSwitchState(sw, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
        /**************************************************
         * Now bring the switch back up so we can perform
         * the memory test.
         **************************************************/
        
        err = fmSetSwitchState(sw, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
    }
    
    /**************************************************
     * Make Sure Parity Sweeper is disabled
     **************************************************/
    paritySweeper.enabled = FALSE;
    fmGetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &currentParitySweeper);
    fmSetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &paritySweeper);

    UNLOCK_SWITCH(sw);
    switchLocked = FALSE;

    /************************************************** 
     * Initialize basic configuration needed for the test.
     **************************************************/
    fmCreateVlan(sw, 1);

    for (i = 0 ; i < portCnt ; i++)
    {
        fmSetPortState(sw, portList[i], FM_PORT_STATE_UP, 0);
        fmAddVlanPort(sw, 1, portList[i], FALSE);
        fmSetVlanPortState(sw,
                           1,
                           portList[i],
                           FM_STP_STATE_FORWARDING);
    }

    /**************************************************
     * Now perform the test.
     **************************************************/
    FM_API_CALL_FAMILY(testResult,
                       switchPtr->DbgPolicerTest,
                       sw,
                       portList,
                       portCnt,
                       mrlLimiter);
     
    if (ctrlState)
    {
        /**************************************************
         * Bring switch down again. We have a test result,
         * so ignore the return code when calling 
         * fmSetSwitchState since we only care about the
         * test result now.
         **************************************************/
        
        fmSetSwitchState(sw, FALSE);
    }

    fmSetSwitchAttribute(sw, FM_PARITY_SWEEPER_CONFIG, &currentParitySweeper);
    
ABORT:

    if (switchLocked)
    {
        UNLOCK_SWITCH(sw);
    }
    
    if (err == FM_OK)
    {
        err = testResult;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_SWITCH, err);

}   /* end fmDbgPolicerTest */

