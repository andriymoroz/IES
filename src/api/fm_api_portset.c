/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_portset.c
 * Creation Date:   May 30th, 2013.
 * Description:     PortSet Management.
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

#define IS_PORTSET_INTERNAL(portSet)            (portSet < 0)
#define GET_INTERNAL_PORTSET_INDEX(portSet)     ((-portSet) - 1)


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
/** fmFreePortSet
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Frees an fm_portSet entry
 * 
 * \param[in]       ptr is a pointer to a fm_portSet structure
 * 
 * \return          None.
 *
 *****************************************************************************/
static void fmFreePortSet(void * ptr)
{
    fm_status err;
    fm_portSet *portset;

    portset = (fm_portSet *) ptr;

    err = fmDeleteBitArray(&portset->associatedPorts);

    if (err != FM_OK)
    {
        FM_LOG_WARNING(FM_LOG_CAT_PORT,
                       "Failed to delete fm_portSet.associatedPorts\n");
    }

    fmFree(ptr);

}   /* end fmFreePortSet */




/*****************************************************************************/
/** ReservePortSet
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Reserves the first free portset available
 * 
 * \note            This function assumes that the portSet lock has been taken.
 *                                                                      
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      portSet points to caller-allocated storage where
 *                  this function should place the port set number
 *                  (handle) of the newly created port set.
 * 
 * \param[in]       isInternal should be set to true if an internal
 *                  portSet is required. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_PORT_SET if the maximum number of port
 *                  sets have already been created.
 *
 *****************************************************************************/
static fm_status ReservePortSet(fm_int sw, fm_int *portSet, fm_bool isInternal)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_bitArray *bitArrayPtr;
    fm_int       freePortSet;

    switchPtr = GET_SWITCH_PTR(sw);

    if (isInternal)
    {
        bitArrayPtr = &switchPtr->portSetInfo.portSetUsageInt;
    }
    else
    {
        bitArrayPtr = &switchPtr->portSetInfo.portSetUsage;
    }

    err = fmFindBitInBitArray(bitArrayPtr, 
                              0, 
                              FM_PORT_SET_UNUSED,
                              &freePortSet);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    if (freePortSet == -1)
    {
        err = FM_ERR_NO_FREE_PORT_SET;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }
    else
    {
        err = fmSetBitArrayBit(bitArrayPtr, 
                               freePortSet, 
                               FM_PORT_SET_RESERVED);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

        if (isInternal)
        {
            /* Internal portsets have a negative value, the sign bit,
             * should always be set */
            *portSet = -(freePortSet + 1);
        }
        else
        {
            *portSet = freePortSet;
        }
    }

ABORT:
    return err;

}   /* end ReservePortSet */




/*****************************************************************************/
/** ReleasePortSet
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Releases a previously reserved portSet and marks it
 *                  as unused.
 * 
 * \note            This function assumes that the portSet lock has been taken.
 *                                                                      
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the portSet to release. Can be public or
 *                  internal.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status ReleasePortSet(fm_int sw, fm_int portSet)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_int       portSetIndex;
    fm_bitArray *bitArrayPtr;

    switchPtr = GET_SWITCH_PTR(sw);
    
    /* Negative portSets are internal */
    if (IS_PORTSET_INTERNAL(portSet))
    {
        /* Get the internal portSet index */
        portSetIndex = GET_INTERNAL_PORTSET_INDEX(portSet);
        bitArrayPtr  = &switchPtr->portSetInfo.portSetUsageInt;
    }
    else
    {
        portSetIndex = portSet;
        bitArrayPtr  = &switchPtr->portSetInfo.portSetUsage;
    }

    err = fmSetBitArrayBit(bitArrayPtr, 
                           portSetIndex, 
                           FM_PORT_SET_UNUSED);
    
    return err;

}   /* end ReleasePortSet */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmInitPortSetTable
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Initializes the portset table. 
 *
 * \param[in]       switchPtr points to the switch table entry.
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
fm_status fmInitPortSetTable(fm_switch *switchPtr)
{
    fm_status        err;
    fm_int           sw;
    fm_portSetInfo * psi;
    fm_int           portSetAll;
    fm_int           portSetAllButCpu;
    fm_int           portSetAllExternal;
    fm_int           portSetNone;
    fm_int           portSetCpu;
    fm_int           cpi;
    fm_int           logPort;
#if FM_SUPPORT_SWAG
    fm_int           swagId;
    fmSWAG_switch *  aggExt;
    fm_swagIntLink * curLink;
#endif
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "switchPtr = %p\n",
                     (void *) switchPtr);

    sw  = switchPtr->switchNumber;
    psi = &switchPtr->portSetInfo;

    /* Initialise the port set tree */
    fmTreeInit(&psi->portSetTree);

    /* Initialize the fm_bitArrays that track portset usage */
    err = fmCreateBitArray(&psi->portSetUsage, switchPtr->maxPortSets);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmCreateBitArray(&psi->portSetUsageInt, FM_NB_INTERNAL_PORTSET);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /********************************************* 
     * Initialize the three static portsets:
     *   FM_PORT_SET_ALL
     *   FM_PORT_SET_ALL_BUT_CPU
     *   FM_PORT_SET_ALL_EXTERNAL
     *********************************************/

    err = fmCreatePortSetInt(sw, &portSetAll, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Protection to prevent erroneous static portSet */
    if (portSetAll != FM_PORT_SET_ALL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Could not initialize internal portset FM_PORT_SET_ALL\n");
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err);
    }

    err = fmCreatePortSetInt(sw, &portSetAllButCpu, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Protection to prevent erroneous static portSet */
    if (portSetAllButCpu != FM_PORT_SET_ALL_BUT_CPU)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Could not initialize internal portset FM_PORT_SET_ALL_BUT_CPU\n");
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err);
    }

    err = fmCreatePortSetInt(sw, &portSetAllExternal, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Protection to prevent erroneous static portSet */
    if (portSetAllExternal != FM_PORT_SET_ALL_EXTERNAL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Could not initialize internal portset FM_PORT_SET_ALL_EXTERNAL\n");
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err);
    }

    err = fmCreatePortSetInt(sw, &portSetNone, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Protection to prevent erroneous static portSet */
    if (portSetNone != FM_PORT_SET_NONE)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Could not initialize internal portset FM_PORT_SET_NONE\n");
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err);
    }

    err = fmCreatePortSetInt(sw, &portSetCpu, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Protection to prevent erroneous static portSet */
    if (portSetCpu != FM_PORT_SET_CPU)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Could not initialize internal portset FM_PORT_SET_CPU\n");
        err = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PORT, err);
    }

    err = fmAddPortSetPortInt(sw, FM_PORT_SET_ALL, switchPtr->cpuPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmAddPortSetPortInt(sw, FM_PORT_SET_CPU, switchPtr->cpuPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        logPort = GET_LOGICAL_PORT(sw, cpi);

        if (logPort != switchPtr->cpuPort)
        {
            err = fmAddPortSetPortInt(sw, FM_PORT_SET_ALL, logPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            err = fmAddPortSetPortInt(sw, FM_PORT_SET_ALL_BUT_CPU, logPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            err = fmAddPortSetPortInt(sw, FM_PORT_SET_ALL_EXTERNAL, logPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
        }
    }

#if FM_SUPPORT_SWAG

    /* Remove SWAG Internal ports from FM_PORT_SET_ALL_EXTERNAL portSet */
    swagId = switchPtr->swag;

    if (swagId >= 0)
    {
        aggExt  = GET_SWITCH_EXT(swagId);
        curLink = fmGetFirstLinkInLinkList(aggExt);

        while (curLink != NULL)
        {
            if ((curLink->link.swId == sw) &&
                (curLink->link.type != FM_SWAG_LINK_EXTERNAL))
            {
                err = fmDeletePortSetPortInt(sw,
                                             FM_PORT_SET_ALL_EXTERNAL,
                                             curLink->link.swPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }
            else if ((curLink->link.partnerSwitch == sw) &&
                     (curLink->link.type != FM_SWAG_LINK_EXTERNAL))
            {
                err = fmDeletePortSetPortInt(sw,
                                             FM_PORT_SET_ALL_EXTERNAL,
                                             curLink->link.partnerPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
            }

            curLink = fmGetNextLinkInLinkList(curLink);
        }
    }
    else if (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG)
    {
        aggExt  = GET_SWITCH_EXT(sw);
        curLink = fmGetFirstLinkInLinkList(aggExt);

        while (curLink != NULL)
        {
            if (curLink->link.type != FM_SWAG_LINK_EXTERNAL)
            {
                err = fmDeletePortSetPortInt(sw,
                                             FM_PORT_SET_ALL_EXTERNAL,
                                             curLink->link.logicalPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

                if (curLink->link.type == FM_SWAG_LINK_INTERNAL)
                {
                    err = fmDeletePortSetPortInt(sw,
                                                 FM_PORT_SET_ALL_EXTERNAL,
                                                 curLink->link.partnerLogicalPort);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
                }
            }

            curLink = fmGetNextLinkInLinkList(curLink);
        }
    }

#endif

ABORT:
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmInitPortSetTable */




/*****************************************************************************/
/** fmDestroyPortSetTable
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Destroys the portset table.
 *
 * \param[in]       psi points to the fm_portSetTable structure
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDestroyPortSetTable(fm_portSetInfo *psi)
{
    fm_status err; 

    FM_LOG_ENTRY(FM_LOG_CAT_PORT, "psi = %p\n", (void *) psi);

    fmTreeDestroy(&psi->portSetTree, fmFreePortSet);
    
    err = fmDeleteBitArray(&psi->portSetUsage);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmDeleteBitArray(&psi->portSetUsageInt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmDestroyPortSetTable */




/*****************************************************************************/
/** fmCreatePortSet
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Creates a port set.
 *                                                                      \lb\lb
 *                  Once the port set is created, ports can be individually
 *                  associated with the port set by calling
 *                  ''fmAddPortSetPort''. 
 *
 * \note            The user application is responsible for managing port sets,
 *                  deleting those that are no longer needed and avoiding
 *                  redundant sets.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      portSet points to caller-allocated storage where
 *                  this function should place the port set number
 *                  (handle) of the newly created port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_PORT_SET if the maximum number of port
 *                  sets have already been created.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated
 * \return          FM_ERR_INVALID_ARGUMENT if portSet is NULL.
 *
 *****************************************************************************/
fm_status fmCreatePortSet(fm_int sw, fm_int *portSet)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %p\n",
                     sw,
                     (void *) portSet);

    if (portSet == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    /* Create public portset */
    err =  fmCreatePortSetInt(sw, portSet, FALSE);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmCreatePortSet */




/*****************************************************************************/
/** fmCreatePortSetInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Creates a port set.
 *                                                                      \lb\lb
 *                  Once the port set is created, ports can be individually
 *                  associated with the port set by calling
 *                  ''fmAddPortSetPort''. 
 *
 * \note            The user application is responsible for managing port sets,
 *                  deleting those that are no longer needed and avoiding
 *                  redundant sets.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      portSet points to caller-allocated storage where
 *                  this function should place the port set number
 *                  (handle) of the newly created port set.
 * 
 * \param[in]       isInternal specifies if the portSet created should be
 *                  an internal one. Note that internal portSets cannot be
 *                  modified by public APIs.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FREE_PORT_SET if the maximum number of port
 *                  sets have already been created.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated
 * \return          FM_ERR_INVALID_ARGUMENT if portSet is NULL.
 *
 *****************************************************************************/
fm_status fmCreatePortSetInt(fm_int sw, fm_int *portSet, fm_bool isInternal)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;
    fm_int       tmpPortSet;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %p\n",
                     sw,
                     (void *) portSet);

    TAKE_PORTSET_LOCK(sw);

    if (portSet == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = ReservePortSet(sw, &tmpPortSet, isInternal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    
    portSetEntry = (fm_portSet *) fmAlloc( sizeof(fm_portSet) );

    if (portSetEntry == NULL)
    {
        /* Reset the reserved port set to unused. */
        ReleasePortSet(sw, tmpPortSet);
        
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    FM_CLEAR(*portSetEntry);

    if (fmCreateBitArray(&portSetEntry->associatedPorts,
                         switchPtr->numCardinalPorts) != FM_OK)
    {
        /* Reset the reserved port set to unused. */
        ReleasePortSet(sw, tmpPortSet);

        fmFree(portSetEntry);

        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->CreatePortSetInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->CreatePortSetInt(sw, tmpPortSet, isInternal);

        /* Try to return a clean state by removing the portSet from switches that
         * actually created it (if some). Sorting order of creation and deletion
         * should be the same. */
        if (err != FM_OK)
        {
            switchPtr->DeletePortSetInt(sw, tmpPortSet);

            /* Reset the reserved port set to unused. */
            ReleasePortSet(sw, tmpPortSet);

            fmFree(portSetEntry);
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    err = fmTreeInsert(&switchPtr->portSetInfo.portSetTree,
                       tmpPortSet & FM_PORTSET_MASK,
                       portSetEntry);

    if (err != FM_OK)
    {
        /* Reset the reserved port set to unused. */
        ReleasePortSet(sw, tmpPortSet);

        fmFree(portSetEntry);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* Set the portSet return value */
    *portSet = tmpPortSet;

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmCreatePortSetInt */




/*****************************************************************************/
/** fmDeletePortSet
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number returned by a previous
 *                  call to ''fmCreatePortSet''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_PORTSET_IS_INTERNAL if the portSet is internal.
 *
 *****************************************************************************/
fm_status fmDeletePortSet(fm_int sw, fm_int portSet)
{
    fm_status    err = FM_OK;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT, "sw = %d, portSet = %d\n", sw, portSet);

    if (IS_PORTSET_INTERNAL(portSet))
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_PORT, FM_ERR_PORTSET_IS_INTERNAL);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    err = fmDeletePortSetInt(sw, portSet);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmDeletePortSet */




/*****************************************************************************/
/** fmDeletePortSetInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number returned by a previous
 *                  call to ''fmCreatePortSet''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmDeletePortSetInt(fm_int sw, fm_int portSet)
{

    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT, "sw = %d, portSet = %d\n", sw, portSet);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->DeletePortSetInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->DeletePortSetInt(sw, portSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    err = fmTreeRemoveCertain(&switchPtr->portSetInfo.portSetTree,
                              portSet & FM_PORTSET_MASK,
                              NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    /* Reset the reserved port set to unused. */
    ReleasePortSet(sw, portSet);

    fmDeleteBitArray(&portSetEntry->associatedPorts);
    fmFree(portSetEntry);

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmDeletePortSetInt */




/*****************************************************************************/
/** fmAddPortSetPort
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds a port to a port set created by a previous call to 
 *                  ''fmCreatePortSet''. This function must be called
 *                  multiple times to associate multiple ports to a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       port is the port number to be added to the port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmAddPortSetPort(fm_int sw, fm_int portSet, fm_int port)
{
    fm_status    err = FM_OK;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d, port = %d\n",
                     sw,
                     portSet,
                     port);

    if (IS_PORTSET_INTERNAL(portSet))
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_PORTSET_IS_INTERNAL);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    err = fmAddPortSetPortInt(sw, portSet, port);

    UNPROTECT_SWITCH(sw);
    
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmAddPortSetPort */




/*****************************************************************************/
/** fmAddPortSetPortInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds a port to a port set created by a previous call to 
 *                  ''fmCreatePortSet''. This function must be called
 *                  multiple times to associate multiple ports to a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       port is the port number to be added to the port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmAddPortSetPortInt(fm_int sw, fm_int portSet, fm_int port)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw = %d, portSet = %d, port = %d\n",
                 sw,
                 portSet,
                 port);

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->AddPortSetPortInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->AddPortSetPortInt(sw, portSet, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* Set the port in the array (in cardinal port form) */
    err = fmSetPortInBitArray(sw,
                              &portSetEntry->associatedPorts,
                              GET_PORT_INDEX(sw, port),
                              TRUE);

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmAddPortSetPortInt */




/*****************************************************************************/
/** fmDeletePortSetPort
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Removes a port from a port set created by a previous call
 *                  to ''fmCreatePortSet''. This function must be called
 *                  multiple times to remove multiple ports from a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       port is the port number to be removed from the port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeletePortSetPort(fm_int sw, fm_int portSet, fm_int port)
{
    fm_status    err = FM_OK;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d, port = %d\n",
                     sw,
                     portSet,
                     port);

    if (IS_PORTSET_INTERNAL(portSet))
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_PORT, FM_ERR_PORTSET_IS_INTERNAL);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_CPU);

    err = fmDeletePortSetPortInt(sw, portSet, port);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmDeletePortSetPort */




/*****************************************************************************/
/** fmDeletePortSetPortInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Removes a port from a port set created by a previous call
 *                  to ''fmCreatePortSet''. This function must be called
 *                  multiple times to remove multiple ports from a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       port is the port number to be removed from the port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmDeletePortSetPortInt(fm_int sw, fm_int portSet, fm_int port)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw = %d, portSet = %d, port = %d\n",
                 sw,
                 portSet,
                 port);

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->DeletePortSetPortInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->DeletePortSetPortInt(sw, portSet, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* Set the port in the array (in cardinal port form) */
    err = fmSetPortInBitArray(sw,
                              &portSetEntry->associatedPorts,
                              GET_PORT_INDEX(sw, port),
                              FALSE);

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmDeletePortSetPortInt */




/*****************************************************************************/
/** fmGetPortSetFirst
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the first port set created.
 * 
 * \note            Negative portsets are internal and are read-only.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      portSet points to caller-allocated storage where
 *                  this function should place the port set number
 *                  (handle) of the first port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_PORT_SET if no port set found.
 *
 *****************************************************************************/
fm_status fmGetPortSetFirst(fm_int sw, fm_int *portSet)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_treeIterator it;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %p\n",
                     sw,
                     (void *) portSet);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_PORTSET_LOCK(sw);

    if (portSet == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    fmTreeIterInit(&it, &switchPtr->portSetInfo.portSetTree);

    err = fmTreeIterNext(&it, &nextKey, &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }


    *portSet = (fm_int) nextKey;

ABORT:
    DROP_PORTSET_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetFirst */




/*****************************************************************************/
/** fmGetPortSetNext
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the next port set created.
 * 
 * \note            Negative portsets are internal and are read-only.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentPortSet is the current port set number (handle).
 *
 * \param[out]      nextPortSet points to caller-allocated storage where
 *                  this function should place the port set number
 *                  (handle) of the next port set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_PORT_SET if no port set found.
 * \return          FM_ERR_INVALID_PORT_SET if port set is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmGetPortSetNext(fm_int sw,
                           fm_int currentPortSet,
                           fm_int *nextPortSet)
{
    fm_status       err = FM_OK;
    fm_switch *     switchPtr;
    fm_uint64       nextKey;
    void *          nextValue;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, currentPortSet = %d, nextPortSet = %p\n",
                     sw,
                     currentPortSet,
                     (void *) nextPortSet);

    if (nextPortSet == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeSuccessor(&switchPtr->portSetInfo.portSetTree,
                          currentPortSet & FM_PORTSET_MASK,
                          &nextKey,
                          &nextValue);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_ERR_NO_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    *nextPortSet = (fm_int) nextKey;

ABORT:
    DROP_PORTSET_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetNext */




/*****************************************************************************/
/** fmGetPortSetPortFirst
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the first port associated with a given port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[out]      port points to caller-allocated storage where
 *                  this function should place the first port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_PORT_SET_PORT if no port available.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmGetPortSetPortFirst(fm_int sw, fm_int portSet, fm_int *port)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;
    fm_int       cpi;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d, port = %p\n",
                     sw,
                     portSet,
                     (void *) port);

    if (port == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    err = fmFindPortInBitArray(sw,
                               &portSetEntry->associatedPorts,
                               -1,
                               &cpi,
                               FM_ERR_NO_PORT_SET_PORT);

    if (cpi != -1)
    {
        *port = GET_LOGICAL_PORT(sw, cpi);
    }

ABORT:
    DROP_PORTSET_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetPortFirst */




/*****************************************************************************/
/** fmGetPortSetPortFirstInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the first port associated with a given port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[out]      port points to caller-supplied storage where
 *                  this function should place the first port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_PORT_SET_PORT if no port available.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmGetPortSetPortFirstInt(fm_int sw, fm_int portSet, fm_int *port)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;
    fm_int       cpi;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw = %d, portSet = %d, port = %p\n",
                 sw,
                 portSet,
                 (void *) port);

    if (port == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);

    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_PORT_SET;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    err = fmFindPortInBitArray(sw,
                               &portSetEntry->associatedPorts,
                               -1,
                               &cpi,
                               FM_ERR_NO_PORT_SET_PORT);

    if (cpi != -1)
    {
        *port = GET_LOGICAL_PORT(sw, cpi);
    }

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetPortFirstInt */




/*****************************************************************************/
/** fmGetPortSetPortNext
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the next port associated with a given port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       currentPort is the current port number.
 *
 * \param[out]      nextPort points to caller-allocated storage where
 *                  this function should place the next port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_NO_PORT_SET_PORT if no port available.
 *
 *****************************************************************************/
fm_status fmGetPortSetPortNext(fm_int sw,
                               fm_int portSet,
                               fm_int currentPort,
                               fm_int *nextPort)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;
    fm_int       currentCpi;
    fm_int       nextCpi;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d, currentPort = %d, nextPort = %p\n",
                     sw,
                     portSet,
                     currentPort,
                     (void *) nextPort);

    if (nextPort == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, currentPort, ALLOW_CPU);

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    currentCpi = GET_PORT_INDEX(sw, currentPort);
    
    err = fmFindPortInBitArray(sw,
                               &portSetEntry->associatedPorts,
                               currentPort,
                               &nextCpi,
                               FM_ERR_NO_PORT_SET_PORT);

    if (nextCpi != -1)
    {
        *nextPort = GET_LOGICAL_PORT(sw, nextCpi);
    }

ABORT:
    DROP_PORTSET_LOCK(sw);
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetPortNext */




/*****************************************************************************/
/** fmGetPortSetPortNextInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Finds the next port associated with a given port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       currentPort is the current port number.
 *
 * \param[out]      nextPort points to caller-supplied storage where
 *                  this function should place the next port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 * \return          FM_ERR_NO_PORT_SET_PORT if no port available.
 *
 *****************************************************************************/
fm_status fmGetPortSetPortNextInt(fm_int sw,
                                  fm_int portSet,
                                  fm_int currentPort,
                                  fm_int *nextPort)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;
    fm_int       currentCpi;
    fm_int       nextCpi;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT,
                 "sw = %d, portSet = %d, currentPort = %d, nextPort = %p\n",
                 sw,
                 portSet,
                 currentPort,
                 (void *) nextPort);

    if (nextPort == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_ARGUMENT);
    }

    if (!fmIsValidPort(sw, currentPort, ALLOW_CPU))
    {
        FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_INVALID_PORT);
    }

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);

    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_PORT_SET;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    currentCpi = GET_PORT_INDEX(sw, currentPort);
    
    err = fmFindPortInBitArray(sw,
                               &portSetEntry->associatedPorts,
                               currentPort,
                               &nextCpi,
                               FM_ERR_NO_PORT_SET_PORT);

    if (nextCpi != -1)
    {
        *nextPort = GET_LOGICAL_PORT(sw, nextCpi);
    }

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_PORT, err);

}   /* end fmGetPortSetPortNextInt */




/*****************************************************************************/
/** fmClearPortSet
 * \ingroup portSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Removes all the ports from a port set created by a previous
 *                  call to ''fmCreatePortSet''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmClearPortSet(fm_int sw, fm_int portSet)
{
    fm_status    err = FM_OK;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d\n",
                     sw,
                     portSet);

    if (IS_PORTSET_INTERNAL(portSet))
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_PORT, FM_ERR_PORTSET_IS_INTERNAL);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmClearPortSetInt(sw, portSet);
    
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmClearPortSet */




/*****************************************************************************/
/** fmClearPortSetInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Removes all the ports from a port set created by a previous
 *                  call to ''fmCreatePortSet''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 *
 *****************************************************************************/
fm_status fmClearPortSetInt(fm_int sw, fm_int portSet)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d\n",
                     sw,
                     portSet);

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->ClearPortSetInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->ClearPortSetInt(sw, portSet);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* Clear the ports in the array */
    err = fmClearBitArray(&portSetEntry->associatedPorts);

ABORT:
    DROP_PORTSET_LOCK(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmClearPortSetInt */




/*****************************************************************************/
/** fmSetPortSetPortInt
 * \ingroup intPortSet
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds or removes a port in a port set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portSet is the port set number (handle).
 *
 * \param[in]       port is the port number to be updated in the port set.
 * 
 * \param[in]       state is TRUE if the port is to be added to the set,
 *                  or FALSE if it should be removed from the set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if portSet is not the
 *                  handle of an existing port set.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fmSetPortSetPortInt(fm_int  sw,
                              fm_int  portSet,
                              fm_int  port,
                              fm_bool state)
{
    fm_status    err = FM_OK;
    fm_switch *  switchPtr;
    fm_portSet * portSetEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw = %d, portSet = %d, port = %d\n",
                     sw,
                     portSet,
                     port);

    TAKE_PORTSET_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree,
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetEntry);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_PORT_SET;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    if (switchPtr->SetPortSetPortInt != NULL)
    {
        /* for SWAG */
        err = switchPtr->SetPortSetPortInt(sw, portSet, port, state);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);
    }

    /* Set the port in the array (in cardinal port form) */
    err = fmSetPortInBitArray(sw,
                              &portSetEntry->associatedPorts,
                              GET_PORT_INDEX(sw, port),
                              state);

ABORT:
    DROP_PORTSET_LOCK(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmSetPortSetPortInt */




/*****************************************************************************/
/** fmPortSetToPortList
 * \ingroup intSwitch
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Converts a port set to a list of ports.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       portSet is the portset to operate on.
 *
 * \param[out]      numPorts points to the location to receive the number
 *                  of ports in the list.
 *
 * \param[out]      portList points to a caller-supplied array to receive
 *                  the list of ports.
 *
 * \param[in]       maxPorts is the size of portList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is NULL.
 * \return          FM_ERR_INVALID_PORT_SET if the portset does not exist.
 * \return          FM_ERR_BUFFER_FULL is portList is not large enough
 *                  to hold all the ports in the port set.
 *
 *****************************************************************************/
fm_status fmPortSetToPortList(fm_int    sw, 
                              fm_int    portSet,
                              fm_int *  numPorts, 
                              fm_int *  portList, 
                              fm_int    maxPorts)
{
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_bitArray *bitArrayPtr;
    fm_status    err;
    
    switchPtr = GET_SWITCH_PTR(sw);

    if (numPorts == NULL || portList == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void **) &portSetPtr);

    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_PORT_SET;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    bitArrayPtr = &portSetPtr->associatedPorts;

    err = fmBitArrayToPortList(sw, bitArrayPtr, numPorts, portList, maxPorts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_PORTSET_LOCK(sw);

    return err;

}   /* end fmPortSetToPortList */




/*****************************************************************************/
/** fmPortSetToPortMask
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 * 
 * \desc            Converts a port set from a bit array to an fm_portmask.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       portSet is the portset to operate on.
 *
 * \param[out]      maskPtr points to the location where the resulting mask
 *                  should be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if the portset does not exist.
 *
 *****************************************************************************/
fm_status fmPortSetToPortMask(fm_int       sw, 
                              fm_int       portSet, 
                              fm_portmask *maskPtr)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_bitArray *bitArrayPtr;
    
    switchPtr = GET_SWITCH_PTR(sw);

    if (maskPtr == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetPtr);
    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_PORT_SET;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    bitArrayPtr = &portSetPtr->associatedPorts;

    err = fmBitArrayToPortMask(bitArrayPtr, maskPtr, bitArrayPtr->bitCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_PORTSET_LOCK(sw);

    return err;

}   /* end fmPortSetToPortMask */




/*****************************************************************************/
/** fmPortSetToPhysMask
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 * 
 * \desc            Converts a port set to a physical port mask.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       portSet is the portset to operate on.
 *
 * \param[out]      maskPtr points to the location where the resulting mask
 *                  should be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT_SET if the portset does not exist.
 *
 *****************************************************************************/
fm_status fmPortSetToPhysMask(fm_int       sw, 
                              fm_int       portSet, 
                              fm_portmask *maskPtr)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_bitArray *arrayPtr;
    
    switchPtr = GET_SWITCH_PTR(sw);

    if (maskPtr == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void**) &portSetPtr);
    if (err == FM_ERR_NOT_FOUND)
    {
        err = FM_ERR_INVALID_PORT_SET;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

    arrayPtr = &portSetPtr->associatedPorts;

    err = fmBitArrayLogicalToPhysMask(switchPtr,
                                      arrayPtr,
                                      maskPtr,
                                      arrayPtr->bitCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

ABORT:
    DROP_PORTSET_LOCK(sw);

    return err;

}   /* end fmPortSetToPhysMask */




/*****************************************************************************/
/** fmIsPortSetEmpty
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 * 
 * \desc            Determines whether a port set has any members.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       portSet is the portset to operate on.
 *
 * \return          TRUE if the port set is valid and has any members.
 *
 *****************************************************************************/
fm_bool fmIsPortSetEmpty(fm_int sw, fm_int portSet)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_bitArray *bitArray;
    fm_bool      isEmpty;
    
    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void **) &portSetPtr);
    if (err != FM_OK)
    {
        isEmpty = TRUE;
        goto ABORT;
    }

    bitArray = &portSetPtr->associatedPorts;

    isEmpty = (bitArray == NULL) ||
              (bitArray->wordCount <= 0) ||
              (bitArray->nonZeroBitCount == 0);

ABORT:
    DROP_PORTSET_LOCK(sw);

    return isEmpty;

}   /* end fmIsPortSetEmpty */




/*****************************************************************************/
/** fmGetPortSetCountInt
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 * 
 * \desc            Returns the number of members in the port set.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       portSet is the portset to operate on.
 *
 * \return          The number of ports in the port set.
 *
 *****************************************************************************/
fm_int fmGetPortSetCountInt(fm_int sw, fm_int portSet)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_bitArray *bitArray;
    fm_int       portCount;
    
    switchPtr = GET_SWITCH_PTR(sw);

    portCount = 0;

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void **) &portSetPtr);

    if (err == FM_OK)
    {
        bitArray = &portSetPtr->associatedPorts;

        if (bitArray && (bitArray->wordCount > 0))
        {
            portCount = bitArray->nonZeroBitCount;
        }
    }

    DROP_PORTSET_LOCK(sw);

    return portCount;

}   /* end fmGetPortSetCountInt */




/*****************************************************************************/
/** fmIsPortInPortSetInt
 * \ingroup intPortSet
 * 
 * \chips           FM4000, FM6000, FM10000
 * 
 * \desc            Determines whether a port set has a specified member.
 * 
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       portSet is the portset to operate on.
 * 
 * \param[in]       port is the port number to test for.
 *
 * \return          TRUE if the port set is valid and has any members.
 *
 *****************************************************************************/
fm_bool fmIsPortInPortSetInt(fm_int sw, fm_int portSet, fm_int port)
{
    fm_switch *  switchPtr;
    fm_portSet * portSetPtr;
    fm_status    err;
    fm_bool      bitValue;
    
    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_PORTSET_LOCK(sw);

    err = fmTreeFind(&switchPtr->portSetInfo.portSetTree, 
                     portSet & FM_PORTSET_MASK,
                     (void **) &portSetPtr);

    if (err == FM_OK)
    {
        err = fmGetBitArrayBit(&portSetPtr->associatedPorts, port, &bitValue);
    }

    DROP_PORTSET_LOCK(sw);

    return (err == FM_OK) && bitValue;

}   /* end fmIsPortInPortSetInt */

