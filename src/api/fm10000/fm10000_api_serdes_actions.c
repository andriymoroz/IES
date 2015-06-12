/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_serdes_actions.c
 * Creation Date:   December 13, 2013
 * Description:     Action callbacks for the FM10000 serdes state machines
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
 
static void HandlePllTimeout( void *arg );
static void HandleSignalTimeout( void *arg );
static void HandleTuningTimeout( void *arg );

 

static void HandleSerDesTimeout( void *arg );
static fm_status SendDfeEventReq( fm_smEventInfo *eventInfo,
                                  void           *userInfo,
                                  fm_int          eventId );
static fm_status StartTimeoutTimer(fm_smEventInfo *eventInfo,
                                   void           *userInfo,
                                   fm_timestamp   *pTimeout);
static fm_status EnableRxBistMode(fm_smEventInfo *eventInfo,
                                  void           *userInfo,
                                  fm_int          bistSubMode);


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
/** SendDfeEventReq
 * \ingroup intSerdes
 *
 * \desc            Send a DFE-level event notification to the dfe state
 *                  machines belonging to the current serdes
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[in]       eventId is the event to be notified.
 *
 * \return          FM_OK if successful
 *
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *****************************************************************************/
fm_status SendDfeEventReq( fm_smEventInfo *eventInfo,
                           void           *userInfo,
                           fm_int          eventId )
{
    fm_status        err;
    fm_int           sw;
    fm10000_lane    *pLaneExt;
    fm_smEventInfo   dfeEventInfo;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    dfeEventInfo.smType  = pLaneExt->dfeExt.smType;
    dfeEventInfo.eventId = eventId;
    dfeEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    dfeEventInfo.dontSaveRecord = FALSE;

    err = fmNotifyStateMachineEvent( pLaneExt->dfeExt.smHandle,
                                     &dfeEventInfo,
                                     &pLaneExt->dfeExt.eventInfo,
                                     &pLaneExt->serDes );


    return err;

}   /* end SendDfeEventReq */




/*****************************************************************************/
/** HandlePllTimeout
 * \ingroup intSerdes
 *
 * \desc            Handles a timeout simulating the time needed for the SerDes
 *                  Tx and Rx PLLs to gain lock
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandlePllTimeout( void *arg )
{
    fm_smEventInfo             eventInfo;
    fm10000_serDesSmEventInfo *serDesEventInfo;
    fm10000_lane              *laneExt;
    fm_int                     sw;


    laneExt           = arg;
    eventInfo.smType  = laneExt->smType;
    eventInfo.eventId = FM10000_SERDES_EVENT_RXTX_PLLS_LOCKED_IND;
    serDesEventInfo   = &laneExt->eventInfo;
    sw                = serDesEventInfo->switchPtr->switchNumber;



    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    fmNotifyStateMachineEvent( laneExt->smHandle,
                               &eventInfo,
                               serDesEventInfo,
                               &laneExt->serDes );
    UNPROTECT_SWITCH( sw );

}   /* end HandlePllTimeout */




/*****************************************************************************/
/** HandleSignalTimeout
 * \ingroup intSerdes
 *
 * \desc            Handles a timeout simulating the time needed for the SerDes
 *                  to start detecting input signal
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleSignalTimeout( void *arg )
{
    fm_smEventInfo             eventInfo;
    fm10000_serDesSmEventInfo *serDesEventInfo;
    fm10000_lane              *laneExt;
    fm_int                     sw;


    laneExt           = arg;
    eventInfo.smType  = laneExt->smType;
    eventInfo.eventId = FM10000_SERDES_EVENT_SIGNALOK_ASSERTED_IND;
    serDesEventInfo   = &laneExt->eventInfo;
    sw                = serDesEventInfo->switchPtr->switchNumber;




    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    fmNotifyStateMachineEvent( laneExt->smHandle,
                               &eventInfo,
                               serDesEventInfo,
                               &laneExt->serDes );
    UNPROTECT_SWITCH( sw );

}   /* end HandleSignalTimeout */




/*****************************************************************************/
/** HandleTuningTimeout
 * \ingroup intSerdes
 *
 * \desc            Handles a timeout simulating the time needed for the SerDes
 *                  to complete DFE tuning or KR training
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleTuningTimeout( void *arg )
{
    fm_smEventInfo             eventInfo;
    fm10000_serDesSmEventInfo *serDesEventInfo;
    fm10000_lane              *laneExt;
    fm_int                     sw;


    laneExt           = arg;
    eventInfo.smType  = laneExt->smType;
    if ( laneExt->krTrainingEn == TRUE )
    {


        eventInfo.eventId = FM10000_SERDES_EVENT_SIGNALOK_ASSERTED_IND;
    }
    else
    {
        eventInfo.eventId = FM10000_PORT_EVENT_LANE_DFE_COMPLETE_IND;
    }
    serDesEventInfo   = &laneExt->eventInfo;
    sw                = serDesEventInfo->switchPtr->switchNumber;



    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    fmNotifyStateMachineEvent( laneExt->smHandle,
                               &eventInfo,
                               serDesEventInfo,
                               &laneExt->serDes );
    UNPROTECT_SWITCH( sw );

}   /* end HandleTuningTimeout */




 
 
 



/*****************************************************************************/
/** HandleSerDesTimeout
 * \ingroup intSerdes
 *
 * \desc            Handles a generic SerDes timeout situation sending a
 *                  ''FM10000_SERDES_EVENT_TIMEOUT_IND'' event to the proper
 *                  SerDes state machine.
 *
 * \param[in]       arg is an opaque pointer that points to the argument passed
 *                  when the timer was started and it must be properly casted.
 *                  In this case it points to the lane extension structure
 *                  (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
void HandleSerDesTimeout( void *arg )
{
    fm_smEventInfo             eventInfo;
    fm10000_serDesSmEventInfo *serDesEventInfo;
    fm10000_lane              *laneExt;
    fm_int                     sw;


    laneExt           = arg;
    eventInfo.smType  = laneExt->smType;
    eventInfo.eventId = FM10000_SERDES_EVENT_TIMEOUT_IND;
    serDesEventInfo   = &laneExt->eventInfo;
    sw                = serDesEventInfo->switchPtr->switchNumber;



    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    fmNotifyStateMachineEvent( laneExt->smHandle,
                               &eventInfo,
                               serDesEventInfo,
                               &laneExt->serDes );
    UNPROTECT_SWITCH( sw );

}   /* end HandleSerDesTimeout */




/*****************************************************************************/
/** StartTimeoutTimer
 * \ingroup intSerdes
 *
 * \desc            Start SerDes timeout timer. Timer is started using the
 *                  provided expiration time. The number of repetitions is
 *                  always set to 1.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[in]       pTimeout points to a ''fm_timestamp'' structure that
 *                  specifies the timer expiration time.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status StartTimeoutTimer(fm_smEventInfo *eventInfo,
                            void           *userInfo,
                            fm_timestamp   *pTimeout)
{
    fm_status     err;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    err = fmStartTimer( pLaneExt->timerHandle,
                        pTimeout,
                        1,
                        HandleSerDesTimeout,
                        pLaneExt );
    return err;

}   /* end StartTimeoutTimer */




/*****************************************************************************/
/** EnableRxBistMode
 * \ingroup intSerdes
 *
 * \desc            Enable Bist on the Rx section of this SerDes. Only Rx and
 *                  TxRx modes are supported. In the case of the latter, only
 *                  the Rx section is configured. Bist error counter is reset
 *                  to zero.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[in]       bistSubMode is the BIST submode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status EnableRxBistMode(fm_smEventInfo *eventInfo,
                                  void           *userInfo,
                                  fm_int          bistSubMode)
{
    fm_status     err;
    fm10000_lane *pLaneExt;
    fm_int        serDes;
    fm_int        sw;

    err = FM_OK;

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    switch (bistSubMode)
    {
        
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        
        case FM_BIST_TXRX_PRBS_128:
        case FM_BIST_TXRX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_2G:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        {
            err = SendDfeEventReq(eventInfo,
                                  userInfo,
                                  FM10000_SERDES_DFE_EVENT_PAUSE_TUNING_REQ);

            if (err == FM_OK)
            {
                err = fm10000SetSerdesRxPattern(sw,
                                                serDes,
                                                bistSubMode,
                                                pLaneExt->bistCustomData0,
                                                pLaneExt->bistCustomData1);
            }
            break;
        }
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TX_CJPAT:
        case FM_BIST_RX_CJPAT:
        case FM_BIST_TXRX_CJPAT:
        {
            err = FM_ERR_UNSUPPORTED;
            break;
        }
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TX_HIGHFREQ:
        case FM_BIST_TX_MIXEDFREQ:
        case FM_BIST_TX_SQUARE8:
        case FM_BIST_TX_SQUARE10:
        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TX_CUSTOM80:
        default:
            err = FM_ERR_INVALID_SUBMODE;
    }

    if (err == FM_OK)
    {
        err = fm10000ResetSerdesErrorCounter(sw, serDes);
    }

    return err;

}   /* end EnableRxBistMode */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000LogSerDesTransition
 * \ingroup intSerdes
 *
 * \desc            Provide a description of what the function does.
 *
 * \param[in]       record is the pointer to a caller-allocated structure
 *                  containing the SerDes state transition information to be
 *                  logged
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000LogSerDesTransition( fm_smTransitionRecord *record )
{
    fm_text currentState;
    fm_text event;

    if ( record->currentState != FM_STATE_UNSPECIFIED )
    {
        currentState = fm10000SerDesStatesMap[record->currentState];
    }
    else
    {
        currentState = "N/A";
    }
    if ( record->eventInfo.eventId != FM_EVENT_UNSPECIFIED )
    {
        event = fm10000SerDesEventsMap[record->eventInfo.eventId];
    }
    else
    {
        event = "N/A";
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_SERDES,
                     record->smUserID,
                     "SerDes %d Transition: "
                     "'%s': '%s' ==> '%s'\n",
                     record->smUserID,
                     event,
                     currentState,
                     fm10000SerDesStatesMap[record->nextState] );

    return FM_OK;

}   /* end fm10000LogSerDesTransition */




/*****************************************************************************/
/** fm10000SerDesStubAction
 * \ingroup intSerdes
 *
 * \desc            Action replacing actual actions operating on SerDes
 *                  registers in the stub SerDes State Machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unused in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (unused in this function)
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000SerDesStubAction( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    if ( eventInfo->eventId == FM10000_SERDES_EVENT_CONFIGURE_DFE_REQ )
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_SERDES,
                         laneExt->serDes,
                         "SERDES_EVENT_CONFIGURE_DFE_REQ: dfeMode=%d\n",
                         laneExt->eventInfo.info.dfeMode );
    }


    return FM_OK;

}   /* end fm10000SerDesStubAction */




/*****************************************************************************/
/** fm10000SerDesFlagError
 * \ingroup intSerdes
 *
 * \desc            Action reporting an error for an invalid combination
 *                  state-event.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unused in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (unused in this function)
 *
 * \return          FM_ERR_INVALID_STATE
 *
 *****************************************************************************/
fm_status fm10000SerDesFlagError(fm_smEventInfo *eventInfo,
                                 void           *userInfo)
{
    FM_NOT_USED(eventInfo);
    FM_NOT_USED(userInfo);



    return FM_ERR_INVALID_STATE;

}   /* end fm10000SerDesFlagError */




/*****************************************************************************/
/** fm10000FlagError
 * \ingroup intSerdes
 *
 * \desc            Action reporting an error for an invalid combination
 *                  state-event.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unused in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (unused in this function)
 *
 * \return          FM_ERR_INVALID_STATE
 *
 *****************************************************************************/
fm_status fm10000FlagError(fm_smEventInfo *eventInfo,
                           void           *userInfo)
{
    FM_NOT_USED(eventInfo);
    FM_NOT_USED(userInfo);

    




    return FM_ERR_INVALID_STATE;

}   /* end fm10000FlagError */




/*****************************************************************************/
/** fm10000SerDesStartStubPllTimer
 * \ingroup intSerdes
 *
 * \desc            Action starting a 64 msec timer simulating the time needed
 *                  for the PLL to gain lock
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartStubPllTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;
    fm_timestamp  timeout = { 0, 64000 };

    FM_NOT_USED(eventInfo);



    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return  fmStartTimer( laneExt->timerHandle,
                          &timeout,
                          1,
                          HandlePllTimeout,
                          laneExt );

}   /* end fm10000SerDesStartStubPllTimer */




/*****************************************************************************/
/** fm10000SerDesStopStubPllTimer
 * \ingroup intSerdes
 *
 * \desc            Action stopping the 64 msec timer simulating the time
 *                  needed for the PLL to gain lock
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStopStubPllTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane              *laneExt;

    FM_NOT_USED(eventInfo);


    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return fmStopTimer( laneExt->timerHandle );

}   /* end fm10000SerDesStartStubPllTimer */




/*****************************************************************************/
/** fm10000SerDesStartStubSignalTimer
 * \ingroup intSerdes
 *
 * \desc            Action starting the 100 msec timer simulating the timer
 *                  needed for the SerDes to detect valid signal on the Rx side
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartStubSignalTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;
    fm_timestamp  timeout = { 0, 100000 };

    FM_NOT_USED(eventInfo);



    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return  fmStartTimer( laneExt->timerHandle,
                          &timeout,
                          1,
                          HandleSignalTimeout,
                          laneExt );

}   /* end fm10000SerDesStartStubSignalTimer */




/*****************************************************************************/
/** fm10000SerDesStopStubSignalTimer
 * \ingroup intSerdes
 *
 * \desc            Action stopping the 100 msec timer simulating the timer
 *                  needed for the SerDes to detect valid signal on the Rx side
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStopStubSignalTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane              *laneExt;

    FM_NOT_USED(eventInfo);


    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return fmStopTimer( laneExt->timerHandle );

}   /* end fm10000SerDesStartStubSignalTimer */




/*****************************************************************************/
/** fm10000SerDesStartStubTuningTimer
 * \ingroup intSerdes
 *
 * \desc            Action starting the 500 msec timer simulating the time
 *                  needed for the SerDes to completed KR training or DFE
 *                  tuning (depending on the configured ethMode)
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartStubTuningTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane  *laneExt;
    fm_timestamp   timeout = { 0, 500000 };

    FM_NOT_USED(eventInfo);




    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return  fmStartTimer( laneExt->timerHandle,
                          &timeout,
                          1,
                          HandleTuningTimeout,
                          laneExt );

}   /* end fm10000SerDesStartStubTuningTimer */




/*****************************************************************************/
/** fm10000SerDesStopStubTuningTimer
 * \ingroup intSerdes
 *
 * \desc            Action stopping the 500 msec timer simulating the time
 *                  needed for the SerDes to completed KR training or DFE
 *                  tuning (depending on the configured ethMode)
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 *
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStopStubTuningTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane              *laneExt;

    FM_NOT_USED(eventInfo);


    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    return fmStopTimer( laneExt->timerHandle );

}   /* end fm10000SerDesStartStubTuningTimer */




/*****************************************************************************/
/** fm10000SerDesSendPortLaneReadyInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a Lane Ready Indication event to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortLaneReadyInd( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;

    portEventInfo.smType  = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_READY_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortLaneReadyInd */




/*****************************************************************************/
/** fm10000SerDesSendPortLaneNotReadyInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a Lane Not Ready Indication event to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortLaneNotReadyInd( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;


    portEventInfo.smType = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_NOT_READY_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortLaneNotReadyInd */




/*****************************************************************************/
/** fm10000SerDesSendPortKrTrainingCompleteInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a KR Training Complete indication to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortKrTrainingCompleteInd(fm_smEventInfo *eventInfo,
                                                     void           *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;


    portEventInfo.smType = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_KR_COMPLETE_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortKrTrainingCompleteInd */




/*****************************************************************************/
/** fm10000SerDesSendPortKrTrainingFailedInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a KR Training Failed indication to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortKrTrainingFailedInd( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;


    portEventInfo.smType = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_KR_FAILED_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortKrTrainingFailedInd */




/*****************************************************************************/
/** fm10000SerDesSendPortDfeTuningCompleteInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a DFE Tuning Complete indication to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortDfeTuningCompleteInd( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;


    portEventInfo.smType = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_DFE_COMPLETE_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortDfeTuningCompleteInd */




/*****************************************************************************/
/** fm10000SerDesSendPortDfeTuningFailedInd
 * \ingroup intSerdes
 *
 * \desc            Action sending a DFE Tuning Failed indication to the
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendPortDfeTuningFailedInd( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm_smEventInfo  portEventInfo;
    fm_int          sw;
    fm_int          physLane;

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    physLane = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->physLane;
    portExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->parentPortExt;


    portEventInfo.smType = portExt->smType;
    portEventInfo.eventId = FM10000_PORT_EVENT_LANE_DFE_FAILED_IND;
    portEventInfo.lock    = FM_GET_STATE_LOCK( sw );
    portExt->eventInfo.info.physLane = physLane;
    portEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->smHandle,
                                      &portEventInfo,
                                      &portExt->eventInfo,
                                      &portExt->base->portNumber );

}   /* end fm10000SerDesSendPortDfeTuningFailedInd */




/*****************************************************************************/
/** fm10000SerDesMarkBothPllsUp
 * \ingroup intSerdes
 *
 * \desc            Action marking both PLLs up for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkBothPllsUp( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask = 3;
    }
    return FM_OK;

}   /* end fm10000SerDesMarkBothPllsUp */




/*****************************************************************************/
/** fm10000SerDesMarkTxPllUp
 * \ingroup intSerdes
 *
 * \desc            Action marking TX PLL up for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkTxPllUp( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask |= 1;
    }

    return FM_OK;

}   /* end fm10000SerDesMarkTxPllUp */




/*****************************************************************************/
/** fm10000SerDesMarkRxPllUp
 * \ingroup intSerdes
 *
 * \desc            Action marking RX PLL up for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkRxPllUp( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask |= 2;
    }
    return FM_OK;

}   /* end fm10000SerDesMarkRxPllUp */




/*****************************************************************************/
/** fm10000SerDesMarkBothPllsDown
 * \ingroup intSerdes
 *
 * \desc            Action marking both PLLs down for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkBothPllsDown( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask = 0;
    }
    return FM_OK;

}   /* end fm10000SerDesMarkBothPllsDown */




/*****************************************************************************/
/** fm10000SerDesMarkTxPllDown
 * \ingroup intSerdes
 *
 * \desc            Action marking the TX PLL down for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkTxPllDown( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask &= 0xFE;
    }
    return FM_OK;

}   /* end fm10000SerDesMarkTxPllDown */




/*****************************************************************************/
/** fm10000SerDesMarkRxPllDown
 * \ingroup intSerdes
 *
 * \desc            Action marking the RX PLL down for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000SerDesMarkRxPllDown(fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm10000_lane *laneExt;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_EPL )
    {
        laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
        laneExt->pllMask &= 0xFD;
    }
    return FM_OK;

}   /* end fm10000SerDesMarkRxPllDown */




/*****************************************************************************/
/** fm10000SerDesProcessStubSignalTimer
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing a signal
 *                  detection event by switching to a configuration-dependent
 *                  state
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessStubSignalTimer( fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status         status;
    fm10000_lane     *laneExt;
    fm_laneAttr      *laneAttr;
    fm_int            serDes;

    FM_NOT_USED(eventInfo);


     

    laneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    laneAttr = ((fm10000_serDesSmEventInfo *)userInfo)->laneAttr;
    serDes   = laneExt->serDes;

    status = FM_OK;
    switch ( laneExt->pllMask )
    {
        
        case 0:
            *nextState = FM10000_SERDES_STATE_CONFIGURED;
            break;

        
        case 1:
            *nextState = FM10000_SERDES_STATE_TX_ON;
            break;

        
        case 2:
        case 3:
            if ( laneExt->krTrainingEn != FALSE              ||
                 laneAttr->dfeMode     != FM_DFE_MODE_STATIC )
            {
                if ( laneExt->krTrainingEn )
                {
                    *nextState = FM10000_SERDES_STATE_KR_TRAINING;
                }
                else
                {
                    *nextState = FM10000_SERDES_STATE_DFE_TUNING;
                }

                status = fm10000SerDesStartStubTuningTimer( eventInfo, userInfo );
            }
            else
            {
                *nextState = FM10000_SERDES_STATE_MISSION;
            }
            break;

        
        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Invalid PLLs mask 0x%08x\n",
                             laneExt->pllMask );
            status = FM_ERR_INVALID_STATE;
            break;

    }   /* end switch ( laneExt->pllMask ) */


    return status;

}   /* end fm10000SerDesProcessStubSignalTimer */




/*****************************************************************************/
/** fm10000SerDesProcessStubPllTimer
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing the expiration
 *                  of the Stub PLL timer by switching to a state dependent
 *                  on the combination of PLLs that are powered up
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessStubPllTimer( fm_smEventInfo *eventInfo,
                                      void           *userInfo,
                                      fm_int         *nextState )
{
    fm_status      status;
    fm10000_lane  *laneExt;
    fm_int         serDes;

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = laneExt->serDes;

    status = FM_OK;
    switch ( laneExt->pllMask )
    {
        
        case 0:
            *nextState = FM10000_SERDES_STATE_CONFIGURED;
            break;

        
        case 1:
            status     = fm10000SerDesSendPortLaneReadyInd( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, serDes, status );

            *nextState = FM10000_SERDES_STATE_TX_ON;
            break;

        
        case 2:
        case 3:
            status     = fm10000SerDesStartStubSignalTimer( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, serDes, status );

            status     = fm10000SerDesSendPortLaneReadyInd( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, serDes, status );

            *nextState = FM10000_SERDES_STATE_POWERED_UP;
            break;

        
        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Invalid PLLs mask 0x%08x\n",
                             laneExt->pllMask );
            status = FM_ERR_INVALID_STATE;
            break;

    }   /* end switch ( laneExt->pllMask ) */


ABORT:
    return status;

}   /* end fm10000SerDesProcessStubPllTimer */




/*****************************************************************************/
/** fm10000SerDesProcessStubTuningTimer
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing the expiration
 *                  of the tuning timer by switching to a state dependent
 *                  on the combination of PLLs that are powered up
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_serdesSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessStubTuningTimer( fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      status;
    fm10000_lane  *laneExt;
    fm_int         serDes;

    FM_NOT_USED(eventInfo);

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = laneExt->serDes;

    status = FM_OK;
    switch ( laneExt->pllMask )
    {
        
        case 0:
            *nextState = FM10000_SERDES_STATE_CONFIGURED;
            break;

        
        case 1:
            *nextState = FM10000_SERDES_STATE_TX_ON;
            break;

        
        case 2:
            *nextState = FM10000_SERDES_STATE_RX_ON;
            break;

        
        case 3:
            *nextState = FM10000_SERDES_STATE_MISSION;
            break;

        
        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Invalid PLLs mask 0x%08x\n",
                             laneExt->pllMask );
            status = FM_ERR_INVALID_STATE;
            break;

    }   /* end switch ( laneExt->pllMask ) */


    return status;

}   /* end fm10000SerDesProcessStubTuningTimer */




 
 
 




/*****************************************************************************/
/** fm10000SerDesInitStateVar
 * \ingroup intSerdes
 *
 * \desc            Action initializing several state variables used by the
 *                  state machine.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesInitStateVar( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm10000_lane   *pLaneExt;
    fm_serDesOpMode serdesOpMode;

    FM_NOT_USED(eventInfo);

    err = FM_OK;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    pLaneExt->serDesEnableCache = 0;
    pLaneExt->serDesPllStatus = 0;
    pLaneExt->serDesEnableRetryCtrl = 0;
    pLaneExt->bistActive = FALSE;
    pLaneExt->bistTxSubMode = FM_BIST_MAX;
    pLaneExt->bistRxSubMode = FM_BIST_MAX;

    
    err = fm10000SerdesGetOpMode(sw,0,&serdesOpMode,NULL,NULL);

    if (err == FM_OK && serdesOpMode == FM_SERDES_OPMODE_TEST_BOARD)
    {
        pLaneExt->dbgLvl = FM10000_SERDES_DBG_TEST_BOARD_LVL_1;
    }
    else
    {
        pLaneExt->dbgLvl = FM10000_SERDES_DGB_DISABLED;
    }

    return err;

}   /* end fm10000SerDesInitStateVar */




/*****************************************************************************/
/** fm10000SerDesSaveDfeConfig
 * \ingroup intSerdes
 *
 * \desc            Action saving the DFE mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSaveDfeConfig(fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status     err;
    fm_int        newDfeMode;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    err = FM_OK;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    newDfeMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.dfeMode;

    
    if (newDfeMode < 0 || newDfeMode > FM_DFE_MODE_ICAL_ONLY )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        
        pLaneExt->dfeMode = newDfeMode;
    }

    return err;

}   /* end fm10000SerDesSaveDfeConfig */




/*****************************************************************************/
/** fm10000SerDesSaveBitRateConfig
 * \ingroup intSerdes
 *
 * \desc            Action saving the bit rate.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSaveBitRateConfig(fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_status           err;
    fm_int              serDes;
    fm_int              newBitRate;
    fm10000_lane *      pLaneExt;
    fm_uint             rateSel;
    fm_serdesWidthMode  widthMode;

    FM_NOT_USED(eventInfo);

    pLaneExt   = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes     = pLaneExt->serDes;
    newBitRate = ((fm10000_serDesSmEventInfo *)userInfo)->info.bitRate;

    pLaneExt->nearLoopbackEn = FALSE;
    err = fm10000GetSerdesWidthModeRateSel(serDes, newBitRate, &widthMode, &rateSel);

    
    if (err == FM_OK )
    {
        pLaneExt->prevBitRate = pLaneExt->bitRate;
        pLaneExt->bitRate = newBitRate;
        pLaneExt->rateSel = rateSel;
        pLaneExt->widthMode = widthMode;
    }

    return err;

}   /* end fm10000SerDesSaveBitRateConfig */




/*****************************************************************************/
/** fm10000SerDesSetFarLoopbackModeOn
 * \ingroup intSerdes
 *
 * \desc            Action enabling the far loopback mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSetFarLoopbackModeOn(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    pLaneExt->farLoopbackStatus = 0x01;

    return FM_OK;

}   /* end fm10000SerDesSetFarLoopbackModeOn */




/*****************************************************************************/
/** fm10000SerDesSetFarLoopbackModeOff
 * \ingroup intSerdes
 *
 * \desc            Action disabling the far loopback mode (default).
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSetFarLoopbackModeOff(fm_smEventInfo *eventInfo,
                                             void           *userInfo )
{
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    pLaneExt->farLoopbackStatus = 0x00;

    return FM_OK;

}   /* end fm10000SerDesSetFarLoopbackModeOff */




/*****************************************************************************/
/** fm10000SerDesStartTimeoutTimerShrt
 * \ingroup intSerdes
 *
 * \desc            Action starting the SerDes timeout timer using the a short
 *                  expiration time.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartTimeoutTimerShrt(fm_smEventInfo *eventInfo,
                                             void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_SHORT_TIMEOUT;

    err = StartTimeoutTimer(eventInfo, userInfo, &timeout);

    return err;

}   /* end fm10000SerDesStartTimeoutTimerShrt */




/*****************************************************************************/
/** fm10000SerDesStartTimeoutTimerLng
 * \ingroup intSerdes
 *
 * \desc            Action starting the SerDes timeout timer using the a long
 *                  expiration time.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartTimeoutTimerLng(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_LONG_TIMEOUT;

    err = StartTimeoutTimer(eventInfo,userInfo,&timeout);

    return err;

}   /* end fm10000SerDesStartTimeoutTimerLng */




/*****************************************************************************/
/** fm10000SerDesStartTimeoutTimerXLng
 * \ingroup intSerdes
 *
 * \desc            Action starting the SerDes timeout timer using the an extra
 *                  long expiration time.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartTimeoutTimerXLng(fm_smEventInfo *eventInfo,
                                             void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_XLONG_TIMEOUT;

    err = StartTimeoutTimer(eventInfo,userInfo,&timeout);

    return err;

}   /* end fm10000SerDesStartTimeoutTimerXLng */




/*****************************************************************************/
/** fm10000SerDesStartKrDeferralTimer
 * \ingroup intSerdes
 *
 * \desc            Action starting the SerDes timer deferring complementary
 *                  actions after KR training is complete.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartKrDeferralTimer(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_DEFERRED_KR_COMPLETE;

    err = StartTimeoutTimer(eventInfo,userInfo,&timeout);

    return err;

}   /* end fm10000SerDesStartKrDeferralTimer */




/*****************************************************************************/
/** fm10000SerDesStopTimeoutTimer
 * \ingroup intSerdes
 *
 * \desc            Action stopping the SerDes timeout timer.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStopTimeoutTimer( fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_status       err;
    fm10000_lane   *pLaneExt;

    FM_NOT_USED(eventInfo);

    
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    err = fmStopTimer( pLaneExt->timerHandle );

    return err;

}   /* end fm10000SerDesStopTimeoutTimer */




/*****************************************************************************/
/** fm10000SerDesRstSignalOkDebounce
 * \ingroup intSerdes
 *
 * \desc            Action reseting the signalOk debounce counter.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesRstSignalOkDebounce(fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_status     err;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    err = FM_OK;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    pLaneExt->signalOkDebounce = 0;

    return err;

}   /* end fm10000SerDesRstSignalOkDebounce */




/*****************************************************************************/
/** fm10000SerDesSetSignalDtctForcedBadC
 * \ingroup intSerdes
 *
 * \desc            Action setting the field SdOverride of LANE_SIGNAL_DETECT_CFG
 *                  register to FORCE_BAD. 
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSetSignalDtctForcedBadC(fm_smEventInfo *eventInfo,
                                               void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm10000_lane *pLaneExt;
    fm_int        serDes;
    fm_int        epl;
    fm_int        channel;
    fm_switch    *switchPtr;
    fm_uint32     laneSignalDetectCfgAddr;
    fm_uint32     laneSignalDetectCfg;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    

    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    channel   = pLaneExt->channel;
    epl       = pLaneExt->epl;

    
    laneSignalDetectCfgAddr = FM10000_LANE_SIGNAL_DETECT_CFG(epl, channel);

    err = switchPtr->ReadUINT32(sw, laneSignalDetectCfgAddr, &laneSignalDetectCfg);

    if (err == FM_OK)
    {
        
        FM_SET_FIELD(laneSignalDetectCfg, FM10000_LANE_SIGNAL_DETECT_CFG, SdOverride, 2);

        err = switchPtr->WriteUINT32(sw, laneSignalDetectCfgAddr, laneSignalDetectCfg);
    }

    return err;

}   /* end fm10000SerDesSetSignalDtctForcedBadC */




/*****************************************************************************/
/** fm10000SerDesSetStaticDfeSignalDtctNormal
 * \ingroup intSerdes
 *
 * \desc            Action restoring to the normal state the field SdOverride of
 *                  LANE_SIGNAL_DETECT_CFG when the serdes is configured with
 *                  a static DFE mode or the API attribute ''LINK_DEPENDS_ON_DFE''
 *                  is FALSE. 
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSetStaticDfeSignalDtctNormal(fm_smEventInfo *eventInfo,
                                                    void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm10000_lane *pLaneExt;
    fm_bool       linkDependsOnDfe;
    fm_int        serDes;
    fm_int        epl;
    fm_int        channel;
    fm_switch    *switchPtr;
    fm_uint32     laneSignalDetectCfgAddr;
    fm_uint32     laneSignalDetectCfg;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;


    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    channel   = pLaneExt->channel;
    epl       = pLaneExt->epl;

    
     

     

     

    linkDependsOnDfe = fmGetBoolApiProperty(FM_AAK_API_FM10000_LINK_DEPENDS_ON_DFE,
                                            FM_AAD_API_FM10000_LINK_DEPENDS_ON_DFE);

    if ( pLaneExt->dfeMode != FM_DFE_MODE_KR           &&
        (!linkDependsOnDfe                             ||
        (pLaneExt->dfeMode != FM_DFE_MODE_ONE_SHOT &&
         pLaneExt->dfeMode != FM_DFE_MODE_CONTINUOUS &&
         pLaneExt->dfeMode != FM_DFE_MODE_ICAL_ONLY)) )
    {

        laneSignalDetectCfgAddr = FM10000_LANE_SIGNAL_DETECT_CFG(epl, channel);

        err = switchPtr->ReadUINT32( sw, laneSignalDetectCfgAddr, &laneSignalDetectCfg);

        if (err == FM_OK)
        {
            

            FM_SET_FIELD(laneSignalDetectCfg, FM10000_LANE_SIGNAL_DETECT_CFG, SdOverride, 0);

            err = switchPtr->WriteUINT32( sw, laneSignalDetectCfgAddr, laneSignalDetectCfg);
        }
    }

    return err;

}   /* end fm10000SerDesSetStaticDfeSignalDtctNormal */




/*****************************************************************************/
/** fm10000SerDesSetSignalDtctNormal
 * \ingroup intSerdes
 *
 * \desc            Action setting the field SdOverride of LANE_SIGNAL_DETECT_CFG
 *                  register to NORMAL. This is action performed inconditionally.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSetSignalDtctNormal( fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm10000_lane *pLaneExt;
    fm_int        serDes;
    fm_int        epl;
    fm_int        channel;
    fm_switch    *switchPtr;
    fm_uint32     laneSignalDetectCfgAddr;
    fm_uint32     laneSignalDetectCfg;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    channel   = pLaneExt->channel;
    epl       = pLaneExt->epl;

    
    laneSignalDetectCfgAddr = FM10000_LANE_SIGNAL_DETECT_CFG(epl, channel);

    err = switchPtr->ReadUINT32( sw, laneSignalDetectCfgAddr, &laneSignalDetectCfg);

    if (err == FM_OK)
    {
        

        FM_SET_FIELD(laneSignalDetectCfg, FM10000_LANE_SIGNAL_DETECT_CFG, SdOverride, 0);

        err = switchPtr->WriteUINT32( sw, laneSignalDetectCfgAddr, laneSignalDetectCfg);
    }

    return err;

}   /* end fm10000SerDesSetSignalDtctNormal */




/*****************************************************************************/
/** fm10000SerDesConfigureBitRateAndWidthMode
 * \ingroup intSerdes
 *
 * \desc            Action configuring the SerDes Tx and Rx bit rate and width
 *                  mode according to the current ethernet mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureBitRateAndWidthMode( fm_smEventInfo *eventInfo,
                                                     void           *userInfo )
{
    fm_status           err;
    fm_int              sw;
    fm_int              serDes;
    fm10000_lane       *pLaneExt;
    fm10000_port       *portExt;


    FM_NOT_USED(eventInfo);

    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    portExt  = pLaneExt->parentPortExt;

    
    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_TX_ENA_MASK      |
                                   FM10000_SERDES_CTRL_RX_ENA_MASK      |
                                   FM10000_SERDES_CTRL_OUTPUT_ENA_MASK  |
                                   0);


    
    if (err == FM_OK)
    {
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                        "Configure Serdes %d, bitRate=%d, widthMode=%d, rateSel=%d\n",
                        serDes,
                        pLaneExt->bitRate,
                        pLaneExt->widthMode,
                        pLaneExt->bitRate);

        err = fm10000SerdesSetBitRate(sw,serDes,pLaneExt->rateSel);

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Cannot set Bit Rate on serDes 0x%2.2x\n",
                             serDes );
        }
        else
        {
            
            err = fm10000SetPcslCfgWidthMode(sw, serDes, pLaneExt->widthMode);

            if ( err != FM_OK )
            {
                FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                                 serDes,
                                 "Cannot set PCSl width mode on serDes 0x%2.2x\n",
                                 serDes );
            }
        }
    }
    else
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                         serDes,
                         "Cannot disable serDes 0x%2.2x\n",
                         serDes );
    }


    return err;

}   /* end fm10000SerDesConfigureBitRateAndWidthMode */




/*****************************************************************************/
/** fm10000SerDesConfigureWidthMode
 * \ingroup intSerdes
 *
 * \desc            Action configuring the SerDes Tx and Rx width
 *                  mode according to the current ethernet mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureWidthMode( fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_status           err;
    fm_int              sw;
    fm_int              serDes;
    fm_serdesWidthMode  widthMode;
    fm10000_lane       *laneExt;


    FM_NOT_USED(eventInfo);

    sw      = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = laneExt->serDes;

    err = fm10000GetSerdesWidthModeRateSel(serDes, laneExt->bitRate, &widthMode, NULL);

    
    if (err == FM_OK)
    {
        
        FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES,serDes,
                        "Configure Serdes %d, widthMode=%d\n",
                        serDes,
                        widthMode);
 
        err = fm10000SerdesSetWidthMode(sw,serDes,widthMode);

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Cannot set Width Mode on serDes 0x%2.2x\n",
                             serDes );
        }
    }

    return err;

}   /* end fm10000SerDesConfigureBitRateAndWidthMode */




/*****************************************************************************/
/** fm10000SerDesConfigureDataSelect
 * \ingroup intSerdes
 *
 * \desc            Action marking the TX PLL down for a lane in the context
 *                  state machine associated to the parent port of a given lane
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureDataSelect( fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status   err;
    fm_int      sw;
    fm_int      serDes;


    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    
    err = fm10000SerdesSetTxDataSelect(sw,
                                       serDes,
                                       FM10000_SERDES_TX_DATA_SEL_CORE);
    if ( err != FM_OK )
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                         serDes,
                         "Cannot configure Tx Data Select on serDes 0x%2.2x\n",
                         serDes );
    }

    return err;

}   /* end fm10000SerDesConfigureDataSelect */




/*****************************************************************************/
/** fm10000SerDesConfigureOptions
 * \ingroup intSerdes
 *
 * \desc            Action configuring several SerDes options according to the
 *                  current SerDes attributes. These options include the Rx
 *                  termination, forced Tx phase calibration, etc.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureOptions( fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SerdesSetPllCalibrationMode(sw, serDes);

    if (err == FM_OK)
    {
        
        err = fm10000SerdesConfigurePhaseSlip(sw,serDes);
    }

    if (err == FM_OK)
    {
        
        err = fm10000SerdesSetRxTerm(sw, serDes, pLaneExt->rxTermination);
    }

    return err;

}   /* end fm10000SerDesConfigureOptions */




/*****************************************************************************/
/** fm10000SerDesDisableLanePolarity
 * \ingroup intSerdes
 *
 * \desc            Action disabling lane polarity changes in both Tx and Rx.
 *                  This is required before enabling near loopback.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableLanePolarity( fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status   err;
    fm_int      sw;
    fm_int      serDes;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SetSerdesLanePolarity(sw, serDes, FALSE, FALSE);

    return err;

}   /* end fm10000SerDesDisableLanePolarity */



/*****************************************************************************/
/** fm10000SerDesConfigLanePolarity
 * \ingroup intSerdes
 *
 * \desc            Action setting the lane polarity according to the values
 *                  set in laneAttr structure.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigLanePolarity(fm_smEventInfo *eventInfo,
                                          void           *userInfo )
{
    fm_status    err;
    fm_int       sw;
    fm_int       serDes;
    fm_laneAttr *laneAttr;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;
    laneAttr = ((fm10000_serDesSmEventInfo *)userInfo)->laneAttr;

    err = fm10000SetSerdesLanePolarity(sw, 
                                       serDes, 
                                       (laneAttr->txPolarity != 0),
                                       (laneAttr->rxPolarity != 0));
    if ( err != FM_OK )
    {
        FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                         serDes,
                         "Cannot set lane polarity on serDes 0x%2.2x\n",
                         serDes );
    }

    return err;

}   /* end fm10000SerDesConfigLanePolarity */



/*****************************************************************************/
/** fm10000SerDesConfigureTxEqualization
 * \ingroup intSerdes
 *
 * \desc            Action configuring Tx equalization: cursor, pre-cursor
 *                  and post-cursor. The set of values used to configure the Tx
 *                  equalization depends on the ethernet mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureTxEqualization( fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm_int          preCursor;
    fm_int          cursor;
    fm_int          postCursor;
    fm10000_lane   *pLaneExt;
    fm_laneAttr    *pLaneAttr;


    FM_NOT_USED(eventInfo);

    err = FM_OK;
    sw  = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneAttr = ((fm10000_serDesSmEventInfo *)userInfo)->laneAttr;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;

    
     

     
    if (pLaneExt->dfeMode != FM_DFE_MODE_KR)
    {
        
        preCursor  = pLaneAttr->preCursor;
        cursor     = pLaneAttr->cursor;
        postCursor = pLaneAttr->postCursor;
    }
    else
    {
        
        preCursor  = 0;
        cursor     = 0;
        postCursor = 0;
    }

    
    err = fm10000SetSerdesCursor(sw, serDes, cursor);

    if (err == FM_OK)
    {
        err = fm10000SetSerdesPreCursor(sw, serDes, preCursor);

        if (err == FM_OK)
        {
            err = fm10000SetSerdesPostCursor(sw, serDes, postCursor);
        }
    }

    return err;

}   /* end fm10000SerDesConfigureTxEqualization */




/*****************************************************************************/
/** fm10000SerDesConfigureDfeMode
 * \ingroup intSerdes
 *
 * \desc            Action configuring the SerDes according to the DFE mode.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureDfeMode( fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_status    err;


    FM_NOT_USED(eventInfo);
    FM_NOT_USED(userInfo);


    err = FM_OK;

    
     

    return err;

}   /* end fm10000SerDesConfigureDfeMode */




/*****************************************************************************/
/** fm10000SerDesEnableInterrupts
 * \ingroup intSerdes
 *
 * \desc            Action enabling SerDes interrupts: RxRdy, TxRdy and
 *                  signalOk.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableInterrupts( fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_switch *     switchPtr;
    fm_int          serDes;
    fm10000_lane *  pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    
    err = fm10000SerdesEnableSerDesInterrupts(sw, serDes);

    if (err == FM_OK)
    {
        pLaneExt->serdesInterruptMask = FM10000_SERDES_PWRUP_INTR_MASK;

        
        TAKE_REG_LOCK( sw );

        err = switchPtr->WriteUINT32( sw,
                                      FM10000_SERDES_IP(pLaneExt->epl, pLaneExt->physLane),
                                      ~0);
        if (err == FM_OK)
        {

            err = switchPtr->WriteUINT32( sw,
                                          FM10000_SERDES_IM(pLaneExt->epl, pLaneExt->physLane),
                                         ~pLaneExt->serdesInterruptMask );
        }
        DROP_REG_LOCK( sw );
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                     "Cannot get SBus address of SerDes 0x%2.2x\n",
                     serDes);
    }

    return err;

}   /* end fm10000SerDesEnableInterrupts */




/*****************************************************************************/
/** fm10000SerDesDisableInterrupts
 * \ingroup intSerdes
 *
 * \desc            Action disabling ALL SerDes interrupts for the specified
 *                  serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableInterrupts( fm_smEventInfo *eventInfo,
                                          void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_switch *     switchPtr;
    fm10000_lane *  pLaneExt;
    fm_int          serDes;

    FM_NOT_USED(eventInfo);


    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    
    TAKE_REG_LOCK( sw );
    pLaneExt->serdesInterruptMask = 0;

    err = switchPtr->WriteUINT32( sw,
                                  FM10000_SERDES_IM(pLaneExt->epl, pLaneExt->physLane),
                                  ~pLaneExt->serdesInterruptMask);
    if (err == FM_OK)
    {
        err = switchPtr->WriteUINT32( sw,
                                      FM10000_SERDES_IP(pLaneExt->epl, pLaneExt->physLane),
                                      ~0);
    }

    DROP_REG_LOCK( sw );

    return err;

}   /* end fm10000SerDesDisableInterrupts */




/*****************************************************************************/
/** fm10000SerDesResetSpico
 * \ingroup intSerdes
 *
 * \desc            Action resetting the SerDes SPICO processor.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesResetSpico( fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;

    FM_NOT_USED(eventInfo);


    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SerdesResetSpico(sw,serDes);

    return err;

}   /* end fm10000SerDesResetSpico */




/*****************************************************************************/
/** fm10000SerDesRestoreSpico
 * \ingroup intSerdes
 *
 * \desc            Action resetting and restoring, if necessary,  the SerDes
 *                  SPICO processor.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesRestoreSpico( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane *  pLaneExt;

    FM_NOT_USED(eventInfo);


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes   = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;


    if ( pLaneExt->prevBitRate == FM10000_LANE_BITRATE_2500MBPS &&
         pLaneExt->bitRate     != FM10000_LANE_BITRATE_2500MBPS &&
         pLaneExt->dfeMode     == FM_DFE_MODE_STATIC)
    {
        err = FM_OK;
    }
    else
    {
        err = fm10000SerdesSpicoSetup(sw,serDes);
    }

    return err;

}   /* end fm10000SerDesRestoreSpico */



/*****************************************************************************/
/** fm10000SerDesEnableNearLoopback
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes near loopback.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableNearLoopback( fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_status   err;
    fm_int      sw;
    fm_int      serDes;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_INTERNAL_ON);

    return err;

}   /* end fm10000SerDesEnableNearLoopback */




/*****************************************************************************/
/** fm10000SerDesDisableNearLoopback
 * \ingroup intSerdes
 *
 * \desc            Action disabling the near loopback.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableNearLoopback( fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status   err;
    fm_int      sw;
    fm_int      serDes;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_INTERNAL_OFF);


    return err;

}   /* end fm10000SerDesDisableNearLoopback */



/*****************************************************************************/
/** fm10000SerDesSaveNearLoopbackOnConfig
 * \ingroup intSerdes
 *
 * \desc            Action saving near loopback ON configuration.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSaveNearLoopbackOnConfig(fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    fm_status     err;
    fm_int        newDfeMode;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    err = FM_OK;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    newDfeMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.dfeMode;

    
    if (newDfeMode < 0 || newDfeMode > FM_DFE_MODE_ICAL_ONLY )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pLaneExt->nearLoopbackEn = TRUE;
    }

    return err;

}   /* end fm10000SerDesSaveNearLoopbackOnConfig */




/*****************************************************************************/
/** fm10000SerDesSaveNearLoopbackOffConfig
 * \ingroup intSerdes
 *
 * \desc            Action saving near loopback OFF configuration.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSaveNearLoopbackOffConfig(fm_smEventInfo *eventInfo,
                                                 void           *userInfo )
{
    fm_status     err;
    fm_int        newDfeMode;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    err = FM_OK;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    newDfeMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.dfeMode;

    
    if (newDfeMode < 0 || newDfeMode > FM_DFE_MODE_ICAL_ONLY )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pLaneExt->nearLoopbackEn = FALSE;
    }

    return err;

}   /* end fm10000SerDesSaveNearLoopbackOffConfig */




/*****************************************************************************/
/** fm10000SerDesEnableParallelLoopback
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes parallel near loopback.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableParallelLoopback(fm_smEventInfo *eventInfo,
                                              void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    if (pLaneExt->farLoopbackStatus & 0x01)
    {
        err = fm10000SerdesSetLoopbackMode(sw,
                                           serDes,
                                           FM10000_SERDES_LB_PARALLEL_ON_RX_CLK);

        if (err == FM_OK)
        {
            pLaneExt->farLoopbackStatus |= 0x02;

            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes,
                            "Enabled parallel loopback on serdes %d\n",
                            serDes);
        }
    }

    return err;

}   /* end fm10000SerDesEnableParallelLoopback */




/*****************************************************************************/
/** fm10000SerDesDisableParallelLoopback
 * \ingroup intSerdes
 *
 * \desc            Action disabling the parallel loopback.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableParallelLoopback(fm_smEventInfo *eventInfo,
                                               void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;

    FM_NOT_USED(eventInfo);

    err = FM_OK;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    if ((pLaneExt->farLoopbackStatus & 0x03) == 0x03)
    {
        err = fm10000SerdesSetLoopbackMode(sw, serDes, FM10000_SERDES_LB_INTERNAL_OFF);
        if (err == FM_OK)
        {
            pLaneExt->farLoopbackStatus &= 0xfd;

            FM_LOG_DEBUG_V2(FM_LOG_CAT_SERDES, serDes,
                            "Disabled parallel loopback on serdes %d\n",
                            serDes);
        }
    }

    return err;

}   /* end fm10000SerDesDisableParallelLoopback */




/*****************************************************************************/
/** fm10000SerDesEnableBistMode
 * \ingroup intSerdes
 *
 * \desc            Action enabling Bist on this SerDes. This action can only
 *                  be called from MISSION state and supports all submodes:
 *                  TxRx, Tx and Rx. Error counter is reset to zero.
 *
 * \param[in]       eventInfo is a pointer to the generic event descriptor.
 *
 * \param[in]       userInfo pointer to a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableBistMode(fm_smEventInfo *eventInfo,
                                      void           *userInfo )
{
    fm_status     err;
    fm_status     locErr;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_int        bistSubMode;
    fm_bool       configTxBist;
    fm_bool       configRxBist;


    FM_NOT_USED(eventInfo);

    err = FM_OK;
    configTxBist = FALSE;
    configRxBist = FALSE;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;
    bistSubMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.bistSubmode;

    switch (bistSubMode)
    {
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TX_HIGHFREQ:
        case FM_BIST_TX_MIXEDFREQ:
        case FM_BIST_TX_SQUARE8:
        case FM_BIST_TX_SQUARE10:
        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TX_CUSTOM80:
        {
            configTxBist = TRUE;
            break;
        }
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        {
            configRxBist = TRUE;
            break;
        }
        case FM_BIST_TXRX_PRBS_128:
        case FM_BIST_TXRX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_2G:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        {
            configTxBist = TRUE;
            configRxBist = TRUE;
            break;
        }
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TX_CJPAT:
        case FM_BIST_RX_CJPAT:
        case FM_BIST_TXRX_CJPAT:
        {
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Unsupported BIST submode: %d\n", bistSubMode );
            err = FM_ERR_UNSUPPORTED;
            break;
        }
        default:
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Invalid BIST submode: %d\n", bistSubMode );
            err = FM_ERR_INVALID_SUBMODE;
    }

    if (err == FM_OK && configTxBist == TRUE)
    {
        
        if ((pLaneExt->farLoopbackStatus & 0x01) == 0)
        {
            
            if ( bistSubMode != pLaneExt->bistTxSubMode )
            {
                err = fm10000SetSerdesTxPattern(sw,
                                                serDes,
                                                bistSubMode,
                                                pLaneExt->bistCustomData0,
                                                pLaneExt->bistCustomData1);
                if ( err == FM_OK )
                {
                    pLaneExt->bistTxSubMode = bistSubMode;
                }
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Incompatible test mode\n");
            
             
            err = FM_ERR_INVALID_SUBMODE;
        }
    }

    if (err == FM_OK && configRxBist == TRUE)
    {
        
         


        err = SendDfeEventReq(eventInfo,
                              userInfo,
                              FM10000_SERDES_DFE_EVENT_PAUSE_TUNING_REQ);

        if (err == FM_OK)
        {
            
             
            err = fm10000SetSerdesRxPattern(sw,
                                            serDes,
                                            bistSubMode,
                                            pLaneExt->bistCustomData0,
                                            pLaneExt->bistCustomData1);

            if (err == FM_OK)
            {
                err = fm10000ResetSerdesErrorCounter(sw, serDes);
                pLaneExt->bistRxSubMode = bistSubMode;
            }
        }
    }

    if (err == FM_OK)
    {
        pLaneExt->bistActive  = TRUE;
    }
    else
    {
        
         

        if ((pLaneExt->farLoopbackStatus & 0x01) == 0)
        {
            fm10000SerdesSetTxDataSelect(sw,
                                         serDes,
                                         FM10000_SERDES_TX_DATA_SEL_CORE);
            fm10000SerdesSetRxCmpData(sw,
                                      serDes,
                                      FM10000_SERDES_RX_CMP_DATA_OFF);
        }
        else
        {
            
            locErr = fm10000SerDesEnableParallelLoopback(eventInfo, userInfo);
            if (locErr != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                             "Cannot restore parallel loopback on serdes=%d\n",
                              serDes);
            }
        }
    }

    return err;

}   /* end fm10000SerDesEnableBistMode */




/*****************************************************************************/
/** fm10000SerDesEnableTxBistMode
 * \ingroup intSerdes
 *
 * \desc            Action enabling Bist on the Tx section of this SerDes.
 *                  Only Tx and TxRx modes are supported. In the case of the
 *                  latter, only the Tx section is configured.
 *
 * \param[in]       eventInfo is a pointer to the generic event descriptor.
 *
 * \param[in]       userInfo pointer to a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableTxBistMode(fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_int        bistSubMode;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;
    bistSubMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.bistSubmode;

    switch (bistSubMode)
    {
        
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TX_HIGHFREQ:
        case FM_BIST_TX_MIXEDFREQ:
        case FM_BIST_TX_SQUARE8:
        case FM_BIST_TX_SQUARE10:
        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TX_CUSTOM80:
        
        case FM_BIST_TXRX_PRBS_128:
        case FM_BIST_TXRX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_2G:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        {
            
            if ((pLaneExt->farLoopbackStatus & 0x01) == 0)
            {
                
                if ( bistSubMode != pLaneExt->bistTxSubMode )
                {
                    err = fm10000SetSerdesTxPattern(sw,
                                                    serDes,
                                                    bistSubMode,
                                                    pLaneExt->bistCustomData0,
                                                    pLaneExt->bistCustomData1);
                    if ( err == FM_OK )
                    {
                        pLaneExt->bistTxSubMode = bistSubMode;
                        pLaneExt->bistActive = TRUE;
                    }
                }
            }
            else
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Incompatible test mode\n");
                
                 
                err = FM_ERR_INVALID_SUBMODE;
            }
            break;
        }
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TX_CJPAT:
        case FM_BIST_RX_CJPAT:
        case FM_BIST_TXRX_CJPAT:
        {
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Unsupported BIST submode: %d\n", bistSubMode );
            err = FM_ERR_UNSUPPORTED;
            break;
        }
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        default:
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Invalid BIST submode: %d\n", bistSubMode );
            err = FM_ERR_INVALID_SUBMODE;
    }

    return err;

}   /* end fm10000SerDesEnableTxBistMode */




/*****************************************************************************/
/** fm10000SerDesEnableRxBistMode
 * \ingroup intSerdes
 *
 * \desc            Action enabling Bist on the Rx section of this SerDes.
 *                  Only Rx and TxRx modes are supported. In the case of the
 *                  latter, only the Rx section is configured. Bist error
 *                  counter is reset to zero.
 *
 * \param[in]       eventInfo is a pointer to the generic event descriptor.
 *
 * \param[in]       userInfo pointer to a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableRxBistMode(fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_int        bistSubMode;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;
    bistSubMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.bistSubmode;

    err = EnableRxBistMode(eventInfo, userInfo, bistSubMode);

    if ( err == FM_ERR_UNSUPPORTED )
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                                "Unsupported BIST submode: %d\n", bistSubMode );
    }
    else if ( err == FM_ERR_INVALID_SUBMODE )
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Invalid BIST submode: %d\n", bistSubMode );
    }
    else 
    {
        pLaneExt->bistRxSubMode = bistSubMode;
        pLaneExt->bistActive = TRUE;
    }

    return err;

}   /* end fm10000SerDesEnableRxBistMode */




/*****************************************************************************/
/** fm10000SerDesRemoveBistConfig
 * \ingroup intSerdes
 *
 * \desc            Action removing Bist configuration on serdes control
 *                  structures.
 *
 * \param[in]       eventInfo is a pointer to the generic event descriptor.
 *
 * \param[in]       userInfo pointer to a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesRemoveBistConfig(fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;


    FM_NOT_USED(eventInfo);

    err = FM_OK;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;

    pLaneExt->bistTxSubMode = FM_BIST_MAX;
    pLaneExt->bistRxSubMode = FM_BIST_MAX;
    pLaneExt->bistActive    = FALSE;

    err = fm10000SerdesSetTxDataSelect(sw,
                                       serDes,
                                       FM10000_SERDES_TX_DATA_SEL_CORE);
    if ( err == FM_OK )
    {
        err = fm10000SerdesSetRxCmpData(sw,
                                        serDes,
                                        FM10000_SERDES_RX_CMP_DATA_OFF);
    }

    return err;

}   /* end fm10000SerDesRemoveBistConfig */




/*****************************************************************************/
/** fm10000SerDesInitSignalOk
 * \ingroup intSerdes
 *
 * \desc            Action initializing the SerDes signalOK.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesInitSignalOk( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status   err;
    fm_int      sw;
    fm_int      serDes;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    serDes  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt->serDes;

    err = fm10000SerdesInitSignalOk(sw, serDes, 0);

    return err;

}   /* end fm10000SerDesInitSignalOk */




/*****************************************************************************/
/** fm10000SerDesDisableTxRx
 * \ingroup intSerdes
 *
 * \desc            Action disabling both Tx and Rx PLLs of the current SerDes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableTxRx( fm_smEventInfo *eventInfo,
                                    void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    
    if (pLaneExt->farLoopbackStatus & 0x02)
    {
        err = fm10000SerDesDisableParallelLoopback(eventInfo,userInfo);
    }

    if (err == FM_OK)
    {
        err = fm10000SerdesDisable(sw,serDes);
    }

    pLaneExt->krExt.eyeScoreHeight = 0xffff;

    return err;

}   /* end fm10000SerDesDisableTxRx */




/*****************************************************************************/
/** fm10000SerDesEnableRx
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes Rx PLL. When this PLL is
 *                  disabled, the Rx section of the SerDes remains inactive
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableRx( fm_smEventInfo *eventInfo,
                                 void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_RX_ENA_MASK | FM10000_SERDES_CTRL_RX_ENA);

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Cannot Enable Rx on SerDes=%d\n",
                        serDes);
    }

    return err;

}   /* end fm10000SerDesEnableRx */




/*****************************************************************************/
/** fm10000SerDesEnableTxRx
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes Tx and Rx PLLs. 
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableTxRx( fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;


    fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_TX_ENA_MASK | 
                                   FM10000_SERDES_CTRL_TX_ENA      |
                                   FM10000_SERDES_CTRL_RX_ENA_MASK | 
                                   FM10000_SERDES_CTRL_RX_ENA);

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Cannot Enable Tx and Rx on SerDes=%d\n",
                        serDes);
    }

    return err;

}   /* end fm10000SerDesEnableTxRx */




/*****************************************************************************/
/** fm10000SerDesDisableRx
 * \ingroup intSerdes
 *
 * \desc            Action disabling the SerDes Rx PLL. When this PLL is
 *                  disabled the Rx section of the SerDes remains inactive
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableRx( fm_smEventInfo *eventInfo,
                                  void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    
    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_RX_ENA_MASK | 0);

    return err;

}   /* end fm10000SerDesDisableRx */




/*****************************************************************************/
/** fm10000SerDesEnableTx
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes Tx PLL. When this PLL is
 *                  disabled, the Tx section of the SerDes remains inactive.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableTx( fm_smEventInfo *eventInfo,
                                 void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_TX_ENA_MASK | FM10000_SERDES_CTRL_TX_ENA);

    if (err != FM_OK)
    {
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                        "Cannot Enable Tx on SerDes=%d\n",
                        serDes);
    }

    return err;

}   /* end fm10000SerDesEnableTx */




/*****************************************************************************/
/** fm10000SerDesDisableTx
 * \ingroup intSerdes
 *
 * \desc            Action disabling the SerDes Tx PLL and output enable for
 *                  the specified serdes. When Tx PLL is disabled, the serdes
 *                  Tx section remains inactive.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableTx( fm_smEventInfo *eventInfo,
                                  void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_TX_ENA_MASK      |
                                   FM10000_SERDES_CTRL_OUTPUT_ENA_MASK  |
                                   0);

    return err;

}   /* end fm10000SerDesDisableTx */




/*****************************************************************************/
/** fm10000SerDesEnableTxOutput
 * \ingroup intSerdes
 *
 * \desc            Action enabling the SerDes Tx output. TX_EN bit must be
 *                  already set before enabling the output.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 * *****************************************************************************/
fm_status fm10000SerDesEnableTxOutput( fm_smEventInfo *eventInfo,
                                       void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_uint32     serDesEnable;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    
    serDesEnable = pLaneExt->serDesEnableCache;

    if (serDesEnable & FM10000_SERDES_CTRL_TX_ENA)
    {
        err = fm10000SerdesTxRxEnaCtrl(sw,
                                       serDes,
                                       FM10000_SERDES_CTRL_OUTPUT_ENA_MASK  |
                                       FM10000_SERDES_CTRL_OUTPUT_ENA);

    }
    else
    {
        err = FM_FAIL;
        FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                         serDes,
                         "Cannot enable serdes output if Tx section is not active. "
                         "Serdes=0x%2.2x, serdesEnable=%8.8x\n",
                         serDes,
                         serDesEnable);
    }

    return err;

}   /* end fm10000SerDesEnableTxOutput */




/*****************************************************************************/
/** fm10000SerDesDisableTxOutput
 * \ingroup intSerdes
 *
 * \desc            Action disabling the SerDes Tx output.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableTxOutput( fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = fm10000SerdesTxRxEnaCtrl(sw,
                                   serDes,
                                   FM10000_SERDES_CTRL_OUTPUT_ENA_MASK  | 0);

    return err;


}   /* end fm10000SerDesDisableTxOutput */




/*****************************************************************************/
/** fm10000SerDesEnableEeeOpMode
 * \ingroup intSerdes
 *
 * \desc            Action enabling EEE mode support at serdes level.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesEnableEeeOpMode( fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_int        sw;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    pLaneExt->eeeModeActive = 0x01;


    return FM_OK;


}   /* end fm10000SerDesEnableEeeOpMode */




/*****************************************************************************/
/** fm10000SerDesDisableEeeOpMode
 * \ingroup intSerdes
 *
 * \desc            Action disabling EEE mode support at serdes level.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDisableEeeOpMode( fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    fm_int        sw;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;

    pLaneExt->eeeModeActive = 0x00;


    return FM_OK;


}   /* end fm10000SerDesDisableEeeOpMode */




/*****************************************************************************/
/** fm10000SerDesConfigureEee
 * \ingroup intSerdes
 *
 * \desc            Enable EEE support at serdes level.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureEee( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_int        sw;
    fm10000_lane *pLaneExt;
    fm_int        serDes;
    fm_status     err=FM_OK;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes  = pLaneExt->serDes;


 
 
    err = fm10000SerdesConfigureEeeInt(sw,serDes);

    return err;


}   /* end fm10000SerDesConfigureEee */




/*****************************************************************************/
/** fm10000SerDesDumpBitRate
 * \ingroup intSerdes
 *
 * \desc            SerDes-level state machine actions that simply displays
 *                  a log message to record the bit rate required by the
 *                  port-level state machine upon a request to configure the
 *                  serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SerDesDumpBitRate(fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_int               serDes;
    fm10000_lane        *laneExt;
    fm10000_laneBitrate  bitRate;
    fm_text              bitRateString;

    FM_NOT_USED(eventInfo);

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    bitRate = ((fm10000_serDesSmEventInfo *)userInfo)->info.bitRate;
    serDes  = laneExt->serDes;

    if ( bitRate >=0 && bitRate < FM10000_LANE_BITRATE_MAX )
    {
        bitRateString = fm10000LaneBitRatesMap[bitRate];
    }
    else
    {
        bitRateString = "Invalid";
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_SERDES,
                     serDes,
                     "Bit rate requested on serDes %d: %s\n",
                     serDes,
                     bitRateString );

    return FM_OK;

}   /* end fm10000SerDesDumpBitRate */




/*****************************************************************************/
/** fm10000SerDesStartKrTraining
 * \ingroup intSerdes
 *
 * \desc            Action starting KR training.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStartKrTraining( fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;
    fm10000_switch *switchExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    switchExt = GET_SWITCH_EXT(sw);

    if ( switchExt->serdesSupportsKR )
    {
        err = fm10000StartKrTraining(sw, serDes);
    }
    else
    {
        err = FM_FAIL;
    }

    return err;

}   /* end fm10000SerDesStartKrTraining */




/*****************************************************************************/
/** fm10000SerDesStopKrTraining
 * \ingroup intSerdes
 *
 * \desc            Action stopping the KR training on this SerDes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesStopKrTraining(fm_smEventInfo *eventInfo,
                                      void           *userInfo )
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;
    fm10000_switch *switchExt;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    switchExt = GET_SWITCH_EXT(sw);

    if ( pLaneExt->dfeMode == FM_DFE_MODE_KR && switchExt->serdesSupportsKR )
    {
        
        err = fm10000StopKrTraining(sw, serDes, FALSE);
        pLaneExt->krExt.eyeScoreHeight = 0xffff;
    }
    else
    {
        
        err = FM_OK;
    }

    return err;

}   /* end fm10000SerDesStopKrTraining */




/*****************************************************************************/
/** fm10000SerDesSendDfeStartTuningReq
 * \ingroup intSerdes
 *
 * \desc            Action sending a start-DFE tuning request to the dfe
 *                  state machine associated with the current serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendDfeStartTuningReq(fm_smEventInfo *eventInfo,
                                             void           *userInfo )
{

    return SendDfeEventReq(eventInfo,
                           userInfo,
                           FM10000_SERDES_DFE_EVENT_START_TUNING_REQ);

}   /* end fm10000SerDesSendDfeStartTuningReq */




/*****************************************************************************/
/** fm10000SerDesSendDfeStopTuningReq
 * \ingroup intSerdes
 *
 * \desc            Action sending a stop-DFE tuning request to the dfe
 *                  state machine associated with the current serdes. This
 *                  action is only performed is the current DFE mode is
 *                  ''FM_DFE_MODE_ONE_SHOT'', ''FM_DFE_MODE_CONTINUOUS'' or
 *                  ''FM_DFE_MODE_ICAL_ONLY''
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendDfeStopTuningReq(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status       err;
    fm10000_lane   *pLaneExt;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    err = FM_OK;

    if (pLaneExt->dfeMode == FM_DFE_MODE_ONE_SHOT   ||
        pLaneExt->dfeMode == FM_DFE_MODE_CONTINUOUS ||
        pLaneExt->dfeMode == FM_DFE_MODE_ICAL_ONLY  ||
        pLaneExt->dfeMode == FM_DFE_MODE_KR)
    {
        err = SendDfeEventReq(eventInfo,
                              userInfo,
                              FM10000_SERDES_DFE_EVENT_STOP_TUNING_REQ);
    }

    return err;

}   /* end fm10000SerDesSendDfeStopTuningReq */




/*****************************************************************************/
/** fm10000SerDesSendDfeSuspendTuningReq
 * \ingroup intSerdes
 *
 * \desc            Action sending a stop-DFE tuning request to the dfe
 *                  state machine associated with the current serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendDfeSuspendTuningReq(fm_smEventInfo *eventInfo,
                                               void           *userInfo )
{

    return SendDfeEventReq(eventInfo,
                           userInfo,
                           FM10000_SERDES_DFE_EVENT_PAUSE_TUNING_REQ);

}   /* end fm10000SerDesSendDfeSuspendTuningReq */




/*****************************************************************************/
/** fm10000SerDesSendDfeResumeTuningReq
 * \ingroup intSerdes
 *
 * \desc            Action sending a stop-DFE tuning request to the dfe
 *                  state machine associated with the current serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendDfeResumeTuningReq(fm_smEventInfo *eventInfo,
                                              void           *userInfo )
{

    return SendDfeEventReq(eventInfo,
                           userInfo,
                           FM10000_SERDES_DFE_EVENT_RESUME_TUNING_REQ);

}   /* end fm10000SerDesSendDfeResumeTuningReq */




/*****************************************************************************/
/** fm10000SerDesSendKrStartPcalReq
 * \ingroup intSerdes
 *
 * \desc            Action sending a start-DFE pCal request to the dfe
 *                  state machine associated with the current serdes.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSendKrStartPcalReq(fm_smEventInfo *eventInfo,
                                          void           *userInfo )
{
    fm_status       err;
    fm10000_lane   *pLaneExt;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    err = FM_OK;

    if (pLaneExt->krExt.pCalMode == FM10000_SERDES_KR_PCAL_MODE_ONE_SHOT   ||
        pLaneExt->krExt.pCalMode == FM10000_SERDES_KR_PCAL_MODE_CONTINUOUS)
    {
        err =  SendDfeEventReq(eventInfo,
                               userInfo,
                               FM10000_SERDES_DFE_EVENT_START_PCAL_REQ);
    }

    return err;

}   /* end fm10000SerDesSendKrStartPcalReq */




/*****************************************************************************/
/** fm10000SerDesDontSaveTransitionRecord
 * \ingroup intSerdesDfe
 *
 * \desc            Action setting the flag to do not save record for the
 *                  current transaction. This tipically is used with timeouts
 *                  and periodically events to avoid saving redundant information
 *                  and sweep the state machine history buffer.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDontSaveTransitionRecord(fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    FM_NOT_USED(userInfo);

    eventInfo->dontSaveRecord = TRUE;

    return FM_OK;

}   /* end fm10000SerDesDontSaveTransitionRecord */




/*****************************************************************************/
/** fm10000SerDesSaveTransitionRecord
 * \ingroup intSerdesDfe
 *
 * \desc            Action setting to FALSE the flag to do not save record for
 *                  the current transaction. The default state for that flag
 *                  is FALSE so normally calling this function is not necessary.
 *                  This function may be called to cancel the action of a
 *                  previous call to fm10000SerDesDontSaveTransitionRecord.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesSaveTransitionRecord(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    FM_NOT_USED(userInfo);

    
    eventInfo->dontSaveRecord = FALSE;

    return FM_OK;

}   /* end fm10000SerDesSaveTransitionRecord */




/*****************************************************************************/
/** fm10000SerDesProcessRxTxPllLockEvents
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing PLL ready
 *                  events.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessRxTxPllLockEvents( fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         sw;
    fm_int         serDes;


    FM_NOT_USED(eventInfo);

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = FM_OK;

    switch (eventInfo->eventId)
    {
        case FM10000_SERDES_EVENT_RX_PLL_LOCKED_IND:
            pLaneExt->serDesPllStatus |= 0x02;
            break;
        case FM10000_SERDES_EVENT_TX_PLL_LOCKED_IND:
            pLaneExt->serDesPllStatus |= 0x01;
            break;
        case FM10000_SERDES_EVENT_RXTX_PLLS_LOCKED_IND:
            pLaneExt->serDesPllStatus |= 0x03;
            break;
        default:
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                         "Invalid event: %d\n",
                         eventInfo->eventId);
    }

    if (err == FM_OK && ((pLaneExt->serDesPllStatus & 0x03) == 0x03))
    {
        

        
        err = fm10000ConfigurePcslBitSlip(sw, serDes);

        if (err == FM_OK)
        {
            err = fm10000SerDesConfigureWidthMode(eventInfo, userInfo);
        }

        if (err == FM_OK)
        {
            
            err = fm10000SerDesEnableTxOutput(eventInfo,userInfo);
        }

        
        if (err == FM_OK)
        {
            err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);
        }

        if (err == FM_OK)
        {
            err = fm10000SerDesSendPortLaneReadyInd(eventInfo,userInfo);
        }

        if (err == FM_OK)
        {
            err = fm10000SerDesSetStaticDfeSignalDtctNormal(eventInfo, userInfo);
        }

        if ( err == FM_OK )
        {
            err = fm10000SerDesInitSignalOk(eventInfo,userInfo);
        }

        if (err == FM_OK)
        {
            if (!pLaneExt->nearLoopbackEn )
            {
                *nextState = FM10000_SERDES_STATE_POWERED_UP;
            }
            else
            {
                err = fm10000SerDesDisableLanePolarity(eventInfo,userInfo);

                if (err == FM_OK)
                {
                    err = fm10000SerDesEnableNearLoopback(eventInfo,userInfo);
                }
        
                if (err == FM_OK)
                {
                    err = fm10000SerDesDisableTxOutput(eventInfo,userInfo);
                }
        
                if (err == FM_OK)
                {
                    err = fm10000SerDesSetSignalDtctNormal(eventInfo,userInfo);
                }
                *nextState = FM10000_SERDES_STATE_LOOPBACK;
            }
            
            pLaneExt->serdesInterruptMask = FM10000_SERDES_OPSTATE_INTR_MASK;
        }
    }
    else
    {
        
        pLaneExt->serdesInterruptMask = FM10000_SERDES_PWRUP_INTR_MASK;
    }

    return err;

}   /* end fm10000SerDesProcessRxTxPllLockEvents */




/*****************************************************************************/
/** fm10000SerDesProcessRxTxPllLockTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing a timeout
 *                  while waiting for Tx and/or Rx Pll lock. PLL status is
 *                  verified once more time to consider the case of missed
 *                  interrupts. If any of the PLL is still not locked, this
 *                  function will retry enabling them.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessRxTxPllLockTimeout( fm_smEventInfo *eventInfo,
                                                  void           *userInfo,
                                                  fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         sw;
    fm_bool        txRdy;
    fm_bool        rxRdy;
    fm_switch *    switchPtr;


    FM_NOT_USED(eventInfo);

    
     


    sw        = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    if (pLaneExt->serDesEnableRetryCtrl == 0)
    {
        err = fm10000SerdesGetTxRxReadyStatus(sw, serDes, &txRdy, &rxRdy);

        if (err == FM_OK)
        {
            if (txRdy == TRUE && rxRdy == TRUE)
            {
                
                 

                
                pLaneExt->serDesPllStatus = 0x03;

                
                err = fm10000ConfigurePcslBitSlip(sw, serDes);

                if (err == FM_OK)
                {
                    err = fm10000SerDesConfigureWidthMode(eventInfo, userInfo);
                }

                if (err == FM_OK)
                {
                    
                    err = fm10000SerDesEnableTxOutput(eventInfo,userInfo);
                }

                
                if (err == FM_OK)
                {
                    err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);
                }

                if (err == FM_OK)
                {
                    err = fm10000SerDesSendPortLaneReadyInd( eventInfo, userInfo);
                }

                if (err == FM_OK)
                {
                    err = fm10000SerDesSetStaticDfeSignalDtctNormal(eventInfo, userInfo);
                }

                if ( err == FM_OK )
                {
                    err = fm10000SerDesInitSignalOk(eventInfo,userInfo);
                }
                
                if (err == FM_OK)
                {
                    if (!pLaneExt->nearLoopbackEn )
                    {
                        *nextState = FM10000_SERDES_STATE_POWERED_UP;
                    }
                    else
                    {
                        err = fm10000SerDesDisableLanePolarity(eventInfo,userInfo);

                        if (err == FM_OK)
                        {
                            err = fm10000SerDesEnableNearLoopback(eventInfo,userInfo);
                        }
                
                        if (err == FM_OK)
                        {
                            err = fm10000SerDesDisableTxOutput(eventInfo,userInfo);
                        }
                
                        if (err == FM_OK)
                        {
                            err = fm10000SerDesSetSignalDtctNormal(eventInfo,userInfo);
                        }
                        *nextState = FM10000_SERDES_STATE_LOOPBACK;
                    }

                    
                    pLaneExt->serdesInterruptMask = FM10000_SERDES_OPSTATE_INTR_MASK;

                    TAKE_REG_LOCK( sw );

                    err = switchPtr->WriteUINT32( sw, 
                                                  FM10000_SERDES_IP(pLaneExt->epl, pLaneExt->physLane),
                                                  ~0 );

                    if (err == FM_OK)
                    {
                        err = switchPtr->WriteUINT32( sw,
                                                      FM10000_SERDES_IM(pLaneExt->epl, pLaneExt->physLane),
                                                      ~pLaneExt->serdesInterruptMask);
                    }
                    
                    DROP_REG_LOCK( sw );
                }

            }
            else
            {
                
                 

                 

                 

                 

                 
                err = fm10000SerDesDisableTxRx(eventInfo,userInfo);

                pLaneExt->serDesEnableRetryCtrl = 1;

                if (err == FM_OK)
                {
                    err = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
                }
            }
        }
    }
    else
    {
        err = fm10000SerDesEnableTxRx(eventInfo,userInfo);

        fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);

        pLaneExt->serDesEnableRetryCtrl = 0;
    }

    if (err != FM_OK)
    {
        
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);

    }

    return err;

}   /* end fm10000SerDesProcessRxTxPllLockTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessRxPllLockTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing a timeout
 *                  while waiting for Rx Pll lock. The PLL status is
 *                  verified once more time to consider the case of missed
 *                  interrupts. If the PLL is still not locked, this function
 *                  will retry enabling it.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessRxPllLockTimeout(fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         sw;
    fm_bool        rxRdy;
    fm_switch *    switchPtr;


    FM_NOT_USED(eventInfo);

    

    sw        = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    err = fm10000SerdesGetTxRxReadyStatus(sw, serDes, NULL, &rxRdy);

    if (err == FM_OK)
    {
        if (rxRdy == TRUE)
        {
            
             
            pLaneExt->serDesPllStatus = 0x02;

                       
            err = fm10000ConfigurePcslBitSlip(sw, serDes);

            if (err == FM_OK)
            {
                err = fm10000SerDesSendPortLaneReadyInd( eventInfo, userInfo);
            }

            
            if (err == FM_OK)
            {
                err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);
            }

            if ( err == FM_OK )
            {
                err = fm10000SerDesInitSignalOk(eventInfo,userInfo);
            }
            
            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_RX_ON;

                
                pLaneExt->serdesInterruptMask = FM10000_SERDES_OPSTATE_INTR_MASK;

                TAKE_REG_LOCK( sw );
                err = switchPtr->WriteUINT32( sw,
                                              FM10000_SERDES_IP(pLaneExt->epl, pLaneExt->physLane),
                                             ~0 );

                if (err == FM_OK)
                {
                    err = switchPtr->WriteUINT32( sw,
                                                  FM10000_SERDES_IM(pLaneExt->epl, pLaneExt->physLane),
                                                 ~pLaneExt->serdesInterruptMask );
                }
                DROP_REG_LOCK( sw );
            }

        }
        else
        {
            
             

             

             

             

             
            err = fm10000SerDesDisableTxRx(eventInfo,userInfo);

            if (err == FM_OK)
            {
                err = fm10000SerDesEnableRx(eventInfo,userInfo);
            }

            if (err == FM_OK)
            {
                err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
            }
        }
    }

    if (err != FM_OK)
    {
        
         
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);

    }

    return err;

}   /* end fm10000SerDesProcessRxPllLockTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessTxPllLockTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing a timeout
 *                  while waiting for Tx Pll lock. The PLL status is
 *                  verified once more time to consider the case of missed
 *                  interrupts. If the PLL is still not locked, this function
 *                  will retry enabling it.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessTxPllLockTimeout(fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         sw;
    fm_bool        txRdy;
    fm_switch *    switchPtr;


    FM_NOT_USED(eventInfo);

    

    sw        = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    err = fm10000SerdesGetTxRxReadyStatus(sw, serDes, &txRdy, NULL);

    if (err == FM_OK)
    {
        if (txRdy == TRUE)
        {
            
             
            pLaneExt->serDesPllStatus = 0x01;

            
            err = fm10000SerDesEnableTxOutput(eventInfo, userInfo);

            
            if (err == FM_OK)
            {
                err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);
            }

            if (err == FM_OK)
            {
                err = fm10000SerDesSendPortLaneReadyInd(eventInfo, userInfo);
            }

            
            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_TX_ON;

                
                pLaneExt->serdesInterruptMask = 0;

                TAKE_REG_LOCK( sw );
                err = switchPtr->WriteUINT32( sw,
                                              FM10000_SERDES_IM(pLaneExt->epl, pLaneExt->physLane),
                                              ~pLaneExt->serdesInterruptMask );

                if (err == FM_OK)
                {
                    err = switchPtr->WriteUINT32( sw,
                                                  FM10000_SERDES_IP(pLaneExt->epl, pLaneExt->physLane),
                                                  ~0);
                }
                DROP_REG_LOCK( sw );
            }

        }
        else
        {
            
             

             

             

             

             
            err = fm10000SerDesDisableTxRx(eventInfo,userInfo);

            if (err == FM_OK)
            {
                err = fm10000SerDesEnableTx(eventInfo,userInfo);
            }

            if (err == FM_OK)
            {
                err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
            }
        }
    }

    if (err != FM_OK)
    {
        
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);

    }

    return err;

}   /* end fm10000SerDesProcessTxPllLockTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkAsserted
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  assertion events.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkAsserted(fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane * pLaneExt;
    fm_int         serDes;
    fm_bool        restartTimer;

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = FM_OK;
    locErr = FM_OK;
    restartTimer = TRUE;

    switch (pLaneExt->dfeMode)
    {
        case FM_DFE_MODE_STATIC:
        {
            

            
             


            err = fm10000SerDesSetStaticDfeSignalDtctNormal(eventInfo,userInfo);

            if ( err == FM_OK )
            {
                err = fm10000SerDesEnableParallelLoopback(eventInfo,userInfo);
            }

            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_MISSION;
            }
            break;
        }
        case FM_DFE_MODE_ONE_SHOT:
        case FM_DFE_MODE_CONTINUOUS:
        case FM_DFE_MODE_ICAL_ONLY:
        {
            pLaneExt->signalOkDebounce = 1;

            err = fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
            restartTimer = FALSE;

            break;
        }
        case FM_DFE_MODE_KR:
        {
            err = fm10000SerDesStartKrTraining(eventInfo, userInfo );
            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_KR_TRAINING;
            }
            else if (pLaneExt->dbgLvl == FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
            {
                FM_LOG_PRINT("Error starting KR training on serdes %d\n", serDes);
            }
            break;
        }
        default:
            err = FM_FAIL;
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Invalid DFE mode on serdes=%d\n",
                            serDes);
            
    }
     

    if ( restartTimer )
    {
        locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }

    
    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessSignalOkAsserted */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkAssertedRx
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  assertion events when only Rx is enabled.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkAssertedRx(fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane * pLaneExt;
    fm_int         serDes;


    FM_NOT_USED(eventInfo);

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = FM_OK;

    switch (pLaneExt->dfeMode)
    {
        case FM_DFE_MODE_STATIC:
        {
            
            *nextState = FM10000_SERDES_STATE_MISSION;
            break;
        }
        case FM_DFE_MODE_ONE_SHOT:
        case FM_DFE_MODE_CONTINUOUS:
        case FM_DFE_MODE_ICAL_ONLY:
        {
            
             

            err = fm10000SerDesSendDfeStartTuningReq(eventInfo,userInfo);

            if (err != FM_OK)
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Cannot send start DFE request on serdes=%d\n",
                                serDes);
            }
            break;
        }
        case FM_DFE_MODE_KR:
        default:
        {
            
            err = FM_FAIL;
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Invalid DFE mode on serdes=%d\n",
                            serDes);
            
        }
    }

    
     
    locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);

    
    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessSignalOkAssertedRx */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkDeasserted
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  deassertion events.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkDeasserted(fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         curState;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = FM_OK;
    locErr = FM_OK;
    curState = *nextState;

    if (!pLaneExt->eeeModeActive)
    {

        switch (pLaneExt->dfeMode)
        {
            case FM_DFE_MODE_KR:
            {
                
                err = fm10000SerDesStopKrTraining(eventInfo, userInfo);

                if ( err == FM_OK && 
                     (pLaneExt->krExt.pCalMode == FM10000_SERDES_KR_PCAL_MODE_ONE_SHOT ||
                      pLaneExt->krExt.pCalMode == FM10000_SERDES_KR_PCAL_MODE_CONTINUOUS) )
                {
                    err = fm10000SerDesSendDfeStopTuningReq(eventInfo, userInfo);
                }

                if (err == FM_OK)
                {
                    *nextState = FM10000_SERDES_STATE_POWERED_UP;
                }
                break;
            }
            case FM_DFE_MODE_STATIC:
            {
                
                *nextState = FM10000_SERDES_STATE_POWERED_UP;
                break;
            }
            case FM_DFE_MODE_ONE_SHOT:
            case FM_DFE_MODE_CONTINUOUS:
            case FM_DFE_MODE_ICAL_ONLY:
            {
                err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);
    
                if ( err == FM_OK )
                {
                    err = fm10000SerDesSendDfeStopTuningReq(eventInfo,userInfo);
                }
    
                if ( err == FM_OK )
                {
                    *nextState = FM10000_SERDES_STATE_POWERED_UP;
                }
                else
                {
                    FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                    serDes,
                                    "Cannot change to POWERED_UP state on serdes=%d\n",
                                    serDes);
                }
                break;
            }
            default:
                err = FM_FAIL;
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Invalid DFE mode on serdes=%d\n",
                                serDes);

        }   /* end switch (pLaneExt->dfeMode) */

        locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }   
    else
    {
        err = fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);

        locErr = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
    }   /* end if (!pLaneExt->eeeModeActive) */

    
    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessSignalOkDeasserted */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkDeassertedRx
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  deassertion events when only Rx is enabled.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkDeassertedRx(fm_smEventInfo *eventInfo,
                                                   void           *userInfo,
                                                   fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane * pLaneExt;
    fm_int         serDes;


    FM_NOT_USED(eventInfo);

    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;

    err = FM_OK;

    switch (pLaneExt->dfeMode)
    {
        case FM_DFE_MODE_STATIC:
        {
            *nextState = FM10000_SERDES_STATE_RX_ON;
            break;
        }
        case FM_DFE_MODE_ONE_SHOT:
        case FM_DFE_MODE_CONTINUOUS:
        case FM_DFE_MODE_ICAL_ONLY:
        {
            err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

            if ( err == FM_OK )
            {
                err = fm10000SerDesSendDfeStopTuningReq(eventInfo,userInfo);
            }

            if ( err == FM_OK )
            {
                *nextState = FM10000_SERDES_STATE_RX_ON;
            }
            else
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Cannot change to RX_ON state on serdes=%d\n",
                                serDes);
            }
            break;
        }
        case FM_DFE_MODE_KR:
        default:
            err = FM_FAIL;
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Invalid DFE mode on serdes=%d\n",
                            serDes);
            
    }

    
     
    locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);

    
    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessSignalOkDeassertedRx */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk assertion detection. This is a signalOk
 *                  polling mechanism used used as a recovery when a signalOk
 *                  assertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkTimeout(fm_smEventInfo *eventInfo,
                                              void           *userInfo,
                                              fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        signalOk;
    fm_int         sw;
    fm_int         currentState;
    fm_switch *    switchPtr;
    fm_bool        debounceActive;


    pLaneExt  = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    sw        = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    switchPtr = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr;

    
    currentState = *nextState;
    debounceActive = FALSE;


    err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

    if (err == FM_OK && signalOk == TRUE)
    {
        switch (pLaneExt->dfeMode)
        {
            case FM_DFE_MODE_STATIC:
            {

                
                 


                err = fm10000SerDesSetStaticDfeSignalDtctNormal(eventInfo,userInfo);

                if ( err == FM_OK )
                {
                    err = fm10000SerDesEnableParallelLoopback(eventInfo,userInfo);
                }

                
                *nextState = FM10000_SERDES_STATE_MISSION;
                break;
            }
            case FM_DFE_MODE_ONE_SHOT:
            case FM_DFE_MODE_CONTINUOUS:
            case FM_DFE_MODE_ICAL_ONLY:
            {
                if ( pLaneExt->signalOkDebounce++ > FM10000_SERDES_SIGNALOK_DEBOUNCE_THRESHOLD )
                {
                    err = fm10000SerDesSendDfeStartTuningReq(eventInfo,userInfo);
                    if (err == FM_OK)
                    {
                        *nextState = FM10000_SERDES_STATE_DFE_TUNING;
                    }
                    else
                    {
                        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                        serDes,
                                        "Cannot send event to DFE sm on serdes=%d\n",
                                        serDes);
                    }
                }

                debounceActive = TRUE;
                break;
            }
            case FM_DFE_MODE_KR:
            {
                err = fm10000SerDesStartKrTraining(eventInfo, userInfo );
                if (err == FM_OK)
                {
                    *nextState = FM10000_SERDES_STATE_KR_TRAINING;
                }
                else if (pLaneExt->dbgLvl == FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
                {
                    FM_LOG_PRINT("Error starting KR training on serdes %d\n", serDes);
                }
                break;
            }
            default:
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Invalid DFE mode on serdes=%d\n",
                                serDes);
                
        }

        if ( currentState == *nextState )
        {
            fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
        }
    }
    else
    {
        if ( pLaneExt->signalOkDebounce )
        {
            debounceActive = TRUE;
        }

        pLaneExt->signalOkDebounce = 0;


        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Cannot read SignalOk status for serdes=%d\n",
                            serDes);
        }
        else
        {
            
            fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
        }

    }

    
    if ( debounceActive ||
        (currentState != *nextState &&
         *nextState != FM10000_SERDES_STATE_KR_TRAINING))
    {
        err = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }
    else
    {
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
    }

    return err;

}   /* end fm10000SerDesProcessSignalOkTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessSignalOkTimeoutRx
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk assertion detection in Rx only
 *                  configurations.
 *                  This is a signalOk polling mechanism used used as a
 *                  recovery when a signalOk assertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalOkTimeoutRx(fm_smEventInfo *eventInfo,
                                                void           *userInfo,
                                                fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        signalOk;
    fm_int         sw;
    fm_int         currentState;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    
    currentState = *nextState;

    
    err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

    if (err == FM_OK && signalOk == TRUE)
    {
        switch (pLaneExt->dfeMode)
        {
        case FM_DFE_MODE_STATIC:
            
            *nextState = FM10000_SERDES_STATE_RX_MISSION;
            break;
        case FM_DFE_MODE_ONE_SHOT:
        case FM_DFE_MODE_CONTINUOUS:
        case FM_DFE_MODE_ICAL_ONLY:

            err = fm10000SerDesSendDfeStartTuningReq(eventInfo,userInfo);
            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_RX_DFE_TUNING;
            }
            else
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Cannot send event to DFE sm on serdes=%d\n",
                                serDes);
            }
            break;

        case FM_DFE_MODE_KR:
        default:
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Invalid DFE mode on serdes=%d\n",
                            serDes);
            
        }
    }
    else
    {
        
         

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Cannot read SignalOk status for serdes=%d\n",
                            serDes);
        }

        
        fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
    }

    
     


    if (currentState != *nextState)
    {
        err = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }
    else
    {
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
    }

    return err;

}   /* end fm10000SerDesProcessSignalOkTimeoutRx */





/*****************************************************************************/
/** fm10000SerDesProcessDfeComplete
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing DFE tuning
 *                  complete events.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessDfeComplete(fm_smEventInfo *eventInfo,
                                          void           *userInfo,
                                          fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         sw;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    
    err = fm10000SerDesSetSignalDtctNormal(eventInfo,userInfo);
    
    if ( err == FM_OK )
    {
        err =fm10000SerDesEnableParallelLoopback(eventInfo,userInfo);
    }

    if ( err == FM_OK )
    {
        if ( pLaneExt->bistActive == FALSE )
        {
            *nextState = FM10000_SERDES_STATE_MISSION;
        }
        else
        {
            if ( pLaneExt->bistRxSubMode < FM_BIST_MAX )
            {
                err = EnableRxBistMode(eventInfo, userInfo,pLaneExt->bistRxSubMode);
            }

            if ( err == FM_OK )
            {
                *nextState = FM10000_SERDES_STATE_BIST;
            }
            else if ( err == FM_ERR_UNSUPPORTED ||
                      err == FM_ERR_INVALID_SUBMODE )
            {
                err = FM_OK;
            }
        }
    }

    locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);

    if (locErr != FM_OK)
    {
        
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Cannot start timer, serdes=%d\n",
                        serDes);
    }

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessDfeComplete */




/*****************************************************************************/
/** fm10000SerDesProcessDfeICalComplete
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing DFE iCal
 *                  complete events.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessDfeICalComplete(fm_smEventInfo *eventInfo,
                                              void           *userInfo,
                                              fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_int         sw;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    err = FM_OK;
    locErr = FM_OK;

    if ( pLaneExt->bistActive == FALSE  &&
         (fmGetBoolApiProperty(FM_AAK_API_DFE_ALLOW_EARLY_LINK_UP_MODE,
                              FM_AAD_API_DFE_ALLOW_EARLY_LINK_UP_MODE) == TRUE) )
    {
        err = fm10000SerDesSetSignalDtctNormal(eventInfo,userInfo);
        
        if ( err == FM_OK )
        {
            err =fm10000SerDesEnableParallelLoopback(eventInfo,userInfo);
        }
    
        *nextState = FM10000_SERDES_STATE_MISSION;
    
        locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    
        if (locErr != FM_OK)
        {
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Cannot start timer, serdes=%d\n",
                            serDes);
        }
    }

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessDfeICalComplete */




/*****************************************************************************/
/** fm10000SerDesProcessKrTrainingTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk assertion detection. This is a signalOk
 *                  polling mechanism used used as a recovery when a signalOk
 *                  assertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessKrTrainingTimeout(fm_smEventInfo *eventInfo,
                                                void           *userInfo,
                                                fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        krSignalOk;
    fm_bool        krFailed;
    fm_int         sw;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    

    fm10000SerDesSetSignalDtctNormal(eventInfo,userInfo);
     
    err = fm10000SerdesGetKrTrainingStatus(sw, serDes, &krSignalOk, &krFailed);

    if (err == FM_OK)
    {
        if (krFailed == TRUE || (--pLaneExt->krExt.krTrainingCtrlCnt <= 0) )
        {
            
             

            
             

            if (pLaneExt->dbgLvl >= FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
            {
                FM_LOG_PRINT("*> serdes=%d: KR Training has failed. KrFail flag=%s, Timeout=%s\n",
                             serDes,
                             krFailed ? "TRUE " : "FALSE",
                             (pLaneExt->krExt.krTrainingCtrlCnt <= 0)? "TRUE " : "FALSE");
            }

            err = fm10000SerDesStopKrTraining(eventInfo, userInfo);

            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_POWERED_UP;


                fm10000SerdesIncrKrStatsCounter(sw, serDes, 1);
            }
        }
        else if (krSignalOk == TRUE)
        {
            
            if (pLaneExt->dbgLvl == FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
            {
                
                 

                
                err = fm10000SetSerdesTxPattern(sw, serDes, FM_BIST_TX_PRBS_2048, 0, 0);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                 "Cannot set PRBS Tx pattern after completed KR training on serdes=%d\n",
                                  serDes);
                }
                else
                {
                    err = fm10000SetSerdesRxPattern(sw, serDes, FM_BIST_RX_PRBS_2048, 0, 0);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                     "Cannot set PRBS Rx pattern after completed KR training on serdes=%d\n",
                                      serDes);
                    }
                }

            }

            if (err == FM_OK)
            {
                if (pLaneExt->dbgLvl >= FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
                {
                    FM_LOG_PRINT("*> serdes=%d: KR Training Completed\n",serDes);
                }

                
                if (err == FM_OK)
                {
                    locErr = fm10000SerDesEnableParallelLoopback(eventInfo, userInfo);
                    if (locErr != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                     "Cannot set parallel loopback on serdes=%d\n",
                                      serDes);
                    }

                    
                     
                    fm10000SerdesGetSignalOk(sw, serDes, NULL);


                    fm10000SerdesGetEyeHeight(sw,serDes,&pLaneExt->krExt.eyeScoreHeight, NULL);
                    
                    if ( fmGetBoolApiProperty(FM_AAK_API_DFE_ALLOW_KR_PCAL_MODE,
                                              FM_AAD_API_DFE_ALLOW_KR_PCAL_MODE) == TRUE)
                    {
                        *nextState = FM10000_SERDES_STATE_KR_COMPLETE;
                    }
                    else
                    {
                        *nextState = FM10000_SERDES_STATE_MISSION;
                    }


                    fm10000SerdesSaveKrTrainingDelayInfo(sw,serDes);
                }
            }
        }
        else
        {
            fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
        }
    }

    if (*nextState ==  FM10000_SERDES_STATE_KR_COMPLETE )
    {
        locErr = fm10000SerDesStartKrDeferralTimer(eventInfo,userInfo);
    }
    else
    {
        locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }

    if (locErr != FM_OK)
    {
        
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Cannot start timer during KR training, serdes=%d\n",
                        serDes);
    }

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessKrTrainingTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessKrTrainingSignalOk
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  assertion events. When serdes is performing KR training
 *                  signalOk indicates that KR training is complete.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessKrTrainingSignalOk(fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        krSignalOk;
    fm_bool        krFailed;
    fm_int         sw;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;


    err = fm10000SerdesGetKrTrainingStatus(sw, serDes, &krSignalOk, &krFailed);

    if (err == FM_OK)
    {
        if (krFailed == TRUE )
        {

            if (pLaneExt->dbgLvl >= FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
            {
                FM_LOG_PRINT("*> serdes=%d: KR Training has failed. KrFail flag=%s, Timeout=%s\n",
                             serDes,
                             krFailed ? "TRUE " : "FALSE",
                             (pLaneExt->krExt.krTrainingCtrlCnt <= 0)? "TRUE " : "FALSE");
            }

            err = fm10000SerDesStopKrTraining(eventInfo, userInfo);


            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_STATE_POWERED_UP;


                fm10000SerdesIncrKrStatsCounter(sw, serDes, 1);
            }
        }
        else if (krSignalOk == TRUE)
        {
            
            if (pLaneExt->dbgLvl == FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
            {

                err = fm10000SetSerdesTxPattern(sw, serDes, FM_BIST_TX_PRBS_2048, 0, 0);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                 "Cannot set PRBS Tx pattern after completed KR training on serdes=%d\n",
                                  serDes);
                }
                else
                {
                    err = fm10000SetSerdesRxPattern(sw, serDes, FM_BIST_RX_PRBS_2048, 0, 0);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                     "Cannot set PRBS Rx pattern after completed KR training on serdes=%d\n",
                                      serDes);
                    }
                }
            }

            if (err == FM_OK)
            {
                if (pLaneExt->dbgLvl >= FM10000_SERDES_DBG_TEST_BOARD_LVL_1)
                {
                    FM_LOG_PRINT("*> serdes=%d: KR Training Completed\n",serDes);
                }

                
                if (err == FM_OK)
                {
                    locErr = fm10000SerDesEnableParallelLoopback(eventInfo, userInfo);
                    if (locErr != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_SERDES,
                                     "Cannot set parallel loopback on serdes=%d\n",
                                      serDes);
                    }

                    fm10000SerdesGetSignalOk(sw, serDes, NULL);


                    fm10000SerdesGetEyeHeight(sw,serDes,&pLaneExt->krExt.eyeScoreHeight, NULL);

                    if ( fmGetBoolApiProperty(FM_AAK_API_DFE_ALLOW_KR_PCAL_MODE,
                                              FM_AAD_API_DFE_ALLOW_KR_PCAL_MODE) == TRUE)
                    {
                        *nextState = FM10000_SERDES_STATE_KR_COMPLETE;
                    }
                    else
                    {
                        *nextState = FM10000_SERDES_STATE_MISSION;
                    }


                    fm10000SerdesSaveKrTrainingDelayInfo(sw,serDes);
                }
            }
        }
    }

    if (*nextState ==  FM10000_SERDES_STATE_KR_COMPLETE )
    {
        locErr = fm10000SerDesStartKrDeferralTimer(eventInfo,userInfo);
    }
    else
    {
        locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }

    if (locErr != FM_OK)
    {
        
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Cannot start timer during KR training, serdes=%d\n",
                        serDes);
    }

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessKrTrainingSignalOk */




/*****************************************************************************/
/** fm10000SerDesProcessKrTrainingComplete
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing signalOk
 *                  assertion during KR training. This is considered as an
 *                  indication that KR is complete. Currently this event is
 *                  processed in the same way than when KR training timeout
 *                  hits, so this function calls just calls
 *                  fm10000SerDesProcesskrTrainingTimeout.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessKrTrainingComplete(fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;

    err = fm10000SerDesProcessKrTrainingTimeout(eventInfo, userInfo, nextState);

    if (err == FM_OK)
    {
       
       err = fm10000SerDesSaveTransitionRecord(eventInfo,userInfo);
    }

    return err;

}   /* end fm10000SerDesProcessKrTrainingComplete */




/*****************************************************************************/
/** fm10000SerDesProcessDfeTuningTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk assertion detection. This is a signalOk
 *                  polling mechanism used used as a recovery when a signalOk
 *                  assertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessDfeTuningTimeout(fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      err;
    fm_status      locErr;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        dfeTuningActive;
    fm_bool        signalOk;
    fm_int         sw;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    err = fm10000SerdesIsDfeTuningActive(sw, serDes, &dfeTuningActive);

    if (err == FM_OK)
    {
        if (dfeTuningActive)
        {
            fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
            
        }
        else
        {
            err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

            if (err == FM_OK)
            {
                if (signalOk)
                {
                    
                    err = fm10000SerDesSendDfeStartTuningReq(eventInfo,userInfo);
                }
                else
                {
                    *nextState = FM10000_SERDES_STATE_POWERED_UP;
                }
            }
        }
    }

    
    locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);

    if (locErr != FM_OK)
    {
        
        FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                        serDes,
                        "Cannot start timer during KR training, serdes=%d\n",
                        serDes);
    }

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessDfeTuningTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessSignalNokTimeout
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk deassertion detection. This is a
 *                  signalOk polling mechanism used used as a recovery when a
 *                  signalOk deassertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalNokTimeout(fm_smEventInfo *eventInfo,
                                               void           *userInfo,
                                               fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        signalOk;
    fm_int         sw;
    fm_int         currentState;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    
    currentState = *nextState;

    
    err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

    if (pLaneExt->eeeModeActive)
    {

        signalOk = TRUE;
    }

    if (err == FM_OK && signalOk == FALSE)
    {
        switch (pLaneExt->dfeMode)
        {
            case FM_DFE_MODE_KR:
                
                err = fm10000SerDesStopKrTraining(eventInfo, userInfo);
            case FM_DFE_MODE_STATIC:
                if (err == FM_OK)
                {
                    
                    *nextState = FM10000_SERDES_STATE_POWERED_UP;
                }
                break;
            case FM_DFE_MODE_ONE_SHOT:
            case FM_DFE_MODE_CONTINUOUS:
            case FM_DFE_MODE_ICAL_ONLY:

                err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

                if ( err == FM_OK )
                {
                    err = fm10000SerDesSendDfeStopTuningReq(eventInfo,userInfo);
                }
    
                if ( err == FM_OK )
                {
                    *nextState = FM10000_SERDES_STATE_POWERED_UP;
                }
                else
                {
                    FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                    serDes,
                                    "Cannot change to POWERED_UP state on serdes=%d\n",
                                    serDes);
                }
                break;

            default:
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Invalid DFE mode on serdes=%d\n",
                                serDes);
                
        }
    }
    else
    {
        
         

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Cannot read SignalOk status for serdes=%d\n",
                            serDes);
        }

    }

    
     


    if (currentState != *nextState)
    {
        err = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }
    else
    {
        err = fm10000SerDesStartTimeoutTimerXLng(eventInfo,userInfo);

        
        fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);

    }

    return err;

}   /* end fm10000SerDesProcessSignalNokTimeout */




/*****************************************************************************/
/** fm10000SerDesProcessSignalNokTimeoutRx
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing timeout events
 *                  related to signalOk deassertion detection in Rx only
 *                  configurations.
 *                  This is a signalOk polling mechanism used used as a
 *                  recovery when a signalOk deassertion interrupt is missed.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessSignalNokTimeoutRx(fm_smEventInfo *eventInfo,
                                                 void           *userInfo,
                                                 fm_int         *nextState )
{
    fm_status      err;
    fm10000_lane  *pLaneExt;
    fm_int         serDes;
    fm_bool        signalOk;
    fm_int         sw;
    fm_int         currentState;


    pLaneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;

    
    currentState = *nextState;

    
    err = fm10000SerdesGetSignalOk(sw, serDes, &signalOk);

    if (err == FM_OK && signalOk == FALSE)
    {
        switch (pLaneExt->dfeMode)
        {
        case FM_DFE_MODE_STATIC:
            
            *nextState = FM10000_SERDES_STATE_RX_ON;
            break;
        case FM_DFE_MODE_ONE_SHOT:
        case FM_DFE_MODE_CONTINUOUS:
        case FM_DFE_MODE_ICAL_ONLY:

            err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

            if ( err == FM_OK )
            {
                err = fm10000SerDesSendDfeStopTuningReq(eventInfo,userInfo);
            }

            if ( err == FM_OK )
            {
                *nextState = FM10000_SERDES_STATE_RX_ON;
            }
            else
            {
                FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                                serDes,
                                "Cannot change to RX_ON state on serdes=%d\n",
                                serDes);
            }

            break;

        case FM_DFE_MODE_KR:
        default:
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES,
                            serDes,
                            "Invalid DFE mode on serdes=%d\n",
                            serDes);
            
        }
    }
    else
    {
        
         

        if (err != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Cannot read Signa.Ok status for serdes=%d\n",
                            serDes);
        }

        
        fm10000SerDesDontSaveTransitionRecord(eventInfo,userInfo);
    }

    
     


    if (currentState != *nextState)
    {
        err = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);
    }
    else
    {
        err = fm10000SerDesStartTimeoutTimerLng(eventInfo,userInfo);
    }

    return err;

}   /* end fm10000SerDesProcessSignalNokTimeoutRx */




/*****************************************************************************/
/** fm10000SerDesConfigureEthOrPcie
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback (used only by the stub
 *                  serDes state machine) that handles a CONFIGURE event
 *                  differently for PCIE and Ethernet ports
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesConfigureEthOrPcie( fm_smEventInfo *eventInfo,
                                           void           *userInfo,
                                           fm_int         *nextState )
{
    fm10000_lane *laneExt;
    fm_status     status;

    FM_NOT_USED( eventInfo );

    laneExt = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    if ( laneExt->parentPortExt->ring == FM10000_SERDES_RING_PCIE )
    {
        
        *nextState = FM10000_SERDES_STATE_MISSION;
        status     = FM_OK;
    }
    else
    {
        status = fm10000SerDesDumpBitRate( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_SERDES, laneExt->serDes, status );
        *nextState = FM10000_SERDES_STATE_CONFIGURED;
    }

ABORT:
    return status;

}  /* end fm10000SerDesConfigureEthOrPcie */




/*****************************************************************************/
/** fm10000SerDesProcessEnableBistMode
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing enabling Bist
 *                  on this SerDes. The final state depends on transmitter
 *                  and receiver statuses.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessEnableBistMode(fm_smEventInfo *eventInfo,
                                              void           *userInfo,
                                              fm_int         *nextState)
{
    fm_status     err;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;
    fm_int        bistSubMode;
    fm_bool       configTxBist;
    fm_bool       configRxBist;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    configTxBist = FALSE;
    configRxBist = FALSE;

    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;
    bistSubMode = ((fm10000_serDesSmEventInfo *)userInfo)->info.bistSubmode;

    pLaneExt->bistTxSubMode = FM_BIST_MAX;
    pLaneExt->bistRxSubMode = FM_BIST_MAX;

    switch (bistSubMode)
    {
        case FM_BIST_TX_PRBS_128:
        case FM_BIST_TX_PRBS_512B:
        case FM_BIST_TX_PRBS_2048:
        case FM_BIST_TX_PRBS_32K:
        case FM_BIST_TX_PRBS_8M:
        case FM_BIST_TX_PRBS_2G:
        case FM_BIST_TX_IDLECHAR:
        case FM_BIST_TX_TESTCHAR:
        case FM_BIST_TX_LOWFREQ:
        case FM_BIST_TX_HIGHFREQ:
        case FM_BIST_TX_MIXEDFREQ:
        case FM_BIST_TX_SQUARE8:
        case FM_BIST_TX_SQUARE10:
        case FM_BIST_TX_CUSTOM10:
        case FM_BIST_TX_CUSTOM20:
        case FM_BIST_TX_CUSTOM40:
        case FM_BIST_TX_CUSTOM80:
        {
            configTxBist = TRUE;
            break;
        }
        case FM_BIST_RX_PRBS_128:
        case FM_BIST_RX_PRBS_512B:
        case FM_BIST_RX_PRBS_2048:
        case FM_BIST_RX_PRBS_32K:
        case FM_BIST_RX_PRBS_8M:
        case FM_BIST_RX_PRBS_2G:
        case FM_BIST_RX_IDLECHAR:
        case FM_BIST_RX_TESTCHAR:
        case FM_BIST_RX_LOWFREQ:
        case FM_BIST_RX_HIGHFREQ:
        case FM_BIST_RX_MIXEDFREQ:
        case FM_BIST_RX_SQUARE8:
        case FM_BIST_RX_SQUARE10:
        case FM_BIST_RX_CUSTOM10:
        case FM_BIST_RX_CUSTOM20:
        case FM_BIST_RX_CUSTOM40:
        case FM_BIST_RX_CUSTOM80:
        {
            configRxBist = TRUE;
            break;
        }
        case FM_BIST_TXRX_PRBS_128:
        case FM_BIST_TXRX_PRBS_512B:
        case FM_BIST_TXRX_PRBS_2048:
        case FM_BIST_TXRX_PRBS_32K:
        case FM_BIST_TXRX_PRBS_8M:
        case FM_BIST_TXRX_PRBS_2G:
        case FM_BIST_TXRX_IDLECHAR:
        case FM_BIST_TXRX_TESTCHAR:
        case FM_BIST_TXRX_LOWFREQ:
        case FM_BIST_TXRX_HIGHFREQ:
        case FM_BIST_TXRX_MIXEDFREQ:
        case FM_BIST_TXRX_SQUARE8:
        case FM_BIST_TXRX_SQUARE10:
        case FM_BIST_TXRX_CUSTOM10:
        case FM_BIST_TXRX_CUSTOM20:
        case FM_BIST_TXRX_CUSTOM40:
        case FM_BIST_TXRX_CUSTOM80:
        {
            configTxBist = TRUE;
            configRxBist = TRUE;
            break;
        }
        case FM_BIST_TX_PRBS_512A:
        case FM_BIST_RX_PRBS_512A:
        case FM_BIST_TXRX_PRBS_512A:
        case FM_BIST_TX_PRBS_1024:
        case FM_BIST_RX_PRBS_1024:
        case FM_BIST_TXRX_PRBS_1024:
        case FM_BIST_TX_CJPAT:
        case FM_BIST_RX_CJPAT:
        case FM_BIST_TXRX_CJPAT:
        {
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Unsupported BIST submode: %d\n", bistSubMode );
            err = FM_ERR_UNSUPPORTED;
            break;
        }
        default:
            
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Invalid BIST submode: %d\n", bistSubMode );
            err = FM_ERR_INVALID_SUBMODE;
    }

    if (err == FM_OK && configTxBist == TRUE)
    {
        
        if ((pLaneExt->farLoopbackStatus & 0x01) == 0)
        {
            err = fm10000SetSerdesTxPattern(sw,
                                            serDes,
                                            bistSubMode,
                                            pLaneExt->bistCustomData0,
                                            pLaneExt->bistCustomData1);
            if ( err == FM_OK )
            {
                pLaneExt->bistTxSubMode = bistSubMode;
            }
        }
        else
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_SERDES, serDes,
                            "Incompatible test mode\n");
            
             
            err = FM_ERR_INVALID_SUBMODE;
        }
    }

    if (err == FM_OK && configRxBist == TRUE)
    {
        
         


        err = SendDfeEventReq(eventInfo,
                              userInfo,
                              FM10000_SERDES_DFE_EVENT_PAUSE_TUNING_REQ);

        if (err == FM_OK)
        {
            
             
            err = fm10000SetSerdesRxPattern(sw,
                                            serDes,
                                            bistSubMode,
                                            pLaneExt->bistCustomData0,
                                            pLaneExt->bistCustomData1);

            if (err == FM_OK)
            {
                pLaneExt->bistRxSubMode = bistSubMode;
                err = fm10000ResetSerdesErrorCounter(sw, serDes);
            }
        }
    }

    if (err == FM_OK)
    {
        if ( pLaneExt->dfeMode == FM_DFE_MODE_STATIC )
        {
            *nextState = FM10000_SERDES_STATE_BIST;
        }
        pLaneExt->bistActive = TRUE;
    }
    else
    {
        fm10000SerdesSetTxDataSelect(sw,
                                     serDes,
                                     FM10000_SERDES_TX_DATA_SEL_CORE);
        fm10000SerdesSetRxCmpData(sw,
                                  serDes,
                                  FM10000_SERDES_RX_CMP_DATA_OFF);
    }

    return err;

}   /* end fm10000SerDesProcessEnableBistMode */




/*****************************************************************************/
/** fm10000SerDesProcessDisableBistMode
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing disabling Bist
 *                  on this SerDes. The final state depends on transmitter
 *                  and receiver statuses and also on the far loopback status.
 *                  If serdes receiver is enabled a timer is started to poll
 *                  for signalOk status.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessDisableBistMode(fm_smEventInfo *eventInfo,
                                              void           *userInfo,
                                              fm_int         *nextState)
{
    fm_status       err;
    fm_int          sw;
    fm_int          serDes;
    fm10000_lane   *pLaneExt;


    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;

    
     

     


    switch ((pLaneExt->serDesPllStatus) & 3)
    {
        case 1:
        {
            
             
            err = fm10000SerdesSetTxDataSelect(sw,
                                               serDes,
                                               FM10000_SERDES_TX_DATA_SEL_CORE);
            if (err == FM_OK)
            {
                err = fm10000SerdesSetRxCmpData(sw,
                                                serDes,
                                                FM10000_SERDES_RX_CMP_DATA_OFF);
                if (err == FM_OK)
                {
                    *nextState = FM10000_SERDES_STATE_TX_ON;
                }
            }
            break;
        }

        case 2:
        {
            
             


            
            err = SendDfeEventReq(eventInfo,
                                  userInfo,
                                  FM10000_SERDES_DFE_EVENT_STOP_TUNING_REQ);
            if (err == FM_OK)
            {
                err = fm10000SerdesSetTxDataSelect(sw,
                                                   serDes,
                                                   FM10000_SERDES_TX_DATA_SEL_CORE);
                if (err == FM_OK)
                {
                    err = fm10000SerdesSetRxCmpData(sw,
                                                    serDes,
                                                    FM10000_SERDES_RX_CMP_DATA_OFF);

                    if (err == FM_OK)
                    {
                        err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);

                        if (err == FM_OK)
                        {
                            err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

                            if (err == FM_OK)
                            {
                                *nextState = FM10000_SERDES_STATE_RX_ON;
                            }
                        }
                    }
                }
            }
            break;
        }

        case 3:
        {
            
             

             

            
            err = SendDfeEventReq(eventInfo,
                                  userInfo,
                                  FM10000_SERDES_DFE_EVENT_STOP_TUNING_REQ);

            if (err == FM_OK)
            {
                if ((pLaneExt->farLoopbackStatus & 0x01) == 0)
                {
                    err = fm10000SerdesSetTxDataSelect(sw,
                                                       serDes,
                                                       FM10000_SERDES_TX_DATA_SEL_CORE);
                    if (err == FM_OK)
                    {
                        err = fm10000SerdesSetRxCmpData(sw,
                                                        serDes,
                                                        FM10000_SERDES_RX_CMP_DATA_OFF);
                    }
                }
                else
                {
                    err = fm10000SerDesEnableParallelLoopback(eventInfo, userInfo);
                }
            }

            if (err == FM_OK)
            {
                err = fm10000SerDesStartTimeoutTimerShrt(eventInfo, userInfo);

                if (err == FM_OK)
                {
                   err = fm10000ConfigurePcslBitSlip(sw,serDes);
                }

                if (err == FM_OK)
                {
                    err = fm10000SerDesSetSignalDtctForcedBadC(eventInfo,userInfo);

                    if (err == FM_OK)
                    {
                        *nextState = FM10000_SERDES_STATE_POWERED_UP;
                    }
                }
            }
            break;
        }
        default:
            FM_LOG_ERROR_V2( FM_LOG_CAT_SERDES,
                             serDes,
                             "Invalid PLLs status mask = 0x%08x\n",
                             pLaneExt->serDesPllStatus );
            err = FM_FAIL;
    }

    if ( err == FM_OK )
    {
        err = fm10000SerdesSetWidthMode(sw,serDes,pLaneExt->widthMode);
    }

    if ( err == FM_OK )
    {
        err = fm10000SerDesRemoveBistConfig(eventInfo,userInfo);
    }

    return err;

}   /* end fm10000SerDesProcessDisableBistMode */




/*****************************************************************************/
/** fm10000SerDesProcessBistDisableLoopback
 * \ingroup intSerdes
 *
 * \desc            Conditional transition callback processing disabling
 *                  loopback when in BIST mode. The final state depends
 *                  mostly on transmitter dfeMode and signalOk statuses.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_serdesSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesProcessBistDisableLoopback(fm_smEventInfo *eventInfo,
                                                  void           *userInfo,
                                                  fm_int         *nextState)
{
    fm_status     err;
    fm_status     locErr;
    fm_int        sw;
    fm_int        serDes;
    fm10000_lane *pLaneExt;

    FM_NOT_USED(eventInfo);


    sw = ((fm10000_serDesSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt    = ((fm10000_serDesSmEventInfo *)userInfo)->laneExt;
    serDes      = pLaneExt->serDes;

    err = fm10000SerDesDisableNearLoopback(eventInfo,userInfo);

    if ( err == FM_OK )
    {
        err = fm10000SerDesConfigLanePolarity(eventInfo,userInfo);
    }

    if ( err == FM_OK )
    {
        err = fm10000SerDesEnableTxOutput(eventInfo,userInfo);
    }

    if (err == FM_OK && pLaneExt->bistTxSubMode < FM_BIST_MAX)
    {
        
        err = fm10000SetSerdesTxPattern(sw,
                                        serDes,
                                        pLaneExt->bistTxSubMode,
                                        pLaneExt->bistCustomData0,
                                        pLaneExt->bistCustomData1);
    }

    if (err == FM_OK && pLaneExt->bistRxSubMode < FM_BIST_MAX)
    {
        err = fm10000SetSerdesRxPattern(sw,
                                        serDes,
                                        pLaneExt->bistRxSubMode,
                                        pLaneExt->bistCustomData0,
                                        pLaneExt->bistCustomData1);

        if (err == FM_OK)
        {
            err = fm10000ResetSerdesErrorCounter(sw, serDes);
        }
    }

    if (err == FM_OK)
    {
        if ( pLaneExt->dfeMode == FM_DFE_MODE_STATIC )
        {
            *nextState = FM10000_SERDES_STATE_BIST;
        }
        else
        {
            *nextState = FM10000_SERDES_STATE_POWERED_UP;
        }
        pLaneExt->bistActive = TRUE;
    }
    else
    {
        fm10000SerdesSetTxDataSelect(sw,
                                     serDes,
                                     FM10000_SERDES_TX_DATA_SEL_CORE);
        fm10000SerdesSetRxCmpData(sw,
                                  serDes,
                                  FM10000_SERDES_RX_CMP_DATA_OFF);
    }


    locErr = fm10000SerDesStartTimeoutTimerShrt(eventInfo,userInfo);

    err = (err != FM_OK)? err : locErr;

    return err;

}   /* end fm10000SerDesProcessBistDisableLoopback */


