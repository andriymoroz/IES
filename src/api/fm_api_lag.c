/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lag.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with link aggregation
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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
/** fmCreateLAG
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a link aggregation group.
 *
 * \note            This function should not be used after calling
 *                  ''fmAllocateStackLAGs'' (see ''Stacking and GloRT 
 *                  Management''). Instead, use ''fmCreateStackLAG''.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      lagNumber points to caller-allocated storage where this
 *                  function should place the LAG number (handle) of the
 *                  newly created LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_LAG if max LAGs (''FM_MAX_NUM_LAGS'') already
 *                  created.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lag structure.
 *
 *****************************************************************************/
fm_status fmCreateLAG(fm_int sw, fm_int *lagNumber)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %p\n",
                     sw,
                     (void *) lagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmCreateLAGInt(sw, lagNumber, FALSE);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmCreateLAG */




/*****************************************************************************/
/** fmDeleteLAG
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a link aggregation group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       lagNumber is the LAG number of the LAG to delete, as
 *                  returned by ''fmCreateLAG'' or
 *                  ''fmCreateStackLAG''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LAG if lagNumber is out of range or is
 *                  not the handle of an existing LAG.
 *
 *****************************************************************************/
fm_status fmDeleteLAG(fm_int sw, fm_int lagNumber)
{
    fm_switch *  switchPtr;
    fm_int       lagIndex;
    fm_status    err;
    fm_bool      asyncDeletion;
    fm_lag *     lagPtr = NULL;
    fm_char      semName[20];
    fm_timestamp wait;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG, "sw = %d, lagNumber = %d\n", sw, lagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    asyncDeletion = fmGetBoolApiProperty(FM_AAK_API_ASYNC_LAG_DELETION, 
                                         FM_AAD_API_ASYNC_LAG_DELETION);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * We need to take the routing lock (for the benefit 
     * of fmMcastDeleteLagNotify) before the LAG lock
     * to prevent an inversion.
     **************************************************/
    
    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);
    }
    
    TAKE_LAG_LOCK(sw);

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    err = fmMcastDeleteLagNotify(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    /* Asynchronous deletion used unless disabled and the switch is
     * not a member of a switch aggregate.  For switch aggregates, the
     * synchronized deletion is handled at the swag switch level.
     */
    if ( !asyncDeletion && (switchPtr->swag < 0) )
    {
        lagPtr->deleteSemaphore = (fm_semaphore *) fmAlloc( sizeof(fm_semaphore) );

        if (lagPtr->deleteSemaphore == NULL)
        {
            err = FM_ERR_NO_MEM;
            goto ABORT;
        }

        FM_SNPRINTF_S( semName, sizeof(semName), "lagDelSem%d", lagIndex );
        err = fmCreateSemaphore(semName,
                                FM_SEM_BINARY,
                                lagPtr->deleteSemaphore,
                                0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    }

    FM_API_CALL_FAMILY(err, switchPtr->DeleteLagFromSwitch, sw, lagIndex);

    if ( (err == FM_OK) && (lagPtr->deleteSemaphore != NULL) )
    {
        wait.sec = fmGetIntApiProperty(FM_AAK_API_LAG_DELETE_SEMAPHORE_TIMEOUT,
                                       FM_AAD_API_LAG_DELETE_SEMAPHORE_TIMEOUT);
        wait.usec = 0;
        DROP_LAG_LOCK(sw);

        err = fmWaitSemaphore(lagPtr->deleteSemaphore, &wait);

        TAKE_LAG_LOCK(sw);

        if (err == FM_OK)
        {
            /* Note that fmFreeLAG releases and deletes the deleteSemaphore */
            fmFreeLAG(sw, lagIndex);
            lagPtr = NULL;
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_LAG, "The deleteSemaphore timed out\n");
        }
    }

ABORT:

    if ( (lagPtr != NULL) && (lagPtr->deleteSemaphore != NULL) )
    {
        fmReleaseSemaphore(lagPtr->deleteSemaphore);
        fmDeleteSemaphore(lagPtr->deleteSemaphore);

        lagPtr->deleteSemaphore = NULL;
    }

    DROP_LAG_LOCK(sw);
    fmReleaseWriteLock(&switchPtr->routingLock);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmDeleteLAG */




/*****************************************************************************/
/** fmAddLAGPort
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a port to a link aggregation group.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagNumber is the LAG number (returned by
 *                  fmCreateLAG) to which the port should be added.
 *
 * \param[in]       port is the number of the port to be added to the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LAG if lagNumber is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_ALREADYUSED_PORT if the port is already a member
 *                  of a LAG.
 * \return          FM_ERR_FULL_LAG if the LAG already contains the maximum
 *                  number of ports (''FM_MAX_NUM_LAG_MEMBERS'').
 *
 *****************************************************************************/
fm_status fmAddLAGPort(fm_int sw, fm_int lagNumber, fm_int port)
{
    fm_switch *switchPtr;
    fm_int     lagIndex;
    fm_status  err;
    fm_lag *   lagPtr;
    fm_port *  portPtr;
    fm_bool    lagLockTaken = FALSE;
    fm_bool    routeLockTaken = FALSE;
    fm_uint32  allowedPortTypes;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d, port = %d\n",
                     sw,
                     lagNumber,
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
 
    switchPtr = GET_SWITCH_PTR(sw);
    allowedPortTypes = switchPtr->lagInfoTable.allowedPortTypes;

    VALIDATE_LOGICAL_PORT(sw, port, allowedPortTypes);

    /* Validate the port is LAG capable */
    portPtr = GET_PORT_PTR(sw, port);

    if ( !fmIsRemotePort(sw, port)
        && !(portPtr->capabilities & FM_PORT_CAPABILITY_LAG_CAPABLE) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err = FM_ERR_INVALID_PORT);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    routeLockTaken = TRUE;

    TAKE_LAG_LOCK(sw);

    lagLockTaken = TRUE;

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err = FM_ERR_INVALID_LAG);
    }

    if ( fmPortIsInALAG(sw, port) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err = FM_ERR_ALREADYUSED_PORT);
    }

    /* Internal LAG should be filled only by Internal Port AND */
    /* External LAG should be filled only by External Port     */
    if (fmIsCardinalPort(sw, port))
    {
        lagPtr = GET_LAG_PTR(sw, lagIndex);
        if ( lagPtr->isInternalPort != fmIsInternalPort(sw, port) )
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err = FM_FAIL);
        }
    }

    switchPtr->portTable[port]->lagIndex = lagIndex;

    FM_API_CALL_FAMILY(err, switchPtr->AddPortToLag, sw, lagIndex, port);

    if (err != FM_OK)
    {
        switchPtr->portTable[port]->lagIndex = -1;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    
ABORT:
    if (lagLockTaken)
    {
        DROP_LAG_LOCK(sw);
    }

    if (routeLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmAddLAGPort */




/*****************************************************************************/
/** fmDeleteLAGPort
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a port from a link aggregation group.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagNumber is the LAG number (returned by
 *                  fmCreateLAG) from which the port should be deleted.
 *
 * \param[in]       port is the number of the port to be deleted from the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LAG if lagNumber is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_INVALID_PORT if the port is not a member of the
 *                  specified LAG.
 *
 *****************************************************************************/
fm_status fmDeleteLAGPort(fm_int sw, fm_int lagNumber, fm_int port)
{
    fm_switch *switchPtr;
    fm_int     lagIndex;
    fm_status  err;
    fm_bool    lagLockTaken = FALSE;
    fm_bool    routeLockTaken = FALSE;
    fm_uint32  allowedPortTypes;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d, port = %d\n",
                     sw,
                     lagNumber,
                     port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    allowedPortTypes = switchPtr->lagInfoTable.allowedPortTypes;

    VALIDATE_LOGICAL_PORT(sw, port, allowedPortTypes);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    routeLockTaken = TRUE;

    TAKE_LAG_LOCK(sw);

    lagLockTaken = TRUE;

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Check that the port is actually in the LAG we're trying to
     * remove it from. */
    if ( !fmPortIsInLAG(sw, port, lagIndex) )
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    FM_API_CALL_FAMILY(err, switchPtr->DeletePortFromLag, sw, lagIndex, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

ABORT:

    if (lagLockTaken)
    {
        DROP_LAG_LOCK(sw);
    }

    if (routeLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmDeleteLAGPort */




/*****************************************************************************/
/** fmGetLAGList
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Function to return a list of valid LAGs.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[out]      nLAG contains the number of valid LAGs found.
 *
 * \param[out]      lagNumbers contains the array of valid LAG numbers.
 *
 * \param[in]       maxLags is the maximum number of LAGs, i.e., the size
 *                  of the lagNumbers array.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if maxLags was too small to accommodate 
 *                  the entire list of valid LAGs.
 *
 *****************************************************************************/
fm_status fmGetLAGList(fm_int  sw,
                       fm_int *nLAG,
                       fm_int *lagNumbers,
                       fm_int  maxLags)
{
    fm_switch *switchPtr;
    fm_int     lagCount = 0;
    fm_int     lagIndex;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, nLAG = %p, lagNumbers = %p, maxLags = %d\n",
                     sw,
                     (void *) nLAG,
                     (void *) lagNumbers,
                     maxLags);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            if (lagCount >= maxLags)
            {
                err = FM_ERR_BUFFER_FULL;
                goto ABORT;
            }

            *lagNumbers++ = fmGetLagLogicalPort(sw, lagIndex);
            lagCount++;
        }
    }

    err = FM_OK;

ABORT:
    *nLAG = lagCount;

    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGList */




/*****************************************************************************/
/** fmGetLAGPortList
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Function to return a list of ports in a LAG
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagNumber is the lag number
 *
 * \param[out]      nPorts contains the number of ports found.
 *
 * \param[out]      ports contains the array of ports found
 *
 * \param[in]       maxPorts is the maximum number of ports, i.e., size of
 *                  ports and switches arrays.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetLAGPortList(fm_int  sw,
                           fm_int  lagNumber,
                           fm_int *nPorts,
                           fm_int *ports,
                           fm_int  maxPorts)
{
    fm_int    lagIndex;
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d, nPorts = %p, ports = %p, "
                     "maxPorts = %d\n",
                     sw,
                     lagNumber,
                     (void *) nPorts,
                     (void *) ports,
                     maxPorts);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    err = fmGetLAGMemberPorts(sw, lagIndex, nPorts, ports, maxPorts, FALSE);

ABORT:
    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGPortList */




/*****************************************************************************/
/** fmGetLAGFirst
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first link aggregation group number.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[out]      firstLagNumber points to caller-allocated storage where
 *                  this function should place the number of the first LAG.
 *                  Will be set to -1 if no LAGs found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_LAGS if no LAG found.
 *
 *****************************************************************************/
fm_status fmGetLAGFirst(fm_int sw, fm_int *firstLagNumber)
{
    fm_switch *switchPtr;
    fm_int     lagIndex;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, firstLagNumber = %p\n",
                     sw,
                     (void *) firstLagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    *firstLagNumber = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            *firstLagNumber = fmGetLagLogicalPort(sw, lagIndex);
            err = FM_OK;
            goto ABORT;
        }
    }

    err = FM_ERR_NO_LAGS;

ABORT:
    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGFirst */




/*****************************************************************************/
/** fmGetLAGNext
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next link aggregation group number, following
 *                  a prior call to this function or to fmGetLAGFirst.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       currentLagNumber is the last LAG number found by a previous
 *                  call to this function or to fmGetLAGFirst.
 *
 * \param[out]      nextLagNumber points to caller-allocated storage where
 *                  this function should place the number of the next LAG.
 *                  Will be set to -1 if no LAGs found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_LAGS if no more LAGs found.
 *
 *****************************************************************************/
fm_status fmGetLAGNext(fm_int  sw,
                       fm_int  currentLagNumber,
                       fm_int *nextLagNumber)
{
    fm_switch *switchPtr;
    fm_int     lagIndex;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, currentLagNumber = %d, nextLagNumber = %p\n",
                     sw,
                     currentLagNumber,
                     (void *) nextLagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    *nextLagNumber = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Get the internal index for the current LAG. */
    lagIndex = fmGetLagIndex(sw, currentLagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    for (lagIndex = lagIndex + 1 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            *nextLagNumber = fmGetLagLogicalPort(sw, lagIndex);
            err = FM_OK;
            goto ABORT;
        }
    }

    err = FM_ERR_NO_LAGS;

ABORT:
    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGNext */




/*****************************************************************************/
/** fmGetLAGPortFirst
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first port in a LAG.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagNumber is the number of the LAG (returned by
 *                  fmCreateLAG) that should be searched for ports.
 *
 * \param[out]      firstPort points to caller-allocated storage where this
 *                  function should place the first port in the LAG.
 *                  Will be set to -1 if no ports found in LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if lagNumber is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_NO_PORTS_IN_LAG if no ports in LAG.
 *
 *****************************************************************************/
fm_status fmGetLAGPortFirst(fm_int  sw, fm_int  lagNumber, fm_int *firstPort)
{
    fm_lag *  lagPtr;
    fm_int    lagIndex;
    fm_status err;
    fm_int    i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d, firstPort = %p\n",
                     sw,
                     lagNumber,
                     (void *) firstPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    *firstPort = -1;

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    for (i = 0 ; i < FM_MAX_NUM_LAG_MEMBERS ; i++)
    {
        if (lagPtr->member[i].port != FM_LAG_UNUSED_PORT)
        {
            *firstPort = lagPtr->member[i].port;
            err = FM_OK;
            goto ABORT;
        }
    }

    err = FM_ERR_NO_PORTS_IN_LAG;

ABORT:
    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGPortFirst */




/*****************************************************************************/
/** fmGetLAGPortNext
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next port in a LAG.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagNumber is the number of the LAG (returned by
 *                  fmCreateLAG) that should be searched for ports.
 *
 * \param[in]       currentPort is the last port number found by a previous
 *                  call to this function or to fmGetLAGPortFirst.
 *
 * \param[out]      nextPort points to caller-allocated storage where this
 *                  function should place the next port in the LAG.
 *                  Will be set to -1 if no more ports found in LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if lagNumber is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_INVALID_PORT if nextPort is not a port in lagNumber
 *                  on currentSwitch.
 * \return          FM_ERR_NO_PORTS_IN_LAG if no more ports in LAG.
 *
 *****************************************************************************/
fm_status fmGetLAGPortNext(fm_int  sw,
                           fm_int  lagNumber,
                           fm_int  currentPort,
                           fm_int *nextPort)
{
    fm_lag *  lagPtr;
    fm_int    lagIndex;
    fm_int    memberIndex;
    fm_status err;
    fm_int    i;
 
    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d, currentPort = %d, nextPort = %p\n",
                     sw,
                     lagNumber,
                     currentPort,
                     (void *) nextPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    *nextPort = -1;

    /* Get the internal index for this LAG. */
    lagIndex = fmGetLagIndex(sw, lagNumber);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    /* Find the previous port in the member list. */
    memberIndex = fmGetPortMemberIndex(sw, currentPort);
    if (memberIndex < 0)
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    /* Find the next port in the member list. */
    err = FM_ERR_NO_PORTS_IN_LAG;

    for (i = memberIndex + 1 ; i < FM_MAX_NUM_LAG_MEMBERS ; ++i)
    {
        if (lagPtr->member[i].port != FM_LAG_UNUSED_PORT)
        {
            *nextPort = lagPtr->member[i].port;
            err = FM_OK;
            break;
        }
    }

ABORT:
    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGPortNext */




/*****************************************************************************/
/** fmGetLAGAttribute
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get a link aggregation group attribute.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       attribute is the link aggregation group attribute
 *                  (see 'Link Aggregation Group Attributes') to get.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Link Aggregation Group Attributes'' to determine the
 *                  use of this argument.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH sw is invalid or switch is not up.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 *
 *****************************************************************************/
fm_status fmGetLAGAttribute(fm_int sw,
                            fm_int attribute,
                            fm_int index,
                            void * value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, attribute = %d, index = %d, value = %p\n",
                     sw,
                     attribute,
                     index,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetLagAttribute,
                       sw,
                       attribute,
                       index,
                       value);

    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGAttribute */




/*****************************************************************************/
/** fmSetLAGAttribute
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a link aggregation group attribute.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       attribute is the link aggregation group attribute
 *                  (see 'Link Aggregation Group Attributes') to set.
 *
 * \param[in]       index is an attribute-specific index. See
 *                  ''Link Aggregation Group Attributes'' to determine the
 *                  use of this argument.
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH sw is invalid or switch is not up.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 * \return          FM_ERR_UNSUPPORTED if attribute is not supported.
 *
 *****************************************************************************/
fm_status fmSetLAGAttribute(fm_int sw,
                            fm_int attribute,
                            fm_int index,
                            void * value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, attribute = %d, index = %d, *value = %u\n",
                     sw,
                     attribute,
                     index,
                     *(fm_uint32 *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_LAG_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetLagAttribute,
                       sw,
                       attribute,
                       index,
                       value);

    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmSetLAGAttribute */




/*****************************************************************************/
/** fmLogicalPortToLAGNumber
 * \ingroup lag
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert a LAG's logical port number to the LAG number
 *                  generated by ''fmCreateLAG''.
 *
 * \note            While this function is supported for all devices, it need
 *                  only be used for FM2000 devices. On all other devices,
 *                  the LAG number returned by ''fmCreateLAG'' is always
 *                  identical to the LAG's logical port number, so this
 *                  function just acts as an identify function. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the LAG's logical port number.
 *
 * \param[out]      lagNumber points to caller allocated storage where the
 *                  value of the LAG number will be written by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if logicalPort is not the logical port
 *                  of a LAG.
 *
 *****************************************************************************/
fm_status fmLogicalPortToLAGNumber(fm_int  sw,
                                   fm_int  logicalPort,
                                   fm_int *lagNumber)
{
    fm_port * portPtr;
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, logicalPort = %d\n",
                     sw,
                     logicalPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, logicalPort, ALLOW_LAG | ALLOW_REMOTE);
    TAKE_LAG_LOCK(sw);

    portPtr = GET_PORT_PTR(sw, logicalPort);

    /* Make sure it is a LAG logical port number */
    if (portPtr->portType != FM_PORT_TYPE_LAG)
    {
        err = FM_ERR_INVALID_PORT;
        *lagNumber = -1;
    }
    else
    {
        /* The lag number is the lag logical port. */
        *lagNumber = logicalPort;
    }

    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_LAG,
                           err,
                           "lagNumber = %d\n",
                           *lagNumber);

}   /* end fmLogicalPortToLAGNumber */




/*****************************************************************************/
/** fmLAGNumberToLogicalPort
 * \ingroup  lag 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Convert the LAG number generated by ''fmCreateLAG'' to
 *                  the LAG's logical port number.
 *
 * \note            While this function is supported for all devices, it need
 *                  only be used for FM2000 devices. On all other devices,
 *                  the LAG number returned by ''fmCreateLAG'' is always
 *                  identical to the LAG's logical port number, so this
 *                  function just acts as an identify function. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagNumber is the LAG number returned by ''fmCreateLAG''.
 *
 * \param[out]      logicalPort points to caller allocated storage where the
 *                  LAG's logicalPort will be written by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if lagNumber is not valid.
 *
 *****************************************************************************/
fm_status fmLAGNumberToLogicalPort(fm_int  sw,
                                   fm_int  lagNumber,
                                   fm_int *logicalPort)
{
    fm_port * portPtr;
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, lagNumber = %d\n",
                     sw,
                     lagNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    /* lagNumber contains a LAG logical port.*/
    VALIDATE_LOGICAL_PORT(sw, lagNumber, ALLOW_LAG | ALLOW_REMOTE);
    TAKE_LAG_LOCK(sw);

    portPtr = GET_PORT_PTR(sw, lagNumber);

    /* Make sure it is a LAG logical port number */
    if (portPtr->portType != FM_PORT_TYPE_LAG)
    {
        err = FM_ERR_INVALID_LAG;
        *logicalPort = -1;
    }
    else
    {
        /* The lag number is the lag logical port. */
        *logicalPort = lagNumber;
    }

    DROP_LAG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_LAG,
                           err,
                           "logicalPort = %d\n",
                           *logicalPort);

}   /* end fmLAGNumberToLogicalPort */
