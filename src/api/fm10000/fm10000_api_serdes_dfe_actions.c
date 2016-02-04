/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_serdes_dfe_actions.c
 * Creation Date:   Mars 19, 2014
 * Description:     Action callbacks for the FM10000 serdes state machines
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

static fm_status SendDfeEventInd(fm_smEventInfo *eventInfo,
                                 void           *userInfo,
                                 fm_int          eventId);
static fm_status StartTimeoutTimer(fm_smEventInfo *eventInfo,
                                   void           *userInfo,
                                   fm_timestamp   *pTimeout);
static void HandleDfeTuningTimeout(void *arg);


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
/** SendDfeEventInd
 * \ingroup intPort
 *
 * \desc            Sends a DFE-level event notification to the parent serdes
 *                  state machine.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \param[in]       eventId is the event to be notified.
 *
 * \return          FM_OK if successful
 *
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status SendDfeEventInd(fm_smEventInfo *eventInfo,
                          void           *userInfo,
                          fm_int          eventId )
{
    fm_status        err;
    fm_int           sw;
    fm10000_lane    *pLaneExt;
    fm_smEventInfo   serDesEventInfo;

    FM_NOT_USED(eventInfo);

    sw = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneExt = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;

    serDesEventInfo.smType  = pLaneExt->smType;
    serDesEventInfo.srcSmType = pLaneExt->dfeExt.smType;
    serDesEventInfo.eventId = eventId;
    serDesEventInfo.lock    = FM_GET_STATE_LOCK( sw );

    err = fmNotifyStateMachineEvent(pLaneExt->smHandle,
                                    &serDesEventInfo,
                                    &pLaneExt->eventInfo,
                                    &pLaneExt->serDes);

    return err;

}




/*****************************************************************************/
/** HandleDfeTuningTimeout
 * \ingroup intSerDesDfe
 *
 * \desc            Callback that handles a timeout and sends an event to the
 *                  DFE state machine.
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_laneDfe'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleDfeTuningTimeout(void *arg)
{
    fm_smEventInfo             eventInfo;
    fm10000_dfeSmEventInfo    *pDfeEventInfo;
    fm10000_laneDfe           *pLaneDfe;
    fm_int                     sw;


    pLaneDfe          = (fm10000_laneDfe *) arg;
    pDfeEventInfo     = &pLaneDfe->eventInfo;
    sw                = pDfeEventInfo->switchPtr->switchNumber;

    PROTECT_SWITCH(sw);


    eventInfo.smType  = pLaneDfe->smType;
    eventInfo.srcSmType = 0;
    eventInfo.eventId = FM10000_SERDES_DFE_EVENT_TIMEOUT_IND;
    eventInfo.lock    = FM_GET_STATE_LOCK(sw);

    fmNotifyStateMachineEvent(pLaneDfe->smHandle,
                              &eventInfo,
                              pDfeEventInfo,
                              &pLaneDfe->pLaneExt->serDes);

    UNPROTECT_SWITCH(sw);

}




/*****************************************************************************/
/** StartTimeoutTimer
 * \ingroup intSerDesDfe
 *
 * \desc            Starts SerDes-DFE timeout timer. Timer is started using the
 *                  provided expiration time. The number of repetitions is
 *                  always set to 1.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
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
    fm_status        err;
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);



    pLaneDfe = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    err = fmStartTimer( pLaneDfe->timerHandle,
                        pTimeout,
                        1,
                        HandleDfeTuningTimeout,
                        pLaneDfe );
    return err;

}




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000LogSerDesDfeTransition
 * \ingroup intSerDesDfe
 *
 * \desc            Logs transitions of the SerDes-DFE state machine.
 *
 * \param[in]       record is the pointer to a caller-allocated structure
 *                  containing the SerDes state transition information to be
 *                  logged
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000LogSerDesDfeTransition(fm_smTransitionRecord *record)
{
    fm_text currentState;
    fm_text event;

    if ( record->currentState != FM_STATE_UNSPECIFIED )
    {
        currentState = fm10000SerDesDfeStatesMap[record->currentState];
    }
    else
    {
        currentState = "N/A";
    }
    if ( record->eventInfo.eventId != FM_EVENT_UNSPECIFIED )
    {
        event = fm10000SerDesDfeEventsMap[record->eventInfo.eventId];
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
                     fm10000SerDesDfeStatesMap[record->nextState] );

    return FM_OK;

}



/*****************************************************************************/
/** fm10000SerDesDfeFlagError
 * \ingroup intSerDesDfe
 *
 * \desc            Action indicating an invalid state-event combination for
 *                  this state machine.
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
fm_status fm10000SerDesDfeFlagError(fm_smEventInfo *eventInfo,
                                    void           *userInfo)
{
    FM_NOT_USED(eventInfo);
    FM_NOT_USED(userInfo);

    return FM_ERR_INVALID_STATE;

}




/*****************************************************************************/
/** fm10000SerDesDfeConfigDfe
 * \ingroup intSerDesDfe
 *
 * \desc            Action initializing several DFE parameters.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeConfigDfe(fm_smEventInfo *eventInfo,
                                    void           *userInfo )
{
    fm_status        err;
    fm_switch       *switchPtr;
    fm10000_lane    *pLaneExt;
    fm10000_laneDfe *pLaneDfe;
    fm_int           sw;
    fm_int           serDes;

    FM_NOT_USED(eventInfo);

    err = FM_OK;

    pLaneExt  = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;


    pLaneDfe->dfeDataLevThreshold = FM10000_SERDES_DFE_DATA_LEVEL0_THRESHLD;
    pLaneDfe->pause = FALSE;


    err = fm10000SerdesDfeTuningConfig(sw,serDes);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting DFE tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartTuning(fm_smEventInfo *eventInfo,
                                      void           *userInfo )
{
    fm_status        err;
    fm10000_lane    *pLaneExt;
    fm10000_laneDfe *pLaneDfe;
    fm_int           sw;
    fm_int           serDes;


    FM_NOT_USED(eventInfo);

    pLaneExt = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneDfe = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;



    pLaneDfe->dfeAdaptive = (pLaneExt->dfeMode == FM_DFE_MODE_CONTINUOUS);
    pLaneDfe->refTimeMs = fm10000SerdesGetTimestampMs();
    pLaneDfe->pause = FALSE;
    pLaneExt->dfeExt.sendDfeComplete = FALSE;
    pLaneExt->dfeExt.pCalKrMode = FALSE;

    err = fm10000SerdesIncrStatsCounter(sw,serDes,0);

    if (err == FM_OK)
    {

        err = fm10000SerdesDfeTuningStartICal(sw, serDes);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartKrPCalTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting pCal tuning after KR training.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartKrPCalTuning(fm_smEventInfo *eventInfo,
                                            void           *userInfo )
{
    fm_status        err;
    fm10000_lane    *pLaneExt;
    fm10000_laneDfe *pLaneDfe;
    fm_int           sw;
    fm_int           serDes;


    FM_NOT_USED(eventInfo);

    pLaneExt = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes   = pLaneExt->serDes;
    sw       = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneDfe = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    pLaneDfe->dfeAdaptive = (pLaneExt->krExt.pCalMode ==  FM10000_SERDES_KR_PCAL_MODE_CONTINUOUS);
    pLaneDfe->refTimeMs = fm10000SerdesGetTimestampMs();
    pLaneDfe->pause = FALSE;
    pLaneDfe->dfeDataLevThreshold = FM10000_SERDES_DFE_DATA_LEVEL0_THRESHLD;
    pLaneExt->dfeExt.sendDfeComplete = FALSE;
    pLaneExt->dfeExt.pCalKrMode = TRUE;

    err = fm10000SerdesDfeTuningStartPCalSingleExec(sw, serDes);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfePauseTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action pausing DFE tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfePauseTuning(fm_smEventInfo *eventInfo,
                                      void           *userInfo )
{
    fm_status        err;
    fm_int           sw;
    fm_int           serDes;
    fm_bool         *pPause;

    FM_NOT_USED(eventInfo);

    serDes = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt->serDes;
    sw     = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pPause = &(((fm10000_dfeSmEventInfo *)userInfo)->laneExt->dfeExt.pause);
    err    = FM_OK;

    if (*pPause == FALSE)
    {
        err = fm10000SerdesDfeTuningStop(sw,serDes);

        if (err == FM_OK)
        {
            *pPause = TRUE;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeResumeTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action resuming DFE tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeResumeTuning(fm_smEventInfo *eventInfo,
                                       void           *userInfo )
{
    fm_status        err;
    fm10000_laneDfe *pLaneDfe;
    fm_int           serDes;
    fm_int           sw;

    FM_NOT_USED(eventInfo);

    serDes   = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt->serDes;
    sw       = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneDfe = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    err = FM_OK;


    if (pLaneDfe->dfeAdaptive && pLaneDfe->pause == TRUE)
    {

        err = fm10000SerdesDfeTuningStartPCalContinuous(sw, serDes);

        if (err == FM_OK)
        {
            pLaneDfe->pause = FALSE;
        }
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeProcessPushAutoStartTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Conditional transition callback processing autostart
 *                  events used to retry DFE tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeProcessPushAutoStartTuning(fm_smEventInfo *eventInfo,
                                                     void           *userInfo,
                                                     fm_int         *nextState)
{
    fm_status        err;
    fm_int           sw;
    fm10000_laneDfe *pLaneDfe;
    fm_smEventInfo   dfeEventInfo;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    sw = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;



    if (--pLaneDfe->retryCntr > 0)
    {
        dfeEventInfo.smType  = pLaneDfe->smType;
        dfeEventInfo.srcSmType = 0;
        dfeEventInfo.eventId = FM10000_SERDES_DFE_EVENT_START_TUNING_REQ;
        dfeEventInfo.lock    = FM_GET_STATE_LOCK( sw );

        err = fmNotifyStateMachineEvent(pLaneDfe->smHandle,
                                        &dfeEventInfo,
                                        &pLaneDfe->eventInfo,
                                        &pLaneDfe->pLaneExt->serDes);
        if (err == FM_OK)
        {
            *nextState = FM10000_SERDES_DFE_STATE_WAIT_ICAL;
        }
        else
        {

            err = fm10000SerDesDfeStartTimeoutTimerShrt(eventInfo,userInfo);
        }
    }
    else
    {

        err = fm10000SerDesDfeSendTuningStoppedInd(eventInfo,userInfo);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartTimeoutTimerShrt
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting the SerDes-DFE timeout timer using the
 *                  short expiration time.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartTimeoutTimerShrt(fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_DFE_SHORT_TIMEOUT;

    err = StartTimeoutTimer(eventInfo, userInfo, &timeout);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartTimeoutTimerStopTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting the SerDes-DFE timeout timer used to
 *                  stop DFE.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartTimeoutTimerStopTuning(fm_smEventInfo *eventInfo,
                                                      void           *userInfo )
{
    fm_status     err;
    fm_timestamp  timeout;


    timeout.sec  = 0;
    timeout.usec = FM10000_SERDES_DFE_STOP_TUNING_TIMEOUT;

    err = StartTimeoutTimer(eventInfo, userInfo, &timeout);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartTimeoutTimerDebounce
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting the SerDes-DFE timeout timer for the
 *                  debounce state after DFE is complete.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartTimeoutTimerDebounce(fm_smEventInfo *eventInfo,
                                                    void           *userInfo )
{
    fm_status        err;
    fm10000_laneDfe *pLaneDfe;
    fm_uint32        dfeDebounceTime;
    fm_timestamp     timeout;

    FM_NOT_USED(eventInfo);


    err = FM_OK;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;


    dfeDebounceTime = pLaneDfe->dfeDebounceTime;

    if (dfeDebounceTime == 0)
    {
        dfeDebounceTime = FM10000_SERDES_DFE_DFAULT_DEBOUNCE_TIME;
    }


    timeout.sec  = 0;



    timeout.usec = dfeDebounceTime * 50;


    err = StartTimeoutTimer(eventInfo,userInfo,&timeout);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStartTimeoutTimerAdaptive
 * \ingroup intSerDesDfe
 *
 * \desc            Action starting the SerDes-DFE timeout timer using an
 *                  adaptive algorithm for eye score reading. This allows
 *                  changing the reading pace, which is faster at the begining
 *                  to become slower when the score is more or less stable.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStartTimeoutTimerAdaptive(fm_smEventInfo *eventInfo,
                                                    void           *userInfo )
{
    fm_status        err;
    fm_timestamp     timeout;
    fm10000_laneDfe *pLaneDfe;


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    timeout.usec = 0;
    timeout.sec  = FM10000_SERDES_DFE_ADAPTIVE_TIMEOUT_MIN;


    timeout.sec += ((pLaneDfe->cycleCntr) >> 3);

    if (timeout.sec > FM10000_SERDES_DFE_ADAPTIVE_TIMEOUT_MAX)
    {
        timeout.sec = FM10000_SERDES_DFE_ADAPTIVE_TIMEOUT_MAX;
    }

    err = StartTimeoutTimer(eventInfo, userInfo, &timeout);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeStopTimeoutTimer
 * \ingroup intSerDesDfe
 *
 * \desc            Action stopping the SerDes-DFE timeout timer.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor (unused
 *                  in this function)
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStopTimeoutTimer(fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_status        err;
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;
    err = fmStopTimer(pLaneDfe->timerHandle);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeClrCycleCntr
 * \ingroup intSerDesDfe
 *
 * \desc            Action clearing the dfe cycle counter.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeClrCycleCntr(fm_smEventInfo *eventInfo,
                                       void           *userInfo )
{
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;
    pLaneDfe->cycleCntr = 0;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerDesDfeIncCycleCntr
 * \ingroup intSerDesDfe
 *
 * \desc            Action incrementing the dfe cycle counter.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeIncCycleCntr(fm_smEventInfo *eventInfo,
                                       void           *userInfo )
{
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;
    pLaneDfe->cycleCntr++;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerDesDfeLoadRetryCntr
 * \ingroup intSerDesDfe
 *
 * \desc            Action loading the dfe retry counter.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeLoadRetryCntr(fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;
    if (pLaneDfe->retryCntr == 0)
    {
        pLaneDfe->retryCntr =  FM10000_SERDES_DFE_TUNING_MAX_RETRIES;
    }

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerDesDfeClrRetryCntr
 * \ingroup intSerDesDfe
 *
 * \desc            Action clearing the dfe retry counter.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeClrRetryCntr(fm_smEventInfo *eventInfo,
                                       void           *userInfo )
{
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);


    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;
    pLaneDfe->retryCntr =  0;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerDesDfeStopTuning
 * \ingroup intSerDesDfe
 *
 * \desc            Action stopping DFE tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo pointer a purpose specific event descriptor (must
 *                  be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeStopTuning(fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    fm_status        err;
    fm_int           serDes;
    fm_int           sw;

    FM_NOT_USED(eventInfo);

    serDes = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt->serDes;
    sw     = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr->switchNumber;

    err = fm10000SerdesDfeTuningStop(sw,serDes);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeSendTuningStartedInd
 * \ingroup intSerDesDfe
 *
 * \desc            Action sending a 'DFE-started' event notification to the
 *                  parent state machine.
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
fm_status fm10000SerDesDfeSendTuningStartedInd(fm_smEventInfo *eventInfo,
                                               void           *userInfo )
{
    fm10000_lane    *pLaneExt;


    pLaneExt = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    pLaneExt->dfeExt.dfeTuningStat = 0x01;

    return SendDfeEventInd(eventInfo, userInfo, FM10000_SERDES_EVENT_DFE_TUNING_STARTED_IND);

}




/*****************************************************************************/
/** fm10000SerDesDfeSendTuningStoppedInd
 * \ingroup intSerDesDfe
 *
 * \desc            Action sending a 'DFE-stopped' event notification to the
 *                  parent state machine.
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
fm_status fm10000SerDesDfeSendTuningStoppedInd(fm_smEventInfo *eventInfo,
                                               void           *userInfo )
{
    fm_status   err;

    err = SendDfeEventInd(eventInfo, userInfo, FM10000_SERDES_EVENT_DFE_TUNING_STOPPED_IND);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeSendTuningCompleteInd
 * \ingroup intSerDesDfe
 *
 * \desc            Action sending a 'DFE-complete' event notification to the
 *                  parent state machine.
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
fm_status fm10000SerDesDfeSendTuningCompleteInd(fm_smEventInfo *eventInfo,
                                                void           *userInfo )
{
    fm_status        err;
    fm10000_lane    *pLaneExt;

    err = FM_OK;
    pLaneExt  = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;

    if (pLaneExt->dfeExt.sendDfeComplete)
    {
        err = SendDfeEventInd(eventInfo, userInfo, FM10000_SERDES_EVENT_DFE_TUNING_COMPLETE_IND);
        pLaneExt->dfeExt.sendDfeComplete = FALSE;
    }
    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeSendICalTuningCompleteInd
 * \ingroup intSerDesDfe
 *
 * \desc            Action sending a 'iCal-complete' event notification to
 *                  the parent state machine.
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
fm_status fm10000SerDesDfeSendICalTuningCompleteInd(fm_smEventInfo *eventInfo,
                                                    void           *userInfo )
{
    fm_status        err;

    err = SendDfeEventInd(eventInfo, userInfo, FM10000_SERDES_EVENT_DFE_ICAL_COMPLETE_IND);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeReadEyeH
 * \ingroup intSerDesDfe
 *
 * \desc            Action reading the height of the eye.
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
fm_status fm10000SerDesDfeReadEyeH(fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_status        err;
    fm_switch       *switchPtr;
    fm10000_lane    *pLaneExt;
    fm_int           serDes;
    fm_int           sw;
    fm_int           eyeHeight;
    fm_int           eyeHeightMv;

    FM_NOT_USED(eventInfo);

    pLaneExt  = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;

    err = fm10000SerdesGetEyeHeight(sw, serDes, &eyeHeight, &eyeHeightMv);

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeReadEyeW
 * \ingroup intSerDesDfe
 *
 * \desc            Action reading the width  of the eye.
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
fm_status fm10000SerDesDfeReadEyeW(fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_status     err;

    FM_NOT_USED(eventInfo);

    err = FM_OK;




    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeConfigureEyeW
 * \ingroup intSerDesDfe
 *
 * \desc            Action configuring the serdes to read the width of the eye.
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
fm_status fm10000SerDesDfeConfigureEyeW(fm_smEventInfo *eventInfo,
                                        void           *userInfo )
{
    fm_status     err;

    FM_NOT_USED(eventInfo);

    err = FM_OK;




    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeConfigureEyeH
 * \ingroup intSerDesDfe
 *
 * \desc            Action configuring the serdes to read the height of the eye.
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
fm_status fm10000SerDesDfeConfigureEyeH(fm_smEventInfo *eventInfo,
                                        void           *userInfo)
{
    fm_status     err;

    FM_NOT_USED(eventInfo);

    err = FM_OK;




    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeDontSaveRecord
 * \ingroup intSerDesDfe
 *
 * \desc            Action setting the flag to do not save the record for the
 *                  current transaction. This tipically is used with timeouts
 *                  and periodical events.
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
fm_status fm10000SerDesDfeDontSaveRecord(fm_smEventInfo *eventInfo,
                                         void           *userInfo )
{
    FM_NOT_USED(userInfo);

    eventInfo->dontSaveRecord = TRUE;

    return FM_OK;

}




/*****************************************************************************/
/** fm10000SerDesDfeRestartStopTuningTimer
 * \ingroup intSerDesDfe
 *
 * \desc            Action restarting the timer used during the DFE halting
 *                  sequence.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeRestartStopTuningTimer(fm_smEventInfo *eventInfo,
                                                 void           *userInfo )
{
    fm_status        err;
    fm10000_laneDfe *pLaneDfe;

    FM_NOT_USED(eventInfo);

    err = FM_OK;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;



    if (pLaneDfe->retryCntr > 0)
    {

        err = fm10000SerDesDfeStartTimeoutTimerShrt(eventInfo,userInfo);
    }
    else
    {


        err = fm10000SerDesDfeStartTimeoutTimerStopTuning(eventInfo,userInfo);
    }

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeProcessICalTimeout
 * \ingroup intSerDesDfe
 *
 * \desc            Conditional transition callback processing timeout events
 *                  during iCal tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeProcessICalTimeout(fm_smEventInfo *eventInfo,
                                             void           *userInfo,
                                             fm_int         *nextState )
{
    fm_status        err;
    fm_status        err2;
    fm_switch       *switchPtr;
    fm10000_lane    *pLaneExt;
    fm10000_laneDfe *pLaneDfe;
    fm_int           serDes;
    fm_int           sw;
    fm_bool          iCalInProgress;
    fm_bool          iCalSuccessful;


    pLaneExt  = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    err = FM_OK;
    err2 = FM_OK;
    iCalInProgress = FALSE;
    iCalSuccessful = FALSE;

    if (pLaneDfe->cycleCntr == 0)
    {


        err = fm10000SerDesDfeSendTuningStartedInd (eventInfo,userInfo);
    }

    if (err == FM_OK)
    {

        err = fm10000SerdesDfeTuningGetICalStatus(sw,serDes,&iCalInProgress);
    }


    if ( err == FM_OK && !iCalInProgress)
    {
        err = fm10000SerdesDfeTuningCheckICalConvergence(sw, serDes, &iCalSuccessful);

        if (err == FM_OK)
        {
            if (iCalSuccessful)
            {


                fm10000SerdesSaveICalTuningStatsInfo(sw,serDes,pLaneDfe->cycleCntr + 1);
                fm10000SerdesSaveICalTuningDelayInfo(sw,serDes);
                pLaneDfe->refTimeMs = fm10000SerdesGetTimestampMs();
                pLaneDfe->cycleCntr = -1;

                if (pLaneExt->dfeMode != FM_DFE_MODE_ICAL_ONLY)
                {
                    *nextState = FM10000_SERDES_DFE_STATE_WAIT_PCAL;

                    err = fm10000SerdesDfeTuningStartPCalSingleExec(sw, serDes);

                    pLaneExt->dfeExt.dfeTuningStat = 0x02;
                }
                else
                {


                    if (pLaneDfe->dfeDebounceTime >= 0)
                    {
                        *nextState = FM10000_SERDES_DFE_STATE_DEBOUNCE;
                        err = fm10000SerDesDfeStartTimeoutTimerDebounce(eventInfo, userInfo);
                    }
                    else
                    {
                        *nextState = FM10000_SERDES_DFE_STATE_EYE_H_DELAY;
                        err = fm10000SerDesDfeReadEyeH(eventInfo,userInfo);

                        if (err == FM_OK)
                        {
                            err = fm10000SerDesDfeClrCycleCntr(eventInfo,userInfo);
                        }

                        if (err == FM_OK)
                        {
                            err = fm10000SerDesDfeStartTimeoutTimerAdaptive(eventInfo, userInfo);
                        }

                        if (err == FM_OK)
                        {
                            pLaneExt->dfeExt.sendDfeComplete = TRUE;
                        }
                    }

                    pLaneExt->dfeExt.dfeTuningStat = 0x06;
                }

                if (err == FM_OK)
                {
                    err = fm10000SerDesDfeSendICalTuningCompleteInd(eventInfo,userInfo);
                }
            }
            else
            {

                pLaneDfe->cycleCntr = FM10000_SERDES_ICAL_TUNING_MAX_CYCLES;
            }
        }
    }


    if (++pLaneDfe->cycleCntr > FM10000_SERDES_ICAL_TUNING_MAX_CYCLES )
    {
        err = fm10000SerDesDfeStopTuning(eventInfo,userInfo);

        if (err == FM_OK)
        {
            *nextState = FM10000_SERDES_DFE_STATE_STOP_TUNING;

            err2 = fm10000SerDesDfeStartTimeoutTimerStopTuning(eventInfo, userInfo);
        }
    }
    else
    {

        err2 = fm10000SerDesDfeStartTimeoutTimerShrt( eventInfo, userInfo );
    }

    if (*nextState == FM10000_SERDES_DFE_STATE_WAIT_ICAL)
    {

        eventInfo->dontSaveRecord = TRUE;
    }



    err = (err != FM_OK) ? err : err2;

    return err;

}




/*****************************************************************************/
/** fm10000SerDesDfeProcessPCalTimeout
 * \ingroup intSerDesDfe
 *
 * \desc            Conditional transition callback processing timeout events
 *                  during pCal tuning.
 *
 * \param[in]       eventInfo is a pointer the generic event descriptor.
 *
 * \param[in]       userInfo is pointer a purpose specific event descriptor
 *                  (must be casted to ''fm10000_dfeSmEventInfo'')
 *
 * \param[out]      nextState is a pointer to a caller-allocated area where
 *                  this function will return the state the state machine
 *                  will transition to
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SerDesDfeProcessPCalTimeout(fm_smEventInfo *eventInfo,
                                             void           *userInfo,
                                             fm_int         *nextState )
{
    fm_status        err;
    fm_status        err2;
    fm_switch       *switchPtr;
    fm10000_lane    *pLaneExt;
    fm10000_laneDfe *pLaneDfe;
    fm_int           serDes;
    fm_int           sw;
    fm_bool          pCalInProgress;


    pLaneExt  = ((fm10000_dfeSmEventInfo *)userInfo)->laneExt;
    serDes    = pLaneExt->serDes;
    switchPtr = ((fm10000_dfeSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    pLaneDfe  = ((fm10000_dfeSmEventInfo *)userInfo)->laneDfe;

    err = FM_OK;
    err2 = FM_OK;

    err = fm10000SerdesDfeTuningGetPCalStatus(sw,serDes,&pCalInProgress);

    if (err == FM_OK && !pCalInProgress)
    {
        if (pLaneDfe->dfeAdaptive)
        {

            err = fm10000SerdesDfeTuningStartPCalContinuous(sw, serDes);
        }


        if (!pLaneExt->dfeExt.pCalKrMode)
        {
            pLaneExt->dfeExt.dfeTuningStat = 0x0a;


            fm10000SerdesSavePCalTuningStatsInfo(sw,serDes,pLaneDfe->cycleCntr + 1);
            fm10000SerdesSavePCalTuningDelayInfo(sw,serDes);
        }
        else
        {
            pLaneExt->dfeExt.dfeTuningStat = 0x08;
        }

        if (pLaneDfe->dfeDebounceTime >= 0)
        {
            *nextState = FM10000_SERDES_DFE_STATE_DEBOUNCE;
            err = fm10000SerDesDfeStartTimeoutTimerDebounce(eventInfo, userInfo);
        }
        else
        {
            *nextState = FM10000_SERDES_DFE_STATE_EYE_H_DELAY;
            err = fm10000SerDesDfeReadEyeH(eventInfo,userInfo);

            if (err == FM_OK)
            {
                err = fm10000SerDesDfeClrCycleCntr(eventInfo,userInfo);
            }

            if (err == FM_OK)
            {
                err = fm10000SerDesDfeStartTimeoutTimerShrt( eventInfo, userInfo );
            }

            if (err == FM_OK)
            {
                pLaneExt->dfeExt.sendDfeComplete = TRUE;
            }
        }
    }
    else
    {

        if (++pLaneDfe->cycleCntr > FM10000_SERDES_DFE_TUNING_MAX_CYCLES )
        {

            err = fm10000SerDesDfeStopTuning(eventInfo,userInfo);

            if (err == FM_OK)
            {
                *nextState = FM10000_SERDES_DFE_STATE_STOP_TUNING;


                err2 = fm10000SerDesDfeStartTimeoutTimerStopTuning(eventInfo, userInfo);
            }
        }
        else
        {

            err2 = fm10000SerDesDfeStartTimeoutTimerShrt( eventInfo, userInfo );


            eventInfo->dontSaveRecord = TRUE;
        }
    }

    err = (err != FM_OK)? err : err2;

    return err;

}








