/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_lag.c
 * Creation Date:   Aug 13, 2007
 * Description:     Structures and functions for dealing with link aggregation
 *
 * Copyright (c) 2007 - 2014, Intel Corporation
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

/* Macro to round a value to the closest upper multiple of 4 */
#define FM_CEIL_CHUNK_OF_4(val)         ((val + 3) & ~0x3);

/* Most significant bit of an unsigned 32 bit */
#define USGI32_MSB                      31


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
/** PortListToDestMask
 * \ingroup intLag
 *
 * \desc            Converts a port list into a destination mask (fm_portMask).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portList is a pointer to an array containing the port list.
 * 
 * \param[in]       numPorts is the number of ports contained in the port list.
 * 
 * \param[out]      destMask is a pointer to the destination mask structure
 *                  to be filled. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
 static fm_status PortListToDestMask(fm_int       sw, 
                                     fm_int *     portList,
                                     fm_int       numPorts,
                                     fm_portmask *destMask)
 {
     fm_status   err = FM_OK;
     fm_int      i;
     fm_portmask activeDestMask;

     FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, numPorts = %d\n",
                 sw,
                 numPorts);

     FM_PORTMASK_DISABLE_ALL(destMask);

     for (i = 0; i < numPorts; i++)
     {
         if (fmIsRemotePort(sw, portList[i]))
         {
             err = fmGetRemotePortDestMask(sw,
                                           portList[i],
                                           NULL,
                                           &activeDestMask);
             FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

             /* Enable ports from active mask in the destination port mask */
             FM_OR_PORTMASKS(destMask, destMask, &activeDestMask);
         }
         else
         {
             FM_PORTMASK_ENABLE_PORT(destMask, portList[i]);
         }
     }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

 }   /* end PortListToDestMask */




/*****************************************************************************/
/** WriteCanonCamEntry
 * \ingroup intLag
 *
 * \desc            Write canonical CAM entry
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the CAM index.
 *
 * \param[in]       entry is the pointer to the fm10000CanonCamEntry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteCanonCamEntry(fm_int                sw,
                                    fm_int                index,
                                    fm10000CanonCamEntry *entry)
{
    fm_uint32 rv = 0;
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);

    FM_SET_FIELD(rv,
                 FM10000_CANONICAL_GLORT_CAM,
                 LagGlort,
                 entry->glort);
    FM_SET_FIELD(rv,
                 FM10000_CANONICAL_GLORT_CAM,
                 MaskSize,
                 entry->maskSize);
    FM_SET_FIELD(rv,
                 FM10000_CANONICAL_GLORT_CAM,
                 PortFieldSize,
                 entry->fieldSize);

    return switchPtr->WriteUINT32(sw, FM10000_CANONICAL_GLORT_CAM(index), rv);

}   /* end WriteCanonCamEntry */




/*****************************************************************************/
/** WritePortLagCfg
 * \ingroup intLag
 *
 * \desc            Set the LAG_CFG register for the given port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to set.
 *
 * \param[in]       index is the index of the port in the LAG. Can be
 *                  set to -1 if it should not be updated.
 *
 * \param[in]       lagSize is the number of ports in the LAG. Can be set to
 *                  -1 if it should not be updated.
 *
 * \param[in]       hashRotation Selects hash rotation A (0) or rotation B (1).
 *
 * \param[in]       inLag indicates if the port is active in the LAG.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status WritePortLagCfg(fm_int  sw, 
                                 fm_int  port,
                                 fm_int  index,
                                 fm_int  lagSize,
                                 fm_uint hashRotation,
                                 fm_bool inLag)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;
    fm_int     physPort;
    fm_uint32  rv;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d, port=%d, index=%d "
                 "lagSize=%d hashRotation=%d inLag=%s\n", 
                 sw,
                 port,
                 index,
                 lagSize,
                 hashRotation,
                 (inLag) ? "TRUE" : "FALSE");

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmMapLogicalPortToPhysical(switchPtr, port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    rv = 0;
    FM_SET_FIELD(rv, FM10000_LAG_CFG, LagSize, lagSize);
    FM_SET_FIELD(rv, FM10000_LAG_CFG, Index, index);
    FM_SET_BIT(rv, FM10000_LAG_CFG, HashRotation, hashRotation);
    FM_SET_BIT(rv, FM10000_LAG_CFG, InLAG, inLag);

    err = switchPtr->WriteUINT32(sw, FM10000_LAG_CFG(physPort), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end WritePortLagCfg */




/*****************************************************************************/
/** UpdatePortMaskForLag
 * \ingroup intLag
 *
 * \desc            Updates the port mask to prevent flooding back to the link
 *                  agg group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdatePortMaskForLag(fm_int sw, fm_int lagIndex)
{
    fm_int        portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int        numPorts;
    fm_int        port;
    fm_portmask   mask;
    fm_status     err;
    fm_int        i;
    fm10000_port *portExt;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    /* Note that all these masks are the logical masks
     * and that FM_PORT_MASK deals in logical masks */
    err = fmGetLAGMemberPorts(sw,
                              lagIndex,
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    FM_PORTMASK_DISABLE_ALL(&mask);

    /* first compute the mask */
    for (i = 0; i < numPorts ; i++)
    {
        port = portList[i];

        /***************************************************
         * Remote ports should not be filtered out of the
         * port mask, otherwise flood traffic ingressing
         * on LAG links won't be able to get to the remote
         * ports.
         **************************************************/
        if (fmIsCardinalPort(sw, port))
        {
            FM_PORTMASK_ENABLE_PORT(&mask, port);
        }
    }

    /* Inverse portmask so that it can be anded with the desired portmask */
    FM_INVERSE_PORTMASKS(&mask, &mask);

    /* now apply the mask */
    for (i = 0 ; i < numPorts ; i++)
    {
        port = portList[i];

        if (!fmIsCardinalPort(sw, port))
        {
            /* Do we need to do anything for remote ports */
            continue;
        }

        portExt = GET_PORT_EXT(sw, port);

        /*************************************************** 
         * The port mask must be configured in addition of 
         * the FM10000_FH_LOOPBACK_SUPPRESS register because 
         * the FM10000 Loopback suppression is applied after 
         * the trigger. In other words, if the portmask of 
         * the ingress LAG member port is not set as above, 
         * the destMask seen by the triggers might include 
         * the other LAG members. This has undesirable 
         * side effect of firing triggers configured for 
         * mirroring purpose, and resulting in mirror 
         * frames being sent even if the original frame is 
         * loopback-suppressed later in the pipeline. 
         **************************************************/
        portExt->internalPortMask = mask;

        err = fm10000UpdatePortMask(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdatePortMaskForLag */




/*****************************************************************************/
/** UpdatePortMaskForLagRemove
 * \ingroup intLag
 *
 * \desc            Updates the port mask to prevent flooding back to the link
 *                  agg group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[in]       port is the port number
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdatePortMaskForLagRemove(fm_int sw,
                                            fm_int lagIndex,
                                            fm_int port)
{
    fm_status     err;
    fm_switch  *  switchPtr;
    fm10000_port *portExt;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Update portmasks for remaining member ports */
    err = UpdatePortMaskForLag(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    if (!fmIsCardinalPort(sw, port))
    {
        /* The port is remote and does not have a portmask */
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);
    }

    /***************************************************
     * Update egress port mask for the port being removed.
     **************************************************/
    portExt = GET_PORT_EXT(sw, port);

    /*************************************************** 
     * The port mask must be configured in addition of 
     * the FM10000_FH_LOOPBACK_SUPPRESS register because 
     * the FM10000 Loopback suppression is applied after 
     * the trigger. In other words, if the portmask of 
     * the ingress LAG member port is not set as above, 
     * the destMask seen by the triggers might include 
     * the other LAG members. This has undesirable 
     * side effect of firing triggers configured for 
     * mirroring purpose, and resulting in mirror 
     * frames being sent even if the original frame is 
     * loopback-suppressed later in the pipeline. 
     **************************************************/
    FM_PORTMASK_ENABLE_ALL(&portExt->internalPortMask, 
                           switchPtr->numCardinalPorts);

    err = fm10000UpdatePortMask(sw, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdatePortMaskForLagRemove */




/*****************************************************************************/
/** UpdatePortCfgISL
 * \ingroup intLag
 *
 * \desc            Updates the PORT_CFG_ISL register of each lag member port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the lag on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdatePortCfgISL(fm_int sw, fm_int lagIndex)
{
    fm_status    err;
    fm_switch *  switchPtr;
    fm_int       portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int       numPorts;
    fm_int       lagLogicalPort;
    fm_int       memberPort;
    fm_int       memberIndex;
    fm_int       physPort;
    fm_int       portGlort;
    fm_lag *     lagPtr;
    fm10000_lag *lagExt;
    fm_bool      regLockTaken;
    fm_uint32    rv;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    regLockTaken = FALSE;

    /**************************************************
     * We need to update other member ports
     * Since the index and glort might be changed
     * for each member port.
     **************************************************/
    err = fmGetLAGMemberPorts(sw, 
                              lagIndex, 
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_LAG_PTR(sw, lagIndex);
    lagExt    = GET_LAG_EXT(sw, lagIndex);

    /* The following steps involve read/modify/write operations */
    FM_FLAG_TAKE_REG_LOCK(sw);

    while (numPorts)
    {
        numPorts--;
        memberPort = portList[numPorts];

        /* Update members */
        err = fmGetLAGMemberIndex(sw, memberPort, &memberIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        lagLogicalPort = lagPtr->logicalPort + memberIndex;

        /* Change the logical port number to point to member port,
         * so fm10000GetLogicalPort can return correct member port
         * given a member glort. */
        switchPtr->portTable[lagLogicalPort]->portNumber = memberPort;

        if ( fmIsCardinalPort(sw, memberPort) ) 
        {
            err = fmMapLogicalPortToPhysical(switchPtr, memberPort, &physPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            err = switchPtr->ReadUINT32(sw, FM10000_PORT_CFG_ISL(physPort), &rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            portGlort = lagExt->lagGlort + memberIndex;

            FM_SET_FIELD(rv, FM10000_PORT_CFG_ISL, srcGlort, portGlort);

            err = switchPtr->WriteUINT32(sw, FM10000_PORT_CFG_ISL(physPort), rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
            
            /* Indicate to the platform code that we are changing state */
            fmPlatformAddGlortToPortMapping(sw, portGlort, physPort);
        }
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdatePortCfgISL */




/*****************************************************************************/
/** UpdateLagCfg
 * \ingroup intLag
 *
 * \desc            Updates the LAG_CFG register of each lag member port.
 * 
 * \note            If traffic is flowing to the LAG, this function will
 *                  disrupt traffic. Some frames may be lost or duplicated. 
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the lag on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateLagCfg(fm_int sw, fm_int lagIndex)
{
    fm_status err;
    fm_int    portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int    numPorts;
    fm_int    i;
    fm_lag *  lagPtr;      

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    /* Get the active member port list */
    err = fmGetLAGMemberPorts(sw, 
                              lagIndex, 
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* Update LAG_CFG for all members */
    for (i = 0; i < numPorts; i++)
    {
        /* Only update local ports */
        if (fmIsCardinalPort(sw, portList[i]))
        {
            err = WritePortLagCfg(sw, 
                                  portList[i], 
                                  i, 
                                  numPorts,
                                  lagPtr->hashRotation,
                                  lagPtr->filteringEnabled);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }

ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdateLagCfg */




/*****************************************************************************/
/** UpdateGlortDestTableFiltering
 * \ingroup intLag
 *
 * \desc            Updates the glort destination table using the
 *                  LAG filtering method only.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on which to operate
 * 
 * \param[in]       portList is a pointer to an array containing the list
 *                  of ports on which LAG filtering will be done.
 * 
 * \param[in]       numPorts is the number of ports contained in the port list.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateGlortDestTableFiltering(fm_int  sw, 
                                               fm_int  lagIndex,
                                               fm_int *portList,
                                               fm_int  numPorts)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_int             lagLogicalPort;
    fm_port *          lagPortPtr;
    fm_portmask        destMask;
    fm_uint32          destIndex;
    fm_glortDestEntry *destEntry;
    fm_glortCamEntry * camEntry;
    fm_int             i;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    switchPtr      = GET_SWITCH_PTR(sw);
    lagLogicalPort = fmGetLagLogicalPort(sw, lagIndex);
    lagPortPtr     = GET_PORT_PTR(sw, lagLogicalPort);

    destIndex = lagPortPtr->destEntry->destIndex;
    destEntry = &switchPtr->logicalPortInfo.destEntries[destIndex];
    camEntry  = lagPortPtr->camEntry;

    err = PortListToDestMask(sw, portList, numPorts, &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* No pruning required since all ports are on one switch, just update
     * the glort destination mask */
    err = fm10000SetGlortDestMask(sw, destEntry, &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* Restore back to LAG filtering */
    if ( (camEntry->destCount != 1) || 
         (camEntry->destIndex != destIndex) )
    {
        camEntry->destCount = 1;
        camEntry->destIndex = destIndex;

        err = fm10000WriteGlortCamEntry(sw,
                                        camEntry,
                                        FM_UPDATE_RAM_ONLY);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    if (lagPortPtr->numDestEntries > 1)
    {
        /**************************************************
         * Free extra allocated dest entries, if any
         **************************************************/

        for (i = 1 ; i < lagPortPtr->numDestEntries ; i++)
        {
            err = fm10000FreeDestEntry(sw, 
                                       &switchPtr->logicalPortInfo.
                                           destEntries[camEntry->destIndex+i]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        lagPortPtr->numDestEntries = 1;
    }

ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdateGlortDestTableFiltering */




/*****************************************************************************/
/** UpdateGlortDestTablePruning
 * \ingroup intLag
 *
 * \desc            Updates the glort destination table using the
 *                  LAG filtering/pruning methods.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG on which to operate
 * 
 * \param[in]       portList points to an array containing the list
 *                  of ports on which LAG pruning/filtering will be done.
 * 
 * \param[in]       numPorts is the number of ports contained in the port list.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateGlortDestTablePruning(fm_int  sw, 
                                             fm_int  lagIndex,
                                             fm_int *portList,
                                             fm_int  numPorts)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_lag *           lagPtr;
    fm_int             lagLogicalPort;
    fm_port *          lagPortPtr;
    fm_int             destIndex;
    fm_glortCamEntry * camEntry;
    fm_uint32          numEntriesDesired;
    fm_glortDestEntry *newDestEntries[FM_MAX_NUM_LAG_MEMBERS];
    fm_int             index;
    fm_int             port;
    fm_portmask        activeDestMask;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    switchPtr      = GET_SWITCH_PTR(sw);
    lagPtr         = GET_LAG_PTR(sw, lagIndex);
    lagLogicalPort = fmGetLagLogicalPort(sw, lagIndex);
    lagPortPtr     = GET_PORT_PTR(sw, lagLogicalPort);
    camEntry       = lagPortPtr->camEntry;

    /* Allocate in chunks of 4-entry to minimize fragments in dest table */
    numEntriesDesired = FM_CEIL_CHUNK_OF_4(numPorts);

    /********************************************** 
     * To be as least disruptive as possible,
     * 1. allocate a new block of dest entries 
     * 2. fill in the new block of dest entries 
     * 3. update the glort-ram pointer to the new 
     *    block of dest entries 
     * 4. free the old block of dest entries 
     **********************************************/ 

    /********************************************** 
     * 1. allocate a new block of dest entries 
     **********************************************/
    err = fmAllocDestEntries(sw,
                             numEntriesDesired,
                             camEntry,
                             newDestEntries,
                             FM_PORT_TYPE_LAG);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /********************************************** 
     * 2. fill in the new block of dest entries 
     **********************************************/
    for (index = 0 ; index < numPorts ; index++)
    {
        port = portList[index];
        if (fmIsRemotePort(sw, port))
        {
            err = fmGetRemotePortDestMask(sw, port, NULL, &activeDestMask);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
        else
        {
            err = fmAssignPortToPortMask(sw, &activeDestMask, port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        err = fm10000SetGlortDestMask(sw,
                                      newDestEntries[index],
                                      &activeDestMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /********************************************** 
     * 3. update the glort-ram pointer to the new 
     *    block of dest entries 
     **********************************************/

    /* Validate that the information stored in the lag port is the same
     * as in the camEntry. This information will be used in step 4) to
     * free old entries. */
    FM_LOG_ASSERT(FM_LOG_CAT_LAG, 
                  (lagPortPtr->destEntry->destIndex == camEntry->destIndex),
                  "Expected destIndex to match (%d) != (%d)\n",
                  lagPortPtr->destEntry->destIndex,
                  camEntry->destIndex);

    camEntry->destCount = numPorts;
    camEntry->destIndex = newDestEntries[0]->destIndex;

    /* Under pruning the cam is doing the hash */
    camEntry->hashRotation = lagPtr->hashRotation;

    err = fm10000WriteGlortCamEntry(sw,
                                    camEntry,
                                    FM_UPDATE_RAM_ONLY);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /********************************************** 
     * 4. free the old block of dest entries.
     **********************************************/
    destIndex = lagPortPtr->destEntry->destIndex;
    for (index = 0 ; index < lagPortPtr->numDestEntries ; index++)
    {
        err = fm10000FreeDestEntry(sw,
                                   &switchPtr->logicalPortInfo.
                                       destEntries[destIndex++]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    lagPortPtr->destEntry      = newDestEntries[0];
    lagPortPtr->numDestEntries = numEntriesDesired;
    
ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdateGlortDestTablePruning */




/*****************************************************************************/
/** UpdateGlortDestTable
 * \ingroup intLag
 *
 * \desc            Updates the glort destination table for the LAG port.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the lag on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateGlortDestTable(fm_int sw, fm_int lagIndex)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_int             portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int             numPorts;
    fm_int             i;
    fm_int             numRemotePorts; 
    fm_bool            doPruning;
    fm_int             lastInternalPort;
    fm_int             internalPort;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    
    /* Get the active member port list */
    err = fmGetLAGMemberPorts(sw, 
                              lagIndex, 
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    numRemotePorts = 0;
    doPruning = FALSE;

    /* Count the number of remote ports */
    for (i = 0; i < numPorts; i++)
    {
        if (fmIsRemotePort(sw, portList[i]))
        {
            numRemotePorts++;
        }
    }

    /* Check if pruning is needed. Pruning is required in multi-chip systems
     * when the LAG contains a remote port (which is accessible via an
     * internal port) and one of the following conditions:
     *  - The LAG contains one or more cardinal ports on the local switch.
     *  - The LAG contains another remote port (accessed through
     *    another internal port) */
    if ( (numRemotePorts > 0) &&
         (!switchPtr->lagInfoTable.pruningDisabled) )
    {
        /* Do pruning if not all members on one switch */
        if (numRemotePorts != numPorts)
        {
            doPruning = TRUE;
        }
        else
        {
            /* check if all ports map to the same internal port */
            lastInternalPort = -1;
            for (i = 0 ; i < numPorts ; i++)
            {
                err = fmGetInternalPortFromRemotePort(sw,
                                                      portList[i],
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

    if (doPruning)
    {
        err = UpdateGlortDestTablePruning(sw, lagIndex, portList, numPorts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = UpdateGlortDestTableFiltering(sw, lagIndex, portList, numPorts);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdateGlortDestTable */




/*****************************************************************************/
/** UpdateGlortDestTableAllLags
 * \ingroup intLag
 *
 * \desc            Updates the glort destination table for the all lags.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateGlortDestTableAllLags(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_int     lagList[FM_MAX_NUM_LAGS];
    fm_int     numLags;
    fm_int     lagIndex;
    fm_int     i;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    numLags = 0;

    for (lagIndex = 0 ; lagIndex < FM_MAX_NUM_LAGS ; lagIndex++)
    {
        if (switchPtr->lagInfoTable.lag[lagIndex])
        {
            lagList[numLags] = lagIndex;
            numLags++;
        }
    }

    for (i = 0 ; i < numLags ; i++)
    {
        lagIndex = lagList[i];

        /**************************************************
         * Update hardware with new active ports
         **************************************************/
        err = UpdateGlortDestTable(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end UpdateGlortDestTableAllLags */




/*****************************************************************************/
/** DeletePortRegisters
 * \ingroup intLag
 *
 * \desc            Updates the configuration registers of a port when it
 *                  is removed from a LAG.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the number of the port to be updated.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status DeletePortRegisters(fm_int sw, fm_int port)
{
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, port = %d\n", sw, port);

    err = WritePortLagCfg(sw, port, 0, 0, 0, 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    err = fm10000ResetPortSettings(sw, port);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end DeletePortRegisters */




/*****************************************************************************/
/** fm10000DeleteLagCallback
 * \ingroup intLag
 *
 * \desc            Called by the MA table maintenance code to inform the 
 *                  LAG code that the MA table purge initiated by
 *                  ''fm10000DeleteLagFromSwitch'' has completed, and the 
 *                  remaining LAG resources can be freed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       context is an opaque pointer to the LAG extension.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status fm10000DeleteLagCallback(fm_int sw, void *context)
{
    fm_lag *  lagPtr = (fm_lag *) context;
    fm_status err;
    fm_status err2;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                 "sw = %d, logicalPort = %d\n", 
                 sw, 
                 lagPtr->logicalPort);

    if (lagPtr->extension == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);
    }

    err = fm10000FreeLogicalPort(sw, lagPtr->logicalPort);
    
    if (err != FM_OK)
    {
        /* An unexpected error here */
        FM_LOG_ERROR(FM_LOG_CAT_LAG,
                     "Failed [%s] to free lag port.\n",fmErrorMsg(err));
    }

    fmFree(lagPtr->extension);

    lagPtr->extension = NULL;

    err2 = fmDeleteLagCallback(sw, lagPtr);

    if (err == FM_OK)
    {
        err = err2;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000DeleteLagCallback */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000LagGroupInit
 * \ingroup intLag
 *
 * \desc            Perform initialization for LAG group subsystem,
 *                  called at switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000LagGroupInit(fm_int sw)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint32       rangeBase;
    fm_uint32       rangeMax;
    fm_int          i;
    fm_glortRange * glorts;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    glorts    = &switchPtr->glortRange;

    /* In FM10000, remote ports, local ports and also CPU port can be 
     * added to LAG. */
    switchPtr->lagInfoTable.allowedPortTypes = ALLOW_REMOTE | ALLOW_CPU;

    rangeBase = glorts->glortBase;
    rangeMax  = rangeBase | glorts->glortMask;

    /* Reset canonCamEntry to unused */
    for (i = 0 ; i < FM10000_CANONICAL_GLORT_CAM_ENTRIES ; i++)
    {
        switchExt->canonCamEntry[i].used = 0;
    }

    /* Only allocate when there is adequate glort range
     * If not, the user is doing stacking and is responsible
     * for calling allocate lag group and it is done there */
    if (glorts->lagCount > 0)
    {
        /* Create the LAG canonical CAM entries using default range */
        err = fm10000CreateCanonicalCamEntries(sw,
                                               glorts->lagBaseGlort,
                                               glorts->lagCount,
                                               FM10000_LAG_MASK_SIZE);
    }   
    else
    {
        err = FM_OK;
    }


    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000LagGroupInit */




/*****************************************************************************/
/** fm10000CreateCanonicalCamEntries
 * \ingroup intLag
 *
 * \desc            Create neccessary canonical CAM entries given a glort range.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       glort is the start glort value.
 *
 * \param[in]       glortSize is the size of the glort range.
 *
 * \param[in]       clearSize is the field size to clear in the canonical CAM.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CreateCanonicalCamEntries(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_uint   glortSize,
                                           fm_int    clearSize)
{
    fm_status             err = FM_OK;
    fm10000_switch *      switchExt;
    fm10000CanonCamEntry *canonCamEntry;
    fm_uint32             startGlort;
    fm_uint               startSize;
    fm_int                loopCnt;
    fm_int                numCamNeeded;
    fm_int                numCamFree;
    fm_int                camIdx;
    fm_uint32             glortStart[FM10000_CANONICAL_GLORT_CAM_ENTRIES];
    fm_uint32             glortMask[FM10000_CANONICAL_GLORT_CAM_ENTRIES];
    fm_int                camIndex[FM10000_CANONICAL_GLORT_CAM_ENTRIES];
    fm_int                i;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                "sw = %d, glort = 0x%x, glortSize = %d, fieldSize = %d\n",
                sw,
                glort,
                glortSize,
                clearSize);

    /* Find out how many entries needed for this range */
    numCamNeeded = 0;
    startGlort   = glort;
    startSize    = glortSize;
    loopCnt      = 0;

    while (startGlort < (glort + glortSize))
    {
        for (i = USGI32_MSB ; i >= 0 ; i--)
        {
            fm_uint32 size;

            size = (1 << i);
        
            if (size > startSize)
            {
                continue;
            }

            if ( (startGlort == (startGlort & ~(size-1))) )
            {
                if (numCamNeeded >= FM10000_CANONICAL_GLORT_CAM_ENTRIES)
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
                }

                glortStart[numCamNeeded] = startGlort;
                startGlort += (1 << i);
                startSize -= (1 << i);
                glortMask[numCamNeeded] = i;
                numCamNeeded++;
                break;
            }
        }

        loopCnt++;
        if (loopCnt > 16)
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }

    if (!numCamNeeded)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Some sanity check for the logic above */
    if (glort != (glortStart[0] & ~((1 << glortMask[0]) - 1) ))
    {
        FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                     "Inconsistent first canonical cam entry (0x%04x/0x%04x)\n",
                     glortStart[0], 
                     glortMask[0]);

        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    for (i = 1 ; i < numCamNeeded ; i++)
    {
        if (glortStart[i] != (glortStart[i] & ~((1 << glortMask[i]) - 1) ))
        {
            FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                         "Inconsistent canonical cam entry (0x%04x/0x%04x) "
                         "at index %d\n",
                         glortStart[i], 
                         glortMask[i], 
                         i);

            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }

        if (glortStart[i] != glortStart[i-1] + (( 1 << glortMask[i-1])))
        {
            FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                         "Non-continous canonical cam entry#%d (0x%04x/0x%04x) "
                         "entry#%d (0x%04x/0x%04x)\n",
                         i-1, 
                         glortStart[i-1], 
                         glortMask[i-1], 
                         i, 
                         glortStart[i], 
                         glortMask[i]);

            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }

    if (glortStart[numCamNeeded-1] + ((1 << glortMask[numCamNeeded-1])) != 
                                                            (glort + glortSize))
    {
        /* Make sure the last entry cover the end range properly */ 
        FM_LOG_DEBUG(FM_LOG_CAT_LAG,
                     "Last cam entry (0x%04x/0x%04x) does not "
                     "match end size 0x%x\n",
                     glortStart[numCamNeeded-1], 
                     glortMask[numCamNeeded-1], 
                     glort + glortSize);

        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    switchExt = GET_SWITCH_EXT(sw);
    canonCamEntry = switchExt->canonCamEntry;

    /* Find if there is enough free space */
    numCamFree = 0;
    for (i = 0 ; i < FM10000_CANONICAL_GLORT_CAM_ENTRIES ; i++)
    {
        if (!canonCamEntry[i].used)
        {
            camIndex[numCamFree] = i;
            numCamFree++;
            if (numCamFree == numCamNeeded)
            {
                break;
            }
        }
    }

    if (i >= FM10000_CANONICAL_GLORT_CAM_ENTRIES)
    {
        err = FM_ERR_NO_FREE_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    for (i = 0 ; i < numCamNeeded ; i++)
    {
        camIdx = camIndex[i];
        canonCamEntry[camIdx].used = TRUE;
        canonCamEntry[camIdx].glort = glortStart[i];
        canonCamEntry[camIdx].maskSize = glortMask[i];
        canonCamEntry[camIdx].fieldSize = clearSize;

        err = WriteCanonCamEntry(sw, camIdx, &canonCamEntry[camIdx]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000CreateCanonicalCamEntries */




/*****************************************************************************/
/** fm10000DeleteCanonicalCamEntries
 * \ingroup intLag
 *
 * \desc            Delete neccessary canonical CAM entries that are in range
 *                  of the specified glort range.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       glort is the start glort value.
 *
 * \param[in]       glortSize is the size of the glort range.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteCanonicalCamEntries(fm_int    sw,
                                          fm_uint32 glort,
                                          fm_uint   glortSize)
{
    fm_status             err = FM_OK;
    fm10000_switch *      switchExt;
    fm10000CanonCamEntry *canonCamEntry;
    fm_int                camIdx;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG, 
                "sw = %d, glort = 0x%x, glortSize = %d\n",
                sw,
                glort,
                glortSize);

    switchExt = GET_SWITCH_EXT(sw);
    canonCamEntry = switchExt->canonCamEntry;

    for (camIdx = 0 ; camIdx < FM10000_CANONICAL_GLORT_CAM_ENTRIES ; camIdx++)
    {
        /* Delete the entry if in the range */
        if (canonCamEntry[camIdx].used &&
            ( (canonCamEntry[camIdx].glort >= glort) && 
              (canonCamEntry[camIdx].glort < (glort+glortSize)) ) )
        {
            canonCamEntry[camIdx].used = FALSE;
            canonCamEntry[camIdx].glort = 0;
            canonCamEntry[camIdx].maskSize = 0;
            canonCamEntry[camIdx].fieldSize = 0;

            err = WriteCanonCamEntry(sw, camIdx, &canonCamEntry[camIdx]);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_LAG, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000DeleteCanonicalCamEntries */




/*****************************************************************************/
/** fm10000InformLAGPortDown
 * \ingroup intLag
 *
 * \desc            Called to inform the LAG code that the state of a port is
 *                  no longer "up".  If the port was an active member of
 *                  any LAG, it will be set to inactive.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000InformLAGPortDown(fm_int sw, fm_int port)
{
    fm_status err = FM_OK;
    fm_int    lagIndex;
    fm_lag *  lagPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, port = %d\n", sw, port);

    lagIndex = fmGetPortLagIndex(sw, port);

    if (lagIndex >= 0 && lagIndex < FM_MAX_NUM_LAGS)
    {
        lagPtr = GET_LAG_PTR(sw, lagIndex);
    }
    else
    {
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* The port should have been deactivated by the caller (fmInformLAGPortDown).
     * We can now proceed with register updates */

    if (fmIsInternalPort(sw, port))
    {
        /* We need to update the glort table of all LAGs that make use
         * of the internal port. */
        err = UpdateGlortDestTableAllLags(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = UpdateGlortDestTable(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* The LAG_CFG register always needs to be updated, even for remote ports
     * as this affects the lag_size. */
    err = UpdateLagCfg(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    
    if (fmCountActiveLagMembers(sw, lagIndex) == 0)
    {
        /* this was the last active port, we need to delete any MA table
         * entries for the LAG */
        err = fmFlushPortAddrInternal(sw, lagPtr->logicalPort, NULL, NULL);
    }

ABORT:

    /* Also update the redirect CPU port if it is a LAG */
    fm10000InformRedirectCPUPortLinkChange(sw, port, FM_PORT_STATUS_LINK_DOWN);

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000InformLAGPortDown */




/*****************************************************************************/
/** fm10000InformLAGPortUp
 * \ingroup intLag
 *
 * \desc            Called to inform the LAG code that the state of a port is
 *                  now "up".  If the port was an inactive member of
 *                  any LAG, it will be reactivated.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000InformLAGPortUp(fm_int sw, fm_int port)
{
    fm_status err = FM_OK;
    fm_int    lagIndex;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG, "sw = %d, port = %d\n", sw, port);

    lagIndex = fmGetPortLagIndex(sw, port);

    /* The port should have been activated by the caller (fmInformLAGPortUp).
     * We can now proceed with register updates */

    /* The LAG_CFG register always needs to be updated, even for remote ports
    * as this affects the lag_size. */
    err = UpdateLagCfg(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    
    if (fmIsInternalPort(sw, port))
    {
        /* We need to update the glort table of all LAGs that make use
         * of the internal port. */
        err = UpdateGlortDestTableAllLags(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = UpdateGlortDestTable(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:

    /* Also update the redirect CPU port if it is a LAG */
    fm10000InformRedirectCPUPortLinkChange(sw, port, FM_PORT_STATUS_LINK_UP);
    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000InformLAGPortUp */




/*****************************************************************************/
/** fm10000AllocateLAGs
 * \ingroup intLag
 *
 * \desc            Allocate LAGs given a glort range. The function
 *                  returns the base LAG handle and the number of handles
 *                  created. The caller can then emumerate these handles up to
 *                  the number of handles allocated. These handles will have 
 *                  the same CAM resources across multiple switches, given the
 *                  input glort information is the same.
 *
 * \note            The return base handle might not be the same on different
 *                  switches. However the cam resources for 
 *                  (baseLAGHandle + n) will be consistent on different 
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
 * \param[out]      baseLagHandle points to caller-allocated storage 
 *                  where this function should place the base LAG group
 *                  handle of the newly allocated stacking LAGs.
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
 * \return          FM_ERR_INVALID_ARGUMENT input parameters fail checking.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  required in the given glort range is being used.
 * \return          FM_ERR_NO_LAG_RESOURCES if no more resources are available
 *
 *****************************************************************************/
fm_status fm10000AllocateLAGs(fm_int  sw,
                             fm_uint startGlort,
                             fm_uint glortSize,
                             fm_int *baseLagHandle,
                             fm_int *numLags,
                             fm_int *step)
{
    fm_status      err;
    fm_switch *    switchPtr;
    fm_glortRange *glorts;
    fm_uint        maxStackLags;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, startGlort = %u, glortSize = %u, "
                 "baseLagHandle = %p, numLags = %p, step = %p\n",
                 sw,
                 startGlort,
                 glortSize,
                 (void *) baseLagHandle,
                 (void *) numLags,
                 (void *) step);

    switchPtr = GET_SWITCH_PTR(sw);
    glorts    = &switchPtr->glortRange;

    maxStackLags = FM_MAX_NUM_LAGS - (glorts->lagCount / FM10000_GLORTS_PER_LAG);

    if ( (glortSize / FM10000_GLORTS_PER_LAG) > maxStackLags)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    err = fmAllocateLagHandles(sw,
                               startGlort,
                               glortSize,
                               FM10000_GLORTS_PER_LAG,
                               FM10000_LAG_MASK_SIZE,
                               baseLagHandle,
                               numLags,
                               step);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000AllocateLAGs */




/*****************************************************************************/
/** fm10000FreeStackLAGs
 * \ingroup intLag
 *
 * \desc            Free allocated LAGs previously created with
 *                  ''fm10000AllocateLAGs''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLagHandle is the handle previously created with
 *                  ''fm10000AllocateLAGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fm10000FreeStackLAGs(fm_int sw, fm_int baseLagHandle)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, baseLagHandle = %d\n",
                 sw,
                 baseLagHandle);

    err = fmFreeLagHandles(sw, baseLagHandle);

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000FreeStackLAGs */




/*****************************************************************************/
/** fm10000CreateLagOnSwitch
 * \ingroup intLag
 *
 * \desc            Create a link aggregation group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG to create.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CreateLagOnSwitch(fm_int sw, fm_int lagIndex)
{
    fm_switch *  switchPtr;
    fm_lag *     lagPtr;
    fm10000_lag *lagExt = NULL;
    fm_int       firstLogicalPort;
    fm_int       useHandle;
    fm_status    err;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);
    if (lagPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_ARGUMENT);
    }

    lagExt = (fm10000_lag*) fmAlloc( sizeof(fm10000_lag) );
    if (lagExt == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    lagPtr->extension = lagExt;

    lagPtr->hashRotation = FM_HASH_ROTATION_A;

    /* suggestion to fm10000AllocLogicalPort */
    firstLogicalPort = FM_LOGICAL_PORT_ANY;

    /* This is for stacking, the lag handle passed thru here */
    useHandle = lagPtr->logicalPort;

    err = fm10000AllocLogicalPort(sw,
                                 FM_PORT_TYPE_LAG,
                                 FM10000_GLORTS_PER_LAG,
                                 &firstLogicalPort,
                                 useHandle);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    lagPtr->logicalPort = firstLogicalPort;

    lagExt->lagGlort = FM_GET_GLORT_NUMBER(switchPtr, lagPtr->logicalPort);

    /*************************************************************************
     * NOTE:
     * fm10000InitLAGPortAttributes is not needed.When a new logical port is
     * created for LAG it is initialized default port attributes.
     *************************************************************************/
#if 0
    if (switchPtr->perLagMgmt)
    {
        /* Set the LAG port attributes to their default values */
        err = fm10000InitLAGPortAttributes(sw, lagPtr->logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
#endif

ABORT:
    if (err && (lagExt != NULL))
    {
        fmFree(lagExt);
        lagPtr->extension = NULL;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000CreateLagOnSwitch */




/*****************************************************************************/
/** fm10000DeleteLagFromSwitch
 * \ingroup intLag
 *
 * \desc            Delete a link aggregation group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteLagFromSwitch(fm_int sw, fm_int lagIndex)
{
    fm10000_lag *     lagExt;
    fm_switch *       switchPtr;
    fm_lag *          lagPtr;
    fm_int            port;
    fm_status         err;
    fm_int            i;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw=%d lagIndex=%d\n",
                 sw,
                 lagIndex);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_SWITCH_LAG_PTR(switchPtr, lagIndex);

    if (lagPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_INVALID_ARGUMENT);
    }
    lagExt    = GET_LAG_EXT(sw, lagIndex);

    while (lagPtr->nbMembers != 0)
    {
        /* Because members are always sorted, there should always
         * be a member in index 0 if lagPtr->nbMembers != 0 */
        port = lagPtr->member[0].port;
        err = fm10000DeletePortFromLag(sw, lagIndex, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    
    /* Because flushes are done in another thread, we don't know
     * when the flush will occur. We therefore provide a callback so that 
     * the LAG deletion can be completed once the LAG's glort has aged. */ 
     err = fmFlushPortAddrInternal(sw,
                                   lagPtr->logicalPort,
                                   fm10000DeleteLagCallback,
                                   (void *) lagPtr);

    if (err == FM_OK)
    {
        fm_logicalPortInfo* lportInfo;

        /* Since another thread is freeing the port and glort
         * we have to mark the port and glort delete pending
         * in order for caller to free the allocated LAGs if needed */
        lportInfo = &switchPtr->logicalPortInfo;

        for (i = 0 ; i < FM10000_GLORTS_PER_LAG ; i++)
        {
            FM_SET_GLORT_FREE_PEND(lportInfo, lagExt->lagGlort + i);
            FM_SET_LPORT_FREE_PEND(lportInfo, lagPtr->logicalPort + i);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000DeleteLagFromSwitch */




/*****************************************************************************/
/** fm10000FreeLAG
 * \ingroup intLag
 *
 * \desc            Free a link aggregation group's structures.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG to free.
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000FreeLAG(fm_int sw, fm_int lagIndex)
{
    fm_lag *lagPtr;

    lagPtr = GET_LAG_PTR(sw, lagIndex);

    if (lagPtr->extension != NULL)
    {
        /* Will not get to this code since DeleteLag set to null
         * before this and another thread is responsible for cleanup */
        fm10000FreeLogicalPort(sw, lagPtr->logicalPort);
    
        fmFree(lagPtr->extension);
        lagPtr->extension = NULL;
    }

}  /* end fm10000FreeLAG */




/*****************************************************************************/
/** fm10000AddPortToLag
 * \ingroup intLag
 *
 * \desc            Add a port to a link aggregation group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG to which the port
 *                  should be added.
 *
 * \param[in]       port is the number of the port to be added to the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if lagIndex is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_ALREADYUSED_PORT if the port is already a member
 *                  of a LAG.
 * \return          FM_ERR_FULL_LAG if the LAG already contains the maximum
 *                  number of ports (FM_MAX_NUM_LAG_MEMBERS).
 *
 *****************************************************************************/
fm_status fm10000AddPortToLag(fm_int sw, fm_int lagIndex, fm_int port)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_lag *   lagPtr;
    fm_bool    mcastReserved = FALSE;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_LAG_PTR(sw, lagIndex);

    /* Assume the common API code checked if port is already a member port */

    /**************************************************
     * Make sure we don't try to add more ports to the
     * LAG then what the number of logical ports 
     * that have been allocated to the LAG or the 
     * maximum number of port allowed in the LAG.. 
     * Since one logical port represents the LAG itself, 
     * the maximum number of ports allowed should be 1 
     * less then FM10000_GLORTS_PER_LAG.
     **************************************************/

    if ( (lagPtr->nbMembers >= (FM10000_GLORTS_PER_LAG-1)) ||
         (lagPtr->nbMembers >= FM_MAX_NUM_LAG_MEMBERS)  ||
         (lagPtr->nbMembers >= FM10000_MAX_NUM_LAG_MEMBERS) )
    {
        err = FM_ERR_FULL_LAG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /**************************************************
     * If the lag port is a listener in one or more 
     * multicast groups, new mcast group resources
     * may be necessary to include the newly added 
     * port. Try to reserve the needed multicast 
     * resources. 
     *  
     * WARNING:
     * If this operation is successful, the ROUTE
     * lock will have been taken and held.  It will not
     * be released until fm10000McastReleaseReservation
     * is called below.  Any code called from this point
     * on must be careful not to cause a deadlock due to
     * attempting to procure other locks.
     **************************************************/
    err = fm10000McastAddPortToLagReserveResources(sw, lagIndex, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    mcastReserved = TRUE;

    /**************************************************
     * Add port to LAG.
     **************************************************/
    err = fmAddLAGMember(sw, lagIndex, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /**************************************************
     * Set up port registers
     **************************************************/
    if (fmIsCardinalPort(sw, port))
    {
        err = UpdatePortMaskForLag(sw, lagIndex); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        err = fm10000UpdateLoopbackSuppress(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    err = UpdatePortCfgISL(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    if ( fmIsPortLinkUp(sw, port) )
    {
        /* The LAG_CFG register only needs to be
         * updated when active ports are added/removed */
        err = UpdateLagCfg(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    if (fmIsInternalPort(sw, port))
    {
        /* We need to update the glort table of all LAGs that make use
         * of the internal port. */
        err = UpdateGlortDestTableAllLags(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = UpdateGlortDestTable(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    
    /**************************************************
     * Flush any dynamic address that were associated
     * with the new port.
     **************************************************/
    if (fmGetBoolApiProperty(FM_AAK_API_MA_FLUSH_ON_LAG_CHANGE,
                             FM_AAD_API_MA_FLUSH_ON_LAG_CHANGE))
    {
        err = fmFlushPortAddresses(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Notify interested parties */
    err = fmMcastAddPortToLagNotify(sw, lagIndex, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* Now that the port is a member of the LAG, configure 
     * it using the LAG's port attributes that are 
     * common to all member ports. */
    if ( switchPtr->perLagMgmt &&
         fmIsCardinalPort(sw, port) )
    {
        /* Update port attributes */
        err = fm10000ApplyLagMemberPortAttr(sw, port, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        /* Update vlan settings */
        err = fmApplyLagMemberPortVlanStp(sw, port, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    
ABORT:

    if (mcastReserved)
    {
        fm10000McastReleaseReservation(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000AddPortToLag */




/*****************************************************************************/
/** fm10000DeletePortFromLag
 * \ingroup intLag
 *
 * \desc            Delete a port from a link aggregation group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagIndex is the index of the LAG from which the port 
 *                  should be deleted.
 *
 * \param[in]       port is the number of the port to be deleted from the LAG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_LAG if lagIndex is out of range or is
 *                  not the handle of an existing LAG.
 * \return          FM_ERR_INVALID_PORT if the port is not a member of the
 *                  specified LAG.
 *
 *****************************************************************************/
fm_status fm10000DeletePortFromLag(fm_int sw, fm_int lagIndex, fm_int port)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_lag *   lagPtr;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);
    lagPtr    = GET_LAG_PTR(sw, lagIndex);

    /* Assume the that common API code already checked if port is not a member
     * of the LAG */
    err = fmRemoveLAGMember(sw, lagIndex, port); 
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /**************************************************
     * Update hardware with member removed 
     **************************************************/

    if (fmIsInternalPort(sw, port))
    {
        /* We need to update the glort table of all LAGs that make use
         * of the internal port. */
        err = UpdateGlortDestTableAllLags(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }
    else
    {
        err = UpdateGlortDestTable(sw, lagIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* The LAG_CFG register only needs to be
     * updated when active, non-remote ports are added/removed */
    err = UpdateLagCfg(sw, lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /**************************************************
     * Unset port registers
     **************************************************/

    /* Don't do for remote logical ports */
    if (fmIsCardinalPort(sw, port))
    {
        err = DeletePortRegisters(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        /* when deleting we set the port mask last */
        err = UpdatePortMaskForLagRemove(sw, lagIndex, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

        err = fm10000UpdateLoopbackSuppress(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

    /* Notify interested parties */
    err = fmMcastRemovePortFromLagNotify(sw, lagIndex, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

    /* Now the port is no longer a member of the LAG 
     * restore its original port attribute settings. */
    if (switchPtr->perLagMgmt && 
        fmIsCardinalPort(sw, port) )
    {
        err = fm10000RestoreLagMemberPortAttr(sw, port);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000DeletePortFromLag */




/*****************************************************************************/
/** fm10000GetLagAttribute
 * \ingroup intLag
 *
 * \desc            Get a link aggregation group attribute.
 *
 * \param[in]       sw is the switch on which to operate.
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
 * \return          FM_ERR_INVALID_SWITCH if no switches are up.
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fm10000GetLagAttribute(fm_int sw,
                                 fm_int attribute,
                                 fm_int index,
                                 void * value)
{
    fm_status err = FM_OK;
    fm_lag *  lagPtr;
    fm_int    lagIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, attribute = %d, index = %d, value = %p\n",
                 sw,
                 attribute,
                 index,
                 (void *) value);

    switch (attribute)
    {
        case FM_LAG_HASH_ROTATION:
            /* Get the internal LAG index for this LAG logical port. */
            err = fmLogicalPortToLagIndex(sw, index, &lagIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            lagPtr = GET_LAG_PTR(sw,lagIndex);
            if (lagPtr)
            {
                *( (fm_uint32 *) value ) = lagPtr->hashRotation;
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_LAG_FILTERING:
            /* Get the internal LAG index for this LAG logical port. */
            err = fmLogicalPortToLagIndex(sw, index, &lagIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            lagPtr = GET_LAG_PTR(sw,lagIndex);
            if (lagPtr)
            {
                *( (fm_uint32 *) value ) = lagPtr->filteringEnabled;
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attribute) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000GetLagAttribute */




/*****************************************************************************/
/** fm10000SetLagAttribute
 * \ingroup intLag
 *
 * \desc            Set a link aggregation group attribute.
 *
 * \param[in]       sw is the switch on which to operate.
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
 * \return          FM_ERR_INVALID_ATTRIB if unrecognized attribute.
 *
 *****************************************************************************/
fm_status fm10000SetLagAttribute(fm_int sw,
                                 fm_int attribute,
                                 fm_int index,
                                 void * value)
{
    fm_status  err = FM_OK;
    fm_lag *   lagPtr;
    fm_int     lagIndex;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                 "sw = %d, attribute = %d, index = %d, value = %p\n",
                 sw,
                 attribute,
                 index,
                 (void *) value);

    switch (attribute)
    {
        case FM_LAG_HASH_ROTATION:
            /* Get the internal LAG index for this LAG logical port. */
            err = fmLogicalPortToLagIndex(sw, index, &lagIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            lagPtr = GET_LAG_PTR(sw,lagIndex);
            if (lagPtr != NULL)
            {
                if ( (*((fm_uint32 *) value)) <= 1 )
                {
                    lagPtr->hashRotation = *( (fm_uint32 *) value); 
                    err = UpdateLagCfg(sw, lagIndex);
                }
                else
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                }
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        case FM_LAG_FILTERING:
            /* Get the internal LAG index for this LAG logical port. */
            err = fmLogicalPortToLagIndex(sw, index, &lagIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, err);

            lagPtr = GET_LAG_PTR(sw,lagIndex);
            if (lagPtr != NULL)
            {
                lagPtr->filteringEnabled = *( (fm_bool *) value);
                err = UpdateLagCfg(sw, lagIndex);
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attribute) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LAG, err);

}   /* end fm10000SetLagAttribute */




/*****************************************************************************/
/** fm10000GetHardwareLagGlortRange
 * \ingroup intLag
 *
 * \desc            Get the glort range usable for LAGs.
 *
 * \param[out]      lagGlortBase points to caller-provided storage into
 *                  which the base glort value will be written.
 *
 * \param[out]      lagGlortCount points to caller-provided storage into
 *                  which the number of usable glorts will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetHardwareLagGlortRange(fm_uint32 *lagGlortBase,
                                          fm_uint32 *lagGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_LAG,
                "lagGlortBase=%p, lagGlortCount=%p\n",
                 (void *) lagGlortBase,
                 (void *) lagGlortCount);

    if (lagGlortBase != NULL)
    {
        *lagGlortBase = FM10000_GLORT_LAG_BASE;
    }

    if (lagGlortCount != NULL)
    {
        *lagGlortCount = FM10000_GLORT_LAG_SIZE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_OK);

}   /* end fm10000GetHardwareLagGlortRange */




/*****************************************************************************/
/** fm10000GetCanonicalLagGlort
 * \ingroup intLag
 *
 * \desc            Get Canonical Lag glort from a Lag member glort.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lagMemberGlort is the glort of LAG member.
 *
 * \param[out]      lagCanonicalGlort points to caller-provided storage into
 *                  which the canonical lag glort value is to be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetCanonicalLagGlort(fm_int     sw,
                                      fm_uint16  lagMemberGlort,
                                      fm_uint16 *lagCanonicalGlort)
{
    fm_status status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LAG,
                         "lagMemberGlort 0x%04x, lagCanonicalGlort=%p\n",
                         lagMemberGlort,
                         (void *) lagCanonicalGlort);

    FM_NOT_USED(sw);

    status = FM_OK;
    if (lagCanonicalGlort == NULL)
    {
        status = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LAG, status); 
    }

    *lagCanonicalGlort = lagMemberGlort & (~( (1 << FM10000_LAG_MASK_SIZE) - 1 ));

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LAG, status);

}   /* end fm10000GetCanonicalLagGlort */
