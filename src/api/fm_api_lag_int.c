/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lag_int.c
 * Creation Date:   Oct 6, 2008
 * Description:     Internal Link Aggregation Group functions. 
 *                  Refactored from fm_api_lag.c.
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
/* Macro to round a value to the closest upper multiple of 4 */
#define FM_CEIL_CHUNK_OF_4(val)         ((val + 3) & ~0x3);


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
/** fmSortLagMembers
 * \ingroup intLag
 *
 * \desc            Sorts the LAG port members by glort within a lag.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have taken the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the LAG on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmSortLagMembers(fm_int sw, fm_int lagIndex)
{
    fm_status err;
    fm_lag *  lagPtr;
    fm_port * portPtr;
    fm_uint32 i;
    fm_int    portList[FM_MAX_NUM_LAG_MEMBERS];
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, lagIndex = %d\n", sw, lagIndex);

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    for (i = 0; i < lagPtr->nbMembers; i++)
    {
        portList[i] = lagPtr->member[i].port;
    }

    err = fmSortPortByGlort(sw, portList, lagPtr->nbMembers, portList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    for (i = 0; i < lagPtr->nbMembers; i++)
    {
        portPtr = GET_PORT_PTR(sw, portList[i]);
        lagPtr->member[i].port = portList[i];
        
        portPtr->memberIndex = i;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmSortLagMembers */




/*****************************************************************************/
/** fmGetLagIndex
 * \ingroup intLag
 *
 * \desc            Returns the internal LAG index for the specified LAG
 *                  logical port number.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       logicalPort is the LAG logical port
 *                  (returned by fmCreateLAG).
 *
 * \return          Index in lagInfoTable of the LAG with the specified handle.
 * \return          -1 if the LAG handle is invalid.
 *
 *****************************************************************************/
fm_int fmGetLagIndex(fm_int sw, fm_int logicalPort)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, logicalPort);

    /* Error if port is not a LAG. */
    if (portPtr == NULL || portPtr->portType != FM_PORT_TYPE_LAG)
    {
        return -1;
    }

    return portPtr->lagIndex;

}   /* end fmGetLagIndex */




/*****************************************************************************/
/** fmGetLagLogicalPort
 * \ingroup intLag
 *
 * \desc            Returns the LAG logical port number for the specified
 *                  internal LAG index.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the specified switch.
 *
 * \return          The LAG logical port number.
 * \return          -1 if the LAG index is invalid.
 *
 *****************************************************************************/
fm_int fmGetLagLogicalPort(fm_int sw, fm_int lagIndex)
{
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (!LAG_INDEX_OUT_OF_BOUNDS(lagIndex) &&
        switchPtr->lagInfoTable.lag[lagIndex])
    {
        return switchPtr->lagInfoTable.lag[lagIndex]->logicalPort;
    }

    return -1;

}   /* end fmGetLagLogicalPort */




/*****************************************************************************/
/** fmGetPortLagIndex
 * \ingroup intLag
 *
 * \desc            Returns the index of the LAG to which the port belongs.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number
 *
 * \return          lagIndex of the port if it is a member of a LAG
 * \return          -1 if it is not a member of a LAG
 *
 ****************************************************************************/
fm_int fmGetPortLagIndex(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);

    return (portPtr) ? portPtr->lagIndex : -1;

}   /* end fmGetPortLagIndex */




/*****************************************************************************/
/** fmGetPortMemberIndex
 * \ingroup intLag
 *
 * \desc            Returns the LAG member identifier for a port.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \return          memberIndex of the port if it is a member of a LAG
 * \return          -1 if it is not a member of a LAG
 *
 ****************************************************************************/
fm_int fmGetPortMemberIndex(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);

    return (portPtr) ? portPtr->memberIndex : -1;

}   /* end fmGetPortMemberIndex */




/*****************************************************************************/
/** fmIsValidLagPort
 * \ingroup intLag
 *
 * \desc            Determines whether a port is a valid LAG port.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          TRUE if the port is a valid LAG port, FALSE otherwise.
 *
 *****************************************************************************/
fm_bool fmIsValidLagPort(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_lag *   lagPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and get pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);

    if (portPtr && portPtr->portType == FM_PORT_TYPE_LAG)
    {
        /* Validate LAG index and get pointer. */
        lagPtr = GET_SWITCH_LAG_PTR(switchPtr, portPtr->lagIndex);

        if (lagPtr && lagPtr->extension)
        {
            return TRUE;
        }
    }

    return FALSE;

}   /* end fmIsValidLagPort */




/*****************************************************************************/
/** fmLagIndexToLogicalPort
 * \ingroup intLag
 *
 * \desc            Returns the logical port number for the specified 
 *                  internal LAG index.
 *
 * \note            This is an internal function used by non-LAG code.
 *                  The caller has protected the switch, but has not 
 *                  claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[out]      logicalPort points to a caller-supplied variable to
 *                  receive the logical port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if the operation is not supported
 *                  for the specified switch.
 * \return          FM_ERR_INVALID_LAG if lagIndex is not a valid LAG.
 *
 *****************************************************************************/
fm_status fmLagIndexToLogicalPort(fm_int  sw,
                                  fm_int  lagIndex,
                                  fm_int *logicalPort)
{
    fm_switch *switchPtr;
    fm_lag *   lagPtr;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LAG,
                         "sw = %d, lagIndex = %d\n",
                         sw,
                         lagIndex);

    *logicalPort = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_LAG_LOCK(sw);

    /* Validate LAG index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);

    if (lagPtr == NULL)
    {
        DROP_LAG_LOCK(sw);
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LAG, FM_ERR_INVALID_LAG);
    }

    *logicalPort = lagPtr->logicalPort;

    DROP_LAG_LOCK(sw);

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_LAG,
                               FM_OK,
                               "logicalPort = %d\n",
                               *logicalPort);

}   /* end fmLagIndexToLogicalPort */




/*****************************************************************************/
/** fmLogicalPortToLagIndex
 * \ingroup intLag
 *
 * \desc            Returns the index of the LAG to which the port belongs.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number
 *
 * \param[out]      lagIndex points to a variable to receive the index of
 *                  the LAG on the switch. Will be -1 in case of error.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is not valid.
 * \return          FM_ERR_INVALID_LAG if lagIndex is not valid.
 *
 ****************************************************************************/
fm_status fmLogicalPortToLagIndex(fm_int sw, fm_int port, fm_int *lagIndex)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_lag *   lagPtr;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LAG, "sw=%d port=%d\n", sw, port);

    *lagIndex = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL || portPtr->portType != FM_PORT_TYPE_LAG)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    /* Validate lag index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, portPtr->lagIndex);
    if (lagPtr == NULL)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LAG, FM_ERR_INVALID_LAG);
    }

    *lagIndex = portPtr->lagIndex;

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_LAG,
                               FM_OK,
                               "lagIndex=%d\n",
                               *lagIndex);

}   /* end fmLogicalPortToLagIndex */




/*****************************************************************************/
/** fmPortIsInALAG
 * \ingroup intLag
 *
 * \desc            Returns true if the specified port is a member of any
 *                  LAG.  A port is considered to be a LAG member
 *                  even if the port is currently down.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number
 *
 * \return          TRUE if the port is a member of a lag
 * \return          FALSE if the port is not a member of a lag
 *
 *****************************************************************************/
fm_bool fmPortIsInALAG(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL)
    {
        return FALSE;
    }

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_LAG,
                         "port = %d, "
                         "port.lagIndex = %d, port.memberIndex = %d\n",
                         port,
                         portPtr->lagIndex,
                         portPtr->memberIndex);

    /* Determine whether port is a member of a LAG. */
    return FM_IS_PORT_IN_A_LAG(portPtr);

}   /* end fmPortIsInALAG */




/*****************************************************************************/
/** fmPortIsInLAG
 * \ingroup intLag
 *
 * \desc            Returns true if the specified port is a member of the
 *                  specified LAG.  A port is considered to be a LAG member
 *                  even if the port is currently down.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \return          TRUE if the port is a member of the lag
 * \return          FALSE if the port is not a member of the lag
 *
 *****************************************************************************/
fm_bool fmPortIsInLAG(fm_int sw, fm_int port, fm_int lagIndex)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_lag *   lagPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL)
    {
        return FALSE;
    }

    /* Validate LAG index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);
    if (lagPtr == NULL)
    {
        return FALSE;
    }

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_LAG,
                         "port = %d, lagIndex = %d, "
                         "port.lagIndex = %d, port.memberIndex = %d\n",
                         port,
                         lagIndex,
                         portPtr->lagIndex,
                         portPtr->memberIndex);

    /* Determine whether port is a member of the LAG. */
    if (portPtr->lagIndex == lagIndex &&
        portPtr->memberIndex >= 0)
    {
        FM_LOG_ASSERT( FM_LOG_CAT_LAG,
                       (portPtr->memberIndex < FM_MAX_NUM_LAG_MEMBERS),
                       "Port %d LAG member ID is %d, which is out of range (> %d)!\n",
                       port,
                       portPtr->memberIndex,
                       FM_MAX_NUM_LAG_MEMBERS );
        
        FM_LOG_ASSERT( FM_LOG_CAT_LAG,
                       (lagPtr->member[portPtr->memberIndex].port == port),
                       "LAG's port number for member ID %d is %d, but port is %d!\n",
                       portPtr->memberIndex,
                       lagPtr->member[portPtr->memberIndex].port,
                       port );
        
        return TRUE;
    }

    return FALSE;

}   /* end fmPortIsInLAG */




/*****************************************************************************/
/** fmCountActiveLagMembers
 * \ingroup intLag
 *
 * \desc            Returns the number of active members in the lag. Active
 *                  members are lag ports which are up.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 * 
 * \return          number of active ports in the lag
 *
 *****************************************************************************/
fm_uint32 fmCountActiveLagMembers(fm_int sw, fm_int lagIndex)
{
    fm_lag *   lagPtr;
    fm_uint32  numPorts = 0;
    fm_uint32  i;
    fm_port *  portPtr;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LAG,
                         "sw = %d, lagIndex = %d\n",
                         sw,
                         lagIndex);

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    if (lagPtr)
    {
        for (i = 0 ; i < lagPtr->nbMembers ; i++)
        {
            portPtr = GET_PORT_PTR(sw, lagPtr->member[i].port);

            if (portPtr->linkUp)
            {
                numPorts++;
            }
        }
    }
    
    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_LAG,
                               numPorts,
                               "numPorts = %d\n",
                               numPorts);

}   /* end fmCountActiveLagMembers */




/*****************************************************************************/
/** fmInitLAGTable
 * \ingroup intLag
 *
 * \desc            Initializes each entry of the LAG table cache to match
 *                  chip defaults.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmInitLAGTable(fm_int sw)
{
    fm_switch *switchPtr;
    fm_uint    lagIndex;
    
    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_LAG);

    switchPtr = GET_SWITCH_PTR(sw);

    if (!switchPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);
    }

    /* Cache the LAG management mode as defined by the API attribute. */
    switchPtr->perLagMgmt = 
                fmGetBoolApiProperty(FM_AAK_API_PER_LAG_MANAGEMENT, 
                                     FM_AAD_API_PER_LAG_MANAGEMENT);

    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            /***************************************************
             * Free instances from previous switch up states
             * including the device specific extension.
             **************************************************/
            fmFreeLAG(sw, lagIndex);
        }

        switchPtr->lagInfoTable.lag[lagIndex] = NULL;
        /* Reset the LAG allocation */
        switchPtr->lagInfoTable.resvLag[lagIndex] = 0;

    }

    for (lagIndex = 0 ; lagIndex < FM_ALLOC_LAGS_MAX ; lagIndex++)
    {
        switchPtr->lagInfoTable.allocLags[lagIndex].numLags = 0;

    }

    /* static mode by default */
    switchPtr->lagInfoTable.lagMode = FM_MODE_STATIC;

    /* FM_LAG_PRUNING is enabled by default (note reverse logic) */
    switchPtr->lagInfoTable.pruningDisabled = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmInitLAGTable */




/*****************************************************************************/
/** fmDeleteLagCallback
 * \ingroup intLag
 *
 * \desc            Called by the hardware layer when all necessary hardware
 *                  operations have completed and LAG deletion can be
 *                  finished.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagPtr points to the LAG record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDeleteLagCallback(fm_int sw, fm_lag *lagPtr)
{
    fm_status err = FM_OK;
    fm_int    lagIndex;
#if FM_SUPPORT_SWAG
    fm_int    swagSw;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagPtr = %p(%d)\n",
                 sw,
                 (void *) lagPtr,
                 lagPtr->index);

    lagIndex = lagPtr->index;

    if (lagPtr->deleteSemaphore == NULL)
    {
        fmFreeLAG(sw, lagIndex);
    }
    else
    {
        err = fmSignalSemaphore(lagPtr->deleteSemaphore);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_LAG, err);
        }
    }

#if FM_SUPPORT_SWAG
    err = fmIsSwitchInASWAG(sw, &swagSw);

    if (err == FM_OK)
    {
        /* Switch is in a SWAG, pass the delete-notification to the SWAG */
        err = fmSWAGDeleteLagCallback(swagSw, sw, lagIndex);
    }
    else
    {
        /* The switch is not in a swag */
        err = FM_OK;
    }

#endif

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmDeleteLagCallback */




/*****************************************************************************/
/** fmFreeLAG
 * \ingroup intLag
 *
 * \desc            Free the structure for a link aggregation group.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG to delete.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmFreeLAG(fm_int sw, fm_int lagIndex)
{
    fm_switch *  switchPtr;
    fm_lagInfo * lagInfo;
    fm_lag *     lagPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, lagIndex = %d\n", sw, lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfo   = GET_LAG_INFO_PTR(sw);
    lagPtr    = GET_LAG_PTR(sw,lagIndex);

    if (lagPtr != NULL)
    {
        switchPtr->FreeLAG(sw, lagIndex);

        if (lagPtr->deleteSemaphore != NULL)
        {
            fmReleaseSemaphore(lagPtr->deleteSemaphore);
            fmDeleteSemaphore(lagPtr->deleteSemaphore);
        }

        if (lagPtr->vlanMembership)
        {
            fmFree(lagPtr->vlanMembership);
        }

        fmFree(lagPtr);
        lagInfo->lag[lagIndex] = NULL;

        FM_SET_LAG_FREE(lagInfo, lagIndex);
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_LAG);

}   /* end fmFreeLAG */




/*****************************************************************************/
/** fmFreeLAGTable
 * \ingroup intLag
 *
 * \desc            Release all LAG structures used on the given switch
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmDestroyLAGTable(fm_int sw)
{
    fm_switch *switchPtr;
    fm_int     lagIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            fmFreeLAG(sw, lagIndex);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmFreeLAGTable */




/*****************************************************************************/
/** fmInformLAGPortDown
 * \ingroup intLag
 *
 * \desc            Called to inform the LAG code that the state of a port is
 *                  no longer "up".  If the port was an active member of
 *                  any LAG, it will be set to inactive.
 *
 * \note            This is an internal function used by non-LAG code.
 *                  The caller has protected the switch, but has not 
 *                  claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmInformLAGPortDown(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;
    fm_int     lagIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, port = %d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_LAG_LOCK(sw);

    if ( !fmPortIsInALAG(sw, port) )
    {
        goto ABORT;
    }

    lagIndex = fmGetPortLagIndex(sw, port);
    
    err = switchPtr->InformLAGPortDown(sw, port);

ABORT:
    DROP_LAG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmInformLAGPortDown */




/*****************************************************************************/
/** fmInformLAGPortUp
 * \ingroup intLag
 *
 * \desc            Called to inform the LAG code that the state of a port is
 *                  now "up".  If the port was an inactive member of
 *                  any LAG, it will be reactivated.
 *
 * \note            This is an internal function used by non-LAG code.
 *                  The caller has protected the switch, but has not 
 *                  claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmInformLAGPortUp(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;
    fm_int     lagIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, port = %d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_LAG_LOCK(sw);
    FM_TAKE_PORT_ATTR_LOCK(sw);

    if ( !fmPortIsInALAG(sw, port) && 
         !fmIsInternalPort(sw, port) )
    {
        goto ABORT;
    }

    lagIndex = fmGetPortLagIndex(sw, port);
    
    err = switchPtr->InformLAGPortUp(sw, port);

ABORT:
    FM_DROP_PORT_ATTR_LOCK(sw);
    DROP_LAG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmInformLAGPortUp */




/*****************************************************************************/
/** fmCheckLACPFilter
 * \ingroup intLag
 *
 * \desc            Determine if a received LACP frame should be dropped
 *                  or forwarded to the application.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       event points to the received packet event structure.
 *
 * \param[out]      filter points to caller-allocated storage where this
 *                  function will store TRUE if the packet should be dropped
 *                  or FALSE if it should be forwarded.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmCheckLACPFilter(fm_int sw, fm_eventPktRecv *event, fm_bool *filter)
{
    fm_switch *switchPtr;
    fm_macaddr destAddr;
    fm_buffer *pktData;

    /* Note: This function is optimized for the receive path */
 
    *filter   = FALSE;

    pktData  = event->pkt;

    /* A optimized snippet for faster packet receive processing */
    if (pktData->data[0] != htonl(0x0180C200))
    {
        return FM_OK;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Extract the destination address from the
     * packet.
     **************************************************/

    destAddr = fmGetPacketDestAddr(sw, pktData);

    if (destAddr == FM_LACP_DEST_ADDRESS)
    {
        /* We don't need to lock the LAG resources to do this test. */
        if (switchPtr->lagInfoTable.lagMode == FM_MODE_STATIC)
        {
            *filter = TRUE;
        }
    }

    return FM_OK;

}   /* end fmCheckLACPFilter */




/*****************************************************************************/
/** fmAllocateLAGsInt
 * \ingroup intLag 
 *
 * \desc            Allocate LAGs given a glort range. The function
 *                  returns the base LAG number and the number of LAGs
 *                  created. The caller can then enumerate these LAG number up
 *                  to the number of LAGs allocated. These LAG numbers will have 
 *                  the same CAM resources across multiple switches, given the
 *                  input glort information is the same.
 *
 * \note            The return base number might not be the same on different
 *                  switches. However the CAM resources for 
 *                  (baseLagNumber + n*step) will be consistent on different 
 *                  switches.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to use to create the
 *                  multicast resources.
 *
 * \param[in]       glortSize is the glort size to use. This value must be a
 *                  power of two.
 *
 * \param[in]       stackLag is TRUE if the LAGs are to be used in a stacking
 *                  environment.
 *
 * \param[out]      baseLagIndex points to caller-allocated storage 
 *                  where this function should place the base LAG number
 *                  (handle) of the newly allocated LAGs.
 *
 * \param[out]      numLags points to caller-allocated storage 
 *                  where this function should place the number of LAGs
 *                  allocated given the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage 
 *                  where this function should place the step value, where
 *                  the caller can increment from base by to get
 *                  subsequent LAG numbers
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if baseLagIndex, numLags, or 
 *                  step is NULL or input parameters fail checking.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  required in in the given glort range is being used.
 * \return          FM_ERR_NO_LAG_RESOURCES if no more resources are available.
 *
 *****************************************************************************/
fm_status fmAllocateLAGsInt(fm_int  sw,
                            fm_uint startGlort,
                            fm_uint glortSize,
                            fm_bool stackLag,
                            fm_int *baseLagIndex,
                            fm_int *numLags,
                            fm_int *step)
{
    fm_switch *   switchPtr;
    fm_status     err;
    fm_lagInfo *  lagInfo;
    fm_allocLags *allocLagsEntry = NULL;
    fm_int        lagIndex;
    fm_int        startIndex;
    fm_int        baseLagHandle;
    fm_int        numHandles;
    fm_int        off;
    fm_int        i;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, startGlort = %u, glortSize = %u, stackLag = %d\n",
                 sw,
                 startGlort,
                 glortSize,
                 stackLag);

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfo   = GET_LAG_INFO_PTR(sw);

    TAKE_LAG_LOCK(sw);

    *numLags = 0;
    *baseLagIndex = 0;

    err = FM_ERR_NO_LAG_RESOURCES;

    /* Find if there is a free alloc entry */
    for (i = 0 ; i < FM_ALLOC_LAGS_MAX ; i++)
    {
        if (!switchPtr->lagInfoTable.allocLags[i].numLags)
        {
            allocLagsEntry = &lagInfo->allocLags[i];
            err = FM_OK;
            FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                         " &lagInfo->allocLags[i=%d] => %p\n",
                         i, 
                         (void *)&lagInfo->allocLags[i]);
            break;
        }
    }

    /* No more free alloc lag entry */
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_API_CALL_FAMILY(err, 
                       switchPtr->AllocateLAGs,
                       sw,
                       startGlort,
                       glortSize,
                       &baseLagHandle,
                       &numHandles,
                       &off);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* Find and reserve the lag range */
    startIndex = -1;
    i = 0;
    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if ( !switchPtr->lagInfoTable.lag[lagIndex] &&
             !FM_IS_LAG_TAKEN(lagInfo, lagIndex) )
        {
            i++;
            if (i == 1)
            {
                startIndex = lagIndex;
            }
            if (i >= numHandles)
            {
                break;
            }
        }
        else
        {
            i = 0;
        }
    }

    /* Don't find a space for this */
    if (lagIndex >= FM_MAX_NUM_LAGS)
    {
        /* Free the allocated LAGs */
        FM_API_CALL_FAMILY(err,
                           switchPtr->FreeStackLAGs,
                           sw,
                           baseLagHandle);
        
        err = FM_ERR_NO_LAG_RESOURCES;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

#ifndef FM_SUPPORT_SWAG
    /* Reserve the lag number */
    for (lagIndex = startIndex ; lagIndex < startIndex + numHandles ; lagIndex++)
    {
        FM_RESERVE_LAG(lagInfo, lagIndex);
    }
#endif

    /* Save the alloc info so later we can remap the handle */
    allocLagsEntry->baseHandle = baseLagHandle;
    allocLagsEntry->numLags = numHandles;
    allocLagsEntry->baseLagIndex = startIndex;
    allocLagsEntry->step = off;

    FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                 "\n      allocLagsEntry->baseHandle=%d\n"
                 "      allocLagsEntry->numLags=%d\n"
                 "      allocLagsEntry->baseLagIndex=%d\n"
                 "      allocLagsEntry->step=%d\n",
                 baseLagHandle, 
                 numHandles,
                 startIndex,
                 off);

    *baseLagIndex = allocLagsEntry->baseHandle;
    *numLags = allocLagsEntry->numLags;
    *step = off;

ABORT:
    DROP_LAG_LOCK(sw);
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LAG,
                       err,
                       "baseLagIndex = %d, numLags = %d, step = %d\n",
                       *baseLagIndex,
                       *numLags,
                       *step);

}   /* end fmAllocateLAGsInt */




/*****************************************************************************/
/** fmFreeLAGsInt
 * \ingroup intLag
 *
 * \desc            Free allocated LAGs previously created with
 *                  ''fmAllocateLAGsInt''.
 *
 * \note            This requires all the LAGs associated with these LAGs
 *                  resources are deleted, before these LAGs resources
 *                  can be freed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLagNumber is the LAG number previously obtained from
 *                  function ''fmAllocateLAGsInt''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if baseLagIndex is not found.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeLAGsInt(fm_int sw, fm_int baseLagNumber)
{
    fm_switch *   switchPtr;
    fm_lagInfo *  lagInfo;
    fm_allocLags *allocLagsEntry = NULL;
    fm_int        lagIndex;
    fm_status     err;
    fm_int        i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw = %d, baseLagNumber = %d\n",
                     sw,
                     baseLagNumber);

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfo   = GET_LAG_INFO_PTR(sw);

    TAKE_LAG_LOCK(sw);

    err = FM_ERR_INVALID_LAG;

    /* Find the allocLagsEntry for the baseLagNumber */
    for (i = 0 ; i < FM_ALLOC_LAGS_MAX ; i++)
    {
        if (!lagInfo->allocLags[i].numLags)
        {
            continue;
        }

        if ( (baseLagNumber >= lagInfo->allocLags[i].baseHandle) &&
             (baseLagNumber < (lagInfo->allocLags[i].baseHandle + 
                                (lagInfo->allocLags[i].step * 
                                 lagInfo->allocLags[i].numLags))) ) 
        {
            allocLagsEntry = &lagInfo->allocLags[i];
            err = FM_OK;
            break;
        }
    }

    /* Unable to find the associated lag pool */
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_API_CALL_FAMILY(err,
                       switchPtr->FreeStackLAGs,
                       sw,
                       lagInfo->allocLags[i].baseHandle);

    /* Set this to unused if entry is deleted */
    if (err == FM_OK)
    {
        /* Unreserve the lag number */
        for (lagIndex = allocLagsEntry->baseLagIndex ;
             lagIndex < (allocLagsEntry->baseLagIndex + 
                         allocLagsEntry->numLags) ;
             lagIndex++)
        {
            FM_UNRESERVE_LAG(lagInfo, lagIndex);
        }
        lagInfo->allocLags[i].numLags = 0;
    }

ABORT:
    DROP_LAG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);


}   /* end fmFreeLAGsInt */




/*****************************************************************************/
/** fmCreateLAGInt
 * \ingroup intLag
 *
 * \desc            Create a link aggregation group.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in,out]   lagNumber points to caller-allocated storage where this
 *                  function should place the LAG number (handle) of the
 *                  newly created LAG if stacking is FALSE. If stacking is
 *                  TRUE then the caller should pass the specified handle
 *                  in this storage.
 *
 * \param[in]       stacking is the a flag to indicate if the call is from
 *                  stacking or non-stacking API.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_LAG if max LAGs (''FM_MAX_NUM_LAGS'') already
 *                  created.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  lag structure.
 *
 *****************************************************************************/
fm_status fmCreateLAGInt(fm_int sw, fm_int *lagNumber, fm_bool stacking)
{
    fm_switch    *switchPtr;
    fm_lagInfo *  lagInfo;
    fm_lag *      lagPtr;
    fm_port *     portPtr;
    fm_int        lagIndex;
    fm_status     err;
    fm_int        lagLogicalPort;
    fm_int        cnt;
    fm_int        i;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagNumber = %d stacking = %s\n",
                 sw,
                 *lagNumber,
                 FM_BOOLSTRING(stacking));

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfo   = GET_LAG_INFO_PTR(sw);

    TAKE_LAG_LOCK(sw);

    if (stacking)
    {
        /* The lag number contains the LAG logical port number */
        lagLogicalPort = *lagNumber;

        /**************************************************
         * Find the allocLagsEntry from the logical port 
         * number so we can get the corresponding LAG index.
         **************************************************/

        lagIndex = -1;
        for (cnt = 0 ; cnt < FM_ALLOC_LAGS_MAX ; cnt++)
        {
            if (!lagInfo->allocLags[cnt].numLags)
            {
                continue;
            }

            if ( (lagLogicalPort >= lagInfo->allocLags[cnt].baseHandle) &&
                 (lagLogicalPort < (lagInfo->allocLags[cnt].baseHandle + 
                                    (lagInfo->allocLags[cnt].step * 
                                     lagInfo->allocLags[cnt].numLags))) ) 
            {
                lagIndex = lagInfo->allocLags[cnt].baseLagIndex + 
                           ((lagLogicalPort - 
                            lagInfo->allocLags[cnt].baseHandle) /
                            lagInfo->allocLags[cnt].step);
            }
        }

        if ( LAG_INDEX_OUT_OF_BOUNDS(lagIndex) )
        {
            err = FM_ERR_INVALID_LAG;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

#ifndef FM_SUPPORT_SWAG
        if (!FM_IS_LAG_RESERVED(lagInfo, lagIndex))
        {
            err = FM_ERR_INVALID_LAG;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
#endif

        if (FM_IS_LAG_IN_USE(lagInfo, lagIndex))
        {
            err = FM_ERR_LAG_IN_USE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }
    else
    {
        /* The LAG logical port is not provided, so set it to 0 and the chip
           specific code will create and initialize it. */
        lagLogicalPort = 0;

        for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
        {
            if ( !FM_IS_LAG_IN_USE(lagInfo, lagIndex) &&
                 !FM_IS_LAG_RESERVED(lagInfo, lagIndex) )
            {
                /* Found a free one - reserved is ok */
                break;
            }
        }
    }

    if (lagIndex >= FM_MAX_NUM_LAGS)
    {
        err = FM_ERR_NO_FREE_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    lagPtr = lagInfo->lag[lagIndex];

    if (lagPtr == NULL)
    {
        /* Allocate and initialize the fm_lag structure */
        lagPtr = (fm_lag *) fmAlloc( sizeof(fm_lag) );

        if (lagPtr == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        FM_MEMSET_S(lagPtr, sizeof(fm_lag), 0, sizeof(fm_lag));

        if (switchPtr->perLagMgmt)
        {
            /* Allocate the vlan membership array */
            lagPtr->vlanMembership = 
               (fm_byte *) fmAlloc(sizeof(fm_byte) * switchPtr->vlanTableSize);

            if (lagPtr->vlanMembership == NULL)
            {
                err = FM_ERR_NO_MEM;
                fmFree(lagPtr);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }

            FM_MEMSET_S(lagPtr->vlanMembership, 
                        sizeof(fm_byte) * switchPtr->vlanTableSize,
                        0, 
                        sizeof(fm_byte) * switchPtr->vlanTableSize);
        }

        lagInfo->lag[lagIndex] = lagPtr;

        for (i = 0 ; i < FM_MAX_NUM_LAG_MEMBERS ; i++)
        {
            lagPtr->member[i].port = FM_LAG_UNUSED_PORT;
        }

        /* LAG filtering is enable by default on the LAG. */
        lagPtr->filteringEnabled = TRUE;

        lagPtr->logicalPort = lagLogicalPort;
        lagPtr->index = lagIndex;

        FM_API_CALL_FAMILY(err, switchPtr->CreateLagOnSwitch, sw, lagIndex);

        if (err != FM_OK)
        {
            /* We couldn't create the LAG on the switch */
            lagInfo->lag[lagIndex] = NULL;
            if (lagPtr->vlanMembership)
            {
                fmFree(lagPtr->vlanMembership);
            }
            fmFree(lagPtr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        FM_SET_LAG_IN_USE(lagInfo, lagIndex);

        /* Save the lag index in the LAG logical port structure. The LAG
           logical port has been created and initialized in the chip specific
           code (in CreateLagOnSwitch). */
        portPtr = GET_PORT_PTR(sw, lagPtr->logicalPort);
        portPtr->lagIndex = lagIndex;

        /* Return the LAG logical port as LAG number. */
        *lagNumber = lagPtr->logicalPort;
    }
    else
    {
        err = FM_ERR_LAG_IN_USE;
    }

ABORT:
    DROP_LAG_LOCK(sw);
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LAG,
                       err,
                       "Exit Status %d (%s), lagNumber = %d\n",
                       err,
                       fmErrorMsg(err),
                       *lagNumber);

}   /* end fmCreateLAGInt */




/*****************************************************************************/
/** fmAddLAGMember
 * \ingroup intLag
 *
 * \desc            Adds a member to the LAG internal software structure.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[in]       port is the logical port
 *
 * \return          number of ports in the lag
 *
 *****************************************************************************/
fm_status fmAddLAGMember(fm_int sw, fm_int lagIndex, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_lag *   lagPtr;
    fm_status  err;
    fm_int     index;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LAG,
                         "sw = %d, lagIndex = %d, port = %d\n",
                         sw,
                         lagIndex,
                         port);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate LAG index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);
    if (lagPtr == NULL)
    {
        err = FM_ERR_INVALID_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    FM_LOG_DEBUG_VERBOSE(
        FM_LOG_CAT_LAG,
        "port: lagIndex = %d, memberIndex = %d\n",
        portPtr->lagIndex,
        portPtr->memberIndex);

    /* See if port is already in the LAG. */
    if (portPtr->lagIndex == lagIndex &&
        portPtr->memberIndex >= 0)
    {
        err = FM_OK;
        goto ABORT;
    }

    /* Error if the port is in another LAG. */
    if (portPtr->lagIndex != lagIndex &&
        portPtr->lagIndex >= 0)
    {
        err = FM_ERR_ALREADYUSED_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Get the next index in which the new member should be added */
    index = lagPtr->nbMembers;

    /* Validate that we are not exceeding the table */
    if (index >= FM_MAX_NUM_LAG_MEMBERS)
    {
        err = FM_ERR_FULL_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    if (lagPtr->member[index].port != FM_LAG_UNUSED_PORT)
    {
        /* Should never happen */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Add the port to the LAG in the next member structure */
    lagPtr->member[index].port     = port;
    lagPtr->nbMembers++;

    portPtr->lagIndex = lagIndex;
    portPtr->memberIndex = index;

    /* Sort the members by glort */
    err = fmSortLagMembers(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmAddLAGMember */





/*****************************************************************************/
/** fmGetLAGMemberPorts
 * \ingroup intLag
 *
 * \desc            Returns all member ports of a given lag or only the active
 *                  ones depending on the 'active' argument.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[out]      numPorts points to caller-allocated storage where this
 *                  function should place the number of ports in the portList.
 *
 * \param[out]      portList points to caller-allocated array where this
 *                  function should place the all the member ports. Ports
 *                  are sorted by glort.
 *
 * \param[in]       maxPorts is the size of the portList array. 
 *
 * \param[in]       active TRUE  indicates to return active (linkup) member
 *                  ports only. FALSE indicates to return all the member ports.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetLAGMemberPorts(fm_int  sw,
                              fm_int  lagIndex,
                              fm_int *numPorts,
                              fm_int *portList,
                              fm_int  maxPorts,
                              fm_bool active)
{
    fm_switch *switchPtr;
    fm_lag *   lagPtr;
    fm_uint32  i;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d lagIndex=%d maxPorts=%d\n",
                 sw,
                 lagIndex,
                 maxPorts);

    *numPorts = 0;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate LAG index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);
    if (lagPtr == NULL)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    for (i = 0 ; i < lagPtr->nbMembers ; i++)
    {
        if ( !active || fmIsPortLinkUp(sw, lagPtr->member[i].port) )
        {
            if (*numPorts >= maxPorts)
            {
                err = FM_ERR_BUFFER_FULL;
                goto ABORT;
            }

            portList[*numPorts] = lagPtr->member[i].port;
            (*numPorts)++;
        }
    }

ABORT:
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LAG, err, "numPorts=%d\n", *numPorts);

}   /* end fmGetLAGMemberPorts */




/*****************************************************************************/
/** fmGetLAGMemberPortsForPort
 * \ingroup intLag
 *
 * \desc            Returns all the member ports of a LAG given a member port.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       memberPort is one of the LAG member ports or the lag port.
 *
 * \param[out]      numPorts points to caller-allocated storage where this
 *                  function should place the number of ports in the portList.
 *
 * \param[out]      portList points to caller-allocated array where this
 *                  function should place the all the member ports.
 *
 * \param[in]       maxPorts is the size of the portList array. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetLAGMemberPortsForPort(fm_int sw,
                                     fm_int memberPort,
                                     fm_int *numPorts,
                                     fm_int *portList,
                                     fm_int maxPorts)
{
    fm_int    lagIndex;
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d port=%d maxPorts=%d\n",
                 sw,
                 memberPort,
                 maxPorts);

    lagIndex = fmGetPortLagIndex(sw, memberPort);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    err = fmGetLAGMemberPorts(sw, lagIndex, numPorts, portList, maxPorts, FALSE);

ABORT:
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LAG, err, "numPorts=%d\n", *numPorts);

}   /* end fmGetLAGMemberPortsForPort */




/*****************************************************************************/
/** fmGetLAGCardinalPortList
 * \ingroup intLag
 *
 * \desc            Return all logical member ports of a given LAG, or
 *                  the port itself if a logical port.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number of a LAG or a 
 *                  physical port.
 *
 * \param[out]      numMembers points to caller-allocated storage where this
 *                  function should place the number of ports in the members.
 *
 * \param[out]      members points to caller-allocated array where this
 *                  function should place the member ports.
 *
 * \param[in]       maxPorts is the size of the members array. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetLAGCardinalPortList(fm_int sw,
                                   fm_int port,
                                   fm_int *numMembers,
                                   fm_int *members,
                                   fm_int maxPorts)
{
    fm_status err;
    fm_int    numPorts;
    fm_int    numMem;
    fm_int    i;

    if (fmIsCardinalPort(sw, port))
    {
        /* return the port itself */
        members[0] = port;
        *numMembers = 1;
    }
    else
    {
        err = fmGetLAGMemberPortsForPort(sw,
                                         port,
                                         &numMem,
                                         members,
                                         maxPorts);
        if (err != FM_OK)
        {
            *numMembers = 0;
            return err;
        }

        numPorts = 0;

        /* Filter out the remote ports */
        for (i = 0 ; i < numMem ; i++)
        {
            if (fmIsCardinalPort(sw, members[i]))
            {
                members[numPorts++] = members[i];
            }
        }

        *numMembers = numPorts;
    }

    return FM_OK;

}   /* end fmGetLAGCardinalPortList */




/*****************************************************************************/
/** fmGetFirstPhysicalMemberPort
 * \ingroup intLag
 *
 * \desc            Return the first member physical port of a LAG
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port number of a LAG.
 *
 * \param[out]      physMember points to caller-allocated storage into which
 *                  the function will write the port number for a member of
 *                  the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the LAG has no physical members.
 *
 *****************************************************************************/
fm_status fmGetFirstPhysicalMemberPort(fm_int sw,
                                       fm_int port,
                                       fm_int *physMember)
{
    fm_status err;
    fm_int    numMembers;
    fm_int    members[FM_MAX_NUM_LAG_MEMBERS];

    err = fmGetLAGMemberPortsForPort(sw,
                                     port,
                                     &numMembers,
                                     members,
                                     FM_MAX_NUM_LAG_MEMBERS);
    if (err != FM_OK)
    {
        return err;
    }

    /* Assume all members will have the same properties
     * So we will just get the property from one member
     */
    if (numMembers <= 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    while (numMembers)
    {
        /* Find the first physical port from this member */
        if (fmIsCardinalPort(sw, members[numMembers-1]))
        {
            *physMember = members[numMembers-1];
            return FM_OK;
        }
        numMembers--;
    }

    /* We don't support getting info from LAG port
     * with all remote members
     */
    return FM_ERR_INVALID_PORT;

}   /* end fmGetFirstPhysicalMemberPort */




/*****************************************************************************/
/** fmGetLAGMemberIndex
 * \ingroup intLag
 *
 * \desc            Return the index of a member port. This index is consistent
 *                  across stacking switches, so the software can determine
 *                  which member port the packet is from.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       memberPort is a port member of a LAG.
 *
 * \param[out]      memberIndex points to caller-allocated storage into which
 *                  the function will write the member index for a member of
 *                  the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the port is not member of a LAG
 *
 *****************************************************************************/
fm_status fmGetLAGMemberIndex(fm_int sw, fm_int memberPort, fm_int *memberIndex)
{
    fm_status err = FM_OK;
    fm_int    index;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw=%d port=%d\n", sw, memberPort);

    /* The member index stored in the port is already sorted by glort */
    index = fmGetPortMemberIndex(sw, memberPort);

    if (index == -1)
    {
        *memberIndex = -1;
        err = FM_ERR_INVALID_PORT;
    }
    else
    {
        /* The index returned by fmGetPortMemberIndex() is 0 based.
         * However, 0 is the LAG's canonical port index, add 1 */
        *memberIndex = index + 1;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_LAG,
                       err,
                       "memberIndex=%d\n",
                       *memberIndex);

}   /* end fmGetLAGMemberIndex */




/*****************************************************************************/
/** fmRemoveLAGMember
 * \ingroup intLag
 *
 * \desc            Removes a member from the LAG internal software structure.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[in]       port is the logical port
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRemoveLAGMember(fm_int sw, fm_int lagIndex, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_lag *   lagPtr;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate LAG index and fetch pointer. */
    lagPtr = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);
    if (lagPtr == NULL)
    {
        err = FM_ERR_INVALID_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL)
    {
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    if (portPtr->lagIndex == lagIndex &&
        portPtr->memberIndex >= 0)
    {
        /* Replace the member by the last member */
        if ( (portPtr->memberIndex) != ((fm_int)lagPtr->nbMembers - 1) )
        {
            lagPtr->member[portPtr->memberIndex] = 
                lagPtr->member[lagPtr->nbMembers - 1];
        }

        /* Remove the last entry from the LAG member table. */
        lagPtr->member[lagPtr->nbMembers - 1].port = FM_LAG_UNUSED_PORT;
        lagPtr->nbMembers--;

        portPtr->lagIndex = -1;
        portPtr->memberIndex = -1;

        /* Make sure that the members are in sorted order after port removal */
        err = fmSortLagMembers(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmRemoveLAGMember */




/*****************************************************************************/
/** fmGetRemotePortDestMask
 * \ingroup intLag
 *
 * \desc            Returns the destMask for a given remote port based on the
 *                  forwarding entry.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the remote logical port
 *
 * \param[out]      destMask points to caller-allocated storage where this
 *                  function should place all the ports belong to this
 *                  remote port. May be null, in which case no value will be
 *                  returned.
 *
 * \param[out]      activeDestMask points to caller-allocated storage where this
 *                  function should place the active ports belong to this
 *                  remote port.
 *
 * \return          FM_OK if succesful.
 *
 *****************************************************************************/
fm_status fmGetRemotePortDestMask(fm_int       sw,
                                  fm_int       port,
                                  fm_portmask *destMask,
                                  fm_portmask *activeDestMask)
{
    fm_switch *  switchPtr;
    fm_int       internalPort;
    fm_int       memberPort;
    fm_status    err;
    fm_portmask  localDestMask;
    fm_portmask  localActiveDestMask;
#if FM_SUPPORT_SWAG
    fm_port *    portPtr;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, port = %d destMask = %p activeDestMask = %p\n", 
                 sw,
                 port,
                 (void *) destMask,
                 (void *) activeDestMask);

    if (destMask)
    {
        FM_PORTMASK_DISABLE_ALL(destMask);
    }

    FM_PORTMASK_DISABLE_ALL(activeDestMask);
    FM_PORTMASK_DISABLE_ALL(&localDestMask);
    FM_PORTMASK_DISABLE_ALL(&localActiveDestMask);

    /* This is the remote logical port, so use the internal port */
    err = fmGetInternalPortFromRemotePort(sw, port, &internalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    switchPtr = GET_SWITCH_PTR(sw);

    if (!fmIsCardinalPort(sw, internalPort))
    {
        fm_int intMemberPorts[FM_MAX_NUM_LAG_MEMBERS];
        fm_int intPortCount;

        /* The internal port is a LAG port */
        err = fmGetLAGMemberPortsForPort(sw,
                                         internalPort,
                                         &intPortCount,
                                         intMemberPorts,
                                         FM_MAX_NUM_LAG_MEMBERS);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        while (intPortCount)
        {
            intPortCount--;
            memberPort = intMemberPorts[intPortCount];

#if FM_SUPPORT_SWAG

            /* If we are SWAG in stacking mode, it is possible that
             * the internal port is a remote port. In this case 
             * we need to search one level deeper to retrieve the 
             * physical ports.
             */

            portPtr = switchPtr->portTable[memberPort];

            if (portPtr->portType == FM_PORT_TYPE_REMOTE)
            {
                err = fmGetRemotePortDestMask(sw, memberPort, &localDestMask, &localActiveDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
                if (destMask)
                {
                    FM_OR_PORTMASKS(destMask, destMask, &localDestMask);
                }
                FM_OR_PORTMASKS(activeDestMask, activeDestMask, &localActiveDestMask);
                continue;
            }

#endif
            
            if (!fmIsCardinalPort(sw, memberPort))
            {
                /* Member port of an internal port must be a local port */
                err = FM_ERR_INVALID_PORT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }

            if (fmIsPortLinkUp(sw, memberPort))
            {
                fmEnablePortInPortMask(sw, activeDestMask, memberPort);
            }

            if (destMask)
            {
                fmEnablePortInPortMask(sw, destMask, memberPort);
            }

        }   /* end while (intPortCount) */
    }
    else
    {
        /* When get here, the port is an internal physical port */
        if (fmIsPortLinkUp(sw, internalPort))
        {
            fmEnablePortInPortMask(sw, activeDestMask, internalPort);
        }

        if (destMask)
        {
            fmEnablePortInPortMask(sw, destMask, internalPort);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmGetRemotePortDestMask */




/*****************************************************************************/
/** fmGetLAGForPort
 * \ingroup intLag
 *
 * \desc            Returns the LAG number to which a port belongs.
 *
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the port number
 *
 * \return          lag index if the port is a member of a lag
 * \return          -1 if the port is not a member of a lag
 *
 *****************************************************************************/
fm_int fmGetLAGForPort(fm_int sw, fm_int port)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate port number and fetch pointer. */
    portPtr = GET_SWITCH_PORT_PTR(switchPtr, port);
    if (portPtr == NULL)
    {
        return -1;
    }

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_LAG,
                         "port = %d, "
                         "port.lagIndex = %d, port.memberIndex = %d\n",
                         port,
                         portPtr->lagIndex,
                         portPtr->memberIndex);

    /* Verify that port is a member of a LAG. */
    if (portPtr->lagIndex >= 0 &&
        portPtr->memberIndex >= 0)
    {
        return switchPtr->lagInfoTable.lag[portPtr->lagIndex]->logicalPort;
    }

    return -1;

}   /* end fmGetLAGForPort */




/*****************************************************************************/
/** fmApplyLagMemberPortVlanStp
 * \ingroup intLag
 *
 * \desc            Configure the port with the LAG settings.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       lagIndex is the index of the LAG on the switch.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmApplyLagMemberPortVlanStp(fm_int sw, fm_int port, fm_int lagIndex)
{
    fm_switch *switchPtr;
    fm_lag *   lagPtr;
    fm_status  err = FM_OK;
    fm_int     vlan;
    fm_int     stp;
    fm_bool    tag;
    fm_byte    value;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw = %d, port = %d lagIndex = %d \n", 
                 sw, 
                 port,
                 lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_LAG_PTR(sw, lagIndex);

    /***************************************************     
     * Iterate through the list of VLANs the LAG is
     * member of and add the port to them. 
     * The VLAN tagging option and the STP state are 
     * also set on the port. 
     **************************************************/

    for (vlan = 0 ; vlan < switchPtr->vlanTableSize ; vlan++)
    {
        value = lagPtr->vlanMembership[vlan];

        if ( value & FM_LAG_VLAN_MEMBERSHIP && fmIsValidPort(sw, port, ALLOW_CPU | ALLOW_LAG) )
        {
            /* The LAG is member of this vlan */
            tag = (value & FM_LAG_VLAN_TAG) ? TRUE : FALSE;
            stp = (value & FM_LAG_VLAN_STP_MASK) >> FM_LAG_VLAN_STP_LOW_BIT;

            /* Add port to vlan */
            err = fmAddVlanPort(sw, (fm_uint16)vlan, port, tag);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            /* tagging state of vlan2 */
            tag = (value & FM_LAG_VLAN2_TAG) ? TRUE : FALSE;
            if (tag)
            {
                err = fmChangeVlanPortExt(sw,
                                          FM_VLAN_SELECT_VLAN2,
                                          (fm_uint16)vlan,
                                          port,
                                          tag);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }
    
            /* Set the STP state */
            err = fmSetVlanPortState(sw, (fm_uint16) vlan, port, stp);

            if (err == FM_ERR_PORT_IS_INTERNAL)
            {
                /* Port is internal and STP state must remain in FORWARDING */
                err = FM_OK;
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmApplyLagMemberPortVlanStp */




/*****************************************************************************/
/** fmSetLAGVlanMembership
 * \ingroup intLag
 *
 * \desc            Save VLAN membership and tagging state on the LAG.
 *                  So when a port is added to the LAG it will be
 *                  automatically added to this vlan and the tagging state
 *                  will also be applied.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the LAG logical port on which to operate
 *
 * \param[in]       state is TRUE if the LAG is to become a member of the
 *                  vlan, FALSE if it is to be removed from the vlan.
 * 
 * \param[in]       tag is the tagging state to save.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetLAGVlanMembership(fm_int    sw, 
                                 fm_uint16 vlanID, 
                                 fm_int    port, 
                                 fm_bool   state,
                                 fm_bool   tag)
{
    fm_lag * lagPtr;
    fm_port *portPtr;
    fm_int   stp;
    fm_byte  value = 0;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw=%d vlanID=%d port=%d tag=%d\n", 
                 sw, 
                 vlanID,
                 port,
                 tag);

    portPtr = GET_PORT_PTR(sw, port);

    if ( (portPtr == NULL) ||
         (portPtr->portType != FM_PORT_TYPE_LAG) ||
         (portPtr->lagIndex < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    lagPtr  = GET_LAG_PTR(sw, portPtr->lagIndex);

    if (state)
    {
        /* Mark the LAG as member of this vlan */
        value |= FM_LAG_VLAN_MEMBERSHIP;
    }

    if (tag)
    {
        /* Set tag bit*/
        value |= FM_LAG_VLAN_TAG;
    }

    /* Get default state for a port that is a member of a VLAN. */
    stp = fmGetIntApiProperty(FM_AAK_API_STP_DEF_STATE_VLAN_MEMBER,
                              FM_AAD_API_STP_DEF_STATE_VLAN_MEMBER);

    value |= stp << FM_LAG_VLAN_STP_LOW_BIT;

    lagPtr->vlanMembership[vlanID] = value;

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmSetLAGVlanMembership */




/*****************************************************************************/
/** fmSetLAGVlanPortState
 * \ingroup intLag
 *
 * \desc            Save the spanning tree state of a LAG in a VLAN.
 *                  So when a port is added to a LAG this STP state will be
 *                  automatically set on that port.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the LAG logical port on which to operate
 *
 * \param[in]       state is the spanning tree state to save.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetLAGVlanPortState(fm_int    sw, 
                                fm_uint16 vlanID, 
                                fm_int    port, 
                                fm_int    state)
{
    fm_lag * lagPtr;
    fm_port *portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw=%d vlanID=%d port=%d state=%d\n", 
                 sw, 
                 vlanID,
                 port,
                 state);

    portPtr = GET_PORT_PTR(sw, port);

    if ( (portPtr == NULL) ||
         (portPtr->portType != FM_PORT_TYPE_LAG) ||
         (portPtr->lagIndex < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    lagPtr  = GET_LAG_PTR(sw, portPtr->lagIndex);

    lagPtr->vlanMembership[vlanID] &= ~FM_LAG_VLAN_STP_MASK;
    lagPtr->vlanMembership[vlanID] |= (state << FM_LAG_VLAN_STP_LOW_BIT);

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmSetLAGVlanPortState */




/*****************************************************************************/
/** fmGetLAGVlanPortState
 * \ingroup intLag
 *
 * \desc            Retrieve the spanning tree state of a LAG in a VLAN.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the LAG logical port on which to operate
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function should place the spanning tree state.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetLAGVlanPortState(fm_int    sw, 
                                fm_uint16 vlanID, 
                                fm_int    port, 
                                fm_int *  state)
{
    fm_lag *  lagPtr;
    fm_port * portPtr;
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw=%d vlanID=%d port=%d state=%p\n", 
                 sw, 
                 vlanID,
                 port,
                 (void *) state);

    portPtr = GET_PORT_PTR(sw, port);

    if ( (portPtr == NULL) ||
         (portPtr->portType != FM_PORT_TYPE_LAG) ||
         (portPtr->lagIndex < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    lagPtr  = GET_LAG_PTR(sw, portPtr->lagIndex);

    /* First make sure the LAG is member of the VLAN. */
    if (lagPtr->vlanMembership[vlanID] & FM_LAG_VLAN_MEMBERSHIP)
    {
        *state = (lagPtr->vlanMembership[vlanID] & FM_LAG_VLAN_STP_MASK) >>
                                                        FM_LAG_VLAN_STP_LOW_BIT;
        err = FM_OK;
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }


    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGVlanPortState */




/*****************************************************************************/
/** fmSetLAGVlanTag
 * \ingroup intLag
 *
 * \desc            Save the tagging state of a LAG in a VLAN.
 *                  So when a port is added to the LAG this tagging state will
 *                  be automatically set on that port.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the LAG logical port on which to operate
 *
 * \param[in]       tag is the tagging state to save.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetLAGVlanTag(fm_int        sw, 
                          fm_vlanSelect vlanSel,
                          fm_uint16     vlanID, 
                          fm_int        port, 
                          fm_bool       tag)
{
    fm_lag * lagPtr;
    fm_port *portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw=%d vlanSel=%d vlanID=%d port=%d tag=%d\n", 
                 sw, 
                 vlanSel,
                 vlanID,
                 port,
                 tag);

    portPtr = GET_PORT_PTR(sw, port);

    if ( (portPtr == NULL) ||
         (portPtr->portType != FM_PORT_TYPE_LAG) ||
         (portPtr->lagIndex < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    lagPtr  = GET_LAG_PTR(sw, portPtr->lagIndex);

    if (tag)
    {
        if (vlanSel == FM_VLAN_SELECT_VLAN1)
        {
            lagPtr->vlanMembership[vlanID] |= FM_LAG_VLAN_TAG;
        }
        else
        {
            lagPtr->vlanMembership[vlanID] |= FM_LAG_VLAN2_TAG;
        }
    }
    else
    {
        if (vlanSel == FM_VLAN_SELECT_VLAN1)
        {
            lagPtr->vlanMembership[vlanID] &= ~FM_LAG_VLAN_TAG;
        }
        else
        {
            lagPtr->vlanMembership[vlanID] &= ~FM_LAG_VLAN2_TAG;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fmSetLAGVlanTag */




/*****************************************************************************/
/** fmGetLAGVlanTag
 * \ingroup intLag
 *
 * \desc            Retrieve the tagging state for a LAG in a VLAN.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and claimed the LAG lock.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanSel define which VLAN to select.
 *
 * \param[in]       port is the port on which to operate.
 * 
 * \param[in]       vlanID is the VLAN number on which to operate.
 *
 * \param[in]       port is the LAG logical port on which to operate
 *
 * \param[out]      tag points to caller-allocated storage where this
 *                  function should place the boolean tagging state.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmGetLAGVlanTag(fm_int        sw, 
                          fm_vlanSelect vlanSel,
                          fm_uint16     vlanID, 
                          fm_int        port, 
                          fm_bool      *tag)
{
    fm_lag *  lagPtr;
    fm_port * portPtr;
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw=%d vlanID=%d port=%d tag=%p\n", 
                 sw, 
                 vlanID,
                 port,
                 (void *) tag);

    portPtr = GET_PORT_PTR(sw, port);

    if ( (portPtr == NULL) ||
         (portPtr->portType != FM_PORT_TYPE_LAG) ||
         (portPtr->lagIndex < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_PORT);
    }

    lagPtr  = GET_LAG_PTR(sw, portPtr->lagIndex);

    /* First make sure the LAG is member of the VLAN. */
    if (lagPtr->vlanMembership[vlanID] & FM_LAG_VLAN_MEMBERSHIP)
    {
        if (vlanSel == FM_VLAN_SELECT_VLAN1)
        {
            *tag = lagPtr->vlanMembership[vlanID] & FM_LAG_VLAN_TAG;
        }
        else
        {
            *tag = (lagPtr->vlanMembership[vlanID] & FM_LAG_VLAN2_TAG) >> 1;
        }
        err = FM_OK;
    }
    else
    {
        err = FM_ERR_INVALID_PORT;
    }


    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGVlanTag */




/*****************************************************************************/
/** fmLAGPortAddDelete
 * \ingroup intLag
 *
 * \desc            Updates the hardware configuration and software state 
 *                  of a LAG following a change in state (addition, deletion, 
 *                  port up/down) of one or more of its member ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fmLAGPortAddDelete(fm_int sw, fm_int lagIndex)
{
    fm_switch *        switchPtr;
    fm_lag *           lagPtr;
    fm_port *          portPtr;
    fm_lagInfo *       lagInfo;
    fm_status          err = FM_OK;
    fm_int             ports[FM_MAX_NUM_LAG_MEMBERS];
    fm_int             portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int             nPorts = 0;
    fm_int             numPorts;
    fm_int             numRemotePorts = 0;
    fm_int             internalPort;
    fm_int             lastInternalPort;
    fm_int             cnt;
    fm_int             i;
    fm_int             port;
    fm_int             physPort;
    fm_uint32          destIndex;
    fm_portmask        activeDestMask;
    fm_portmask        destMask;
    fm_glortDestEntry *destEntry;
    fm_glortCamEntry * camEntry;
    fm_bool            doPruning = FALSE;
    fm_int             numEntriesDesired;

   
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n", 
                 sw,
                 lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    lagInfo   = GET_LAG_INFO_PTR(sw);
    lagPtr    = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);

    if (lagPtr == NULL)
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    portPtr = GET_SWITCH_PORT_PTR(switchPtr, lagPtr->logicalPort);
    if (portPtr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Zero out the destination port mask */
    FM_PORTMASK_DISABLE_ALL(&destMask);

    /**************************************************
     * Find all lag ports that are up
     **************************************************/

    /* Get the list of ports member of this LAG */
    err = fmGetLAGMemberPorts(sw,
                              lagIndex,
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    for (cnt = 0 ; cnt < numPorts ; cnt++)
    {
        port = portList[cnt];

        if ( (fmIsPortLinkUp(sw, port)) && 
             (nPorts < switchPtr->info.maxPortsPerLag) )
        {
            if (fmIsRemotePort(sw, port))
            {
                err = fmGetRemotePortDestMask(sw,
                                              port,
                                              NULL,
                                              &activeDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

                /* Enable ports from active mask in the destination port mask */
                FM_OR_PORTMASKS(&destMask, &destMask, &activeDestMask)
                ++numRemotePorts;
            }
            else
            {
                /* Enable this port in destination port mask */
                fmEnablePortInPortMask(sw, &destMask, port);
            }

            ports[nPorts] = port;
            ++nPorts;
        }
        else
        {
            if (fmIsCardinalPort(sw, port))
            {
                err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
 
                FM_API_CALL_FAMILY(err, 
                                   switchPtr->SetPortLAGConfig,
                                   sw,
                                   physPort,
                                   0,                           // port index
                                   1,                           // lag size
                                   lagPtr->hashRotation,
                                   FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }
         }

    }   /* end for (cnt = 0 ; cnt < numPorts ; cnt++) */
 
    if (numRemotePorts)
    {
        /**************************************************
         * Ports must be sorted in order to guarantee 
         * the index for LAG_PORT_TABLE is consistent for 
         * each port on all the switches otherwise will get 
         * duplicate traffic and good packets will be 
         * dropped instead. Ports should be in sort order
         * when they are retrieved from 
         * fmGetLAGMemberPorts(), no need to sort again.
         **************************************************/
         
        if (!lagInfo->pruningDisabled)
        {
            /* Do pruning if not all members on one switch */
            if (numRemotePorts != nPorts)
            {
                doPruning = TRUE;
            }
            else
            {
                /* If all remote ports have multiple internal ports */
                lastInternalPort = -1;
                for (i = 0 ; i < nPorts ; i++)
                {
                    if (fmIsRemotePort(sw, ports[i]))
                    {
                        err = fmGetInternalPortFromRemotePort(sw,
                                                              ports[i],
                                                              &internalPort);
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

                        if (lastInternalPort > 0)
                        {
                            if (lastInternalPort != internalPort)
                            {
                                /* Can't do filtering */
                                doPruning = TRUE;
                                break;
                            }
                        }
                        lastInternalPort = internalPort;
                    }
                }
            }

        }   /* end if (!lagInfo->pruningDisabled) */

    }   /* end if (numRemotePorts) */


    /**************************************************
     * Set LAG dest mask in dest table entry.
     **************************************************/

    destIndex = portPtr->destEntry->destIndex;
    destEntry = &switchPtr->logicalPortInfo.destEntries[destIndex];
    camEntry  = portPtr->camEntry;

    if (!doPruning)
    {
        /* No pruning required since all ports are on one switch */
        FM_API_CALL_FAMILY(err,
                           switchPtr->SetGlortDestMask,
                           sw,
                           destEntry,
                           &destMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        
        /* Restore back to LAG filtering */
        if ( (camEntry->destCount != 1) || (camEntry->destIndex != destIndex) )
        {
            camEntry->destCount = 1;
            camEntry->destIndex = destIndex;

            FM_API_CALL_FAMILY(err,
                               switchPtr->WriteGlortCamEntry,
                               sw,
                               camEntry,
                               FM_UPDATE_CAM_AND_RAM);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        if (portPtr->numDestEntries > 1)
        {
            /**************************************************
             * Free extra allocated dest entries, if any
             **************************************************/

            for (i = 1 ; i < portPtr->numDestEntries ; i++)
            {
                FM_API_CALL_FAMILY(err,
                                   switchPtr->FreeDestEntry,
                                   sw,
                                   &switchPtr->logicalPortInfo.
                                         destEntries[camEntry->destIndex+i]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }

            portPtr->numDestEntries = 1;
        }
    }
    else
    {
        /* Allocate in chunks of 4-entry to minimize fragments in dest table */
        numEntriesDesired = FM_CEIL_CHUNK_OF_4(nPorts);
        

        /* Not enough dest entries, get new block */
        if (nPorts > portPtr->numDestEntries)
        {
            fm_glortDestEntry *newDestEntries[FM_MAX_NUM_LAG_MEMBERS];

            /**************************************************
             * Free old dest table entries first, so we can reuse
             **************************************************/
            for (i = 0 ; i < portPtr->numDestEntries ; i++)
            {
                FM_API_CALL_FAMILY(err,
                                   switchPtr->FreeDestEntry,
                                   sw,
                                   &switchPtr->logicalPortInfo.
                                         destEntries[camEntry->destIndex+i]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }

            portPtr->numDestEntries = numEntriesDesired; 

            /**************************************************
             * NOTE: Instead of just allocating new entries
             *       We could check if the subsequent entries
             *       are free and just claim it
             **************************************************/

            err = fmAllocDestEntries(sw,
                                     portPtr->numDestEntries,
                                     camEntry,
                                     newDestEntries,
                                     FM_PORT_TYPE_LAG);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            destEntry = newDestEntries[0];
            portPtr->destEntry = destEntry;
        }
        else if (portPtr->numDestEntries > numEntriesDesired)
        {
            /* Free entries when too many are unused */
            for (i = numEntriesDesired ; i < portPtr->numDestEntries ; i++)
            {
                FM_API_CALL_FAMILY(err,
                                   switchPtr->FreeDestEntry,
                                   sw,
                                   &switchPtr->logicalPortInfo.
                                         destEntries[camEntry->destIndex+i]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }

            portPtr->numDestEntries = numEntriesDesired;

        }   /* end if (portExt->numDestEntries > numEntriesDesired) */

        /**************************************************
         * Set dest mask on each entry.
         **************************************************/

        for (cnt = 0 ; cnt < nPorts ; cnt++)
        {
            port = ports[cnt];
            if (fmIsRemotePort(sw, port))
            {
                err = fmGetRemotePortDestMask(sw, port, NULL, &activeDestMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            }
            else
            {
                fmAssignPortToPortMask(sw, &activeDestMask, port);
            }

            FM_API_CALL_FAMILY(err,
                               switchPtr->SetGlortDestMask,
                               sw,
                               &destEntry[cnt],
                               &activeDestMask);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        /**************************************************
         * Update glort ram & cam.
         **************************************************/

        camEntry->destCount = nPorts;
        camEntry->destIndex = destEntry[0].destIndex;

        /* Under pruning the cam is doing the hash */
        camEntry->hashRotation = lagPtr->hashRotation;

        FM_API_CALL_FAMILY(err,
                           switchPtr->WriteGlortCamEntry,
                           sw,
                           camEntry,
                           FM_UPDATE_CAM_AND_RAM);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /**************************************************
     * Update the hardware config for each LAG port
     **************************************************/

    for (i = 0 ; i < nPorts ; i++)
    {
        if (!fmIsCardinalPort(sw, ports[i]))
        {
            /* Only apply to physical ports */
            continue;
        }

        err = fmMapLogicalPortToPhysical(switchPtr, ports[i], &physPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
 
        FM_API_CALL_FAMILY(err, 
                           switchPtr->SetPortLAGConfig,
                           sw,
                           physPort,
                           i,
                           nPorts,
                           lagPtr->hashRotation,
                           lagPtr->filteringEnabled);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmLAGPortAddDelete */




/*****************************************************************************/
/** fmGetLAGVlanAttribute
 * \ingroup intLag
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves a VLAN attribute for a LAG port.
 * 
 * \note            This is an internal function. It assumes that the
 *                  caller has taken the switch and LAG locks and has
 *                  validated the VLAN and port numbers.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN number from which to get the attribute.
 * 
 * \param[in]       port is the LAG port number.
 *
 * \param[in]       attr is the VLAN port attribute (see 'VLAN Port Attributes')
 *                  to retrieve.
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_INVALID_LAG if LAG is invalid.
 * \return          FM_ERR_UNSUPPORTED if per-lag attributes not supported.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fmGetLAGVlanAttribute(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    port,
                                fm_int    attr,
                                void *    value)
{
    fm_int      lagIndex;
    fm_lag *    lagPtr;
    fm_byte     lagVlan;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_LAG,
                     "sw=%d vlanID=%u port=%d attr=%d value=%p\n",
                     sw, vlanID, port, attr, (void *) value);

    lagIndex = fmGetLagIndex(sw, port);
    if (lagIndex < 0)
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    lagPtr  = GET_LAG_PTR(sw, lagIndex);
    if (lagPtr == NULL)
    {
        err = FM_ERR_INVALID_LAG;
        goto ABORT;
    }

    if (lagPtr->vlanMembership == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    lagVlan = lagPtr->vlanMembership[vlanID];

    switch (attr)
    {
        case FM_VLAN_PORT_MEMBERSHIP:
            *((fm_bool *) value) = (lagVlan & FM_LAG_VLAN_MEMBERSHIP) != 0;
            break;

        case FM_VLAN_PORT_TAG1:
            *((fm_bool *) value) = (lagVlan & FM_LAG_VLAN_TAG) != 0;
            break;

        case FM_VLAN_PORT_TAG2:
            *((fm_bool *) value) = (lagVlan & FM_LAG_VLAN2_TAG) != 0;
            break;

        case FM_VLAN_PORT_STP_STATE:
            *((fm_int *) value) = (lagVlan >> FM_LAG_VLAN_STP_LOW_BIT) & 7;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT_API(FM_LOG_CAT_LAG, err);

}   /* end fmGetLAGVlanAttribute */




/*****************************************************************************/
/** fmSetLagListVlanMembership
 * \ingroup intLag
 *
 * \desc            Sets the VLAN membership and tagging state for a list of
 *                  LAG ports. Assumes that the caller has taken the switch
 *                  lock, and validated the VLAN number and port list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN to be updated.
 * 
 * \param[in]       numPorts is the number of entries in the port list.
 * 
 * \param[in]       portList points to an array containing a list of LAG
 *                  logical ports.
 *
 * \param[in]       state is TRUE if the LAG is to become a member of the
 *                  vlan, FALSE if it is to be removed from the vlan.
 * 
 * \param[in]       tag is the tagging state to save.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetLagListVlanMembership(fm_int     sw,
                                     fm_uint16  vlanID,
                                     fm_int     numPorts,
                                     fm_int *   portList,
                                     fm_bool    state,
                                     fm_bool    tag)
{
    fm_int      index;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d vlanID=%u numPorts=%d state=%d tag=%d\n",
                 sw, vlanID, numPorts, state, tag);

    TAKE_LAG_LOCK(sw);

    for (index = 0 ; index < numPorts ; index++)
    {
        err = fmSetLAGVlanMembership(sw, vlanID, portList[index], state, tag);
        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_LAG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmSetLagListVlanMembership */




/*****************************************************************************/
/** fmSetLagListVlanPortState
 * \ingroup intLag
 * 
 * \desc            Sets the spanning tree state of a list of LAGS in a VLAN.
 * 
 * \note            This is an internal function. The caller is assumed
 *                  to have protected the switch and validated the VLAN number
 *                  and port list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanID is the VLAN to be updated.
 * 
 * \param[in]       numPorts is the number of entries in the port list.
 * 
 * \param[in]       portList points to an array containing a list of LAG
 *                  logical ports.
 *
 * \param[in]       state is the spanning tree state.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetLagListVlanPortState(fm_int     sw,
                                    fm_uint16  vlanID,
                                    fm_int     numPorts,
                                    fm_int *   portList,
                                    fm_int     state)
{
    fm_int      index;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d vlanID=%u numPorts=%d state=%d\n",
                 sw, vlanID, numPorts, state);

    TAKE_LAG_LOCK(sw);

    for (index = 0 ; index < numPorts ; index++)
    {
        err = fmSetLAGVlanPortState(sw, vlanID, portList[index], state);
        if (err != FM_OK)
        {
            break;
        }
    }

    DROP_LAG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fmSetLagListVlanPortState */

