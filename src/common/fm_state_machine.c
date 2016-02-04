/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_state_machine.c
 * Creation Date:   October 8, 2013
 * Description:     Generic State Machine implementation
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

#include <fm_sdk_int.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define STATE_MACHINE_MAGIC_NUMBER      0x75A9156F

#define GET_TABLE_ENTRY_PTR( table, i, j, cols ) ((table)+(i)*(cols)+(j))
#define GET_TABLE_ENTRY( table, i, j, cols )    *((table)+(i)*(cols)+(j))
#define TAKE_GSME_LOCK()    fmCaptureLock( &smEngine.lock, FM_WAIT_FOREVER )
#define DROP_GSME_LOCK()    fmReleaseLock( &smEngine.lock )

#define FLAG_TAKE_GSME_LOCK()   \
{                               \
    TAKE_GSME_LOCK();           \
    gsmeLockTaken = TRUE;       \
}
                                 
#define FLAG_DROP_GSME_LOCK()   \
{                               \
    DROP_GSME_LOCK();           \
    gsmeLockTaken = FALSE;      \
}

#define TAKE_CALLER_LOCK( info ) fmCaptureLock( (info)->lock, FM_WAIT_FOREVER )
#define DROP_CALLER_LOCK( info ) fmReleaseLock( (info)->lock )

#define FLAG_TAKE_CALLER_LOCK( info )   \
{                                       \
    TAKE_CALLER_LOCK( info );           \
    callerLockTaken = TRUE;             \
}
                                 
#define FLAG_DROP_CALLER_LOCK( info )   \
{                                       \
    DROP_CALLER_LOCK( info );           \
    callerLockTaken = FALSE;            \
}


/* internal data structure representing a registered state machine type */
typedef struct _fm_stateMachineType
{
    /* state machine type */
    fm_int                      smType;
                                
    /* number of states */      
    fm_int                      nrStates;
                                
    /* number of states */      
    fm_int                      nrEvents;

    /* pointer to the state machine transition table */
    fm_smTransitionEntry       *smTransitionTable;

    /* default action callback */
    fm_smTransitionLogCallback  logCallback;

    /* linked list node */
    FM_DLL_DEFINE_NODE( _fm_stateMachineType, next, prev );

} fm_stateMachineType;



/* internal generic state machine data structure */
typedef struct _fm_stateMachine
{
    /* magic number */
    fm_uint32              smMagicNumber;

    /* reference value */
    fm_uint32              smRefValue;

    /* user's ID for this state machine */
    fm_int                 smUserID;

    /* pointer to the state machine type structure */
    fm_stateMachineType   *type;

    /* current state */
    fm_int                 curState;

    /* pointer to the state transition history */
    fm_smTransitionRecord *smTransitionHistory;

    /* Size of the state transition history */
    fm_int                 transitionHistorySize;

    /* event data buffer */
    fm_byte               *recordData;

    /* Caller-specific event data size to be saved in the transition record */
    fm_int                 recordDataSize;

    /* Number of state transitions occurred on this state machine */
    fm_int                 nrTransitions;

    /* timestamp at initialization time */
    fm_timestamp           initTimeStamp;

    /* linked list node */
    FM_DLL_DEFINE_NODE( _fm_stateMachine, next, prev );

} fm_stateMachine;



/* internal data structure representing the state machine engine */
typedef struct _fm_stateMachineEngine 
{
    /* flag indicating that the State Machine initialization was successful */
    fm_bool            init; 

    /* number of registered state machine types  */
    fm_int             nrRegisteredTypes;

    /* reference value */
    fm_uint32          refValue;

    /* GSME lock */
    fm_lock            lock;

    /* Init time to be used as reference for event time stamping */
    fm_timestamp       initTime;

    fm_smTimestampMode tsMode;

    /* linked list of registered state machine types */
    FM_DLL_DEFINE_LIST( _fm_stateMachineType, smTypeHead, smTypeTail );

    /* linked list of existing state machines */
    FM_DLL_DEFINE_LIST( _fm_stateMachine, smHead, smTail );

} fm_stateMachineEngine;



/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_status CreateStateMachine( fm_int                 smUserID,
                                     fm_int                 historySize,
                                     fm_int                 recordDataSize,
                                     fm_smHandle           *handle );

static fm_status StartStateMachine( fm_smHandle            handle,
                                    fm_int                 smType,
                                    fm_int                 initState );

static fm_status InitRecordDataPtr( fm_stateMachine       *sm, 
                                    fm_int                 historySize );

static fm_status SaveTransitionRecord( fm_stateMachine       *sm, 
                                       fm_smTransitionRecord *record,
                                       void                  *recordData );

static fm_stateMachineType *SearchRegisteredStateMachineTypes( fm_int smType );

static fm_stateMachine     *SearchExistingStateMachinesByType( fm_int smType );

static fm_status ClearStateTransitionHistory( fm_stateMachine *sm );

static fm_status SaveEventTime( fm_stateMachine *sm, fm_timestamp *ts );

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static fm_stateMachineEngine smEngine = 
{ 
    FALSE,
};

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** ClearStateTransitionHistory
 * \ingroup intStateMachine
 *
 * \desc            Internal version of ''fmClearStateTransitionHistory''. It
 *                  clears the state transition history buffer, but skips
 *                  the initial sanity checks
 * 
 * \param[in]       sm is the pointer to a caller-allocated structure
 *                  representing the state machine instance whose history
 *                  buffer needs to be cleared
 *
 * \return          FM_OK 
 *****************************************************************************/
static fm_status ClearStateTransitionHistory( fm_stateMachine *sm )
{
    sm->nrTransitions = 0;
    if ( smEngine.tsMode == FM_GSME_TIMESTAMP_MODE_SINCE_CLEAR )
    {
        fmGetTime( &sm->initTimeStamp );
    }
    return FM_OK;

}   /* end ClearStateTransitionHistory */


/*****************************************************************************/
/** fmCreateStateMachine
 * \ingroup intStateMachine
 *
 * \desc            Internal function to create a state machine by allocating
 *                  all necessary internal data structures. For the state 
 *                  machine to become operational, it needs to be bound to
 *                  a registered state machine type and therefore to a state
 *                  machine transition table by calling ''fmStartStateMachine''
 * 
 * \param[in]       smUserID The user's ID for this state machine
 * 
 * \param[in]       historySize is the number of records of the transition
 *                  historybuffer for this state machine. The state machine
 *                  engine will keep a buffer containing the most recente
 *                  historySize entries of type ''fm_smTransitionRecord'' for a
 *                  given state machine. It can be zero to disable transition
 *                  history tracking
 * 
 * \param[in]       recordDataSize is the size in bytes of the caller-provided
 *                  event data buffer that is saved by GSME in a transition
 *                  record upon event notification
 * 
 * \param[out]      handle is a pointer to a caller-allocated area where this
 *                  function will return a handle for this state machine. The
 *                  caller is required to use this handle for any subsequent
 *                  operation on the same state machine
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure
 *                  while creating one of the state machine internal structures
 * 
 *****************************************************************************/
static fm_status CreateStateMachine( fm_int       smUserID,
                                     fm_int       historySize,
                                     fm_int       recordDataSize,
                                     fm_smHandle *handle )
{
    fm_status        status;
    fm_stateMachine *sm;
    fm_int           recordSize;
    fm_int           eventBufferSize;

    /* validate input arguments */
    if ( historySize < 0  || handle == NULL )
    {
        sm     = NULL;
        status = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /* Allocate memory for the internal state machine structuree */
    sm = fmAlloc( sizeof(fm_stateMachine ));
    if ( sm == NULL )
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* clear the object */
    FM_MEMSET_S( sm, sizeof(fm_stateMachine), 0, sizeof(fm_stateMachine) );

    if ( historySize > 0 )
    {
        /* each record will have a caller-specific data buffer piggybacked */
        recordSize = historySize * sizeof(fm_smTransitionRecord);

        /* allocate the state transition history table */
        sm->smTransitionHistory = fmAlloc( recordSize );
        if ( sm->smTransitionHistory == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }

        /* clear the object */
        FM_MEMSET_S( sm->smTransitionHistory, recordSize, 0, recordSize ); 

        /* allocate the data buffer for the transition records */
        eventBufferSize = recordDataSize * historySize;
        sm->recordData = fmAlloc( eventBufferSize );
        if ( sm->recordData == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }

        /* clear the object */
        FM_MEMSET_S(sm->recordData, eventBufferSize, 0, eventBufferSize); 

        InitRecordDataPtr( sm, historySize );

    }   /* end if ( historySize > 0 ) */

    /* now fill out the state machine data structure */
    sm->smMagicNumber          = STATE_MACHINE_MAGIC_NUMBER;
    sm->smUserID               = smUserID;
    sm->transitionHistorySize  = historySize;
    sm->recordDataSize         = recordDataSize;
    sm->initTimeStamp          = smEngine.initTime;

    /* Currently it's not bound to any state machine type */
    sm->type                    = NULL;

    /* add it to the list of existing state machines */
    FM_DLL_INSERT_LAST( &smEngine, smHead, smTail, sm, next, prev );

    /* the handle is the pointer to this state machine structure */
    *(fm_stateMachine **)handle = sm;

    /* State machine created successfully */
    sm->smRefValue = smEngine.refValue++;
    status         = FM_OK;

ABORT:
    /* Do some clean up if the there was an error */
    if ( status != FM_OK )
    {
        /* Was the state machine structure created? */
        if ( sm != NULL )
        {
            /* Yes, was the state transition table created? */
            if ( sm->smTransitionHistory != NULL )
            {
                /* Yes, then free it */
                fmFree( sm->smTransitionHistory );
            }

            /* Was the event data buffer created? */
            if ( sm->recordData != NULL )
            {
                /* Yes, then free it */
                fmFree( sm->recordData );
            }


            fmFree( sm );

        }   /* end if ( sm != NULL) */

    }   /* end if ( status != FM_OK ) */

    return status;

}   /* end CreateStateMachine */


/*****************************************************************************/
/** StartStateMachine
 * \ingroup intStateMachine
 *
 * \desc            Internal function to puts a state machine in operational
 *                  state by binding it to a registered  state machine type
 *                  (and therefore to a State Machine Transition table) and
 *                  by setting its initial state
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type 
 * 
 * \param[in]       initState is the initial state for this state machine
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE if the specified type does not
 *                  appear to have been registered 
 *                   
 * \return          FM_ERR_BOUND_STATE_MACHINE if the operation is attempted
 *                  on a state machine that is already bound to a state
 *                  transition type. ''fmStopStateMachineMachine'' should be
 *                  invoked before a new binding can be created.
 *****************************************************************************/
static fm_status StartStateMachine( fm_smHandle handle,
                                    fm_int      smType,
                                    fm_int      initState )
{
    fm_status              status;
    fm_stateMachineType   *type;
    fm_stateMachine       *sm;
    fm_smTransitionRecord  record;
    fm_voidptr             recordData;

    /* consistency check on the handle and other input arguments */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* Is this an unbound state machine? */
    if ( sm->type != NULL )
    {
        /* no, it must be closed first */
        status = FM_ERR_BOUND_STATE_MACHINE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* make sure the indicated type is registered */
    type = SearchRegisteredStateMachineTypes( smType );
    if ( type == NULL )
    {
        /* no, it must be closed first */
        status = FM_ERR_STATE_MACHINE_TYPE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* We confirmed the type is registered, we can now check the initState */
    if ( initState < 0 || initState >= type->nrStates )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    sm->type     = type;
    sm->curState = initState;

    /* fill out the transition record, with a pseudo-event */
    SaveEventTime( sm, &record.eventTime );
    record.eventInfo.smType  = type->smType;
    record.eventInfo.eventId = FM_EVENT_UNSPECIFIED;
    record.smUserID          = sm->smUserID;
    record.currentState      = FM_STATE_UNSPECIFIED;
    record.nextState         = initState;
    record.status            = FM_OK;

    recordData = fmAlloc( sm->recordDataSize );
    if ( recordData )
    {
        FM_MEMSET_S( recordData, sm->recordDataSize, 0, sm->recordDataSize );
        status = SaveTransitionRecord( sm, &record, recordData );
        fmFree( recordData );
    }
    else
    {
        status = FM_ERR_NO_MEM;
    }

    /* successful if we got here */
    status = FM_OK;

ABORT:
    return status;

}   /* end StartStateMachine */


/*****************************************************************************/
/** SearchRegisteredStateMachineTypes
 * \ingroup intStateMachine
 *
 * \desc            Internal function that searches the list of registered
 *                  state machine type objects looking for one matching a
 *                  given type
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type 
 *
 * \return          Pointer to the state machine type object, if found. NULL
 *                  otherwise
 *****************************************************************************/
static fm_stateMachineType *SearchRegisteredStateMachineTypes( fm_int smType )
{
    fm_stateMachineType *type;

    type = FM_DLL_GET_FIRST( &smEngine, smTypeHead );
    while ( type != NULL && type->smType != smType )
    {
        type = FM_DLL_GET_NEXT( type, next );
    }

    return type;

}   /* end SearchRegisteredStateMachineTypes */



/*****************************************************************************/
/** SearchExistingStateMachinesByType
 * \ingroup intStateMachine
 *
 * \desc            Internal function that searches the list of existing
 *                  state machine objects looking for one matching a given
 *                  type. If multiple state machines exist this function
 *                  returns at the first the pointer to the first one found
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *
 * \return          Pointer to the state machine object, if found. NULL
 *                  otherwise
 *****************************************************************************/
static fm_stateMachine *SearchExistingStateMachinesByType( fm_int smType )
{
    fm_stateMachine *sm;

    sm = FM_DLL_GET_FIRST( &smEngine, smHead );
    while ( sm != NULL && ( sm->type == NULL || sm->type->smType != smType ) )
    {
        sm = FM_DLL_GET_NEXT( sm, next );
    }

    return sm;

}   /* end SearchExistingStateMachinesByType */


/*****************************************************************************/
/** SaveTransitionRecord
 * \ingroup intStateMachine
 *
 * \desc            Save a transition record for this state machine.
 *
 * \param[in]       sm is the pointer to a caller-allocated structure
 *                  representing the state machine instance for which where
 *                  this transition record needs to be saved.
 * 
 * \param[out]      record is the pointer to a caller-allocated structure
 *                  containing the transition record to be saved.
 * 
 * \param[in]       recordData points to the event data.
 *                  
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status SaveTransitionRecord( fm_stateMachine       *sm, 
                                       fm_smTransitionRecord *record,
                                       void                  *recordData )
{
    fm_int                     recordIdx;
    fm_status                  status;
    fm_smTransitionLogCallback log;
    fm_smTransitionRecord      *recordPtr;


    /* record the transition whether or not it was successful */
    /* but only if transaction history tracking is enabled     */
    if ( sm->transitionHistorySize > 0 )
    {
        recordIdx  = sm->nrTransitions % sm->transitionHistorySize;
        recordPtr  = &sm->smTransitionHistory[recordIdx];
        *recordPtr = *record;
        recordPtr->recordData = sm->recordData + recordIdx*sm->recordDataSize;
        FM_MEMCPY_S( recordPtr->recordData, 
                     sm->recordDataSize, 
                     recordData,
                     sm->recordDataSize );
        sm->nrTransitions++;
    }
    else
    {
        recordPtr = record;
        recordPtr->recordData = recordData;
    }

    /* now log this transition */
    log = sm->type->logCallback;
    if ( log != NULL )
    {
        status = log( recordPtr );
    }
    else
    {
        /* empty list, just log the event and the state transition */
        FM_LOG_DEBUG( FM_LOG_CAT_STATE_MACHINE, 
                      "Event %d occurred on State Machine %d of type %d - "
                      "Current State is %d, Next State is %d\n",
                      record->eventInfo.eventId,
                      sm->smUserID,
                      record->eventInfo.smType,
                      record->currentState,
                      record->nextState );

        status = FM_OK;
    }

    return status;

}   /* end SaveTransitionRecord */


/*****************************************************************************/
/** InitRecordDataPtr
 * \ingroup intStateMachine
 *
 * \desc            This function initializes the data pointer for the state
 *                  transition records
 *
 * \param[in]       sm is the pointer to a caller-allocated structure
 *                  representing the state machine instance whose history
 *                  buffer needs to be initialized
 * 
 * \param[in]       historySize is the number of records of the transition
 *                  history buffer for this state machine. 
 *
 * \return          FM_OK
 *****************************************************************************/
static fm_status InitRecordDataPtr( fm_stateMachine *sm, fm_int historySize )
{
    fm_int                  idx;
    fm_byte                *bufStart;
    fm_smTransitionRecord  *record;

    record   = sm->smTransitionHistory;
    bufStart = sm->recordData;

    /* the event data buffers are attched to the transition record array */
    for ( idx = 0 ; idx < historySize ; idx++ )
    {
        record->recordData = bufStart;
        bufStart++;
        record++;
    }

    return FM_OK;

}   /* end InitRecordDataPtr */


/*****************************************************************************/
/** SaveEventTime
 * \ingroup intStateMachine
 *
 * \chips           FM10000
 *
 * \desc            Save the event time depending on the desired value of the
 *                  GSME timestamp mode
 *
 * \param[in]       sm is the pointer to a caller-allocated structure
 *                  representing the state machine instance for which where
 *                  this transition record needs to be saved.
 * 
 * \param[in,out]   ts is the pointer to a caller allocated variable where
 *                  this function will return the event timestamp
 * 
 * \return          FM_OK
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if an input pointer is NULL
 *
 *****************************************************************************/
static fm_status SaveEventTime( fm_stateMachine *sm, fm_timestamp *ts )
{
    if ( sm == NULL || ts == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    fmGetTime( ts );
    switch( smEngine.tsMode )
    {
        case FM_GSME_TIMESTAMP_MODE_ABSOLUTE:
            break;

        case FM_GSME_TIMESTAMP_MODE_SINCE_CLEAR:
            fmSubTimestamps( ts, &sm->initTimeStamp, ts );
            break;

        case FM_GSME_TIMESTAMP_MODE_SYSUPTIME:
        default:
            fmSubTimestamps( ts, &smEngine.initTime, ts );
            break;
    }
    
    return FM_OK;

} /* end SaveEventTime */

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmInitStateMachineEngine
 * \ingroup intStateMachine
 *
 * \desc            Function to initialize the State Machine Engine
 * 
 * \param[in]       initTime pointer to a caller-allocated variable indicating
 *                  the timestamp to be used by GSME to timestamp events
 * 
 * \param[in]       mode is the event timestamping mode. See
 *                  ''fm_smTimestampMode''
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 *****************************************************************************/
fm_status fmInitStateMachineEngine( fm_timestamp       *initTime,
                                    fm_smTimestampMode  mode )
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "Initializing GSME\n" );

    status = fmCreateLock( "GSME Lock", &smEngine.lock );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );

    smEngine.init              = TRUE;
    smEngine.nrRegisteredTypes = 0;
    smEngine.refValue          = 0;
    smEngine.smTypeHead        = NULL;
    smEngine.smTypeTail        = NULL;
    smEngine.smHead            = NULL;
    smEngine.smTail            = NULL;
    smEngine.initTime          = *initTime;
    smEngine.tsMode            = mode;

ABORT:
    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmInitStateMachineEngine() */


/*****************************************************************************/
/** fmRegisterStateTransitionTable
 * \ingroup intStateMachine
 *
 * \desc            This function registers a new state machine type with 
 *                  GSME by associating a caller-provided state machine type
 *                  with a State Transition Table.
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type 
 * 
 * \param[in]       nrStates is number of states for this state machine. The
 *                  range of valid state IDs is assumed to be (0, nrStates-1)
 * 
 * \param[in]       nrEvents is number of events for this state machine. The
 *                  range of valid event IDs is assumed to be (0, nrEvents-1)
 * 
 * \param[in]       stt is a caller-provided array of nrStates
 *                  pointers each pointing to an array of nrEvents State
 *                  Transition descriptors for a given state.
 * 
 * \param[in]       log is a callback function used by the state
 *                  machine engine to print a log message describing the
 *                  transition
 * 
 * \param[in]       okIfRegistered is a boolean variable indicating how to
 *                  handle the case where this state transition table is
 *                  already registered: if TRUE the operation will be silently
 *                  skipped; if FALSE this function will return an error
 *
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure
 *                  while creating one of the state machine internal structures
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE if this state machine type is
 *                  already registered and okIfRegistered is set to FALSE
 *****************************************************************************/
fm_status fmRegisterStateTransitionTable( fm_int smType,           
                                          fm_int nrStates,        
                                          fm_int nrEvents,        
                                          fm_smTransitionEntry      **stt,
                                          fm_smTransitionLogCallback  log,
                                          fm_bool okIfRegistered )
{
    fm_stateMachineType *type = NULL;
    fm_status            status;
    fm_smTransitionEntry entry;
    fm_int               i;
    fm_int               j;
    fm_bool              gsmeLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "smType=%d nrStates=%d nrEvents=%d " 
                  "smTransitionTable=%p\n",
                  smType,
                  nrStates,
                  nrEvents,
                  (void *)stt );

    /* make sure it was initialized */
    if ( smEngine.init != TRUE )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* check the input arguments */
    if ( ( stt      == NULL ) ||
         ( nrStates  < 0    ) ||
         ( nrEvents  < 0    ) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    FLAG_TAKE_GSME_LOCK();

    /* see if this state machine type was already registered */
    type = SearchRegisteredStateMachineTypes ( smType );
    if ( type != NULL )
    {
        if ( okIfRegistered )
        {
            status = FM_OK;
            goto ABORT;
        }
        else
        {
            type = NULL;
            status = FM_ERR_STATE_MACHINE_TYPE;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }
    }

    /* allocate memory for the state machine type */
    type = ( fm_stateMachineType *)fmAlloc( sizeof(fm_stateMachineType) );
    if ( type == NULL )
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* allocate memory for the state transition table */
    type->smTransitionTable = 
        (fm_smTransitionEntry *)fmAlloc( sizeof(fm_smTransitionEntry) * 
                                         nrStates                     * 
                                         nrEvents );
    if ( type->smTransitionTable == NULL )
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* fill out the state machine type structure */
    type->smType      = smType;
    type->nrStates    = nrStates;
    type->nrEvents    = nrEvents;
    type->logCallback = log;

    /* now copy each entry one by one */
    for (i = 0 ; i < nrStates  ; i++)
    {
        for (j = 0 ; j < nrEvents ; j++)
        {
            /***********************************************************
             * If this entry isn't initialized, populate it as follows: 
             * - NextState is set to the current state (loop transition)
             * - action is set to the default action
             ***********************************************************/

            entry = stt[i][j];
            if ( entry.used == FALSE )
            {
                entry.used               = TRUE;
                entry.transitionCallback = NULL;
                entry.nextState          = i;
            }
            GET_TABLE_ENTRY(type->smTransitionTable, i, j, nrEvents) = entry;

        }   /* end for ( j = 0, ... ) */

    }   /* end for ( i = 0, ... ) */


    /* Add this new registered state machine type to the list */
    FM_DLL_INSERT_LAST( &smEngine, smTypeHead, smTypeTail, type, next, prev );
    smEngine.nrRegisteredTypes++;

    /* if we got here, we're ok */
    status = FM_OK;

ABORT:
    if ( status != FM_OK && type != NULL )
    {
        fmFree( type );
    }
    if ( gsmeLockTaken )
    {
        DROP_GSME_LOCK();
    }
    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmRegisterStateTransitionTable */


/*****************************************************************************/
/** fmUnregisterStateTransitionTable
 * \ingroup intStateMachine
 *
 * \desc            This function removes the association between a state
 *                  machine type and the State Transition Table previously
 *                  created using ''fmRegisterStateTransitionTable''
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type
 * 
 * \param[in]       skipIfUsed is a boolean variable indicating how to handle
 *                  the case where there is at least a state machine bound to
 *                  this state transition table: if TRUE the operation is
 *                  silently skipped; if FALSE this function will report an
 *                  error
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *****************************************************************************/
fm_status fmUnregisterStateTransitionTable( fm_int  smType,
                                            fm_bool skipIfUsed )
{
    fm_status            status;
    fm_stateMachineType *type;
    fm_stateMachine     *entry;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "smType=%d\n", smType );

    /* make sure GSME is initializd */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* see if this state machine type was already registered */
    type = SearchRegisteredStateMachineTypes( smType );
    if ( type != NULL )
    {
        /* see if there is an existing state machine bound to this type */
        entry = SearchExistingStateMachinesByType( smType );
        if ( entry != NULL )
        {
            if ( skipIfUsed )
            {
                status = FM_OK;
                goto ABORT;
            }

            /* yes there is one, flag it as an error */
            status = FM_ERR_STATE_MACHINE_TYPE;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }

        /* remove it from the list of state machine types */
        smEngine.nrRegisteredTypes--;
        FM_DLL_REMOVE_NODE( &smEngine, 
                            smTypeHead, 
                            smTypeTail, 
                            type, 
                            next, 
                            prev );

        /****************************************************
         * free the memory allocated for the state transition 
         * table and for the state machine type object itself
         ****************************************************/
        
        fmFree( type->smTransitionTable );
        fmFree( type );

        /* if we got here, we're ok */
        status = FM_OK;

    }   /* end if ( type != NULL ) */
    else
    {
        /* not a registered state machine type, silently ignore it */
        status = FM_OK;
    }


ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmUnregisterStateTransitionTable */



/*****************************************************************************/
/** fmCreateStateMachine
 * \ingroup intStateMachine
 *
 * \desc            This function creates a state machine by allocating all
 *                  necessary internal data structures. For the state 
 *                  machine to become operational, it needs to be bound to
 *                  a registered state transition table by calling
 *                  ''fmStartStateMachine''
 * 
 * \param[in]       smUserID The user's ID for this state machine
 * 
 * \param[in]       historySize is the size of the transition history buffer
 *                  for this state machine. The state machine engine will keep
 *                  a buffer containing the most recente historySize entries of
 *                  type ''fm_smTransitionRecord'' for a given state machine.
 *                  It can be zero to disable transition logging
 * 
 * \param[in]       recordDataSize is the size in bytes of the caller-provided
 *                  event data buffer that is saved by GSME in a transition
 *                  record upon event notification
 * 
 * \param[out]      handle is a pointer to a caller-allocated area where this
 *                  function will return a handle for this state machine. The
 *                  caller is required to use this handle for any subsequent
 *                  operation on the same state machine
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure
 *                  while creating one of the state machine internal structures
 *****************************************************************************/
fm_status fmCreateStateMachine( fm_int       smUserID,
                                fm_int       historySize,
                                fm_int       recordDataSize,
                                fm_smHandle *handle )
{
    fm_status        status;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "smUserID=%d historySize=%d\n",
                  smUserID,
                  historySize );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    status = CreateStateMachine( smUserID, 
                                 historySize, 
                                 recordDataSize, 
                                 handle );

    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmCreateStateMachine */


/*****************************************************************************/
/** fmStartStateMachine
 * \ingroup intStateMachine
 *
 * \desc            This function puts a state machine in operational state by
 *                  binding it to a registered  state machine type (and
 *                  therefore to a State Machine Transition table) and by
 *                  setting its initial state
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type 
 * 
 * \param[in]       initState is the initial state for this state machine
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE if the specified type does not
 *                  appear to have been registered 
 *                   
 * \return          FM_ERR_BOUND_STATE_MACHINE if the operation is attempted
 *                  on a state machine that is already bound to a state
 *                  transition type. ''fmStopStateMachine'' should be
 *                  invoked before a new binding can be created.
 *****************************************************************************/
fm_status fmStartStateMachine( fm_smHandle   handle,
                               fm_int        smType,
                               fm_int        initState )
{
    fm_status            status;


    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "handle=%p initState=%d\n", 
                  (void *)handle,
                  initState );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();
    status = StartStateMachine( handle, smType, initState );
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmStartStateMachine */


/*****************************************************************************/
/** fmCreateAndStartStateMachine
 * \ingroup intStateMachine
 *
 * \desc            This function creates a state machine by allocating all
 *                  necessary internal data structure and then puts it in
 *                  an operational state, by binding it to a registered
 *                  state transition table 
 * 
 * \param[in]       smUserID The user's ID for this state machine
 * 
 * \param[in]       historySize is the size of the transition history buffer
 *                  for this state machine. The state machine engine will keep
 *                  a buffer containing the most recente historySize entries of
 *                  type ''fm_smTransitionRecord'' for a given state machine.
 *                  It can be zero to disable transition logging
 * 
 * \param[in]       recordDataSize is the size in bytes of the caller-provided
 *                  event data buffer that is saved by GSME in a transition
 *                  record upon event notification
 * 
 * \param[in]       smType is a reserved key used by the caller to uniquely
 *                  identify the state machine type 
 * 
 * \param[in]       initState is the initial state for this state machine
 * 
 * \param[out]      handle is a pointer to a caller-allocated area where this
 *                  function will return a handle for this state machine. The
 *                  caller is required to use this handle for any subsequent
 *                  operation on the same state machine
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure
 *                  while creating one of the state machine internal structures
 *****************************************************************************/
fm_status fmCreateAndStartStateMachine( fm_int       smUserID,
                                        fm_int       historySize, 
                                        fm_int       recordDataSize,
                                        fm_int       smType,
                                        fm_int       initState, 
                                        fm_smHandle *handle ) 
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "smUserID=%d historySize=%d recordDataSize=%d "
                  "smType=%d initState=%d\n",
                  smUserID,
                  historySize,
                  recordDataSize,
                  smType,
                  initState );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();
    status = CreateStateMachine( smUserID, 
                                 historySize, 
                                 recordDataSize, 
                                 handle );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );

    status = StartStateMachine( *handle, smType, initState );

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmCreateAndStartStateMachine */


/*****************************************************************************/
/** fmStopStateMachine
 * \ingroup intStateMachine
 *
 * \desc            This function puts a state machine in a non-operational
 *                  state by unbinding it from the current transition table.
 *                  Existing internal state information and the transition
 *                  history buffer are also cleared
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *****************************************************************************/
fm_status fmStopStateMachine( fm_smHandle  handle )
{
    fm_status        status;
    fm_stateMachine *sm;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "handle=%p\n", (void *)handle );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* consistency check on the handle and other input arguments */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER ) 
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* unbind it from the registered type */
    sm->type = NULL;

    status = FM_OK;

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );


}   /* end fmStopStateMachine */


/*****************************************************************************/
/** fmDeleteStateMachine
 * \ingroup intStateMachine
 *
 * \desc            This function deletes an existing state machine. If the
 *                  state machine was not explicitely stopped by the caller
 *                  using ''fmStopStateMachine'', it will be stopped by this
 *                  function before the state machine is deleted 
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *****************************************************************************/
fm_status fmDeleteStateMachine( fm_smHandle  handle )
{
    fm_status status;
    fm_stateMachine *sm;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "handle=%p\n", (void *)handle );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    sm->smMagicNumber = 0;

    /* remove it from the linked list of state machines */
    FM_DLL_REMOVE_NODE( &smEngine, smHead, smTail, sm, next, prev );

    /* Free the memory allocated to this entry */
    if ( sm->smTransitionHistory != NULL )
    {
        fmFree( sm->smTransitionHistory );
    }

    if ( sm->recordData != NULL )
    {
        fmFree( sm->recordData );
    }

    fmFree( sm );

    status = FM_OK;

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmDeleteStateMachine */



/*****************************************************************************/
/** fmNotifyStateMachineEvent
 * \ingroup intStateMachine
 *
 * \desc            This function notifies the state machine engine that
 *                  a specified event has occurred on a given state machine
 *                  and it should be processed accordingly.
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 * 
 * \param[in]       userInfo is a pointer to a caller-allocated containing
 *                  purpose-specific event info
 * 
 * \param[in]       recordData is a pointer to a caller-allocated area
 *                  containing purpose-specific data that the caller wants GSME
 *                  to save in the transition record. The amount of data to be
 *                  saved must be indicated by the caller when the State
 *                  Machine instance is created using ''fmCreateStateMachine''
 *                  or ''fmCreateAndStartStateMachine''
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE  the state machine type
 *                  indicated in the event info header does not match that of 
 *                  the referred state machine
 *****************************************************************************/
fm_status fmNotifyStateMachineEvent( fm_smHandle     handle,
                                     fm_smEventInfo *eventInfo,
                                     void           *userInfo,
                                     void           *recordData )
{

    fm_status                status;
    fm_stateMachine         *sm;
    fm_smTransitionEntry     entry;
    fm_smTransitionCallback  transition;
    fm_smConditionCallback   condition;
    fm_smTransitionRecord    record;
    fm_int                   nextState;
    fm_bool                  gsmeLockTaken   = FALSE;
    fm_bool                  callerLockTaken = FALSE;
    fm_int                   smType;
    fm_uint32                refValue;
    fm_int                   precedence;


    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "handle=%p eventInfo=%p\n", 
                  (void *)handle,
                  (void *)eventInfo );

        /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    /* check the event pointer */
    if ( eventInfo == NULL )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_INVALID_ARGUMENT );
    }

    /* make sure the caller's lock is valid and NOT a super-precedence lock */
    status = fmGetLockPrecedence( eventInfo->lock, &precedence );
    if ( status != FM_OK )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );
    }

    if ( precedence == FM_LOCK_SUPER_PRECEDENCE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_INVALID_ARGUMENT );
    }


    FLAG_TAKE_CALLER_LOCK( eventInfo );
    FLAG_TAKE_GSME_LOCK( );

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }


    /* make sure there's no state machine type mismatch */
    if ( sm->type           == NULL               || 
         eventInfo->smType  != sm->type->smType   || 
         eventInfo->eventId >= sm->type->nrEvents  )
    {
        status = FM_ERR_STATE_MACHINE_TYPE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* save the current reference value and state machine type
       to perform a consistency check later on */
    smType   = sm->type->smType;
    refValue = sm->smRefValue;

    /* before calling any action or condition, make sure this flag is
     * set to its default value, which is FALSE*/
    eventInfo->dontSaveRecord = FALSE;

    /* retrieve the State Transition Table entry */
    entry = GET_TABLE_ENTRY( sm->type->smTransitionTable, 
                             sm->curState,
                             eventInfo->eventId,
                             sm->type->nrEvents );

    /* 
     * Save event timestamp here. 
     * Further processing may request another events which would be saved 
     * with earlier time.
     */
    SaveEventTime( sm, &record.eventTime );

    if ( entry.nextState         == FM_STATE_UNSPECIFIED && 
         entry.conditionCallback != NULL )
    {
        condition = entry.conditionCallback;

        /* by default, nextState is set to the current state */
        nextState  = sm->curState;

        /* drop the GSME lock to allow the callback to use other locks */
        FLAG_DROP_GSME_LOCK();
        status = condition( eventInfo, userInfo, &nextState );
        FLAG_TAKE_GSME_LOCK();
    }
    else
    {
        /* retrieve the transition callback */
        transition = entry.transitionCallback;

        /* assume it'll be ok unless the transition callback
           tell us otherwise */
        status     = FM_OK;
        nextState  = entry.nextState;

        /* default action if the action list is empty */
        if ( transition != NULL )
        {
            /* drop the GSME lock to allow the callback to use other locks */
            FLAG_DROP_GSME_LOCK();
            status = transition( eventInfo, userInfo );
            FLAG_TAKE_GSME_LOCK();
        }
    }

    /* we may have release the GSME lock temporarily, make sure the state
       machine instance is still valid and nothing changed meanwhile */
    if ( ( sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER ) ||
         ( sm->smRefValue    != refValue )                   ||
         ( sm->type          == NULL )                       ||
         ( sm->type->smType  != smType ) )
    {
        FM_LOG_DEBUG( FM_LOG_CAT_STATE_MACHINE, 
                      "State Machine Instance modified during transition: "
                      "sm->magicNumber=0x%08x "
                      "sm->smRefValue=%d refValue=%d "
                      "sm->type=%p sm->type->smType=%d smType=%d\n",
                      sm->smMagicNumber,
                      sm->smRefValue,
                      refValue,
                      (void *)sm->type,
                      (sm->type ? sm->type->smType : -1 ),
                      smType );
        status = FM_OK;
        goto ABORT;
    }

    /* make sure the next state is valid */
    if ( nextState < 0 || nextState >= sm->type->nrStates )

    {
        status = FM_ERR_STATE_MACHINE_TYPE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* Check dontSaveRecord flag, it may been modified (set to TRUE)
     * by an action or condition in order to not record the current
     * transaction. Note that this flag is always restored to its
     * default value when a new event is notified and processed */
    if (eventInfo->dontSaveRecord == FALSE)
    {
        /* fill out the transition record */
        record.eventInfo    = *eventInfo;
        record.currentState =  sm->curState;
        record.status       =  status;
        record.smUserID     = sm->smUserID;
        if ( status == FM_OK )
        {
            record.nextState = nextState;
        }
        else
        {
            record.nextState = sm->curState;
        }
    
        /* Save this transition record */
        SaveTransitionRecord( sm, &record, recordData );
    }

    if ( status == FM_OK )
    {
        sm->curState = nextState;
    }
    
ABORT:
    if ( gsmeLockTaken )
    {
        DROP_GSME_LOCK();
    }
    if ( callerLockTaken )
    {
        DROP_CALLER_LOCK( eventInfo );
    }
    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmNotifyStateMachineEvent */


/*****************************************************************************/
/** fmGetStateMachineCurrentState
 * \ingroup intStateMachine
 *
 * \desc            This function returns the current state for a given state
 *                  machine
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \param[out]      state is a pointer to a caller-allocated area
 *                  where this function will return the current state
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 * 
 * \return          FM_ERR_STATE_MACHINE_TYPE  the state machine isn't bound
 *                  to any registered type
 *****************************************************************************/
fm_status fmGetStateMachineCurrentState( fm_smHandle  handle, fm_int *state )
{
    fm_status        status;
    fm_stateMachine *sm;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "handle=%p\n", (void *)handle );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* make sure this state machine is currently bound */
    if ( sm->type == NULL )
    {
        status = FM_ERR_STATE_MACHINE_TYPE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    *state = sm->curState;
    status = FM_OK;

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status )


}   /* end fmGetStateMachineCurrentState */


/*****************************************************************************/
/** fmGetStateTransitionHistory
 * \ingroup intStateMachine
 *
 * \desc            This function returns the most recent N transitions
 *                  occurred on the specified state machine. N is the smallest
 *                  between the number of transitions kept in this state
 *                  machines transition history buffer and the size of the
 *                  caller-specified buffer
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \param[in,out]   nrRecords is a pointer to a caller-allocated area
 *                  used by the caller to indicate the size of its transition
 *                  history buffer and by this function to return the number
 *                  of transitions records
 * 
 * \param[in]       records is a pointer to a caller-allocated area where
 *                  this function will return up to most recent *nrTransitions
 *                  transition records for this state machine
 *
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *****************************************************************************/
fm_status fmGetStateTransitionHistory( fm_smHandle            handle,
                                       fm_int                *nrRecords,
                                       fm_smTransitionRecord *records )
{

    fm_status              status;
    fm_stateMachine       *sm;
    fm_int                 idx;
    fm_int                 firstIdx;
    fm_int                 recordIdx;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "handle=%p nrTransitions=%p transitions=%p\n", 
                  (void *)handle,
                  (void *)nrRecords,
                  (void *)records );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* check the pointer arguments */
    if ( nrRecords == NULL || records == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* has the transition history circular buffer wrapped at least once? */ 
    if ( sm->nrTransitions > sm->transitionHistorySize )
    {
        /* Yes, determine the index of the least recent transition */
        *nrRecords = sm->transitionHistorySize;
        firstIdx   = sm->nrTransitions - sm->transitionHistorySize;
        firstIdx   = firstIdx % sm->transitionHistorySize;
    }
    else
    {
        /* Not yet, the least recent transition is at location 0  */
        *nrRecords = sm->nrTransitions;
        firstIdx       = 0;
    }

    /* copy the transition records into the caller-provided buffer */
    for ( idx = 0 ; idx < *nrRecords  ; idx++ )
    {
        recordIdx = ((firstIdx + idx) % sm->transitionHistorySize);
        *(records + idx) = sm->smTransitionHistory[recordIdx];
    }

    /* Successful, if we got here */
    status = FM_OK;

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmGetStateTransitionHistory */


/*****************************************************************************/
/** fmClearStateTransitionHistory
 * \ingroup intStateMachine
 *
 * \desc            This function clears the transition history buffer for the
 *                  specified state machine
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 *
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *****************************************************************************/
fm_status fmClearStateTransitionHistory( fm_smHandle handle )
{
    fm_status        status;
    fm_stateMachine *sm;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, "handle=%p\n", (void *)handle );

    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /**************************************************
     * No reason to check the state machine type. The 
     * operation is harmless if it isn't bound 
     **************************************************/
    
    /* just clear the log */
    status = ClearStateTransitionHistory( sm );

ABORT:
    DROP_GSME_LOCK();

    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmClearStateTransitionHistory */


/*****************************************************************************/
/** fmChangeStateTransitionHistorySize
 * \ingroup intStateMachine
 *
 * \desc            This function changes the size of the transition history
 *                  buffer for the specified state machine. Up to N transitions
 *                  are preserved in the history buffer where N is the smallest
 *                  between historySize and the number of transitions kept in
 *                  the buffer when this function is invoked
 * 
 * \param[in]       handle is the handle for this state machine generated when
 *                  the state machine was created 
 * 
 * \param[in]       historySize is the new size of the transition history
 *                  buffer
 * 
 * \return          FM_OK if the state machine was created successfully
 * 
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the historySize is invalid
 * 
 * \return          FM_ERR_NO_MEM if memory allocation for the new history
 *                  buffer failed
 *****************************************************************************/
fm_status fmChangeStateTransitionHistorySize( fm_smHandle handle,
                                              fm_int      historySize )
{
    fm_status              status;
    fm_stateMachine       *sm;
    fm_smTransitionRecord *newHistoryStart;
    fm_smTransitionRecord *oldHistoryStart;
    fm_smTransitionRecord *newRecord;
    fm_smTransitionRecord *oldRecord;
    fm_byte               *newDataBuf;
    fm_int                 nrTransitions;
    fm_int                 firstFromIdx;
    fm_int                 idx;
    fm_int                 recordSize;
    fm_int                 bufferSize;

    FM_LOG_ENTRY( FM_LOG_CAT_STATE_MACHINE, 
                  "handle=%p historySize=%d\n", 
                  (void *)handle,
                  historySize );


    /* Make sure it's initialized */
    if ( smEngine.init != TRUE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, FM_ERR_UNINITIALIZED );
    }

    TAKE_GSME_LOCK();

    newDataBuf      = NULL;
    newHistoryStart = NULL;
    nrTransitions   = 0;

    /* consistency check on the handle */
    sm = (fm_stateMachine *)handle;
    if ( sm == NULL || sm->smMagicNumber != STATE_MACHINE_MAGIC_NUMBER )
    {
        status = FM_ERR_STATE_MACHINE_HANDLE;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }

    /* Is the size valid? */
    if ( historySize < 0 )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
    }


    /* Proceed only if the caller still wants to keep a transition history */
    if ( historySize > 0 )
    {
        recordSize = historySize * sizeof( fm_smTransitionRecord );
        newHistoryStart = fmAlloc( recordSize );
        if ( newHistoryStart == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }

        /* clear the new buffer */
        FM_MEMSET_S( newHistoryStart, recordSize, 0, recordSize );

        bufferSize = historySize * sm->recordDataSize;
        newDataBuf = fmAlloc( bufferSize );
        if ( newDataBuf == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_STATE_MACHINE, status );
        }

        /* clear the new buffer */
        FM_MEMSET_S( newDataBuf, bufferSize, 0, bufferSize );


        /*****************************************************
         * determine how many transition records to keep
         *****************************************************/

        /* is the old history buffer full? */
        if ( sm->nrTransitions > sm->transitionHistorySize )
        {
            /* yes */
            nrTransitions  = sm->transitionHistorySize;
            firstFromIdx   = sm->nrTransitions - sm->transitionHistorySize;
            firstFromIdx   = firstFromIdx % sm->transitionHistorySize;
        }
        else
        {
            /* not yet */
            nrTransitions  = sm->nrTransitions;
            firstFromIdx   = 0;
        }

        /* Do not exceed the size of the new history buffer */
        if ( nrTransitions > historySize )
        {
            firstFromIdx   += (nrTransitions - historySize);
            firstFromIdx    = firstFromIdx % sm->transitionHistorySize;
            nrTransitions   = historySize;
        }


        /********************************************************
         * Copy all transition records that we want to keep
         ********************************************************/

        oldHistoryStart = sm->smTransitionHistory;
        if ( oldHistoryStart != 0 )
        {
            newRecord  = newHistoryStart;
            for ( idx = 0 ; idx < nrTransitions ; idx++ )
            {
                oldRecord  = oldHistoryStart;
                oldRecord += ((firstFromIdx + idx) % sm->transitionHistorySize );
                *newRecord = *oldRecord;
                newRecord->recordData = newDataBuf + sm->recordDataSize*idx;

                /* copy also the event-specific data */
                FM_MEMCPY_S( newRecord->recordData, 
                             sm->recordDataSize,
                             oldRecord->recordData,
                             sm->recordDataSize );

                newRecord++;
            }

        }

    }   /* end if ( historySize > 0 ) */


    /* Free the previous transition buffer, if any */
    if ( sm->smTransitionHistory != NULL )
    {
        fmFree( sm->smTransitionHistory );
    }

    if ( sm->recordData != NULL )
    {
        fmFree( sm->recordData );
    }

    /* finalize the change by recording size and pointer of the new buffer */
    sm->transitionHistorySize = historySize;
    if ( historySize > 0 )
    {
        sm->smTransitionHistory = newHistoryStart;
        sm->recordData          = newDataBuf;
        sm->nrTransitions       = nrTransitions;
    }
    else
    {
        sm->smTransitionHistory = NULL;
        sm->recordData          = NULL;
        sm->nrTransitions       = 0;
    }

    /* Successful if we got here */
    status = FM_OK;

ABORT:
    DROP_GSME_LOCK();

    if ( status != FM_OK )
    {
        if ( newDataBuf != NULL )
        {
            fmFree( newDataBuf );
        }
        if ( newHistoryStart != NULL )
        {
            fmFree( newHistoryStart );
        }
    }
    FM_LOG_EXIT( FM_LOG_CAT_STATE_MACHINE, status );

}   /* end fmChangeStateTransitionHistory */

