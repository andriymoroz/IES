/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_eye_diagram.c
 * Creation Date:   December 12, 2012
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2012 - 2014, Intel Corporation
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

static fm_status ValidateEyeDiagramId( fm_int               eyeDiagramId,
                                       fmDbgEyeDiagram    **eyeDiagramPtr );
static fm_status DeleteEyeDiagram( fm_int               eyeDiagramId  );
static fm_status GetEyeDiagramSample( fm_int               eyeDiagramId,
                                      fm_int               sampleId,
                                      fm_eyeDiagramSample *sample );


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** ValidateEyeDiagramId
 * \ingroup intDiagEye
 *
 * \chips           FM6000
 *
 * \desc            Validates an eye diagram identifier.
 *
 * \param[in]       eyeDiagramId is the eye diagram identifier.
 * 
 * \param[out]      eyeDiagramPtr points to to a caller-supplied area
 *                  where this function will return a pointer to the eye
 *                  diagram descriptor associated with the specified ID.
 * 
 * \return          FM_OK if the eye diagram ID is valid.
 * \return          FM_ERR_INVALID_ARGUMENT if the eye diagram ID is invalid.
 *
 *****************************************************************************/
static fm_status ValidateEyeDiagramId( fm_int            eyeDiagramId,
                                       fmDbgEyeDiagram **eyeDiagramPtr )
{
    fm_status        err;

    /* Make sure the eye diagram ID is in range */
    if ( eyeDiagramId < 0 || eyeDiagramId >= FM_DBG_MAX_EYE_DIAGRAMS )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );
    }

    /* is the eye diagram ID currently being used? */
    *eyeDiagramPtr = fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId];
    if ( *eyeDiagramPtr == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );
    }

    err = FM_OK;

ABORT:
    return err;

}   /* end ValidateEyeDiagramId */



/*****************************************************************************/
/** DeleteEyeDiagram
 * \ingroup intDiagEye
 *
 * \chips           FM6000
 *
 * \desc            Private function to delete an eye diagram.
 *
 * \param[in]       eyeDiagramId is the ID of the eye diagram to be deleted.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteEyeDiagram( fm_int eyeDiagramId )
{
    fm_status err;
    fmDbgEyeDiagram *eyeDiagramPtr;

    eyeDiagramPtr = fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId];

    /* invoke the chip specific function */
    FM_API_CALL_FAMILY( err,
                        GET_SWITCH_PTR(eyeDiagramPtr->sw)->DbgDeleteEyeDiagram,
                        eyeDiagramPtr->eyeDiagram );

    /* free the block of memory allocated for the eye diagram descriptor */
    fmFree( fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId] );
    
    /* mark the entry as unused by setting the pointer to NULL */
    fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId] = NULL;

    return FM_OK;

}   /* end DeleteEyeDiagram */



/*****************************************************************************/
/** GetEyeDiagramSample
 * \ingroup intDiagEye
 *
 * \chips           FM6000
 *
 * \desc            Private function to get an eye diagram sample by ID.
 *
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \param[in]       sampleId is the ID of the sample.
 * 
 * \param[out]      sample points to a caller-supplied area where the
 *                  function will return eye diagram sample corresponding to
 *                  the specified sample ID.
 * 
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram,
 *                  or the sample pointer is invalid.
 * 
 * \return          FM_ERR_NO_EYE_DIAGRAM_SAMPLES if no more eye diagram
 *                  samples are left. In this case, sample->sampleId will be
 *                  set to -1.
 *                  
 *****************************************************************************/
static fm_status GetEyeDiagramSample( fm_int               eyeDiagramId,
                                      fm_int               sampleId,
                                      fm_eyeDiagramSample *sample )
{
    fm_status        err;
    fm_int           sw;
    fmDbgEyeDiagram *eyeDiagramPtr;

    /* validate the eye diagram ID */
    err = ValidateEyeDiagramId( eyeDiagramId, &eyeDiagramPtr );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );

    /* recover the ID of the switch the eye diagram belongs to */
    sw = eyeDiagramPtr->sw;

    /* lock it before accessing the eye diagram information */
    PROTECT_SWITCH(sw);

    if ( eyeDiagramPtr->sampleCount > sampleId )
    {
        *sample = eyeDiagramPtr->eyeDiagram[sampleId];
        err = FM_OK;
    }
    else
    {
        sample->sampleId = -1;
        err = FM_ERR_NO_EYE_DIAGRAM_SAMPLES;
    }
    UNPROTECT_SWITCH(sw);

ABORT:
    return err;

}   /* end GetEyeDiagramSample */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmDbgInitEyeDiagrams
 * \ingroup intDiagEye
 *
 * \chips           FM6000
 *
 * \desc            Initialize data structures related to the eye diagram
                    facility.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgInitEyeDiagrams(void)
{
    memset( fmRootDebug->fmDbgEyeDiagrams, 0, 
            sizeof(fmRootDebug->fmDbgEyeDiagrams) );

    return FM_OK;

}   /* end fmDbgInitEyeDiagrams */




/*****************************************************************************/
/** fmDbgTakeEyeDiagram
 * \ingroup diagEye 
 *
 * \chips           FM6000
 *
 * \desc            Captures the eye diagram for a given port lane.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       mac is the port's zero-based MAC number on which to operate.
 *                  May be specified as FM_PORT_ACTIVE_MAC to operate on the
 *                  currently selected active MAC. Must be specified as either
 *                  FM_PORT_ACTIVE_MAC or zero for ports that have only one 
 *                  MAC (physical link connection). It may  not be specified
 *                  as FM_PORT_MAC_ALL.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. It may not be specified as FM_PORT_LANE_NA or
 *                  FM_PORT_LANE_ALL.
 * 
 * \param[in]       eyeDiagramId is an arbitrary eye diagram number
 *                  betweeen 0 and ''FM_DBG_MAX_EYE_DIAGRAMS'' - 1 by which to
 *                  refer the eye diagram later. The eye diagram ID is global
 *                  across all switches in the system.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not UP yet.
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid.
 * \return          FM_ERR_INVALID_PORT_MAC if the MAC ID is invalid.
 * \return          FM_ERR_INVALID_PORT_LANE if the lane ID is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the eye diagram ID is invalid.
 * \return          FM_ERR_NO_MEM if the function was unable to allocate
 *                  memory for the eye samples table or the eye diagram
 *                  description.
 *
 *****************************************************************************/
fm_status fmDbgTakeEyeDiagram( fm_int sw, 
                               fm_int port, 
                               fm_int mac, 
                               fm_int lane, 
                               fm_int eyeDiagramId )
{
    fm_status        err;
    fm_switch       *switchPtr;
    fmDbgEyeDiagram *eyeDiagramPtr;

    eyeDiagramPtr = NULL;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR( sw );

    /* Make sure the eye diagram ID is in range */
    if ( eyeDiagramId < 0 || eyeDiagramId >= FM_DBG_MAX_EYE_DIAGRAMS )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* is the eye diagram ID currently being used? */
    eyeDiagramPtr = fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId];
    if ( eyeDiagramPtr != NULL )
    {
        /* Yes, delete the eye diagram */
        DeleteEyeDiagram( eyeDiagramId );
    }

    /* allocate the eye diagram descriptor */
    eyeDiagramPtr = (fmDbgEyeDiagram *) fmAlloc( sizeof(fmDbgEyeDiagram) );
    if ( eyeDiagramPtr == NULL )
    {
        /* allocation failed */
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
    fmRootDebug->fmDbgEyeDiagrams[eyeDiagramId] = eyeDiagramPtr;
    memset( eyeDiagramPtr, 0, sizeof(fmDbgEyeDiagram) );

    /* initialize the generic fields in the eye diagram descriptor */
    eyeDiagramPtr->sw   = sw;
    eyeDiagramPtr->port = port;
    eyeDiagramPtr->mac  = mac;
    eyeDiagramPtr->lane = lane;

    /* invoke the chip specific function */
    FM_API_CALL_FAMILY( err,
                        switchPtr->DbgTakeEyeDiagram,
                        sw, port, mac, lane,
                        &eyeDiagramPtr->sampleCount,
                        &eyeDiagramPtr->eyeDiagram );

ABORT:
    if ( err != FM_OK && eyeDiagramPtr != NULL )
    {
        /* Delete the eye if it was allocated but we failed to take it */
        DeleteEyeDiagram( eyeDiagramId );
    }
    if ( swProtected )
    {
        UNPROTECT_SWITCH(sw);
    }

    return err;

}   /* end fmDbgTakeEyeDiagram */




/*****************************************************************************/
/** fmDbgPlotEyeDiagram
 * \ingroup portDiag
 *
 * \chips           FM6000
 *
 * \desc            Plots an eye diagram previously captured using the
 *                  function ''fmDbgTakeEyeDiagram''.
 *
 * \param[in]       eyeDiagramId is the ID specified when the eye diagram was
 *                  captured.
 * 
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram.
 * 
 *****************************************************************************/
fm_status fmDbgPlotEyeDiagram( fm_int eyeDiagramId )
{
    fm_int           sw;
    fm_status        err;
    fmDbgEyeDiagram *eyeDiagramPtr;

    /* validate the eye diagram ID */
    err = ValidateEyeDiagramId( eyeDiagramId, &eyeDiagramPtr );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );

    /* retrieve the ID of the switch associated with this eye diagram */
    sw = eyeDiagramPtr->sw;

    /* now protect the switch, then call the chip-specific plot function */
    PROTECT_SWITCH(sw);

    FM_API_CALL_FAMILY( err, 
                        GET_SWITCH_PTR(sw)->DbgPlotEyeDiagram, 
                        eyeDiagramPtr->eyeDiagram );

    UNPROTECT_SWITCH(sw);

ABORT:
    return err;

}   /* end fmDbgPlotEyeDiagram */




/*****************************************************************************/
/** fmDbgDeleteEyeDiagram
 * \ingroup diagEye
 *
 * \chips           FM6000
 *
 * \desc            Deletes an existing eye diagram previously captured
 *                  using ''fmDbgTakeEyeDiagram''.
 * 
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \return          FM_OK if successful.
 *                  
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram.
 *
 *****************************************************************************/
fm_status fmDbgDeleteEyeDiagram( fm_int eyeDiagramId )
{
    fm_status        err;
    fm_int           sw;
    fmDbgEyeDiagram *eyeDiagramPtr;

    /* validate the eye diagram ID */
    err = ValidateEyeDiagramId( eyeDiagramId, &eyeDiagramPtr );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );

    /* recover the ID of the switch the eye diagram belongs to */
    sw = eyeDiagramPtr->sw;

    /* lock it before deleting the eye diagram */
    PROTECT_SWITCH(sw);

    DeleteEyeDiagram( eyeDiagramId );

    UNPROTECT_SWITCH(sw);

ABORT:
    return err;

}   /* end fmDbgDeleteEyeDiagram */




/*****************************************************************************/
/** fmDbgGetEyeDiagramSampleList
 * \ingroup diagEye
 *
 * \chips           FM6000
 *
 * \desc            Retrieves the list of samples of an eye diagram
 *                  previously captured using ''fmDbgTakeEyeDiagram''.
 * 
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \param[out]      count points to a caller-supplied area where this
 *                  function will return the number of samples in the eye
 *                  diagram.
 * 
 * \param[out]      sample points to a caller-supplied area where this
 *                  function will return a pointer to an array containing
 *                  the list of eye diagram samples.
 * 
 * \return          FM_OK if successful.
 *                  
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram,
 *                  or one of the pointer arguments is invalid.
 *
 *****************************************************************************/
fm_status fmDbgGetEyeDiagramSampleList( fm_int                eyeDiagramId,
                                        fm_int               *count,
                                        fm_eyeDiagramSample **sample )
{
    fm_status        err;
    fm_int           sw;
    fmDbgEyeDiagram *eyeDiagramPtr;

    /* validate the eye diagram ID */
    err = ValidateEyeDiagramId( eyeDiagramId, &eyeDiagramPtr );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );

    /* sanity check on the pointer arguments */
    if ( count == NULL || sample == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );
    }

    /* recover the ID of the switch the eye diagram belongs to */
    sw = eyeDiagramPtr->sw;

    /* lock it before accessing the eye diagram information */
    PROTECT_SWITCH(sw);

    *count = eyeDiagramPtr->sampleCount;
    *sample = eyeDiagramPtr->eyeDiagram;

    UNPROTECT_SWITCH(sw);

ABORT:
    return err;


}   /* end fmDbgGetEyeDiagramSampleList */



/*****************************************************************************/
/** fmDbgGetEyeDiagramSampleFirst
 * \ingroup diagEye
 *
 * \chips           FM6000
 *
 * \desc            Retrieves the first sample of an eye diagram previously
 *                  captured using ''fmDbgTakeEyeDiagram''.
 * 
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \param[out]      sample points to a caller-supplied area where this
 *                  function will return the first sample of the eye diagram.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram,
 *                  or the sample pointer is invalid.
 * \return          FM_ERR_NO_EYE_DIAGRAM_SAMPLES if no more eye diagram
 *                  samples are left. In this case, sample->sampleId will be
 *                  set to -1.
 *                  
 *****************************************************************************/
fm_status fmDbgGetEyeDiagramSampleFirst( fm_int               eyeDiagramId, 
                                         fm_eyeDiagramSample *sample )
{

    /* sanity check on the pointer arguments */
    if ( sample == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    return GetEyeDiagramSample( eyeDiagramId, 0, sample );

}   /* end fmDbgGetEyeDiagramSampleFirst */




/*****************************************************************************/
/** fmDbgGetEyeDiagramSampleNext
 * \ingroup diagEye
 *
 * \chips           FM6000
 *
 * \desc            Retrieves the next sample of an eye diagram previously
 *                  captured using ''fmDbgTakeEyeDiagram''. Allows the
 *                  whole diagram to be retrieved one sample at a time.
 * 
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \param[in,out]   sample points to a caller-supplied area where the
 *                  function will return the next sample of the eye diagram.
 *                  The function takes the current sample ID from the
 *                  sample->sampleId field and returns the next sample in
 *                  the samples list (if any).
 * 
 * \return          FM_OK if successful.
 *                  
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram,
 *                  or the sample pointer is invalid.
 * 
 * \return          FM_ERR_NO_EYE_DIAGRAM_SAMPLES if no more eye diagram
 *                  samples are left. In this case, sample->sampleId will be
 *                  set to -1.
 *                  
 *****************************************************************************/
fm_status fmDbgGetEyeDiagramSampleNext( fm_int               eyeDiagramId, 
                                        fm_eyeDiagramSample *sample )
{

    /* sanity check on the pointer arguments */
    if ( sample == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    return GetEyeDiagramSample( eyeDiagramId, sample->sampleId + 1, sample );

}   /* end fmDbgGetEyeDiagramSampleNext */




/*****************************************************************************/
/** fmDbgGetEyeDiagramSampleCount
 * \ingroup diagEye
 *
 * \chips           FM6000
 *
 * \desc            Returns the number of samples of an eye diagram captured
 *                  using ''fmDbgTakeEyeDiagram''
 * 
 * \param[in]       eyeDiagramId is the ID of the eye diagram.
 * 
 * \param[out]      count points to a caller-supplied area where this
 *                  function will return the number of samples in the eye
 *                  diagram.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the ID of the eye diagram is
 *                  out of range or does not correspond to a valid eye diagram.
 *                  or the count pointer is invalid.
 *
 *****************************************************************************/
fm_status fmDbgGetEyeDiagramSampleCount( fm_int  eyeDiagramId, 
                                         fm_int *count )
{
    fm_status         err;
    fm_int            sw;
    fmDbgEyeDiagram  *eyeDiagramPtr;


    /* validate the eye diagram ID */
    err = ValidateEyeDiagramId( eyeDiagramId, &eyeDiagramPtr );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );

    /* sanity check on the pointer arguments */
    if ( count == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_PORT, err );
    }

    /* recover the ID of the switch the eye diagram belongs to */
    sw = eyeDiagramPtr->sw;

    /* lock it before accessing the eye diagram information */
    PROTECT_SWITCH(sw);

    *count = eyeDiagramPtr->sampleCount;

    UNPROTECT_SWITCH(sw);

ABORT:
    return err;

}   /* end fmDbgGetEyeDiagramSampleCount */

