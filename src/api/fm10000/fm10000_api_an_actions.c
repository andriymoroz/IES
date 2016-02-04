/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_an_actions.c
 * Creation Date:   April 09, 2014
 * Description:     Action callbacks for the FM10000 AN state machine
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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
 * Local function prototypes
 *****************************************************************************/

static fm_status SendPortAnEventInd( fm_smEventInfo *eventInfo, 
                                     void           *userInfo, 
                                     fm_int          eventId );
static fm_status ConfigureAn37Timers( fm_smEventInfo *eventInfo, 
                                      void           *userInfo );
static fm_status ConfigureAn73Timers( fm_smEventInfo *eventInfo, 
                                      void           *userInfo );
static fm_int An73AbilityToHCD(fm_uint ability);

static fm_status SendApiAutoNegEvent( fm_int            sw,
                                      fm_int            logPort,
                                      fm_portLinkStatus status,
                                      fm_uint64         msg );
static fm_status ConfigureAn73BasePage( fm_smEventInfo *eventInfo, 
                                        void           *userInfo );
static fm_status ConfigureAn37BasePage( fm_smEventInfo *eventInfo, 
                                        void           *userInfo );
static fm_status ConfigureSgmiiBasePage( fm_smEventInfo *eventInfo, 
                                            void           *userInfo );
static fm_status ConfigureAn73NextPages( fm_smEventInfo *eventInfo, 
                                         void           *userInfo );
static fm_status ConfigureAn37NextPages( fm_smEventInfo *eventInfo, 
                                         void           *userInfo );
static void HandleAnPollingTimerEvent( void *arg );



/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** SendPortAnEventInd
 * \ingroup intPort
 *
 * \desc            Send the port state machine an auto-negotiation related
 *                  event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[in]       eventId is the ID of the event to be notified
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status SendPortAnEventInd( fm_smEventInfo *eventInfo, 
                                     void           *userInfo, 
                                     fm_int          eventId )
{
    fm_smEventInfo   portEventInfo;
    fm10000_port    *portExt;
    fm_switch       *switchPtr;

    portExt   = ( (fm10000_portSmEventInfo *)userInfo )->portExt;
    switchPtr = ( (fm10000_portSmEventInfo *)userInfo )->switchPtr;

    portEventInfo.smType = portExt->smType;
    portEventInfo.srcSmType = portExt->anSmType;
    portEventInfo.eventId = eventId;
    portEventInfo.lock    = FM_GET_STATE_LOCK( switchPtr->switchNumber );
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo, 
                                      userInfo,
                                      &portExt->base->portNumber );

}   /* end SendPortAnEventInd */


/*****************************************************************************/
/** ConfigureAn37Timers
 * \ingroup intPort
 *
 * \desc            Action callback that configures Clause 37 timers in
 *                  the context of a state machine event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn37Timers( fm_smEventInfo *eventInfo, 
                                      void           *userInfo )
{
    fm_status     status;
    fm_uint       linkTimeoutUsec = 0;
    fm_uint       timeScale;
    fm_uint       linkTimeout;
    fm_portAttr  *portAttr;
    fm10000_port *portExt;
    fm_switch    *switchPtr;
    fm_uint32     timerCfg;
    fm_int        epl;
    fm_int        physLane;
    fm_int        anMode;

    portAttr   = ( ( fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt    = ( ( fm10000_portSmEventInfo *)userInfo)->portExt;
    switchPtr  = ( ( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    anMode     = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegMode;

    if ( anMode == FM_PORT_AUTONEG_SGMII )
    {
        linkTimeoutUsec = GET_FM10000_PROPERTY()->autonegSgmiiTimeout;
    }
    else if ( anMode == FM_PORT_AUTONEG_CLAUSE_37 )
    {
        linkTimeoutUsec = GET_FM10000_PROPERTY()->autonegCl37Timeout;
    }

    fm10000AnGetTimeScale( linkTimeoutUsec, 
                           FM10000_AN37_LINK_TIMER_TIMEOUT_MAX,
                           &timeScale,
                           &linkTimeout );

    if( linkTimeout >= FM10000_AN37_LINK_TIMER_TIMEOUT_MAX || 
        linkTimeout <= 1 )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         portExt->base->portNumber,
                         "Invalid AN_37_TIMER_CFG configuration: "
                         "linkTimeoutUsec=%d timeScale=%d "
                         "LinkTimerTimeout=%d\n",
                         linkTimeoutUsec, 
                         timeScale, 
                         linkTimeout );

        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                portExt->base->portNumber,
                                status );
    }

    timerCfg = 0;
    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_37_TIMER_CFG, 
                  TimeScale, 
                  timeScale );
    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_37_TIMER_CFG, 
                  LinkTimerTimeout, 
                  linkTimeout );

    epl = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    status = switchPtr->WriteUINT32( switchPtr->switchNumber, 
                                     FM10000_AN_37_TIMER_CFG(epl, physLane),
                                     timerCfg );

ABORT:
    return status;

}   /* end ConfigureAn37Timers */



/*****************************************************************************/
/** ConfigureAn73Timers
 * \ingroup intPort
 *
 * \desc            Action callback that configures Clause 73 timers in
 *                  the context of a state machine event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn73Timers( fm_smEventInfo *eventInfo, 
                                      void           *userInfo )
{
    fm_int             hcd;
    fm_int             sw;
    fm_int             port;
    fm_int             epl;
    fm_int             physLane;
    fm10000_portAttr  *portAttrExt;
    fm10000_port      *portExt;
    fm_uint            linkTimeout;
    fm_status          status;

    portExt  = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    sw       = (( fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port     = portExt->base->portNumber;
    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;
    portAttrExt = (( fm10000_portSmEventInfo *)userInfo)->portAttrExt;

    /* assume success */
    status = FM_OK;

    /* the value to be configured is different depending on the event */
    if ( eventInfo->eventId == FM10000_AN_EVENT_GOOD_CHECK_IND )
    {
        /* Link Inhibit Timer */
        hcd      = ( ( fm10000_portSmEventInfo *)userInfo )->hcd;

        switch (hcd)
        {
            case AN73_HCD_KX:
            case AN73_HCD_KX4:
                linkTimeout = portAttrExt->autoNegLinkInhbTimerKx;
                break;

            default:
                linkTimeout = portAttrExt->autoNegLinkInhbTimer;
                break;
        }

        status = fm10000An73UpdateLinkInhibitTimer( sw, 
                                                    port,
                                                    epl, 
                                                    physLane,
                                                    linkTimeout );
        FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT_AUTONEG,port,status);
    }

    if ( eventInfo->eventId == FM10000_AN_EVENT_START_REQ ||
         eventInfo->eventId == FM10000_AN_EVENT_TRANSMIT_DISABLE_IND )
    {
        /* Link break timer */
        status =  fm10000An73UpdateBreakLinkTimer( sw, 
                                                   port,
                                                   epl, 
                                                   physLane,
                                                   BREAK_LINK_TIMER_MILLISEC );
        FM_LOG_EXIT_ON_ERR_V2(FM_LOG_CAT_PORT_AUTONEG,port,status);
    }

    return status;

}   /* end ConfigureAn73Timers */








/*****************************************************************************/
/** An73AbilityToHCD
 * \ingroup intPort
 *
 * \desc            Return HCD for the given technology ability.
 *
 * \param[in]       ability is the AN Clause 73 technology ability.
 *
 * \return          AN73_HCD_XXX.
 *
 *****************************************************************************/
static fm_int An73AbilityToHCD(fm_uint ability)
{

    if (ability & FM10000_AN73_ABILITY_100GBASE_CR4 )
    {
        return AN73_HCD_100_CR4;
    }
    else if ( ability & FM10000_AN73_ABILITY_100GBASE_KR4)
    {
        return AN73_HCD_100_KR4;
    }
    else if (ability & FM10000_AN73_ABILITY_40GBASE_CR4)
    {
        return AN73_HCD_40_CR4;
    }
    else if (ability & FM10000_AN73_ABILITY_40GBASE_KR4)
    {
        return AN73_HCD_40_KR4;
    }
    if (ability & FM10000_AN73_ABILITY_25GBASE_CR )
    {
        return AN73_HCD_25_CR;
    }
    if (ability & FM10000_AN73_ABILITY_25GBASE_KR )
    {
        return AN73_HCD_25_KR;
    }
    else if (ability & FM10000_AN73_ABILITY_10GBASE_KR)
    {
        return AN73_HCD_10_KR;
    }
    else if (ability & FM10000_AN73_ABILITY_1000BASE_KX)
    {
        return AN73_HCD_KX;
    }

    return AN73_HCD_INCOMPATIBLE_LINK;

}   /* end An73AbilityToHCD */



/*****************************************************************************/
/** SendApiAutoNegEvent
 * \ingroup intPort
 *
 * \desc            Generates an auto-neg event when one occurs.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       logPort is the logical port number of the port that the
 *                  event occurred on.
 *
 * \param[in]       status is the type of auto-neg event.
 *
 * \param[in]       msg is the autoneg codeword to send as auxiliary
 *                  information.
 *
 * \return          None
 *
 *****************************************************************************/
static fm_status SendApiAutoNegEvent( fm_int            sw,
                                      fm_int            logPort,
                                      fm_portLinkStatus status,
                                      fm_uint64         msg )
{
    fm_status     err;
    fm_event *    event;
    fm_eventPort *portEvent;

    FM_LOG_ENTRY_V2 (FM_LOG_CAT_PORT_AUTONEG,
                     logPort,
                     "sw=%d logPort=%d linkStatus=%d msg=0x%llx\n",
                     sw, 
                     logPort, 
                     status, 
                     msg );

    if (!GET_FM10000_PROPERTY()->autonegGenerateEvents)
    {
        /* exit if not allowed */
        FM_LOG_EXIT_V2(FM_LOG_CAT_PORT_AUTONEG, logPort, FM_OK)
    }

    event = fmAllocateEvent(sw,
                            FM_EVID_HIGH_PORT,
                            FM_EVENT_PORT,
                            FM_EVENT_PRIORITY_LOW);

    /* Allocate event can be blocked for low priority or return NULL.
     * Handle NULL properly
     */
    if (event == NULL)
    {
        FM_LOG_EXIT_V2( FM_LOG_CAT_PORT_AUTONEG, 
                        logPort,
                        FM_ERR_NO_EVENTS_AVAILABLE );
    }

    portEvent = &event->info.fpPortEvent;
    FM_CLEAR(*portEvent);

    portEvent->port        = logPort;
    portEvent->mac         = 0,
    portEvent->lane        = FM_PORT_LANE_NA;
    portEvent->activeMac   = TRUE;
    portEvent->linkStatus  = status;
    portEvent->autonegCode = msg;

    err = fmSendThreadEvent(&fmRootApi->eventThread, event);

    if (err != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(event);

        FM_LOG_EXIT_V2( FM_LOG_CAT_PORT_AUTONEG, logPort, err);
    }

    FM_LOG_EXIT_V2( FM_LOG_CAT_PORT_AUTONEG, logPort, FM_OK);

}   /* end SendApiAutoNegEvent */



/*****************************************************************************/
/** ConfigureAn73BasePage
 * \ingroup intPort
 *
 * \desc            Configure Clause 73 base page when the auto-negotiation 
 *                  mode is configured or when auto-negotiation is restarted     
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn73BasePage( fm_smEventInfo *eventInfo, 
                                        void           *userInfo )
{
    fm_status         status;
    fm_switch        *switchPtr;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm10000_port     *portExt;
    fm_uint64         txMsg;
    fm_uint           ability;
    fm_int            physLane;
    fm_int            port;
    fm_int            sw;
    fm_int            epl;
    fm_bool           nextPageEnabled;
    fm_bool           is40GCapable;
    fm_bool           is100GCapable;

    switchPtr   = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr    = (( fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = (( fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    sw        = switchPtr->switchNumber;
    physLane  = portExt->nativeLaneExt->physLane;
    port      = portExt->base->portNumber;
    epl       = portExt->endpoint.epl;

    if ( eventInfo->eventId == FM10000_AN_EVENT_START_REQ )
    {
        txMsg = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegBasePage;
    }
    else
    {
        txMsg = portAttr->autoNegBasePage;
    }

    /* Use default if page is not set */
    if ( !txMsg )
    {
        /* Selector field */
        FM_SET_FIELD64(txMsg, FM10000_AN_73_BASE_PAGE_TX, S, 1);

        /* Echo Nonce */
        FM_SET_FIELD64(txMsg, FM10000_AN_73_BASE_PAGE_TX, E, 0);

        /* Pause Ability */
        FM_SET_FIELD64(txMsg, FM10000_AN_73_BASE_PAGE_TX, C, 0);
        FM_SET_BIT64(txMsg, FM10000_AN_73_BASE_PAGE_TX, RF, 0);
        FM_SET_BIT64(txMsg, FM10000_AN_73_BASE_PAGE_TX, ACK, 1);
        FM_SET_BIT64(txMsg, FM10000_AN_73_BASE_PAGE_TX, NP, 0);

        /* Transmitted Nonce */
        FM_SET_FIELD64(txMsg, FM10000_AN_73_BASE_PAGE_TX, T, 0);

        /* Technology Ability */
        ability = FM10000_AN73_SUPPORTED_ABILITIES;

        status = fm10000GetMultiLaneCapabilities(sw,
                                                 port,
                                                 &is40GCapable,
                                                 &is100GCapable);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

        if (!is40GCapable)
        {
            ability &= ~FM10000_AN73_ABILITIES_40G;
        }

        if (!is100GCapable)
        {
            ability &= ~FM10000_AN73_ABILITIES_100G;
        }

        FM_SET_FIELD64( txMsg, FM10000_AN_73_BASE_PAGE_TX, A, ability );

        /* FEC Capability */
        FM_SET_FIELD64(txMsg, FM10000_AN_73_BASE_PAGE_TX, F, 0);
    }

    /* Advise Energy-Efficient Ethernet (EEE) if enabled */
    if ( portAttrExt->eeeEnable )
    {
        /* EEE is advertized in the NextPage */
        FM_SET_BIT64(txMsg, FM10000_AN_73_BASE_PAGE_TX, NP, 1);

        if ( !portAttrExt->eeeNextPageAdded )
        {
            /* Add an EEE NextPage */
            status = fm10000AnAddNextPage(sw, port, FM10000_AN_73_NEXTPAGE_EEE);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );
            portAttrExt->eeeNextPageAdded = TRUE;
            FM_SET_BIT64(portAttr->autoNegBasePage, 
                         FM10000_AN_73_BASE_PAGE_TX, NP, 1);
        }
    }

    nextPageEnabled = FM_GET_BIT64( txMsg, FM10000_AN_73_BASE_PAGE_TX, NP );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port,
                     "Enable auto-negotiation: clause 73.%s"
                     " basePage=0x%016llx\n",
                     nextPageEnabled ? " Enable nextPages" : "",
                     txMsg);

    status = 
        switchPtr->WriteUINT64( sw,
                                FM10000_AN_73_BASE_PAGE_TX(epl, physLane, 0),
                                txMsg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    portExt->basePage = txMsg;

ABORT:
    return status;

}   /* end ConfigureAn73BasePage */


/*****************************************************************************/
/** ConfigureAn37BasePage
 * \ingroup intPort
 *
 * \desc            Configure Clause 37 base page when the auto-negotiation
 *                  mode is configured or when auto-negotiation is restarted     
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn37BasePage( fm_smEventInfo *eventInfo, 
                                        void           *userInfo )
{
    fm_status         status;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm_uint64         txMsg;
    fm_int            physLane;
    fm_int            sw;
    fm_int            epl;
    fm_int            port;
    
    switchPtr   = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr    = (( fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = (( fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    physLane  = portExt->nativeLaneExt->physLane;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    port      = portExt->base->portNumber;

    if ( eventInfo->eventId == FM10000_AN_EVENT_START_REQ )
    {
        txMsg = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegBasePage;
    }
    else
    {
        txMsg = portAttr->autoNegBasePage;
    }

    if ( !txMsg )
    {
        /* Use default is page is not set */
        FM_SET_BIT(txMsg, FM10000_AN_37_BASE_PAGE_TX, ACK, 1);
        FM_SET_BIT(txMsg, FM10000_AN_37_BASE_PAGE_TX, Pause, 1);
        FM_SET_BIT(txMsg, FM10000_AN_37_BASE_PAGE_TX, FullDuplex, 1);
        FM_SET_BIT(txMsg, FM10000_AN_37_BASE_PAGE_TX, NP, 0);
    }

    /* Only 32-bit register but is an alias of 64-bit register */
    status = 
        switchPtr->WriteUINT64( sw,
                                FM10000_AN_37_BASE_PAGE_TX(epl, physLane, 0),
                                txMsg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    portExt->basePage = txMsg;
        
ABORT:
    return status;

}   /* end ConfigureAn37BasePage */


/*****************************************************************************/
/** ConfigureSgmiiBasePage
 * \ingroup intPort
 *
 * \desc            Configure SGMII base page when the auto-negotiation mode is
 *                  configured or when auto-negotiation is restarted     
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureSgmiiBasePage( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_status     status;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_portAttr  *portAttr;
    fm_uint64     txMsg;
    fm_int        physLane;
    fm_int        sw;
    fm_int        epl;
    fm_int        port;

    switchPtr = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr  = (( fm10000_portSmEventInfo *)userInfo)->portAttr;
    physLane  = portExt->nativeLaneExt->physLane;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    port      = portExt->base->portNumber;

    /* Check if we need defaults */
    if ( eventInfo->eventId == FM10000_AN_EVENT_START_REQ )
    {
        txMsg = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegBasePage;
    }
    else
    {
        txMsg = portAttr->autoNegBasePage;
    }

    if( !txMsg )
    {
        /***************************************************
         * From SGMII auto-negotiation standard:
         *
         * Bit  0     = 1 (By SGMII spec)
         * Bit  14    = ACK
         ***************************************************/
        FM_SET_BIT(txMsg, FM10000_SGMII_AN_TX_CONFIG, OneB14, 1);
        FM_SET_BIT(txMsg, FM10000_SGMII_AN_TX_CONFIG, B0, 1);
    }

    /* Only 32-bit register but is an alias of 64-bit register */
    status = 
        switchPtr->WriteUINT64( sw,
                                FM10000_SGMII_AN_TX_CONFIG(epl, physLane, 0),
                                txMsg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    portExt->basePage = txMsg;
        
ABORT:            
    return status;

}   /* end ConfigureSgmiiBasePage */



/*****************************************************************************/
/** ConfigureAn73NextPages
 * \ingroup intPort
 *
 * \desc            Configure Clause 73 next page when needed
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn73NextPages( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_status     status;
    fm_portAttr  *portAttr;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_uint64     txPage;
    fm_uint64     rxPage;
    fm_int        sw;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_bool       doNextPage;
    fm_int        pageNum;
    fm_uint32     anCfg;

    portExt   = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr  = (( fm10000_portSmEventInfo *)userInfo)->portAttr;
    switchPtr = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    port      = portExt->base->portNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    status = 
        switchPtr->ReadUINT64( sw,
                               FM10000_AN_73_BASE_PAGE_RX(epl, physLane, 0),
                               &rxPage );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT_AUTONEG, port, status);

    FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                      port,
                      "Sw#%d Port %d: An73CompleteAck "
                      "PageRx#%d=0x%llx\n",
                      sw,
                      port,
                      portAttr->autoNegPartnerNextPages.numPages, 
                      rxPage);

    if( portAttr->autoNegPartnerBasePage == 0)
    {
        /* store it for later access */
        portAttr->autoNegPartnerBasePage = rxPage;
        txPage     = portAttr->autoNegBasePage;
        doNextPage = ( FM_GET_BIT(txPage, FM10000_AN_73_BASE_PAGE_TX, NP) ||
                       FM_GET_BIT(rxPage, FM10000_AN_73_BASE_PAGE_RX, NP) );

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Port %d: An73CompleteAck RxBasePage=0x%llx - %s\n",
                         port, 
                         rxPage, 
                         doNextPage ? " Enable nextPage" : "");

    }
    else
    {
        pageNum    = portAttr->autoNegPartnerNextPages.numPages;
        doNextPage = ( FM_GET_BIT(rxPage, FM10000_AN_73_BASE_PAGE_RX, NP) ||
                       (pageNum+1) < portAttr->autoNegNextPages.numPages );

        FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                          port,
                          "Port %d: An73CompleteAck "
                          "NextPageRx#%d=0x%llx.%s\n",
                          port, 
                          pageNum, 
                          rxPage, 
                          doNextPage ? " Do nextPage" : "");

        if (!portAttr->autoNegPartnerNextPages.nextPages)
        {
            portAttr->autoNegPartnerNextPages.nextPages = 
                fmAlloc(sizeof(fm_uint64) * MAX_NUM_NEXTPAGES);
        }

        if (portAttr->autoNegPartnerNextPages.nextPages &&
            portAttr->autoNegPartnerNextPages.numPages < MAX_NUM_NEXTPAGES)
        {
            portAttr->autoNegPartnerNextPages.nextPages[pageNum] = rxPage;
            portAttr->autoNegPartnerNextPages.numPages++;
        }
    }

    /* do exchange next pages */
    if (doNextPage)
    {
        pageNum = portAttr->autoNegPartnerNextPages.numPages;

        /* Next pages to sent? */
        if ( pageNum < portAttr->autoNegNextPages.numPages )
        {
            txPage = portAttr->autoNegNextPages.nextPages[pageNum];

            /* Force NP even if the user is not setting it */
            FM_SET_BIT( txPage, 
                        FM10000_AN_73_NEXT_PAGE_TX, 
                        NP,
                       ( pageNum < (portAttr->autoNegNextPages.numPages - 1) ) );
        }
        else
        {
            txPage = 0;
            FM_SET_BIT(txPage, FM10000_AN_73_NEXT_PAGE_TX, MP, 1);
        }

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Port %d: An73CompleteAck: "
                         "NextPageTx#%d = 0x%llx\n",
                         port, 
                         pageNum, 
                         txPage );

        status = 
            switchPtr->WriteUINT64( sw, 
                                    FM10000_AN_73_NEXT_PAGE_TX(epl, physLane, 0), 
                                    txPage );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

        /* Toggle TxNextPageLoaded on DUT to indicate next page is loaded */
        status = 
            switchPtr->ReadUINT32( sw, 
                                   FM10000_AN_73_CFG(epl, physLane), 
                                   &anCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

        FM_SET_BIT(anCfg, FM10000_AN_73_CFG, TxNextPageLoaded, 0);
        status =
            switchPtr->WriteUINT32( sw, 
                                    FM10000_AN_73_CFG(epl, physLane), 
                                    anCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);

        FM_SET_BIT(anCfg, FM10000_AN_73_CFG, TxNextPageLoaded, 1);
        status =
            switchPtr->WriteUINT32( sw, 
                                    FM10000_AN_73_CFG(epl, physLane), 
                                    anCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status);


    }

ABORT:
    return status;

}   /* end ConfigureAn73NextPages */


/*****************************************************************************/
/** ConfigureAn37NextPages
 * \ingroup intPort
 *
 * \desc            Configure Clause 37 next page when the auto-negotiation
 *                  mode is configured or when auto-negotiation is restarted     
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
static fm_status ConfigureAn37NextPages( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_status     status;
    fm_portAttr  *portAttr;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_uint32     txPage;
    fm_uint32     rxPage;
    fm_int        sw;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_bool       doNextPage;
    fm_int        pageNum;
    fm_uint32     anCfg;

    portExt   = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr  = (( fm10000_portSmEventInfo *)userInfo)->portAttr;
    switchPtr = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    port      = portExt->base->portNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    status = 
        switchPtr->ReadUINT32( sw,
                               FM10000_AN_37_BASE_PAGE_RX(epl, physLane),
                               &rxPage );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                      port,
                      "Sw#%d Port %d: An37CompleteAck "
                      "PageRx#%d=0x%x\n",
                      sw,
                      port,
                      portAttr->autoNegPartnerNextPages.numPages, 
                      rxPage);

    if (portAttr->autoNegPartnerBasePage == 0)
    {
        /* store it for later access */
        portAttr->autoNegPartnerBasePage = rxPage;
        txPage = portAttr->autoNegBasePage;

        doNextPage = FALSE;
        if ( portAttr->autoNegMode == FM_PORT_AUTONEG_CLAUSE_37 )
        {
            doNextPage = (FM_GET_BIT(txPage, FM10000_AN_37_BASE_PAGE_TX, NP) &&
                          FM_GET_BIT(rxPage, FM10000_AN_37_BASE_PAGE_RX, NP) );
        }

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Port %d: An37CompleteAck RxBasePage=0x%x - %s\n",
                         port, 
                         rxPage, 
                         doNextPage ? " Enable nextPage" : "");

    }
    else
    {
        pageNum = portAttr->autoNegPartnerNextPages.numPages;

        doNextPage = ( FM_GET_BIT(rxPage, FM10000_AN_37_BASE_PAGE_RX, NP) ||
                       (pageNum+1) < portAttr->autoNegNextPages.numPages );

        FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG,
                          port,
                          "Port %d: An37CompleteAck "
                          "NextPageRx#%d=0x%x.%s\n",
                          port, 
                          pageNum, 
                          rxPage, 
                          doNextPage ? " Do nextPage" : "");

        if ( !portAttr->autoNegPartnerNextPages.nextPages )
        {
            portAttr->autoNegPartnerNextPages.nextPages = 
                fmAlloc(sizeof(fm_uint64) * MAX_NUM_NEXTPAGES);
        }

        if ( portAttr->autoNegPartnerNextPages.nextPages &&
            portAttr->autoNegPartnerNextPages.numPages < MAX_NUM_NEXTPAGES)
        {
            portAttr->autoNegPartnerNextPages.nextPages[pageNum] = rxPage;
            portAttr->autoNegPartnerNextPages.numPages++;
        }
    }

    /* do exchange next pages */
    if (doNextPage)
    {
        pageNum = portAttr->autoNegPartnerNextPages.numPages;

        if (pageNum < portAttr->autoNegNextPages.numPages )
        {
            txPage = portAttr->autoNegNextPages.nextPages[pageNum];

            txPage &= 0xFFFF;

            /* Force NP even if the user is not setting it */
            FM_SET_BIT( txPage, 
                        FM10000_AN_37_NEXT_PAGE_TX, 
                        NP,
                       ( pageNum < (portAttr->autoNegNextPages.numPages - 1) ) );
        }
        else
        {
            txPage = 0;
            FM_SET_BIT(txPage, FM10000_AN_37_NEXT_PAGE_TX, MP, 1);
        }

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                         port ,
                         "Port %d: An37CompleteAcknowledge: "
                         "NextPageTx#%d = 0x%x\n",
                         port, 
                         pageNum, 
                         txPage );

        status = 
            switchPtr->WriteUINT64( sw, 
                                    FM10000_AN_37_NEXT_PAGE_TX(epl, physLane, 0), 
                                    txPage );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

        /* Toggle ToggleNpLoaded on DUT to indicate next page is loaded */
        status = 
            switchPtr->ReadUINT32( sw, 
                                   FM10000_AN_37_CFG(epl, physLane), 
                                   &anCfg );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

        FM_SET_BIT(anCfg, FM10000_AN_37_CFG, ToggleNpLoaded, 0);
        status =
            switchPtr->WriteUINT32( sw, 
                                    FM10000_AN_37_CFG(epl, physLane), 
                                    anCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );


        FM_SET_BIT(anCfg, FM10000_AN_37_CFG, ToggleNpLoaded, 1);
        status =
            switchPtr->WriteUINT32( sw, 
                                    FM10000_AN_37_CFG(epl, physLane), 
                                    anCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    }

ABORT:
    return status;

}   /* end ConfigureAn37NextPages */



/*****************************************************************************/
/** HandleAnPollingTimerEvent
 * \ingroup intPort
 * 
 * \desc            Handles the expiration of the timer used for polling
 *                  during the AN_GOOD state
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleAnPollingTimerEvent( void *arg )
{
    fm_smEventInfo           eventInfo; 
    fm10000_portSmEventInfo *portEventInfo;
    fm10000_port            *portExt;
    fm_int                   sw;
    fm_int                   port;

    
    portExt             = arg;
    eventInfo.smType    = portExt->anSmType;
    eventInfo.srcSmType = 0;
    eventInfo.eventId   = FM10000_AN_EVENT_POLLING_TIMER_EXP_IND;
    portEventInfo       = &portExt->eventInfo;
    sw                  = portEventInfo->switchPtr->switchNumber;
    port                = portExt->base->portNumber;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                     port,
                     "AN polling timer expired on port %d (switch %d, portPtr=%p)\n",
                     port,
                     sw,
                     (void *)portExt->base );

    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    portExt->eventInfo.regLockTaken = FALSE;

    fmNotifyStateMachineEvent( portExt->anSmHandle,
                               &eventInfo,
                               portEventInfo,
                               &port );
    UNPROTECT_SWITCH( sw );

}   /* end HandleAnPollingTimerEvent */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000TakeRegLock
 * \ingroup intPort
 *
 * \desc            Action callback to take the register lock for the switch
 *                  a given port belongs to
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000TakeRegLock( fm_smEventInfo *eventInfo, void *userInfo )
{
    /* Scheduler lock is taken as well because it is required to get scheduler
     * token information to filter ethernet mode that can't be supported. */
    TAKE_SCHEDULER_LOCK( ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber );
    TAKE_REG_LOCK( ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber );
    ( ( fm10000_portSmEventInfo * )userInfo )->regLockTaken = TRUE;
    return FM_OK;

}   /* end fm10000TakeRegLock */


/*****************************************************************************/
/** fm10000DropRegLock
 * \ingroup intPort
 *
 * \desc            Action callback to take the register lock for the switch
 *                  a given port belongs to
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000DropRegLock( fm_smEventInfo *eventInfo, void *userInfo )
{
    DROP_REG_LOCK( ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber );
    ( ( fm10000_portSmEventInfo * )userInfo )->regLockTaken = FALSE;
    DROP_SCHEDULER_LOCK( ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber );
    return FM_OK;

}   /* end fm10000DropRegLock */




/*****************************************************************************/
/** fm10000An73UpdateLinkInhibitTimer
 * \ingroup intPort
 *
 * \desc            Update hardware link fail inhibit timer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the ID of the port on which to operate
 * 
 * \param[in]       epl is the epl port number.
 *
 * \param[in]       physLane is the corresponding epl physical lane ID
 *
 * \param[in]       timeout is the time value in milliseconds to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000An73UpdateLinkInhibitTimer( fm_int  sw,
                                             fm_int  port,
                                             fm_int  epl,
                                             fm_int  physLane,
                                             fm_uint timeout )
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_uint    timeScale;
    fm_uint    linkTimeout;
    fm_uint    breakTimeout;
    fm_uint32  timerCfg;

    switchPtr = GET_SWITCH_PTR(sw);

    timerCfg     = 0;

    /* practical timescales are
     *   (timeout <  512ms) --> 5
     *   (timeout >= 512ms) --> 6 */
    if ( timeout < 512 )
    {
        /* timeout < 512 ms */
        linkTimeout  = timeout;
        breakTimeout = BREAK_LINK_TIMER_MILLISEC;
        timeScale    = 5;
    }
    else
    {
        /* 512 <= timeout */
        linkTimeout  = timeout/10;
        breakTimeout = BREAK_LINK_TIMER_MILLISEC/10;
        timeScale    = 6;
    }

    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_73_TIMER_CFG, 
                  TimeScale, 
                  timeScale );
    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_73_TIMER_CFG, 
                  BreakLinkTimeout,
                  breakTimeout );
    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_73_TIMER_CFG, 
                  LinkFailInhibitTimeout, 
                  linkTimeout );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG, 
                     port,
                     "Setting timer cfg to 0x%08x\n",
                     timerCfg );
    status = switchPtr->WriteUINT32( sw, 
                                     FM10000_AN_73_TIMER_CFG(epl, physLane),
                                     timerCfg );
    return status;

}   /* end fm10000An73UpdateLinkInhibitTimer */




/*****************************************************************************/
/** fm10000An73UpdateBreakLinkTimer
 * \ingroup intPort
 *
 * \desc            Update hardware break link timer.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port to operate on.
 *
 * \param[in]       epl is the epl port number.
 *
 * \param[in]       physLane is the corresponding epl physLane number.
 *
 * \param[in]       timeout is the time value (in milliseconds) to configure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000An73UpdateBreakLinkTimer( fm_int  sw,
                                           fm_int  port,
                                           fm_int  epl,
                                           fm_int  physLane,
                                           fm_uint timeout )
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_uint    timeScale;
    fm_uint    breakTimeout;
    fm_uint    linkTimeout;
    fm_uint32  timerCfg;
    fm10000_portAttr *portAttrExt;
    fm_portAttr      *portAttr;
    fm_uint64  ability;
    fm_uint32  inhibitTimer;

    switchPtr = GET_SWITCH_PTR(sw);
    portAttr  = GET_PORT_ATTR(sw, port);
    portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

    timerCfg    = 0;

    portAttrExt = GET_FM10000_PORT_ATTR( sw, port );
    ability     = ( (portAttr->autoNegBasePage  >> 21) &  
                    (fm_uint64)(FM10000_AN73_SUPPORTED_ABILITIES) );

    if ( ((ability) & ~(fm_uint64)(FM10000_AN73_ABILITY_1000BASE_KX)) != 0)
    {
        inhibitTimer = portAttrExt->autoNegLinkInhbTimer;
    }
    else
    {
        inhibitTimer = portAttrExt->autoNegLinkInhbTimerKx;
    }

    if ( inhibitTimer < 512 )
    {
        /* timeout < 512 ms */
        linkTimeout  = inhibitTimer;
        breakTimeout = timeout;
        timeScale    = 5;
    }
    else
    {
        /* 512 <= timeout */
        linkTimeout  = inhibitTimer/10;
        breakTimeout = timeout/10;
        timeScale    = 6;
    }

    FM_SET_FIELD( timerCfg,
                  FM10000_AN_73_TIMER_CFG,
                  TimeScale,
                  timeScale );
    FM_SET_FIELD( timerCfg,
                  FM10000_AN_73_TIMER_CFG,
                  BreakLinkTimeout,
                  breakTimeout);
    FM_SET_FIELD( timerCfg, 
                  FM10000_AN_73_TIMER_CFG, 
                  LinkFailInhibitTimeout,
                  linkTimeout );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG, 
                     port,
                     "Setting timer cfg to 0x%08x\n",
                     timerCfg );

    status = switchPtr->WriteUINT32( sw, 
                                     FM10000_AN_73_TIMER_CFG(epl, physLane), 
                                     timerCfg );
    return status;

}   /* end fm10000An73UpdateBreakLinkTimer */




/*****************************************************************************/
/** fm10000EnableAn
 * \ingroup intPort
 *
 * \desc            Action enabling auto-negotiation in the context of a state
 *                  machine event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000EnableAn( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_uint32         addr;
    fm_uint32         anCfg;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_switch        *switchPtr;
    fm_portAttr      *portAttr;
    fm_int            sw;
    fm_int            epl;
    fm_int            physLane;
    fm_status         status;
    fm_int            port;
    fm_bool           nextPageEnabled;
    fm_int            anMode;
    fm_uint64         anBasePage;

    portAttr    = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;
    switchPtr   = ( ( fm10000_portSmEventInfo *)userInfo )->switchPtr;
    portExt     = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    portAttrExt = ( ( fm10000_portSmEventInfo *)userInfo )->portAttrExt;
    epl         = portExt->endpoint.epl;
    physLane    = portExt->nativeLaneExt->physLane;
    sw          = switchPtr->switchNumber;
    port        = portExt->base->portNumber;
    anMode = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegMode;
    anBasePage = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegBasePage;


    if ( portExt->anSmType == FM10000_CLAUSE37_AN_STATE_MACHINE )
    {
        addr = FM10000_AN_37_CFG( epl, physLane );
    }
    else 
    {
        addr = FM10000_AN_73_CFG( epl, physLane );
    }

    status = switchPtr->ReadUINT32( sw, addr, &anCfg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, port, status );

    if ( portExt->anSmType == FM10000_CLAUSE37_AN_STATE_MACHINE )
    {
        FM_SET_BIT( anCfg, FM10000_AN_37_CFG, MrAnEnable, 1 );
        if ( anMode == FM_PORT_AUTONEG_CLAUSE_37 )
        {
            nextPageEnabled = FM_GET_BIT( anBasePage,
                                          FM10000_AN_37_BASE_PAGE_TX, 
                                          NP );

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                             port,
                             "Enable auto-negotiation: clause 37.%s\n",
                             nextPageEnabled ? " Enable nextPages" : "");

            FM_SET_BIT( anCfg, FM10000_AN_37_CFG, NpEnable, nextPageEnabled );
        }
    }
    else 
    {
        FM_SET_BIT( anCfg, FM10000_AN_73_CFG, MrAutonegEnable, 1 );
        FM_SET_BIT( anCfg, 
                    FM10000_AN_73_CFG, 
                    IgnoreNonceMatch,
                    portAttrExt->autoNegIgnoreNonce );
    }

    status = switchPtr->WriteUINT32( sw, addr, anCfg );

ABORT:
    return status;

}   /* end fm10000EnableAn */


/*****************************************************************************/
/** fm10000DisableAn
 * \ingroup intPort
 *
 * \desc            Action disabling auto-negotiation in the context of a state
 *                  machine event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000DisableAn( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_uint32     addr;
    fm_uint32     anCfg;
    fm10000_port *portExt;
    fm_switch    *switchPtr;
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;

    switchPtr = ( ( fm10000_portSmEventInfo *)userInfo )->switchPtr;
    portExt   = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;
    sw        = switchPtr->switchNumber;

    anCfg = 0;

    if ( portExt->anSmType == FM10000_CLAUSE37_AN_STATE_MACHINE )
    {
        addr = FM10000_AN_37_CFG( epl, physLane );
    }
    else 
    {
        addr = FM10000_AN_73_CFG( epl, physLane );
    }

    return switchPtr->WriteUINT32( sw, addr, anCfg );

}   /* end fm10000DisableAn */



/*****************************************************************************/
/** fm10000EnableAnInterrupts
 * \ingroup intPort
 *
 * \desc            Enables all relevant autonegotiation-related interrupts in 
 *                  the context of a state machine event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000EnableAnInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_status     status;
    fm_bool       regLockTaken = FALSE;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    if ( portExt->eventInfo.regLockTaken == FALSE )
    {
        FM_FLAG_TAKE_REG_LOCK( sw );
    }

    /* unmask the required interrupts */
    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_AN_IP(epl, physLane),
                                    portExt->anInterruptMask,
                                    TRUE );
    if ( status == FM_OK )
    {
        status = switchPtr->MaskUINT32( sw, 
                                        FM10000_AN_IM(epl, physLane),
                                        portExt->anInterruptMask,
                                        FALSE );
    }


    if ( regLockTaken )
    {
        DROP_REG_LOCK( sw );
    }

    return status;

}   /* end fm10000EnableAnInterrupts */



/*****************************************************************************/
/** fm10000DisableAnInterrupts
 * \ingroup intPort
 *
 * \desc            Disables all relevant autonegotiation-related interrupts in 
 *                  the context of a state machine event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000DisableAnInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_status     status;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    /* Mask the required interrupts */
    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_AN_IM(epl, physLane),
                                    portExt->anInterruptMask,
                                    TRUE );

    return status;

}   /* end fm10000DisableAnInterrupts */



/*****************************************************************************/
/** fm10000ConfigureBasePage
 * \ingroup intPort
 *
 * \desc            Configure Clause 73 or Clause 37 base page when the auto-
 *                  negotiation mode is configured or when auto-negotiation is
 *                  restarted
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000ConfigureBasePage( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status     status;
    fm_portAttr  *portAttr;
    fm10000_port *portExt;
    fm_int        anMode;

    portExt  = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    portAttr = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;

    if ( eventInfo->eventId == FM10000_AN_EVENT_START_REQ )
    {
        anMode = ((fm10000_portSmEventInfo *)userInfo)->info.anConfig.autoNegMode;
    }
    else
    {
        anMode = portAttr->autoNegMode;
    }

    FM_LOG_DEBUG_V2(FM_LOG_CAT_PORT_AUTONEG,
                    portExt->base->portNumber,
                    "anMode=%d\n",
                    anMode );

    switch( anMode )
    {
        case FM_PORT_AUTONEG_CLAUSE_73:
            status = ConfigureAn73BasePage( eventInfo, userInfo );
            break;

        case FM_PORT_AUTONEG_CLAUSE_37:
            status = ConfigureAn37BasePage( eventInfo, userInfo );
            break;

        case FM_PORT_AUTONEG_SGMII:
            status = ConfigureSgmiiBasePage( eventInfo, userInfo );
            break;
            
        case FM_PORT_AUTONEG_NONE:
            status = FM_OK;
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            break;
    }

    /* Reset to indicate base page needs update */
    portAttr->autoNegPartnerBasePage           = 0;
    portAttr->autoNegPartnerNextPages.numPages = 0;
    portExt->anRestartCnt                      = 0;

    
    return status;

}   /* end fm10000ConfigureBasePage */



/*****************************************************************************/
/** fm10000ConfigureNextPages
 * \ingroup intPort
 *
 * \desc            Setup next pages if and when needed
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000ConfigureNextPages( fm_smEventInfo *eventInfo, 
                                     void           *userInfo )
{
    fm_status     status;
    fm_portAttr  *portAttr;
    fm10000_port *portExt;

    portExt  = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    portAttr = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;
    switch( portAttr->autoNegMode )
    {
        case FM_PORT_AUTONEG_CLAUSE_73:
            status = ConfigureAn73NextPages( eventInfo, userInfo );
            break;

        case FM_PORT_AUTONEG_SGMII:
        case FM_PORT_AUTONEG_CLAUSE_37:
            status = ConfigureAn37NextPages( eventInfo, userInfo );
            break;

        case FM_PORT_AUTONEG_NONE:
            status = FM_OK;
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            break;
    }

    return status;

}   /* end fm10000ConfigureNextPages */



/*****************************************************************************/
/** fm10000NotifyApiAutonegCompleteOrFault
 * \ingroup intPort
 *
 * \desc            Notify the API user that autonegotiation is complete or
 *                  that there is a remote fault
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyApiAutonegCompleteOrFault( fm_smEventInfo *eventInfo, 
                                                  void           *userInfo )
{
    fm_port           *portPtr;
    fm10000_port      *portExt;
    fm_portAttr       *portAttr;
    fm_uint64          rxPage;
    fm_int             port;
    fm_portLinkStatus  linkStatus;
    fm_status          status;
    fm_int             sw;

    portPtr  = ( ( fm10000_portSmEventInfo *)userInfo )->portPtr;
    portExt  = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    portAttr = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;
    sw     = ( ( fm10000_portSmEventInfo *)userInfo )->switchPtr->switchNumber;
    rxPage   = portAttr->autoNegPartnerBasePage;
    port     = portExt->base->portNumber;

    /* processing depends on the current autoneg mode */
    switch( portAttr->autoNegMode )
    {
        case FM_PORT_AUTONEG_CLAUSE_73:

            portExt->anRestartCnt = 0;
            if ( FM_GET_BIT(rxPage, FM10000_AN_73_BASE_PAGE_RX, RF) )
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Auto-negotiation Remote Fault received "
                                 "on port %d, AN_RX_MSG = 0x%llx\n",
                                 port, 
                                 rxPage );

                linkStatus = FM_PORT_STATUS_AUTONEG_REMOTE_FAULT;
            }
            else
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Auto-negotiation complete on port %d\n",
                                 port );

                linkStatus = FM_PORT_STATUS_AUTONEG_COMPLETE;
            }

            break;
    
        case FM_PORT_AUTONEG_CLAUSE_37:

            if( FM_GET_FIELD(rxPage, FM10000_AN_37_BASE_PAGE_RX, RemoteFault) )
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                "Auto-negotiation Remote Fault received "
                                "on port %d, AN_RX_MSG = 0x%llx\n",
                                 port, 
                                 rxPage );

                linkStatus = FM_PORT_STATUS_AUTONEG_REMOTE_FAULT;
            }
            else
            {

                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                                 port,
                                 "Auto-negotiation complete on port %d\n",
                                 port );

                linkStatus = FM_PORT_STATUS_AUTONEG_COMPLETE;
            }

            break;

        case FM_PORT_AUTONEG_SGMII:

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG,
                             port,
                            "Port %d: SGMII Complete with Link %d speed "
                            "%d duplex %d\n",
                             port, 
                             FM_GET_BIT( (fm_uint32)rxPage, 
                                         FM10000_SGMII_AN_RX_CONFIG, 
                                         Link ),
                             FM_GET_FIELD( (fm_uint32)rxPage, 
                                           FM10000_SGMII_AN_RX_CONFIG, 
                                           Speed ),
                             FM_GET_BIT( (fm_uint32)rxPage, 
                                         FM10000_SGMII_AN_RX_CONFIG, 
                                         Duplex ) );

            switch (FM_GET_FIELD( (fm_uint32)rxPage, 
                                   FM10000_SGMII_AN_RX_CONFIG, 
                                   Speed ))
            {
                case 0:
                    portExt->speed = 10;
                    break;
                case 1:
                    portExt->speed = 100;
                    break;
                case 2:
                    portExt->speed = 1000;
                    break;
                default:
                    portExt->speed = 0;
                    break;
            }

            if ( FM_GET_BIT(rxPage, FM10000_SGMII_AN_RX_CONFIG, Link) )
            {
                portPtr->phyInfo.phyStatus = FM_PHY_STATUS_LINK_UP;

                if ( !FM_GET_BIT(rxPage, FM10000_SGMII_AN_RX_CONFIG, Duplex) )
                {
                    /* FM10000 does not support half-duplex, the platform must
                     * not advertise half duplex capability, to prevent half
                     * duplex mode to be enabled between its partner.
                     */
                    FM_LOG_ERROR_V2( FM_LOG_CAT_PORT_AUTONEG,
                                     port,
                                     "Auto-negotiation Half Duplex received "
                                     "port %d. FM10000 does not support half "
                                     "duplex, the PHY must not advertise half "
                                     "duplex mode.\n",
                                     port );
                }
            }
            else
            {
                portPtr->phyInfo.phyStatus = FM_PHY_STATUS_LINK_DOWN;
                portExt->speed = 0;
            }

            linkStatus = FM_PORT_STATUS_AUTONEG_COMPLETE;
            break;

            
        case FM_PORT_AUTONEG_NONE:
            return FM_OK;

        default:
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT_AUTONEG, 
                                    port,
                                    status );
            break;
    }
    
    /* send the API event */
    status = SendApiAutoNegEvent( sw, port, linkStatus, rxPage );
        
ABORT:
    return status;

}   /* end fm10000NotifyApiAutonegCompleteOrFault */


/*****************************************************************************/
/** fm10000NotifyApiAutonegFailed
 * \ingroup intPort
 *
 * \desc            Notify the API user that autonegotiation failed and most
 *                  likely the AN state machine is being restarted.
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyApiAutonegFailed( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_int sw;
    fm_int port;

    sw   = ( ( fm10000_portSmEventInfo *)userInfo )->switchPtr->switchNumber;
    port = ( ( fm10000_portSmEventInfo *)userInfo )->portExt->base->portNumber;

    return SendApiAutoNegEvent( sw, port, FM_PORT_STATUS_AUTONEG_FAILED, 0);

}   /* end fm10000NotifyApiAutonegFailed */


/*****************************************************************************/
/** fm10000NotifyApiAutonegStarted
 * \ingroup intPort
 *
 * \desc            Notify the API user that autonegotiation started
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyApiAutonegStarted( fm_smEventInfo *eventInfo, 
                                          void           *userInfo )
{
    fm_int sw;
    fm_int port;

    sw   = ( ( fm10000_portSmEventInfo *)userInfo )->switchPtr->switchNumber;
    port = ( ( fm10000_portSmEventInfo *)userInfo )->portExt->base->portNumber;

    return SendApiAutoNegEvent( sw, port, FM_PORT_STATUS_AUTONEG_STARTED, 0);

}   /* end fm10000NotifyApiAutonegStarted */



/*****************************************************************************/
/** fm10000EnablePhyAutoneg
 * \ingroup intPort
 *
 * \desc            Function that enables autonegotiation on on a given port's
 *                  PHY
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *****************************************************************************/
fm_status fm10000EnablePhyAutoneg( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int      sw;
    fm_int      physPort;
    fm_port    *portPtr;
    fm_status   status = FM_OK;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    physPort = portPtr->physicalPort;

    if ( portPtr->phyInfo.phyAutoNegEnable )
    {
        status = portPtr->phyInfo.phyAutoNegEnable( sw, physPort, 0, portPtr );
    }

    return status;

}   /* end fm10000EnablePhyAutoneg */


/*****************************************************************************/
/** fm10000DisablePhyAutoneg
 * \ingroup intPort
 *
 * \desc            Function that disables autonegotiation on on a given port's
 *                  PHY
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *****************************************************************************/
fm_status fm10000DisablePhyAutoneg( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int   sw;
    fm_int   physPort;
    fm_port *portPtr;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    physPort = portPtr->physicalPort;

    if ( portPtr->phyInfo.phyAutoNegDisable )
    {
        /* Ignore status */
        portPtr->phyInfo.phyAutoNegDisable( sw, physPort, 0, portPtr );
    }

    return FM_OK;

}   /* end fm10000DisablePhyAutoneg */


/*****************************************************************************/
/** fm10000ConfigureAnTimers
 * \ingroup intPort
 *
 * \desc            Action callback that configures auto-negotiation timers in
 *                  the context of a state machine event notification
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000ConfigureAnTimers( fm_smEventInfo *eventInfo, 
                                    void           *userInfo )
{
    fm10000_port      *portExt;
    fm10000_portAttr  *portAttrExt;
    fm_status     status;

    portExt     = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    portAttrExt = ( ( fm10000_portSmEventInfo *)userInfo )->portAttrExt;

    status = FM_OK;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG, 
                     portExt->base->portNumber,
                     "AN Link Inhibit Timer: %d\n",
                     portAttrExt->autoNegLinkInhbTimer );

    /* Different timers between Clause 37 and Clause 73 */
    if ( portExt->anSmType == FM10000_CLAUSE37_AN_STATE_MACHINE )
    {
        status = ConfigureAn37Timers( eventInfo, userInfo );
    }
    else if ( portExt->anSmType == FM10000_CLAUSE73_AN_STATE_MACHINE )
    {
        status = ConfigureAn73Timers( eventInfo, userInfo );
    }
    else
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_PORT_AUTONEG,
                         portExt->base->portNumber,
                         "Unexpected State Machine type %d on port %d\n",
                         portExt->anSmType,
                         portExt->base->portNumber );
    }

    return status;

}   /* end fm10000ConfigureAnTimers */


/*****************************************************************************/
/** fm10000NotifyPortAutonegComplete
 * \ingroup intPort
 *
 * \chips           FM10000
 
 * \desc            Send the port state machine an event notification
 *                  indicating Autonegotiation has just completed
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyPortAutonegComplete( fm_smEventInfo *eventInfo, 
                                            void           *userInfo )
{

    return SendPortAnEventInd( eventInfo, 
                               userInfo, 
                               FM10000_PORT_EVENT_AN_COMPLETE_IND );

}   /* end fm10000NotifyPortAutonegComplete */


/*****************************************************************************/
/** fm10000NotifyPortAutonegRestarted
 * \ingroup intPort
 *
 * \desc            Send the port state machine an event notification
 *                  indicating Autonegotiation has just restarted
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyPortAutonegRestarted( fm_smEventInfo *eventInfo, 
                                             void           *userInfo )
{

    fm10000_port *portExt;

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portExt->anRestartCnt++;

    return SendPortAnEventInd( eventInfo, 
                               userInfo, 
                               FM10000_PORT_EVENT_AN_RESTARTED_IND );

}   /* end fm10000NotifyPortAutonegRestarted */


/*****************************************************************************/
/** fm10000DoAbilityMatch
 * \ingroup intPort
 *
 * \desc            Perform ability matching between the local port and the
 *                  the link parrner
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK
 * 
 *****************************************************************************/
fm_status fm10000DoAbilityMatch( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_portAttr   *portAttr;
    fm10000_port  *portExt;
    fm_int         sw;
    fm_int         port;
    fm_uint64      txPage;
    fm_uint64      rxPage;
    fm_int         hcd;
    fm_uint        ability;
    fm_int         basepage_hcd;
    fm_uint64      page;
    fm_uint        idx;

    portAttr = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;
    portExt  = ( ( fm10000_portSmEventInfo *)userInfo )->portExt;
    sw       = (( fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port     = portExt->base->portNumber;
    txPage   = portExt->basePage;
    rxPage   = portAttr->autoNegPartnerBasePage;

    hcd = AN73_HCD_INCOMPATIBLE_LINK;

    /* look for the highest common denominator with the link partner */
    ability = FM_GET_FIELD64( rxPage, FM10000_AN_73_BASE_PAGE_RX, A ) &
              FM_GET_FIELD64( txPage, FM10000_AN_73_BASE_PAGE_TX, A );

    /* determine the HCD using only the base page */
    basepage_hcd = An73AbilityToHCD(ability);

    FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG, 
                      port,
                      "Port %d basepage only HCD= %s\n",
                      port,
                      fm10000An73HCDStr( basepage_hcd ));
    /* only explore next pages if priority of the HCD is lower than 25G KR.
     * Note that IEEE 25G KR/CR have higher priority than consortium 25G AN */
    if (basepage_hcd < AN73_HCD_25_CR)
    {
        txPage = 0;
        rxPage = 0;
        /* Go thru next pages to see if there is extended tech ability there */
        if (fm10000AnGetNextPageExtTechAbilityIndex(sw,
                port,
                portAttr->autoNegNextPages.nextPages,
                portAttr->autoNegNextPages.numPages,
                &idx,
                "Tx") == FM_OK)
        {
            txPage = portAttr->autoNegNextPages.nextPages[idx];
        }
        if (fm10000AnGetNextPageExtTechAbilityIndex(sw,
                port,
                portAttr->autoNegPartnerNextPages.nextPages,
                portAttr->autoNegPartnerNextPages.numPages,
                &idx,
                "Rx") == FM_OK)
        {
            rxPage = portAttr->autoNegPartnerNextPages.nextPages[idx];
        }

        /* Resolve HCD */
        page = rxPage & txPage;

        /* We only support 25G and non-FEC, then we only need 
         *to check for whether to use KR1 or CR1 */

        if (FM_GET_UNNAMED_BIT64(page, 21))
        {
            hcd = AN73_HCD_25_CR;
            FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG, 
                              port,
                              "Port %d HCD= 25G-Consortium CR\n",
                              port);
        }
        else if (FM_GET_UNNAMED_BIT64(page, 20))
        {
            hcd = AN73_HCD_25_KR;
            FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG, 
                              port,
                              "Port %d HCD= 25G-Consortium KR\n",
                              port);
        }
        else
        {
            FM_LOG_DEBUG2_V2( FM_LOG_CAT_PORT_AUTONEG, 
                              port,
                              "Port %d HCD= 25G-Consortium. No abiltiy bit is set\n",
                              port);
        }

        if (hcd != AN73_HCD_INCOMPATIBLE_LINK)
        {
            FM_LOG_DEBUG2_V2(FM_LOG_CAT_PORT_AUTONEG,
                             port,
                             "Sw#%d Port %d: UM ExtTechAbility "
                             "TxNextPage=0x%016llx RxNextPage=0x%016llx HCD=%s\n",
                             sw,
                             port,
                             txPage,
                             rxPage,
                             fm10000An73HCDStr( hcd ) );
        }
    }

    /* If not resolved via next page exchange */
    if (hcd == AN73_HCD_INCOMPATIBLE_LINK)
    {
        txPage   = portExt->basePage;
        rxPage   = portAttr->autoNegPartnerBasePage;

        /* look for the highest common denominator with the link partner */
        ability = FM_GET_FIELD64( rxPage, FM10000_AN_73_BASE_PAGE_RX, A ) &
                  FM_GET_FIELD64( txPage, FM10000_AN_73_BASE_PAGE_TX, A );

        hcd = An73AbilityToHCD(ability);

        FM_LOG_DEBUG2_V2(FM_LOG_CAT_PORT_AUTONEG,
                         port,
                         "Sw#%d Port %d: TxPage=0x%016llx RxPage=0x%016llx HCD=%s\n",
                         sw,
                         port,
                         txPage,
                         rxPage,
                         fm10000An73HCDStr( hcd ) );

    }


    /* save the HCD in the event structure so that it can be picked up by
       the following action callbacks */
    ( ( fm10000_portSmEventInfo *)userInfo )->hcd = hcd;

    return FM_OK;

}   /* end fm10000DoAbilityMatch */




/*****************************************************************************/
/** fm10000StartAnQuickPollingTimer
 * \ingroup intPort
 *
 * \desc            Action starting the AN quick polling timer to hit after
 *                  a short delay. Typically this function is called the first
 *                  time this polling timer is started after AN is complete.
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartAnQuickPollingTimer( fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_status     status;
    fm10000_port *portExt;
    fm_timestamp  timeout;

    FM_NOT_USED(eventInfo);

    status = FM_OK;

    timeout.sec  = 0;
    timeout.usec = FM10000_AN_73_FAST_POLLING_DELAY_US;

    if (GET_PROPERTY()->enableStatusPolling == TRUE)
    {
        /* start an 500ms port-level timer. The callback will generate
           a FM10000_PORT_EVENT_POLLING_TIMER_EXP_IND event */
        portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

        /* Do not start this timer if eth mode is 1000Base-KX: in that mode
         * link up is usually very fast, so the timer is started later, when
         * processing the deferred_up timeout */
        if (portExt->ethMode != FM_ETH_MODE_1000BASE_KX)
        {
            status = fmStartTimer( portExt->timerHandle,
                                   &timeout,
                                   1, 
                                   HandleAnPollingTimerEvent,
                                   portExt );
        }
    }

    return status;

}   /* end fm10000StartAnQuickPollingTimer */




/*****************************************************************************/
/** fm10000StartAnPollingTimer
 * \ingroup intPort
 *
 * \desc            Action starting the AN polling timer.
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartAnPollingTimer( fm_smEventInfo *eventInfo,
                                      void           *userInfo )
{
    fm_status     status;
    fm10000_port *portExt;
    fm_timestamp  timeout;

    FM_NOT_USED(eventInfo);

    status = FM_OK;

    timeout.sec  = FM10000_AN_73_POLLING_DELAY_SEC;
    timeout.usec = 0;

    if (GET_PROPERTY()->enableStatusPolling == TRUE)
    {
        /* start an 2 sec port-level timer. The callback will generate
           a FM10000_PORT_EVENT_POLLING_TIMER_EXP_IND event */
        portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

        if (((fm10000_portSmEventInfo *)userInfo)->portAttrExt->eeeEnable &&
            portExt->ethMode == FM_ETH_MODE_1000BASE_KX)
        {
            /* 0.5 seconds for faster link down detection */
            timeout.sec  = 0;
            timeout.usec = FM10000_AN_73_FAST_POLLING_DELAY_US;
        }
    
        status = fmStartTimer( portExt->timerHandle,
                               &timeout,
                               1, 
                               HandleAnPollingTimerEvent,
                               portExt );
    }

    return status;

}   /* end fm10000StartAnPollingTimer */




/*****************************************************************************/
/** fm10000StopAnPollingTimer
 * \ingroup intPort
 *
 * \desc            Action stopping the AN polling timer 
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StopAnPollingTimer( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status     status;
    fm10000_port *portExt;

    FM_NOT_USED(eventInfo);

    status = FM_OK;

    if (GET_PROPERTY()->enableStatusPolling == TRUE)
    {
        portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    
        status = fmStopTimer( portExt->timerHandle );
    }
    return status;

}   /* end fm10000StopAnPollingTimer */




/*****************************************************************************/
/** fm10000PerformAnPortStatusValidation
 * \ingroup intPort
 *
 * \desc            Action performing a port status validation for
 *                  autonegotiated modes
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000PerformAnPortStatusValidation( fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    fm_status     status;
    fm_switch    *switchPtr;
    fm_int        sw;
    fm_port      *portPtr;
    fm10000_port *portExt;
    fm_int        port;
    fm_int        physLane;
    fm_int        epl;
    fm_uint32     pcsRxStatus;
    fm_uint       state;
    fm_uint       codeSyncStatus;
    fm_uint       rxLpiActive;
    fm_bool       eeeEnabled;
    fm_uint32     portStatus;
    fm_int        linkFault;
    fm_int        physPort;


    status = FM_OK;
    eventInfo->dontSaveRecord = TRUE;

    if (GET_PROPERTY()->enableStatusPolling == TRUE)
    {
        switchPtr  = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
        sw         = switchPtr->switchNumber;
        portExt    = ((fm10000_portSmEventInfo *)userInfo)->portExt;
        port       = portExt->base->portNumber;
        portPtr    = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
        physPort   = portPtr->physicalPort;
        eeeEnabled = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt->eeeEnable;
        epl        = portExt->endpoint.epl;
        physLane   = portExt->nativeLaneExt->physLane;

        if (eeeEnabled && portExt->ethMode == FM_ETH_MODE_1000BASE_KX)
        {
            status = switchPtr->ReadUINT32(sw,
                    FM10000_PCS_1000BASEX_RX_STATUS(epl, physLane),
                    &pcsRxStatus);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, status);

            state = FM_GET_FIELD(pcsRxStatus,
                                FM10000_PCS_1000BASEX_RX_STATUS, State);
            codeSyncStatus = FM_GET_BIT(pcsRxStatus,
                                FM10000_PCS_1000BASEX_RX_STATUS, CodeSyncStatus);
            rxLpiActive = FM_GET_BIT(pcsRxStatus,
                                FM10000_PCS_1000BASEX_RX_STATUS, RxLpiActive);

            if (state < 18 && codeSyncStatus == 0 && rxLpiActive == 1)
            {
                FM_LOG_DEBUG( FM_LOG_CAT_PORT,
                             "Force port %d link down on signal loss\n", 
                             port);

                /* Disable AN and reenable AN to bring link down */
                switchPtr->WriteUINT32(sw, FM10000_AN_73_CFG(epl, physLane), 0);
                switchPtr->WriteUINT32(sw, FM10000_AN_73_CFG(epl, physLane), 1);
            }
        }
        
        /* read the port status register */
        status = switchPtr->ReadUINT32( sw, 
                                        FM10000_PORT_STATUS( epl, physLane ), 
                                        &portStatus );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, status);

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Port %d: PORT_STATUS=0x%08x\n",
                         port,
                         portStatus );

        /* set the next state depending on the link fault value */
        linkFault = FM_GET_FIELD( portStatus, 
                                  FM10000_PORT_STATUS, 
                                  LinkFaultDebounced );
        if ( linkFault == 0 )
        {
            /* no fault */
            if (!portPtr->linkUp)
            {
                /* send a LINK_UP event if port is still DOWN at API Level */
                status = SendPortAnEventInd(eventInfo,userInfo,FM10000_PORT_EVENT_LINK_UP_IND);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, status);
            }
            
        }
        else if ( linkFault == 1)
        {
            /* local fault condition:
             *   enable event logging */
            eventInfo->dontSaveRecord = FALSE;

            /*   and disable AN and reenable AN to bring link down */
            switchPtr->WriteUINT32(sw, FM10000_AN_73_CFG(epl, physLane), 0);
            switchPtr->WriteUINT32(sw, FM10000_AN_73_CFG(epl, physLane), 1);
        }
    }

    return status;

}   /* end fm10000PerformAnPortStatusValidation */




/*****************************************************************************/
/** fm10000LogAnStateTransition
 * \ingroup intPort
 *
 * \desc            Log callback for one or more registered AN-level State
 *                  Transition tables
 * 
 * \param[in]       record is the pointer to a caller-allocated structure
 *                  containing the port state transition information to be
 *                  logged
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000LogAnStateTransition( fm_smTransitionRecord *record )
{
    fm_text currentState;
    fm_text event;

    if ( record->currentState != FM_STATE_UNSPECIFIED )
    {
        currentState = fm10000AnStatesMap[record->currentState];
    }
    else
    {
        currentState = "N/A";
    }
    if ( record->eventInfo.eventId != FM_EVENT_UNSPECIFIED )
    {
        event = fm10000AnEventsMap[record->eventInfo.eventId];
    }
    else
    {
        event = "N/A";
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     record->smUserID,
                     "Port %d Transition: "
                     "'%s': '%s' ==> '%s'\n",
                     record->smUserID,
                     event,
                     currentState,
                     fm10000AnStatesMap[record->nextState] );

    return FM_OK;

}   /* end fm10000LogAnStateTransition */


/*****************************************************************************/
/** fm10000Dummy
 * \ingroup intPort
 *
 * \desc            Dummy conditional transition callback. Required because
 *                  the auto-generation engine requires at least one
 *                  conditional transition callback to be defined
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 *
 * \return          FM_OK if successful
 *****************************************************************************/
fm_status fm10000Dummy( fm_smEventInfo *eventInfo, 
                        void           *userInfo, 
                        fm_int         *nextState )
{
    FM_NOT_USED( eventInfo );
    FM_NOT_USED( userInfo );
    FM_NOT_USED( nextState );

    return FM_OK;

}   /* end fm10000Dummy */
