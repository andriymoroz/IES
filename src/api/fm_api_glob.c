/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_glob.c
 * Creation Date:   2005
 * Description:     Contains structure definitions for various table entries
 *                  and switch state information
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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
 ****************************************************************************/

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#ifndef FM_GLOBAL_DECL 
#define FM_GLOBAL_DECL 
#endif


#if 0
#define FM_LOG_EXIT_CUSTOM(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_FUNC_EXIT, "Exiting... " __VA_ARGS__ )
#endif


/*****************************************************************************
 * Global Variables
 *****************************************************************************/
/* pointer to event handler */
fm_eventHandler fmEventHandler;

/* state shared between processes */
FM_GLOBAL_DECL fm_rootApi *    fmRootApi;

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
/** MapLagPortToMemberPort
 * \ingroup intSwitch
 *
 * \desc            Maps a LAG logical port to one of its member ports.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       port is the LAG logical port number.
 *
 * \param[out]      logPort points to the location to receive the logical
 *                  port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the port number is invalid.
 * \return          FM_ERR_NO_PORTS_IN_LAG if port refers to an empty LAG.
 *
 *****************************************************************************/
static fm_status MapLagPortToMemberPort(fm_int  sw,
                                        fm_int  port,
                                        fm_int *logPort)
{
    fm_int      portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_port *   portPtr;
    fm_lag *    lagPtr;
    fm_int      numPorts;
    fm_status   err;

    portPtr = GET_PORT_PTR(sw, port);
    if (portPtr == NULL)
    {
        return FM_ERR_INVALID_PORT;
    }

    lagPtr = GET_LAG_PTR(sw, portPtr->lagIndex);

    if(lagPtr == NULL)
    {
        return FM_ERR_INVALID_PORT;
    }

    /* Get a list of active member ports. */
    err = fmGetLAGCardinalPortList(sw, 
                                   lagPtr->logicalPort,
                                   &numPorts,
                                   portList, 
                                   FM_MAX_NUM_LAG_MEMBERS);

    if (err != FM_OK)
    {
        return err;
    }

    if (numPorts == 0)
    {
        /* Return the first port even if the link is currently down.
         * This should ensure that a SWAG or Stacked Configuration
         * works properly at init time. */
        err = fmGetLAGMemberPorts(sw,
                                  portPtr->lagIndex,
                                  &numPorts,
                                  portList,
                                  FM_MAX_NUM_LAG_MEMBERS,
                                  FALSE);
        if (err != FM_OK)
        {
            return err;
        }
        else if (numPorts == 0)
        {
            return FM_ERR_NO_PORTS_IN_LAG;
        }
    }

    /*
     * Select one of the LAG ports using pseudo-random selection. 
     *  
     * Why are we doing "pseudo-random" selection? Why not (for 
     * example) return the logical port that corresponds to the canonical 
     * logical port?
     */
    *logPort = portList[port % numPorts];

    return FM_OK;

}   /* end MapLagPortToMemberPort */




/*****************************************************************************/
/** MapVirtualPortToPepPort
 * \ingroup intSwitch
 *
 * \desc            Maps a VIRTUAL logical port to the underlying PEP
 *                  logical port.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       port is the LAG logical port number.
 *
 * \param[out]      logPort points to the location to receive the logical
 *                  port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the port number is invalid.
 *
 *****************************************************************************/
static fm_status MapVirtualPortToPepPort(fm_int  sw,
                                         fm_int  port,
                                         fm_int *logPort)
{
    fm_switch * switchPtr;
    fm_uint32   glort;
    fm_status   err;

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmGetLogicalPortGlort(sw, port, &glort);

    if (err == FM_OK)
    {
        FM_API_CALL_FAMILY(err,
                           switchPtr->MapVirtualGlortToPepLogicalPort,
                           sw,
                           glort,
                           logPort);
    }

    return err;

}   /* end MapVirtualPortToPepPort */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmMapLogicalPortToPhysical
 * \ingroup intSwitch
 *
 * \desc            Maps a logical port number to a physical port number.
 *
 * \param[in]       sstate points to the switch state entry.
 *
 * \param[in]       logPort is the logical port number. Must be a cardinal
 *                  port or one of the supported non-cardinal port types
 *                  (LAG, REMOTE, VIRTUAL).
 *
 * \param[out]      physPort points to the location to receive the physical
 *                  port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if logPort is not a valid port number.
 * \return          FM_ERR_NO_PORTS_IN_LAG if logPort refers to an empty LAG.
 * \return          FM_ERR_INVALID_ARGUMENT if there is no port structure
 *                  allocated for given port number.
 *
 *****************************************************************************/
fm_status fmMapLogicalPortToPhysical(fm_switch *sstate,
                                     fm_int     logPort,
                                     fm_int *   physPort)
{
    fm_int      sw;
    fm_int      dummySw;
    fm_int      intPort;
    fm_status   err;

    sw = sstate->switchNumber;
    intPort = logPort;

    /*
     * This function is used primarily by code that needs to program the 
     * hardware. It is generally applied to cardinal ports. 
     *  
     * In some cases, however, the function may be applied to certain 
     * non-cardinal port types. In these cases, the logical port must be 
     * reduced to a cardinal port. 
     */
    if (!fmIsCardinalPort(sw, intPort))
    {
        /* If this is a remote logical port, get the internal port. */
        if (fmIsRemotePort(sw, intPort))
        {
            err = fmGetInternalPortFromRemotePort(sw, intPort, &intPort);
            if (err != FM_OK)
            {
                return err;
            }
        }

        /* If this is a LAG logical port, get one of its member ports. */
        if (fmIsLagPort(sw, intPort))
        {
            err = MapLagPortToMemberPort(sw, intPort, &intPort);
            if (err != FM_OK)
            {
                return err;
            }
        }

        /* If this is a virtual logical port, get PEP logical port. */
        if (fmIsVirtualPort(sw, intPort))
        {
            err = MapVirtualPortToPepPort(sw, intPort, &intPort);
            if (err != FM_OK)
            {
                return err;
            }
        }

        /* Verify that the end result is a cardinal port. */
        if (!fmIsCardinalPort(sw, intPort))
        {
#if 1
            /* A failure at this point probably means that there is a bug
             * in the API. Generate an error that will register in the
             * regression test system. */
            if (intPort != logPort)
            {
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "Port %d (%s) maps to non-cardinal port %d (%s)\n",
                             logPort,
                             fmGetPortTypeAsText(sw, logPort),
                             intPort,
                             fmGetPortTypeAsText(sw, intPort));
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                             "Port %d (%s) does not map to a cardinal port\n",
                             logPort,
                             fmGetPortTypeAsText(sw, intPort));
            }
#endif

            /* Implement FM_ERR_INVALID_PORT_TYPE? */
            return FM_ERR_INVALID_PORT;
        }

    }   /* end !fmIsCardinalPort(sw, intPort) */

    return fmPlatformMapLogicalPortToPhysical(sw, intPort, &dummySw, physPort);

}   /* end fmMapLogicalPortToPhysical */




/*****************************************************************************/
/** fmMapPhysicalPortToLogical
 * \ingroup intSwitch
 *
 * \desc            maps physical port number to logical port number
 *
 * \param[in]       sstate points to the switch state entry
 *
 * \param[in]       physPort is the physical port number
 *
 * \param[out]      logPort contains the logical port number
 *
 * \return          FM_OK if request succeeded.
 *
 *****************************************************************************/
fm_status fmMapPhysicalPortToLogical(fm_switch *sstate,
                                     fm_int     physPort,
                                     fm_int *   logPort)
{
    fm_int switchNum;

    return fmPlatformMapPhysicalPortToLogical(sstate->switchNumber,
                                              physPort,
                                              &switchNum,
                                              logPort);

}   /* end fmMapPhysicalPortToLogical */




/*****************************************************************************/
/** fmMapBitMaskLogicalToPhysical
 * \ingroup intSwitch
 *
 * \desc            maps logical port bit mask to physical port bit mask
 *
 * \param[in]       switchPtr points to the switch state entry
 *
 * \param[in]       logMask is the logical port bit mask
 *
 * \param[out]      physMask contains the physical port bit mask
 *
 * \return          FM_OK if request succeeded.
 *
 *****************************************************************************/
fm_status fmMapBitMaskLogicalToPhysical(fm_switch * switchPtr,
                                        fm_uint32   logMask,
                                        fm_uint32 * physMask)
{
    fm_int    limit;
    fm_int    cpi;
    fm_int    physPort;
    fm_uint32 newMask = 0;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, logMask = 0x%08x, "
                         "physMask = %p\n",
                         switchPtr->switchNumber,
                         logMask,
                         (void *) physMask);

    limit = switchPtr->numCardinalPorts;
    if (limit > (fm_int)(sizeof(logMask) * 8))
    {
        limit = sizeof(logMask) * 8;
    }

    for (cpi = 0 ; cpi < limit ; cpi++)
    {
        if ( logMask & (1 << cpi) )
        {
            physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;
            newMask |= (1 << physPort);
        }
    }

    *physMask = newMask;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmMapBitMaskLogicalToPhysical */




/*****************************************************************************/
/** fmMapBitMaskPhysicalToLogical
 * \ingroup intSwitch
 *
 * \desc            maps physical port bit mask to logical port bit mask
 *
 * \param[in]       switchPtr points to the switch state entry
 *
 * \param[in]       physMask is the physical port bit mask
 *
 * \param[out]      logMask contains the logical port bit mask
 *
 * \return          FM_OK if request succeeded.
 *
 *****************************************************************************/
fm_status fmMapBitMaskPhysicalToLogical(fm_switch * switchPtr,
                                        fm_uint32   physMask,
                                        fm_uint32 * logMask)
{
    fm_int    limit;
    fm_int    cpi;
    fm_int    physPort;
    fm_uint32 newMask = 0;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, physMask = 0x%08x, "
                         "logMask = %p\n",
                         switchPtr->switchNumber,
                         physMask,
                         (void *) logMask);

    limit = switchPtr->numCardinalPorts;
    if (limit > (fm_int)(sizeof(physMask) * 8))
    {
        limit = sizeof(physMask) * 8;
    }

    for (cpi = 0 ; cpi < limit ; cpi++)
    {
        physPort = switchPtr->cardinalPortInfo.portMap[cpi].physPort;
        if ( physMask & (1 << physPort) )
        {
            newMask |= (1 << cpi);
        }
    }

    *logMask = newMask;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmMapBitMaskPhysicalToLogical */




/*****************************************************************************/
/** fmMapBitMaskLogicalToLinkUpMask
 * \ingroup intSwitch
 *
 * \desc            maps logical port bit mask to link up mask
 *
 * \param[in]       switchPtr points to the switch state entry
 *
 * \param[in]       logMask is the logical port bit mask
 *
 * \param[out]      upMask contains the link up mask, i.e., only those ports
 *                  that were set in the logMask and are also in a link up 
 *                  state will be set in this mask.
 *
 * \return          FM_OK if request succeeded.
 *
 *****************************************************************************/
fm_status fmMapBitMaskLogicalToLinkUpMask(fm_switch * switchPtr,
                                          fm_uint32   logMask,
                                          fm_uint32 * upMask)
{
    fm_int      limit;
    fm_int      cpi;
    fm_int      port;
    fm_uint32   um = 0;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw = %d, logMask = 0x%08x, "
                         "upMask = %p\n",
                         switchPtr->switchNumber,
                         logMask,
                         (void *) upMask);

    limit = switchPtr->numCardinalPorts;
    if (limit > (fm_int)(sizeof(logMask) * 8))
    {
        limit = sizeof(logMask) * 8;
    }

    for (cpi = 0 ; cpi < limit ; cpi++)
    {
        port = switchPtr->cardinalPortInfo.portMap[cpi].logPort;

        /* CPU port is always available to send to */
        if ( switchPtr->portTable[port]->linkUp ||
             switchPtr->portTable[port]->isPortForceUp ||
             port == switchPtr->cpuPort)
        {
            um |= (1 << cpi);
        }
    }

    *upMask = logMask & um;

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fmMapBitMaskLogicalToLinkUpMask */




/*****************************************************************************/
/** fmFindNextPortInMask
 * \ingroup intSwitch
 *
 * \desc            Finds the next port with its bit set in a port bit mask.
 * 
 * \note            This function operates directly on the bits in the mask.
 *                  It does not perform port number translation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mask is the port bit mask to check.
 *
 * \param[in]       firstBit is the first port (bit) number to be checked.
 *
 * \return          Port (bit) number or -1 if no more ports can be found.
 *
 *****************************************************************************/
fm_int fmFindNextPortInMask(fm_int sw, fm_uint32 mask, fm_int firstBit)
{
    fm_int  limit;
    fm_int  i;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d, mask=0x%08x, firstBit=%d\n",
                         sw,
                         mask,
                         firstBit);

    limit = GET_SWITCH_PTR(sw)->maxPhysicalPort + 1;
    if (limit > (fm_int)(sizeof(mask) * 8))
    {
        limit = sizeof(mask) * 8;
    }

    for (i = firstBit ; i < limit ; i++)
    {
        if ( mask & (1 << i) )
        {
            FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, i, "%d\n", i);
        }
    }

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, -1, "-1\n");

}   /* end fmFindNextPortInMask */




/*****************************************************************************/
/** fmFindNextPowerOf2
 * \ingroup intSwag
 *
 * \desc            Returns the number that is a power of 2 equal to or
 *                  greater than the value provided.
 *
 * \param[in]       value contains the value for which the next power of
 *                  2 is desired.
 *
 * \return          value if value is a power of 2, otherwise the next
 *                  power of 2 greater than value.
 *
 *****************************************************************************/
fm_uint fmFindNextPowerOf2(fm_uint value)
{
    fm_uint power2 = 1;
    fm_uint temp   = value;

    while (temp > 0)
    {
        temp   >>= 1;
        power2 <<= 1;
    }

    if ( (value << 1) == power2 )
    {
        power2 = value;
    }

    return power2;

}   /* end fmFindNextPowerOf2 */




/*****************************************************************************/
/** fmLoadPortRemapTable
 * \ingroup intSwitch
 *
 * \desc            Load a new port mapping table from a given file.
 *
 * \param[in]       filename is the file name.
 *
 * \param[in]       portTable is ptr to table where to store port read from file
 *
 * \param[in]       origTable is ptr to the original port mapping table
 *
 * \param[in]       maxport is number of entries in portTable
 *
 * \return          FM_OK or FM_FAIL if unable to read table from file
 *
 *****************************************************************************/
fm_status fmLoadPortRemapTable(fm_char *filename,
                               fm_int *portTable,
                               fm_int *origTable,
                               fm_int maxport)
{
    FILE       *fp;
    fm_char    *pch;
    fm_char     line[200];
    fm_int      i;
    fm_int      j;
    fm_int      portNum;
    fm_char *   context;
    fm_uint     s1max;
    fm_uint     lineLeft;

    if ((fp = fopen(filename,"r")) == NULL)
    {
        FM_LOG_PRINT("cannot open file %s \n", filename);
        return(FM_FAIL);
    }

    /* Display the original port mapping table. */
    FM_CLEAR(line);
    lineLeft = sizeof(line);
    FM_SPRINTF_S(line, lineLeft, "\noriginalTable[%d,", origTable[0]);
    i = strlen(line);
    lineLeft -= i;
    pch = &line[i];

    for (i = 1 ; i < maxport ; i++)
    {
        FM_SPRINTF_S(pch, lineLeft, "%d,",origTable[i]);
        j = strlen(pch);
        lineLeft -= j;
        pch += j;
    }

    /* Reposition back to the trailing comma, then overwrite it */
    --pch;
    *pch = ']';
    FM_LOG_PRINT("%s\n", line);
 
    while ( fgets(line,200,fp) != NULL ) 
    {
        if (strstr(line,"mappingTable") != NULL)
        {
            FM_LOG_PRINT("%s\n", line);
            if ((pch = strchr(line,'[')) != NULL)
            {
                i       = 0;
                context = NULL;
                s1max   = RSIZE_MAX - 1;
                pch     = FM_STRTOK_S(++pch, &s1max, " ,", &context);

                while (pch != NULL && i < maxport)
                {
                    portNum = atoi(pch);
                    if (portNum >= 0 && portNum < maxport)
                    {
                        portTable[i++] = portNum;
                        pch = FM_STRTOK_S(NULL, &s1max, " ,]", &context);
                    }
                    else
                    {
                        FM_LOG_PRINT("*** Invalid port number %d ***\n",portNum);
                        fclose(fp);
                        return(FM_FAIL);
                    }
                }
            }
        }
    }

    fclose(fp);

    return FM_OK;

}   /* end fmLoadPortRemapTable */




/*****************************************************************************/
/** fmGetPartNumberMaxPort
 * \ingroup intSwitch
 *
 * \desc            Returns the number of the last physical port for a given
 *                  part number
 *
 * \param[in]       pn is the part number
 * 
 * \param[out]      maxPort is the caller allocated storage where
 *                  the port number should be stored
 *
 * \return          FM_OK if successful
 *                  FM_ERR_NOT_FOUND if the part number does not exist
 *
 *****************************************************************************/
fm_status fmGetPartNumberMaxPort(fm_switchPartNum pn, fm_int *maxPort)
{
    fm_status status = FM_OK;

    switch (pn)
    {
        /* FM6000 series */
        case FM_SWITCH_PART_NUM_FM6232:
        case FM_SWITCH_PART_NUM_FM6248:
        case FM_SWITCH_PART_NUM_FM6264:
        case FM_SWITCH_PART_NUM_FM6316:
        case FM_SWITCH_PART_NUM_FM6324:
        case FM_SWITCH_PART_NUM_FM6332:
        case FM_SWITCH_PART_NUM_FM6348:
        case FM_SWITCH_PART_NUM_FM6364:
        case FM_SWITCH_PART_NUM_FM6372:
        /* FM7000 series */
        case FM_SWITCH_PART_NUM_FM7232:
        case FM_SWITCH_PART_NUM_FM7248:
        case FM_SWITCH_PART_NUM_FM7264:
        case FM_SWITCH_PART_NUM_FM7316:
        case FM_SWITCH_PART_NUM_FM7324:
        case FM_SWITCH_PART_NUM_FM7332:
        case FM_SWITCH_PART_NUM_FM7348:
        case FM_SWITCH_PART_NUM_FM7364:
        case FM_SWITCH_PART_NUM_FM7372:
            *maxPort = 75;
            break;

        /* FM10000 series */
        case FM_SWITCH_PART_NUM_FM10124:
        case FM_SWITCH_PART_NUM_FM10136:
        case FM_SWITCH_PART_NUM_FM10148:
        case FM_SWITCH_PART_NUM_FM10424:
        case FM_SWITCH_PART_NUM_FM10440:
            *maxPort = 47;
            break;

        case FM_SWITCH_PART_NUM_FM2224:
        case FM_SWITCH_PART_NUM_FM2112:
        case FM_SWITCH_PART_NUM_FM3224:
        case FM_SWITCH_PART_NUM_FM3112:
        case FM_SWITCH_PART_NUM_FM4224:
        case FM_SWITCH_PART_NUM_FM4112:
            *maxPort = 24;
            break;

        case FM_SWITCH_PART_NUM_FM2220:
        case FM_SWITCH_PART_NUM_FM3220:
        case FM_SWITCH_PART_NUM_FM4220:
            *maxPort = 20;
            break;

        case FM_SWITCH_PART_NUM_FM3410:
        case FM_SWITCH_PART_NUM_FM4410:
            *maxPort = 18;
            break;

        case FM_SWITCH_PART_NUM_FM2212:
        case FM_SWITCH_PART_NUM_FM3212:
        case FM_SWITCH_PART_NUM_FM4212:
            *maxPort = 12;
            break;

        case FM_SWITCH_PART_NUM_FM2104:
        case FM_SWITCH_PART_NUM_FM3104:
        case FM_SWITCH_PART_NUM_FM4104:
            *maxPort = 10;
            break;

        case FM_SWITCH_PART_NUM_FM2208:
        case FM_SWITCH_PART_NUM_FM3208:
        case FM_SWITCH_PART_NUM_FM4208:
            *maxPort = 8;
            break;

        case FM_SWITCH_PART_NUM_FM2103:
        case FM_SWITCH_PART_NUM_FM3103:
        case FM_SWITCH_PART_NUM_FM4103:
            *maxPort = 6;
            break;

        case FM_SWITCH_PART_NUM_UNKNOWN:
            *maxPort = 0;
            break;

        default:
            status = FM_ERR_NOT_FOUND;
    }

    return status;

}   /* end fmGetPartNumberMaxPort */



/*****************************************************************************/
/** fmBitArrayToBitMask
 * \ingroup intPort
 *
 * \desc            Converts a bit array to a bitmask.
 *
 * \param[in]       bitArray points to the bit array to be converted.
 *
 * \param[out]      bitMask points to the caller-supplied location where the
 *                  converted bit array is to be stored.
 *
 * \param[in]       numBits is the maximum number of bits in the mask.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmBitArrayToBitMask(fm_bitArray * bitArray,
                              fm_uint32 *   bitMask,
                              fm_int        numBits)
{
    fm_int      bitNo;
    fm_status   err = FM_OK;

    if (numBits > (fm_int)(sizeof(*bitMask) * 8))
    {
        numBits = sizeof(*bitMask) * 8;
    }

    *bitMask = 0;
    bitNo = 0;

    for ( ; ; )
    {
        err = fmFindBitInBitArray(bitArray, bitNo, TRUE, &bitNo);
        if (err != FM_OK || bitNo < 0 || bitNo >= numBits)
        {
            break;
        }
        *bitMask |= (1 << bitNo);
        bitNo++;
    }

    return err;

}   /* end fmBitArrayToBitMask */




/*****************************************************************************/
/** fmFindNextBitInMask
 * \ingroup intSwitch
 *
 * \desc            Finds the next bit set in a bit mask.
 * 
 * \note            This function operates directly on the bits in the mask.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mask is the bit mask to check.
 *
 * \param[in]       firstBit is the first bit number to be checked.
 *
 * \return          Bit number or -1 if no more bits can be found.
 *
 *****************************************************************************/
fm_int fmFindNextBitInMask(fm_int sw, fm_uint32 mask, fm_int firstBit)
{
    fm_int  limit;
    fm_int  i;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d, mask=0x%08x, firstBit=%d\n",
                         sw,
                         mask,
                         firstBit);

    limit = sizeof(mask) * 8;

    for (i = firstBit ; i < limit ; i++)
    {
        if ( mask & (1 << i) )
        {
            FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, i, "%d\n", i);
        }
    }

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, -1, "-1\n");

}   /* end fmFindNextBitInMask */




/*****************************************************************************/
/** fmClearBitInMask
 * \ingroup intSwitch
 *
 * \desc            Clears a single bit in a bit mask.
 * 
 * \note            This function operates directly on the bits in the mask.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      mask is a pointer to the bit mask.
 *
 * \param[in]       bitPosition is the bit number to be cleared.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_INDEX if bit position is
 *                  out of mask range.
 *
 *****************************************************************************/
fm_status fmClearBitInMask(fm_int sw, fm_uint32 *mask, fm_int bitPosition)
{
    fm_status  status;
    fm_uint32  clearMask;
    fm_int     limit;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d, mask=%p, bitPosition=%d\n",
                         sw,
                         (void *) mask,
                         bitPosition);

    status = FM_OK;

    limit = sizeof(*mask) * 8;

    if (bitPosition >= limit)
    {
        status = FM_ERR_INVALID_INDEX;
        FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, status, "err=%d\n", status);
    }

    clearMask = ~(1 << bitPosition);

    *mask &= clearMask;

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_SWITCH, status, "err=%d\n", status);

}   /* end fmClearBitInMask */
