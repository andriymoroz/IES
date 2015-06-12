/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_cardinal.c
 * Creation Date:   October 22, 2011
 * Description:     Cardinal port support functions.
 *
 * Copyright (c) 2011 - 2014, Intel Corporation
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

#if 1
    #define COMPLAIN(str, ...) \
        FM_LOG_FATAL(FM_LOG_CAT_PORT, str, __VA_ARGS__)
#else
    #define COMPLAIN(str, ...) { }
#endif

#ifdef __KLOCWORK__
    #define INVALID_PORT    0
#else
    #define INVALID_PORT    -1
#endif


/*---------------------------------------------------------------------------*/
/*                              Internal Functions                           */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmAllocCardinalPortMap
 * \ingroup intSwitch
 *
 * \desc            Allocates the cardinal port map.
 *
 * \param[in]       switchPtr is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocCardinalPortMap(fm_switch * switchPtr)
{
    fm_cardinalPortInfo * cardinalPortInfo;
    fm_int                nbytes;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    cardinalPortInfo = &switchPtr->cardinalPortInfo;

    if (cardinalPortInfo->portMap != NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "switch %d cardinal port map not NULL on entry: %p\n",
                     switchPtr->switchNumber,
                     (void *) cardinalPortInfo->portMap);

        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_FAIL);
    }

    if (cardinalPortInfo->indexTable != NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "switch %d cardinal port index table not NULL: %p\n",
                     switchPtr->switchNumber,
                     (void *) cardinalPortInfo->indexTable);
    }

    nbytes = (switchPtr->maxPhysicalPort + 1) * sizeof(fm_cardinalPort);

    cardinalPortInfo->portMap = (fm_cardinalPort *)fmAlloc(nbytes);

    if (cardinalPortInfo->portMap == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
    }

    cardinalPortInfo->maxLogicalPort = -1;

    switchPtr->numCardinalPorts = 0;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmAllocCardinalPortMap */




/*****************************************************************************/
/** fmCreateCardinalPortIndexTable
 * \ingroup intSwitch
 *
 * \desc            Creates the cardinal port index table.
 *
 * \param[in]       switchPtr is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCreateCardinalPortIndexTable(fm_switch * switchPtr)
{
    fm_cardinalPortInfo * cardinalPortInfo;
    fm_int      nbytes;
    fm_int      logPort;
    fm_int      cpi;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    cardinalPortInfo = &switchPtr->cardinalPortInfo;

    nbytes = sizeof(fm_int) * (cardinalPortInfo->maxLogicalPort + 1);

    cardinalPortInfo->indexTable = (fm_int *) fmAlloc(nbytes);

    if (cardinalPortInfo->indexTable == NULL)
    {
        fmFree(cardinalPortInfo->portMap);
        cardinalPortInfo->portMap = NULL;
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
    }

    memset(cardinalPortInfo->indexTable, -1, nbytes);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        logPort = cardinalPortInfo->portMap[cpi].logPort;
        cardinalPortInfo->indexTable[logPort] = cpi;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmCreateCardinalPortIndexTable */




/*****************************************************************************/
/** fmDbgDumpCardinalPorts
 * \ingroup intSwitch
 *
 * \desc            Dumps the cardinal port data structures.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpCardinalPorts(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;
    fm_int      cpi;
    fm_int      logPort;
    fm_int      physPort;
    fm_port *   portPtr;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("maxPhysicalPort   : %d\n", switchPtr->maxPhysicalPort);
    FM_LOG_PRINT("numCardinalPorts  : %d\n", switchPtr->numCardinalPorts);
    FM_LOG_PRINT("maxReservedPort   : %d\n", switchPtr->maxReservedPort);
    FM_LOG_PRINT("info.numPorts     : %d\n", switchPtr->info.numPorts);
    FM_LOG_PRINT("info.numCardPorts : %d\n", switchPtr->info.numCardPorts);
    FM_LOG_PRINT("maxPort           : %d\n", switchPtr->maxPort);
    FM_LOG_PRINT("maxLogicalPort    : %d\n", switchPtr->cardinalPortInfo.maxLogicalPort);

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("CPI  logPort  physPort    portPtr   portType\n");
    FM_LOG_PRINT("---  -------  --------  ----------  --------\n");

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        err = fmMapCardinalPort(sw, cpi, &logPort, &physPort);
        if (err != FM_OK)
        {
            FM_LOG_PRINT("Error mapping cpi %d: %s\n", sw, fmErrorMsg(err));
            goto ABORT;
        }

        portPtr = switchPtr->portTable[logPort];

        FM_LOG_PRINT("%3d  %5d    %6d    %10p",
                     cpi,
                     logPort,
                     physPort,
                     (void *) portPtr);

        if (portPtr != NULL)
        {
            FM_LOG_PRINT("  %-8s", fmPortTypeToText(portPtr->portType));
        }

        FM_LOG_PRINT("\n");
    }

    FM_LOG_PRINT("\n");

ABORT:
    UNPROTECT_SWITCH(sw);

    return FM_OK;

}   /* end fmDbgDumpCardinalPorts */




/*****************************************************************************/
/** fmFreeCardinalPortDataStructures
 * \ingroup intSwitch
 *
 * \desc            Frees the cardinal port data structures.
 *
 * \param[in]       switchPtr is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeCardinalPortDataStructures(fm_switch * switchPtr)
{

    if (switchPtr->cardinalPortInfo.portMap)
    {
        fmFree(switchPtr->cardinalPortInfo.portMap);
        switchPtr->cardinalPortInfo.portMap = NULL;
    }

    if (switchPtr->cardinalPortInfo.indexTable)
    {
        fmFree(switchPtr->cardinalPortInfo.indexTable);
        switchPtr->cardinalPortInfo.indexTable = NULL;
    }

    return FM_OK;

}   /* end fmFreeCardinalPortDataStructures */




/*****************************************************************************/
/** fmCompareCardinalPorts
 * \ingroup intSwitch
 *
 * \desc            Compares two cardinal port map table entries to determine
 *                  which one precedes the other. This function is passed to
 *                  qsort().
 *
 * \param[in]       aPtr is a void pointer to an fm_cardinalPort.
 *
 * \param[in]       bPtr is a void pointer to an fm_cardinalPort.
 *
 * \return          -1 if a should be ordered before b
 * \return          0 if a and b should be considered equal
 * \return          1 if a should be ordered after b
 *
 *****************************************************************************/
int fmCompareCardinalPorts(const void * aPtr, const void * bPtr)
{
    fm_cardinalPort * ptrA = (fm_cardinalPort *) aPtr;
    fm_cardinalPort * ptrB = (fm_cardinalPort *) bPtr;

    if (ptrA->logPort < ptrB->logPort)
    {
        return -1;
    }
    else if (ptrA->logPort > ptrB->logPort)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}   /* end fmCompareCardinalPorts */


/*---------------------------------------------------------------------------*/
/*                              BitArray Functions                           */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmBitArrayToPortList
 * \ingroup intSwitch
 *
 * \desc            Converts a bit array to a list of cardinal ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bitArray is the cardinal port bit array to convert.
 *
 * \param[out]      numPorts points to the location to receive the number
 *                  of ports in the list.
 *
 * \param[out]      portList points to a caller-supplied array to receive
 *                  the list of cardinal ports.
 *
 * \param[in]       maxPorts is the size of portList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is
 *                  out of range.
 * \return          FM_ERR_BUFFER_FULL is portList is not large enough
 *                  to hold all the ports in the bit array.
 *
 *****************************************************************************/
fm_status fmBitArrayToPortList(fm_int        sw, 
                               fm_bitArray * bitArray,
                               fm_int *      numPorts, 
                               fm_int *      portList, 
                               fm_int        maxPorts)
{
    fm_switch * switchPtr;
    fm_int      bitNo;
    fm_status   err;

    if (bitArray == NULL || numPorts == NULL || portList == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    *numPorts = 0;
    err = FM_OK;

    for (bitNo = 0 ; ; ++bitNo)
    {
        err = fmFindBitInBitArray(bitArray, bitNo, TRUE, &bitNo);
        if (err != FM_OK || bitNo < 0 || bitNo >= switchPtr->numCardinalPorts)
        {
            break;
        }

        if (*numPorts >= maxPorts)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }

        portList[*numPorts] = GET_LOGICAL_PORT(sw, bitNo);
        *numPorts += 1;
    }

    return err;

}   /* end fmBitArrayToPortList */




/*****************************************************************************/
/** fmFindPortInBitArray
 * \ingroup intSwitch
 *
 * \desc            Finds the next cardinal port in a bit array.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bitArray is the cardinal port bit array to check.
 * 
 * \param[in]       lastPort is the previous logical port number,
 *                  or -1 if this is the first call to the function.
 * 
 * \param[out]      nextPort points to the location in which the function
 *                  should store the logical port number of the next port
 *                  in the bit array, or -1 if there are no more ports.
 * 
 * \param[out]      notFound is the status to be returned if there are
 *                  no more ports in the array.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFindPortInBitArray(fm_int        sw,
                               fm_bitArray * bitArray,
                               fm_int        lastPort,
                               fm_int      * nextPort,
                               fm_status     notFound)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_status   err;

    *nextPort = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    cpi = (lastPort < 0) ? -1 : GET_PORT_INDEX(sw, lastPort);

    err = fmFindBitInBitArray(bitArray, cpi + 1, TRUE, &cpi);

    if (err == FM_OK)
    {
        if (cpi >= 0 && cpi < switchPtr->numCardinalPorts)
        {
            *nextPort = GET_LOGICAL_PORT(sw, cpi);
        }
        else
        {
            err = notFound;
        }
    }

    return err;

}   /* end fmFindPortInBitArray */




/*****************************************************************************/
/** fmGetPortInBitArray
 * \ingroup intSwitch
 *
 * \desc            Gets the state of a port in a cardinal port bit array.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       bitArray is the cardinal port bit array.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[out]      state points to caller-supplied storage that is set to
 *                  TRUE if the port is enabled, or FALSE if it is disabled.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmGetPortInBitArray(fm_int        sw,
                              fm_bitArray * bitArray,
                              fm_int        port,
                              fm_bool *     state)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_status   err;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    err = fmGetBitArrayBit(bitArray, cpi, state);

    return err;

}   /* end fmGetPortInBitArray */




/*****************************************************************************/
/** fmSetPortInBitArray
 * \ingroup intSwitch
 *
 * \desc            Sets the state of a port in a cardinal port bit array.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   bitArray is the cardinal port bit array to update.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[in]       state is TRUE if the port is to be enabled, or FALSE
 *                  if it is to be disabled.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmSetPortInBitArray(fm_int        sw,
                              fm_bitArray * bitArray,
                              fm_int        port,
                              fm_bool       state)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_status   err;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    err = fmSetBitArrayBit(bitArray, cpi, state);

    return err;

}   /* end fmSetPortInBitArray */


/*---------------------------------------------------------------------------*/
/*                              BitMask Functions                            */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmFindPortInBitMask
 * \ingroup intSwitch
 *
 * \desc            Finds the next cardinal port in a bit mask.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bitMask is the cardinal port bit mask to check.
 * 
 * \param[in]       lastPort is the previous logical port number,
 *                  or -1 if this is the first call to the function.
 * 
 * \param[out]      nextPort points to the location in which the function
 *                  should store the logical port number of the next port
 *                  in the bit mask, or -1 if there are no more ports.
 * 
 * \param[out]      notFound is the status to be returned if there are
 *                  no more ports in the bit mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFindPortInBitMask(fm_int         sw,
                              fm_uint32      bitMask,
                              fm_int         lastPort,
                              fm_int       * nextPort,
                              fm_status      notFound)
{
    fm_switch * switchPtr;
    fm_int      limit;
    fm_int      cpi;

    *nextPort = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    limit = switchPtr->numCardinalPorts;
    if (limit > (fm_int)(sizeof(bitMask) * 8))
    {
        limit = sizeof(bitMask) * 8;
    }

    cpi = (lastPort < 0) ? -1 : GET_PORT_INDEX(sw, lastPort);

    for (cpi = cpi + 1 ; cpi < limit ; cpi++)
    {
        if ( bitMask & (1 << cpi) )
        {
            *nextPort = GET_LOGICAL_PORT(sw, cpi);
            return FM_OK;
        }
    }

    return notFound;

}   /* end fmFindPortInBitMask */




/*****************************************************************************/
/** fmGetPortInBitMask
 * \ingroup intSwitch
 *
 * \desc            Gets the state of a port in a cardinal port bit mask.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       bitMask is the cardinal port bit mask.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[out]      state points to caller-supplied storage that is set to
 *                  TRUE if the port is enabled, or FALSE if it is disabled.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid
 *                  or the port index is out of range.
 *
 *****************************************************************************/
fm_status fmGetPortInBitMask(fm_int      sw,
                             fm_uint32   bitMask,
                             fm_int      port,
                             fm_bool *   state)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);
                            
    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        COMPLAIN("Logical port %d is out of bounds\n", port);
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        COMPLAIN("Port %d is not a cardinal port\n", port);
        return FM_ERR_INVALID_PORT;
    }

    if (cpi >= switchPtr->numCardinalPorts ||
        cpi >= (fm_int)(sizeof(fm_uint32) * 8))
    {
        COMPLAIN("Port %d maps to bit %d, which is out of bounds\n", port, cpi);
        return FM_ERR_INVALID_PORT;
    }

    *state = (bitMask & (1 << cpi)) != 0;

    return FM_OK;

}   /* end fmGetPortInBitMask */




/*****************************************************************************/
/** fmSetPortInBitMask
 * \ingroup intSwitch
 *
 * \desc            Sets the state of a port in a cardinal port bit mask.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   bitMask points to the cardinal port bit mask.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[in]       state is TRUE if the port is to be enabled, or FALSE
 *                  if it is to be disabled.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid
 *                  or the port index is out of range.
 *
 *****************************************************************************/
fm_status fmSetPortInBitMask(fm_int      sw,
                             fm_uint32 * bitMask,
                             fm_int      port,
                             fm_bool     state)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        COMPLAIN("Logical port %d is out of bounds\n", port);
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        COMPLAIN("Port %d is not a cardinal port\n", port);
        return FM_ERR_INVALID_PORT;
    }

    if (cpi >= switchPtr->numCardinalPorts ||
        cpi >= (fm_int)(sizeof(fm_uint32) * 8))
    {
        COMPLAIN("Port %d maps to bit %d, which is out of bounds\n", port, cpi);
        return FM_ERR_INVALID_PORT;
    }

    if (state)
    {
        *bitMask |= (1 << cpi);
    }
    else
    {
        *bitMask &= ~(1 << cpi);
    }

    return FM_OK;

}   /* end fmSetPortInBitMask */


/*---------------------------------------------------------------------------*/
/*                              PortMask Functions                           */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmFindPortInPortMask
 * \ingroup intSwitch
 *
 * \desc            Finds the next cardinal port in a port mask.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portMask is the cardinal port mask to check.
 * 
 * \param[in]       lastPort is the previous logical port number,
 *                  or -1 if this is the first call to the function.
 * 
 * \param[out]      nextPort points to the location in which the function
 *                  should store the logical port number of the next port
 *                  in the port mask, or -1 if there are no more ports.
 * 
 * \param[out]      notFound is the status to be returned if there are
 *                  no more ports in the port mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFindPortInPortMask(fm_int        sw,
                               fm_portmask * portMask,
                               fm_int        lastPort,
                               fm_int      * nextPort,
                               fm_status     notFound)
{
    fm_switch * switchPtr;
    fm_int      limit;
    fm_int      cpi;

    *nextPort = -1;

    switchPtr = GET_SWITCH_PTR(sw);

    limit = switchPtr->numCardinalPorts;
    if (limit > FM_PORTMASK_NUM_BITS)
    {
        limit = FM_PORTMASK_NUM_BITS;
    }

    cpi = (lastPort < 0) ? -1 : GET_PORT_INDEX(sw, lastPort);

    for (cpi = cpi + 1 ; cpi < limit ; cpi++)
    {
        if ( FM_PORTMASK_IS_BIT_SET(portMask, cpi) )
        {
            *nextPort = GET_LOGICAL_PORT(sw, cpi);
            return FM_OK;
        }
    }

    return notFound;

}   /* end fmFindPortInPortMask */




/*****************************************************************************/
/** fmEnablePortInPortMask
 * \ingroup intSwitch
 *
 * \desc            Enables a port in a cardinal port mask.
 * 
 * \note            Replaces FM_PORTMASK_ENABLE_PORT for cardinal ports.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   maskPtr points to the port mask.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmEnablePortInPortMask(fm_int sw, fm_portmask * maskPtr, fm_int port)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    FM_PORTMASK_ENABLE_BIT(maskPtr, cpi);

    return FM_OK;

}   /* end fmEnablePortInPortMask */




/*****************************************************************************/
/** fmDisablePortInPortMask
 * \ingroup intSwitch
 *
 * \desc            Disables a port in a cardinal port mask.
 * 
 * \note            Replaces FM_PORTMASK_DISABLE_PORT for cardinal ports.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   maskPtr points to the port mask.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmDisablePortInPortMask(fm_int sw, fm_portmask * maskPtr, fm_int port)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    FM_PORTMASK_DISABLE_BIT(maskPtr, cpi);

    return FM_OK;

}   /* end fmDisablePortInPortMask */




/*****************************************************************************/
/** fmSetPortInPortMask
 * \ingroup intSwitch
 *
 * \desc            Sets the state of a port in a cardinal port mask.
 * 
 * \note            Replaces FM_PORTMASK_SET_PORT for cardinal ports.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   maskPtr points to the port mask.
 *
 * \param[in]       port is the logical port number.
 * 
 * \param[in]       state is TRUE if the port is to be enabled, or FALSE
 *                  if it is to be disabled.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmSetPortInPortMask(fm_int        sw,
                              fm_portmask * maskPtr,
                              fm_int        port,
                              fm_bool       state)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    if (state)
    {
        FM_PORTMASK_ENABLE_BIT(maskPtr, cpi);
    }
    else
    {
        FM_PORTMASK_DISABLE_BIT(maskPtr, cpi);
    }

    return FM_OK;

}   /* end fmSetPortInPortMask */




/*****************************************************************************/
/** fmAssignPortToPortMask
 * \ingroup intSwitch
 *
 * \desc            Enables exactly one port in a cardinal port mask.
 * 
 * \note            Replaces FM_PORTMASK_ASSIGN_PORT for cardinal ports.
 * 
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in,out]   maskPtr points to the port mask.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful
 *                  FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
fm_status fmAssignPortToPortMask(fm_int        sw,
                                 fm_portmask * maskPtr,
                                 fm_int        port)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return FM_ERR_INVALID_PORT;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];
    if (cpi < 0)
    {
        return FM_ERR_INVALID_PORT;
    }

    FM_PORTMASK_DISABLE_ALL(maskPtr);
    FM_PORTMASK_ENABLE_BIT(maskPtr, cpi);

    return FM_OK;

}   /* end fmAssignPortToPortMask */


/*---------------------------------------------------------------------------*/
/*                              Validation Functions                         */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmIsCardinalPort
 * \ingroup intSwitch
 *
 * \desc            Determines whether a logical port is a cardinal port.
 * 
 * \note            Assumes that the switch number is valid and the
 *                  appropriate lock has been taken.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port number.
 *
 * \return          TRUE if the port is a valid cardinal port, FALSE otherwise.
 *
 *****************************************************************************/
fm_bool fmIsCardinalPort(fm_int sw, fm_int port)
{
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (port >= 0 && port <= switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        return (switchPtr->cardinalPortInfo.indexTable[port] != -1);
    }

    return FALSE;

}   /* end fmIsCardinalPort */


/*---------------------------------------------------------------------------*/
/*                              Mapping Functions                            */
/*---------------------------------------------------------------------------*/


/*****************************************************************************/
/** fmMapCardinalPortInternal
 * \ingroup intSwitch
 *
 * \desc            Retrieves the logical and physical port numbers of
 *                  the specified cardinal port.
 *
 * \param[in]       switchPtr is the switch on which to operate.
 * 
 * \param[in]       cpi is the cardinal port index, in the range
 *                  0..numCardinalPorts-1.
 *
 * \param[out]      logPort is a pointer to caller-provided memory
 *                  in which this function should store the logical port
 *                  number. May be NULL, in which case no value will be
 *                  returned.
 * 
 * \param[out]      physPort is a pointer to caller-provided memory
 *                  in which this function should store the physical port
 *                  number. May be NULL, in which case no value will be
 *                  returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if cpi is out of range.
 *
 *****************************************************************************/
fm_status fmMapCardinalPortInternal(fm_switch * switchPtr,
                                    fm_int      cpi,
                                    fm_int *    logPort,
                                    fm_int *    physPort)
{

    if (cpi >= switchPtr->numCardinalPorts || cpi < 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (logPort != NULL)
    {
        *logPort = switchPtr->cardinalPortInfo.portMap[cpi].logPort;
    }

    if (physPort != NULL)
    {
        *physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;
    }

    return FM_OK;

}   /* end fmMapCardinalPortInternal */




#if !defined(GET_LOGICAL_PORT)

/*****************************************************************************/
/** GET_LOGICAL_PORT
 * \ingroup intSwitch
 *
 * \desc            Retrieves the logical port number of the specified
 *                  cardinal port.
 * 
 * \note            This is a function implementation of the GET_LOGICAL_PORT
 *                  macro, used for debugging.
 * 
 * \note            The switch number and port index are assumed to be valid.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       cpi is the cardinal port index, in the range
 *                  0..numCardinalPorts-1.
 *
 * \return          The logical port number of the cardinal port.
 *
 *****************************************************************************/
fm_int GET_LOGICAL_PORT(fm_int sw, fm_int cpi)
{
    fm_switch * switchPtr = GET_SWITCH_PTR(sw);
    fm_int      port;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d cpi=%d\n",
                         sw, cpi);

    if (switchPtr->numCardinalPorts <= 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT, "No cardinal ports defined\n");
        port = INVALID_PORT;
        goto ABORT;
    }

    if (cpi < 0 || cpi >= switchPtr->numCardinalPorts)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT, "Invalid CPI (%d)\n", cpi);
        port = INVALID_PORT;
        goto ABORT;
    }

    port = switchPtr->cardinalPortInfo.portMap[cpi].logPort;

ABORT:
    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH,
                               port,
                               "port=%d\n",
                               port);

}   /* end GET_LOGICAL_PORT */

#endif



#if !defined(GET_PORT_INDEX)

/*****************************************************************************/
/** GET_PORT_INDEX
 * \ingroup intSwitch
 *
 * \desc            Returns the cardinal port index for a port, given the
 *                  logical port number.
 * 
 * \note            This is a function version of the GET_PORT_INDEX macro,
 *                  used for debugging.
 * 
 * \note            Assumes that the switch and port numbers are valid.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       port is the logical port number.
 *
 * \return          The cardinal port index for the port.
 *
 *****************************************************************************/
fm_int GET_PORT_INDEX(fm_int sw, fm_int port)
{
    fm_switch * switchPtr;
    fm_int      cpi;

    if (sw < 0 || sw >= FM_MAX_NUM_SWITCHES)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Switch number %d is out of bounds\n",
                     sw);
        return -1;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    if (switchPtr == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Switch number %d does not exist\n",
                     sw);
        return -1;
    }

    if (port < 0 || port > switchPtr->cardinalPortInfo.maxLogicalPort)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Logical port %d is out of bounds\n",
                     port);
        return -1;
    }

    cpi = switchPtr->cardinalPortInfo.indexTable[port];

    if (cpi < 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PORT,
                     "Port %d is not a cardinal port\n",
                     port);
        return -1;
    }

    return cpi;

}   /* end GET_PORT_INDEX */

#endif




/*****************************************************************************/
/** fmMapCardinalPort
 * \ingroup intSwitch
 *
 * \desc            Retrieves the logical and physical port numbers for
 *                  the specified cardinal port index.
 * 
 * \note            This is an unpublished API. It exists primarily
 *                  for use by SQA test programs.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       portIndex is the cardinal port index, in the range
 *                  0..numCardinalPorts-1.
 *
 * \param[out]      logicalPort is a pointer to caller-provided memory
 *                  in which this function should place the logical port
 *                  number. May be NULL, in which case no value will be
 *                  returned.
 * 
 * \param[out]      physicalPort is a pointer to caller-provided memory
 *                  in which this function should store the physical port
 *                  number. May be NULL, in which case no value will be
 *                  returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if portIndex is out of range.
 *
 *****************************************************************************/
fm_status fmMapCardinalPort(fm_int   sw,
                            fm_int   portIndex,
                            fm_int * logicalPort,
                            fm_int * physicalPort)
{
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw=%d index=%d logPort=%p physPort=%p\n",
                     sw,
                     portIndex,
                     (void *) logicalPort,
                     (void *) physicalPort);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmMapCardinalPortInternal(GET_SWITCH_PTR(sw),
                                    portIndex,
                                    logicalPort,
                                    physicalPort);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmMapCardinalPort */




/*****************************************************************************/
/** fmGetCardinalPortList
 * \ingroup intSwitch
 *
 * \desc            Returns a list of the cardinal ports on the switch.
 * 
 * \note            This is an unpublished API. It exists primarily
 *                  for use by SQA test programs.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       numPorts points to a caller-provided location where
 *                  this function should store the number of entries
 *                  in the port list.
 *
 * \param[out]      portList points to a caller-provided array where this
 *                  function should store the list of logical port numbers.
 * 
 * \param[in]       maxPorts is the maximum number of entries that may be
 *                  stored in the port list.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BUFFER_FULL if maxPorts was too small to hold the
 *                  port list.
 *
 *****************************************************************************/
fm_status fmGetCardinalPortList(fm_int   sw,
                                fm_int * numPorts,
                                fm_int * portList,
                                fm_int   maxPorts)
{
    fm_switch * switchPtr;
    fm_int      cpi;
    fm_status   err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PORT,
                     "sw=%d portList=%p maxPorts=%d\n",
                     sw,
                     (void *) portList,
                     maxPorts);

    *numPorts = 0;

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        if (cpi >= maxPorts)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }
        portList[cpi] = GET_LOGICAL_PORT(sw, cpi);
    }

    *numPorts = cpi;

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_PORT, err);

}   /* end fmGetCardinalPortList */

