/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_state_machine.h
 * Creation Date:   October 8, 2013
 * Description:     Declarations related to the Generic State Machine
 *                  implementation
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
 
#ifndef __FM_FM_STATE_MACHINE_H
#define __FM_FM_STATE_MACHINE_H

#include <fm_alos_time.h>
#include <fm_alos_lock.h>

#define FM_SMTYPE_UNSPECIFIED -1
#define FM_STATE_UNSPECIFIED  -1
#define FM_EVENT_UNSPECIFIED  -1

/***********************************************************/
/** \ingroup typeStruct
 * Definition of the generic State Machine Event info header 
 * type.
 * Each individual state machine event info shall have 
 * this structure as header. 
 ***********************************************************/
typedef struct _fm_smEventInfo
{
    /** State Machine type. */
    fm_int     smType;

    /** Event Id. */
    fm_int     eventId;

    /** Caller lock. */
    fm_lock    *lock;

    /** Bypass current transaction record registration flag. */
    fm_bool     dontSaveRecord;
    
    /** Event source State Machine type. */
    fm_int      srcSmType;

} fm_smEventInfo;


/**************************************************/
/** \ingroup typeStruct
 * Structure describing a state machine transition
 **************************************************/
typedef struct _fm_smTransition
{
    /** event info */
    fm_smEventInfo eventInfo;

    /** event timestamp */
    fm_timestamp   eventTime;

    /** current state */
    fm_int         currentState;

    /** next state  */
    fm_int         nextState;

    /** caller-specific event data   */
    fm_voidptr     recordData;

    /** user ID for the state machine instance */
    fm_int         smUserID;

    /** transition status */
    fm_status      status;

} fm_smTransitionRecord;


/**************************************************/
/** \ingroup typeScalar
 * Definition of State Machine Transition callback
 **************************************************/
typedef fm_status (*fm_smTransitionCallback)( fm_smEventInfo *eventInfo,
                                              void           *userInfo );



/**************************************************/
/** \ingroup typeScalar
 * Definition of State Machine condition callback
 **************************************************/
typedef fm_status (*fm_smConditionCallback)( fm_smEventInfo *eventInfo, 
                                             void           *userInfo,
                                             fm_int         *nextState );


/**************************************************/
/** \ingroup typeScalar
 * Definition of State Machine transition log 
 * callback 
 **************************************************/
typedef fm_status (*fm_smTransitionLogCallback)(fm_smTransitionRecord *record);


/****************************************************************/
/** \ingroup typeScalar
 * Definition of the State Machine Transition table entry type 
 ****************************************************************/
typedef struct _fm_smTransitionEntry
{
    /** flag indicating whether or not this entry is used */
    fm_bool                 used;

    /** callback for direct transitions */
    fm_smTransitionCallback transitionCallback;

    /** callback for conditional transitions */
    fm_smConditionCallback  conditionCallback;

    /** next state */
    fm_int                  nextState;

} fm_smTransitionEntry;


/****************************************************************************/
/** \ingroup typeEnum
 *  Event timestamping mode in the transition history buffers
 ****************************************************************************/
typedef enum
{
    /** Event timestamp is the system up time. */
    FM_GSME_TIMESTAMP_MODE_SYSUPTIME = 0,

    /** Event timestamp is the absolute time. */
    FM_GSME_TIMESTAMP_MODE_ABSOLUTE,

    /** Event timestamp is time since the history buffers were last
     *  cleared. */
    FM_GSME_TIMESTAMP_MODE_SINCE_CLEAR,

    /** UNPUBLISHED: Max value, do not move. */
    FM_GSME_TIMESTAMP_MODE_MAX

} fm_smTimestampMode;


/**************************************************/
/** \ingroup typeScalar
 * Definition of the State Machine handle
 **************************************************/
typedef void *fm_smHandle;


/* Declaration of a function to initialize the state machine engine */
fm_status fmInitStateMachineEngine( fm_timestamp       *initTime,
                                    fm_smTimestampMode  mode );
                                           
/* Declaration of a function to register a new state machine type */
fm_status fmRegisterStateTransitionTable( fm_int smType,           
                                          fm_int nrStates,        
                                          fm_int nrEvents,        
                                          fm_smTransitionEntry      **stt,
                                          fm_smTransitionLogCallback  log,
                                          fm_bool okIfRegistered );

 
/* Declaration of a function to de-register a state machine type */
fm_status fmUnregisterStateTransitionTable( fm_int  smType, 
                                            fm_bool skipIfUsed );

/* Declaration of a function to create a state machine */
fm_status fmCreateStateMachine( fm_int       smUserID,
                                fm_int       historySize,
                                fm_int       recordDataSize,
                                fm_smHandle *handle );

/* Declaration of a function to delete a state machine */
fm_status fmDeleteStateMachine( fm_smHandle  handle );

/* Declaration of a function to start a state machine */
fm_status fmStartStateMachine( fm_smHandle   handle,
                               fm_int        smType,
                               fm_int        initState );

/* Declaration of a function to create and start and state machine */
fm_status fmCreateAndStartStateMachine( fm_int       smUserID,
                                        fm_int       historySize, 
                                        fm_int       recordDataSize,
                                        fm_int       smType,
                                        fm_int       initState, 
                                        fm_smHandle *handle );

/* Declaration of a function to close a state machine */
fm_status fmStopStateMachine( fm_smHandle  handle );

/* Declaration of a function to notify a state machine event */
fm_status fmNotifyStateMachineEvent( fm_smHandle     handle,
                                     fm_smEventInfo *eventInfo,
                                     void           *userInfo,
                                     void           *dataToLog );

/* Declaration of a function to return the current state of a state machine */
fm_status fmGetStateMachineCurrentState( fm_smHandle  handle,
                                         fm_int      *state );

/* Declaration of a function to get the last 'n' state machine transitions */
fm_status fmGetStateTransitionHistory( fm_smHandle            handle,
                                       fm_int                *nrRecords,
                                       fm_smTransitionRecord *records );

/* Declaration of a function to clear the state machine transitions log */
fm_status fmClearStateTransitionHistory( fm_smHandle handle );

/* Declaration of a function to change the state machine transition log size */
fm_status fmChangeStateTransitionHistorySize( fm_smHandle handle,
                                              fm_int      historySize );

#endif /* __FM_FM_STATE_MACHINE_H */
