
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_eye_diagram.c
 * Creation Date:   May 13, 2014
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2014, Intel Corporation
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
/** PrintEyeDiagramHorizontalBorder
 * \ingroup intDiagEye
 *
 * \chips           FM10000
 *
 * \desc            Draw the top or bottom border of the eye diagram frame
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintEyeDiagramHorizontalBorder(void)
{

    fm_int i;
    printf("+");
    for (i = 0 ; i < 128  ; i++)
    {
        printf("-");
    }
    printf("+\n");

} 	/* end PrintEyeDiagramHorizontalBorder */


/*****************************************************************************/
/** PrintEyeDiagramHorizontalBorder
 * \ingroup intDiagEye
 *
 * \chips           FM10000
 *
 * \desc            Draw the left or right border of one line of the eye
 *                  diagram frame
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintEyeDiagramVerticalBorder ( void )
{
    printf("|");

}   /* end PrintEyeDiagramVerticalBorder */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000DbgTakeEyeDiagram
 * \ingroup intDiagEye
 *
 * \chips           FM10000
 *
 * \desc            This function collects FM10000_PORT_EYE_SAMPLES eye
 *                  diagram sample points from a given Ethernet logical port
 *                  by computing the bit error rate for every combination of
 *                  phase and offset level
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       port is the logical port on which to operatre
 * 
 * \param[in]       mac is the port's zero-based MAC number on which to operate.
 *                  May be specified as FM_PORT_ACTIVE_MAC to operate on the
 *                  currently selected active MAC. Must be specified as either
 *                  FM_PORT_ACTIVE_MAC or zero for ports that have only one 
 *                  MAC (physical link connection). It may  not be specified
 *                  as FM_PORT_MAC_ALL
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. It may not be specified as FM_PORT_LANE_NA or
 *                  FM_PORT_LANE_ALL
 * 
 * \param[out]      count is the pointer to a caller-allocated area where this
 *                  function will return the number of samples in this eye
 *                  diagram
 * 
 * \param[out]      eyeDiagramSamples is a pointer to a caller-allocated area
 *                  where this function is going to return a pointer to the
 *                  array of eye diagram samples
 * 
 * \return          FM_OK if successful
 * 
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * 
 * \return          FM_ERR_SWITCH_NOT_UP if the switch is not UP yet
 * 
 * \return          FM_ERR_INVALID_PORT if the logical port ID is invalid
 * 
 * \return          FM_ERR_INVALID_PORT_MAC if the MAC ID is invalid
 * 
 * \return          FM_ERR_INVALID_PORT_LANE if the lane ID is invalid
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if at least one of the pointer
 *                  arguments is invalid
 * 
 *****************************************************************************/
fm_status fm10000DbgTakeEyeDiagram ( fm_int               sw,
                                    fm_int                port,
                                    fm_int                mac,
                                    fm_int                lane,
                                    fm_int               *count,
                                    fm_eyeDiagramSample **eyeDiagramSamples )
{
    fm_status   err;


    FM_NOT_USED(mac);

    err = FM_OK;

    /* sanity check on the pointer arguments */
    if ( count == NULL || eyeDiagramSamples == NULL )
    {
        return  FM_ERR_INVALID_ARGUMENT;
    }

    /* allocate memory for the sample table */
    *eyeDiagramSamples = fmAlloc( FM10000_PORT_EYE_SAMPLES *
                                  sizeof(fm_eyeDiagramSample) );
    if ( *eyeDiagramSamples == NULL )
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR ( FM_LOG_CAT_PORT, err );
    }

    /* capture the eye diagram */
    err = fm10000GetPortEyeDiagram(sw, port, lane, *eyeDiagramSamples);

ABORT:
    if ( err != FM_OK )
    {
        /* If memory was allocated and there's an error, free it now */
        if ( *eyeDiagramSamples != NULL )
        {
            fmFree( *eyeDiagramSamples );
        }
        *count             = 0;
        *eyeDiagramSamples = NULL;
    }
    else
    {
        *count             = FM10000_PORT_EYE_SAMPLES;
    }

    return err;

}   /* end fm10000DbgTakeEyeDiagram */



/*****************************************************************************/
/** fm10000DbgPlotEyeDiagram
 * \ingroup intDiagEye
 *
 * \chips           FM10000
 *
 * \desc            This function plots the eye diagram of a given ethernet
 *                  port lane
 *
 * \param[in]       sampleTable is the pointer to the eye diagram sample list
 * 
 * \return          FM_OK if successful
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the pointer argument is invalid
 *
 *****************************************************************************/
fm_status fm10000DbgPlotEyeDiagram ( fm_eyeDiagramSample *sampleTable )
{
    fm_int                   eyeSample;
    fm_int                   offset;
    fm_int                   phase;

    /* sanity check on the pointer arguments */
    if ( sampleTable == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
     
    /**************************************************
     * Plot the eye diagram: use 1/2 scaling for the 
     * offset axis and 2x scaling for the phase axis
     **************************************************/
    
    PrintEyeDiagramHorizontalBorder();

    for ( offset = 62 ; offset >= 0 ; offset -= 2 )
    {
        PrintEyeDiagramVerticalBorder();
        for (phase = 0; phase < 64 ; phase++)
        {
            eyeSample = offset+64*phase;
            if ( sampleTable[eyeSample].errors != 0 )
            {
                printf("00");
            }
            else
            {
                printf("  ");
            }
        }
        PrintEyeDiagramVerticalBorder();

        /* print the offset axis labels */
        if ( offset == 62 )
        {
            printf("   400 mV");
        }
        if ( offset == 30 )
        {
            printf("     0 mV");
        }
        if ( offset == 0 )
        {
            printf("  -400 mV");
        }

        printf("\n");
    }

    PrintEyeDiagramHorizontalBorder();

    /* print the phase axis labels */
    printf(" 0");
    for (phase = 0 ; phase < 126  ; phase++)
    {
        printf(" ");
    }
    printf("64\n");

    /* if we got here, everything ok */
    return FM_OK;

}   /* end fm10000DbgPlotEyeDiagram */



/*****************************************************************************/
/** fm10000DbgDeleteEyeDiagram
 * \ingroup intDiagEye
 *
 * \chips           FM10000
 *
 * \desc            This function plots the eye diagram previously captured
 *                  using the function ''fmDbgTakeEyeDiagram''
 *
 * \param[in]       sampleTable is the pointer to the eye diagram sample list
 * 
 * \return          FM_OK if successful
 * 
 * \return          FM_ERR_INVALID_ARGUMENT if the pointer argument is invalid
 * 
 *****************************************************************************/
fm_status fm10000DbgDeleteEyeDiagram( fm_eyeDiagramSample *sampleTable )
{

    /* sanity check on the pointer arguments */
    if ( sampleTable == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* free the block of memory allocated for the eye diagram */
    fmFree( sampleTable );

    return FM_OK;

}   /* end fm10000DbgDeleteEyeDiagram */


