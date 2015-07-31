/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_event_intr.c
 * Creation Date:   May 16, 2013
 * Description:     Focalpoint 10000 series interrupt task
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Detects an interrupt, masks it, and clears it. */
#define DETECT_AND_MASK_INT32(master, bitmask, ipreg, imreg, ignoreMask, intMask) \
    if ( master & (bitmask) )                                                     \
    {                                                                             \
        switchPtr->ReadUINT32(sw, (ipreg), &ipTmp32);                             \
        switchPtr->ReadUINT32(sw, (imreg), &imTmp32);                             \
        ipTmp32 &= ~ignoreMask;                                                   \
        *(intMask) |= ipTmp32 & ~imTmp32;                                         \
        switchPtr->WriteUINT32(sw, (imreg), imTmp32 | ipTmp32);                   \
        switchPtr->WriteUINT32(sw, (ipreg), imTmp32 | ipTmp32);                   \
    }

/* Detects an interrupt, masks it, and clears it.
 * Unlike DETECT_AND_MASK_INT32, this macro only acknowledges UNMASKED
 * IP register bits. */
#define DETECT_AND_MASK32_V2(master, bitmask, ipreg, imreg, ignoreMask, intMask) \
    if ( master & (bitmask) )                                                    \
    {                                                                            \
        switchPtr->ReadUINT32(sw, (ipreg), &ipTmp32);                            \
        switchPtr->ReadUINT32(sw, (imreg), &imTmp32);                            \
        ipTmp32 &= ~ignoreMask;                                                  \
        *(intMask) |= ipTmp32 & ~imTmp32;                                        \
        switchPtr->WriteUINT32(sw, (imreg), imTmp32 | ipTmp32);                  \
        switchPtr->WriteUINT32(sw, (ipreg), ipTmp32 & ~imTmp32);                 \
    }

/* Detects a global interrupt, masks it, and clears it.
 * The variable 'global' must be defined in the calling function. */
#define DETECT_AND_MASK_GLOBAL(bitmask, ipreg, imreg, ignoreMask, intMask) \
    DETECT_AND_MASK_INT32(global, bitmask, ipreg, imreg, ignoreMask, intMask)

/* Detects a global interrupt, masks it, and clears it.
 * The variable 'global' must be defined in the calling function. 
 * Unlike DETECT_AND_MASK_GLOBAL, this macro only acknowledges UNMASKED
 * IP register bits. */
#define DETECT_AND_MASK_GLOBAL_V2(bitmask, ipreg, imreg, ignoreMask, intMask) \
    DETECT_AND_MASK32_V2(global, bitmask, ipreg, imreg, ignoreMask, intMask)

/* For cases where aborting after failure isn't desireable, such as 
 * processing multiple PEP interrupts.  If one PEP fails, continue to
 * handle interrupts from other PEPs instead of aborting completely. 
 * badStatus allows storing of this error if a failure occurred. */
#define FM_LOG_CONTINUE_ON_ERR(cat, errcode, badStatus)                 \
    {                                                                   \
        fm_status localError = (errcode);                               \
        if ( localError != FM_OK )                                      \
        {                                                               \
            FM_LOG_DEBUG((cat),                                         \
                         "Failure occurred, continuing: %s\n",          \
                         fmErrorMsg(localError));                       \
            FM_ERR_COMBINE(badStatus, localError);                      \
            continue;                                                   \
        }                                                               \
    }

/* PCIE recovery mechanism related defines */
#define PCIE_IM_RECOVER_MASK                (1 << FM10000_PCIE_IM_b_DeviceStateChange)


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
/** StopPepStatusPollingTimer
 * \ingroup intSwitch
 *
 * \desc            Action stopping the pep status polling timer 
 * 
 * \param[in]       sw is the switch number on which to operate on.
 * 
 * \param[in]       port is the logical port of the PEP on which we want
 *                  to start a recovery timer.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
static fm_status StopPepStatusPollingTimer( fm_int sw, fm_int port )
{
    fm_status     status;
    fm10000_port *portExt;

    status = FM_OK;

    portExt = GET_PORT_EXT(sw, port);

    status = fmStopTimer( portExt->pcieIntrTimerHandle );

    return status;

}   /* end StopPepStatusPollingTimer */



/*****************************************************************************/
/** HandlePepStatusPollingTimerEvent
 * \ingroup intSwitch
 * 
 * \desc            Handles a the expiration of the timer used to defer the
 *                  completion of the PLL power up process
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the port extension
 *                  structure (type ''fm10000_port'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandlePepStatusPollingTimerEvent( void *arg )
{
    fm_status                err;
    fm_int                   sw;
    fm_switch               *switchPtr;
    fm10000_port            *portExt;
    fm_int                   port;
    fm_int                   pep;
    fm_bool                  curPepState;
    fm_uint32                imTmp32;
    
    portExt           = arg;
    switchPtr         = portExt->base->switchPtr;
    sw                = switchPtr->switchNumber;
    port              = portExt->base->portNumber;
    pep               = portExt->endpoint.pep;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_EVENT_INTR,
                     port,
                     "Polling timer expired on PEP %d\n",
                     pep);

    PROTECT_SWITCH(sw);
    TAKE_REG_LOCK(sw);

    /* Is the PEP still in reset? */
    err = fm10000GetPepResetState(sw, pep, &curPepState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, err);

    /* PEP is not in reset */
    if ( curPepState )
    {
        /* Unmask the PCIE_IM.DeviceStateChange interrupt */
        err = switchPtr->ReadUINT32(sw, 
                              FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                              pep),
                              &imTmp32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, err);

        FM_SET_BIT(imTmp32, FM10000_PCIE_IP, DeviceStateChange, 0);
        err = switchPtr->WriteUINT32(sw, 
                               FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                               pep), 
                               imTmp32);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, err);


        /* Stop the timer */
        err = StopPepStatusPollingTimer(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, err);
    }

ABORT:
    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

}   /* end HandlePepStatusPollingTimerEvent */



/*****************************************************************************/
/** fm10000StartPepStatusPollingTimer
 * \ingroup intSwitch
 *
 * \desc            Action starting the pep status polling timer.
 * 
 * \param[in]       sw is the switch number on which to operate on.
 * 
 * \param[in]       port is the logical port of the PEP on which we want
 *                  to start a recovery timer.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartPepStatusPollingTimer( fm_int sw, fm_int port )
{
    fm_status     status;
    fm10000_port *portExt;
    fm_timestamp  timeout = { 2, 0 };

    status = FM_OK;

    portExt = GET_PORT_EXT(sw, port);

    /* start an 2 sec port-level timer. The callback will check if
       the PEP is still in reset or not. */
    status = fmStartTimer( portExt->pcieIntrTimerHandle,
                           &timeout,
                           FM_TIMER_REPEAT_FOREVER, 
                           HandlePepStatusPollingTimerEvent,
                           portExt );
    return status;

}   /* end fm10000StartPepStatusPollingTimer */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000InterruptHandler
 * \ingroup intSwitch
 *
 * \desc            FocalPoint task-level interrupt handler.  Waits on a
 *                  semaphore set by the platform layer.  Masks interrupts
 *                  as needed, and calls platform layer handlers for packet
 *                  recipt and sending.
 *
 * \param[in]       switchPtr points to the fm_switch structure for the
 *                  interrupting switch.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InterruptHandler(fm_switch *switchPtr)
{
    fm_int              sw;
    fm10000_interrupt   currentIntr;
    fm_status           status;
    fm_uint64           global;
    fm_uint64           pcieIntMask;
    fm_uint64           eplIntMask;
    fm_uint32           ipTmp32;
    fm_uint32           imTmp32;
    fm_uint32           fh_tail;
    fm_int              i;
    fm_int              j;
    fm_bool             intrSendPackets;
    fm_uint32           eplIp;
    fm_uint32           swIp;
    fm_uint32           anIntMask;
    fm_uint32           linkIntMask;
    fm_uint32           serDesIntMask;
    fm_status           retStatus = FM_OK;
    fm_bool             regLockTaken = FALSE;
    fm10000_switch *    switchExt;
    fm_uint32           devCfg;
    fm_uint32           rv;
    fm_bool             curPepState;
    fm_uint32           softReset;
    fm_bool             sendLinkDownEvent = FALSE;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_INTR,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    sw = switchPtr->switchNumber;

    /**************************************************
     * Once signaled, we need to do interrupt processing
     * repeatedly until we see no more interrupts on
     * the Focalpoint.
     **************************************************/

    /* clear temp storage */
    memset( &currentIntr, 0, sizeof(currentIntr) );

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Sw %d: Executing pass\n", sw);

    /**************************************************
     * Get the current set of message flags.
     *
     * The platform lock is used instead of state lock
     * because on FIBM platforms, there is an access
     * to intrSendPackets that must be protected before
     * the switch's locks are even created. 
     **************************************************/

    FM_TAKE_PKT_INT_LOCK(sw);

    intrSendPackets = switchPtr->intrSendPackets;
    
    switchPtr->intrSendPackets = FALSE;
    
    FM_DROP_PKT_INT_LOCK(sw);

    /**************************************************
     * See if we have any pending interrupt conditions.
     **************************************************/

    /* read current global interrupt state */
    status = switchPtr->ReadUINT64( sw, 
                                    FM10000_GLOBAL_INTERRUPT_DETECT(0), 
                                    &global );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                 "global=0x%" FM_FORMAT_64 "x send=%d\n",
                 global,
                 intrSendPackets);

    /**************************************************
     * Check if the TX queue is full. If so, force a 
     * transmission to check if we can now transmit. 
     **************************************************/

    if (switchPtr->transmitterLock)
    {
        intrSendPackets = TRUE;
    }

    /**************************************************
     * If no flags and no interrupt conditions
     * pending, then we are done.
     **************************************************/

    if (!global && !intrSendPackets)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR,
                     "No flags or interrupt conditions, ending pass.\n");

        return FM_OK;
    }

    /**************************************************
     * Determine all sources that are currently
     * interrupting and mask further interrupts by
     * that source.
     **************************************************/

    /* We need to take the register lock since we do
     * read-modify-write on various registers. */
    FM_FLAG_TAKE_REG_LOCK(sw);


    /**************************************************
     * Handle CORE interrupts.
     **************************************************/
    if ( global & FM10000_INT_CORE )
    {
        status = fm10000CoreInterruptHandler(switchPtr);
        FM_ERR_COMBINE(retStatus, status);
    }


    /**************************************************
     * Save PCIE interrupt information.
     **************************************************/
    switchExt = GET_SWITCH_EXT(sw);

    status = switchPtr->ReadUINT32(sw, 
                                   FM10000_DEVICE_CFG(),
                                   &devCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

    pcieIntMask = FM10000_INT_PCIe_0;
    for ( i = 0 ; i < FM10000_NUM_PEPS ; i++ )
    {
        /* Don't mask out the DeviceStateChange Interrupt. This is to protect
         * the following corner case:
         *
         * 1. FM10000_PCIE_IP.DeviceStateChange==1 Interrupt
         * 2. FM10000_PCIE_IM.DeviceStateChange==1
         * 3. WARM reset occurs (PCIE_{IM,IP} are no longer accessible)
         * 4. FM10000_PCIE_IM.DeviceStateChange==0  <== FAILS
         * 5. API can never get DeviceStateChange interrupts ever because
         *    the interrupt is masked out.
         *
         * To workaround this corner case, we never modify
         * FM10000_PCIE_IM.DeviceStateChange, instead we use
         * INTERRUPT_MASK_XXX.PCIE[pep] to disable interrupts while processing
         * them. */

        if ( (devCfg & (1 << (FM10000_DEVICE_CFG_b_PCIeEnable_0 + i) ) ) == 0 )
        {
            /* PEP is disabled, skip it to prevent deadlock */
            pcieIntMask <<= 1;
            continue;
        }

        if ( global & (pcieIntMask) )
        {
            status = switchPtr->ReadUINT32(sw, 
                                           FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), 
                                           i),
                                           &ipTmp32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

            status = switchPtr->ReadUINT32(sw, 
                                           FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                                           i),
                                           &imTmp32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

            currentIntr.pcie[i] |= ipTmp32 & ~imTmp32;

            if ( currentIntr.pcie[i] == 0 )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, 
                               "PEP[%d] Interrupt Bit Stuck\n", 
                               i);
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, 
                               "    GLOBAL_INTERRUPT_DETECT = 0x%016llx\n", 
                               global);
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, 
                               "    INTERRUPT_MASK_XXX      = 0x%016llx\n", 
                               switchExt->interruptMaskValue);
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, 
                               "    PCIE_{IP/IM}[%d]         = 0x%08x/0x%08x\n", 
                               i, 
                               ipTmp32, 
                               imTmp32);

                /* Apply Recovery if possible */
                status = switchPtr->ReadUINT32(sw, 
                                               FM10000_CHIP_VERSION(),
                                               &rv);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                if ( rv == FM10000_CHIP_VERSION_A0 )
                {
                    /* Make sure the PEP is in reset */
                    status = fm10000GetPepResetState(sw, i, &curPepState);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                    if ( !curPepState )
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "    Applying A0 recovery\n");

                        /* Recovery for A0 is done the following way.
                         *  
                         * 1- Take the PEP out of reset
                         * 2- API masks the DeviceStateChange interrupt
                         * 3- API starts a timer for each PEP to poll their
                         *    respective states
                         * 4- PEP is put in reset again
                         * 5- While polling, if a PEP comes out of reset,
                         *    its respective timer callback will unmask the
                         *    DeviceStateChange interrupt and stop the timer
                         *    for this PEP.
                         *
                         * After the last step, no interrupt should be
                         * detected until the PEP is taken out
                         * of reset from a clean DeviceStateChange interrupt */

                        /* Take the SOFT_RESET lock */
                        status = fm10000TakeSoftResetLock(sw);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        /* Take the PEP out of reset */
                        status = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &softReset);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        /* Force PEP into active state so BSM can update PCIE_IP
                         * if in recovery mode (from API PoV) */
                        FM_SET_UNNAMED_FIELD(softReset,
                                             FM10000_SOFT_RESET_b_PCIeActive_0 + i,
                                             1,
                                             1);

                        status = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), softReset);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        status = switchPtr->ReadUINT32(sw, 
                                           FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                                           i),
                                           &imTmp32);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        /* Mask the PCIE_IM.DeviceStateChange interrupt */
                        FM_SET_BIT(imTmp32, FM10000_PCIE_IP, DeviceStateChange, 1);
                        status = switchPtr->WriteUINT32(sw, 
                                                        FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                                                        i), 
                                                        imTmp32);

                        /* Restore the PEP back to its original state */
                        FM_SET_UNNAMED_FIELD(softReset,
                                 FM10000_SOFT_RESET_b_PCIeActive_0 + i,
                                 1,
                                 0);

                        status = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), softReset);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        /* Drop the SOFT_RESET lock */
                        status = fm10000DropSoftResetLock(sw);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

                        /* Flag that a link down event is needed when
                           pep event handler will be called */
                        sendLinkDownEvent = TRUE;
                    }
                }
                else
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "    Applying B0 recovery\n");

                    /* clear the GLOBAL_INTERRUPT_DETECT bit for this PEP */
                    rv = (1 << ( FM10000_GLOBAL_INTERRUPT_DETECT_b_PCIE_0 + i) );
                    status = switchPtr->WriteUINT32(sw, 
                                                    FM10000_GLOBAL_INTERRUPT_DETECT(0),
                                                    rv);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
                }

                pcieIntMask <<= 1;
                continue;
            }

            if (ipTmp32 & FM10000_PCIE_INT_MASK)
            {
                FM_SET_UNNAMED_FIELD64(switchExt->interruptMaskValue,
                                       FM10000_INTERRUPT_MASK_INT_b_PCIE_0 + i,
                                       1,
                                       1);
                status = switchPtr->WriteUINT64(sw,
                                                switchExt->interruptMaskReg,
                                                switchExt->interruptMaskValue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
            }

            status = switchPtr->WriteUINT32(sw, 
                                           FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                                           i),
                                           (imTmp32 | ipTmp32) & ~FM10000_PCIE_INT_MASK);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

            status = switchPtr->WriteUINT32(sw, 
                                           FM10000_PCIE_PF_ADDR(FM10000_PCIE_IP(), 
                                           i),
                                           ipTmp32 & ~imTmp32);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
        }

        pcieIntMask <<= 1;

    }   /* for ( i = 0; i < FM10000_NUM_PEPS; i++ ) */


    /**************************************************
     * Save EPL interrupt information.
     **************************************************/
    eplIntMask = FM10000_INT_EPL_0;
    for ( i = 0 ; i < FM10000_NUM_EPLS ; i++ )
    {
        /* any interrupts on this EPL? */
        if ( ( global & eplIntMask ) != 0 )
        {
            /* Yes, save the interrupt sources, mask and clear the interrupt */
            status = switchPtr->ReadUINT32( sw, FM10000_EPL_IP(i), &eplIp );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

            FM_LOG_DEBUG( FM_LOG_CAT_EVENT_INTR, 
                          "EPL_IP(%d)=0x%08x\n",
                          i,
                          eplIp );

            /* save per-lane interrupt information */
            anIntMask     = (1U << FM10000_EPL_IP_l_AnPortInterrupt);
            linkIntMask   = (1U << FM10000_EPL_IP_l_LinkPortInterrupt);
            serDesIntMask = (1U << FM10000_EPL_IP_l_SerdesInterrupt);

            for ( j = 0 ; j < FM10000_PORTS_PER_EPL ; j++ )
            {
                /* Save any AN interrupt sources, mask+clear the interrupt */
                DETECT_AND_MASK_INT32(eplIp,
                                      anIntMask,
                                      FM10000_AN_IP(i, j),
                                      FM10000_AN_IM(i, j),
                                      switchExt->anImProp,
                                      &currentIntr.epl[i].an[j]);

                FM_LOG_DEBUG( FM_LOG_CAT_EVENT_INTR, 
                              "AN_IP(%d,%d)=0x%08x\n",
                              i,
                              j,
                              currentIntr.epl[i].an[j] );


                /* Save any link interrupt sources, mask+clear the interrupt */
                DETECT_AND_MASK_INT32(eplIp,
                                      linkIntMask,
                                      FM10000_LINK_IP(i, j),
                                      FM10000_LINK_IM(i, j),
                                      switchExt->linkImProp,
                                      &currentIntr.epl[i].link[j]);

                FM_LOG_DEBUG( FM_LOG_CAT_EVENT_INTR, 
                              "LINK_IP(%d,%d)=0x%08x\n",
                              i,
                              j,
                              currentIntr.epl[i].link[j] );


                /* Save any SerDes interrupt sources, mask+clear the interrupt */
                DETECT_AND_MASK_INT32(eplIp,
                                      serDesIntMask,
                                      FM10000_SERDES_IP(i, j),
                                      FM10000_SERDES_IM(i, j),
                                      switchExt->serdesImProp,
                                      &currentIntr.epl[i].serdes[j] );

                FM_LOG_DEBUG( FM_LOG_CAT_EVENT_INTR, 
                              "SERDES_IP(%d,%d)=0x%08x\n",
                              i,
                              j,
                              currentIntr.epl[i].serdes[j] );
#if 0
                /* Check for SerDes parity error notifications. */
                if ( currentIntr.epl[i].serdes[j] &
                     FM10000_INT_SERDES_PARITY_ERR )
                {
                    /* handle SerDes parity errors */
                }
#endif

                /* Next lane */
                anIntMask     <<= 1;
                linkIntMask   <<= 1;
                serDesIntMask <<= 1;

            }   /* end for (j = 0 ; j < FM10000_PORTS_PER_EPL ; j++) */

#if 0
            /* Save any EPL parity error interrupt sources. */
            DETECT_AND_MASK_INT32(eplIp,
                                  FM10000_INT_EPL_ErrorInterrupt, 
                                  FM10000_EPL_ERROR_IP(i),
                                  FM10000_EPL_ERROR_IM(0),
                                  0,
                                  &currentIntr.epl[i].epl_err);

            /* Check for EPL parity error notifications. */
            if ( currentIntr.epl[i].epl_err )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, 
                             "EPL_ERROR_IP(%d)=0x%08x\n",
                             i,
                             currentIntr.epl[i].epl_err);

                /* handle EPL parity errors */
            }
#endif

        }   /* end if ( ( global & eplIntMask ) != 0 ) */

        /* next EPL */
        eplIntMask <<= 1;

    }   /* end for ( i = 0; i < FM10000_NUM_EPLS; i++ ) */


    /**************************************************
     * Save TE interrupt information.
     **************************************************/
    if ( global & FM10000_INT_TUNNEL_0 )
    {
        status = fm10000TEInterruptHandler(switchPtr, 0, NULL);
        FM_ERR_COMBINE(status, retStatus);
    }

    if ( global & FM10000_INT_TUNNEL_1 )
    {
        status = fm10000TEInterruptHandler(switchPtr, 1, NULL);
        FM_ERR_COMBINE(status, retStatus);
    }


    /**************************************************
     * Handle FH_HEAD interrupts.
     **************************************************/
    if ( global & FM10000_INT_FH_HEAD )
    {
        status = fm10000FHHeadInterruptHandler(switchPtr);
        FM_ERR_COMBINE(status, retStatus);
    }


    /**************************************************
     * Handle FH_TAIL interrupts.
     **************************************************/
    if ( global & FM10000_INT_FH_TAIL )
    {
        status = fm10000FHTailInterruptHandler(switchPtr, &fh_tail);
        FM_ERR_COMBINE(status, retStatus);

        DETECT_AND_MASK32_V2(fh_tail,
                             FM10000_INT_FH_TAIL_TCN,
                             FM10000_MA_TCN_IP(),
                             FM10000_MA_TCN_IM(),
                             switchExt->maTcnImProp,
                             &currentIntr.ma_tcn);

    }   /* end if ( global & FM10000_INT_FH_TAIL ) */


    /**************************************************
     * Handle CRM interrupts.
     **************************************************/
    if ( global & FM10000_INT_CRM )
    {
        status = fm10000CrmInterruptHandler(switchPtr);
        FM_ERR_COMBINE(status, retStatus);
    }

    /**************************************************
     * Handle SW interrupts.
     **************************************************/
    if ( global & FM10000_INT_SOFTWARE )
    {
        status = switchPtr->ReadUINT32( sw, FM10000_SW_IP(), &ipTmp32 );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);

        swIp = ipTmp32 & ~switchExt->swImProp;

        /* For now just log the interrupts and clear them */
        FM_LOG_DEBUG( FM_LOG_CAT_EVENT_INTR, "swIp=0x%08x\n", swIp );

        status = switchPtr->WriteUINT32( sw, FM10000_SW_IP(), ipTmp32 & switchExt->swImProp );
    }

    /**************************************************
     * Handle GPIO interrupt.
     **************************************************/
    if ( global & FM10000_INT_GPIO )
    {
        status = fmPlatformGpioInterruptHandler(switchPtr);
        FM_ERR_COMBINE(status, retStatus);
    }

    /**************************************************
     * Finished saving interrupt information.
     **************************************************/
    FM_FLAG_DROP_REG_LOCK(sw);

    
    /**************************************************
     * Decode detected parity errors.
     **************************************************/
    status = fm10000ParityErrorDecoder(switchPtr);
    FM_ERR_COMBINE(status, retStatus);

    /**************************************************
     * Handle EPL interrupts
     **************************************************/
    for ( i = 0 ; i < FM10000_NUM_EPLS ; i++ )
    {

        for ( j = 0 ; j < FM10000_PORTS_PER_EPL ; j++ )
        {

            if ( ( currentIntr.epl[i].an[j] & FM10000_AN_INT_MASK ) != 0 )
            {
                status = fm10000AnEventHandler( sw,
                                                i,
                                                j,
                                                currentIntr.epl[i].an[j] );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
            }

            /* Can we use fm10000_port->linkInterruptMask as mask? */
            if ( ( currentIntr.epl[i].link[j] & 
                 ( FM10000_LINK_INT_MASK | FM10000_TIMESTAMP_INT_MASK ) ) != 0 )
            {
                status = fm10000LinkEventHandler( sw,
                                                  i,
                                                  j,
                                                  currentIntr.epl[i].link[j] );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
            }

            if ( ( currentIntr.epl[i].serdes[j] & FM10000_SERDES_INT_MASK ) != 0 )
            {
                status = fm10000SerDesEventHandler( sw,
                                                    i,
                                                    j,
                                                    currentIntr.epl[i].serdes[j] );
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
            }

        }   /* end for ( j = 0 ; j < FM10000_PORTS_PER_EPL ; j++ ) */

    }   /* end for ( i = 0 ; i < FM10000_NUM_EPLS ; i++ ) */
    

    /***************************************************
     * Handle TCN FIFO interrupts.
     **************************************************/
    if (currentIntr.ma_tcn)
    {
        fm10000TCNInterruptHandler(sw, currentIntr.ma_tcn);
    }

    /**************************************************
     * If a transmit packet is ready, process it.
     **************************************************/
    if ( intrSendPackets )
    {
        fmPlatformSendPackets(sw);
    }

    /**************************************************
     * Handle PCIE_IP interrupts.
     **************************************************/
    for (i = 0 ; i < FM10000_NUM_PEPS ; i++)
    {
        /* PEP-related interrupts */
        if ( currentIntr.pcie[i] & FM10000_PCIE_INT_MASK )
        {
            status = fm10000PepEventHandler(sw, i, currentIntr.pcie[i], &sendLinkDownEvent);
            FM_LOG_CONTINUE_ON_ERR(FM_LOG_CAT_EVENT_INTR, status, retStatus);
        }

        /* Mailbox interrupts */
        if ( currentIntr.pcie[i] & FM10000_INT_PCIE_IP_MAILBOX )
        {
            status = fm10000PCIeMailboxInterruptHandler(sw, i);

            /* if switch is not up, do not handle next interrupts. */
            if (status == FM_ERR_SWITCH_NOT_UP)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_INTR, status);
            }

            FM_LOG_CONTINUE_ON_ERR(FM_LOG_CAT_EVENT_INTR, status, retStatus);
        }
    }   /* end for (i = 0 ; i < FM10000_NUM_PEPS ; i++) */

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_INTR, FM_OK);

ABORT:
    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Sw %d: Completing pass\n", sw);

    if ( regLockTaken )
    {
        DROP_REG_LOCK( sw );
    }

    FM_ERR_COMBINE(retStatus, status);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_INTR, retStatus);

}   /* end fm10000InterruptHandler */
